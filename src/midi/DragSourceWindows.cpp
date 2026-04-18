#include "DragSourceWindows.h"

#if JUCE_WINDOWS
 #include <windows.h>
 #include <shlobj.h>
 #include <ole2.h>
 #include <shellapi.h>
#endif

namespace AIMC {

#if JUCE_WINDOWS

// Minimal IDropSource implementation
class DropSource : public IDropSource
{
public:
    DropSource() = default;
    virtual ~DropSource() = default;

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override  { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
        if (iid == IID_IUnknown || iid == IID_IDropSource) {
            *ppv = static_cast<IDropSource*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // IDropSource
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escapePressed, DWORD keyState) override {
        if (escapePressed) return DRAGDROP_S_CANCEL;
        if (!(keyState & (MK_LBUTTON | MK_RBUTTON))) return DRAGDROP_S_DROP;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override {
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

private:
    ULONG m_ref = 1;
};

// Minimal IDataObject for CF_HDROP file list
class FilesDataObject : public IDataObject
{
public:
    explicit FilesDataObject(const juce::StringArray& paths) : m_paths(paths) { }
    virtual ~FilesDataObject() {
        if (m_storage.pUnkForRelease) m_storage.pUnkForRelease->Release();
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override  { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) override {
        if (iid == IID_IUnknown || iid == IID_IDataObject) {
            *ppv = static_cast<IDataObject*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // Build a DROPFILES structure (double-null-terminated wide-char list)
    HGLOBAL buildDropFiles()
    {
        // Compute size in wide chars (incl. final terminator pair)
        size_t totalChars = 1; // the final extra NUL
        for (const auto& p : m_paths)
            totalChars += (size_t) p.toWideCharPointer() ? (p.length() + 1) : 0;

        SIZE_T bytes = sizeof(DROPFILES) + totalChars * sizeof(WCHAR);
        HGLOBAL hMem = GlobalAlloc(GHND, bytes);
        if (! hMem) return nullptr;

        auto* df = static_cast<DROPFILES*>(GlobalLock(hMem));
        df->pFiles = sizeof(DROPFILES);
        df->fWide  = TRUE;

        auto* dst = reinterpret_cast<WCHAR*>(reinterpret_cast<BYTE*>(df) + sizeof(DROPFILES));
        for (const auto& p : m_paths) {
            auto w = p.toWideCharPointer();
            if (! w) continue;
            size_t len = wcslen(w);
            std::memcpy(dst, w, (len + 1) * sizeof(WCHAR));
            dst += len + 1;
        }
        *dst = L'\0'; // final terminator
        GlobalUnlock(hMem);
        return hMem;
    }

    // IDataObject
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pformatetc, STGMEDIUM* pmedium) override {
        if (pformatetc->cfFormat != CF_HDROP || !(pformatetc->tymed & TYMED_HGLOBAL))
            return DV_E_FORMATETC;
        HGLOBAL h = buildDropFiles();
        if (! h) return E_OUTOFMEMORY;
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = h;
        pmedium->pUnkForRelease = nullptr;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*, STGMEDIUM*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pformatetc) override {
        return (pformatetc->cfFormat == CF_HDROP && (pformatetc->tymed & TYMED_HGLOBAL))
            ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*, FORMATETC* pout) override {
        pout->ptd = nullptr; return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC*, STGMEDIUM*, BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dir, IEnumFORMATETC** ppenum) override {
        if (dir != DATADIR_GET) { *ppenum = nullptr; return E_NOTIMPL; }
        // SHCreateStdEnumFmtEtc gives us a std enum for a single format.
        FORMATETC fe{};
        fe.cfFormat = CF_HDROP;
        fe.dwAspect = DVASPECT_CONTENT;
        fe.lindex   = -1;
        fe.tymed    = TYMED_HGLOBAL;
        return SHCreateStdEnumFmtEtc(1, &fe, ppenum);
    }
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override { return OLE_E_ADVISENOTSUPPORTED; }
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return OLE_E_ADVISENOTSUPPORTED; }
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override { return OLE_E_ADVISENOTSUPPORTED; }

private:
    ULONG m_ref = 1;
    juce::StringArray m_paths;
    STGMEDIUM m_storage{};
};

bool WinFileDragSource::performDrag(const juce::StringArray& absolutePaths)
{
    if (absolutePaths.isEmpty()) return false;

    // Ensure COM is initialized on this thread
    HRESULT coInit = OleInitialize(nullptr);
    const bool weInitialized = SUCCEEDED(coInit);

    auto* data = new FilesDataObject(absolutePaths);
    auto* source = new DropSource();

    DWORD effect = 0;
    HRESULT hr = DoDragDrop(data, source, DROPEFFECT_COPY | DROPEFFECT_LINK, &effect);

    data->Release();
    source->Release();

    if (weInitialized) OleUninitialize();
    return hr == DRAGDROP_S_DROP;
}

#else

bool WinFileDragSource::performDrag(const juce::StringArray&) { return false; }

#endif

} // namespace AIMC
