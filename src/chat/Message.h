#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <string>

namespace AIMC {

/** A single MIDI stem belonging to an assistant message. */
struct Stem {
    std::string  instrumentName;   // "Piano", "Bass", "Drums" etc.
    int          generalMidiProgram = 0;     // 0-127 GM program
    int          noteCount = 0;
    double       durationSeconds = 0.0;
    juce::File   midiFile;                   // absolute path on disk
};

enum class MessageRole { User, Assistant, System };

/** One turn in the conversation. */
struct Message {
    MessageRole         role = MessageRole::User;
    juce::String        text;                 // user prompt, or assistant summary
    juce::int64         unixTimeMs = 0;

    // Populated only for assistant messages that produced MIDI:
    juce::File          fullMidiFile;         // combined multitrack .mid
    std::vector<Stem>   stems;

    // Generation metadata (assistant only):
    juce::String        detectedKey;
    juce::String        detectedTempo;
    juce::String        detectedTimeSig;
    bool                isError = false;
    juce::String        errorText;

    Message() = default;
    Message(MessageRole r, juce::String t)
        : role(r), text(std::move(t)),
          unixTimeMs(juce::Time::currentTimeMillis()) {}
};

} // namespace AIMC
