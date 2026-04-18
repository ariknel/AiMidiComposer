#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../chat/Message.h"

namespace AIMC {

/** Displays one MIDI stem as a draggable chip. Click-and-hold initiates a
    Windows OLE file drag so the user can drop into any DAW track. */
class StemStrip : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    explicit StemStrip(const Stem& s);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    Stem  m_stem;
    bool  m_hovered = false;
    bool  m_dragStarted = false;
};

} // namespace AIMC
