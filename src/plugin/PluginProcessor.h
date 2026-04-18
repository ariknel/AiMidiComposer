#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "../chat/ChatHistory.h"
#include "../inference/InferenceBackend.h"

namespace AIMC {

/** Top-level plugin processor.
    - Holds the InferenceBackend (Python sidecar v1).
    - Holds per-instance ChatHistory.
    - Caches a "pending MIDI file to emit" for the next processBlock
      so the user can drag the generated MIDI directly onto the current track. */
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    // -- Boilerplate -----------------------------------------------------------
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AI MIDI Composer"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // -- Exposed to editor -----------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS()   { return m_apvts; }
    ChatHistory&       getHistory()                  { return m_history; }
    InferenceBackend*  getBackend()                  { return m_backend.get(); }
    juce::File         getSessionDir() const         { return m_sessionDir; }

private:
    juce::AudioProcessorValueTreeState m_apvts;
    ChatHistory                        m_history;
    std::unique_ptr<InferenceBackend>  m_backend;
    juce::File                         m_sessionDir;

    void ensureSessionDir();
    juce::File locateBundledSidecar() const;
    juce::File resolveModelCacheDir() const;

    // Address-tag helper: used by locateBundledSidecar to discover the
    // plugin DLL's path via GetModuleHandleEx. Intentionally does nothing.
    static void locateBundledSidecarAddressTag();
};

} // namespace AIMC
