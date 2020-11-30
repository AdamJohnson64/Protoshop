#include "Core_D3D12Util.h"
#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include <array>
#include <assert.h>
#include <atlbase.h>
#include <functional>

D3D12_RECT Make_D3D12_RECT(LONG width, LONG height) {
  D3D12_RECT desc = {};
  desc.right = width;
  desc.bottom = height;
  return desc;
}

D3D12_RENDER_TARGET_VIEW_DESC
Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault() {
  D3D12_RENDER_TARGET_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
  return desc;
}

D3D12_RESOURCE_BARRIER Make_D3D12_RESOURCE_BARRIER(ID3D12Resource *resource,
                                                   D3D12_RESOURCE_STATES from,
                                                   D3D12_RESOURCE_STATES to) {
  D3D12_RESOURCE_BARRIER desc = {};
  desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  desc.Transition.pResource = resource;
  desc.Transition.StateBefore = from;
  desc.Transition.StateAfter = to;
  desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  return desc;
}

D3D12_VIEWPORT Make_D3D12_VIEWPORT(FLOAT width, FLOAT height) {
  D3D12_VIEWPORT desc = {};
  desc.Width = RENDERTARGET_WIDTH;
  desc.Height = RENDERTARGET_HEIGHT;
  desc.MaxDepth = 1.0f;
  return desc;
}

CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV(ID3D12Device *device) {
  CComPtr<ID3DBlob> pD3D12BlobSignature;
  CComPtr<ID3DBlob> pD3D12BlobError;
  D3D12_ROOT_SIGNATURE_DESC descSignature = {};
  D3D12_ROOT_PARAMETER parameters = {};
  D3D12_DESCRIPTOR_RANGE range = {};
  range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  range.NumDescriptors = 1;
  parameters.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters.DescriptorTable.pDescriptorRanges = &range;
  parameters.DescriptorTable.NumDescriptorRanges = 1;
  parameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  descSignature.pParameters = &parameters;
  descSignature.NumParameters = 1;
  descSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1,
                              &pD3D12BlobSignature, &pD3D12BlobError);
  if (nullptr != pD3D12BlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3D12BlobError->GetBufferPointer()));
  }
  CComPtr<ID3D12RootSignature> pRootSignature;
  TRYD3D(device->CreateRootSignature(0, pD3D12BlobSignature->GetBufferPointer(),
                                     pD3D12BlobSignature->GetBufferSize(),
                                     __uuidof(ID3D12RootSignature),
                                     (void **)&pRootSignature.p));
  return pRootSignature;
}

CComPtr<ID3D12RootSignature>
D3D12_Create_Signature_1CBV1SRV(ID3D12Device *device) {
  CComPtr<ID3DBlob> pD3D12BlobSignature;
  CComPtr<ID3DBlob> pD3D12BlobError;
  D3D12_ROOT_SIGNATURE_DESC descSignature = {};
  std::array<D3D12_ROOT_PARAMETER, 3> parameters = {};
  std::array<D3D12_DESCRIPTOR_RANGE, 3> range = {};
  range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  range[0].NumDescriptors = 1;
  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[0].DescriptorTable.pDescriptorRanges = &range[0];
  parameters[0].DescriptorTable.NumDescriptorRanges = 1;
  parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  range[1].NumDescriptors = 1;
  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[1].DescriptorTable.pDescriptorRanges = &range[1];
  parameters[1].DescriptorTable.NumDescriptorRanges = 1;
  parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
  range[2].NumDescriptors = 1;
  parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  parameters[2].DescriptorTable.pDescriptorRanges = &range[2];
  parameters[2].DescriptorTable.NumDescriptorRanges = 1;
  parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  descSignature.pParameters = &parameters[0];
  descSignature.NumParameters = 3;
  descSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1,
                              &pD3D12BlobSignature, &pD3D12BlobError);
  if (nullptr != pD3D12BlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3D12BlobError->GetBufferPointer()));
  }
  CComPtr<ID3D12RootSignature> pRootSignature;
  TRYD3D(device->CreateRootSignature(0, pD3D12BlobSignature->GetBufferPointer(),
                                     pD3D12BlobSignature->GetBufferSize(),
                                     __uuidof(ID3D12RootSignature),
                                     (void **)&pRootSignature.p));
  return pRootSignature;
}

