#include <atlbase.h>

#include <algorithm>
#include <atlstr.h>
#include <atltypes.h>
#include <atlwin.h>
#include <dwrite.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <shobjidl_core.h>
#include <windows.h>

#include "ArcParser.h"
#include "Common.hpp"
#include "Dialog.h"
#include "Interpolator.h"
#include "config.h"

#pragma region DirectX

void Dialog::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = {60, 1};
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
    };

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray,
                                  _countof(featureLevelArray), D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice,
                                  &featureLevel, &m_pd3dContext);
    CreateRenderTarget();
}

void Dialog::CreateRenderTarget() {
    ID3D11Texture2D *pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void Dialog::CleanupRenderTarget() {
    if (m_mainRenderTargetView) {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

void Dialog::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (m_pSwapChain) {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    if (m_pd3dContext) {
        m_pd3dContext->Release();
        m_pd3dContext = nullptr;
    }
    if (m_pd3dDevice) {
        m_pd3dDevice->Release();
        m_pd3dDevice = nullptr;
    }
}

#pragma endregion DirectX

#pragma region Initialization

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Dialog::OnRange(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, BOOL &bHandled) const {
    if (uMsg == WM_NCHITTEST) {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(&pt);

        RECT rc = {};
        GetClientRect(&rc);

        if (pt.y < 30 && pt.x < rc.right - 30) {
            return HTCAPTION;
        }
    }

    if (ImGui_ImplWin32_WndProcHandler(m_hWnd, uMsg, wParam, lParam)) {
        bHandled = TRUE;
    } else {
        bHandled = FALSE;
    }

    return 0;
}


Dialog::Dialog(IMargretePluginContext *ctx, ConfigContext &cctx) : m_ctx(ctx), m_cctx(cctx) {
}

Dialog::~Dialog() {
    CleanupDeviceD3D();
}

LRESULT Dialog::OnCreate(UINT, WPARAM, LPARAM, BOOL &) {
    CreateDeviceD3D();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dContext);

    SetFocus();

    return 0;
}

LRESULT Dialog::OnDestroy(UINT, WPARAM, LPARAM, BOOL &) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();

    return 0;
}

#pragma endregion Initialization

HRESULT Dialog::ShowDialog(const HWND hWndParent) {
    RECT rect = {0, 0, 300, 300};
    Create(hWndParent, rect, W_EN_TITLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    CenterWindow();
    ShowWindow(SW_SHOW);
    UpdateWindow();

    if (m_ctx) {
        auto result = m_ctx->getDocument(m_doc.put());
        if (result != MP_TRUE) {
            throw std::runtime_error("Failed to get IMargretePluginDocument from IMargretePluginContext");
        }

        result = m_doc->getUndoBuffer(m_undoBuffer.put());
        if (result != MP_TRUE) {
            throw std::runtime_error("Failed to get IMargretePluginUndoBuffer from IMargretePluginDocument");
        }
    }

    MSG msg = {};
    while (!m_shouldClose && IsWindow()) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        RenderUI();

        constexpr float cc[4] = {0.45f, 0.55f, 0.60f, 1.00f};
        m_pd3dContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
        m_pd3dContext->ClearRenderTargetView(m_mainRenderTargetView, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_pSwapChain->Present(1, 0);
    }

    DestroyWindow();
    return S_OK;
}

void Dialog::RenderUI() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_shouldClose = true;
    }

    // Bug: the ImGuiWindowFlags_AlwaysAutoResize is not working because of this
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    constexpr auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin(EN_TITLE, &m_showWindow, flags)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Open File")) {
        OpenFileAction();
    }

    ImGui::SameLine();
    if (m_filePath.empty()) {
        ImGui::TextUnformatted("No file selected");
    } else {
        const auto filename = std::filesystem::path(m_filePath).filename().string();
        ImGui::TextUnformatted(filename.c_str());
    }

    ImGui::Separator();

    static const char *eaTypeNames[] = {"Sine", "Power", "Circular"};
    auto esType = static_cast<int>(m_cctx.esType);

    ImGui::PushItemWidth(100.0f);
    if (ImGui::Combo("Easing", &esType, eaTypeNames, IM_ARRAYSIZE(eaTypeNames))) {
        m_cctx.esType = static_cast<EasingKind>(esType);
    }

    if (m_cctx.esType == EasingKind::Power) {
        ImGui::InputInt("Exponent (≠0)", &m_cctx.esExponent);
        if (m_cctx.esExponent == 0) {
            m_cctx.esExponent = 1;
        }
    } else if (m_cctx.esType == EasingKind::Circular) {
        ImGui::InputDouble("Linearity", &m_cctx.esEpsilon, 0.01, 0.1, "%.3f");
        m_cctx.esEpsilon = std::clamp(m_cctx.esEpsilon, 0.0, 1.0);
    }

    ImGui::Separator();

    static const std::vector snapDivisors = {1920, 960, 480, 384, 192, 128, 96, 64, 48, 32, 24, 16, 12, 8, 4};

    {
        int selectedDivisorIndex = -1;
        if (m_cctx.snap > 0 && MG_RESOLUTION % m_cctx.snap == 0) {
            const int divisor = MG_RESOLUTION / m_cctx.snap;
            const auto it = std::ranges::find(snapDivisors, divisor);
            if (it != snapDivisors.end()) {
                selectedDivisorIndex = static_cast<int>(std::distance(snapDivisors.begin(), it));
            }
        }

        std::string previewStr;
        if (selectedDivisorIndex >= 0) {
            previewStr = std::to_string(snapDivisors[selectedDivisorIndex]);
        }

        ImGui::PushItemWidth(54.99999f);
        if (ImGui::BeginCombo("=", previewStr.c_str())) {
            for (int i = 0; i < snapDivisors.size(); i++) {
                const bool isSelected = selectedDivisorIndex == i;
                if (ImGui::Selectable(std::to_string(snapDivisors[i]).c_str(), isSelected)) {
                    m_cctx.snap = MG_RESOLUTION / snapDivisors[i];
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::PushItemWidth(85.0f);
        ImGui::SameLine();
        int snapInput = m_cctx.snap;
        if (ImGui::InputInt("Precision", &snapInput)) {
            snapInput = std::clamp(snapInput, 1, MG_RESOLUTION);
            m_cctx.snap = snapInput;
        }
        ImGui::PopItemWidth();
    }

    ImGui::Separator();

    ImGui::InputInt("Width (1-16)", &m_cctx.width);
    m_cctx.width = std::clamp(m_cctx.width, 1, 16);

    ImGui::InputInt("TIL (0-15)", &m_cctx.til);
    m_cctx.til = std::clamp(m_cctx.til, 0, 15);

    ImGui::InputDouble("y Offset (Margrete)", &m_cctx.yOffset, 0.1, 1, "%.1f");

    ImGui::Checkbox("Clamp", &m_cctx.clamp);
    ImGui::PopItemWidth();

    if (ImGui::Button("Run")) {
        RunAction();
    }

    if (!!m_undoBuffer) {
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_undoBuffer->canUndo());
        if (ImGui::Button("Undo") && m_undoBuffer->canUndo()) {
            m_undoBuffer->undo();
            m_ctx->update();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(!m_undoBuffer->canRedo());
        if (ImGui::Button("Redo") && m_undoBuffer->canRedo()) {
            m_undoBuffer->redo();
            m_ctx->update();
        }
        ImGui::EndDisabled();
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_errorMessage.c_str());
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_showErrorPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::Render();

    if (!m_showWindow) {
        m_shouldClose = true;
    }
}

void Dialog::OpenFileAction() {
    OPENFILENAMEW ofn = {};
    WCHAR szFile[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"ArcCreate Chart (*.aff)\0*.aff\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        const CW2AEX<MAX_PATH * 4> utf8Path(szFile, CP_UTF8);
        m_filePath = utf8Path;
    }
}

void Dialog::RunAction() {
    try {
        ArcParser parser(m_cctx);
        parser.ParseFile(m_filePath);
        const auto &arcs = parser.m_chains;

        auto intp = Interpolator(m_cctx);
        intp.Interpolate(arcs);
        intp.PostProcess(m_cctx, m_ctx ? m_ctx->getCurrentTick() : 0);

        Log(arcs, intp);
        PutChart(intp);
    } catch (const std::exception &e) {
        m_errorMessage = e.what();
        m_showErrorPopup = true;
        ImGui::OpenPopup("Error");
    }

    if (m_ctx) {
        m_ctx->update();
    }
}

void Dialog::Log(const std::vector<std::vector<ArcNote>> &arcs, const Interpolator &intp) {
    std::cout << "Parsed " << arcs.size() << " Arcs.\n";
    const auto &notes = intp.m_notes;
    std::cout << "Interpolated " << notes.size() << " Notes.\n";

    int i = 0;
    for (const auto &chain: notes) {
        std::cout << ++i << ":\n";
        for (const auto &note: chain) {
            std::cout << "    t=" << note.tick << ", x=" << note.x << ", y=" << note.height << std::endl;
        }
    }
}

void Dialog::PutChart(const Interpolator &intp) const {
    if (!m_doc) {
        throw std::runtime_error("IMargretePluginDocument is null");
    }

    MargreteComPtr<IMargretePluginChart> chart;
    const auto result = m_doc->getChart(chart.put());
    if (result != MP_TRUE) {
        throw std::runtime_error("Failed to get IMargretePluginChart from IMargretePluginDocument");
    }

    try {
        m_undoBuffer->beginRecording();
        intp.PutIntoChart(chart);
        m_undoBuffer->commitRecording();
    } catch (const std::exception &) {
        m_undoBuffer->discardRecording();
        throw;
    }
}
