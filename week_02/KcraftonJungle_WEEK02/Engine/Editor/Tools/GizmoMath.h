#pragma once
#include "ITool.h"

enum class ETransformMode { Translate, Rotate, Scale };
enum class ECoordSpace { World, Local };

class ToolContext {
public:
    ToolContext();

    ITool* GetActiveTool()  const;
    ETransformMode GetMode()        const;
    ECoordSpace    GetCoordSpace()  const;
    float          GetSnapValue()   const;
    bool           IsSnapEnabled()  const;

    // ★ SetMode()는 ETransformMode를 갱신하고,
    //   대응하는 ActiveTool(Translate/Rotate/Scale)을 함께 전환한다.
    //   Mode와 ActiveTool은 항상 일관된 상태를 유지한다.
    //   SelectTool이 활성 중일 때 SetMode()를 호출하면 즉시 Transform 툴로 전환된다.
    void SetMode(ETransformMode mode);    // Space Bar로 전환
    void SetCoordSpace(ECoordSpace cs);
    void SetSnapEnabled(bool enabled);
    void SetSnapValue(float value);

    // 툴 등록 및 활성화
    // ★ 소유권: ToolContext는 비소유 포인터만 등록한다.
    //   툴 인스턴스의 실소유자는 AppContext이며,
    //   AppContext::Shutdown()에서 툴 메모리를 해제한다.
    void RegisterTool(ITool* tool);
    void ActivateTool(const FString& name);

    TDelegate<ETransformMode> OnModeChanged;

private:
    TMap<FString, ITool*> Tools;
    ITool* ActiveTool = nullptr;
    ETransformMode        Mode = ETransformMode::Translate;
    ECoordSpace           CoordSpace = ECoordSpace::World;
    bool                  SnapEnabled = false;
    float                 SnapValue = 0.25f;
};