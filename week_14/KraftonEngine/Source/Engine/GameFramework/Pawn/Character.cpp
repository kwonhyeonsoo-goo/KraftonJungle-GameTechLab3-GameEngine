#include "GameFramework/Pawn/Character.h"

#include "Component/Shape/CapsuleComponent.h"
#include "Component/Input/InputComponent.h"
#include "Component/Movement/CharacterMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Input/InputSystem.h"
#include "Math/Rotator.h"
#include "Mesh/MeshManager.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Physics/PhysicsAssetInstance.h"
#include "Runtime/Engine.h"
#include "Core/Logging/Log.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace
{
	FString ToLowerCopy(const FString& Value)
	{
		FString Result = Value;
		std::transform(
			Result.begin(),
			Result.end(),
			Result.begin(),
			[](unsigned char Character)
			{
				return static_cast<char>(std::tolower(Character));
			});
		return Result;
	}

	int32 FindCharacterRagdollAnchorBoneIndex(const FSkeletalMesh* MeshAsset)
	{
		if (!MeshAsset)
		{
			return -1;
		}

		const char* CandidateTokens[] = { "pelvis", "hips", "hip", "root" };
		for (const char* CandidateToken : CandidateTokens)
		{
			const FString Token = ToLowerCopy(CandidateToken ? FString(CandidateToken) : FString());
			for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
			{
				if (ToLowerCopy(MeshAsset->Bones[BoneIndex].Name) == Token)
				{
					return BoneIndex;
				}
			}
		}

		for (const char* CandidateToken : CandidateTokens)
		{
			const FString Token = ToLowerCopy(CandidateToken ? FString(CandidateToken) : FString());
			for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
			{
				if (ToLowerCopy(MeshAsset->Bones[BoneIndex].Name).find(Token) != FString::npos)
				{
					return BoneIndex;
				}
			}
		}

		return MeshAsset->Bones.empty() ? -1 : 0;
	}

	int32 FindCharacterChestBoneIndex(const FSkeletalMesh* MeshAsset)
	{
		if (!MeshAsset)
		{
			return -1;
		}

		const char* CandidateTokens[] = { "spine_03", "spine_02", "spine_01", "spine", "chest", "upperchest", "torso" };
		for (const char* CandidateToken : CandidateTokens)
		{
			const FString Token = ToLowerCopy(CandidateToken ? FString(CandidateToken) : FString());
			for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
			{
				if (ToLowerCopy(MeshAsset->Bones[BoneIndex].Name) == Token)
				{
					return BoneIndex;
				}
			}
		}

		for (const char* CandidateToken : CandidateTokens)
		{
			const FString Token = ToLowerCopy(CandidateToken ? FString(CandidateToken) : FString());
			for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
			{
				if (ToLowerCopy(MeshAsset->Bones[BoneIndex].Name).find(Token) != FString::npos)
				{
					return BoneIndex;
				}
			}
		}

		return -1;
	}

	const char* LexToString(ECharacterPhysicsOwnershipMode Mode)
	{
		switch (Mode)
		{
		case ECharacterPhysicsOwnershipMode::CharacterDriven:
			return "CharacterDriven";
		case ECharacterPhysicsOwnershipMode::TransitionToRagdoll:
			return "TransitionToRagdoll";
		case ECharacterPhysicsOwnershipMode::RagdollDriven:
			return "RagdollDriven";
		case ECharacterPhysicsOwnershipMode::TransitionFromRagdoll:
			return "TransitionFromRagdoll";
		default:
			return "Unknown";
		}
	}
}
void ACharacter::InitDefaultComponents(const FString& SkeletalMeshFileName)
{
	// 1) Capsule — Root. CharacterMovement 의 UpdatedComponent 가 이걸 가리킴.
	CapsuleComponent = AddComponent<UCapsuleComponent>();
	SetRootComponent(CapsuleComponent);

	// 물리 시뮬레이션(중력/반발)은 CharacterMovement 가 직접 처리하므로 끄되, trigger volume
	// 등과의 overlap 이벤트는 받아야 한다. PhysX 백엔드는 static-static pair 를 생성하지 않아
	// simulate=false 인 static 바디로는 static trigger 와 overlap 이 발화되지 않으므로 kinematic 으로 등록
	// (kinematic↔static pair → setKinematicTarget 으로 위치 추적 + trigger 콜백 수신).
	// ※ overlap 을 받으려면 CollisionEnabled 가 QueryOnly/QueryAndPhysics 여야 물리 씬에 등록됨.
	CapsuleComponent->SetKinematic(true);

	// 2) SkeletalMesh — Capsule 의 자식.
	Mesh = AddComponent<USkeletalMeshComponent>();
	Mesh->AttachToComponent(CapsuleComponent);

	ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
	if (!SkeletalMeshFileName.empty())
	{
		USkeletalMesh* Asset = FMeshManager::LoadSkeletalMesh(SkeletalMeshFileName, Device);
		Mesh->SetSkeletalMesh(Asset);
	}

	// 3) CharacterMovement — non-scene. UpdatedComponent = Capsule.
	CharacterMovement = AddComponent<UCharacterMovementComponent>();
	CharacterMovement->SetUpdatedComponent(CapsuleComponent);
}

