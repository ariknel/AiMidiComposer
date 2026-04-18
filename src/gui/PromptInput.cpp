#include "PromptInput.h"
#include "Colours.h"

namespace AIMC {

PromptInput::PromptInput()
{
    m_editor.setMultiLine(true, true);
    m_editor.setReturnKeyStartsNewLine(false);    // Enter submits
    m_editor.setScrollbarsShown(true);
    m_editor.setPopupMenuEnabled(true);
    m_editor.setTextToShowWhenEmpty(
        "Describe the music you want... (e.g. 'cinematic orchestral piece in D minor, 3/4, slow and melancholic')",
        Colours::textMuted);
    m_editor.setColour(juce::TextEditor::backgroundColourId, Colours::bgAlt);
    m_editor.setColour(juce::TextEditor::textColourId,       Colours::textPrimary);
    m_editor.setFont(juce::Font(juce::FontOptions(14.f)));
    m_editor.addListener(this);
    addAndMakeVisible(m_editor);

    m_send.onClick = [this]{ submit(); };
    m_send.setEnabled(false);     // start disabled until model is ready
    m_send.setButtonText("Wait");
    addAndMakeVisible(m_send);
}

void PromptInput::paint(juce::Graphics& g)
{
    g.setColour(Colours::bg);
    g.fillAll();
    g.setColour(Colours::divider);
    g.drawHorizontalLine(0, 0.f, (float) getWidth());
}

void PromptInput::resized()
{
    auto r = getLocalBounds().reduced(12, 10);
    auto sendArea = r.removeFromRight(72);
    r.removeFromRight(8);
    m_editor.setBounds(r);
    m_send.setBounds(sendArea.withSizeKeepingCentre(72, 36));
}

void PromptInput::textEditorReturnKeyPressed(juce::TextEditor&)
{
    auto mods = juce::ModifierKeys::getCurrentModifiers();
    if (mods.isShiftDown()) {
        m_editor.insertTextAtCaret("\n");
    } else {
        submit();
    }
}

void PromptInput::submit()
{
    if (! m_sendEnabled) return;
    auto txt = m_editor.getText().trim();
    if (txt.isEmpty()) return;
    if (onSubmit) onSubmit(txt);
    m_editor.clear();
}

void PromptInput::setSendEnabled(bool on)
{
    m_sendEnabled = on;
    m_send.setEnabled(on);
    m_send.setButtonText(on ? "Send" : "Wait");
}

void PromptInput::setPlaceholder(const juce::String& text)
{
    m_editor.setTextToShowWhenEmpty(text, Colours::textMuted);
    m_editor.repaint();
}

void PromptInput::grabInputFocus() { m_editor.grabKeyboardFocus(); }
void PromptInput::clearText()      { m_editor.clear(); }

} // namespace AIMC
