#include "ActorSequenceComponent.h"

#include "FloatCurve/FloatCurveAsset.h"
#include "FloatCurve/FloatCurveManager.h"
#include "GameFramework/AActor.h"
#include "Object/GarbageCollection.h"
#include "Object/Object.h"
#include "Serialization/Archive.h"

#include <algorithm>
#include <cstring>

UActorSequenceComponent::UActorSequenceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bTickEnabled = true;
}

void UActorSequenceComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	EnsurePlayers();
	InitializePlayers();

	if (bAutoPlay)
	{
		Play();
	}
}

void UActorSequenceComponent::EndPlay()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop(true);
	}
	if (PreviewSequencePlayer)
	{
		PreviewSequencePlayer->Stop(true);
	}
}

void UActorSequenceComponent::AddReferencedObjects(FReferenceCollector& Collector)
{
	UActorComponent::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(Sequence, "ActorSequenceComponent.Sequence");
	Collector.AddReferencedObject(SequencePlayer, "ActorSequenceComponent.SequencePlayer");
	Collector.AddReferencedObject(PreviewSequencePlayer, "ActorSequenceComponent.PreviewSequencePlayer");
}

void UActorSequenceComponent::OnPreSave(FArchive& Ar)
{
	UActorComponent::OnPreSave(Ar);
	SyncSequenceDataFromRuntime();
}

void UActorSequenceComponent::OnPostLoad(FArchive& Ar)
{
	UActorComponent::OnPostLoad(Ar);
	SyncRuntimeFromSequenceData();
	EnsurePlayers();
	InitializePlayers();
}

void UActorSequenceComponent::PostDuplicate()
{
	UActorComponent::PostDuplicate();
	SyncRuntimeFromSequenceData();
	EnsurePlayers();
	InitializePlayers();
}

void UActorSequenceComponent::PostEditProperty(const char* PropertyName)
{
	UActorComponent::PostEditProperty(PropertyName);
	if (PropertyName && std::strcmp(PropertyName, "SequenceDataJson") == 0)
	{
		SyncRuntimeFromSequenceData();
	}
	PlayRate = std::max(0.0f, PlayRate);
	StartOffsetSeconds = std::max(0.0f, StartOffsetSeconds);
}

void UActorSequenceComponent::Play()
{
	EnsurePlayers();
	if (!SequencePlayer)
	{
		return;
	}

	InitializeRuntimePlayer();
	SequencePlayer->SetPlaybackOptions(bLoop, bPauseAtEnd);
	SequencePlayer->SetCurrentTime(StartOffsetSeconds);
	SequencePlayer->Play(false);
}

void UActorSequenceComponent::Pause()
{
	if (SequencePlayer)
	{
		SequencePlayer->Pause();
	}
}

void UActorSequenceComponent::Stop()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop(true);
	}
}

UActorSequence* UActorSequenceComponent::GetSequence()
{
	EnsureSequence();
	return Sequence;
}

UActorSequencePlayer* UActorSequenceComponent::GetSequencePlayer()
{
	EnsurePlayers();
	return SequencePlayer;
}

UActorSequencePlayer* UActorSequenceComponent::GetPreviewSequencePlayer()
{
	EnsurePlayers();
	return PreviewSequencePlayer;
}

bool UActorSequenceComponent::AddFloatTrack(
	const FString& TargetObjectName,
	const FString& PropertyName,
	const FString& ChannelName,
	float StartTime,
	float Duration,
	const FString& CurveAssetPath)
{
	EnsureSequence();
	UObject* TargetObject = ResolveTargetByName(TargetObjectName);
	if (!IsValid(TargetObject) || !Sequence)
	{
		return false;
	}

	UFloatCurveAsset* Curve = !CurveAssetPath.empty() ? FFloatCurveManager::Get().Load(CurveAssetPath) : nullptr;
	const bool bAdded = Sequence->AddFloatTrack(
		TargetObject,
		PropertyName,
		ChannelName,
		StartTime,
		Duration,
		Curve,
		CurveAssetPath);
	if (bAdded)
	{
		SyncSequenceDataFromRuntime();
	}
	return bAdded;
}

