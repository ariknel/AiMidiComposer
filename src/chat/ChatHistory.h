#pragma once
#include "Message.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace AIMC {

/** Owns the full chat log for one plugin instance. Thread-safe for append;
    reads are expected from the message thread only. */
class ChatHistory
{
public:
    ChatHistory() = default;

    void append(Message msg);
    const std::vector<Message>& messages() const noexcept { return m_messages; }
    void clear();

    /** Build the prompt payload sent to the sidecar, including prior turns.
        MIDI-LLM has no native multi-turn instruction format, so we flatten
        history into one descriptive paragraph - this is how iterative
        refinement works. */
    juce::String buildFlattenedPrompt(const juce::String& newUserPrompt) const;

    /** Serialize to XML for persistence inside the plugin state. */
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml(const juce::XmlElement& xml);

    /** Listener for GUI to refresh after async updates. */
    std::function<void()> onChanged;

private:
    std::vector<Message> m_messages;
};

} // namespace AIMC
