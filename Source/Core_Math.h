#pragma once

struct Vector2
{
    float X, Y;
};

Vector2 operator*(const float& lhs, const Vector2& rhs);
Vector2 operator*(const Vector2& lhs, const float& rhs);
Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);
float Dot(const Vector2& lhs, const Vector2& rhs);
float Length(const Vector2& lhs);
Vector2 Normalize(const Vector2& lhs);
Vector2 Perpendicular(const Vector2& lhs);

struct Vector3
{
    float X, Y, Z;
};

float Dot(const Vector3& lhs, const Vector3& rhs);
float Length(const Vector3& lhs);
Vector3 Normalize(const Vector3& lhs);

struct Vector4
{
    float X, Y, Z, W;
};

Vector4 operator*(float lhs, const Vector4& rhs);
Vector4 operator*(const Vector4& lhs, float rhs);
Vector4 operator/(const Vector4& lhs, float rhs);

struct Matrix44
{
    float
        M11, M12, M13, M14,
        M21, M22, M23, M24,
        M31, M32, M33, M34,
        M41, M42, M43, M44;
};

float Determinant(const Matrix44& lhs);
Matrix44 Invert(const Matrix44& lhs);
Vector4 Transform(const Matrix44& lhs, const Vector4& rhs);
Matrix44 Transpose(const Matrix44& lhs);
Matrix44 operator*(const Matrix44& lhs, const Matrix44& rhs);

struct Quaternion
{
    float X, Y, Z, W;
};

Matrix44 CreateMatrixRotation(const Quaternion& q);
Quaternion CreateQuaternionRotation(const Vector3& axis, float angle);
Quaternion CreateQuaternionRotation(const Matrix44& orthonormal);
Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);

Matrix44 CreateProjection(float near_plane, float far_plane, float fov_horiz, float fov_vert);