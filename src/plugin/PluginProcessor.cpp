#include "PluginProcessor.h"
#include "../inference/HttpSidecarBackend.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace AIMC {

// Forward-declared to avoid pulling the full PluginEditor.h into this TU.
// Defined in PluginEditor.cpp inside namespace AIMC.
juce::AudioProcessorEditor* createEditorFor(PluginProcessor& p);

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties())   // MIDI-effect style: no audio buses
    , m_apvts(*this, nullptr, "Params", createLayout())
{
    ensureSessionDir();
    auto sidecar = locateBundledSidecar();
    auto modelDir = resolveModelCacheDir();
    m_backend = std::make_unique<HttpSidecarBackend>(sidecar, modelDir);
    // The editor will call backend->start() so we don't eagerly consume
    // resources when the DAW is just scanning plugins.
}

PluginProcessor::~PluginProcessor()
{
    if (m_backend) m_backend->stop();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    // This plugin is purely a MIDI-generating tool driven by the UI.
    // We don't inject MIDI into the DAW timeline automatically; users drag
    // generated stems onto tracks. processBlock is a no-op.
    juce::ignoreUnused(buffer, midi);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return createEditorFor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto root = std::make_unique<juce::XmlElement>("AIMCState");

    // APVTS params
    if (auto stateTree = m_apvts.copyState().createXml())
        root->addChildElement(stateTree.release());

    // Chat history in XML (non-automatable - per pitfall doc)
    root->addChildElement(m_history.toXml().release());

    copyXmlToBinary(*root, dest);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    auto xml = getXmlFromBinary(data, size);
    if (! xml) return;

    if (auto* p = xml->getChildByName(m_apvts.state.getType()))
        m_apvts.replaceState(juce::ValueTree::fromXml(*p));

    if (auto* ch = xml->getChildByName("ChatHistory"))
        m_history.fromXml(*ch);
}

void PluginProcessor::ensureSessionDir()
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("AIMidiComposer").getChildFile("sessions");
    base.createDirectory();

    auto name = juce::Time::getCurrentTime().formatted("session_%Y%m%d_%H%M%S_")
              + juce::String::toHexString(juce::Random::getSystemRandom().nextInt());
    m_sessionDir = base.getChildFile(name);
    m_sessionDir.createDirectory();
}

juce::File PluginProcessor::locateBundledSidecar() const
{
    // The sidecar ships BUNDLED INSIDE the .vst3 folder structure:
    //   AI MIDI Composer.vst3/
    //     Contents/
    //       x86_64-win/
    //         AI MIDI Composer.vst3   <- the DLL
    //         sidecar/
    //           sidecar.exe           <- what we want
    //
    // CRITICAL: juce::File::currentExecutableFile returns the DAW process,
    // NOT the plugin DLL. We must use GetModuleHandleExA with the address
    // of one of our own functions to find the DLL's path on disk.

    juce::File pluginDll;

   #if JUCE_WINDOWS
    HMODULE hModule = nullptr;
    // Pass the address of this very method. GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
    // + UNCHANGED_REFCOUNT gives us the HMODULE without adjusting its refcount.
    BOOL ok = ::GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&locateBundledSidecarAddressTag),
        &hModule);

    if (ok && hModule != nullptr) {
        char path[MAX_PATH] = {};
        DWORD n = ::GetModuleFileNameA(hModule, path, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            pluginDll = juce::File(juce::String(path));
            DBG("[locate] plugin DLL path: " + pluginDll.getFullPathName());
        }
    }
   #endif

    // Fallback if GetModuleHandleEx failed
    if (! pluginDll.existsAsFile()) {
        pluginDll = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
        DBG("[locate] fallback to currentApplicationFile: " + pluginDll.getFullPathName());
    }

    auto pluginDir = pluginDll.getParentDirectory();

    // Primary location: inside the VST3 bundle next to the plugin DLL
    auto bundled = pluginDir.getChildFile("sidecar").getChildFile("sidecar.exe");
    DBG("[locate] trying bundled: " + bundled.getFullPathName());
    if (bundled.existsAsFile()) return bundled;

    // Secondary: project-relative for development (when running from build/)
    auto devPath = juce::File::getCurrentWorkingDirectory()
                       .getChildFile("sidecar").getChildFile("dist")
                       .getChildFile("sidecar").getChildFile("sidecar.exe");
    DBG("[locate] trying dev path: " + devPath.getFullPathName());
    if (devPath.existsAsFile()) return devPath;

    // Tertiary: legacy install path from older installer layouts
    auto programFiles = juce::File::getSpecialLocation(
        juce::File::globalApplicationsDirectory);
    auto legacy = programFiles.getChildFile("AIMidiComposer")
                              .getChildFile("sidecar")
                              .getChildFile("sidecar.exe");
    DBG("[locate] trying legacy: " + legacy.getFullPathName());
    if (legacy.existsAsFile()) return legacy;

    // Return the bundled path even if missing so the error message
    // tells the user exactly where the sidecar was expected to be.
    DBG("[locate] NOT FOUND anywhere. Reporting expected path: "
        + bundled.getFullPathName());
    return bundled;
}

// This function exists ONLY to provide a stable address that is guaranteed
// to live inside the plugin DLL. We use its address with GetModuleHandleEx
// to discover the DLL's on-disk path. Never called.
void PluginProcessor::locateBundledSidecarAddressTag() {}

juce::File PluginProcessor::resolveModelCacheDir() const
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("AIMidiComposer").getChildFile("models");
    base.createDirectory();
    return base;
}

} // namespace AIMC

// JUCE entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AIMC::PluginProcessor();
}
