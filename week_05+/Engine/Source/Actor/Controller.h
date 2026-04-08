#pragma once

#include "EngineAPI.h"
#include "Actor.h"
#include "Input/InputManager.h"

class APawn;
class UCameraComponent;

class ENGINE_API AController : public AActor
{
public:
	DECLARE_RTTI(AController, AActor)
	DECLARE_DUPLICATE(AController)

	~AController() override = default;

public:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void EndPlay() override;

	void ProcessInput(int32 KeyCode, EInputEventType EventType);

	void Possess(APawn* InPawn);

	virtual void DuplicateSubObjects() override;

	UCameraComponent* GetCamera() const { return Camera; }
	void SetCamera(UCameraComponent* InCamera) { Camera = InCamera; }


private:
	APawn* Pawn = nullptr;

	UCameraComponent* Camera = nullptr; // 현재는 단순히 Controller가 카메라를 참조해서 view를 보여줍니다.
};
