#include "Core_Math.h"
#include <math.h>

float2 operator*(const float& lhs, const float2& rhs) { return { lhs * rhs.X, lhs * rhs.Y }; }
float2 operator*(const float2& lhs, const float& rhs) { return { lhs.X * rhs, lhs.Y * rhs }; }
float2 operator+(const float2& lhs, const float2& rhs) { return { lhs.X + rhs.X, lhs.Y + rhs.Y }; }
float2 operator-(const float2& lhs, const float2& rhs) { return { lhs.X - rhs.X, lhs.Y - rhs.Y }; }
float Dot(const float2& lhs, const float2& rhs) { return lhs.X * rhs.X + lhs.Y * rhs.Y; }
float Length(const float2& lhs) { return sqrtf(Dot(lhs, lhs)); }
float2 Normalize(const float2& lhs) { return lhs * (1 / Length(lhs)); }
float2 Perpendicular(const float2& lhs) { return { -lhs.Y, lhs.X }; }

float Determinant(matrix44 val)
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