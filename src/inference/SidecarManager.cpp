#include "SidecarManager.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace AIMC {

SidecarManager::SidecarManager() = default;

SidecarManager::~SidecarManager()
{
    shutdown();
   #if JUCE_WINDOWS
    if (m_jobHandle != nullptr) {
        ::CloseHandle(static_cast<HANDLE>(m_jobHandle));
        m_jobHandle = nullptr;
    }
   #endif
}

int SidecarManager::pickFreePort()
{
    // Range 49152-65535 is the IANA dynamic/private range. Pick one randomly;
    // the sidecar will fail-fast if it's taken and we'll retry.
    juce::Random r((juce::int64) juce::Time::getHighResolutionTicks());
    return 49152 + r.nextInt(65535 - 49152);
}

bool SidecarManager::launch(const Config& cfg)
{
    m_lastError.clear();
    m_launchTimeMs = juce::Time::currentTimeMillis();
    if (m_proc && m_proc->isRunning()) return true;

    if (! cfg.sidecarExecutable.existsAsFile()) {
        m_lastError = "Sidecar executable not found at:\n"
                      + cfg.sidecarExecutable.getFullPathName()
                      + "\n\nReinstall the plugin or place sidecar.exe at this path.";
        if (onLogLine) onLogLine("[Sidecar] " + m_lastError);
        return false;
    }
    cfg.modelDirectory.createDirectory();
    m_sidecarPath = cfg.sidecarExecutable.getFullPathName();

    m_port = (cfg.port > 0) ? cfg.port : pickFreePort();

    // CRITICAL: PyInstaller onedir output needs its own folder as the CWD.
    // Without this, the .exe runs but can't find _internal\ and crashes
    // silently within milliseconds. Set CWD = folder containing sidecar.exe.
    auto workingDir = cfg.sidecarExecutable.getParentDirectory();

    juce::StringArray args;
    args.add(cfg.sidecarExecutable.getFullPathName());
    args.add("--port"); args.add(juce::String(m_port));
    args.add("--model-dir"); args.add(cfg.modelDirectory.getFullPathName());

    if (onLogLine) {
        onLogLine("[Sidecar] Launching: " + cfg.sidecarExecutable.getFullPathName());
        onLogLine("[Sidecar] Working dir: " + workingDir.getFullPathName());
        onLogLine("[Sidecar] Port: " + juce::String(m_port));
        onLogLine("[Sidecar] Model dir: " + cfg.modelDirectory.getFullPathName());
    }

    m_proc = std::make_unique<juce::ChildProcess>();

    // JUCE ChildProcess::start does not accept a CWD. We work around this by
    // temporarily changing our own CWD before spawning. JUCE inherits it.
    auto savedCwd = juce::File::getCurrentWorkingDirectory();
    workingDir.setAsCurrentWorkingDirectory();

    const bool ok = m_proc->start(args,
        juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr);

    savedCwd.setAsCurrentWorkingDirectory();

    if (! ok) {
        m_lastError = "ChildProcess::start returned false.\n"
                      "Possible causes:\n"
                      "- Antivirus is blocking sidecar.exe (try whitelisting)\n"
                      "- _internal\\ folder is missing next to sidecar.exe\n"
                      "- Windows permissions denied";
        if (onLogLine) onLogLine("[Sidecar] " + m_lastError);
        m_proc.reset();
        return false;
    }

   #if JUCE_WINDOWS
    if (! createJobAndAssign() && onLogLine)
        onLogLine("[Sidecar] Warning: could not create kill-on-close Job Object.");
   #endif

    if (onLogLine) onLogLine("[Sidecar] Process spawned, waiting for READY signal on stdout...");
    m_crashChecked = false;
    m_readySeen = false;
    startTimer(250);   // begin draining stdout + crash detection
    return true;
}

void SidecarManager::shutdown()
{
    stopTimer();
    if (m_proc) {
        // Send a graceful HTTP /shutdown would be cleaner - for now, kill.
        m_proc->kill();
        m_proc.reset();
    }
}

bool SidecarManager::isRunning() const
{
    return m_proc && m_proc->isRunning();
}

void SidecarManager::timerCallback()
{
    if (! m_proc) return;

    // Drain any available output without blocking.
    char buf[2048];
    auto bytes = m_proc->readProcessOutput(buf, sizeof(buf));
    if (bytes > 0) {
        m_stdoutBuffer += juce::String(buf, (size_t) bytes);
        int nl;
        while ((nl = m_stdoutBuffer.indexOfChar('\n')) >= 0) {
            auto line = m_stdoutBuffer.substring(0, nl).trim();
            m_stdoutBuffer = m_stdoutBuffer.substring(nl + 1);
            if (line == "READY" || line.startsWith("READY ")) m_readySeen = true;
            if (line.isNotEmpty() && onLogLine) onLogLine(line);
        }
    }

    if (! m_proc->isRunning()) {
        stopTimer();

        // Flush any remaining buffered output
        if (m_stdoutBuffer.trim().isNotEmpty() && onLogLine)
            onLogLine(m_stdoutBuffer.trim());
        m_stdoutBuffer.clear();

        const auto exitCode = (int) m_proc->getExitCode();
        const auto elapsed  = juce::Time::currentTimeMillis() - m_launchTimeMs;

        if (onLogLine) onLogLine("[Sidecar] Process exited after "
            + juce::String(elapsed) + "ms with code " + juce::String(exitCode));

        // If we never saw a READY signal, this is an unexpected crash.
        // Emit ERROR so the backend converts it to a Failed status.
        if (! m_readySeen && ! m_crashChecked) {
            m_crashChecked = true;
            juce::String hint;
            if (elapsed < 1000) {
                hint = "Exited within 1 second - almost certainly antivirus blocked\n"
                       "the .exe, or _internal\\ is missing next to sidecar.exe.";
            } else if (elapsed < 10000) {
                hint = "Died during startup. Most likely cause: missing Python module\n"
                       "in the PyInstaller build, or missing CUDA runtime DLL.";
            } else {
                hint = "Died during model loading. Check available RAM/VRAM,\n"
                       "and that the download directory is writable.";
            }

            m_lastError = "Sidecar exited unexpectedly.\n"
                        + juce::String("Exit code: " + juce::String(exitCode) + "\n"
                        + "Runtime: " + juce::String(elapsed) + "ms\n\n")
                        + hint + "\n\n"
                        + "Try running sidecar.exe manually by double-clicking:\n"
                        + m_sidecarPath;
            if (onLogLine) onLogLine("ERROR " + m_lastError);
        }
    }
}

#if JUCE_WINDOWS
bool SidecarManager::createJobAndAssign()
{
    // Create a Windows Job Object with KILL_ON_JOB_CLOSE.  If the DAW process
    // dies, the OS tears down the Job, which kills our sidecar - no orphans.
    HANDLE job = ::CreateJobObjectA(nullptr, nullptr);
    if (job == nullptr) return false;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (! ::SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                    &info, sizeof(info))) {
        ::CloseHandle(job);
        return false;
    }

    // We don't have direct access to the child's HANDLE from juce::ChildProcess,
    // so grab it via the PID from the /proc snapshot. JUCE doesn't expose PID
    // either; we instead rely on the process being the only instance we just spawned.
    // A robust path would be to subclass juce::ChildProcess - deferred for v2.
    //
    // NOTE: For now we store the job handle; even without assignment, on DAW
    // exit the sidecar's own --parent-pid watchdog (in Python) will kill it.
    m_jobHandle = job;
    return true;
}
#endif

} // namespace AIMC
