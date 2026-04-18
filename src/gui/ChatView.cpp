#include "ChatView.h"
#include "Colours.h"

namespace AIMC {

ChatView::ChatView(ChatHistory& h) : m_history(h)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_content, false);
    m_viewport.setScrollBarsShown(true, false);
    m_viewport.setScrollBarThickness(8);
    refresh();
}

void ChatView::refresh()
{
    m_content.bubbles.clear();
    for (const auto& m : m_history.messages()) {
        auto b = std::make_unique<MessageBubble>(m);
        m_content.addAndMakeVisible(*b);
        m_content.bubbles.push_back(std::move(b));
    }
    resized();
    scrollToBottom();
}

void ChatView::resized()
{
    m_viewport.setBounds(getLocalBounds());

    const int contentWidth = juce::jmax(200, getWidth() - 16);
    int totalH = 0;
    for (auto& b : m_content.bubbles)
        totalH += b->computePreferredHeight(contentWidth) + 10;

    m_content.setSize(contentWidth, juce::jmax(totalH, getHeight()));
    m_content.resized();
}

void ChatView::Content::resized()
{
    int y = 8;
    for (auto& b : bubbles) {
        int h = b->computePreferredHeight(getWidth());
        b->setBounds(0, y, getWidth(), h);
        y += h + 10;
    }
}

void ChatView::scrollToBottom()
{
    m_viewport.setViewPosition(0, juce::jmax(0, m_content.getHeight() - m_viewport.getHeight()));
}

} // namespace AIMC
