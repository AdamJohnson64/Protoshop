#include "Core_Math.h"
#define _USE_MATH_DEFINES
#include <math.h>

Vector2 operator*(const float& lhs, const Vector2& rhs) { return { lhs * rhs.X, lhs * rhs.Y }; }
Vector2 operator*(const Vector2& lhs, const float& rhs) { return { lhs.X * rhs, lhs.Y * rhs }; }
Vector2 operator+(const Vector2& lhs, const Vector2& rhs) { return { lhs.X + rhs.X, lhs.Y + rhs.Y }; }
Vector2 operator-(const Vector2& lhs, const Vector2& rhs) { return { lhs.X - rhs.X, lhs.Y - rhs.Y }; }
float Dot(const Vector2& lhs, const Vector2& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }
float Length(const Vector2& lhs) { return sqrtf(Dot(lhs, lhs)); }
Vector2 Normalize(const Vector2& lhs) { return lhs * (1 / Length(lhs)); }
Vector2 Perpendicular(const Vector2& lhs) { return { -lhs.Y, lhs.X }; }

Vector3 operator*(const Vector3& lhs, const float& rhs) { return { lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs }; }
float Dot(const Vector3& lhs, const Vector3& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z; }
float Length(const Vector3& lhs) { return sqrtf(Dot(lhs, lhs)); }
Vector3 Normalize(const Vector3& lhs) { return lhs * (1 / Length(lhs)); }

Vector4 operator*(const Vector4& lhs, float rhs) { return { lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs, lhs.W * rhs }; }
Vector4 operator*(float lhs, const Vector4& rhs) { return { lhs * rhs.X, lhs * rhs.Y, lhs * rhs.Z, lhs * rhs.W }; }
Vector4 operator/(const Vector4& lhs, float rhs) { return lhs * (1 / rhs); }

float Determinant(const Matrix44& lhs)
{
    return (-lhs.M14 * (+lhs.M23 * (lhs.M31 * lhs.M42 - lhs.M32 * lhs.M41) - lhs.M33 * (lhs.M21 * lhs.M42 - lhs.M22 * lhs.M41) + lhs.M43 * (lhs.M21 * lhs.M32 - lhs.M22 * lhs.M31)) + lhs.M24 * (+lhs.M13 * (lhs.M31 * lhs.M42 - lhs.M32 * lhs.M41) - lhs.M33 * (lhs.M11 * lhs.M42 - lhs.M12 * lhs.M41) + lhs.M43 * (lhs.M11 * lhs.M32 - lhs.M12 * lhs.M31)) - lhs.M34 * (+lhs.M13 * (lhs.M21 * lhs.M42 - lhs.M22 * lhs.M41) - lhs.M23 * (lhs.M11 * lhs.M42 - lhs.M12 * lhs.M41) + lhs.M43 * (lhs.M11 * lhs.M22 - lhs.M12 * lhs.M21)) + lhs.M44 * (+lhs.M13 * (lhs.M21 * lhs.M32 - lhs.M22 * lhs.M31) - lhs.M23 * (lhs.M11 * lhs.M32 - lhs.M12 * lhs.M31) + lhs.M33 * (lhs.M11 * lhs.M22 - lhs.M12 * lhs.M21)));
}

