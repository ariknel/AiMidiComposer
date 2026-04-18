#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>

namespace AIMC {

/** Manages the lifetime of the bundled Python sidecar process.
    - Resolves the sidecar binary path (bundled next to VST3).
    - Launches with --port and --model-dir args.
    - Polls /healthz to detect readiness.
    - Ensures child process dies when the plugin unloads
      (Windows Job Object so DAW crash still cleans up). */
class SidecarManager : private juce::Timer
{
public:
    SidecarManager();
    ~SidecarManager() override;

    struct Config {
        juce::File  sidecarExecutable;     // path to sidecar.exe (bundled)
        juce::File  modelDirectory;        // where to cache model weights
        int         port = 0;              // 0 = pick free port automatically
    };

    bool launch(const Config& cfg);
    void shutdown();

    bool isRunning() const;
    int  portInUse() const noexcept { return m_port; }
    juce::String baseUrl() const { return "http://127.0.0.1:" + juce::String(m_port); }

    /** Populated when launch() returns false. Cleared on successful launch. */
    juce::String lastError() const { return m_lastError; }

    std::function<void(const juce::String& line)> onLogLine;

private:
    void timerCallback() override;                      // poll subprocess / drain stdout
    static int pickFreePort();

    std::unique_ptr<juce::ChildProcess> m_proc;
    int             m_port = 0;
    juce::String    m_stdoutBuffer;
    juce::String    m_lastError;
    juce::String    m_sidecarPath;
    juce::int64     m_launchTimeMs = 0;
    bool            m_crashChecked = false;
    bool            m_readySeen = false;

   #if JUCE_WINDOWS
    void* m_jobHandle = nullptr;   // HANDLE to Windows Job Object
    bool  createJobAndAssign();
   #endif
};

} // namespace AIMC
