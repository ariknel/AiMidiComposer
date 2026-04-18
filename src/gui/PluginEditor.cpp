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

    // Gear button - top right corner, opens settings panel
    m_gearButton.setButtonText("...");
    m_gearButton.setTooltip("Sidecar settings");
    m_gearButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    m_gearButton.setColour(juce::TextButton::textColourOffId, Colours::textSecondary);
    m_gearButton.onClick = [this] { showSettings(! m_settingsVisible); };
    addAndMakeVisible(m_gearButton);

    // Settings panel (hidden until gear clicked)
    m_settingsPanel.setVisible(false);
    addAndMakeVisible(m_settingsPanel);

    m_settingsTitle.setText("Sidecar Settings", juce::dontSendNotification);
    m_settingsTitle.setFont(juce::Font(juce::FontOptions(13.f).withStyle("Bold")));
    m_settingsTitle.setColour(juce::Label::textColourId, Colours::textPrimary);
    m_settingsPanel.addAndMakeVisible(m_settingsTitle);

    m_sidecarStatusLabel.setText("Status: Starting...", juce::dontSendNotification);
    m_sidecarStatusLabel.setFont(juce::Font(juce::FontOptions(11.f)));
    m_sidecarStatusLabel.setColour(juce::Label::textColourId, Colours::textSecondary);
    m_sidecarStatusLabel.setJustificationType(juce::Justification::topLeft);
    m_settingsPanel.addAndMakeVisible(m_sidecarStatusLabel);

    m_restartButton.setButtonText("Restart Sidecar");
    m_restartButton.setColour(juce::TextButton::buttonColourId, Colours::accent);
    m_restartButton.setColour(juce::TextButton::textColourOffId, Colours::textPrimary);
    m_restartButton.onClick = [this] { restartSidecar(); };
    m_settingsPanel.addAndMakeVisible(m_restartButton);

    m_closeSettingsButton.setButtonText("Close");
    m_closeSettingsButton.setColour(juce::TextButton::buttonColourId, Colours::divider);
    m_closeSettingsButton.setColour(juce::TextButton::textColourOffId, Colours::textSecondary);
    m_closeSettingsButton.onClick = [this] { showSettings(false); };
    m_settingsPanel.addAndMakeVisible(m_closeSettingsButton);

    m_logPathLabel.setFont(juce::Font(juce::FontOptions(10.f)));
    m_logPathLabel.setColour(juce::Label::textColourId, Colours::textMuted);
    m_logPathLabel.setJustificationType(juce::Justification::bottomLeft);
    auto logPath = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("AIMidiComposer").getChildFile("sidecar.log");
    m_logPathLabel.setText("Log: " + logPath.getFullPathName(), juce::dontSendNotification);
    m_settingsPanel.addAndMakeVisible(m_logPathLabel);

    addAndMakeVisible(m_chat);
    addAndMakeVisible(m_input);

    p.getHistory().onChanged = [safe = m_safe]() {
        if (safe == nullptr) return;
        juce::MessageManager::callAsync([safe]() {
            if (safe != nullptr) { safe->m_chat.refresh(); }
        });
    };

    m_input.onSubmit = [this](juce::String txt) { submitPrompt(std::move(txt)); };

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
    g.setColour(Colours::divider);
    g.drawHorizontalLine(48, 0.f, (float) getWidth());

    if (m_settingsVisible) {
        g.setColour(Colours::bgAlt.withAlpha(0.97f));
        g.fillRect(m_settingsPanel.getBounds());
        g.setColour(Colours::divider);
        g.drawRect(m_settingsPanel.getBounds(), 1);
    }
}

void PluginEditor::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop(48);

    m_gearButton.setBounds(header.removeFromRight(40).reduced(6));
    m_title.setBounds(header.removeFromLeft(240).reduced(12, 0));

    auto statusArea = header.reduced(12, 8);
    if (m_progressBar.isVisible())
        m_progressBar.setBounds(statusArea.removeFromRight(120).withSizeKeepingCentre(120, 12));
    m_statusLine.setBounds(statusArea);

    auto bottom = r.removeFromBottom(120);
    m_input.setBounds(bottom);
    m_chat.setBounds(r);

    // Settings panel: overlays the right side of the content area.
    // Panel is positioned in EDITOR coordinates. Children are positioned
    // in PANEL-LOCAL coordinates (relative to the panel's top-left = 0,0).
    if (m_settingsVisible) {
        auto panelBounds = getLocalBounds()
            .removeFromRight(290)
            .withTrimmedTop(49);   // sit just below header divider
        m_settingsPanel.setBounds(panelBounds);

        // Now lay out children relative to panel origin (0,0)
        auto pr = juce::Rectangle<int>(0, 0, panelBounds.getWidth(), panelBounds.getHeight())
                      .reduced(16);
        m_settingsTitle.setBounds(pr.removeFromTop(28));
        pr.removeFromTop(8);
        m_sidecarStatusLabel.setBounds(pr.removeFromTop(90));
        pr.removeFromTop(12);
        m_restartButton.setBounds(pr.removeFromTop(34));
        pr.removeFromTop(8);
        m_closeSettingsButton.setBounds(pr.removeFromTop(28));

        // Log path label at the bottom of the panel
        auto logRow = juce::Rectangle<int>(0, 0, panelBounds.getWidth(), panelBounds.getHeight())
                          .reduced(16).removeFromBottom(28);
        m_logPathLabel.setBounds(logRow);
    }
}