CComPtr<ID3D12DescriptorHeap>
D3D12_Create_DescriptorHeap_CBVSRVUAV(ID3D12Device *device,
                                      UINT numDescriptors) {
  D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
  descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  descDescriptorHeap.NumDescriptors = numDescriptors;
  descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  CComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
  TRYD3D(device->CreateDescriptorHeap(&descDescriptorHeap,
                                      __uuidof(ID3D12DescriptorHeap),
                                      (void **)&pDescriptorHeap.p));
  pDescriptorHeap->SetName(L"D3D12DescriptorHeap (1024 CBV/SRV/UAV)");
  return pDescriptorHeap;
}

CComPtr<ID3D12DescriptorHeap>
D3D12_Create_DescriptorHeap_1Sampler(ID3D12Device *device) {
  CComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
  {
    D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
    descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descDescriptorHeap.NumDescriptors = 1;
    descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    TRYD3D(device->CreateDescriptorHeap(&descDescriptorHeap,
                                        __uuidof(ID3D12DescriptorHeap),
                                        (void **)&pDescriptorHeap));
    pDescriptorHeap->SetName(L"D3D12DescriptorHeap (1 Sampler)");
  }
  // Convenience: Initialize this single sampler with all default settings and
  // wrapping mode in all dimensions.
  {
    D3D12_SAMPLER_DESC descSampler = {};
    descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    device->CreateSampler(
        &descSampler, pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
  }
  return pDescriptorHeap;
}

CComPtr<ID3D12Resource1>
D3D12_Create_Buffer(ID3D12Device *device, D3D12_RESOURCE_FLAGS flags,
                    D3D12_RESOURCE_STATES state, uint32_t bufferSize,
                    const D3D12_HEAP_PROPERTIES *heap) {
  D3D12_RESOURCE_DESC descResource = {};
  descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  descResource.Width = bufferSize;
  descResource.Height = 1;
  descResource.DepthOrArraySize = 1;
  descResource.MipLevels = 1;
  descResource.SampleDesc.Count = 1;
  descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  descResource.Flags = flags;
  CComPtr<ID3D12Resource1> pD3D12Resource;
  TRYD3D(device->CreateCommittedResource(
      heap, D3D12_HEAP_FLAG_NONE, &descResource, state, nullptr,
      __uuidof(ID3D12Resource1), (void **)&pD3D12Resource.p));
  TRYD3D(pD3D12Resource->SetName(L"D3D12CreateBuffer"));
  return pD3D12Resource;
}

CComPtr<ID3D12Resource1> D3D12_Create_Buffer(ID3D12Device *device,
                                             D3D12_RESOURCE_FLAGS flags,
                                             D3D12_RESOURCE_STATES state,
                                             uint32_t bufferSize) {
  D3D12_HEAP_PROPERTIES descHeapDefault = {};
  descHeapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
  return D3D12_Create_Buffer(device, flags, state, bufferSize,
                             &descHeapDefault);
}

CComPtr<ID3D12Resource1>
D3D12_Create_Buffer(Direct3D12Device *device, D3D12_RESOURCE_FLAGS flags,
                    D3D12_RESOURCE_STATES state, uint32_t bufferSize,
                    uint32_t dataSize, const void *data) {
  CComPtr<ID3D12Resource1> pD3D12Resource = D3D12_Create_Buffer(
      device->m_pDevice, flags, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);
  {
    CComPtr<ID3D12Resource1> pD3D12ResourceUpload;
    {
      D3D12_HEAP_PROPERTIES descHeapUpload = {};
      descHeapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
      pD3D12ResourceUpload = D3D12_Create_Buffer(
          device->m_pDevice, D3D12_RESOURCE_FLAG_NONE,
          D3D12_RESOURCE_STATE_GENERIC_READ, bufferSize, &descHeapUpload);
    }
    void *pMapped = nullptr;
    TRYD3D(pD3D12ResourceUpload->Map(0, nullptr, &pMapped));
    memcpy(pMapped, data, dataSize);
    pD3D12ResourceUpload->Unmap(0, nullptr);
    // Copy this staging buffer to the GPU-only buffer.
    D3D12_Run_Synchronously(
        device, [&](ID3D12GraphicsCommandList5 *uploadCommandList) {
          uploadCommandList->CopyResource(pD3D12Resource, pD3D12ResourceUpload);
          uploadCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(
                     pD3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, state));
        });
  }
  return pD3D12Resource;
}

