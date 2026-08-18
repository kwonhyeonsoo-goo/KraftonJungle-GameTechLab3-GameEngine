#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "./Tools/ITool.h"

enum class ETransformMode { Translate, Rotate, Scale };
enum class ECoordSpace { World, Local };

class ToolContext {
public:
    ToolContext();

    ITool* GetActiveTool() const;
    ETransformMode GetMode() const;
    ECoordSpace GetCoordSpace() const;
    float GetSnapValue() const;
    bool IsSnapEnabled() const;

    void SetMode(ETransformMode mode);
    void SetCoordSpace(ECoordSpace cs);
    void SetSnapEnabled(bool enabled);
    void SetSnapValue(float value);
    void SetUniformScale(bool uniform);
    bool GetUniformScale() const;

    void RegisterTool(ITool* tool);
    void ActivateTool(const FString& name);

    TDelegate<ETransformMode> OnModeChanged;

private:
    TMap<FString, ITool*> Tools;
    ITool* ActiveTool = nullptr;
    ETransformMode Mode = ETransformMode::Translate;
    ECoordSpace CoordSpace = ECoordSpace::World;
    bool SnapEnabled = false;
    float SnapValue = 0.25f;
    bool bUniformScale = false;
};