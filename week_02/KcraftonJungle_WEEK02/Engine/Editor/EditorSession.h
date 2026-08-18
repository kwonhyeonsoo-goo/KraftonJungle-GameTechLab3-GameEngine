#pragma once

#include "../Foundation/Containers/TArray.h"
#include "./SelectionSet.h"
#include "./ToolContext.h"
#include "./Viewport/EditorViewport.h"

struct InputState;
class USceneComponent;

class EditorSession
{
public:
    EditorSession();

public:
    SelectionSet Selection;
    ToolContext  Tools;

private:
    TArray<FEditorViewport> Viewports;
    int32 ActiveViewportIndex = 0;

public:
    // 세션은 "활성 뷰포트/카메라를 관리"만 합니다.
    FEditorViewport& GetActiveViewport();
    const FEditorViewport& GetActiveViewport() const;

    FEditorCameraState& GetActiveCamera();
    const FEditorCameraState& GetActiveCamera() const;

    const TArray<FEditorViewport>& GetViewports() const;

    // 필요 시 나중에 UI에서 active viewport를 전환
    void SetActiveViewportIndex(int32 Index);
    int32 GetActiveViewportIndex() const;

    // 세션 API는 유지하되 내부는 controller / viewport 위임
    void ProcessCameraInput(const InputState& input, float deltaTime);

    FMatrix GetProjectionMatrix() const;
    FMatrix GetOrthogonalMatrix() const;
    FMatrix GetViewProjMatrix() const;
    FMatrix GetViewOrthoMatrix() const;

    void FocusObject(const USceneComponent* comp, float distance);
};