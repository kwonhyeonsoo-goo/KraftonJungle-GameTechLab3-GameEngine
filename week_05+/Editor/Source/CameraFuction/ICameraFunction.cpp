#include "ICameraFunction.h"
#include "CoreMinimal.h"
#include "Math/MathUtility.h"


FVector ICameraFunction::LerpVector(const FVector& A, const FVector& B, float Alpha)
{
	return A + (B - A) * Alpha;
}

float ICameraFunction::LerpFloat(float A, float B, float Alpha)
{
	return A + (B - A) * Alpha;
}

float ICameraFunction::ComputeFocusDistance(float Radius, float FieldOfViewDegrees)
{
	const float SafeRadius = FMath::Max(Radius, 0.5f);
	const float HalfFovRadians = FMath::DegreesToRadians(FieldOfViewDegrees * 0.5f);
	const float SafeTanHalfFov = FMath::Max(std::tanf(HalfFovRadians), 0.01f);
	return FMath::Max((SafeRadius / SafeTanHalfFov), SafeRadius * 2.0f);
}

float ICameraFunction::LerpAngleDegrees(float Start, float End, float Alpha) const
{
	float Delta = std::fmod(End - Start, 360.0f);

	if (Delta > 180.0f)
	{
		Delta -= 360.0f;
	}
	else if (Delta < -180.0f)
	{
		Delta += 360.0f;
	}

	return Start + Delta * Alpha;
}

float ICameraFunction::EvaluateEaseInOut(float T) const
{
	const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
	return ClampedT * ClampedT * (3.0f - 2.0f * ClampedT);
}
