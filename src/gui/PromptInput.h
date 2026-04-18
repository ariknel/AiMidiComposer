#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace AIMC {

/** Bottom bar: multi-line input + Send button. Enter submits; Shift+Enter
    inserts a newline.
    Typing is always allowed so users can compose while the model loads;
    only the Send button is gated by model readiness. */
class PromptInput : public juce::Component,
                    public juce::TextEditor::Listener
{
public:
    PromptInput();

    void resized() override;
    void paint(juce::Graphics&) override;
    void textEditorReturnKeyPressed(juce::TextEditor&) override;

    /** Enable/disable the Send button only. Typing is always allowed. */
    void setSendEnabled(bool shouldBeEnabled);

    /** Set the placeholder shown when the editor is empty (e.g. "Waiting..."). */
    void setPlaceholder(const juce::String& text);

    void grabInputFocus();
    void clearText();

    std::function<void(juce::String)> onSubmit;

private:
    juce::TextEditor  m_editor;
    juce::TextButton  m_send { "Send" };
    bool              m_sendEnabled = false;

    void submit();
};

} // namespace AIMC
