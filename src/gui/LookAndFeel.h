#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Colours.h"

namespace AIMC {

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& background,
                              bool highlighted, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool highlighted, bool down) override;

    void fillTextEditorBackground(juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline   (juce::Graphics&, int w, int h, juce::TextEditor&) override;
};

} // namespace AIMC
