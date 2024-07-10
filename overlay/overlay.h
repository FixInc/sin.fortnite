#pragma once
#include <utility>
#include <cstddef>
#include <intrin.h>
#include <string>
#include <utility>
#include <d3dx11tex.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <iostream>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

inline HWND window;
inline ID3D11Device* d3d_device;
inline ID3D11DeviceContext* d3d_device_ctx;
inline IDXGISwapChain* d3d_swap_chain;
inline ID3D11RenderTargetView* d3d_render_target;

static class overlay
{
public:
    static bool init();
    static void render();
    static bool render_loop();
    static bool init_dx11();
};
