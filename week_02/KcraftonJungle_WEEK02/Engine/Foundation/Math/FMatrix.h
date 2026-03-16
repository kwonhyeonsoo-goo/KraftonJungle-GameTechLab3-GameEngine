#pragma once
#include <cmath>
#include "FVector.h"

struct FMatrix
{
    float M[4][4];

    FMatrix()
        : M{
            { 1.f, 0.f, 0.f, 0.f },
            { 0.f, 1.f, 0.f, 0.f },
            { 0.f, 0.f, 1.f, 0.f },
            { 0.f, 0.f, 0.f, 1.f }
        }
    {
    }

    FMatrix(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33)
        : M{
            { m00, m01, m02, m03 },
            { m10, m11, m12, m13 },
            { m20, m21, m22, m23 },
            { m30, m31, m32, m33 }
        }
    {
    }

    explicit FMatrix(const float m[4][4])
    {
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                M[r][c] = m[r][c];
            }
        }
    }

    static FMatrix Identity()
    {
        return FMatrix();
    }

    FMatrix operator*(const FMatrix& rhs) const
    {
        FMatrix result;

        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                result.M[r][c] = 0.f;

                for (int k = 0; k < 4; ++k)
                {
                    result.M[r][c] += M[r][k] * rhs.M[k][c];
                }
            }
        }

        return result;
    }

    FMatrix Transposed() const
    {
        FMatrix result;

        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                result.M[r][c] = M[c][r];
            }
        }

        return result;
    }

    FMatrix Inversed() const
    {
        float aug[4][8] = {};

        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                aug[r][c] = M[r][c];
            }

            aug[r][r + 4] = 1.f;
        }

        for (int col = 0; col < 4; ++col)
        {
            int pivotRow = col;
            float maxAbs = std::fabs(aug[col][col]);

            for (int row = col + 1; row < 4; ++row)
            {
                const float absValue = std::fabs(aug[row][col]);
                if (absValue > maxAbs)
                {
                    maxAbs = absValue;
                    pivotRow = row;
                }
            }

            if (maxAbs < 1e-8f)
            {
                return FMatrix::Identity();
            }

            if (pivotRow != col)
            {
                for (int c = 0; c < 8; ++c)
                {
                    const float temp = aug[col][c];
                    aug[col][c] = aug[pivotRow][c];
                    aug[pivotRow][c] = temp;
                }
            }

            const float pivot = aug[col][col];
            for (int c = 0; c < 8; ++c)
            {
                aug[col][c] /= pivot;
            }

            for (int row = 0; row < 4; ++row)
            {
                if (row == col)
                {
                    continue;
                }

                const float factor = aug[row][col];
                if (factor == 0.f)
                {
                    continue;
                }

                for (int c = 0; c < 8; ++c)
                {
                    aug[row][c] -= factor * aug[col][c];
                }
            }
        }

        FMatrix result;
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                result.M[r][c] = aug[r][c + 4];
            }
        }

        return result;
    }

    // row-vector ����: v * M
    static FMatrix Translation(const FVector& t)
    {
        FMatrix result = FMatrix::Identity();
        result.M[3][0] = t.x;
        result.M[3][1] = t.y;
        result.M[3][2] = t.z;
        return result;
    }

    static FMatrix Scale(const FVector& s)
    {
        FMatrix result = FMatrix::Identity();
        result.M[0][0] = s.x;
        result.M[1][1] = s.y;
        result.M[2][2] = s.z;
        return result;
    }

    static FMatrix RotationX(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        FMatrix result = FMatrix::Identity();
        result.M[1][1] = c;
        result.M[1][2] = s;
        result.M[2][1] = -s;
        result.M[2][2] = c;
        return result;
    }

    static FMatrix RotationY(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        FMatrix result = FMatrix::Identity();
        result.M[0][0] = c;
        result.M[0][2] = -s;
        result.M[2][0] = s;
        result.M[2][2] = c;
        return result;
    }

    static FMatrix RotationZ(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        FMatrix result = FMatrix::Identity();
        result.M[0][0] = c;
        result.M[0][1] = s;
        result.M[1][0] = -s;
        result.M[1][1] = c;
        return result;
    }

    static FMatrix LookAt(const FVector& eye, const FVector& at, const FVector& up)
    {
        auto NormalizeSafe = [](FVector v) -> FVector
            {
                const float len = v.Length();
                if (len > 1e-8f)
                {
                    v /= len;
                }
                return v;
            };

        const FVector zaxis = NormalizeSafe(at - eye);
        const FVector xaxis = NormalizeSafe(FVector::CrossProduct(up, zaxis));
        const FVector yaxis = NormalizeSafe(FVector::CrossProduct(zaxis, xaxis));

        FMatrix result = FMatrix::Identity();

        result.M[0][0] = xaxis.x;
        result.M[0][1] = yaxis.x;
        result.M[0][2] = zaxis.x;
        result.M[0][3] = 0.f;

        result.M[1][0] = xaxis.y;
        result.M[1][1] = yaxis.y;
        result.M[1][2] = zaxis.y;
        result.M[1][3] = 0.f;

        result.M[2][0] = xaxis.z;
        result.M[2][1] = yaxis.z;
        result.M[2][2] = zaxis.z;
        result.M[2][3] = 0.f;

        result.M[3][0] = -FVector::DotProduct(xaxis, eye);
        result.M[3][1] = -FVector::DotProduct(yaxis, eye);
        result.M[3][2] = -FVector::DotProduct(zaxis, eye);
        result.M[3][3] = 1.f;

        return result;
    }

    static FMatrix Orthographic(float height, float width,float nearZ, float farZ)
    {
        FMatrix result = {};

        result.M[0][0] = 2.0f / width;
        result.M[1][1] = 2.0f / height;
        result.M[2][2] = 1.0f / (farZ - nearZ);
        result.M[3][2] = -nearZ / (farZ - nearZ);
        result.M[3][3] = 1.0f;

        return result;
    }

    static FMatrix Perspective(float fovY, float aspect, float nearZ, float farZ)
    {
        const float yScale = 1.0f / std::tan(fovY * 0.5f);
        const float xScale = yScale / aspect;
        const float zScale = farZ / (farZ - nearZ);
        const float zBias = (-nearZ * farZ) / (farZ - nearZ);

        FMatrix result = {};

        result.M[0][0] = xScale;
        result.M[1][1] = yScale;
        result.M[2][2] = zScale;
        result.M[2][3] = 1.f;
        result.M[3][2] = zBias;

        return result;
    }
};