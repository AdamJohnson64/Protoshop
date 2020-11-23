#pragma once

#include <ctgmath>

template <class T>
constexpr T Pi = static_cast<T>(3.1415926535897932384626433832795029L);

////////////////////////////////////////////////////////////////////////////////
// 2D Vectors (XY)

template <class T>
struct TVector2
{
    T X, Y;
};

using Vector2 = TVector2<float>;

////////////////////////////////////////////////////////////////////////////////
// 3D Vectors (XYZ)

template <class T>
struct TVector3
{
    T X, Y, Z;
};

using Vector3 = TVector3<float>;

////////////////////////////////////////////////////////////////////////////////
// 4D Vectors (XYZW)

template <class T>
struct TVector4
{
    T X, Y, Z, W;
};

using Vector4 = TVector4<float>;

////////////////////////////////////////////////////////////////////////////////
// 4x4 Matrices (M11-M44, Row-Major)

template <class T>
struct TMatrix44
{
    T
        M11, M12, M13, M14,
        M21, M22, M23, M24,
        M31, M32, M33, M34,
        M41, M42, M43, M44;
};

template <class T>
TMatrix44<T> Identity = { 1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1 };

using Matrix44 = TMatrix44<float>;

////////////////////////////////////////////////////////////////////////////////
// Quaternions

template <class T>
struct TQuaternion
{
    T X, Y, Z, W;
};

using Quaternion = TQuaternion<float>;

////////////////////////////////////////////////////////////////////////////////
// 2D Vectors (XY)

template <class T>
TVector2<T> operator*(const T& lhs, const TVector2<T>& rhs) { return { lhs * rhs.X, lhs * rhs.Y }; }

template <class T>
TVector2<T> operator*(const TVector2<T>& lhs, const T& rhs) { return { lhs.X * rhs, lhs.Y * rhs }; }

template <class T>
TVector2<T> operator+(const TVector2<T>& lhs, const TVector2<T>& rhs) { return { lhs.X + rhs.X, lhs.Y + rhs.Y }; }

template <class T>
TVector2<T> operator-(const TVector2<T>& lhs, const TVector2<T>& rhs) { return { lhs.X - rhs.X, lhs.Y - rhs.Y }; }

template <class T>
float Dot(const TVector2<T>& lhs, const TVector2<T>& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }

template <class T>
float Length(const TVector2<T>& lhs) { return sqrt(Dot(lhs, lhs)); }

template <class T>
TVector2<T> Normalize(const TVector2<T>& lhs) { return lhs * (1 / Length(lhs)); }

template <class T>
TVector2<T> Perpendicular(const TVector2<T>& lhs) { return { -lhs.Y, lhs.X }; }

////////////////////////////////////////////////////////////////////////////////
// 3D Vectors (XYZ)

template <class T>
TVector3<T> operator+(const TVector3<T>& lhs, const TVector3<T>& rhs) { return { lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z }; };

template <class T>
TVector3<T> operator-(const TVector3<T>& lhs, const TVector3<T>& rhs) { return { lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z }; };

template <class T>
TVector3<T> operator*(const TVector3<T>& lhs, const T& rhs) { return { lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs }; }

template <class T>
float Dot(const TVector3<T>& lhs, const TVector3<T>& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z; }

template <class T>
float Length(const TVector3<T>& lhs) { return sqrt(Dot(lhs, lhs)); }

template <class T>
TVector3<T> Normalize(const TVector3<T>& lhs) { return lhs * (1 / Length(lhs)); }

////////////////////////////////////////////////////////////////////////////////
// 4D Vectors (XYZW)

template <class T>
TVector4<T> operator*(const TVector4<T>& lhs, T rhs) { return { lhs.X * rhs, lhs.Y * rhs, lhs.Z * rhs, lhs.W * rhs }; }

template <class T>
TVector4<T> operator*(T lhs, const TVector4<T>& rhs) { return { lhs * rhs.X, lhs * rhs.Y, lhs * rhs.Z, lhs * rhs.W }; }

template <class T>
TVector4<T> operator/(const TVector4<T>& lhs, T rhs) { return lhs * (1 / rhs); }

////////////////////////////////////////////////////////////////////////////////
// 4x4 Matrices (M11-M44, Row-Major)

template <class T>
float Determinant(const TMatrix44<T>& lhs)
{
    return (-lhs.M14 * (+lhs.M23 * (lhs.M31 * lhs.M42 - lhs.M32 * lhs.M41) - lhs.M33 * (lhs.M21 * lhs.M42 - lhs.M22 * lhs.M41) + lhs.M43 * (lhs.M21 * lhs.M32 - lhs.M22 * lhs.M31)) + lhs.M24 * (+lhs.M13 * (lhs.M31 * lhs.M42 - lhs.M32 * lhs.M41) - lhs.M33 * (lhs.M11 * lhs.M42 - lhs.M12 * lhs.M41) + lhs.M43 * (lhs.M11 * lhs.M32 - lhs.M12 * lhs.M31)) - lhs.M34 * (+lhs.M13 * (lhs.M21 * lhs.M42 - lhs.M22 * lhs.M41) - lhs.M23 * (lhs.M11 * lhs.M42 - lhs.M12 * lhs.M41) + lhs.M43 * (lhs.M11 * lhs.M22 - lhs.M12 * lhs.M21)) + lhs.M44 * (+lhs.M13 * (lhs.M21 * lhs.M32 - lhs.M22 * lhs.M31) - lhs.M23 * (lhs.M11 * lhs.M32 - lhs.M12 * lhs.M31) + lhs.M33 * (lhs.M11 * lhs.M22 - lhs.M12 * lhs.M21)));
}

