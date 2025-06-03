#include <Windows.h>
#include <config.h>
#include <memory>

#include "Dialog.h"
#include "DllMain.h"

BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD ulReasonForCall, LPVOID lpReserved) {
    if (ulReasonForCall == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

DLLEXPORT void WINAPI MargretePluginGetInfo(MP_PLUGININFO *info) {
    if (!info) {
        return;
    }

    info->sdkVersion = MP_SDK_VERSION;
    if (info->nameBuffer) {
        wcsncpy_s(info->nameBuffer, info->nameBufferLength, W_EN_TITLE, info->nameBufferLength);
    }
    if (info->descBuffer) {
        wcsncpy_s(info->descBuffer, info->descBufferLength, W_EN_DESC, info->descBufferLength);
    }
    if (info->developerBuffer) {
        wcsncpy_s(info->developerBuffer, info->developerBufferLength, W_DEVELOPER, info->developerBufferLength);
    }
}

DLLEXPORT MpBoolean WINAPI MargretePluginCommandCreate(IMargretePluginCommand **ppobj) {
    *ppobj = new PluginCommand();
    (*ppobj)->addRef();
    return MP_TRUE;
}

MpBoolean PluginCommand::getCommandName(wchar_t *text, const MpInteger textLength) const {
    wcsncpy_s(text, textLength, W_EN_TITLE, textLength);
    return MP_TRUE;
}

MpBoolean PluginCommand::invoke(IMargretePluginContext *ctx) {
    static std::unique_ptr<Dialog> dlg;
    static ConfigContext cctx;

    const auto hwnd = static_cast<HWND>(ctx->getMainWindowHandle());

    if (dlg && dlg->IsWindow()) {
        SetForegroundWindow(dlg->m_hWnd);
        dlg->ShowWindow(SW_RESTORE);
        return S_OK;
    }

    HRESULT hr;
    try {
        dlg = std::make_unique<Dialog>(ctx, cctx);
        hr = dlg->ShowDialog(hwnd);
    } catch (const std::exception &e) {
        std::wstring w_err;
        w_err.assign(e.what(), e.what() + strlen(e.what()));
        MessageBox(hwnd, w_err.c_str(), W_EN_TITLE, MB_ICONERROR | MB_OK);
        hr = E_FAIL;
    }

    dlg = nullptr;
    return hr == S_OK ? MP_TRUE : MP_FALSE;
}
