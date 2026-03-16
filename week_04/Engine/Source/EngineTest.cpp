#include "EngineTest.h"

#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"

#include <cassert>
#include <cstdio>

namespace
{
	void RunRotationMathChecks()
	{
#ifndef NDEBUG
		const FRotator RotatorA(10.0f, 20.0f, 30.0f);
		const FRotator RotatorB(-15.0f, 40.0f, 5.0f);

		const FQuat QuatA = RotatorA.Quaternion();
		const FQuat QuatB = RotatorB.Quaternion();
		const FQuat CombinedQuat = QuatA * QuatB;
		const FQuat CombinedMatrixQuat(QuatA.ToMatrix() * QuatB.ToMatrix());
		assert(CombinedQuat.Equals(CombinedMatrixQuat, 1.e-4f));

		const FRotator RoundTripSource(25.0f, -100.0f, 15.0f);
		const FRotator RoundTripResult = RoundTripSource.Quaternion().Rotator();
		assert(RoundTripSource.Equals(RoundTripResult, 1.e-3f));

		const FQuat MatrixSourceQuat = FRotator(35.0f, -70.0f, 12.0f).Quaternion();
		const FMatrix MatrixWithScaleAndTranslation =
			FMatrix::MakeWorld(FVector(7.0f, -2.0f, 5.0f), MatrixSourceQuat.ToMatrix(), FVector(2.0f, 3.0f, 4.0f));
		const FQuat MatrixExtractedQuat(MatrixWithScaleAndTranslation);
		assert(MatrixExtractedQuat.Equals(MatrixSourceQuat, 1.e-4f));
#endif
	}
}

ENGINE_API void EngineTestFunction()
{
	RunRotationMathChecks();
	printf("Engine DLL Test: OK!\n");
}
