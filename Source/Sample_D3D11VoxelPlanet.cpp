///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Sparse "Planet" Voxelization
///////////////////////////////////////////////////////////////////////////////
// This is a fairly unique problem seen in games such as "Astroneer". The core
// problem is that a dense voxelization at such a scale is impractical.
//
// Assuming single byte density functions and a 1 meter scale we would need
// - Earth: 6-7km radius, 14km diameter
// - 14,000,000 elements cube in 3 dimensions.
// - 14,000,000^3 bytes = 2.744*10^21
// - 2.744 zettabytes
// - Utterly impractical.
//
// Propose a sparse voxelization.
// Define a 28 bit cube for a 268 million meter edge
// - Save the upper 4 bits for overflow.
// Split each node from the root into a tagged node of one of the forms:
// - Full of 0s.
// - Full of 255s.
// - A 16*16*16 array of child node pointers.
// - A 16*16*16 array of voxel bytes.
//
// Each level splits 4 bits from the root 28 bit space for a maximum depth of
// 6 levels. Voxelizations are always defined at the 7th level, and all others
// are pointer maps.
//
// Each voxel block is 4KB of bytes.
// Each pointer block is 4*4KB for 32bit pointers.
// Since all addresses are all 4KB aligned we can use a 3 byte encoding.
// Reserve these 12 bits for tag information.
// Everything is nice and VMM friendly on 4KB small pages.
// Nice and easy block allocator scheme.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include <array>
#include <atlbase.h>
#include <functional>

struct VoxelNode {
  uint8_t Value[16][16][16];
};

enum class VoxelNodeState : uint32_t {
  ALL_OUT,
  ALL_IN,
  VARIABLE
};

struct VoxelChildTag {
  VoxelNodeState Meta : 8;
  uint32_t Address : 24;
};

struct VoxelChildNode {
  VoxelChildTag Tags[16][16][16];
};

static_assert(sizeof(VoxelNode) == sizeof(uint8_t[4096]));
static_assert(sizeof(VoxelChildTag) == sizeof(uint32_t));
static_assert(sizeof(VoxelChildNode) == sizeof(uint32_t[4096]));

