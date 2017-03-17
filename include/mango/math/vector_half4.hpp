/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "vector.hpp"
#include "vector_float4.hpp"

namespace mango
{

    // half4 is a storage class and does not support any arithmetic operations.
    template <>
    struct Vector<half, 4> : VectorBase<half, 4>
    {
        simd::float16x4 xyzw;

        explicit Vector()
        {
        }

        explicit Vector(const Vector<float, 4>& v)
        : xyzw(simd::float16x4_convert(v.xyzw))
        {
        }

        explicit Vector(simd::float32x4 v)
        : xyzw(simd::float16x4_convert(v))
        {
        }

        explicit Vector(simd::float16x4 v)
        : xyzw(v)
        {
        }

        ~Vector()
        {
        }

        Vector& operator = (const Vector<float, 4>& v)
        {
            xyzw = simd::float16x4_convert(v.xyzw);
            return *this;
        }

        Vector& operator = (simd::float32x4 v)
        {
            xyzw = simd::float16x4_convert(v);
            return *this;
        }

        Vector& operator = (simd::float16x4 v)
        {
            xyzw = v;
            return *this;
        }

        Vector& operator = (const Vector& v)
        {
            xyzw = v.xyzw;
            return *this;
        }

        operator Vector<float, 4> () const
        {
            return Vector<float, 4>(simd::float32x4_convert(xyzw));
        }

        operator const simd::float16x4& () const
        {
            return xyzw;
        }

        operator simd::float16x4& ()
        {
            return xyzw;
        }
    };

} // namespace mango
