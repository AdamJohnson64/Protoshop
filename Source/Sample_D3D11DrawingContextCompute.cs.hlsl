#include "Sample_D3D11DrawingContextCompute.inc"

RWTexture2D<float4> renderTarget;
ByteAddressBuffer shape : register(t0);

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
  for (uint shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex) {
    uint shapeOffset = asuint(shape.Load(4 + 4 * shapeIndex));
    uint shapeType = asuint(shape.Load(shapeOffset + 0));
    if (shapeType == SHAPE_FILLED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4);
      float radius = asfloat(shape.Load(shapeOffset + 12));
      position -= samplePoint;
      if (abs(sqrt(dot(position, position))) < radius)
        return float4(0, 0, 0, 1);
    }
    if (shapeType == SHAPE_STROKED_CIRCLE) {
      float2 position = LoadFloat2(shape, shapeOffset + 4);
      float radius = asfloat(shape.Load(shapeOffset + 12));
      float line_half_width = asfloat(shape.Load(shapeOffset + 16));
      position -= samplePoint;
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

[numthreads(THREADCOUNT_X, THREADCOUNT_Y, 1)] void main(CSInput csin) {
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