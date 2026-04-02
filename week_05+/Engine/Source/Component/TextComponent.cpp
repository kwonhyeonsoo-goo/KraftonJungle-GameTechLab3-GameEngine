#include "TextComponent.h"
#include "Object/Class.h"
#include "Primitive/PrimitiveBase.h"
#include "Serializer/Archive.h"
#include <algorithm>

IMPLEMENT_RTTI(UTextComponent, UPrimitiveComponent)

void UTextComponent::Initialize()
{
	// 폰트 렌더링용 메시 데이터 객체 생성
	bDrawDebugBounds = true;
	TextMesh = std::make_shared<FMeshData>();
}

void UTextComponent::SetText(const FString& InText)
{
	if (Text != InText)
	{
		Text = InText;
		// NOTE: 실제 정점 데이터 갱신은 RenderCollector에서 TextRenderer를 통해 수행함
	}
}

FBoxSphereBounds UTextComponent::GetWorldBounds() const
{
	const FVector Center = GetRenderWorldPosition();
	const FString DisplayText = GetDisplayText();
	const size_t TextLength = std::max<size_t>(DisplayText.size(), 1);

	const FVector RenderScale = GetRenderWorldScale();
	const float BaseScale = std::max(
		std::max(RenderScale.X, RenderScale.Y),
		std::max(RenderScale.Z, 0.3f)
	);

	const float HalfWidth = static_cast<float>(TextLength) * BaseScale * 0.35f;
	const float HalfHeight = BaseScale * 0.5f;
	const float HalfDepth = BaseScale * 0.15f;

	const FVector BoxExtent(HalfDepth, HalfWidth, HalfHeight);
	return { Center, BoxExtent.Size(), BoxExtent };
}



void UTextComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);

	if (Ar.IsSaving())
	{
		FString TextStr = GetText();
		FVector4 Color4 = GetTextColor();
		bool bIsBillboard = IsBillboard();

		Ar.Serialize("Text", TextStr);
		Ar.Serialize("TextColor", Color4);
		Ar.Serialize("Billboard", bIsBillboard);
	}
	else
	{
		if (Ar.Contains("Text")) { FString S; Ar.Serialize("Text", S); SetText(S); }
		if (Ar.Contains("TextColor")) { FVector4 C; Ar.Serialize("TextColor", C); SetTextColor(C); }
		if (Ar.Contains("Billboard")) { bool B; Ar.Serialize("Billboard", B); SetBillboard(B); }
	}
}