Matrix44 Invert(const Matrix44& lhs)
{
    float invdet = 1 / Determinant(lhs);
    return Matrix44 {
        invdet * (+(+lhs.M42 * (lhs.M23 * lhs.M34 - lhs.M33 * lhs.M24) - lhs.M43 * (lhs.M22 * lhs.M34 - lhs.M32 * lhs.M24) + lhs.M44 * (lhs.M22 * lhs.M33 - lhs.M32 * lhs.M23))),
        invdet * (-(+lhs.M42 * (lhs.M13 * lhs.M34 - lhs.M33 * lhs.M14) - lhs.M43 * (lhs.M12 * lhs.M34 - lhs.M32 * lhs.M14) + lhs.M44 * (lhs.M12 * lhs.M33 - lhs.M32 * lhs.M13))),
        invdet * (+(+lhs.M42 * (lhs.M13 * lhs.M24 - lhs.M23 * lhs.M14) - lhs.M43 * (lhs.M12 * lhs.M24 - lhs.M22 * lhs.M14) + lhs.M44 * (lhs.M12 * lhs.M23 - lhs.M22 * lhs.M13))),
        invdet * (-(+lhs.M32 * (lhs.M13 * lhs.M24 - lhs.M23 * lhs.M14) - lhs.M33 * (lhs.M12 * lhs.M24 - lhs.M22 * lhs.M14) + lhs.M34 * (lhs.M12 * lhs.M23 - lhs.M22 * lhs.M13))),
        invdet * (-(+lhs.M41 * (lhs.M23 * lhs.M34 - lhs.M33 * lhs.M24) - lhs.M43 * (lhs.M21 * lhs.M34 - lhs.M31 * lhs.M24) + lhs.M44 * (lhs.M21 * lhs.M33 - lhs.M31 * lhs.M23))),
        invdet * (+(+lhs.M41 * (lhs.M13 * lhs.M34 - lhs.M33 * lhs.M14) - lhs.M43 * (lhs.M11 * lhs.M34 - lhs.M31 * lhs.M14) + lhs.M44 * (lhs.M11 * lhs.M33 - lhs.M31 * lhs.M13))),
        invdet * (-(+lhs.M41 * (lhs.M13 * lhs.M24 - lhs.M23 * lhs.M14) - lhs.M43 * (lhs.M11 * lhs.M24 - lhs.M21 * lhs.M14) + lhs.M44 * (lhs.M11 * lhs.M23 - lhs.M21 * lhs.M13))),
        invdet * (+(+lhs.M31 * (lhs.M13 * lhs.M24 - lhs.M23 * lhs.M14) - lhs.M33 * (lhs.M11 * lhs.M24 - lhs.M21 * lhs.M14) + lhs.M34 * (lhs.M11 * lhs.M23 - lhs.M21 * lhs.M13))),
        invdet * (+(+lhs.M41 * (lhs.M22 * lhs.M34 - lhs.M32 * lhs.M24) - lhs.M42 * (lhs.M21 * lhs.M34 - lhs.M31 * lhs.M24) + lhs.M44 * (lhs.M21 * lhs.M32 - lhs.M31 * lhs.M22))),
        invdet * (-(+lhs.M41 * (lhs.M12 * lhs.M34 - lhs.M32 * lhs.M14) - lhs.M42 * (lhs.M11 * lhs.M34 - lhs.M31 * lhs.M14) + lhs.M44 * (lhs.M11 * lhs.M32 - lhs.M31 * lhs.M12))),
        invdet * (+(+lhs.M41 * (lhs.M12 * lhs.M24 - lhs.M22 * lhs.M14) - lhs.M42 * (lhs.M11 * lhs.M24 - lhs.M21 * lhs.M14) + lhs.M44 * (lhs.M11 * lhs.M22 - lhs.M21 * lhs.M12))),
        invdet * (-(+lhs.M31 * (lhs.M12 * lhs.M24 - lhs.M22 * lhs.M14) - lhs.M32 * (lhs.M11 * lhs.M24 - lhs.M21 * lhs.M14) + lhs.M34 * (lhs.M11 * lhs.M22 - lhs.M21 * lhs.M12))),
        invdet * (-(+lhs.M41 * (lhs.M22 * lhs.M33 - lhs.M32 * lhs.M23) - lhs.M42 * (lhs.M21 * lhs.M33 - lhs.M31 * lhs.M23) + lhs.M43 * (lhs.M21 * lhs.M32 - lhs.M31 * lhs.M22))),
        invdet * (+(+lhs.M41 * (lhs.M12 * lhs.M33 - lhs.M32 * lhs.M13) - lhs.M42 * (lhs.M11 * lhs.M33 - lhs.M31 * lhs.M13) + lhs.M43 * (lhs.M11 * lhs.M32 - lhs.M31 * lhs.M12))),
        invdet * (-(+lhs.M41 * (lhs.M12 * lhs.M23 - lhs.M22 * lhs.M13) - lhs.M42 * (lhs.M11 * lhs.M23 - lhs.M21 * lhs.M13) + lhs.M43 * (lhs.M11 * lhs.M22 - lhs.M21 * lhs.M12))),
        invdet * (+(+lhs.M31 * (lhs.M12 * lhs.M23 - lhs.M22 * lhs.M13) - lhs.M32 * (lhs.M11 * lhs.M23 - lhs.M21 * lhs.M13) + lhs.M33 * (lhs.M11 * lhs.M22 - lhs.M21 * lhs.M12)))
    };
}

Vector4 Transform(const Matrix44& lhs, const Vector4& rhs)
{
    return {
        lhs.M11 * rhs.X + lhs.M21 * rhs.Y + lhs.M31 * rhs.Z + lhs.M41 * rhs.W,
        lhs.M12 * rhs.X + lhs.M22 * rhs.Y + lhs.M32 * rhs.Z + lhs.M42 * rhs.W,
        lhs.M13 * rhs.X + lhs.M23 * rhs.Y + lhs.M33 * rhs.Z + lhs.M43 * rhs.W,
        lhs.M14 * rhs.X + lhs.M24 * rhs.Y + lhs.M34 * rhs.Z + lhs.M44 * rhs.W };
}

