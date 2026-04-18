#include "ChatHistory.h"

namespace AIMC {

void ChatHistory::append(Message msg)
{
    m_messages.push_back(std::move(msg));
    if (onChanged) onChanged();
}

void ChatHistory::clear()
{
    m_messages.clear();
    if (onChanged) onChanged();
}

juce::String ChatHistory::buildFlattenedPrompt(const juce::String& newUserPrompt) const
{
    // Strategy: walk history, accumulate musical constraints that should
    // carry forward (key, tempo, instrumentation, mood), then append the
    // new user ask. MIDI-LLM responds well to descriptive paragraphs.
    //
    // If no prior context, pass through verbatim.
    if (m_messages.empty())
        return newUserPrompt;

    juce::String context;
    int assistantTurns = 0;
    juce::String lastKey, lastTempo, lastTimeSig;
    std::vector<juce::String> lastInstruments;

    for (const auto& m : m_messages) {
        if (m.role == MessageRole::Assistant && ! m.isError) {
            ++assistantTurns;
            if (m.detectedKey.isNotEmpty())     lastKey     = m.detectedKey;
            if (m.detectedTempo.isNotEmpty())   lastTempo   = m.detectedTempo;
            if (m.detectedTimeSig.isNotEmpty()) lastTimeSig = m.detectedTimeSig;
            lastInstruments.clear();
            for (const auto& s : m.stems)
                lastInstruments.push_back(juce::String(s.instrumentName));
        }
    }

    if (assistantTurns == 0)
        return newUserPrompt;

    // Build a continuation directive MIDI-LLM understands:
    context << "Continuing from the previous composition";
    if (lastKey.isNotEmpty())     context << " in " << lastKey;
    if (lastTimeSig.isNotEmpty()) context << " with a " << lastTimeSig << " time signature";
    if (lastTempo.isNotEmpty())   context << " at a " << lastTempo << " tempo";
    if (! lastInstruments.empty()) {
        context << ", featuring ";
        for (size_t i = 0; i < lastInstruments.size(); ++i) {
            context << lastInstruments[i];
            if (i + 2 < lastInstruments.size()) context << ", ";
            else if (i + 1 < lastInstruments.size()) context << " and ";
        }
    }
    context << ". " << newUserPrompt;

    return context;
}

std::unique_ptr<juce::XmlElement> ChatHistory::toXml() const
{
    auto root = std::make_unique<juce::XmlElement>("ChatHistory");
    for (const auto& m : m_messages) {
        auto* e = root->createNewChildElement("Message");
        e->setAttribute("role", static_cast<int>(m.role));
        e->setAttribute("text", m.text);
        e->setAttribute("time", juce::String(static_cast<juce::int64>(m.unixTimeMs)));
        e->setAttribute("isError", static_cast<int>(m.isError));
        e->setAttribute("errorText", m.errorText);
        e->setAttribute("key", m.detectedKey);
        e->setAttribute("tempo", m.detectedTempo);
        e->setAttribute("timeSig", m.detectedTimeSig);
        e->setAttribute("fullMidi", m.fullMidiFile.getFullPathName());
        for (const auto& s : m.stems) {
            auto* se = e->createNewChildElement("Stem");
            se->setAttribute("instrument", juce::String(s.instrumentName));
            se->setAttribute("gm", s.generalMidiProgram);
            se->setAttribute("notes", s.noteCount);
            se->setAttribute("duration", s.durationSeconds);
            se->setAttribute("file", s.midiFile.getFullPathName());
        }
    }
    return root;
}

void ChatHistory::fromXml(const juce::XmlElement& xml)
{
    m_messages.clear();
    for (auto* e : xml.getChildWithTagNameIterator("Message")) {
        Message m;
        m.role       = static_cast<MessageRole>(e->getIntAttribute("role", 0));
        m.text       = e->getStringAttribute("text");
        m.unixTimeMs = e->getStringAttribute("time").getLargeIntValue();
        m.isError    = e->getBoolAttribute("isError", false);
        m.errorText  = e->getStringAttribute("errorText");
        m.detectedKey     = e->getStringAttribute("key");
        m.detectedTempo   = e->getStringAttribute("tempo");
        m.detectedTimeSig = e->getStringAttribute("timeSig");
        m.fullMidiFile = juce::File(e->getStringAttribute("fullMidi"));
        for (auto* se : e->getChildWithTagNameIterator("Stem")) {
            Stem s;
            s.instrumentName       = se->getStringAttribute("instrument").toStdString();
            s.generalMidiProgram   = se->getIntAttribute("gm", 0);
            s.noteCount            = se->getIntAttribute("notes", 0);
            s.durationSeconds      = se->getDoubleAttribute("duration", 0.0);
            s.midiFile             = juce::File(se->getStringAttribute("file"));
            m.stems.push_back(std::move(s));
        }
        m_messages.push_back(std::move(m));
    }
    if (onChanged) onChanged();
}

} // namespace AIMC
