#pragma once

#include "Core_D3D12.h"
#include <atlbase.h>
#include <functional>

///////////////////////////////////////////////////////////////////////////////
// These handy functions can be used to initialize a selection of D3D12
// structures with far less code. For easy identification these functions are
// all prefixed with Make_* to distinguish them from the more heavyweight
// Create* resource construction functions.
///////////////////////////////////////////////////////////////////////////////

D3D12_RECT Make_D3D12_RECT(LONG width, LONG height);

D3D12_RENDER_TARGET_VIEW_DESC Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault();

D3D12_RESOURCE_BARRIER Make_D3D12_RESOURCE_BARRIER(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

D3D12_VIEWPORT Make_D3D12_VIEWPORT(FLOAT width, FLOAT height);

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
CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV(ID3D12Device* device);

// Create a root signature for rasterization that contains a constant buffer
// resource and shader resource (for a texture?). This is a reasonably simple
// root signature for some meaningful rendering.
CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV1SRV(ID3D12Device* device);

// Create a descriptor heap with 1024 entries for CBVs, SRVs, or UAVs. This
// is a relatively straightforward and simplistic manner of handling
// descriptors which has no management.
CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_CBVSRVUAV(ID3D12Device* device, UINT numDescriptors);

// Create a single sampler heap with a pre-initialized UVW wrapped sampler.
CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_1Sampler(ID3D12Device* device);

CComPtr<ID3D12Resource1> D3D12_Create_Buffer(ID3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize);

CComPtr<ID3D12Resource1> D3D12_Create_Buffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data);

CComPtr<ID3D12Resource> D3D12_Create_Sample_Texture(Direct3D12Device* device);

// Wait for a D3D12 device to finish queue execution and become idle.
void D3D12_Wait_For_GPU_Idle(Direct3D12Device* device);

// Execute a graphics command list on the provided device and wait for all
// execution to complete (GPU Idle) before returning.
void D3D12_Run_Synchronously(Direct3D12Device* device, std::function<void(ID3D12GraphicsCommandList5*)> fn);