void ACharacter::PostDuplicate()
{
	Super::PostDuplicate();
	// 컴포넌트 트리 재발견 — Duplicate 후 멤버 포인터 복원.
	CapsuleComponent  = Cast<UCapsuleComponent>(GetRootComponent());
	Mesh              = GetComponentByClass<USkeletalMeshComponent>();
	CharacterMovement = GetComponentByClass<UCharacterMovementComponent>();
}

bool ACharacter::EnterRagdoll()
{
	if (!Mesh)
	{
		UE_LOG("Character ragdoll entry rejected: no mesh. Actor=%s", GetName().c_str());
		return false;
	}

	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::RagdollDriven)
	{
		return Mesh->IsRagdollActive();
	}

	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::TransitionToRagdoll)
	{
		return true;
	}

	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::TransitionFromRagdoll)
	{
		UE_LOG("Character ragdoll entry rejected: restore is in progress. Actor=%s", GetName().c_str());
		return false;
	}

	SavePreRagdollCharacterState();
	SuspendCharacterForRagdoll();
	CacheRagdollRestoreLocation();
	PhysicsOwnershipMode = ECharacterPhysicsOwnershipMode::TransitionToRagdoll;
	bPendingRagdollBodyEnable = true;
	bAwaitingRagdollRecoveryRestore = false;
	UE_LOG("Character ragdoll transition queued. Actor=%s Mesh=%s Ownership=%s",
		GetName().c_str(),
		Mesh->GetName().c_str(),
		LexToString(PhysicsOwnershipMode));
	return true;
}

void ACharacter::ExitRagdoll()
{
	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::CharacterDriven)
	{
		return;
	}

	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::TransitionToRagdoll)
	{
		bPendingRagdollBodyEnable = false;
		RestoreCharacterAfterRagdoll();
		UE_LOG("Character ragdoll transition cancelled before runtime activation. Actor=%s", GetName().c_str());
		return;
	}

	CacheRagdollRestoreLocation();
	PhysicsOwnershipMode = ECharacterPhysicsOwnershipMode::TransitionFromRagdoll;

	if (Mesh)
	{
		Mesh->DisableRagdollPhysics();
	}

	bAwaitingRagdollRecoveryRestore = false;
	RestoreCharacterAfterRagdoll();
	UE_LOG("Character ragdoll exited. Actor=%s Ownership=%s",
		GetName().c_str(),
		LexToString(PhysicsOwnershipMode));
}

bool ACharacter::BeginRagdollRecovery()
{
	if (PhysicsOwnershipMode != ECharacterPhysicsOwnershipMode::RagdollDriven || !Mesh)
	{
		UE_LOG("Character ragdoll recovery ignored: ragdoll inactive. Actor=%s", GetName().c_str());
		return false;
	}

	const bool bStarted = Mesh->BeginRagdollRecovery();
	if (bStarted)
	{
		PhysicsOwnershipMode = ECharacterPhysicsOwnershipMode::TransitionFromRagdoll;
		bAwaitingRagdollRecoveryRestore = true;
		UE_LOG("Character ragdoll recovery started. Actor=%s Ownership=%s",
			GetName().c_str(),
			LexToString(PhysicsOwnershipMode));
	}
	else
	{
		UE_LOG("Character ragdoll recovery rejected. Actor=%s", GetName().c_str());
	}

	return bStarted;
}

