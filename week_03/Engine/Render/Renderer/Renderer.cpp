#pragma comment( lib, "dxguid.lib")

#include "Renderer.h"

#include "Render/Common/RenderTypes.h"
#include "DirectXTK/WICTextureLoader.h"
#include "Engine/EngineServices/EngineServices.h"

#if DEBUG

#include <iostream>
#include <cassert>

#endif

#include "World/Mesh/MeshManager.h"

void FRenderer::Create(HWND hWindow)
{
	Device.Create(hWindow);

	if (Device.GetDevice() == nullptr)
	{
		std::cout << "Failed to create D3D Device." << std::endl;
	}


	RenderResourceManager.Create(&Device);
	FEngineServices::RegisterResourceManager(&RenderResourceManager);

	Resources.PrimitiveShader.Create(Device.GetDevice(), ShaderFilePath,
		"PrimitiveVS", "PrimitivePS",PrimitiveInputLayout, ARRAYSIZE(PrimitiveInputLayout));
	Resources.GizmoShader.Create(Device.GetDevice(), ShaderFilePath,
		"GizmoVS", "GizmoPS", PrimitiveInputLayout, ARRAYSIZE(PrimitiveInputLayout));
	Resources.OverlayShader.Create(Device.GetDevice(), ShaderFilePath,
		"OverlayVS", "OverlayPS", OverlayInputLayout, ARRAYSIZE(OverlayInputLayout));
	// 픽셀 셰이더를 통한 그리드 그리기 비활성화
	//Resources.EditorShader.Create(Device.GetDevice(), ShaderFilePath,
	//	"EditorVS", "EditorPS", PrimitiveInputLayout, ARRAYSIZE(PrimitiveInputLayout));

	Resources.OutlineShader.Create(Device.GetDevice(), ShaderFilePath,
		"OutlineVS", "OutlinePS", PrimitiveInputLayout, ARRAYSIZE(PrimitiveInputLayout));

	Resources.FontShader.Create(Device.GetDevice(), FontShaderFilePath,
		"VS_Font", "PS_Font", FontInputLayout, ARRAYSIZE(FontInputLayout));

	Resources.SubUVShader.Create(Device.GetDevice(), SubUVShaderFilePath,
		"VS_Sprite", "PS_Sprite", SubUVInputLayout, ARRAYSIZE(SubUVInputLayout));

	Resources.PerObjectConstantBuffer.Create(Device.GetDevice(), sizeof(FTransformConstants));
	Resources.GizmoPerObjectConstantBuffer.Create(Device.GetDevice(), sizeof(FGizmoConstants));
	Resources.OverlayConstantBuffer.Create(Device.GetDevice(), sizeof(FOverlayConstants));

	Resources.SubUVConstantBuffer.Create(Device.GetDevice(), sizeof(FSubUVConstants));
	Resources.FontInstanceBuffer.CreateDynamic(Device.GetDevice(), 1024 * 4, sizeof(FFontInstance));


	// 픽셀 셰이더를 통한 그리드 비활성화 -> Line은 PerObjectConstantBuffer로 그릴 수 있음
	//Resources.EditorConstantBuffer.Create(Device.GetDevice(), sizeof(FEditorConstants));

	Resources.OutlineConstantBuffer.Create(Device.GetDevice(), sizeof(FOutlineConstants));

	CreateWICTextureFromFile(Device.GetDevice(), Device.GetDeviceContext(), FontTextureFIlePath, nullptr, &Resources.FontAtlasSRV);

	//여기에 SUb UV용 쉐이더 생성

	//	MeshManager init
	FMeshManager::Initialize();
}

