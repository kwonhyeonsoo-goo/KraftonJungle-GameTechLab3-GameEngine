#include "GameFramework/Pawn/WheeledVehiclePawn.h"

#include "Component/Camera/CameraComponent.h"
#include "Component/Camera/SpringArmComponent.h"
#include "Component/Input/InputComponent.h"
#include "Component/Movement/WheeledVehicleMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Vehicle/VehicleWheelPoseComponent.h"
#include "Mesh/MeshManager.h"
#include "Runtime/Engine.h"

void AWheeledVehiclePawn::InitDefaultComponents(const FString& SkeletalMeshFileName)
{
    Mesh = AddComponent<USkeletalMeshComponent>();
    SetRootComponent(Mesh);

    ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
    if (!SkeletalMeshFileName.empty())
    {
        USkeletalMesh* Asset = FMeshManager::LoadSkeletalMesh(SkeletalMeshFileName, Device);
        Mesh->SetSkeletalMesh(Asset);
    }

    VehicleMovement = AddComponent<UWheeledVehicleMovementComponent>();
    VehicleMovement->SetUpdatedComponent(Mesh);

    WheelPose = AddComponent<UVehicleWheelPoseComponent>();
    WheelPose->SetVehicleMovement(VehicleMovement);
    WheelPose->SetSkeletalMeshComponent(Mesh);

    SpringArm = AddComponent<USpringArmComponent>();
    SpringArm->AttachToComponent(Mesh);
    SpringArm->TargetArmLength = 7.0f;
    SpringArm->SocketOffset = FVector(0.0f, 0.0f, 2.5f);
    SpringArm->bEnableCameraLag = true;
    SpringArm->bEnableCameraRotationLag = true;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bInheritPitch = true;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;

    Camera = AddComponent<UCameraComponent>();
    Camera->AttachToComponent(SpringArm);
}

void AWheeledVehiclePawn::PostDuplicate()
{
    Super::PostDuplicate();
    Mesh = GetComponentByClass<USkeletalMeshComponent>();
    VehicleMovement = GetComponentByClass<UWheeledVehicleMovementComponent>();
    WheelPose = GetComponentByClass<UVehicleWheelPoseComponent>();
    SpringArm = GetComponentByClass<USpringArmComponent>();
    Camera = GetComponentByClass<UCameraComponent>();

    if (VehicleMovement && Mesh)
    {
        VehicleMovement->SetUpdatedComponent(Mesh);
    }
    if (WheelPose)
    {
        WheelPose->SetVehicleMovement(VehicleMovement);
        WheelPose->SetSkeletalMeshComponent(Mesh);
    }
}

void AWheeledVehiclePawn::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!bAutoInputVehicle || !InputComponent || !VehicleMovement)
    {
        return;
    }

    InputComponent->AddAxisMapping("VehicleThrottle", "W",  1.0f);
    InputComponent->AddAxisMapping("VehicleThrottle", "S", -1.0f);
    InputComponent->AddAxisMapping("VehicleSteering", "D",  1.0f);
    InputComponent->AddAxisMapping("VehicleSteering", "A", -1.0f);
    InputComponent->AddActionMapping("VehicleHandbrake", "Space");

    InputComponent->BindAxis("VehicleThrottle", [this](float Value)
    {
        if (!VehicleMovement)
        {
            return;
        }

        if (Value >= 0.0f)
        {
            VehicleMovement->SetThrottleInput(Value);
            VehicleMovement->SetBrakeInput(0.0f);
        }
        else
        {
            VehicleMovement->SetThrottleInput(0.0f);
            VehicleMovement->SetBrakeInput(-Value);
        }
    });

    InputComponent->BindAxis("VehicleSteering", [this](float Value)
    {
        if (VehicleMovement)
        {
            VehicleMovement->SetSteeringInput(Value);
        }
    });

    InputComponent->BindAction("VehicleHandbrake", EInputEvent::Pressed, [this]()
    {
        if (VehicleMovement)
        {
            VehicleMovement->SetHandbrakeInput(1.0f);
        }
    });

    InputComponent->BindAction("VehicleHandbrake", EInputEvent::Released, [this]()
    {
        if (VehicleMovement)
        {
            VehicleMovement->SetHandbrakeInput(0.0f);
        }
    });
}