bool ACharacter::IsInRagdoll() const
{
	return PhysicsOwnershipMode != ECharacterPhysicsOwnershipMode::CharacterDriven;
}

void ACharacter::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
	if (IsInRagdoll())
	{
		return;
	}

	if (CharacterMovement)
	{
		CharacterMovement->AddInputVector(WorldDirection, ScaleValue);
	}
}

void ACharacter::Jump()
{
	if (IsInRagdoll())
	{
		return;
	}

	if (CharacterMovement)
	{
		CharacterMovement->Jump();
	}
}

void ACharacter::SavePreRagdollCharacterState()
{
	if (bSavedPreRagdollCharacterState)
	{
		return;
	}

	SavedCapsuleCollisionEnabled = CapsuleComponent
		? CapsuleComponent->GetCollisionEnabled()
		: ECollisionEnabled::NoCollision;
	SavedMeshCollisionEnabled = Mesh
		? Mesh->GetCollisionEnabled()
		: ECollisionEnabled::NoCollision;
	bSavedMovementTickEnabled = CharacterMovement
		? CharacterMovement->PrimaryComponentTick.bTickEnabled
		: true;
	bSavedMovementActive = CharacterMovement
		? CharacterMovement->IsActive()
		: true;
	bSavedPreRagdollCharacterState = true;
}

void ACharacter::SetCharacterDrivenCollisionEnabled(bool bEnabled)
{
	// Character-driven collision is treated as a single ownership set. During ragdoll
	// we disable the normal capsule/mesh primitive collision together so only the
	// PhysicsAsset runtime bodies remain as physical blockers.
	if (Mesh)
	{
		Mesh->SetCollisionEnabled(bEnabled ? SavedMeshCollisionEnabled : ECollisionEnabled::NoCollision);
	}

	if (CapsuleComponent)
	{
		CapsuleComponent->SetCollisionEnabled(
			bEnabled ? SavedCapsuleCollisionEnabled : ECollisionEnabled::NoCollision);
	}
}

void ACharacter::SuspendCharacterForRagdoll()
{
	SetCharacterDrivenCollisionEnabled(false);
	if (CharacterMovement)
	{
		FVector DiscardedInput;
		CharacterMovement->ConsumeInputVector(DiscardedInput);
		CharacterMovement->SetComponentTickEnabled(false);
		CharacterMovement->Deactivate();
	}

	UE_LOG("Character collision and movement suspended for ragdoll. Actor=%s", GetName().c_str());
}

void ACharacter::RestoreMovementAfterRagdoll()
{
	if (!CharacterMovement)
	{
		return;
	}

	CharacterMovement->SetComponentTickEnabled(bSavedMovementTickEnabled);
	if (bSavedMovementActive)
	{
		CharacterMovement->Activate();
	}
	else
	{
		CharacterMovement->Deactivate();
	}
}

void ACharacter::FinalizePendingRagdollEntry()
{
	if (!bPendingRagdollBodyEnable)
	{
		return;
	}

	bPendingRagdollBodyEnable = false;
	if (!Mesh)
	{
		RestoreCharacterAfterRagdoll();
		UE_LOG("Character ragdoll entry failed: mesh disappeared before activation. Actor=%s", GetName().c_str());
		return;
	}

	if (!Mesh->EnableRagdollPhysics())
	{
		RestoreCharacterAfterRagdoll();
		UE_LOG("Character ragdoll entry failed: mesh ragdoll rejected. Actor=%s Mesh=%s",
			GetName().c_str(),
			Mesh->GetName().c_str());
		return;
	}

	PhysicsOwnershipMode = ECharacterPhysicsOwnershipMode::RagdollDriven;
	UE_LOG("Character ragdoll entered. Actor=%s Mesh=%s Ownership=%s",
		GetName().c_str(),
		Mesh->GetName().c_str(),
		LexToString(PhysicsOwnershipMode));
}

