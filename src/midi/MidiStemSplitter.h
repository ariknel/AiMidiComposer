#pragma once
#include "../chat/Message.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace AIMC {

/** Parses the combined multitrack MIDI returned by the sidecar and writes one
    .mid file per instrument into `outDir`. Classification groups notes by
    (channel, GM program) pair. Drum channel (10) always becomes "Drums". */
class MidiStemSplitter
{
public:
    struct Options {
        juce::String baseFilename = "stem";     // e.g. "take01_piano.mid"
        juce::File   outDir;
        bool         writeCombined = true;      // also write combined file
    };

    static std::vector<Stem> split(const juce::MemoryBlock& combinedMidi,
                                   const Options& opt,
                                   juce::File* combinedOutPath = nullptr);

    /** Translate a GM program number to a friendly name. */
    static juce::String gmProgramName(int program, bool isDrumChannel);
};

} // namespace AIMC
