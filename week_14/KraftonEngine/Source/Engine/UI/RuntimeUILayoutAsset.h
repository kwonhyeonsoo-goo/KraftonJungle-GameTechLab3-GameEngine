#pragma once

#include "Math/Vector.h"
#include "Object/Object.h"

#include "Source/Engine/UI/RuntimeUILayoutAsset.generated.h"

class FArchive;

enum class ERuntimeUIWidgetType : uint8
{
	Canvas = 0,
	Panel,
	Text,
	Image,
	Button,
};

enum class ERuntimeUIImageFit : uint8
{
	Stretch = 0,
	Contain,
	Cover,
};

struct FRuntimeUIWidgetNode
{
	FString Id;
	FString DisplayName;
	ERuntimeUIWidgetType Type = ERuntimeUIWidgetType::Panel;
	int32 ParentIndex = -1;
	TArray<int32> Children;

	FVector2 Position = FVector2(0.0f, 0.0f);
	FVector2 Size = FVector2(160.0f, 48.0f);
	FString Text;
	FString ImagePath;
	FString StyleClass;
	FString OnClickAction;

	FVector4 BackgroundColor = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	FVector4 TextColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	FVector4 BorderColor = FVector4(1.0f, 1.0f, 1.0f, 0.0f);
	FVector4 BorderWidth = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	FVector4 Padding = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	float BorderRadius = 0.0f;
	float FontSize = 24.0f;
	float Opacity = 1.0f;
	ERuntimeUIImageFit ImageFit = ERuntimeUIImageFit::Stretch;
	bool bVisible = true;
};

UCLASS()
class URuntimeUILayoutAsset : public UObject
{
public:
	GENERATED_BODY()

	static constexpr int32 CurrentPayloadVersion = 1;

	URuntimeUILayoutAsset();

	void Serialize(FArchive& Ar) override;

	void ResetToDefault();
	int32 AddWidget(ERuntimeUIWidgetType Type, int32 ParentIndex);
	bool RemoveWidget(int32 WidgetIndex);

	FRuntimeUIWidgetNode* GetMutableWidget(int32 WidgetIndex);
	const FRuntimeUIWidgetNode* GetWidget(int32 WidgetIndex) const;
	TArray<FRuntimeUIWidgetNode>& GetMutableWidgets() { return Widgets; }
	const TArray<FRuntimeUIWidgetNode>& GetWidgets() const { return Widgets; }
	void SetCanvasSize(const FVector2& InCanvasSize);
	const FVector2& GetCanvasSize() const { return CanvasSize; }

	void SetAssetPath(const FString& InPath) { AssetPath = InPath; }
	const FString& GetAssetPath() const { return AssetPath; }
	void SetGeneratedPaths(const FString& InRmlPath, const FString& InRcssPath);
	const FString& GetGeneratedRmlPath() const { return GeneratedRmlPath; }
	const FString& GetGeneratedRcssPath() const { return GeneratedRcssPath; }

	bool ValidateForExport(FString* OutError = nullptr) const;
	bool ExportRmlAndRcss(const FString& RmlPath, const FString& RcssPath, FString* OutError = nullptr) const;

private:
	FString MakeUniqueWidgetId(ERuntimeUIWidgetType Type) const;
	bool IsValidWidgetIndex(int32 WidgetIndex) const;
	void RebuildChildrenFromParents();

private:
	FString AssetPath;
	FString GeneratedRmlPath;
	FString GeneratedRcssPath;
	FVector2 CanvasSize = FVector2(1920.0f, 1080.0f);
	TArray<FRuntimeUIWidgetNode> Widgets;
};

FArchive& operator<<(FArchive& Ar, FRuntimeUIWidgetNode& Node);
