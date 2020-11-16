#pragma once

#include "Core_D3D12.h"
#include <atlbase.h>
#include <functional>

///////////////////////////////////////////////////////////////////////////////
// NOTE: As a general rule we define our descriptors and signatures using a
// most-common to least-common principle.
// Most samples are bound to require a CBV for moving transforms around, but
// they may not require any SRV if they don't use any textures (most won't).
// UAVs are relatively rare except in compute cases or raytracing, and
// raytracing has its own signature anyway.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// NOTE: We generally pass in naked pointers to these functions because they
// don't own or inherit the inbound pointers. This avoids unnecessary atomic
// increments/decrements during chatty calls. We always try to return CComPtr
// types to avoid memory leaks where the client misbehaves. This at least
// gives the stack unroll a fighting chance of tossing out unbound resources.
///////////////////////////////////////////////////////////////////////////////

// Create a root signature for rasterization that contains a constant buffer
// resource only. If we don't intend to texture anything then we really don't
// need any more than this.
CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV(ID3D12Device* pDevice);

// Create a root signature for rasterization that contains a constant buffer
// resource and shader resource (for a texture?). This is a reasonably simple
// root signature for some meaningful rendering.
CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV1SRV(ID3D12Device* pDevice);

// Create a descriptor heap with 1024 entries for CBVs, SRVs, or UAVs. This
// is a relatively straightforward and simplistic manner of handling
// descriptors which has no management.
CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_CBVSRVUAV(ID3D12Device* pDevice, UINT numDescriptors);

// Create a single sampler heap with a pre-initialized UVW wrapped sampler.
CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_1Sampler(ID3D12Device* pDevice);

uint32_t D3D12Align(uint32_t size, uint32_t alignSize);

CComPtr<ID3D12Resource1> D3D12CreateBuffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize);

CComPtr<ID3D12Resource1> D3D12CreateBuffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data);

D3D12_RECT D3D12MakeRect(LONG width, LONG height);

D3D12_RESOURCE_BARRIER D3D12MakeResourceTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

D3D12_VIEWPORT D3D12MakeViewport(FLOAT width, FLOAT height);

void D3D12WaitForGPUIdle(Direct3D12Device* device);

void RunOnGPU(Direct3D12Device* device, std::function<void(ID3D12GraphicsCommandList5*)> fn);

D3D12_RENDER_TARGET_VIEW_DESC Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault();