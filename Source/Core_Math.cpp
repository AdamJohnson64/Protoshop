#include "Core_Math.h"
#define _USE_MATH_DEFINES
#include <math.h>

float2 operator*(const float& lhs, const float2& rhs) { return { lhs * rhs.X, lhs * rhs.Y }; }
float2 operator*(const float2& lhs, const float& rhs) { return { lhs.X * rhs, lhs.Y * rhs }; }
float2 operator+(const float2& lhs, const float2& rhs) { return { lhs.X + rhs.X, lhs.Y + rhs.Y }; }
float2 operator-(const float2& lhs, const float2& rhs) { return { lhs.X - rhs.X, lhs.Y - rhs.Y }; }
float Dot(const float2& lhs, const float2& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }
float Length(const float2& lhs) { return sqrtf(Dot(lhs, lhs)); }
float2 Normalize(const float2& lhs) { return lhs * (1 / Length(lhs)); }
float2 Perpendicular(const float2& lhs) { return { -lhs.Y, lhs.X }; }

float3 operator*(const float3& lhs, const float& rhs) { return { lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs }; }
float Dot(const float3& lhs, const float3& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z; }
float Length(const float3& lhs) { return sqrtf(Dot(lhs, lhs)); }
float3 Normalize(const float3& lhs) { return lhs * (1 / Length(lhs)); }

float Determinant(const matrix44& val)
{
    return (-val.M14 * (+val.M23 * (val.M31 * val.M42 - val.M32 * val.M41) - val.M33 * (val.M21 * val.M42 - val.M22 * val.M41) + val.M43 * (val.M21 * val.M32 - val.M22 * val.M31)) + val.M24 * (+val.M13 * (val.M31 * val.M42 - val.M32 * val.M41) - val.M33 * (val.M11 * val.M42 - val.M12 * val.M41) + val.M43 * (val.M11 * val.M32 - val.M12 * val.M31)) - val.M34 * (+val.M13 * (val.M21 * val.M42 - val.M22 * val.M41) - val.M23 * (val.M11 * val.M42 - val.M12 * val.M41) + val.M43 * (val.M11 * val.M22 - val.M12 * val.M21)) + val.M44 * (+val.M13 * (val.M21 * val.M32 - val.M22 * val.M31) - val.M23 * (val.M11 * val.M32 - val.M12 * val.M31) + val.M33 * (val.M11 * val.M22 - val.M12 * val.M21)));
}

matrix44 Invert(const matrix44& val)
{
    float invdet = 1 / Determinant(val);
    return matrix44 {
        invdet * (+(+val.M42 * (val.M23 * val.M34 - val.M33 * val.M24) - val.M43 * (val.M22 * val.M34 - val.M32 * val.M24) + val.M44 * (val.M22 * val.M33 - val.M32 * val.M23))),
        invdet * (-(+val.M42 * (val.M13 * val.M34 - val.M33 * val.M14) - val.M43 * (val.M12 * val.M34 - val.M32 * val.M14) + val.M44 * (val.M12 * val.M33 - val.M32 * val.M13))),
        invdet * (+(+val.M42 * (val.M13 * val.M24 - val.M23 * val.M14) - val.M43 * (val.M12 * val.M24 - val.M22 * val.M14) + val.M44 * (val.M12 * val.M23 - val.M22 * val.M13))),
        invdet * (-(+val.M32 * (val.M13 * val.M24 - val.M23 * val.M14) - val.M33 * (val.M12 * val.M24 - val.M22 * val.M14) + val.M34 * (val.M12 * val.M23 - val.M22 * val.M13))),
        invdet * (-(+val.M41 * (val.M23 * val.M34 - val.M33 * val.M24) - val.M43 * (val.M21 * val.M34 - val.M31 * val.M24) + val.M44 * (val.M21 * val.M33 - val.M31 * val.M23))),
        invdet * (+(+val.M41 * (val.M13 * val.M34 - val.M33 * val.M14) - val.M43 * (val.M11 * val.M34 - val.M31 * val.M14) + val.M44 * (val.M11 * val.M33 - val.M31 * val.M13))),
        invdet * (-(+val.M41 * (val.M13 * val.M24 - val.M23 * val.M14) - val.M43 * (val.M11 * val.M24 - val.M21 * val.M14) + val.M44 * (val.M11 * val.M23 - val.M21 * val.M13))),
        invdet * (+(+val.M31 * (val.M13 * val.M24 - val.M23 * val.M14) - val.M33 * (val.M11 * val.M24 - val.M21 * val.M14) + val.M34 * (val.M11 * val.M23 - val.M21 * val.M13))),
        invdet * (+(+val.M41 * (val.M22 * val.M34 - val.M32 * val.M24) - val.M42 * (val.M21 * val.M34 - val.M31 * val.M24) + val.M44 * (val.M21 * val.M32 - val.M31 * val.M22))),
        invdet * (-(+val.M41 * (val.M12 * val.M34 - val.M32 * val.M14) - val.M42 * (val.M11 * val.M34 - val.M31 * val.M14) + val.M44 * (val.M11 * val.M32 - val.M31 * val.M12))),
        invdet * (+(+val.M41 * (val.M12 * val.M24 - val.M22 * val.M14) - val.M42 * (val.M11 * val.M24 - val.M21 * val.M14) + val.M44 * (val.M11 * val.M22 - val.M21 * val.M12))),
        invdet * (-(+val.M31 * (val.M12 * val.M24 - val.M22 * val.M14) - val.M32 * (val.M11 * val.M24 - val.M21 * val.M14) + val.M34 * (val.M11 * val.M22 - val.M21 * val.M12))),
        invdet * (-(+val.M41 * (val.M22 * val.M33 - val.M32 * val.M23) - val.M42 * (val.M21 * val.M33 - val.M31 * val.M23) + val.M43 * (val.M21 * val.M32 - val.M31 * val.M22))),
        invdet * (+(+val.M41 * (val.M12 * val.M33 - val.M32 * val.M13) - val.M42 * (val.M11 * val.M33 - val.M31 * val.M13) + val.M43 * (val.M11 * val.M32 - val.M31 * val.M12))),
        invdet * (-(+val.M41 * (val.M12 * val.M23 - val.M22 * val.M13) - val.M42 * (val.M11 * val.M23 - val.M21 * val.M13) + val.M43 * (val.M11 * val.M22 - val.M21 * val.M12))),
        invdet * (+(+val.M31 * (val.M12 * val.M23 - val.M22 * val.M13) - val.M32 * (val.M11 * val.M23 - val.M21 * val.M13) + val.M33 * (val.M11 * val.M22 - val.M21 * val.M12)))
    };
}

