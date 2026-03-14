#include "Math/Matrix.h"
#include <cmath>

FMatrix FMatrix::Identity()
{
    FMatrix Result;
    Result.M[0][0] = 1.0f;
    Result.M[1][1] = 1.0f;
    Result.M[2][2] = 1.0f;
    Result.M[3][3] = 1.0f;
    return Result;
}

FMatrix FMatrix::operator*(const FMatrix& Other) const
{
    FMatrix Result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            Result.M[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k)
                Result.M[i][j] += M[i][k] * Other.M[k][j];
        }
    return Result;
}

FVector4 FMatrix::TransformVector4(const FVector4& V) const
{
    FVector4 Result;
    Result.X = M[0][0] * V.X + M[0][1] * V.Y + M[0][2] * V.Z + M[0][3] * V.W;
    Result.Y = M[1][0] * V.X + M[1][1] * V.Y + M[1][2] * V.Z + M[1][3] * V.W;
    Result.Z = M[2][0] * V.X + M[2][1] * V.Y + M[2][2] * V.Z + M[2][3] * V.W;
    Result.W = M[3][0] * V.X + M[3][1] * V.Y + M[3][2] * V.Z + M[3][3] * V.W;
    return Result;
}

FMatrix FMatrix::Transpose() const
{
    FMatrix Result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            Result.M[i][j] = M[j][i];
    return Result;
}

FMatrix FMatrix::Inverse() const
{
    // TODO: 4x4 역행렬 구현
    return Identity();
}

FMatrix FMatrix::Translation(float X, float Y, float Z)
{
    FMatrix Result = Identity();
    Result.M[3][0] = X;
    Result.M[3][1] = Y;
    Result.M[3][2] = Z;
    return Result;
}

FMatrix FMatrix::RotationX(float AngleRad)
{
    FMatrix Result = Identity();
    float C = cosf(AngleRad);
    float S = sinf(AngleRad);
    Result.M[1][1] = C;
    Result.M[1][2] = S;
    Result.M[2][1] = -S;
    Result.M[2][2] = C;
    return Result;
}

FMatrix FMatrix::RotationY(float AngleRad)
{
    FMatrix Result = Identity();
    float C = cosf(AngleRad);
    float S = sinf(AngleRad);
    Result.M[0][0] = C;
    Result.M[0][2] = -S;
    Result.M[2][0] = S;
    Result.M[2][2] = C;
    return Result;
}

FMatrix FMatrix::RotationZ(float AngleRad)
{
    FMatrix Result = Identity();
    float C = cosf(AngleRad);
    float S = sinf(AngleRad);
    Result.M[0][0] = C;
    Result.M[0][1] = S;
    Result.M[1][0] = -S;
    Result.M[1][1] = C;
    return Result;
}

FMatrix FMatrix::Scale(float X, float Y, float Z)
{
    FMatrix Result = Identity();
    Result.M[0][0] = X;
    Result.M[1][1] = Y;
    Result.M[2][2] = Z;
    return Result;
}

FMatrix FMatrix::LookAt(const FVector& Eye, const FVector& Target, const FVector& Up)
{
	FVector Forward = (Target - Eye).GetSafeNormal();
    FVector Right = Up.Cross(Forward).Normalize();
    FVector NewUp = Forward.Cross(Right);

    FMatrix Result = Identity();
    Result.M[0][0] = Right.X;    Result.M[0][1] = NewUp.X;    Result.M[0][2] = Forward.X;
    Result.M[1][0] = Right.Y;    Result.M[1][1] = NewUp.Y;    Result.M[1][2] = Forward.Y;
    Result.M[2][0] = Right.Z;    Result.M[2][1] = NewUp.Z;    Result.M[2][2] = Forward.Z;
    Result.M[3][0] = -Right.dot(Eye);
    Result.M[3][1] = -NewUp.Dot(Eye);
    Result.M[3][2] = -Forward.Dot(Eye);
    return Result;
}

FMatrix FMatrix::Perspective(float FOV, float AspectRatio, float Near, float Far)
{
    float TanHalfFOV = tanf(FOV * 0.5f * 3.14159265f / 180.0f);
    FMatrix Result;
    Result.M[0][0] = 1.0f / (AspectRatio * TanHalfFOV);
    Result.M[1][1] = 1.0f / TanHalfFOV;
    Result.M[2][2] = Far / (Far - Near);
    Result.M[2][3] = 1.0f;
    Result.M[3][2] = -(Near * Far) / (Far - Near);
    return Result;
}