void FRenderer::Release()
{
	Resources.PrimitiveShader.Release();
	Resources.GizmoShader.Release();
	Resources.OverlayShader.Release();
	Resources.EditorShader.Release();
	Resources.OutlineShader.Release();
	Resources.FontShader.Release();

	Resources.PerObjectConstantBuffer.Release();
	Resources.GizmoPerObjectConstantBuffer.Release();
	Resources.OverlayConstantBuffer.Release();
	//Resources.EditorConstantBuffer.Release();
	Resources.OutlineConstantBuffer.Release();

	Resources.SubUVConstantBuffer.Release();



	Resources.FontInstanceBuffer.Release();


	if (Resources.FontAtlasSRV) {
		Resources.FontAtlasSRV->Release();
		Resources.FontAtlasSRV = nullptr;
	}

	Device.Release();
}

//	Prepare the rendering state for a new frame. 반드시 Render 이전에 호출되어야 함.
void FRenderer::BeginFrame()
{
	Device.BeginFrame();
}

//	Render Update Main function. RenderBus에 담긴 모든 RenderCommand에 대해서 Draw Call 수행
void FRenderer::Render(FRenderBus& InRenderBus)
{
	ID3D11DeviceContext* context = Device.GetDeviceContext();

	//	순서 지켜야 함. (Component -> Axis -> Grid -> Outline -> Gizmo -> Overlay)
	//	State Caching으로 인해 중복 설정은 자동으로 스킵됨.

	//	Primitive
	if (showFlag & (uint64)EEngineShowFlags::SF_Primitives) {
		Device.SetDepthStencilState(EDepthStencilState::StencilWrite);
		if (viewMode == EViewModeIndex::VMI_Wireframe) Device.SetRasterizerState(ERasterizerState::WireFrame);
		else  Device.SetRasterizerState(ERasterizerState::SolidBackCull);
		Device.SetBlendState(EBlendState::Opaque);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		Resources.PrimitiveShader.Bind(context);
		RenderComponentPass(context, InRenderBus);

		//여기서부터 SubUV 출력
		Resources.SubUVShader.Bind(context);
		RenderSubUVCompPass(context, InRenderBus);
	}

	//	Grid And Box
	Resources.PrimitiveShader.Bind(context);
	Device.SetDepthStencilState(EDepthStencilState::Default);
	RenderLineBatchPass(context, InRenderBus);

	// Font Text
	if (showFlag & (uint64)EEngineShowFlags::SF_BillboardText) {
		Device.SetDepthStencilState(EDepthStencilState::None);
		Device.SetBlendState(EBlendState::AlphaBlend);
		DrawString(context, InRenderBus);
	}

	if (showFlag & (uint64)EEngineShowFlags::SF_Primitives) {
		//	Selection Outline (Stencil)
		Device.SetDepthStencilState(EDepthStencilState::StencilOutline);
		Device.SetRasterizerState(ERasterizerState::SolidFrontCull);
		Device.SetBlendState(EBlendState::Opaque);
		Resources.OutlineShader.Bind(context);
		RenderOutlinePass(context, InRenderBus);
	}

	// Gizmo (Depth-less Variable)
	Device.SetBlendState(EBlendState::Opaque);
	RenderDepthLessPass(context, InRenderBus);

	ID3D11VertexShader* vsCheck = Resources.FontShader.GetVertexShader();
	ID3D11PixelShader* psCheck = Resources.FontShader.GetPixelShader();

	//	Reset to default
	Device.SetRasterizerState(ERasterizerState::SolidBackCull);

	//	NOTE : Overlay는 반드시 따로 호출해야 함. (Engine Loop에서 돌고 있음)
}

void FRenderer::RenderOverlay(const FRenderBus& InRenderBus)
{
	ID3D11DeviceContext* context = Device.GetDeviceContext();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Device.SetDepthStencilState(EDepthStencilState::None);
	Device.SetBlendState(EBlendState::Opaque);
	Resources.OverlayShader.Bind(context);

	RenderOverlayPass(context, InRenderBus);
}


void FRenderer::RenderComponentPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus)
{
	for (const FRenderCommand& command : InRenderBus.GetComponentCommands())
	{
		DrawCommand(InDeviceContext, command);
	}
}