matrix44 operator*(const matrix44& a, const matrix44& b)
{
    return matrix44 {
        a.M11 * b.M11 + a.M12 * b.M21 + a.M13 * b.M31 + a.M14 * b.M41,
        a.M11 * b.M12 + a.M12 * b.M22 + a.M13 * b.M32 + a.M14 * b.M42,
        a.M11 * b.M13 + a.M12 * b.M23 + a.M13 * b.M33 + a.M14 * b.M43,
        a.M11 * b.M14 + a.M12 * b.M24 + a.M13 * b.M34 + a.M14 * b.M44,
        a.M21 * b.M11 + a.M22 * b.M21 + a.M23 * b.M31 + a.M24 * b.M41,
        a.M21 * b.M12 + a.M22 * b.M22 + a.M23 * b.M32 + a.M24 * b.M42,
        a.M21 * b.M13 + a.M22 * b.M23 + a.M23 * b.M33 + a.M24 * b.M43,
        a.M21 * b.M14 + a.M22 * b.M24 + a.M23 * b.M34 + a.M24 * b.M44,
        a.M31 * b.M11 + a.M32 * b.M21 + a.M33 * b.M31 + a.M34 * b.M41,
        a.M31 * b.M12 + a.M32 * b.M22 + a.M33 * b.M32 + a.M34 * b.M42,
        a.M31 * b.M13 + a.M32 * b.M23 + a.M33 * b.M33 + a.M34 * b.M43,
        a.M31 * b.M14 + a.M32 * b.M24 + a.M33 * b.M34 + a.M34 * b.M44,
        a.M41 * b.M11 + a.M42 * b.M21 + a.M43 * b.M31 + a.M44 * b.M41,
        a.M41 * b.M12 + a.M42 * b.M22 + a.M43 * b.M32 + a.M44 * b.M42,
        a.M41 * b.M13 + a.M42 * b.M23 + a.M43 * b.M33 + a.M44 * b.M43,
        a.M41 * b.M14 + a.M42 * b.M24 + a.M43 * b.M34 + a.M44 * b.M44 };
}

matrix44 CreateMatrixRotation(const Quaternion& q)
{
    float m11 = 1 - 2 * q.Y * q.Y - 2 * q.Z * q.Z;
    float m12 = 2 * (q.X * q.Y + q.Z * q.W);
    float m13 = 2 * (q.X * q.Z - q.Y * q.W);
    float m21 = 2 * (q.X * q.Y - q.Z * q.W);
    float m22 = 1 - 2 * q.X * q.X - 2 * q.Z * q.Z;
    float m23 = 2 * (q.Y * q.Z + q.X * q.W);
    float m31 = 2 * (q.X * q.Z + q.Y * q.W);
    float m32 = 2 * (q.Y * q.Z - q.X * q.W);
    float m33 = 1 - 2 * q.X * q.X - 2 * q.Y * q.Y;
    return matrix44 { m11, m12, m13, 0, m21, m22, m23, 0, m31, m32, m33, 0, 0, 0, 0, 1 };
}

Quaternion CreateQuaternionRotation(const float3& axis, float angle)
{
    float radians_over_2 = (angle * M_PI / 180) / 2;
    float sin = sinf(radians_over_2);
    float3 normalized_axis = Normalize(axis);
    return { normalized_axis.X * sin, normalized_axis.Y * sin, normalized_axis.Z * sin, cosf(radians_over_2) };
}

Quaternion CreateQuaternionRotation(const matrix44& orthonormal)
{
    float w = sqrtf(1 + orthonormal.M11 + orthonormal.M22 + orthonormal.M33) / 2;
    float x = (orthonormal.M23 - orthonormal.M32) / (4 * w);
    float y = (orthonormal.M31 - orthonormal.M13) / (4 * w);
    float z = (orthonormal.M12 - orthonormal.M21) / (4 * w);
    return Quaternion { x, y, z, w };
}

Quaternion Multiply(Quaternion a, Quaternion b)
{
    float t0 = b.W * a.W - b.X * a.X - b.Y * a.Y - b.Z * a.Z;
    float t1 = b.W * a.X + b.X * a.W - b.Y * a.Z + b.Z * a.Y;
    float t2 = b.W * a.Y + b.X * a.Z + b.Y * a.W - b.Z * a.X;
    float t3 = b.W * a.Z - b.X * a.Y + b.Y * a.X + b.Z * a.W;
    float ln = 1 / sqrtf(t0 * t0 + t1 * t1 + t2 * t2 + t3 * t3);
    t0 *= ln; t1 *= ln; t2 *= ln; t3 *= ln;
    return Quaternion { t1, t2, t3, t0 };
}