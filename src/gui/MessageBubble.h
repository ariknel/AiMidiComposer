#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../chat/Message.h"
#include "StemStrip.h"
#include <memory>
#include <vector>

namespace AIMC {

/** A single chat bubble in the ChatView. Responsible only for rendering
    itself and owning its stem chip components. Height is computed from
    the wrapped text + number of stems. */
class MessageBubble : public juce::Component
{
public:
    explicit MessageBubble(const Message& m);

    int  computePreferredHeight(int availableWidth);
    void paint(juce::Graphics&) override;
    void resized() override;

    const Message& getMessage() const { return m_msg; }

private:
    Message m_msg;
    juce::GlyphArrangement m_glyphs;    // cached laid-out text
    int m_textHeight = 0;
    std::vector<std::unique_ptr<StemStrip>> m_stemChips;

    void rebuildStems();
};

} // namespace AIMC
