#include "TextRenderComponent.h"
#include "Renderer/Renderer.h"
#include "Object/Class.h"
#include "Actor/Actor.h"
#include "Primitive/PrimitiveBase.h"
#include "Serializer/Archive.h"
#include <algorithm>

IMPLEMENT_RTTI(UTextRenderComponent, UPrimitiveComponent)

void UTextRenderComponent::Initialize()
{
	bDrawDebugBounds = false;
	TextMesh = std::make_shared<FMeshData>();
}

void UTextRenderComponent::SetText(const FString& InText)
{
	if (Text != InText)
	{
		Text = InText;
		if (TextMesh) TextMesh->MarkDirty();
	}
	MarkTransformDirty();
}
void UTextRenderComponent::SetTextColor(const FVector4& InColor)
{
	TextColor = InColor;

	if (DynamicMaterial)
	{
		DynamicMaterial->SetParameterData("TextColor", &TextColor, 16);
	}
	MarkTransformDirty();
}
FString UTextRenderComponent::GetDisplayText() const
{
	if (Text.empty())
	{
		if (AActor* OwnerActor = GetOwner())
		{
			return FString("UUID : ") + OwnerActor->GetUUIDString();
		}
		return "TextRenderComponent";
	}
	return Text;
}
void UTextRenderComponent::DuplicateSubObjects()
{
	DynamicMaterial.reset();
	SetMaterial(nullptr);
}
FDynamicMaterial* UTextRenderComponent::GetOrCreateDynamicMaterial()
{
	if (!DynamicMaterial)
	{
		extern ENGINE_API FRenderer* GRenderer;
		if (GRenderer)
		{
			FMaterial* BaseFontMat = GRenderer->GetTextRenderer().GetFontMaterial();
			if (BaseFontMat)
			{
				DynamicMaterial = BaseFontMat->CreateDynamicMaterial();
				DynamicMaterial->SetParameterData("TextColor", &TextColor, 16);
				SetMaterial(DynamicMaterial.get());
			}
		}
	}
	return DynamicMaterial.get();
}
FBoxSphereBounds UTextRenderComponent::GetWorldBounds() const
{
	const FVector Center = GetRenderWorldPosition();
	const FVector Extent(TextScale * 3.0f, TextScale * 3.0f, TextScale * 3.0f);
	return { Center, Extent.Size(), Extent };
}



void UTextRenderComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);

	if (Ar.IsSaving())
	{
		Ar.Serialize("Text", Text);
		Ar.Serialize("TextColor", TextColor);
		Ar.Serialize("TextScale", TextScale);
		Ar.Serialize("Billboard", bBillboard);
		Ar.Serialize("WorldOffset", WorldOffset);
	}
	else // IsLoading
	{
		if (Ar.Contains("Text")) { Ar.Serialize("Text", Text); }
		if (Ar.Contains("TextColor")) { Ar.Serialize("TextColor", TextColor); }
		if (Ar.Contains("TextScale")) { Ar.Serialize("TextScale", TextScale); }
		if (Ar.Contains("Billboard")) { Ar.Serialize("Billboard", bBillboard); }
		if (Ar.Contains("WorldOffset")) { Ar.Serialize("WorldOffset", WorldOffset); }

	}
}