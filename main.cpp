#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <d3d11.h>
#include <dxgi.h>
#include <thread>
#include <atomic>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"

#include "ProcAttach/variables.h"
#include "ProcAttach/procattach.h"
#include "Assets/font/rubik-font.h"
#include "resource.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static const char* VERSION_NUMBER = "v1.0.3";
    
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Changes theme from Dark to Light
static void ToggleImGuiTheme(bool* is_dark_mode) {
    *is_dark_mode ? ImGui::StyleColorsDark() : ImGui::StyleColorsLight();
    *is_dark_mode = !*is_dark_mode;
}

// Changes theme from Dark to Light
static void ToggleAutoRun(bool* auto_run) {
    *auto_run = !*auto_run;
}

static void ResetVariables(ProcAttachSpace::ProcAttachClass ProcessAttach) {
    if (ProcessAttach.ProcAttach() == 1)
        ProcessAttach.ResetVariables();
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int) // change to _tWinMain for non console, main for console
{
    ProcAttachSpace::ProcAttachClass ProcessAttach; // Main instance init of ProcAttach class

    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1)); // For the icon from resource.rc

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                      hInstance, nullptr, nullptr, nullptr, nullptr,
                      _T("ImGuiBase"), nullptr };
    ::RegisterClassEx(&wc);

    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Enthusia Point Calculator"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX, 20, 20, 310, 450, // x, y, size_x, size_y
        nullptr, nullptr, wc.hInstance, nullptr);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon); // for icon big
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon); // for icon small

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase = 16.0f; // base size

    io.IniFilename = NULL; // to disable config file generation
    io.Fonts->AddFontFromMemoryCompressedTTF(rubikfont_compressed_data, rubikfont_compressed_size); // custom default font

    ImGui::StyleColorsDark(); // default theme

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool attached = false;

    // main IMGUI window here

    bool done = false; // for imgui loop
    bool light_mode = false;
    bool auto_run = false;
    int v_clicked = 1;

    ImVec4 color = ImVec4(0.0196078431372549f, 0.607843137254902f, 0.9450980392156863f, 1);

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize); // max size imgui window to viewport

        ImGui::Begin("Enthusia Point Calculator", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar);
        
        if (auto_run)
        {
            attached = (ProcessAttach.ProcAttach() == 1);
            if (attached)
            {
                static auto lastUpdate = std::chrono::steady_clock::now();
                constexpr auto updateInterval = std::chrono::nanoseconds(16666666);

                auto now = std::chrono::steady_clock::now();

                if (now - lastUpdate >= updateInterval)
                {
                    ProcessAttach.UpdateRankingPointsAndCalculate();
                    lastUpdate = now;
                }
            }
            else
            {
                attached = (ProcessAttach.ProcAttach() == 1);
            }
        }

        if (v_clicked > 6) {
            color = ImVec4(0.0f, 0.8509803921568627f, 0.0f, 1);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("App"))
            {
                if (ImGui::MenuItem("Light Theme", NULL, light_mode)) ToggleImGuiTheme(&light_mode);
                if (ImGui::MenuItem("Auto Run", NULL, auto_run)) ToggleAutoRun(&auto_run);
                if (ImGui::MenuItem("Reset Variables")) { ResetVariables(ProcessAttach); auto_run = false; };
                ImGui::Separator();
                if (ImGui::MenuItem("Made By real-xp")) ShellExecute(0, 0, L"https://github.com/real-xp/", 0, 0, SW_SHOW);
                if (ImGui::MenuItem("License (GPLv3)")) ShellExecute(0, 0, L"https://github.com/real-xp/EnthusiaPointsCalculator/blob/main/LICENSE.txt", 0, 0, SW_SHOW);
                if (ImGui::MenuItem(VERSION_NUMBER)) { if (v_clicked < 7 && v_clicked > 0) v_clicked++; }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) exit(0);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Help")) ShellExecute(0, 0, L"https://github.com/real-xp/EnthusiaPointsCalculator/blob/main/README.md", 0, 0, SW_SHOW);
            ImGui::EndMenuBar();
        }

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::PushFont(NULL, 28);
        if (ImGui::BeginTable("split", 2)) {
            ImGui::TableNextColumn();
            ImGui::Text("RANK");
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%d", calcstruct.current_rank);

            ImGui::TableNextColumn();
            ImGui::Text("POINTS");
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%d", calcstruct.current_points);

            ImGui::TableNextColumn();
            ImGui::Text("GRADE");
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", CalculatorVars::GRADE[calcstruct.grade]);
            ImGui::EndTable();
        }

        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 10));

        ImGui::PushFont(NULL, 20);
        if (calcstruct.current_rank != 1000)
            ImGui::Text("Odds / Points to Next Grade");
        
        ImGui::Dummy(ImVec2(0, 5));

        if (ImGui::BeginTable("split", 3)) {
            if (calcstruct.current_rank != 1)
            {
                ImGui::TableNextColumn();
                ImGui::Text("GRADE");
                ImGui::TableNextColumn();
                if (calcstruct.current_rank <=300 && calcstruct.current_points != 0)
                    ImGui::Text("RS / R1");
                else
                    ImGui::Text("ODDS");
                ImGui::TableNextColumn();
                ImGui::Text("POINTS");


                ImGui::TableNextColumn();
                ImGui::Text("Rank1");
                ImGui::TableNextColumn();
                if (calcstruct.current_rank <=300)
                    ImGui::TextColored(color, "%.1f / %.1f", calcstruct.odds_for_rank1_from_rs, calcstruct.odds_for_rank1_from_r1);
                else 
                    ImGui::TextColored(color, "%.1f", calcstruct.odds_for_rank1_general);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_rank1);
            }
            if (calcstruct.points_rank6 != 0)
            {
                ImGui::TableNextColumn();
                ImGui::Text("Top6");
                ImGui::TableNextColumn();
                if (calcstruct.current_rank <=300)
                    ImGui::TextColored(color, "%.1f / %.1f", calcstruct.odds_for_rank6_from_rs, calcstruct.odds_for_rank6_from_r1);
                else
                    ImGui::TextColored(color, "%.1f", calcstruct.odds_for_rank6_general);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_rank6);
            }
            if (calcstruct.points_rs != 0)
            {
                ImGui::TableNextColumn();
                ImGui::Text("RS");
                ImGui::SameLine();
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%.1f", calcstruct.odds_for_rs);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_rs);
            }
            if (calcstruct.points_r1 != 0)
            {
                ImGui::TableNextColumn();
                ImGui::Text("R1");
                ImGui::SameLine();
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%.1f", calcstruct.odds_for_r1);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_r1);
            }
            if (calcstruct.points_r2 != 0)
            {
                ImGui::TableNextColumn();
                ImGui::Text("R2");
                ImGui::SameLine();
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%.1f", calcstruct.odds_for_r2);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_r2);
            }
            if (calcstruct.points_r3 != 0)
            {
                ImGui::TableNextColumn();
                ImGui::Text("R3");
                ImGui::SameLine();
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%.1f", calcstruct.odds_for_r3);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%d", calcstruct.points_r3);
            }
            ImGui::EndTable();
        }
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 5));

        if (!auto_run) {
            if (ImGui::Button("CHECK", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                if (ProcessAttach.ProcAttach() == 1)
                    ProcessAttach.UpdateRankingPointsAndCalculate();
            }
        }

        ImGui::End();   
        ImGui::Render();

        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // DX11 Window Color
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Other ImGUI + DX11 bloat

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
    );

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