void FRenderer::RenderSubUVCompPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus)
{

	Device.SetDepthStencilState(EDepthStencilState::Default);
	Device.SetBlendState(EBlendState::AlphaBlend);

	//쉐이더 설정
	TArray<FRenderCommand> cmdarrya = InRenderBus.GetSubUVCommands();
	for (const FRenderCommand& command : cmdarrya)
	{

		//shaderTesourceView 설정
		ID3D11ShaderResourceView* srv = RenderResourceManager.GetTexture(command.TextureID).ShaderResourceView;
		
		InDeviceContext->PSSetShaderResources(0, 1, &srv);
		InDeviceContext->PSSetSamplers(0, 1, &(Device.SubUVSampler));
		//대충 drawcommand 날리기
		DrawUVCommand(InDeviceContext, command);
	}

}

void FRenderer::RenderDepthLessPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus)
{
	if (InRenderBus.GetDepthLessCommands().size() == 0) return;
	//	DepthLess (Gizmo)
	Device.SetDepthStencilState(EDepthStencilState::None);
	Device.SetBlendState(EBlendState::AlphaBlend);
	Resources.GizmoShader.Bind(InDeviceContext);

	InDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawCommand(InDeviceContext, InRenderBus.GetDepthLessCommands()[0]);
	//RenderDepthLessPass(InDeviceContext, InRenderBus);


	//	선택된 경우 그리지 않음
	if (InRenderBus.GetDepthLessCommands().size() < 2) return;

	//	Non-DepthLess (Gizmo)
	Device.SetDepthStencilState(EDepthStencilState::Default);
	Device.SetBlendState(EBlendState::Opaque);
	Resources.GizmoShader.Bind(InDeviceContext);

	InDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawCommand(InDeviceContext, InRenderBus.GetDepthLessCommands()[1]);

}

void FRenderer::RenderLineBatchPass(ID3D11DeviceContext* InDeviceContext, FRenderBus& InRenderBus)
{
	FBatchedLine* BatchedLine = InRenderBus.GetBatehdLine();
	if (!BatchedLine->HasLines())
	{
		return;
	}

	BatchedLine->Update(InDeviceContext);
	InDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	DrawCommand(InDeviceContext, InRenderBus.GetBatchLineCommand());
	InDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FRenderer::DrawString(ID3D11DeviceContext* InDeviceContext, FRenderBus& InRenderBus)
{
	FFontCache& FontCache = InRenderBus.GetFontCache();

	TArray<FFontInstance> Instances;

	float FontScale = 0.1f;
	const float CellW = (float)FontCache.GetFontData().CellWidth * FontScale;
	const float CellH = (float)FontCache.GetFontData().CellHeight * FontScale;

	for (const auto& Cmd : InRenderBus.GetFontCommands()) {
		FVector4 colorData = Cmd.FontColor;

		// 각 String의 MVP
		// View^(-1) (Z up) -> 전치
		FMatrix View = InRenderBus.GetCachedView();

		FMatrix BillboardRotation =
			FMatrix(
				1, 0, 0, 0,
				-View.Data[0], -View.Data[4], 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);

		// Scale, Translation 행렬
		FMatrix ScaleMatrix = FMatrix::MakeScaleMatrix(FVector(FontScale, FontScale, FontScale));
		FMatrix Translation = FMatrix::MakeTranslationMatrix(Cmd.FontPosition);

		FMatrix Model = ScaleMatrix * BillboardRotation * Translation;
		FMatrix MVP = Model * View * InRenderBus.GetCachedProjection();

		// String, UV값
		std::wstring Text = L"윢윩앏있띻\nUUID:" + std::to_wstring(Cmd.UUID);

		float TotalWidth = CellW * Text.size();
		float PenX = TotalWidth * 0.5f;  // 중앙 정렬
		float PenY = -CellH * 0.5f;

		for (TCHAR c : Text)
		{
			if (c == TEXT(' ')) { PenX += CellW; continue; }
			if (c == TEXT('\n')) { PenX = TotalWidth * 0.5f; PenY -= CellH; continue; }

			uint32 base = (uint32)Instances.size();

			FCharacterInfo CI = FontCache.GetCharacterAtlasData(c);

			// 인스턴스 저장 -> GPU에서 사용
			Instances.push_back({
				MVP, 
				PenX, PenY,
				CellW, CellH,
				CI.StartU, CI.StartV, 
				CI.USize, CI.VSize, 
				Cmd.FontColor });

			PenX -= CellW;
		}
	}
	if (Instances.empty()) return;

	// Instance Buffer 업데이트
	Resources.FontInstanceBuffer.FontUpdate(InDeviceContext, Instances);

	RenderFont(InDeviceContext, InRenderBus);
}

void FRenderer::RenderFont(ID3D11DeviceContext* InDeviceContext, FRenderBus& InRenderBus)
{
	InDeviceContext->IASetInputLayout(Resources.FontShader.GetInputLayout());
	InDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT strides[2] = { sizeof(FVertex), sizeof(FFontInstance) };
	UINT offsets[2] = { 0, 0 };
	ID3D11Buffer* Buffers[2] = {
		InRenderBus.GetFontCommands()[0].MeshBuffer->GetFVertexBuffer().GetBuffer(),
		Resources.FontInstanceBuffer.GetBuffer(),
	};
	InDeviceContext->IASetVertexBuffers(0, 2, Buffers, strides, offsets);

	InDeviceContext->VSSetShader(Resources.FontShader.GetVertexShader(), nullptr, 0);
	InDeviceContext->PSSetShader(Resources.FontShader.GetPixelShader(), nullptr, 0);

	InDeviceContext->PSSetShaderResources(0, 1, &Resources.FontAtlasSRV);

	InDeviceContext->DrawInstanced(6, Resources.FontInstanceBuffer.GetInstanceCount(), 0, 0);
}

void FRenderer::RenderOverlayPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus)
{
	for(const FRenderCommand& command : InRenderBus.GetOverlayCommands())
	{
		DrawCommand(InDeviceContext, command);
	}
}