Matrix44 operator*(const Matrix44& lhs, const Matrix44& rhs)
{
    return Matrix44 {
        lhs.M11 * rhs.M11 + lhs.M12 * rhs.M21 + lhs.M13 * rhs.M31 + lhs.M14 * rhs.M41,
        lhs.M11 * rhs.M12 + lhs.M12 * rhs.M22 + lhs.M13 * rhs.M32 + lhs.M14 * rhs.M42,
        lhs.M11 * rhs.M13 + lhs.M12 * rhs.M23 + lhs.M13 * rhs.M33 + lhs.M14 * rhs.M43,
        lhs.M11 * rhs.M14 + lhs.M12 * rhs.M24 + lhs.M13 * rhs.M34 + lhs.M14 * rhs.M44,
        lhs.M21 * rhs.M11 + lhs.M22 * rhs.M21 + lhs.M23 * rhs.M31 + lhs.M24 * rhs.M41,
        lhs.M21 * rhs.M12 + lhs.M22 * rhs.M22 + lhs.M23 * rhs.M32 + lhs.M24 * rhs.M42,
        lhs.M21 * rhs.M13 + lhs.M22 * rhs.M23 + lhs.M23 * rhs.M33 + lhs.M24 * rhs.M43,
        lhs.M21 * rhs.M14 + lhs.M22 * rhs.M24 + lhs.M23 * rhs.M34 + lhs.M24 * rhs.M44,
        lhs.M31 * rhs.M11 + lhs.M32 * rhs.M21 + lhs.M33 * rhs.M31 + lhs.M34 * rhs.M41,
        lhs.M31 * rhs.M12 + lhs.M32 * rhs.M22 + lhs.M33 * rhs.M32 + lhs.M34 * rhs.M42,
        lhs.M31 * rhs.M13 + lhs.M32 * rhs.M23 + lhs.M33 * rhs.M33 + lhs.M34 * rhs.M43,
        lhs.M31 * rhs.M14 + lhs.M32 * rhs.M24 + lhs.M33 * rhs.M34 + lhs.M34 * rhs.M44,
        lhs.M41 * rhs.M11 + lhs.M42 * rhs.M21 + lhs.M43 * rhs.M31 + lhs.M44 * rhs.M41,
        lhs.M41 * rhs.M12 + lhs.M42 * rhs.M22 + lhs.M43 * rhs.M32 + lhs.M44 * rhs.M42,
        lhs.M41 * rhs.M13 + lhs.M42 * rhs.M23 + lhs.M43 * rhs.M33 + lhs.M44 * rhs.M43,
        lhs.M41 * rhs.M14 + lhs.M42 * rhs.M24 + lhs.M43 * rhs.M34 + lhs.M44 * rhs.M44 };
}

Matrix44 CreateMatrixRotation(const Quaternion& q)
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
    return Matrix44 { m11, m12, m13, 0, m21, m22, m23, 0, m31, m32, m33, 0, 0, 0, 0, 1 };
}

Quaternion CreateQuaternionRotation(const Vector3& axis, float angle)
{
    float radians_over_2 = (angle * M_PI / 180) / 2;
    float sin = sinf(radians_over_2);
    Vector3 normalized_axis = Normalize(axis);
    return { normalized_axis.X * sin, normalized_axis.Y * sin, normalized_axis.Z * sin, cosf(radians_over_2) };
}

Quaternion CreateQuaternionRotation(const Matrix44& orthonormal)
{
    float w = sqrtf(1 + orthonormal.M11 + orthonormal.M22 + orthonormal.M33) / 2;
    float x = (orthonormal.M23 - orthonormal.M32) / (4 * w);
    float y = (orthonormal.M31 - orthonormal.M13) / (4 * w);
    float z = (orthonormal.M12 - orthonormal.M21) / (4 * w);
    return Quaternion { x, y, z, w };
}

Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs)
{
    float t0 = rhs.W * lhs.W - rhs.X * lhs.X - rhs.Y * lhs.Y - rhs.Z * lhs.Z;
    float t1 = rhs.W * lhs.X + rhs.X * lhs.W - rhs.Y * lhs.Z + rhs.Z * lhs.Y;
    float t2 = rhs.W * lhs.Y + rhs.X * lhs.Z + rhs.Y * lhs.W - rhs.Z * lhs.X;
    float t3 = rhs.W * lhs.Z - rhs.X * lhs.Y + rhs.Y * lhs.X + rhs.Z * lhs.W;
    float ln = 1 / sqrtf(t0 * t0 + t1 * t1 + t2 * t2 + t3 * t3);
    t0 *= ln; t1 *= ln; t2 *= ln; t3 *= ln;
    return Quaternion { t1, t2, t3, t0 };
}

Matrix44 CreateProjection(float near_plane, float far_plane, float fov_horiz, float fov_vert)
{
    float w = 1 / tanf(fov_horiz * 0.5);  // 1/tan(x) == cot(x)
    float h = 1 / tanf(fov_vert * 0.5);   // 1/tan(x) == cot(x)
    float Q = far_plane / (far_plane - near_plane);
    return {
        w, 0, 0, 0,
        0, h, 0, 0,
        0, 0, Q, 1,
        0, 0, -Q * near_plane, 0
    };
}