template <class T>
TMatrix44<T> Invert(const TMatrix44<T>& lhs)
{
    float invdet = 1 / Determinant(lhs);
    return TMatrix44<T> {
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

template <class T>
TVector4<T> Transform(const TMatrix44<T>& lhs, const TVector4<T>& rhs)
{
    return {
        lhs.M11 * rhs.X + lhs.M21 * rhs.Y + lhs.M31 * rhs.Z + lhs.M41 * rhs.W,
        lhs.M12 * rhs.X + lhs.M22 * rhs.Y + lhs.M32 * rhs.Z + lhs.M42 * rhs.W,
        lhs.M13 * rhs.X + lhs.M23 * rhs.Y + lhs.M33 * rhs.Z + lhs.M43 * rhs.W,
        lhs.M14 * rhs.X + lhs.M24 * rhs.Y + lhs.M34 * rhs.Z + lhs.M44 * rhs.W };
}

template <class T>
TMatrix44<T> Transpose(const TMatrix44<T>& lhs)
{
    return {
        lhs.M11, lhs.M21, lhs.M31, lhs.M41,
        lhs.M12, lhs.M22, lhs.M32, lhs.M42,
        lhs.M13, lhs.M23, lhs.M33, lhs.M43,
        lhs.M14, lhs.M24, lhs.M34, lhs.M44 };
}

template <class T>
TMatrix44<T> operator*(const TMatrix44<T>& lhs, const TMatrix44<T>& rhs)
{
    return TMatrix44<T> {
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

template <class T>
TMatrix44<T> CreateMatrixRotation(const TQuaternion<T>& q)
{
    T m11 = 1 - 2 * q.Y * q.Y - 2 * q.Z * q.Z;
    T m12 = 2 * (q.X * q.Y + q.Z * q.W);
    T m13 = 2 * (q.X * q.Z - q.Y * q.W);
    T m21 = 2 * (q.X * q.Y - q.Z * q.W);
    T m22 = 1 - 2 * q.X * q.X - 2 * q.Z * q.Z;
    T m23 = 2 * (q.Y * q.Z + q.X * q.W);
    T m31 = 2 * (q.X * q.Z + q.Y * q.W);
    T m32 = 2 * (q.Y * q.Z - q.X * q.W);
    T m33 = 1 - 2 * q.X * q.X - 2 * q.Y * q.Y;
    return TMatrix44<T> { m11, m12, m13, 0, m21, m22, m23, 0, m31, m32, m33, 0, 0, 0, 0, 1 };
}

template <class T>
TMatrix44<T> CreateMatrixScale(const TVector3<T> scale)
{
    TMatrix44<T> o = {};
    o.M11 = scale.X;
    o.M22 = scale.Y;
    o.M33 = scale.Z;
    o.M44 = 1;
    return o;
}

template <class T>
TMatrix44<T> CreateMatrixTranslate(const TVector3<T> translate)
{
    TMatrix44<T> o = {};
    o.M44 = o.M33 = o.M22 = o.M11 = 1;
    o.M34 = o.M32 = o.M31 = o.M24 = o.M23 = o.M21 = o.M14 = o.M13 = o.M12 = 0; 
    o.M41 = translate.X;
    o.M42 = translate.Y;
    o.M43 = translate.Z;
    return o;
}

////////////////////////////////////////////////////////////////////////////////
// Quaternions

template <class T>
TQuaternion<T> CreateQuaternionRotation(const TVector3<T>& axis, T angle)
{
    T radians_over_2 = (angle * Pi<T> / 180) / 2;
    T sinvalue = sin(radians_over_2);
    TVector3<T> normalized_axis = Normalize(axis);
    return { normalized_axis.X * sinvalue, normalized_axis.Y * sinvalue, normalized_axis.Z * sinvalue, cos(radians_over_2) };
}

template <class T>
TQuaternion<T> CreateQuaternionRotation(const TMatrix44<T>& orthonormal)
{
    T w = sqrt(1 + orthonormal.M11 + orthonormal.M22 + orthonormal.M33) / 2;
    T x = (orthonormal.M23 - orthonormal.M32) / (4 * w);
    T y = (orthonormal.M31 - orthonormal.M13) / (4 * w);
    T z = (orthonormal.M12 - orthonormal.M21) / (4 * w);
    return TQuaternion<T> { x, y, z, w };
}

template <class T>
TQuaternion<T> Multiply(const TQuaternion<T>& lhs, const TQuaternion<T>& rhs)
{
    T t0 = rhs.W * lhs.W - rhs.X * lhs.X - rhs.Y * lhs.Y - rhs.Z * lhs.Z;
    T t1 = rhs.W * lhs.X + rhs.X * lhs.W - rhs.Y * lhs.Z + rhs.Z * lhs.Y;
    T t2 = rhs.W * lhs.Y + rhs.X * lhs.Z + rhs.Y * lhs.W - rhs.Z * lhs.X;
    T t3 = rhs.W * lhs.Z - rhs.X * lhs.Y + rhs.Y * lhs.X + rhs.Z * lhs.W;
    T ln = 1 / sqrt(t0 * t0 + t1 * t1 + t2 * t2 + t3 * t3);
    t0 *= ln; t1 *= ln; t2 *= ln; t3 *= ln;
    return TQuaternion<T> { t1, t2, t3, t0 };
}

template <class T>
TMatrix44<T> CreateProjection(T near_plane, T far_plane, T fov_horiz, T fov_vert)
{
    T w = 1 / tan(fov_horiz * 0.5);  // 1/tan(x) == cot(x)
    T h = 1 / tan(fov_vert * 0.5);   // 1/tan(x) == cot(x)
    T Q = far_plane / (far_plane - near_plane);
    return {
        w, 0, 0, 0,
        0, h, 0, 0,
        0, 0, Q, 1,
        0, 0, -Q * near_plane, 0
    };
}