CComPtr<ID3D12Resource> D3D12_Create_Sample_Texture(Direct3D12Device *device) {
  CComPtr<ID3D12Resource> resource;
  const uint32_t imageWidth = 256;
  const uint32_t imageHeight = 256;
  {
    D3D12_HEAP_PROPERTIES descHeap = {};
    descHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descResource.Width = imageWidth;
    descResource.Height = imageHeight;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    descResource.SampleDesc.Count = 1;
    TRYD3D(device->m_pDevice->CreateCommittedResource(
        &descHeap, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &descResource, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        __uuidof(ID3D12Resource), (void **)&resource.p));
  }
  struct PixelBGRA {
    uint8_t B, G, R, A;
  };
  const uint32_t pixelWidth = sizeof(PixelBGRA);
  const uint32_t imageStride =
      AlignUp(pixelWidth * imageWidth, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  {
    D3D12_HEAP_PROPERTIES descHeap = {};
    descHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    descResource.Width = imageStride * imageHeight;
    descResource.Height = 1;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.Format = DXGI_FORMAT_UNKNOWN;
    descResource.SampleDesc.Count = 1;
    descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    CComPtr<ID3D12Resource> pResourceUpload;
    TRYD3D(device->m_pDevice->CreateCommittedResource(
        &descHeap, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &descResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        __uuidof(ID3D12Resource), (void **)&pResourceUpload.p));
    void *pData = {};
    TRYD3D(pResourceUpload->Map(0, nullptr, &pData));
    Image_Fill_Sample(pData, imageWidth, imageHeight, imageStride);
    pResourceUpload->Unmap(0, nullptr);
    // Copy this staging buffer to the GPU-only buffer.
    D3D12_Run_Synchronously(
        device, [&](ID3D12GraphicsCommandList5 *uploadCommandList) {
          D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
          srcLocation.pResource = pResourceUpload;
          srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
          srcLocation.PlacedFootprint.Footprint.Format =
              DXGI_FORMAT_B8G8R8A8_UNORM;
          srcLocation.PlacedFootprint.Footprint.Width = imageWidth;
          srcLocation.PlacedFootprint.Footprint.Height = imageHeight;
          srcLocation.PlacedFootprint.Footprint.Depth = 1;
          srcLocation.PlacedFootprint.Footprint.RowPitch = imageStride;
          D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
          dstLocation.pResource = resource;
          dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
          dstLocation.SubresourceIndex = 0;
          uploadCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0,
                                               &srcLocation, nullptr);
          // uploadCommandList->CopyResource(m_pTexture, pResourceUpload);
          uploadCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resource,
                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                              D3D12_RESOURCE_STATE_COMMON));
        });
  }
  return resource;
}

void D3D12_Wait_For_GPU_Idle(Direct3D12Device *device) {
  CComPtr<ID3D12Fence> pD3D12Fence;
  TRYD3D(device->m_pDevice->CreateFence(
      1, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&pD3D12Fence));
  pD3D12Fence->SetName(L"D3D12Fence");
  device->m_pCommandQueue->Signal(pD3D12Fence, 123);
  // Set Fence to zero and wait for queue empty.
  HANDLE hWait = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  TRYD3D(pD3D12Fence->SetEventOnCompletion(123, hWait));
  WaitForSingleObject(hWait, INFINITE);
  CloseHandle(hWait);
}

void D3D12_Run_Synchronously(
    Direct3D12Device *device,
    std::function<void(ID3D12GraphicsCommandList5 *)> fn) {
  CComPtr<ID3D12GraphicsCommandList5> pD3D12GraphicsCommandList;
  {
    CComPtr<ID3D12CommandAllocator> pD3D12CommandAllocator;
    TRYD3D(device->m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator),
        (void **)&pD3D12CommandAllocator));
    pD3D12CommandAllocator->SetName(L"D3D12CommandAllocator");
    TRYD3D(device->m_pDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, pD3D12CommandAllocator, nullptr,
        __uuidof(ID3D12GraphicsCommandList5),
        (void **)&pD3D12GraphicsCommandList));
    pD3D12GraphicsCommandList->SetName(L"D3D12GraphicsCommandList");
  }
  fn(pD3D12GraphicsCommandList);
  pD3D12GraphicsCommandList->Close();
  device->m_pCommandQueue->ExecuteCommandLists(
      1, (ID3D12CommandList **)&pD3D12GraphicsCommandList.p);
  D3D12_Wait_For_GPU_Idle(device);
};