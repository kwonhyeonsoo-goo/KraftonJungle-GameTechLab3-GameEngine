#pragma once
#include "../../Foundation/Core/CoreTypes.h"

struct AppContext;

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;
    virtual void OnRender(AppContext& ctx) = 0;
    virtual FString GetName() const = 0;
    bool bVisible = true;
};