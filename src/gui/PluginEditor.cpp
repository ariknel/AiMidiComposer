#include "PluginEditor.h"
#include "Colours.h"
#include "../midi/MidiStemSplitter.h"

namespace AIMC {

PluginEditor::PluginEditor(PluginProcessor& p)
    : juce::AudioProcessorEditor(&p),
      m_proc(p),
      m_chat(p.getHistory()),
      m_input()
{
    setLookAndFeel(&m_lnf);

    m_title.setText("AI MIDI Composer", juce::dontSendNotification);
    m_title.setFont(juce::Font(juce::FontOptions(15.f).withStyle("Bold")));
    m_title.setColour(juce::Label::textColourId, Colours::textPrimary);
    m_title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_title);

    m_statusLine.setFont(juce::Font(juce::FontOptions(11.f)));
    m_statusLine.setColour(juce::Label::textColourId, Colours::textSecondary);
    m_statusLine.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(m_statusLine);

    m_progressBar.setPercentageDisplay(false);
    m_progressBar.setColour(juce::ProgressBar::foregroundColourId, Colours::accent);
    m_progressBar.setColour(juce::ProgressBar::backgroundColourId, Colours::divider);
    addChildComponent(m_progressBar);

    addAndMakeVisible(m_chat);
    addAndMakeVisible(m_input);

    // Chat history updates come from the processor; refresh when they do.
    p.getHistory().onChanged = [safe = m_safe]() {
        if (safe == nullptr) return;
        juce::MessageManager::callAsync([safe]() {
            if (safe != nullptr) { safe->m_chat.refresh(); }
        });
    };

    m_input.onSubmit = [this](juce::String txt) { submitPrompt(std::move(txt)); };

    // Kick off backend. Status updates arrive on message thread via callAsync.
    if (auto* backend = m_proc.getBackend()) {
        backend->start([safe = m_safe](const InferenceBackend::StatusUpdate& u) {
            if (safe != nullptr) safe->onBackendStatus(u);
        });
    }

    setSize(720, 640);
    setResizable(true, true);
    setResizeLimits(520, 400, 1600, 1200);
    startTimerHz(4);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    m_proc.getHistory().onChanged = nullptr;
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours::bg);
    // Header divider
    g.setColour(Colours::divider);
    g.drawHorizontalLine(48, 0.f, (float) getWidth());
}

void PluginEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop(48);

    m_title.setBounds(header.removeFromLeft(240).reduced(12, 0));

    auto statusArea = header.reduced(12, 8);
    if (m_progressBar.isVisible())
        m_progressBar.setBounds(statusArea.removeFromRight(120).withSizeKeepingCentre(120, 12));
    m_statusLine.setBounds(statusArea);

    auto bottom = r.removeFromBottom(120);
    m_input.setBounds(bottom);
    m_chat.setBounds(r);
}

void PluginEditor::timerCallback()
{
    // Keep send button in sync with readiness. Typing is always allowed.
    const bool canSend = m_isModelReady && ! m_isGenerating;
    m_input.setSendEnabled(canSend);
}

void PluginEditor::onBackendStatus(const InferenceBackend::StatusUpdate& u)
{
    using S = InferenceBackend::Status;
    m_isModelReady = (u.status == S::Ready);

    switch (u.status) {
        case S::NotStarted:
            showStatusLine("Idle");
            m_input.setPlaceholder("Starting up...");
            m_progressBar.setVisible(false);
            break;
        case S::Starting:
            showStatusLine("Starting sidecar...");
            m_input.setPlaceholder("Starting local inference engine...");
            m_progress = 0.0;
            m_progressBar.setVisible(true);
            break;
        case S::DownloadingModel:
            showStatusLine("Downloading model (" + juce::String((int)(u.progress01 * 100)) + "%)");
            m_input.setPlaceholder("Downloading AI model (~3 GB, one-time)... you can type your prompt in the meantime.");
            m_progress = u.progress01;
            m_progressBar.setVisible(true);
            break;
        case S::LoadingModel:
            showStatusLine("Loading model (" + juce::String((int)(u.progress01 * 100)) + "%)");
            m_input.setPlaceholder("Loading AI model into memory... almost ready.");
            m_progress = u.progress01;
            m_progressBar.setVisible(true);
            break;
        case S::Ready:
            showStatusLine("Ready");
            m_input.setPlaceholder(
                "Describe the music you want... (e.g. 'cinematic orchestral piece in D minor, 3/4, slow and melancholic')");
            m_progress = 1.0;
            m_progressBar.setVisible(false);
            break;
        case S::Failed: {
            // Short status line on top
            auto firstLine = u.message.upToFirstOccurrenceOf("\n", false, false);
            showStatusLine(firstLine.isEmpty() ? juce::String("Failed") : firstLine, true);
            m_input.setPlaceholder("Local inference engine not available. See error in chat.");
            m_progressBar.setVisible(false);

            // Append the full error as a system/error message in the chat so
            // the user sees the complete path and reason, not just "Failed".
            // Only add it once per failure (avoid spam if status fires twice).
            const auto& msgs = m_proc.getHistory().messages();
            bool alreadyLogged = (! msgs.empty()
                && msgs.back().role == MessageRole::Assistant
                && msgs.back().isError
                && msgs.back().errorText == u.message);
            if (! alreadyLogged) {
                Message err(MessageRole::Assistant, {});
                err.isError = true;
                err.errorText = u.message.isEmpty()
                    ? juce::String("Sidecar failed to start (no detail).")
                    : u.message;
                m_proc.getHistory().append(err);
            }
            break;
        }
    }
    resized();
}

