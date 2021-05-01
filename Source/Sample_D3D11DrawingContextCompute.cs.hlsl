#include "Sample_D3D11DrawingContextCompute.inc"

RWTexture2D<float4> renderTarget;
ByteAddressBuffer shape : register(t0);

groupshared uint clippedObjectCount;
groupshared uint clippedObjects[4096];

bool InsideLine(float2 samplepoint, float2 p1, float2 p2,
                float line_half_width) {
  float2 direction = normalize(p2 - p1);
  float2 perpendicular = {-direction.y, direction.x};
  // Check if we're outide the width of the line.
  if (abs(dot(samplepoint, perpendicular) - dot(p1, perpendicular)) >
      line_half_width)
    return false;
  // Check if the sample point is before p1 (beginning of line).
  if (dot(samplepoint, -direction) - dot(p1, -direction) > line_half_width)
    return false;
  // Check if the sample point is after p2 (end of line).
  if (dot(samplepoint, direction) - dot(p2, direction) > line_half_width)
    return false;
  return true;
}

float2 LoadFloat2(ByteAddressBuffer buffer, int offset) {
  float2 data;
  data.x = asfloat(shape.Load(offset));
  data.y = asfloat(shape.Load(offset + 4));
  return data;
}

float4 Sample(float2 samplePoint) {
  // First 4 bytes; uint count of shapes.
  uint shapeCount = asuint(shape.Load(0));
  // Iterate over all shapes and intersect.
  for (uint clippedObjectIndex = 0; clippedObjectIndex < clippedObjectCount;
       ++clippedObjectIndex) {
    uint shapeOffset = clippedObjects[clippedObjectIndex];
    uint shapeType = asuint(shape.Load(shapeOffset + 0));
    if (shapeType == SHAPE_FILLED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4) - samplePoint;
      float radius = asfloat(shape.Load(shapeOffset + 12));
      if (abs(sqrt(dot(position, position))) < radius)
        return float4(0, 0, 0, 1);
    }
    if (shapeType == SHAPE_STROKED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4) - samplePoint;
      float radius = asfloat(shape.Load(shapeOffset + 12));
      float line_half_width = asfloat(shape.Load(shapeOffset + 16));
      if (abs(sqrt(dot(position, position)) - radius) < line_half_width)
        return float4(0, 0, 0, 1);
    }
    if (shapeType == SHAPE_STROKED_LINE) {
      float2 p1 = LoadFloat2(shape, shapeOffset + 4);
      float2 p2 = LoadFloat2(shape, shapeOffset + 12);
      float line_half_width = asfloat(shape.Load(shapeOffset + 20));
      if (InsideLine(samplePoint, p1, p2, line_half_width))
        return float4(0, 0, 0, 1);
    }
  }
  return float4(1, 1, 1, 1);
}

struct CSInput {
  uint3 dispatchThreadId : SV_DispatchThreadID;
  uint3 groupId : SV_GroupID;
  uint groupIndex : SV_GroupIndex;
};

[numthreads(THREADCOUNT_SQUARE, THREADCOUNT_SQUARE, 1)] void
main(CSInput csin) {
  ////////////////////////////////////////////////////////////////////////////////
  // Initialize groupshared storage for the first thread only.
  if (csin.groupIndex == 0) {
    clippedObjectCount = 0;
  }
  // Wait for all threads to reach here and init to synchronize.
  AllMemoryBarrierWithGroupSync();
  ////////////////////////////////////////////////////////////////////////////////
  // Coarse rasterize all objects to get a rough clip list.
  uint shapeCount = asuint(shape.Load(0));
  // Define the number of edges to clip in one thread group.
  uint objectsPerThreadGroup = THREADCOUNT_SQUARE * THREADCOUNT_SQUARE;
  uint objectsPerThread = shapeCount / objectsPerThreadGroup;
  // If there's a remainder then add it into the earlier threads.
  if (csin.groupIndex < shapeCount % objectsPerThreadGroup) {
    ++objectsPerThread;
  }
  // Iterate over all shapes and intersect.
  for (uint objectInThread = 0; objectInThread < objectsPerThread;
       ++objectInThread) {
    uint shapeIndex = objectsPerThreadGroup * objectInThread + csin.groupIndex;
    uint shapeOffset = asuint(shape.Load(4 + 4 * shapeIndex));
    uint shapeType = asuint(shape.Load(shapeOffset + 0));
    float2 samplePoint =
        csin.groupId * THREADCOUNT_SQUARE + THREADCOUNT_SQUARE / 2;
    if (shapeType == SHAPE_FILLED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4) - samplePoint;
      float radius = asfloat(shape.Load(shapeOffset + 12)) +
                     THREADCOUNT_SQUARE * 8 / 10; // 0.8 ~= sqrt(2) / 2
      if (!(abs(sqrt(dot(position, position))) < radius))
        continue;
    }
    if (shapeType == SHAPE_STROKED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4) - samplePoint;
      float radius = asfloat(shape.Load(shapeOffset + 12));
      float line_half_width = asfloat(shape.Load(shapeOffset + 16)) +
                              THREADCOUNT_SQUARE * 8 / 10; // 0.8 ~= sqrt(2) / 2
      if (!(abs(sqrt(dot(position, position)) - radius) < line_half_width))
        continue;
    }
    if (shapeType == SHAPE_STROKED_LINE) {
      float2 p1 = LoadFloat2(shape, shapeOffset + 4);
      float2 p2 = LoadFloat2(shape, shapeOffset + 12);
      float line_half_width = asfloat(shape.Load(shapeOffset + 20)) +
                              THREADCOUNT_SQUARE * 8 / 10; // 0.8 ~= sqrt(2) / 2
      if (!InsideLine(samplePoint, p1, p2, line_half_width))
        continue;
    }
    uint recordIndex;
    InterlockedAdd(clippedObjectCount, 1, recordIndex);
    clippedObjects[recordIndex] = shapeOffset;
  }
  // Wait for all threads to reach here and init to synchronize.
  AllMemoryBarrierWithGroupSync();
  ////////////////////////////////////////////////////////////////////////////////
  // 8x8 (64 TAP) Supersampling.
  const int superCountX = 8;
  const int superCountY = 8;
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