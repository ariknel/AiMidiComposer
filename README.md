# 🎹 AI MIDI Composer

> **A VST3 plugin that turns natural-language prompts into multitrack MIDI compositions — powered by a locally-running music language model.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)
[![VST3](https://img.shields.io/badge/Plugin%20Format-VST3-orange.svg)](https://www.steinberg.net/vst3/)
[![JUCE 8](https://img.shields.io/badge/Framework-JUCE%208-green.svg)](https://juce.com/)
[![Model: MIDI-LLM](https://img.shields.io/badge/Model-MIDI--LLM%20(NeurIPS%202025)-purple.svg)](#model--research)

---

## ✨ What Is This?

AI MIDI Composer is a **VST3 plugin** that embeds a ChatGPT-style chat interface directly inside your DAW. Describe what you want in plain language, and the plugin generates a ready-to-use multitrack MIDI composition — entirely **on your machine**, with no cloud dependency after the first model download.

```
"Give me a lo-fi hip hop beat with lazy drums, a Rhodes chord stab, and a walking bass line"
```

Hit send. Drag the stems straight into your DAW tracks.

---

## ⚠️ System Requirements

**Please read before installing.** The plugin runs an AI model locally on your CPU — generation takes a few seconds per prompt depending on your hardware. It is not suitable for real-time use.

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| OS | Windows 10 (64-bit) | Windows 11 (64-bit) |
| CPU | 4-core, 2.5 GHz | 8-core, 3.5 GHz+ |
| RAM | 8 GB | 16 GB |
| Disk space | 2 GB free | 4 GB free |
| Internet | Required on first launch (model download ~150 MB) | — |
| DAW | Any VST3-compatible DAW on Windows | FL Studio, Ableton Live, Reaper |

> **Performance note:** Generation is blocking and autoregressive — the plugin processes one prompt at a time. On a mid-range CPU expect roughly 5–15 seconds per generation. The Send button disables during inference and a progress indicator is shown. This is a known limitation of the current model architecture and is tracked as a v2 improvement.

---

## 📸 Screenshots

> *(Replace with real screenshots before release)*

![Workflow placeholder](docs/images/workflow_placeholder.png)
*↑ Chat interface inside FL Studio — prompt → generation → drag stems to tracks*

---

## 🎬 Quick Demo

> *(Embed a GIF or YouTube link here)*

---

## ⬇️ Download & Install

Head to the [**Releases**](../../releases) page and grab the latest version.

### Option A — Installer (recommended)

1. Download `AIMidiComposer_vX.X_Setup.exe`
2. Run the installer — it places the `.vst3` and sidecar in the correct locations automatically
3. Rescan plugins in your DAW
4. On **first launch**, the plugin downloads the MIDI-LLM model weights (~150 MB) — a one-time internet connection is required

### Option B — Manual drag-and-drop

Prefer to manage your own plugin folders? Grab the zip instead.

1. Download `AIMidiComposer_vX.X_Manual.zip`
2. Extract and copy `AI MIDI Composer.vst3` into your VST3 folder:
   ```
   C:\Program Files\Common Files\VST3\
   ```
3. Rescan plugins in your DAW
4. On **first launch**, the plugin downloads the MIDI-LLM model weights (~150 MB)

> **DAW compatibility:** Tested in FL Studio, Ableton Live, and Reaper. Any DAW supporting VST3 on Windows should work.

---

## 🎛️ Using the Plugin

1. **Load** `AI MIDI Composer` as an instrument track in your DAW
2. **Type a prompt** describing the music you want — genre, mood, instruments, tempo, feel
3. **Hit Send** — a progress indicator shows while the model generates
4. When done, **stem chips** appear in the chat bubble, one per instrument
5. **Drag a chip** onto any MIDI track in your DAW to import it
6. **Refine** by continuing the conversation: *"add a synth pad"*, *"slower tempo"*, *"more swing on the drums"*

### Prompt tips

- Be specific about instrumentation: *"Rhodes piano, upright bass, brushed snare"*
- Reference genres or feel: *"in the style of J Dilla"*, *"dark and cinematic"*
- Sculpt iteratively with follow-ups rather than re-describing everything from scratch
- Generation processes one request at a time — wait for the current one to finish before sending another

---

## 🔄 How It Works

The plugin is split into two parts: a **C++ VST3 front-end** (JUCE 8) and a **Python sidecar** that runs the AI model locally. Everything stays on your machine.

```
┌─────────────────────────────────────────────────────────────┐
│                      DAW (FL, Ableton, ...)                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │   AI MIDI Composer.vst3  (JUCE 8, C++)                │  │
│  │                                                        │  │
│  │   Chat UI  ──▶  HttpSidecarBackend ──▶ localhost:XXXX │  │
│  │   MidiStemSplitter ◀── combined .mid                  │  │
│  │   Drag-to-DAW (Win OLE IDataObject CF_HDROP)          │  │
│  └────────────────────────────┬──────────────────────────┘  │
└────────────────────────────────┼────────────────────────────┘
                                 │ child process, Job Object
                                 ▼
                   ┌─────────────────────────────────┐
                   │  sidecar.exe  (PyInstaller)      │
                   │    FastAPI + Uvicorn             │
                   │    MIDI-LLM (Llama 3.2 1B)       │
                   │    Anticipation (AMT tokenizer)  │
                   │    mido (MIDI I/O)               │
                   └─────────────────────────────────┘
```

1. You type a prompt in the chat panel inside your DAW
2. The C++ plugin forwards it over localhost HTTP to the Python sidecar
3. The sidecar tokenises your prompt using the [Anticipation AMT tokenizer](#model--research) and runs autoregressive inference with MIDI-LLM
4. The model outputs a combined multi-track `.mid` file
5. `MidiStemSplitter` separates it into per-instrument stems
6. Each stem appears as a draggable chip in the chat bubble — drop it onto any DAW track
7. Follow-up prompts append a continuation directive to the prior conversation, keeping musical context intact across turns

---

## 🧠 Model & Research

AI MIDI Composer runs **MIDI-LLM**, a music-specific language model presented at the **NeurIPS 2025 AI for Music Workshop**.

| Resource | Link |
|----------|------|
| 📄 MIDI-LLM paper (NeurIPS AI4Music 2025) | *(link TBD — add when published)* |
| 🎵 Anticipation / AMT tokenizer | [GitHub — anticipation](https://github.com/jthickstun/anticipation) |
| 📄 Anticipation paper (Thickstun et al., 2023) | [arXiv:2306.08620](https://arxiv.org/abs/2306.08620) |
| 🦙 Base model: Llama 3.2 1B | [Meta AI](https://ai.meta.com/blog/llama-3-2-connect-2024-vision-edge-mobile-devices/) |
| 🎹 mido — MIDI I/O library | [mido.readthedocs.io](https://mido.readthedocs.io/) |
| 🔊 JUCE framework | [juce.com](https://juce.com/) |

### Why local?

Privacy, no latency after first load, no subscription. Your musical ideas never leave your machine.

---

## 🛠️ Building from Source

> You only need this if you want to **contribute or modify** the plugin. Regular users should grab the [installer or manual zip](#️-download--install) from Releases.

### Prerequisites

**VST3 build:**

| Tool | Notes |
|------|-------|
| Visual Studio 2022 Build Tools | "Desktop development with C++" workload |
| CMake | Must be in `PATH` |
| Git | Must be in `PATH` |
| Ninja | Expected at `C:\Windows\System32\ninja.exe` |
| Inno Setup **5** | ⚠️ NOT version 6 — see [pitfall notes](docs/pitfalls.md) |

**Python sidecar:**

| Tool | Notes |
|------|-------|
| Python 3.11 (x64) | Must be in `PATH` |
| ~8 GB free disk | PyInstaller intermediates + model weights |

### Build steps

```bat
:: 1. Build the VST3
build.bat

:: 2. Freeze the Python sidecar into sidecar\dist\sidecar\sidecar.exe
build_sidecar.bat

:: 3. Package everything into a single installer .exe
make_installer.bat
```

### Project structure

```
AIMidiComposer/
  CMakeLists.txt
  build.bat                          # VST3 build
  build_sidecar.bat                  # Python freeze
  installer.iss                      # Inno Setup 5 script (ASCII only)
  make_installer.bat                 # Installer build

  src/
    plugin/
      PluginProcessor.{h,cpp}        # AudioProcessor, APVTS, session dir
      Parameters.h                   # automatable parameters only
    gui/
      PluginEditor.{h,cpp}           # main window
      ChatView.{h,cpp}               # scrollable message list
      MessageBubble.{h,cpp}          # one bubble, owns stem chips
      PromptInput.{h,cpp}            # multiline input + send button
      StemStrip.{h,cpp}              # draggable stem chip
      LookAndFeel.{h,cpp}            # clean dark palette
      Colours.h
    inference/
      InferenceBackend.h             # abstract interface
      HttpSidecarBackend.{h,cpp}     # concrete impl over localhost HTTP
      SidecarManager.{h,cpp}         # spawn/kill Python process
    midi/
      MidiStemSplitter.{h,cpp}       # multitrack MIDI → per-instrument .mid
      DragSourceWindows.{h,cpp}      # OLE CF_HDROP drag source
    chat/
      ChatHistory.{h,cpp}            # per-instance log + XML roundtrip
      Message.h

  sidecar/
    main.py                          # FastAPI + MIDI-LLM + MIDI analysis
    requirements.txt
    sidecar.spec                     # PyInstaller spec
```

---

## ⚙️ Technical Notes

| Topic | Detail |
|-------|--------|
| **First-launch model download** | Installer is ~150 MB; weights download on first run to stay under the GitHub Releases 2 GB limit |
| **Generation is blocking** | MIDI-LLM is autoregressive — one request at a time. UI shows a progress indicator and disables Send during inference |
| **Iterative refinement** | No native edit API — implemented by flattening prior conversation into a continuation directive appended to each new prompt |
| **Sidecar process lifetime** | Managed via Windows Job Object — sidecar is killed automatically if the DAW crashes or exits |
| **Windows only (v1)** | Drag-to-DAW uses Win32 OLE `IDataObject CF_HDROP`. Cross-platform drag support is a v2 item |

---

## 🤝 Contributing

Contributions are welcome! Please open an issue first for significant changes.

Areas especially open for contribution:
- macOS / Linux port (replacing the OLE drag source)
- GGUF conversion of MIDI-LLM weights (removes Python sidecar dependency entirely)
- Additional stem post-processing (quantisation, humanisation)
- Improved iterative refinement prompting strategies

---

## 📄 License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.

MIDI-LLM model weights are subject to their own license — see the model card when published.

---

## 🙏 Acknowledgements

- [Anticipation](https://github.com/jthickstun/anticipation) by Thickstun et al. for the AMT MIDI tokenizer
- [JUCE](https://juce.com/) for the VST3 / audio plugin framework
- [FastAPI](https://fastapi.tiangolo.com/) and [Uvicorn](https://www.uvicorn.org/) for the sidecar HTTP layer
- [mido](https://mido.readthedocs.io/) for MIDI file I/O
- The NeurIPS AI for Music Workshop community for pushing music AI research forward