void FRenderer::RenderOutlinePass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus)
{

	for(const FRenderCommand& command : InRenderBus.GetSelectionOutlineCommands())
	{

		DrawCommand(InDeviceContext, command);
	}
}

void FRenderer::DrawCommand(ID3D11DeviceContext *InDeviceContext, const FRenderCommand& InCommand)
{
	if (InCommand.MeshBuffer == nullptr || !InCommand.MeshBuffer->IsValid())
	{
		return;
	}

	if (InCommand.Type != ERenderCommandType::Overlay)
	{
		Resources.PerObjectConstantBuffer.Update(InDeviceContext, &InCommand.TransformConstants, sizeof(FTransformConstants));
		
		ID3D11Buffer* cb = Resources.PerObjectConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(0, 1, &cb);
		//InDeviceContext->PSSetConstantBuffers(0, 1, &cb);
	}

	if (InCommand.Type == ERenderCommandType::Gizmo)
	{
		Resources.GizmoPerObjectConstantBuffer.Update(InDeviceContext, &InCommand.GizmoConstants, sizeof(FGizmoConstants));

		ID3D11Buffer* cb = Resources.PerObjectConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(0, 1, &cb);

		cb = Resources.GizmoPerObjectConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(1, 1, &cb);
		InDeviceContext->PSSetConstantBuffers(1, 1, &cb);
	}
	else if (InCommand.Type == ERenderCommandType::Overlay)
	{
		Resources.OverlayConstantBuffer.Update(InDeviceContext, &InCommand.OverlayConstants, sizeof(FOverlayConstants));

		ID3D11Buffer* cb = Resources.OverlayConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(2, 1, &cb);
		InDeviceContext->PSSetConstantBuffers(2, 1, &cb);
	}
	else if(InCommand.Type == ERenderCommandType::SelectionOutline)
	{
		Resources.OutlineConstantBuffer.Update(InDeviceContext, &InCommand.OutlineConstants, sizeof(FOutlineConstants));

		ID3D11Buffer* cb = Resources.OutlineConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(4, 1, &cb);
		InDeviceContext->PSSetConstantBuffers(4, 1, &cb);

		cb = Resources.PerObjectConstantBuffer.GetBuffer();
		InDeviceContext->VSSetConstantBuffers(0, 1, &cb);
		//InDeviceContext->PSSetConstantBuffers(0, 1, &cb);
	}
	uint32 offset = 0;
	ID3D11Buffer* vertexBuffer = InCommand.MeshBuffer->GetFVertexBuffer().GetBuffer();
	if (vertexBuffer == nullptr)
	{
		return;
	}

	uint32 vertexCount = InCommand.MeshBuffer->GetFVertexBuffer().GetVertexCount();
	uint32 stride = InCommand.MeshBuffer->GetFVertexBuffer().GetStride();
	if (vertexCount == 0 || stride == 0)
	{
		return;
	}

	InDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	ID3D11Buffer* indexBuffer = InCommand.MeshBuffer->GetFIndexBuffer().GetBuffer();
	if (indexBuffer != nullptr)
	{
		uint32 indexCount = InCommand.MeshBuffer->GetFIndexBuffer().GetIndexCount();
		InDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		InDeviceContext->DrawIndexed(indexCount, 0, 0);
	}
	else
	{
		InDeviceContext->Draw(vertexCount, 0);
	}
}

