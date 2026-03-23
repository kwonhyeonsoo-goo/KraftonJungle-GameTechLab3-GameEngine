#include "Scene.h"

#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Actor/SubUVActor.h"
#include "Camera/Camera.h"
#include "Component/CameraComponent.h"
#include "Object/ObjectFactory.h"
#include "Component/PrimitiveComponent.h"
#include "Math/Frustum.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderCommand.h"

#include "Component/UUIDBillboardComponent.h"
#include "Object/Class.h"
#include "Core/FEngine.h"
#include "Component/SubUVComponent.h"

#include "Serializer/SceneSerializer.h"
#include <algorithm>



#include "Component/LineBatchComponent.h"

IMPLEMENT_RTTI(UScene, UObject)

UScene::~UScene()
{
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();

	if (SceneCameraComponent)
	{
		SceneCameraComponent->MarkPendingKill();
	}

	if (ActiveCameraComponent == SceneCameraComponent)
	{
		ActiveCameraComponent = nullptr;
	}
	SceneCameraComponent = nullptr;
}

void UScene::SetActiveCameraComponent(UCameraComponent* InCameraComponent)
{
	ActiveCameraComponent = InCameraComponent ? InCameraComponent : SceneCameraComponent;
}

UCameraComponent* UScene::GetActiveCameraComponent() const
{
	return ActiveCameraComponent ? ActiveCameraComponent.Get() : SceneCameraComponent;
}

CCamera* UScene::GetCamera() const
{
	UCameraComponent* CameraComponent = GetActiveCameraComponent();
	return CameraComponent ? CameraComponent->GetCamera() : nullptr;
}

void UScene::InitializeEmptyScene(float AspectRatio)
{
	if (SceneCameraComponent == nullptr)
	{
		SceneCameraComponent = FObjectFactory::ConstructObject<UCameraComponent>(this, "SceneCamera");
	}

	if (ActiveCameraComponent == nullptr)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}

	if (SceneCameraComponent->GetCamera())
	{
		SceneCameraComponent->GetCamera()->SetAspectRatio(AspectRatio);
	}

	if (ActiveCameraComponent != SceneCameraComponent && GetCamera())
	{
		GetCamera()->SetAspectRatio(AspectRatio);
	}
}

void UScene::InitializeDefaultScene(float AspectRatio, ID3D11Device* Device)
{
	InitializeEmptyScene(AspectRatio);
	//LoadSceneFromFile((FPaths::SceneDir() / "DefaultScene.json").string(), Device);
	FSceneSerializer::Load(this, (FPaths::SceneDir() / "DefaultScene.json").string(), Device);
}

void UScene::ClearActors()
{
	if (ActiveCameraComponent != SceneCameraComponent)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}

	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();

	bBegunPlay = false;
}

void UScene::RegisterActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	const auto It = std::find(Actors.begin(), Actors.end(), InActor);
	if (It != Actors.end())
	{
		return;
	}

	Actors.push_back(InActor);
	InActor->SetScene(this);
}

void UScene::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	if (ActiveCameraComponent && ActiveCameraComponent != SceneCameraComponent)
	{
		for (UActorComponent* Component : InActor->GetComponents())
		{
			if (Component == ActiveCameraComponent)
			{
				ActiveCameraComponent = SceneCameraComponent;
				break;
			}
		}
	}

	InActor->Destroy();
}

void UScene::CleanupDestroyedActors()
{
	const auto NewEnd = std::ranges::remove_if(Actors,
		[](const AActor* Actor)
		{
			return Actor == nullptr || Actor->IsPendingDestroy();
		}).begin();

	Actors.erase(NewEnd, Actors.end());
}

void UScene::BeginPlay()
{
	if (bBegunPlay)
	{
		return;
	}

	bBegunPlay = true;

	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->HasBegunPlay())
		{
			Actor->BeginPlay();
		}
	}
}

void UScene::Tick(float DeltaTime)
{
	if (!bBegunPlay)
	{
		BeginPlay();
	}

	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->IsPendingDestroy())
		{
			Actor->Tick(DeltaTime);
		}
	}

	CleanupDestroyedActors();
}

void UScene::FrustrumCull(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible)
{
	
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}
		if (!Actor->IsVisible())
		{
			continue;
		}
		for (UActorComponent* Component : Actor->GetComponents())
		{
		
			if (!Component->IsA(UPrimitiveComponent::StaticClass()))
			{
				continue;
			}

			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);

			const bool bIsUUID = PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass());
			const bool bIsSubUV = PrimitiveComponent->IsA(USubUVComponent::StaticClass());

			if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_UUID))
				{
					continue;
				}
			}
			else if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
			{
				// SubUV??mesh data 체크 불필??
			}
			else
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives))
				{
					continue;
				}
				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
				{
					continue;
				}
			}

			if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
			{
				OutVisible.push_back(PrimitiveComponent);
			}
		}  // for Component
	}  // for Actor
}  // FrustrumCull ??

void UScene::CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{


	TArray<UPrimitiveComponent*> VisiblePrimitives;
	FrustrumCull(Frustum, VisiblePrimitives);

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}
		if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
		{
		
			UUUIDBillboardComponent* UUIDComponent =
				static_cast<UUUIDBillboardComponent*>(PrimitiveComponent);

			FTextRenderCommand TextCmd;
			TextCmd.Text = UUIDComponent->GetDisplayText();
			TextCmd.WorldPosition = UUIDComponent->GetTextWorldPosition();
			TextCmd.WorldScale = UUIDComponent->GetWorldScale();
			TextCmd.Color = UUIDComponent->GetTextColor();

			OutQueue.AddTextCommand(TextCmd);
			continue;
		}
		if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
		{
			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(PrimitiveComponent);
			FSubUVRenderCommand SubUVCmd;
			SubUVCmd.WorldMatrix = SubUVComponent->GetWorldTransform();
			SubUVCmd.Size = SubUVComponent->GetSize();
			SubUVCmd.Columns = SubUVComponent->GetColumns();
			SubUVCmd.Rows = SubUVComponent->GetRows();
			SubUVCmd.TotalFrames = SubUVComponent->GetTotalFrames();
			SubUVCmd.FPS = SubUVComponent->GetFPS();
			SubUVCmd.ElapsedTime = static_cast<float>(GEngine->GetCore()->GetTimer().GetTotalTime());
			SubUVCmd.bLoop = SubUVComponent->IsLoop();
			SubUVCmd.bBillboard = SubUVComponent->IsBillboard();
			SubUVCmd.FirstFrame = SubUVComponent->GetFirstFrame();
			SubUVCmd.LastFrame = SubUVComponent->GetLastFrame();

			OutQueue.AddSubUVCommand(SubUVCmd);
			continue;
		}

		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			continue;
		}

		FRenderCommand Command;
		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
		Command.Material = PrimitiveComponent->GetMaterial();
		OutQueue.AddCommand(Command);
	}
}
