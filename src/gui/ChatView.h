#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MessageBubble.h"
#include "../chat/ChatHistory.h"

namespace AIMC {

/** Scrollable list of chat bubbles. Rebuilds bubble components from a
    ChatHistory on refresh(). Autoscrolls to bottom on append. */
class ChatView : public juce::Component
{
public:
    explicit ChatView(ChatHistory& history);

    void refresh();                        // rebuild bubbles from history
    void resized() override;
    void scrollToBottom();

private:
    class Content : public juce::Component {
    public:
        void resized() override;                                    // lay out bubbles
        std::vector<std::unique_ptr<MessageBubble>> bubbles;
    };

    ChatHistory&            m_history;
    juce::Viewport          m_viewport;
    Content                 m_content;
};

} // namespace AIMC
