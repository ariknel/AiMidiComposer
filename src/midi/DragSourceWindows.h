#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace AIMC {

/** Windows-native file drag source. JUCE's built-in drag-and-drop does NOT
    deliver files to external applications (DAWs). This uses OLE IDataObject
    with CF_HDROP to perform a real Windows shell-style file drag. */
class WinFileDragSource
{
public:
    /** Begins a modal file drag operation. Returns true if drop was accepted.
        Must be called from the message thread in response to a mouse-down. */
    static bool performDrag(const juce::StringArray& absolutePaths);
};

} // namespace AIMC
