#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace Params {
    constexpr std::string_view TEMPERATURE = "temperature";
    constexpr std::string_view TOP_P       = "top_p";
    constexpr std::string_view MAX_TOKENS  = "max_tokens";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    // Sampling temperature for MIDI-LLM. Lower = more deterministic.
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{Params::TEMPERATURE.data(), 1}, "Temperature",
        NormalisableRange<float>(0.1f, 1.5f, 0.01f), 0.9f));

    // Nucleus sampling. Paper recommends 0.98.
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{Params::TOP_P.data(), 1}, "Top-P",
        NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.98f));

    // Generation length cap (in AMT tokens - roughly 3 tokens per note).
    layout.add(std::make_unique<AudioParameterInt>(
        ParameterID{Params::MAX_TOKENS.data(), 1}, "Max Tokens",
        256, 4096, 2000));

    // NOTE: Chat history, sidecar URL, model path etc. are NOT automatable.
    // They go in XML state (getStateInformation / setStateInformation).

    return layout;
}