void FRenderer::DrawUVCommand(ID3D11DeviceContext* InDeviceContext, const FRenderCommand& InCommand)
{

	InDeviceContext->IASetInputLayout(Resources.SubUVShader.GetInputLayout());

	if (InCommand.UVMeshBuffer ==  nullptr  || !InCommand.UVMeshBuffer->IsValid())
	{
		return;
	}
	//MVP 업데이트
	Resources.PerObjectConstantBuffer.Update(
		InDeviceContext,
		&InCommand.TransformConstants,
		sizeof(FTransformConstants)
	);
	ID3D11Buffer* cb = Resources.PerObjectConstantBuffer.GetBuffer();
	InDeviceContext->VSSetConstantBuffers(0, 1, &cb);

	//UV constant 업데이트 하기
	Resources.SubUVConstantBuffer.Update(
		InDeviceContext,
		&InCommand.SubUVConstants,
		sizeof(FSubUVConstants)
	);
	ID3D11Buffer* cbuv = Resources.SubUVConstantBuffer.GetBuffer();
	InDeviceContext->VSSetConstantBuffers(1, 1, &cbuv);


	uint32 offset = 0;

	//버텍스 버퍼 불러오기
	ID3D11Buffer* vertexBuffer = InCommand.UVMeshBuffer->GetFVertexBuffer().GetBuffer();
	if (vertexBuffer == nullptr)
	{
		return;
	}

	uint32 vertexCount = InCommand.UVMeshBuffer->GetFVertexBuffer().GetVertexCount();
	uint32 stride = InCommand.UVMeshBuffer->GetFVertexBuffer().GetStride();
	if (vertexCount == 0 || stride == 0)
	{
		return;
	}
	InDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	//인덱스 버퍼 불러오기
	ID3D11Buffer* indexBuffer = InCommand.UVMeshBuffer->GetFIndexBuffer().GetBuffer();
	if (indexBuffer != nullptr)
	{
		uint32 indexCount = InCommand.UVMeshBuffer->GetFIndexBuffer().GetIndexCount();
		InDeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		InDeviceContext->DrawIndexed(indexCount, 0, 0);
	}
	else
	{
		InDeviceContext->Draw(vertexCount, 0);
	}

}

//	Present the rendered frame to the screen. 반드시 Render 이후에 호출되어야 함.
void FRenderer::EndFrame()
{
	Device.EndFrame();
}
