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
	const float* m = &M[0][0];

	float Coef00 = m[10] * m[15] - m[14] * m[11];
	float Coef02 = m[6] * m[15] - m[14] * m[7];
	float Coef03 = m[6] * m[11] - m[10] * m[7];

	float Coef04 = m[9] * m[15] - m[13] * m[11];
	float Coef06 = m[5] * m[15] - m[13] * m[7];
	float Coef07 = m[5] * m[11] - m[9] * m[7];

	float Coef08 = m[9] * m[14] - m[13] * m[10];
	float Coef10 = m[5] * m[14] - m[13] * m[6];
	float Coef11 = m[5] * m[10] - m[9] * m[6];

	float Coef12 = m[8] * m[15] - m[12] * m[11];
	float Coef14 = m[4] * m[15] - m[12] * m[7];
	float Coef15 = m[4] * m[11] - m[8] * m[7];

	float Coef16 = m[8] * m[14] - m[12] * m[10];
	float Coef18 = m[4] * m[14] - m[12] * m[6];
	float Coef19 = m[4] * m[10] - m[8] * m[6];

	float Coef20 = m[8] * m[13] - m[12] * m[9];
	float Coef22 = m[4] * m[13] - m[12] * m[5];
	float Coef23 = m[4] * m[9] - m[8] * m[5];

	float Fac0[4] = { Coef00, Coef00, Coef02, Coef03 };
	float Fac1[4] = { Coef04, Coef04, Coef06, Coef07 };
	float Fac2[4] = { Coef08, Coef08, Coef10, Coef11 };
	float Fac3[4] = { Coef12, Coef12, Coef14, Coef15 };
	float Fac4[4] = { Coef16, Coef16, Coef18, Coef19 };
	float Fac5[4] = { Coef20, Coef20, Coef22, Coef23 };

	float Vec0[4] = { m[4],  m[0],  m[0],  m[0] };
	float Vec1[4] = { m[5],  m[1],  m[1],  m[1] };
	float Vec2[4] = { m[6],  m[2],  m[2],  m[2] };
	float Vec3[4] = { m[7],  m[3],  m[3],  m[3] };

	float Inv0[4], Inv1[4], Inv2[4], Inv3[4];
	for (int i = 0; i < 4; ++i)
	{
		Inv0[i] = Vec1[i] * Fac0[i] - Vec2[i] * Fac1[i] + Vec3[i] * Fac2[i];
		Inv1[i] = Vec0[i] * Fac0[i] - Vec2[i] * Fac3[i] + Vec3[i] * Fac4[i];
		Inv2[i] = Vec0[i] * Fac1[i] - Vec1[i] * Fac3[i] + Vec3[i] * Fac5[i];
		Inv3[i] = Vec0[i] * Fac2[i] - Vec1[i] * Fac4[i] + Vec2[i] * Fac5[i];
	}

	float Sign[4] = { 1.0f, -1.0f, 1.0f, -1.0f };
	FMatrix Result;
	for (int i = 0; i < 4; ++i)
	{
		Result.M[0][i] = Inv0[i] * Sign[i];
		Result.M[1][i] = Inv1[i] * -Sign[i];
		Result.M[2][i] = Inv2[i] * Sign[i];
		Result.M[3][i] = Inv3[i] * -Sign[i];
	}

	float Det = m[0] * Result.M[0][0] + m[1] * Result.M[1][0]
		+ m[2] * Result.M[2][0] + m[3] * Result.M[3][0];

	if (fabsf(Det) < 1e-8f)
	{
		return Identity();
	}

	float InvDet = 1.0f / Det;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			Result.M[i][j] *= InvDet;

	return Result;
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
	FVector Forward = (Target - Eye).Normalize();
	FVector Right = Up.Cross(Forward).Normalize();
	FVector NewUp = Forward.Cross(Right);

	FMatrix Result = Identity();
	Result.M[0][0] = Right.X;    Result.M[0][1] = NewUp.X;    Result.M[0][2] = Forward.X;
	Result.M[1][0] = Right.Y;    Result.M[1][1] = NewUp.Y;    Result.M[1][2] = Forward.Y;
	Result.M[2][0] = Right.Z;    Result.M[2][1] = NewUp.Z;    Result.M[2][2] = Forward.Z;
	Result.M[3][0] = -Right.Dot(Eye);
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
