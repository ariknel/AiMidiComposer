#include "StemStrip.h"
#include "Colours.h"
#include "../midi/DragSourceWindows.h"

namespace AIMC {

StemStrip::StemStrip(const Stem& s) : m_stem(s)
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setTooltip("Drag into your DAW");
}

void StemStrip::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(2.f);
    auto bg = m_hovered ? Colours::stemChipHover : Colours::stemChipBg;
    g.setColour(bg);
    g.fillRoundedRectangle(r, 6.f);

    g.setColour(Colours::accent);
    g.fillEllipse(r.getX() + 8.f, r.getCentreY() - 4.f, 8.f, 8.f);

    g.setColour(Colours::textPrimary);
    g.setFont(juce::Font(juce::FontOptions(13.f).withStyle("SemiBold")));
    auto textArea = r.withTrimmedLeft(24.f).withTrimmedRight(4.f);
    g.drawFittedText(juce::String(m_stem.instrumentName),
                     textArea.toNearestInt(), juce::Justification::centredLeft, 1);

    g.setColour(Colours::textSecondary);
    g.setFont(juce::Font(juce::FontOptions(10.f)));
    auto meta = juce::String(m_stem.noteCount) + " notes | "
              + juce::String(m_stem.durationSeconds, 1) + "s";
    g.drawFittedText(meta,
                     textArea.removeFromBottom(14).toNearestInt(),
                     juce::Justification::bottomLeft, 1);
}

void StemStrip::mouseDown(const juce::MouseEvent&)
{
    m_dragStarted = false;
}

void StemStrip::mouseDrag(const juce::MouseEvent& e)
{
    if (m_dragStarted) return;
    if (e.getDistanceFromDragStart() < 8) return;
    if (! m_stem.midiFile.existsAsFile()) return;

    m_dragStarted = true;
    juce::StringArray paths;
    paths.add(m_stem.midiFile.getFullPathName());
    WinFileDragSource::performDrag(paths);
}

void StemStrip::mouseEnter(const juce::MouseEvent&) { m_hovered = true;  repaint(); }
void StemStrip::mouseExit (const juce::MouseEvent&) { m_hovered = false; repaint(); }

} // namespace AIMC
