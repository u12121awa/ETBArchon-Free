#pragma once

#include "DX12DrawContext.h"
#include "Render2DUtil.h"
#include "Shader/impr/BlurProgram.h"
#include "Shader/impr/GlowProgram.h"
#include "Shader/impr/RoundedRectProgram.h"
#include "Shader/impr/StencilRectProgram.h"
#include "Shader/impr/ShadowBlurProgram.h"
#include "Shader/impr/MeshProgram.h"
#include <vector>
#include <algorithm>
#include "d3dx12.h"

struct ShaderCmdParams
{
    int type;
    float x, y, w, h;
    float radius;
    float strength;
    float opacity;
    float screenW, screenH;

    float c1[4];
    float c2[4];
    float c3[4];
    float c4[4];

    D3D12_GPU_DESCRIPTOR_HANDLE maskHandle;

    bool useScissor;
    LONG scLeft, scTop, scRight, scBottom;

    uint32_t instanceOffset;
    uint32_t instanceCount;
};

class Shader2DUtil
{
private:
    BlurProgram m_BlurProgram;
    ShadowBlurProgram m_ShadowBlurProgram;
    GlowProgram m_GlowProgram;
    RoundedRectProgram m_RoundedRectProgram;
    StencilRectProgram m_StencilRectProgram;
    MeshProgram m_MeshProgram;

    ID3D12DescriptorHeap* m_SharedSrvHeap = nullptr;
    bool m_IsInit = false;

    std::vector<ShaderCmdParams> m_CmdPool;
    std::vector<MaskInstance> m_InstancePool;

    ID3D12GraphicsCommandList* m_CurrentCmdList = nullptr;
    ID3D12Resource* m_CurrentBackBuffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentRTV = {};

    bool m_IsScissorActive = false;
    bool m_IsScissorPaused = false;
    D3D12_RECT m_CurrentScissorRect = {};

public:
    static Shader2DUtil& Get()
    {
        static Shader2DUtil instance;
        return instance;
    }

    void SetEnableBlur(bool enable)
    {
        m_BlurProgram.SetEnableBlur(enable);
    }

    void Init(ID3D12Device* device, DXGI_FORMAT format, ID3D12DescriptorHeap* sharedSrvHeap,
              D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart,
              UINT srvDescSize, UINT* pSrvCurrentOffset)
    {
        if (m_IsInit) return;
        m_SharedSrvHeap = sharedSrvHeap;
        m_BlurProgram.InitBlur(device, format, sharedSrvHeap, srvCpuStart, srvGpuStart, srvDescSize, pSrvCurrentOffset);

        D3D12_CPU_DESCRIPTOR_HANDLE shadowCpu = srvCpuStart;
        shadowCpu.ptr += (SIZE_T)(*pSrvCurrentOffset) * srvDescSize;
        D3D12_GPU_DESCRIPTOR_HANDLE shadowGpu = srvGpuStart;
        shadowGpu.ptr += (SIZE_T)(*pSrvCurrentOffset) * srvDescSize;
        m_ShadowBlurProgram.InitShadow(device, format, sharedSrvHeap, shadowCpu, shadowGpu, srvDescSize);
        if (pSrvCurrentOffset) *pSrvCurrentOffset += 50;

        m_GlowProgram.Init(device, format);
        m_RoundedRectProgram.Init(device, format);
        m_StencilRectProgram.Init(device, format);

        m_IsInit = true;
        m_CmdPool.reserve(256);
        m_InstancePool.reserve(1024);
    }

    void InitMesh(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE rtvStart, UINT rtvSize, UINT* pSrvOffset,
                  D3D12_CPU_DESCRIPTOR_HANDLE srvStart, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart, UINT srvSize)
    {
        UINT rtvOff = 0;
        m_MeshProgram.InitMesh(device, DXGI_FORMAT_R8_UNORM, m_SharedSrvHeap, srvStart, srvGpuStart, srvSize,
                               pSrvOffset, rtvStart, rtvSize, &rtvOff);
    }

    void BeginFrame(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtv)
    {
        m_CurrentCmdList = cmdList;
        m_CurrentBackBuffer = backBuffer;
        m_CurrentRTV = rtv;
        m_CmdPool.clear();
        m_InstancePool.clear();
    }

