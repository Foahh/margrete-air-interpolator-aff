#pragma once

#include <atlbase.h>

#include <MargretePlugin.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>
#include <d3d11.h>

#include "Common.hpp"
#include "Interpolator.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

enum class EasingKind;
class Dialog final : public CWindowImpl<Dialog> {
public:
    DECLARE_WND_CLASS_EX(L"ImGuiWindowClass", CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(Dialog)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_RANGE_HANDLER(0, 0xFFFF, OnRange)
    END_MSG_MAP()

    HRESULT ShowDialog(HWND hWndParent = nullptr);
    void OpenFileAction();
    static void Log(const std::vector<std::vector<ArcNote>> &arcs, const Interpolator &intp);
    void PutChart(const Interpolator &intp) const;
    void RunAction();

    explicit Dialog(IMargretePluginContext *ctx, ConfigContext &cctx);
    ~Dialog() override;

private:
    // Plugin
    IMargretePluginContext *m_ctx = nullptr;
    MargreteComPtr<IMargretePluginDocument> m_doc;
    MargreteComPtr<IMargretePluginUndoBuffer> m_undoBuffer;

    std::string m_filePath;
    ConfigContext &m_cctx;

    // ImGui
    bool m_shouldClose = false;
    bool m_showWindow = true;
    bool m_showErrorPopup = false;
    std::string m_errorMessage;
    void RenderUI();

    // Win32
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &);
    LRESULT OnRange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) const;

    // DirectX
    ID3D11Device *m_pd3dDevice = nullptr;
    ID3D11DeviceContext *m_pd3dContext = nullptr;
    IDXGISwapChain *m_pSwapChain = nullptr;
    ID3D11RenderTargetView *m_mainRenderTargetView = nullptr;
    void CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
};
