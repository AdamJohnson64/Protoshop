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

struct matrix44
{
    float
        M11, M12, M13, M14,
        M21, M22, M23, M24,
        M31, M32, M33, M34,
        M41, M42, M43, M44;
};

float Determinant(matrix44 val);
matrix44 Invert(const matrix44& val);