    void PauseScissor() { m_IsScissorPaused = true; }
    void ResumeScissor() { m_IsScissorPaused = false; }

    void StartScissor(float x, float y, float x1, float y1)
    {
        m_CurrentScissorRect = {(LONG)x, (LONG)y, (LONG)x1, (LONG)y1};
        m_IsScissorActive = true;
        m_IsScissorPaused = false;
    }

    void StartScissorSize(float x, float y, float w, float h)
    {
        StartScissor(x, y, x + w, y + h);
    }

    void StopScissor() { m_IsScissorActive = false; }

    void UpdateSize(int width, int height)
    {
        if (m_IsInit)
        {
            m_BlurProgram.UpdateSize(width, height);
            m_MeshProgram.UpdateSize(width, height);
        }
    }

    static void RenderCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
    {
        ShaderCmdParams* p = (ShaderCmdParams*)cmd->UserCallbackData;
        Shader2DUtil& inst = Get();
        if (!inst.m_CurrentCmdList || !inst.m_CurrentBackBuffer) return;

        if (inst.m_SharedSrvHeap)
        {
            ID3D12DescriptorHeap* heaps[] = {inst.m_SharedSrvHeap};
            inst.m_CurrentCmdList->SetDescriptorHeaps(1, heaps);
        }

        CD3DX12_VIEWPORT vp(0.f, 0.f, p->screenW, p->screenH);
        inst.m_CurrentCmdList->RSSetViewports(1, &vp);

        CD3DX12_RECT sc;
        if (p->useScissor) sc = CD3DX12_RECT(p->scLeft, p->scTop, p->scRight, p->scBottom);
        else sc = CD3DX12_RECT(0, 0, (LONG)p->screenW, (LONG)p->screenH);
        inst.m_CurrentCmdList->RSSetScissorRects(1, &sc);

        switch (p->type)
        {
        case 0:
            {
                MCBlurParams bp;
                bp.InputResolution[0] = p->screenW;
                bp.InputResolution[1] = p->screenH;
                bp.uSize[0] = p->w;
                bp.uSize[1] = p->h;
                bp.uLocation[0] = p->x;
                bp.uLocation[1] = p->y;
                bp.radius = p->radius;
                bp.Brightness = p->opacity;
                bp.Quality = p->strength;
                memcpy(bp.color1, p->c1, 16);
                inst.m_BlurProgram.RenderToBackBuffer(inst.m_CurrentCmdList, inst.m_CurrentBackBuffer,
                                                      inst.m_CurrentRTV, sc, bp);
                break;
            }
        case 1:
            {
                GlowParams gp;
                gp.uLocation[0] = p->x;
                gp.uLocation[1] = p->y;
                gp.uSize[0] = p->w;
                gp.uSize[1] = p->h;
                gp.radius = p->radius;
                gp.glow = p->strength;
                memcpy(gp.color1, p->c1, 16);
                memcpy(gp.color2, p->c2, 16);
                memcpy(gp.color3, p->c3, 16);
                memcpy(gp.color4, p->c4, 16);
                inst.m_GlowProgram.Render(inst.m_CurrentCmdList, inst.m_CurrentRTV, sc, gp);
                break;
            }
        case 2:
            {
                RoundedRectParams rp;
                rp.uLocation[0] = p->x;
                rp.uLocation[1] = p->y;
                rp.uSize[0] = p->w;
                rp.uSize[1] = p->h;
                rp.radius = p->radius;
                memcpy(rp.color1, p->c1, 16);
                memcpy(rp.color2, p->c2, 16);
                memcpy(rp.color3, p->c3, 16);
                memcpy(rp.color4, p->c4, 16);
                inst.m_RoundedRectProgram.Render(inst.m_CurrentCmdList, inst.m_CurrentRTV, sc, rp);
                break;
            }
        case 3:
            {
                StencilParams sp;
                sp.uLocation[0] = p->x;
                sp.uLocation[1] = p->y;
                sp.uSize[0] = p->w;
                sp.uSize[1] = p->h;
                memcpy(sp.colorTL, p->c1, 16);
                memcpy(sp.colorTR, p->c2, 16);
                memcpy(sp.colorBL, p->c3, 16);
                memcpy(sp.colorBR, p->c4, 16);
                CD3DX12_RECT ir((LONG)p->x, (LONG)p->y, (LONG)(p->x + p->w), (LONG)(p->y + p->h));
                if (p->useScissor)
                {
                    ir.left = std::max(ir.left, p->scLeft);
                    ir.top = std::max(ir.top, p->scTop);
                    ir.right = std::min(ir.right, p->scRight);
                    ir.bottom = std::min(ir.bottom, p->scBottom);
                }
                if (ir.right > ir.left && ir.bottom > ir.top) inst.m_StencilRectProgram.Render(
                    inst.m_CurrentCmdList, inst.m_CurrentRTV, ir, sp, p->maskHandle);
                break;
            }
        case 4:
            {
                inst.m_ShadowBlurProgram.DrawBlurredShadow(inst.m_CurrentCmdList, p->x, p->y, p->w, p->h, p->radius,
                                                           (int)p->strength, p->c1, p->screenW, p->screenH);
                break;
            }
        case 5:
            {
                if (p->instanceCount > 0) inst.m_MeshProgram.UploadInstances(
                    inst.m_InstancePool.data() + p->instanceOffset, p->instanceCount);
                CD3DX12_RECT maskSc((LONG)p->x, (LONG)p->y, (LONG)(p->x + p->w), (LONG)(p->y + p->h));
                inst.m_MeshProgram.DrawMask(inst.m_CurrentCmdList, maskSc, (int)p->screenW, (int)p->screenH,
                                            p->instanceCount);
                MCBlurParams bp;
                bp.InputResolution[0] = p->screenW;
                bp.InputResolution[1] = p->screenH;
                bp.uSize[0] = p->w;
                bp.uSize[1] = p->h;
                bp.uLocation[0] = p->x;
                bp.uLocation[1] = p->y;
                bp.radius = 0;
                bp.Brightness = p->opacity;
                bp.Quality = p->strength;
                memcpy(bp.color1, p->c1, 16);
                inst.m_BlurProgram.RenderMasked(inst.m_CurrentCmdList, inst.m_CurrentBackBuffer, inst.m_CurrentRTV, sc,
                                                bp, inst.m_MeshProgram.GetMaskSRVGpu());
                break;
            }
        }
    }