void PluginEditor::showStatusLine(const juce::String& s, bool isError)
{
    m_statusLine.setText(s, juce::dontSendNotification);
    m_statusLine.setColour(juce::Label::textColourId,
                           isError ? Colours::error : Colours::textSecondary);
}

void PluginEditor::setGenerating(bool on)
{
    m_isGenerating = on;
    m_input.setSendEnabled(! on && m_isModelReady);
    if (on) showStatusLine("Generating...");
    else    showStatusLine(m_isModelReady ? juce::String("Ready") : juce::String("Waiting for model..."));
}

void PluginEditor::submitPrompt(juce::String rawPrompt)
{
    if (! m_isModelReady || m_isGenerating) return;

    // 1. Append user message immediately (optimistic UI)
    Message userMsg(MessageRole::User, rawPrompt);
    m_proc.getHistory().append(userMsg);

    // 2. Build the actual prompt (flattened with prior context)
    auto flattened = m_proc.getHistory().buildFlattenedPrompt(rawPrompt);

    // 3. Prepare generation params from APVTS
    InferenceBackend::Request req;
    req.prompt      = flattened;
    req.temperature = (float) *m_proc.getAPVTS().getRawParameterValue(Params::TEMPERATURE.data());
    req.topP        = (float) *m_proc.getAPVTS().getRawParameterValue(Params::TOP_P.data());
    req.maxTokens   = (int)  *m_proc.getAPVTS().getRawParameterValue(Params::MAX_TOKENS.data());

    setGenerating(true);

    auto onDone = [this, safe = m_safe](InferenceBackend::GenerationResult r) {
        if (safe == nullptr) return;

        if (! r.success) {
            Message err(MessageRole::Assistant, {});
            err.isError = true;
            err.errorText = r.errorMessage.isEmpty() ? juce::String("Unknown error") : r.errorMessage;
            m_proc.getHistory().append(err);
            setGenerating(false);
            return;
        }

        // Split combined MIDI into stems + save under session dir
        MidiStemSplitter::Options opt;
        auto stamp = juce::Time::getCurrentTime().formatted("take_%H%M%S");
        opt.baseFilename = stamp;
        opt.outDir       = m_proc.getSessionDir();

        juce::File combined;
        auto stems = MidiStemSplitter::split(r.combinedMidiBytes, opt, &combined);

        // Build assistant message
        Message ai(MessageRole::Assistant,
                   r.assistantSummary.isEmpty()
                       ? juce::String("Here's a composition matching your prompt. "
                                      "Drag any stem into your DAW.")
                       : r.assistantSummary);
        ai.detectedKey     = r.detectedKey;
        ai.detectedTempo   = r.detectedTempo;
        ai.detectedTimeSig = r.detectedTimeSig;
        ai.fullMidiFile    = combined;
        ai.stems           = std::move(stems);
        m_proc.getHistory().append(ai);

        setGenerating(false);
    };

    auto onProgress = [](float, juce::String) { /* reserved for streaming */ };

    m_proc.getBackend()->generate(req, onProgress, onDone);
}

// -- Factory referenced by PluginProcessor::createEditor() ---------------------
juce::AudioProcessorEditor* createEditorFor(PluginProcessor& p)
{
    return new PluginEditor(p);
}

} // namespace AIMC