void UActorSequenceComponent::PreviewPlay()
{
	EnsurePlayers();
	if (!PreviewSequencePlayer)
	{
		return;
	}

	InitializePreviewPlayer();
	PreviewSequencePlayer->SetPlaybackOptions(bLoop, true);
	PreviewSequencePlayer->Play(false);
}

void UActorSequenceComponent::PreviewPause()
{
	if (PreviewSequencePlayer)
	{
		PreviewSequencePlayer->Pause();
	}
}

void UActorSequenceComponent::PreviewStop()
{
	if (PreviewSequencePlayer)
	{
		PreviewSequencePlayer->Stop(true);
	}
}

void UActorSequenceComponent::SetPreviewTime(float Time)
{
	EnsurePlayers();
	if (PreviewSequencePlayer)
	{
		InitializePreviewPlayer();
		PreviewSequencePlayer->SetCurrentTime(Time);
	}
}

void UActorSequenceComponent::CommitSequenceEditsForSerialization()
{
	SyncSequenceDataFromRuntime();
}

void UActorSequenceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
	UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (SequencePlayer && SequencePlayer->IsPlaying())
	{
		SequencePlayer->Tick(DeltaTime * std::max(0.0f, PlayRate));
	}
}

void UActorSequenceComponent::EnsureSequence()
{
	if (!IsValid(Sequence))
	{
		Sequence = UObjectManager::Get().CreateObject<UActorSequence>(this);
	}
}

void UActorSequenceComponent::EnsurePlayers()
{
	EnsureSequence();

	AActor* Owner = GetOwner();
	if (!IsValid(SequencePlayer))
	{
		SequencePlayer = UObjectManager::Get().CreateObject<UActorSequencePlayer>(this);
		if (SequencePlayer)
		{
			SequencePlayer->Initialize(Sequence, Owner);
		}
	}
	if (!IsValid(PreviewSequencePlayer))
	{
		PreviewSequencePlayer = UObjectManager::Get().CreateObject<UActorSequencePlayer>(this);
		if (PreviewSequencePlayer)
		{
			PreviewSequencePlayer->Initialize(Sequence, Owner);
		}
	}
}

void UActorSequenceComponent::InitializeRuntimePlayer()
{
	AActor* Owner = GetOwner();
	if (SequencePlayer)
	{
		SequencePlayer->Initialize(Sequence, Owner);
	}
}

void UActorSequenceComponent::InitializePreviewPlayer()
{
	AActor* Owner = GetOwner();
	if (PreviewSequencePlayer)
	{
		PreviewSequencePlayer->Initialize(Sequence, Owner);
	}
}

void UActorSequenceComponent::InitializePlayers()
{
	InitializeRuntimePlayer();
	InitializePreviewPlayer();
}

UObject* UActorSequenceComponent::ResolveTargetByName(const FString& TargetObjectName) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	if (TargetObjectName.empty() || TargetObjectName == "Owner" || TargetObjectName == Owner->GetFName().ToString())
	{
		return Owner;
	}

	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (!IsValid(Component))
		{
			continue;
		}

		if (Component->GetFName().ToString() == TargetObjectName
			|| Component->GetPersistentGuid() == TargetObjectName)
		{
			return Component;
		}
	}
	return nullptr;
}

void UActorSequenceComponent::SyncSequenceDataFromRuntime()
{
	if (IsValid(Sequence))
	{
		Sequence->RefreshBindingTargetCache(GetOwner());
		SequenceDataJson = Sequence->ExportToJsonString();
	}
	else
	{
		SequenceDataJson.clear();
	}
}

void UActorSequenceComponent::SyncRuntimeFromSequenceData()
{
	EnsureSequence();
	if (Sequence)
	{
		Sequence->ImportFromJsonString(SequenceDataJson);
		Sequence->RefreshBindingTargetCache(GetOwner());
	}
}