    void PushToImGui(ImDrawList* drawList, ShaderCmdParams& p)
    {
        if (!drawList) return;
        if (m_IsScissorActive && !m_IsScissorPaused)
        {
            p.useScissor = true;
            p.scLeft = m_CurrentScissorRect.left;
            p.scTop = m_CurrentScissorRect.top;
            p.scRight = m_CurrentScissorRect.right;
            p.scBottom = m_CurrentScissorRect.bottom;
        }
        else
        {
            p.useScissor = false;
        }
        m_CmdPool.push_back(p);
        drawList->AddCallback(RenderCallback, &m_CmdPool.back());
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }

    void OnDrawRoundBlur(DX12DrawContext context, float x, float y, float w, float h, float radius, RGBA color,
                         float strength, float opacity, ImDrawList* targetDrawList = nullptr)
    {
        UpdateSize((int)context.ScreenWidth, (int)context.ScreenHeight);
        ShaderCmdParams p = {};
        p.type = 0;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.radius = radius;
        p.strength = strength;
        p.opacity = opacity;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        p.c1[0] = color.R / 255.f;
        p.c1[1] = color.G / 255.f;
        p.c1[2] = color.B / 255.f;
        p.c1[3] = color.A / 255.f;
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }

    void OnDrawRoundGlow(DX12DrawContext context, float x, float y, float w, float h, float radius, float strength,
                         float alpha, RGBA4C color, ImDrawList* targetDrawList = nullptr)
    {
        ShaderCmdParams p = {};
        p.type = 1;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.radius = radius;
        p.strength = strength;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        auto f = [](float* o, RGBA c, float a)
        {
            o[0] = c.R / 255.f;
            o[1] = c.G / 255.f;
            o[2] = c.B / 255.f;
            o[3] = a;
        };
        f(p.c1, color.C1, alpha);
        f(p.c2, color.C2, alpha);
        f(p.c3, color.C3, alpha);
        f(p.c4, color.C4, alpha);
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }

    void OnDrawRoundedRect(DX12DrawContext context, float x, float y, float w, float h, float radius, RGBA4C color,
                           ImDrawList* targetDrawList = nullptr)
    {
        ShaderCmdParams p = {};
        p.type = 2;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.radius = radius;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        auto f = [](float* o, RGBA c)
        {
            o[0] = c.R / 255.f;
            o[1] = c.G / 255.f;
            o[2] = c.B / 255.f;
            o[3] = c.A / 255.f;
        };
        f(p.c1, color.C1);
        f(p.c2, color.C2);
        f(p.c3, color.C3);
        f(p.c4, color.C4);
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }

    void OnDrawStencilImage(DX12DrawContext context, float x, float y, float w, float h, RGBA4C color,
                            D3D12_GPU_DESCRIPTOR_HANDLE mask, ImDrawList* targetDrawList = nullptr)
    {
        ShaderCmdParams p = {};
        p.type = 3;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.maskHandle = mask;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        auto f = [](float* o, RGBA c)
        {
            o[0] = c.R / 255.f;
            o[1] = c.G / 255.f;
            o[2] = c.B / 255.f;
            o[3] = c.A / 255.f;
        };
        f(p.c1, color.C1);
        f(p.c2, color.C2);
        f(p.c3, color.C3);
        f(p.c4, color.C4);
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }

    void OnDrawShadowBlur(DX12DrawContext context, float x, float y, float w, float h, float radius, int strength,
                          RGBA color, ImDrawList* targetDrawList = nullptr)
    {
        ShaderCmdParams p = {};
        p.type = 4;
        p.x = x;
        p.y = y;
        p.w = w;
        p.h = h;
        p.radius = radius;
        p.strength = (float)strength;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        p.c1[0] = color.R / 255.f;
        p.c1[1] = color.G / 255.f;
        p.c1[2] = color.B / 255.f;
        p.c1[3] = color.A / 255.f;
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }
    template <typename T>
    void OnDrawMergeBlur(DX12DrawContext context, 
                         const std::vector<T>& rects, 
                         RGBA color, 
                         float strength, 
                         float opacity, 
                         ImDrawList* targetDrawList = nullptr)
    {
        if (rects.empty()) return;
        
        std::vector<MaskInstance> instances;
        instances.reserve(rects.size());

        for (const auto& r : rects) {
            MaskInstance mi = {};
            mi.x = r.x;
            mi.y = r.y;
            mi.w = r.width;
            mi.h = r.height;
            mi.radius = r.radius;
            instances.push_back(mi);
        }
        
        OnDrawMergeBlur(context, instances, color, strength, opacity, targetDrawList);
    }

    void OnDrawMergeBlur(DX12DrawContext context, const std::vector<MaskInstance>& rects, RGBA color,
                         float strength, float opacity, ImDrawList* targetDrawList = nullptr)
    {
        if (rects.empty()) return;
        UpdateSize((int)context.ScreenWidth, (int)context.ScreenHeight);
        float minX = 100000.f, minY = 100000.f, maxX = -100000.f, maxY = -100000.f;
        for (const auto& r : rects)
        {
            minX = std::min(minX, r.x);
            minY = std::min(minY, r.y);
            maxX = std::max(maxX, r.x + r.w);
            maxY = std::max(maxY, r.y + r.h);
        }
        float pad = 20.0f;
        minX -= pad;
        minY -= pad;
        maxX += pad;
        maxY += pad;
        uint32_t startOffset = (uint32_t)m_InstancePool.size();
        m_InstancePool.insert(m_InstancePool.end(), rects.begin(), rects.end());

        ShaderCmdParams p = {};
        p.type = 5;
        p.x = minX;
        p.y = minY;
        p.w = maxX - minX;
        p.h = maxY - minY;
        p.strength = strength;
        p.opacity = opacity;
        p.screenW = context.ScreenWidth;
        p.screenH = context.ScreenHeight;
        p.c1[0] = color.R / 255.f;
        p.c1[1] = color.G / 255.f;
        p.c1[2] = color.B / 255.f;
        p.c1[3] = color.A / 255.f;
        p.instanceOffset = startOffset;
        p.instanceCount = (uint32_t)rects.size();
        PushToImGui(targetDrawList ? targetDrawList : context.BackGroundDrawList, p);
    }
};
