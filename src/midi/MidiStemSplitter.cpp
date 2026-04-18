#include "MidiStemSplitter.h"

namespace AIMC {

juce::String MidiStemSplitter::gmProgramName(int program, bool isDrumChannel)
{
    if (isDrumChannel) return "Drums";

    // General MIDI Level 1 instrument groups
    static const char* names[128] = {
        "Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano","Honky-tonk Piano",
        "Electric Piano 1","Electric Piano 2","Harpsichord","Clavinet",
        "Celesta","Glockenspiel","Music Box","Vibraphone",
        "Marimba","Xylophone","Tubular Bells","Dulcimer",
        "Drawbar Organ","Percussive Organ","Rock Organ","Church Organ",
        "Reed Organ","Accordion","Harmonica","Tango Accordion",
        "Acoustic Guitar (nylon)","Acoustic Guitar (steel)","Electric Guitar (jazz)","Electric Guitar (clean)",
        "Electric Guitar (muted)","Overdriven Guitar","Distortion Guitar","Guitar Harmonics",
        "Acoustic Bass","Electric Bass (finger)","Electric Bass (pick)","Fretless Bass",
        "Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
        "Violin","Viola","Cello","Contrabass",
        "Tremolo Strings","Pizzicato Strings","Orchestral Harp","Timpani",
        "String Ensemble 1","String Ensemble 2","Synth Strings 1","Synth Strings 2",
        "Choir Aahs","Voice Oohs","Synth Voice","Orchestra Hit",
        "Trumpet","Trombone","Tuba","Muted Trumpet",
        "French Horn","Brass Section","Synth Brass 1","Synth Brass 2",
        "Soprano Sax","Alto Sax","Tenor Sax","Baritone Sax",
        "Oboe","English Horn","Bassoon","Clarinet",
        "Piccolo","Flute","Recorder","Pan Flute",
        "Blown Bottle","Shakuhachi","Whistle","Ocarina",
        "Lead 1 (square)","Lead 2 (sawtooth)","Lead 3 (calliope)","Lead 4 (chiff)",
        "Lead 5 (charang)","Lead 6 (voice)","Lead 7 (fifths)","Lead 8 (bass + lead)",
        "Pad 1 (new age)","Pad 2 (warm)","Pad 3 (polysynth)","Pad 4 (choir)",
        "Pad 5 (bowed)","Pad 6 (metallic)","Pad 7 (halo)","Pad 8 (sweep)",
        "FX 1 (rain)","FX 2 (soundtrack)","FX 3 (crystal)","FX 4 (atmosphere)",
        "FX 5 (brightness)","FX 6 (goblins)","FX 7 (echoes)","FX 8 (sci-fi)",
        "Sitar","Banjo","Shamisen","Koto",
        "Kalimba","Bag pipe","Fiddle","Shanai",
        "Tinkle Bell","Agogo","Steel Drums","Woodblock",
        "Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
        "Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet",
        "Telephone Ring","Helicopter","Applause","Gunshot"
    };
    if (program < 0 || program > 127) return "Unknown";
    return names[program];
}

static juce::String sanitizeForFilename(const juce::String& s)
{
    juce::String out;
    for (auto c : s) {
        if (juce::CharacterFunctions::isLetterOrDigit(c) || c == '-' || c == '_')
            out << c;
        else if (c == ' ')
            out << '_';
    }
    return out.isEmpty() ? "stem" : out;
}

std::vector<Stem> MidiStemSplitter::split(const juce::MemoryBlock& combinedMidi,
                                          const Options& opt,
                                          juce::File* combinedOutPath)
{
    std::vector<Stem> out;
    opt.outDir.createDirectory();

    juce::MemoryInputStream is(combinedMidi, false);
    juce::MidiFile mf;
    if (! mf.readFrom(is)) return out;

    // Optionally write combined MIDI verbatim
    if (opt.writeCombined) {
        auto combinedFile = opt.outDir.getChildFile(opt.baseFilename + "_full.mid");
        combinedFile.replaceWithData(combinedMidi.getData(), combinedMidi.getSize());
        if (combinedOutPath) *combinedOutPath = combinedFile;
    }

    const short timeFormat = mf.getTimeFormat();
    const int   numTracks  = mf.getNumTracks();

    // Group notes by (channel, program). Channel 10 (0-indexed 9) is drums.
    // Key: channel*256 + program  (drums: program forced to 0)
    struct Bucket {
        juce::MidiMessageSequence seq;
        int    channel = 0;
        int    program = 0;
        bool   isDrums = false;
        int    noteCount = 0;
    };
    std::map<int, Bucket> buckets;

    for (int t = 0; t < numTracks; ++t) {
        const auto* track = mf.getTrack(t);
        if (track == nullptr) continue;

        // Running program-per-channel within this track
        int currentProgram[16] = {};
        std::fill_n(currentProgram, 16, 0);

        for (int i = 0; i < track->getNumEvents(); ++i) {
            const auto* ev = track->getEventPointer(i);
            const auto& msg = ev->message;
            int ch = msg.getChannel();   // 1..16, 0 for non-channel events
            if (ch < 1 || ch > 16) {
                // Meta / sysex events go to every bucket we've opened for this track.
                // Simplest: stash them into a "shared meta" bucket later? For now,
                // attach them to bucket 0 (they'll ride along).
                continue;
            }
            bool drums = (ch == 10);

            if (msg.isProgramChange()) {
                currentProgram[ch - 1] = msg.getProgramChangeNumber();
                continue; // don't enter notes bucket from this event
            }

            int prog = drums ? 0 : currentProgram[ch - 1];
            int key = ch * 256 + prog;

            auto& b = buckets[key];
            b.channel = ch;
            b.program = prog;
            b.isDrums = drums;
            b.seq.addEvent(msg, ev->message.getTimeStamp());
            if (msg.isNoteOn() && msg.getVelocity() > 0) ++b.noteCount;
        }
    }

    // Write each bucket as its own single-track MIDI file
    int idx = 0;
    for (auto& [key, b] : buckets) {
        if (b.noteCount == 0) continue;
        b.seq.updateMatchedPairs();

        juce::MidiFile outMf;
        outMf.setTicksPerQuarterNote(timeFormat > 0 ? timeFormat : 480);

        // Tempo track (track 0) preserving original tempo map
        juce::MidiMessageSequence tempoTrack;
        for (int t = 0; t < numTracks; ++t) {
            if (auto* tr = mf.getTrack(t)) {
                for (int i = 0; i < tr->getNumEvents(); ++i) {
                    const auto& m = tr->getEventPointer(i)->message;
                    if (m.isTempoMetaEvent() || m.isTimeSignatureMetaEvent())
                        tempoTrack.addEvent(m, m.getTimeStamp());
                }
            }
        }
        outMf.addTrack(tempoTrack);
        outMf.addTrack(b.seq);

        auto name = gmProgramName(b.program, b.isDrums);
        auto safe = sanitizeForFilename(name);
        auto outFile = opt.outDir.getChildFile(
            opt.baseFilename + "_" + juce::String(++idx).paddedLeft('0', 2)
            + "_" + safe + ".mid");

        juce::FileOutputStream fos(outFile);
        if (fos.openedOk()) {
            outMf.writeTo(fos);
            fos.flush();

            Stem s;
            s.instrumentName     = name.toStdString();
            s.generalMidiProgram = b.program;
            s.noteCount          = b.noteCount;
            s.midiFile           = outFile;

            // duration in seconds: last-note-end / ticks-per-beat / tempo
            double lastTick = b.seq.getEndTime();
            double tpq = timeFormat > 0 ? (double) timeFormat : 480.0;
            // Default to 120 BPM if no tempo meta
            double usPerQuarter = 500000.0;
            for (int i = 0; i < tempoTrack.getNumEvents(); ++i) {
                const auto& m = tempoTrack.getEventPointer(i)->message;
                if (m.isTempoMetaEvent()) {
                    usPerQuarter = m.getTempoSecondsPerQuarterNote() * 1.0e6;
                    break;
                }
            }
            s.durationSeconds = (lastTick / tpq) * (usPerQuarter / 1.0e6);
            out.push_back(std::move(s));
        }
    }

    return out;
}

} // namespace AIMC
