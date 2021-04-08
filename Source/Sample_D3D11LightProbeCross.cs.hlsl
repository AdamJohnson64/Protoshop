cbuffer Constants
{
    float4x4 TransformClipToWorld;
    float2 WindowDimensions;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Form up normalized screen coordinates.
    const float2 Normalized = mad(float2(2, -2) / WindowDimensions, float2(dispatchThreadId.xy), float2(-1, 1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form the world ray.
    float4 front = mul(TransformClipToWorld, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(Normalized.xy, 1, 1));
    back /= back.w;
    float3 origin = front.xyz;
    float3 direction = back.xyz - front.xyz;
    ////////////////////////////////////////////////////////////////////////////////
    // Project the direction onto the unit cube (Chebyshev Norm).
    ////////////////////////////////////////////////////////////////////////////////
    float chebychev = max(abs(direction.x), max(abs(direction.y), abs(direction.z)));
    direction /= chebychev;
    ////////////////////////////////////////////////////////////////////////////////
    // Get the texture and face dimensions.
    ////////////////////////////////////////////////////////////////////////////////
    uint Width, Height, NumberOfLevels;
    userImage.GetDimensions(0, Width, Height, NumberOfLevels);
    uint FaceSize = Height / 4;
    ////////////////////////////////////////////////////////////////////////////////
    // Determine the face to look up.
    ////////////////////////////////////////////////////////////////////////////////
    float4 rgba = float4(1, 0, 0, 0);
    if (abs(direction.x) > abs(direction.y) && abs(direction.x) > abs(direction.z))
    {
        // Major X Direction; left and right of the cube.
        if (direction.x > 0)
        {
            // Right face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 2 + direction.z * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }
        else
        {
            // Left face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 0 - direction.z * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }

    }
    if (abs(direction.y) > abs(direction.x) && abs(direction.y) > abs(direction.z))
    {
        // Major Y Direction; top and bottom of the cube.
        if (direction.y > 0)
        {
            // Top face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 0 - direction.z * FaceSize / 2, 0));
        }
        else
        {
            // Bottom face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 2 + direction.z * FaceSize / 2, 0));
        }
    }
    if (abs(direction.z) > abs(direction.x) && abs(direction.z) > abs(direction.y))
    {
        // Major Z Direction; front and back of the cube.
        if (direction.z > 0)
        {
            // Back face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 3 + direction.y * FaceSize / 2, 0));
        }
        else
        {
            // Front face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }
    }
    renderTarget[dispatchThreadId.xy] = rgba * 2;
}