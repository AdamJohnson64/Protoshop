#pragma once

#include "Core_Object.h"
#include <atlbase.h>
#include <d3d12.h>
#include <memory>

class Direct3D12Device : public Object {
public:
  Direct3D12Device();
  CComPtr<ID3D12Device6> m_pDevice;
  CComPtr<ID3D12CommandQueue> m_pCommandQueue;
  CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapRTV;
};

std::shared_ptr<Direct3D12Device> CreateDirect3D12Device();