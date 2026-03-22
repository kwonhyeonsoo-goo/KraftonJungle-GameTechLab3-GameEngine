#pragma once
#include "AActor.h"
#include "Core/Common.h"

using namespace common::constants;

class ASpotlight : public AActor {
private:
	float ConeHeight		= Actors::ASpotlight::DefaultConeHeight;
	float Radius			= Actors::ASpotlight::DefaultRadius;
	float NumSphereVertex	= Actors::ASpotlight::DefaultSphereVertex;
	FVector UnitNormal;
	
	// Everything is fixed to the actor. Do not allow modular modification from the outside
	using AActor::AddComponent;
	using AActor::RemoveComponent;
public:
	ASpotlight();
	~ASpotlight() = default;


};