void ACharacter::CacheRagdollRestoreLocation()
{
	if (!Mesh)
	{
		return;
	}

	FPhysicsAssetInstance* Instance = Mesh->GetPhysicsAssetInstance();
	USkeletalMesh* SkeletalMesh = Mesh->GetSkeletalMesh();
	FSkeletalMesh* MeshAsset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!Instance || !MeshAsset)
	{
		return;
	}

	TArray<FTransform> BoneWorldTransforms;
	if (!Instance->PullPhysicsPose(BoneWorldTransforms) || BoneWorldTransforms.empty())
	{
		return;
	}

	const int32 AnchorBoneIndex = FindCharacterRagdollAnchorBoneIndex(MeshAsset);
	if (AnchorBoneIndex < 0 || AnchorBoneIndex >= static_cast<int32>(BoneWorldTransforms.size()))
	{
		return;
	}

	CachedRagdollRestoreLocation = BoneWorldTransforms[AnchorBoneIndex].Location;
	bHasCachedRagdollRestoreLocation = true;

	const int32 ChestBoneIndex = FindCharacterChestBoneIndex(MeshAsset);
	if (ChestBoneIndex >= 0 && ChestBoneIndex < static_cast<int32>(BoneWorldTransforms.size()))
	{
		FVector FacingVector = BoneWorldTransforms[ChestBoneIndex].Location - CachedRagdollRestoreLocation;
		FacingVector.Z = 0.0f;

		if (FacingVector.Dot(FacingVector) <= 1.0e-4f)
		{
			FacingVector = BoneWorldTransforms[ChestBoneIndex].Rotation.GetForwardVector();
			FacingVector.Z = 0.0f;
		}

		if (FacingVector.Dot(FacingVector) > 1.0e-4f)
		{
			CachedRagdollRestoreYawDegrees = std::atan2(FacingVector.Y, FacingVector.X) * (180.0f / 3.1415926535f);
			bHasCachedRagdollRestoreYaw = true;
		}
	}
}

void ACharacter::RestoreCharacterAfterRagdoll()
{
	if (CapsuleComponent && bHasCachedRagdollRestoreLocation)
	{
		CapsuleComponent->SetWorldLocation(CachedRagdollRestoreLocation);
	}

	if (CapsuleComponent && bHasCachedRagdollRestoreYaw)
	{
		FRotator RestoreRotation = CapsuleComponent->GetWorldRotation();
		RestoreRotation.Yaw = CachedRagdollRestoreYawDegrees;
		CapsuleComponent->SetWorldRotation(RestoreRotation);

		FRotator Control = GetControlRotation();
		Control.Yaw = CachedRagdollRestoreYawDegrees;
		SetControlRotation(Control);
	}

	SetCharacterDrivenCollisionEnabled(true);
	RestoreMovementAfterRagdoll();

	PhysicsOwnershipMode = ECharacterPhysicsOwnershipMode::CharacterDriven;
	bPendingRagdollBodyEnable = false;
	bAwaitingRagdollRecoveryRestore = false;
	bSavedPreRagdollCharacterState = false;
	bHasCachedRagdollRestoreLocation = false;
	CachedRagdollRestoreLocation = FVector::ZeroVector;
	bHasCachedRagdollRestoreYaw = false;
	CachedRagdollRestoreYawDegrees = 0.0f;
	UE_LOG("Character ragdoll ownership restored. Actor=%s Ownership=%s",
		GetName().c_str(),
		LexToString(PhysicsOwnershipMode));
}

