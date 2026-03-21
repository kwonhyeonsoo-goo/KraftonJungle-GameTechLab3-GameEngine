#include "RenderBus.h"
#include "World/Mesh/MeshManager.h"

#if DEBUG

#include <iostream>

#endif

void FRenderBus::Create(ID3D11Device* InDevice)
{
	FontCache.SetFontData();
	FontCache.Initialize();
	LineBatch.Create(InDevice);
}

void FRenderBus::Release()
{
	LineBatch.Release();
}


void FRenderBus::Clear()
{
	ComponentCommands.clear();
	DepthLessCommands.clear();
	//EditorCommands.clear();
	//EditorGridCommands.clear();
	OverlayCommands.clear();
	OutlineCommands.clear();
	LineBatch.Clear();
	FontCommands.clear();
}

void FRenderBus::AddComponentCommand(const FRenderCommand& InCommand)
{
	if (InCommand.MeshBuffer == nullptr)
	{
		return;
	}

	ComponentCommands.push_back(InCommand);
}

void FRenderBus::AddDepthLessCommand(const FRenderCommand& InCommand)
{
	if (InCommand.MeshBuffer == nullptr)
	{
		return;
	}
	DepthLessCommands.push_back(InCommand);
}

void FRenderBus::AddLineBatchCommand(const FRenderCommand& InCommand)
{
	const FMeshData& MeshData = FMeshManager::GetBox();
	if (MeshData.Vertices.empty() || MeshData.Indices.size() < 2) return;

	CachedView = InCommand.TransformConstants.View;
	CachedProjection = InCommand.TransformConstants.Projection;

	const FMatrix& Model = InCommand.TransformConstants.Model;
	const TArray<FVertex>& Vertices = MeshData.Vertices;
	const TArray<uint32>& Indices = MeshData.Indices;

	for (int32 i = 0; i + 1 < static_cast<int32>(Indices.size()); i += 2)
	{
		FVector Start = Model.TransformPositionWithW(Vertices[Indices[i ]].Position);
		FVector End   = Model.TransformPositionWithW(Vertices[Indices[i + 1]].Position);
		LineBatch.AddLine(Start, End, Vertices[Indices[i]].Color);
	}
}

void FRenderBus::AddFontCommand(const FRenderCommand& InCommand)
{
	FontCommands.push_back(InCommand);
}

//void FRenderBus::AddEditorCommand(const FRenderCommand& InCommand)
//{
//	if(InCommand.MeshBuffer == nullptr)
//	{
//		return;
//	}
//	EditorCommands.push_back(InCommand);
//}
//
//void FRenderBus::AddGridEditorCommand(const FRenderCommand& InCommand)
//{
//	if (InCommand.MeshBuffer == nullptr)
//	{
//		return;
//	}
//	EditorGridCommands.push_back(InCommand);
//}

void FRenderBus::AddOutlineCommand(const FRenderCommand& InCommand)
{
	if(InCommand.MeshBuffer == nullptr)
	{
		return;
	}
	OutlineCommands.push_back(InCommand);
}

void FRenderBus::AddOverlayCommand(const FRenderCommand& InCommand)
{
	if (InCommand.MeshBuffer == nullptr)
	{
		return;
	}
	OverlayCommands.push_back(InCommand);
}

