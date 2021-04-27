#include "Sample_D3D11ComputeVector.inc"

RWTexture2D<float4> renderTarget;
ByteAddressBuffer shape : register(t0);

groupshared int edgeClipCount;
groupshared int edgeClip[4096];

float4 Sample(float2 samplePoint) {
  // First 4 bytes; uint count of float2 points
  uint pointCount = asuint(shape.Load(0));
  // Iterate through all edges.
  bool sampleIsInside = false;
  //  for (uint i = 0; i < pointCount; ++i) {
  for (uint edgeListIndex = 0; edgeListIndex < edgeClipCount; ++edgeListIndex) {
    uint edgeIndex = edgeClip[edgeListIndex];
    // Get the start and end point of this edge.
    float2 p1, p2;
    p1.x = asfloat(shape.Load(4 + edgeIndex * 16 + 0));
    p1.y = asfloat(shape.Load(4 + edgeIndex * 16 + 4));
    p2.x = asfloat(shape.Load(4 + edgeIndex * 16 + 8));
    p2.y = asfloat(shape.Load(4 + edgeIndex * 16 + 12));
    // Sort points into ascending Y order.
    if (p1.y > p2.y) {
      float2 tmp = p1;
      p1 = p2;
      p2 = tmp;
    }
    // If this sample is to the right of this edge then flip inside/outside.
    // If the number of crossings is odd then we must be inside the shape.
    if (samplePoint.y >= p1.y && samplePoint.y < p2.y) {
      float x = p1.x + (p2.x - p1.x) * (samplePoint.y - p1.y) / (p2.y - p1.y);
      if (samplePoint.x >= x)
        sampleIsInside = !sampleIsInside;
    }
  }
  // Use the parity to report the pixel color.
  return sampleIsInside ? float4(0, 0, 0, 1) : float4(1, 1, 1, 1);
}

struct CSInput {
  uint3 dispatchThreadId : SV_DispatchThreadID;
  uint3 groupId : SV_GroupID;
  uint groupIndex : SV_GroupIndex;
};

[numthreads(THREADCOUNT_X, THREADCOUNT_Y, 1)] void main(CSInput csin) {
  ////////////////////////////////////////////////////////////////////////////////
  // Initialize groupshared storage for the first thread only.
  if (csin.groupIndex == 0) {
    edgeClipCount = 0;
  }
  // Wait for all threads to reach here and init to synchronize.
  AllMemoryBarrierWithGroupSync();
  ////////////////////////////////////////////////////////////////////////////////
  // Walk the list of edges and extract indices for edges in this group.
  {
    uint pointCount = asuint(shape.Load(0));
    // Define the upper and lower raster bound for this thread group.
    float rasterMin = (csin.groupId.y + 0) * THREADCOUNT_Y;
    float rasterMax = (csin.groupId.y + 1) * THREADCOUNT_Y;
    // Define the number of edges to clip in one thread group.
    uint edgesPerThreadGroup = THREADCOUNT_X * THREADCOUNT_Y;
    uint edgesPerThread = pointCount / edgesPerThreadGroup;
    // If there's a remainder then add it into the earlier threads.
    if (csin.groupIndex < pointCount % edgesPerThreadGroup) {
      ++edgesPerThread;
    }
    // Iterate through all edges.
    for (uint edgeInThread = 0; edgeInThread < edgesPerThread; ++edgeInThread) {
      uint edgeIndex = edgesPerThreadGroup * edgeInThread + csin.groupIndex;
      // Get the Y span of this edge.
      float edgeY1 = asfloat(shape.Load(4 + edgeIndex * 16 + 4));
      float edgeY2 = asfloat(shape.Load(4 + edgeIndex * 16 + 12));
      // Sort into ascending Y order.
      if (edgeY1 > edgeY2) {
        float swap = edgeY1;
        edgeY1 = edgeY2;
        edgeY2 = swap;
      }
      // If this edge lies within the raster bound then record the index.
      if (edgeY1 <= rasterMax && edgeY2 >= rasterMin) {
        uint recordIndex;
        InterlockedAdd(edgeClipCount, 1, recordIndex);
        edgeClip[recordIndex] = edgeIndex;
      }
    }
  }
  // Wait for all threads to reach here for buckets to be finished.
  AllMemoryBarrierWithGroupSync();
  ////////////////////////////////////////////////////////////////////////////////
  // Now render the actual pixels.
  //renderTarget[csin.dispatchThreadId.xy] =
  //    Sample((float2)csin.dispatchThreadId);
  //return;

  ////////////////////////////////////////////////////////////////////////////////
  // 4x4 (16 TAP) Supersampling.
  const int superCountX = 4;
  const int superCountY = 4;
  const float superSpacingX = 1.0 / superCountX;
  const float superSpacingY = 1.0 / superCountY;
  const float superOffsetX = superSpacingX / 2;
  const float superOffsetY = superSpacingY / 2;
  float4 accumulateSamples;
  for (int superSampleY = 0; superSampleY < superCountY; ++superSampleY) {
    for (int superSampleX = 0; superSampleX < superCountX; ++superSampleX) {
      accumulateSamples += Sample(float2(
          csin.dispatchThreadId.x + superOffsetX + superSampleX * superSpacingX,
          csin.dispatchThreadId.y + superOffsetY +
              superSampleY * superSpacingY));
    }
  }
  accumulateSamples /= superCountX * superCountY;
  renderTarget[csin.dispatchThreadId.xy] = accumulateSamples;
}