void ACharacter::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!bAutoInputWASD || !InputComponent) return;

	// Capsule (RootComponent) 기준 — yaw 회전이 곧 캐릭터 facing. mouse look 이 yaw 만
	// 변경 → forward/right vector 가 자동 회전 → WASD 가 "카메라 보는 방향" 으로 이동.
	InputComponent->AddAxisMapping("MoveForward", 'W',  1.0f);
	InputComponent->AddAxisMapping("MoveForward", 'S', -1.0f);
	InputComponent->AddAxisMapping("MoveRight",   'D',  1.0f);
	InputComponent->AddAxisMapping("MoveRight",   'A', -1.0f);

	// WASD 의 forward/right 는 ControlRotation.Yaw 기준 — capsule rotation 과 무관.
	// "카메라가 보는 방향" (yaw 만, pitch 무시) 으로 이동.
	InputComponent->BindAxis("MoveForward", [this](float Value)
	{
		if (Value == 0.0f) return;
		const FRotator YawOnly(0.0f, GetControlRotation().Yaw, 0.0f);
		AddMovementInput(YawOnly.GetForwardVector(), Value);
	});
	InputComponent->BindAxis("MoveRight", [this](float Value)
	{
		if (Value == 0.0f) return;
		const FRotator YawOnly(0.0f, GetControlRotation().Yaw, 0.0f);
		AddMovementInput(YawOnly.GetRightVector(), Value);
	});

	// Space = Jump (VK_SPACE = 0x20). Walking 중에만 effective (CharacterMovement::Jump 가 guard).
	InputComponent->AddActionMapping("Jump", 0x20);
	InputComponent->BindAction("Jump", EInputEvent::Pressed, [this]()
	{
		Jump();
	});
}

void ACharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PhysicsOwnershipMode == ECharacterPhysicsOwnershipMode::TransitionToRagdoll)
	{
		FinalizePendingRagdollEntry();
	}

	if (IsInRagdoll())
	{
		CacheRagdollRestoreLocation();

		if (bAwaitingRagdollRecoveryRestore &&
			Mesh &&
			!Mesh->IsRagdollActive() &&
			!Mesh->IsRecoveringFromRagdoll())
		{
			RestoreCharacterAfterRagdoll();
		}
	}

	if (bAutoInputMouseLook)
	{
		const InputSystem& In = InputSystem::Get();
		const int DX = In.MouseDeltaX();
		const int DY = In.MouseDeltaY();
		if (DX != 0 || DY != 0)
		{
			// APawn::ControlRotation 누적. SpringArm 이 bUsePawnControlRotation 통해 이걸 사용.
			// capsule 회전은 옵션 (bUseControllerRotationYaw 등) — 아래 ApplyControllerRotationToRoot 가 처리.
			FRotator Rot = GetControlRotation();
			Rot.Yaw   += static_cast<float>(DX) * MouseSensitivity;
			Rot.Pitch += static_cast<float>(DY) * MouseSensitivity;
			Rot.Pitch  = std::clamp(Rot.Pitch, MinCameraPitch, MaxCameraPitch);
			SetControlRotation(Rot);
		}
	}

	// 같은 frame 안 ControlRotation 변경을 capsule (RootComponent) 에 즉시 반영 — 1 frame 지연 없음.
	// 옵션 충돌 가드:
	//   1) bOrientRotationToMovement = true → yaw 는 Movement::PhysOrientToMovement 가 처리.
	//   2) 직전 frame 에 root motion 이 yaw 를 적용했다 → 이번 frame 도 root motion 이 yaw 를
	//      이어받을 가능성이 큼. Character 가 control yaw 로 덮으면 root motion 회전이 즉시
	//      뒤집혀 토글링 됨 (turn-in-place / strafe anim 의 시각 손상). Movement 측에 양보.
	// 두 경우 모두 pitch/roll 만 apply, yaw 는 movement 에 양보.
	if (CapsuleComponent)
	{
		if (IsInRagdoll())
		{
			return;
		}

		const bool bMovementHandlesYaw = CharacterMovement &&
			(CharacterMovement->bOrientRotationToMovement ||
			 CharacterMovement->HasYawDrivenByRootMotion());

		FRotator R = CapsuleComponent->GetRelativeRotation();
		bool bChanged = false;
		if (bUseControllerRotationYaw && !bMovementHandlesYaw)
		{
			R.Yaw   = ControlRotation.Yaw;
			bChanged = true;
		}
		if (bUseControllerRotationPitch)
		{
			R.Pitch = ControlRotation.Pitch;
			bChanged = true;
		}
		if (bUseControllerRotationRoll)
		{
			R.Roll  = ControlRotation.Roll;
			bChanged = true;
		}
		if (bChanged) CapsuleComponent->SetRelativeRotation(R);
	}
}
