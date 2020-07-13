#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Sample_DXRBase.h"
#include <array>
#include <atlbase.h>
#include <d3d12.h>

Sample_DXRBase::Sample_DXRBase(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
    m_pSwapChain(pSwapChain),
    m_pDevice(pDevice)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Create a LOCAL root signature.
    {
        uint32_t setupRange = 0;

        std::array<D3D12_DESCRIPTOR_RANGE, 8> descDescriptorRange;

        descDescriptorRange[setupRange].BaseShaderRegister = 0;
        descDescriptorRange[setupRange].NumDescriptors = 1;
        descDescriptorRange[setupRange].RegisterSpace = 0;
        descDescriptorRange[setupRange].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        descDescriptorRange[setupRange].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ++setupRange;

        descDescriptorRange[setupRange].BaseShaderRegister = 0;
        descDescriptorRange[setupRange].NumDescriptors = 1;
        descDescriptorRange[setupRange].RegisterSpace = 0;
        descDescriptorRange[setupRange].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descDescriptorRange[setupRange].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ++setupRange;

        D3D12_ROOT_PARAMETER descRootParameter = {};
        descRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        descRootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        descRootParameter.DescriptorTable.NumDescriptorRanges = setupRange;
        descRootParameter.DescriptorTable.pDescriptorRanges = &descDescriptorRange[0];

        D3D12_ROOT_SIGNATURE_DESC descSignature = {};
        descSignature.NumParameters = 1;
        descSignature.pParameters = &descRootParameter;
        descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        CComPtr<ID3DBlob> m_blob;
        CComPtr<ID3DBlob> m_blobError;
        TRYD3D(D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1_0, &m_blob, &m_blobError));
        TRYD3D(m_pDevice->GetID3D12Device()->CreateRootSignature(0, m_blob->GetBufferPointer(), m_blob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pRootSignature));
        m_pRootSignature->SetName(L"DXR Root Signature");
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create the unordered access buffer for raytracing output.
    {
        D3D12_HEAP_PROPERTIES descHeapProperties = {};
        descHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC descResource = {};
        descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        descResource.Width = RENDERTARGET_WIDTH;
        descResource.Height = RENDERTARGET_HEIGHT;
        descResource.DepthOrArraySize = 1;
        descResource.MipLevels = 1;
        descResource.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        descResource.SampleDesc.Count = 1;
        descResource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        TRYD3D(m_pDevice->GetID3D12Device()->CreateCommittedResource(&descHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &descResource, D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource1), (void**)&m_pResourceTargetUAV));
        m_pResourceTargetUAV->SetName(L"DXR Output Texture2D UAV");
    }
}