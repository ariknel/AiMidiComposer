#include "MessageBubble.h"
#include "Colours.h"

namespace AIMC {

MessageBubble::MessageBubble(const Message& m) : m_msg(m)
{
    rebuildStems();
}

void MessageBubble::rebuildStems()
{
    m_stemChips.clear();
    if (m_msg.role != MessageRole::Assistant) return;
    for (const auto& s : m_msg.stems) {
        auto chip = std::make_unique<StemStrip>(s);
        addAndMakeVisible(*chip);
        m_stemChips.push_back(std::move(chip));
    }
}

int MessageBubble::computePreferredHeight(int availableWidth)
{
    const int padding = 14;
    const int textWidth = juce::jmax(100, availableWidth - 2 * padding);

    juce::String body = m_msg.text;
    if (m_msg.isError) body = "Error: " + m_msg.errorText;

    m_glyphs.clear();
    juce::Font font(juce::FontOptions(14.f));
    m_glyphs.addFittedText(font, body,
                           0.f, 0.f, (float) textWidth, 10000.f,
                           juce::Justification::topLeft, 100);

    auto bounds = m_glyphs.getBoundingBox(0, -1, true);
    m_textHeight = (int) std::ceil(bounds.getHeight()) + 4;

    int total = padding + m_textHeight + padding;

    // Metadata row for assistant messages
    if (m_msg.role == MessageRole::Assistant
        && (m_msg.detectedKey.isNotEmpty()
            || m_msg.detectedTempo.isNotEmpty()
            || m_msg.detectedTimeSig.isNotEmpty())) {
        total += 22;
    }

    // Stem chips: 2 per row, each 38px tall
    if (! m_stemChips.empty()) {
        int rows = (int) std::ceil(m_stemChips.size() / 2.0);
        total += rows * 44 + 8;
    }

    return total;
}

void MessageBubble::paint(juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    const bool isUser = (m_msg.role == MessageRole::User);
    const bool isErr  = m_msg.isError;

    auto colour = isErr ? Colours::error.withAlpha(0.18f)
                        : (isUser ? Colours::userBubble : Colours::aiBubble);
    g.setColour(colour);
    g.fillRoundedRectangle(full, 10.f);

    if (isErr) {
        g.setColour(Colours::error);
        g.drawRoundedRectangle(full.reduced(0.5f), 10.f, 1.f);
    }

    // Role label
    const float labelH = 16.f;
    g.setColour(isUser ? Colours::textSecondary : Colours::accent);
    g.setFont(juce::Font(juce::FontOptions(11.f).withStyle("Bold")));
    g.drawFittedText(isUser ? "You" : "AI Composer",
                     (int) full.getX() + 14, (int) full.getY() + 8,
                     120, (int) labelH, juce::Justification::topLeft, 1);

    // Body text
    g.setColour(isErr ? Colours::error : Colours::textPrimary);
    g.saveState();
    g.addTransform(juce::AffineTransform::translation(full.getX() + 14.f,
                                                      full.getY() + 14.f + labelH + 2.f));
    m_glyphs.draw(g);
    g.restoreState();

    // Metadata row (key / tempo / time sig) for assistant
    if (m_msg.role == MessageRole::Assistant
        && (m_msg.detectedKey.isNotEmpty()
            || m_msg.detectedTempo.isNotEmpty()
            || m_msg.detectedTimeSig.isNotEmpty())) {
        int y = (int) (full.getY() + 14 + labelH + 2 + m_textHeight + 4);
        juce::String line;
        if (m_msg.detectedKey.isNotEmpty())     line << "Key: "   << m_msg.detectedKey  << "   ";
        if (m_msg.detectedTempo.isNotEmpty())   line << "Tempo: " << m_msg.detectedTempo << "   ";
        if (m_msg.detectedTimeSig.isNotEmpty()) line << "Time: "  << m_msg.detectedTimeSig;
        g.setColour(Colours::textMuted);
        g.setFont(juce::Font(juce::FontOptions(11.f)));
        g.drawFittedText(line, (int) full.getX() + 14, y,
                         (int) full.getWidth() - 28, 18,
                         juce::Justification::topLeft, 1);
    }
}

void MessageBubble::resized()
{
    if (m_stemChips.empty()) return;
    auto r = getLocalBounds().reduced(14, 14);

    const int labelAndText = 16 + 2 + m_textHeight + 4
        + ((m_msg.detectedKey.isNotEmpty()
            || m_msg.detectedTempo.isNotEmpty()
            || m_msg.detectedTimeSig.isNotEmpty()) ? 22 : 0);
    r.removeFromTop(labelAndText);

    const int chipW = (r.getWidth() - 8) / 2;
    const int chipH = 40;
    int row = 0, col = 0;
    for (auto& chip : m_stemChips) {
        int x = r.getX() + col * (chipW + 8);
        int y = r.getY() + row * (chipH + 4);
        chip->setBounds(x, y, chipW, chipH);
        if (++col >= 2) { col = 0; ++row; }
    }
}

} // namespace AIMC
