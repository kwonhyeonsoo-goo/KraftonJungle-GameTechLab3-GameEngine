#pragma once
#include <d3d11.h>
#include "UPrimitiveComponent.h"

class UMesh :
    public UPrimitiveComponent
{
public:
private:
    ID3D11Buffer* vertexBuffer;
};