const int32_t REGION_SIZE = 0x10000000;
const int32_t REGION_HALF = REGION_SIZE / 2;
const double SPHERE_SIZE = 500;

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11VoxelPlanet(std::shared_ptr<Direct3D11Device> device) {

  int NumVoxelBlocks = 0;

  std::function<TVector3<double>(const TVector3<int32_t>&)> IntToDoubleSpace = [&](const TVector3<int32_t>& p) {
    return TVector3<double>{static_cast<double>(p.X - REGION_HALF), static_cast<double>(p.Y - REGION_HALF), static_cast<double>(p.Z - REGION_HALF)};
  };

  std::function<bool(const TVector3<double>&)> ProbePoint = [&](const TVector3<double>& p) {
    // Just do a big sphere for now.
    return Length(p) < SPHERE_SIZE;
  };

  std::function<VoxelNodeState(const TVector3<double>&, const TVector3<double>&)> ProbeVolume = [&](const TVector3<double>& min, const TVector3<double>& max) {
    bool crossing = false;
    for (int bz = 0; bz < 2; ++bz) {
      for (int by = 0; by < 2; ++by) {
        for (int bx = 0; bx < 2; ++bx) {
            // Get a corner of the box.
            TVector3<double> boxpoint = TVector3<double>{
              bx == 0 ? min.X : max.X,
              by == 0 ? min.Y : max.Y,
              bz == 0 ? min.Z : max.Z};
            // Compute an axis from the center of the sphere to this point.
            TVector3<double> axis = Normalize(boxpoint);
            // Project all box points onto this axis.
            double minprojection = std::numeric_limits<double>::infinity();
            double maxprojection = -std::numeric_limits<double>::infinity();
            for (int tz = 0; tz < 2; ++tz) {
              for (int ty = 0; ty < 2; ++ty) {
                for (int tx = 0; tx < 2; ++tx) {
                  TVector3<double> testpoint = TVector3<double>{
                    tx == 0 ? min.X : max.X,
                    ty == 0 ? min.Y : max.Y,
                    tz == 0 ? min.Z : max.Z};
                  minprojection = min(minprojection, Dot(testpoint, axis));
                  maxprojection = max(maxprojection, Dot(testpoint, axis));
                }
              }
            }
            // If the interval is outside the sphere then we're all out (in any case).
            if (minprojection > SPHERE_SIZE || maxprojection < -SPHERE_SIZE) {
              return VoxelNodeState::ALL_OUT;
            }
            // If either point extends outside the sphere then we're crossing.
            if (minprojection < -SPHERE_SIZE || maxprojection > SPHERE_SIZE) {
              crossing = true;
            }
        }
      }
    }
    if (crossing == false) {
      int yay = 0;
    }
    return crossing ? VoxelNodeState::VARIABLE : VoxelNodeState::ALL_IN;
  };

  std::function<void(const TVector3<int32_t>&, const TVector3<int32_t>&)> Evaluate = [&](const TVector3<int32_t>& min, const TVector3<int32_t>& max) {
    TVector3<double> fmin = IntToDoubleSpace(min);
    TVector3<double> fmax = IntToDoubleSpace(max);
    switch (ProbeVolume(fmin, fmax)) {
    case VoxelNodeState::ALL_OUT:
      return;
    case VoxelNodeState::ALL_IN:
      return;
    case VoxelNodeState::VARIABLE: {
      if (max.X - min.X <= 16 && max.Y - min.Y <= 16 && max.Z - min.Z <= 16) {
        bool allzero = true;
        bool allone = true;
        for (uint32_t bz = 0; bz < 16; ++bz) {
          for (uint32_t by = 0; by < 16; ++by) {
            for (uint32_t bx = 0; bx < 16; ++bx) {
              TVector3<int32_t> p{
                static_cast<int64_t>(min.X) + (max.X - min.X) * bx / 16,
                static_cast<int64_t>(min.Y) + (max.Y - min.Y) * by / 16,
                static_cast<int64_t>(min.Z) + (max.Z - min.Z) * bz / 16};
              bool inside = ProbePoint(IntToDoubleSpace(p));
              if (inside) allzero = false;
              if (!inside) allone = false;
            }
          }
        }
        if (!allzero && !allone) {
          //char debugme[256];
          //sprintf_s(debugme, "[%0.1f, %0.1f, %0.1f] -> [%0.1f, %0.1f, %0.1f]\n", fmin.X, fmin.Y, fmin.Z, fmax.X, fmax.Y, fmax.Z);
          //OutputDebugStringA(debugme);
          ++NumVoxelBlocks;
        }
        if (allzero && !allone) {
          int itsallzero = 0;
        }
        if (!allzero && allone) {
          int itsallone = 0;
        }
        return;
      }
      for (uint32_t bz = 0; bz < 16; ++bz) {
        for (uint32_t by = 0; by < 16; ++by) {
          for (uint32_t bx = 0; bx < 16; ++bx) {
            TVector3<int32_t> newmin{
              static_cast<int64_t>(min.X) + (max.X - min.X) * (bx + 0) / 16,
              static_cast<int64_t>(min.Y) + (max.Y - min.Y) * (by + 0) / 16,
              static_cast<int64_t>(min.Z) + (max.Z - min.Z) * (bz + 0) / 16};
            TVector3<int32_t> newmax{
              static_cast<int64_t>(min.X) + (max.X - min.X) * (bx + 1) / 16,
              static_cast<int64_t>(min.Y) + (max.Y - min.Y) * (by + 1) / 16,
              static_cast<int64_t>(min.Z) + (max.Z - min.Z) * (bz + 1) / 16};
            Evaluate(newmin, newmax);
          }
        }
      }
    }
    }
  };

  Evaluate(TVector3<int32_t>{0, 0, 0}, TVector3<int32_t>{REGION_SIZE, REGION_SIZE, REGION_SIZE});
  {
    // This should be the final number of 4KB blocks in our map.
    char debugme[256];
    sprintf_s(debugme, "NumVoxelBlocks = %d\n", NumVoxelBlocks);
    OutputDebugStringA(debugme);
  }

  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    device->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        sampleResources.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &rtvBackbuffer.p, sampleResources.DepthStencilView);
    ////////////////////////////////////////////////////////////////////////////////
    // Finished rendering.
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}