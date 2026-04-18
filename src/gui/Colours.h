#pragma once
#include <juce_graphics/juce_graphics.h>

namespace AIMC::Colours {
    // ChatGPT-inspired dark palette
    const juce::Colour bg            { 0xff1c1c1e };
    const juce::Colour bgAlt         { 0xff2a2a2e };
    const juce::Colour divider       { 0xff3a3a3e };

    const juce::Colour textPrimary   { 0xffececf1 };
    const juce::Colour textSecondary { 0xff8e8ea0 };
    const juce::Colour textMuted     { 0xff6b6b78 };

    const juce::Colour userBubble    { 0xff343541 };
    const juce::Colour aiBubble      { 0xff40414f };
    const juce::Colour accent        { 0xff19c37d };  // OpenAI green
    const juce::Colour accentDim     { 0xff1e8a5f };
    const juce::Colour error         { 0xffef4146 };

    const juce::Colour stemChipBg    { 0xff4a4b5c };
    const juce::Colour stemChipHover { 0xff5a5b6c };
}
