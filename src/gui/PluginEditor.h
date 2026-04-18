#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../plugin/PluginProcessor.h"
#include "LookAndFeel.h"
#include "ChatView.h"
#include "PromptInput.h"

namespace AIMC {

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void onBackendStatus(const InferenceBackend::StatusUpdate& u);
    void submitPrompt(juce::String prompt);
    void setGenerating(bool on);
    void showStatusLine(const juce::String& s, bool isError = false);
    void showSettings(bool visible);
    void restartSidecar();

    PluginProcessor& m_proc;
    LookAndFeel      m_lnf;

    // -- Main UI --
    juce::Label      m_title;
    juce::Label      m_statusLine;
    double           m_progress = 0.0;
    juce::ProgressBar m_progressBar { m_progress };
    juce::TextButton m_gearButton;   // top-right, opens settings overlay

    ChatView         m_chat;
    PromptInput      m_input;

    // -- Settings overlay (shown when gear button clicked) --
    juce::Component  m_settingsPanel;
    juce::Label      m_settingsTitle;
    juce::Label      m_sidecarStatusLabel;  // shows current sidecar status
    juce::TextButton m_restartButton;       // kills + respawns sidecar
    juce::TextButton m_closeSettingsButton;
    juce::Label      m_logPathLabel;        // clickable path to sidecar.log
    bool             m_settingsVisible = false;

    // Cache last status for the settings panel
    juce::String     m_lastStatusText;
    bool             m_lastStatusIsError = false;

    bool             m_isGenerating = false;
    bool             m_isModelReady = false;

    juce::Component::SafePointer<PluginEditor> m_safe { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

juce::AudioProcessorEditor* createEditorFor(PluginProcessor& p);

} // namespace AIMC
