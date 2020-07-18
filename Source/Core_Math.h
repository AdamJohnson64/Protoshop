#pragma once

struct float2
{
    float X, Y;
};

float2 operator*(const float& lhs, const float2& rhs);
float2 operator*(const float2& lhs, const float& rhs);
float2 operator+(const float2& lhs, const float2& rhs);
float2 operator-(const float2& lhs, const float2& rhs);
float Dot(const float2& lhs, const float2& rhs);
float Length(const float2& lhs);
float2 Normalize(const float2& lhs);
float2 Perpendicular(const float2& lhs);

struct float3
{
    float X, Y, Z;
};

float Dot(const float3& lhs, const float3& rhs);
float Length(const float3& lhs);
float3 Normalize(const float3& lhs);

struct matrix44
{
    float
        M11, M12, M13, M14,
        M21, M22, M23, M24,
        M31, M32, M33, M34,
        M41, M42, M43, M44;
};

float Determinant(const matrix44& val);
matrix44 Invert(const matrix44& val);
matrix44 operator*(const matrix44& lhs, const matrix44& rhs);

struct Quaternion
{
    float X, Y, Z, W;
};

matrix44 CreateMatrixRotation(const Quaternion& q);
Quaternion CreateQuaternionRotation(const float3& axis, float angle);
Quaternion CreateQuaternionRotation(const matrix44& orthonormal);
Quaternion Multiply(const Quaternion& a, const Quaternion& b);