void PluginEditor::showSettings(bool visible)
{
    m_settingsVisible = visible;
    m_settingsPanel.setVisible(visible);
    if (visible) {
        auto txt = m_lastStatusIsError
            ? "Status: Failed\n\n" + m_lastStatusText
            : "Status: " + m_lastStatusText;
        m_sidecarStatusLabel.setText(txt, juce::dontSendNotification);
        m_sidecarStatusLabel.setColour(juce::Label::textColourId,
            m_lastStatusIsError ? Colours::error : Colours::textSecondary);
    }
    resized();
    repaint();
}

void PluginEditor::restartSidecar()
{
    m_restartButton.setEnabled(false);
    m_restartButton.setButtonText("Restarting...");
    m_isModelReady = false;
    showStatusLine("Restarting sidecar...");
    m_sidecarStatusLabel.setText("Status: Restarting...", juce::dontSendNotification);

    if (auto* backend = m_proc.getBackend()) {
        backend->stop();
        backend->start([safe = m_safe](const InferenceBackend::StatusUpdate& u) {
            if (safe != nullptr) safe->onBackendStatus(u);
        });
    }

    juce::Timer::callAfterDelay(2000, [safe = m_safe] {
        if (safe != nullptr) {
            safe->m_restartButton.setEnabled(true);
            safe->m_restartButton.setButtonText("Restart Sidecar");
        }
    });
}

void PluginEditor::timerCallback()
{
    const bool canSend = m_isModelReady && ! m_isGenerating;
    m_input.setSendEnabled(canSend);
}

void PluginEditor::onBackendStatus(const InferenceBackend::StatusUpdate& u)
{
    using S = InferenceBackend::Status;
    m_isModelReady = (u.status == S::Ready);
    m_lastStatusIsError = (u.status == S::Failed);
    m_lastStatusText = u.message;

    if (m_settingsVisible) showSettings(true);

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
            m_input.setPlaceholder("Downloading AI model (~3 GB, one-time)...");
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
                "Describe the music you want... (e.g. 'cinematic orchestral piece in D minor')");
            m_progress = 1.0;
            m_progressBar.setVisible(false);
            break;
        case S::Failed: {
            auto firstLine = u.message.upToFirstOccurrenceOf("\n", false, false);
            showStatusLine(firstLine.isEmpty() ? juce::String("Failed") : firstLine, true);
            m_input.setPlaceholder("Local inference engine not available. See error in chat.");
            m_progressBar.setVisible(false);

            const auto& msgs = m_proc.getHistory().messages();
            bool alreadyLogged = (! msgs.empty()
                && msgs.back().role == MessageRole::Assistant
                && msgs.back().isError
                && msgs.back().errorText == u.message);
            if (! alreadyLogged) {
                Message err(MessageRole::Assistant, {});
                err.isError = true;
                err.errorText = u.message.isEmpty()
                    ? juce::String("Sidecar failed to start.")
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
    else    showStatusLine(m_isModelReady ? juce::String("Ready") : juce::String("Waiting..."));
}

void PluginEditor::submitPrompt(juce::String rawPrompt)
{
    if (! m_isModelReady || m_isGenerating) return;

    Message userMsg(MessageRole::User, rawPrompt);
    m_proc.getHistory().append(userMsg);

    auto flattened = m_proc.getHistory().buildFlattenedPrompt(rawPrompt);

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

        MidiStemSplitter::Options opt;
        auto stamp = juce::Time::getCurrentTime().formatted("take_%H%M%S");
        opt.baseFilename = stamp;
        opt.outDir       = m_proc.getSessionDir();

        juce::File combined;
        auto stems = MidiStemSplitter::split(r.combinedMidiBytes, opt, &combined);

        Message ai(MessageRole::Assistant,
                   r.assistantSummary.isEmpty()
                       ? juce::String("Here's your composition. Drag any stem into your DAW.")
                       : r.assistantSummary);
        ai.detectedKey     = r.detectedKey;
        ai.detectedTempo   = r.detectedTempo;
        ai.detectedTimeSig = r.detectedTimeSig;
        ai.fullMidiFile    = combined;
        ai.stems           = std::move(stems);
        m_proc.getHistory().append(ai);

        setGenerating(false);
    };

    auto onProgress = [](float, juce::String) {};
    m_proc.getBackend()->generate(req, onProgress, onDone);
}

juce::AudioProcessorEditor* createEditorFor(PluginProcessor& p)
{
    return new PluginEditor(p);
}

} // namespace AIMC
