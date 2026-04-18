#include "LookAndFeel.h"

namespace AIMC {

LookAndFeel::LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, Colours::bg);
    setColour(juce::TextEditor::backgroundColourId,      Colours::bgAlt);
    setColour(juce::TextEditor::textColourId,            Colours::textPrimary);
    setColour(juce::TextEditor::outlineColourId,         Colours::divider);
    setColour(juce::TextEditor::focusedOutlineColourId,  Colours::accent);
    setColour(juce::TextEditor::highlightColourId,       Colours::accentDim);
    setColour(juce::TextEditor::highlightedTextColourId, Colours::textPrimary);
    setColour(juce::CaretComponent::caretColourId,       Colours::textPrimary);
    setColour(juce::TextButton::buttonColourId,          Colours::accent);
    setColour(juce::TextButton::textColourOnId,          juce::Colours::white);
    setColour(juce::TextButton::textColourOffId,         juce::Colours::white);
    setColour(juce::Label::textColourId,                 Colours::textPrimary);
    setColour(juce::ScrollBar::thumbColourId,            Colours::divider);
}

juce::Font LookAndFeel::getTextButtonFont(juce::TextButton&, int h)
{
    return juce::Font(juce::FontOptions((float) juce::jmin(14, h - 4)).withStyle("SemiBold"));
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                       const juce::Colour& bgCol,
                                       bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    auto c = bgCol;
    if (down) c = c.darker(0.15f);
    else if (hi) c = c.brighter(0.10f);
    if (! b.isEnabled()) c = c.withAlpha(0.4f);
    g.setColour(c);
    g.fillRoundedRectangle(r, 6.f);
}

void LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour(b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId
                                                : juce::TextButton::textColourOffId));
    g.setFont(getTextButtonFont(b, b.getHeight()));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

void LookAndFeel::fillTextEditorBackground(juce::Graphics& g, int w, int h, juce::TextEditor& te)
{
    g.setColour(te.findColour(juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle(0.f, 0.f, (float) w, (float) h, 8.f);
}

void LookAndFeel::drawTextEditorOutline(juce::Graphics& g, int w, int h, juce::TextEditor& te)
{
    auto c = te.hasKeyboardFocus(false) ? Colours::accent : Colours::divider;
    g.setColour(c);
    g.drawRoundedRectangle(0.5f, 0.5f, w - 1.f, h - 1.f, 8.f, 1.f);
}

} // namespace AIMC
