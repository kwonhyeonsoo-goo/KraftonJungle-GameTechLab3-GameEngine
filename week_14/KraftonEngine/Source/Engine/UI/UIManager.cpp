#include "UI/UIManager.h"

#include "Core/Logging/Log.h"
#include "Input/InputSystem.h"
#include "Object/Object.h"
#include "Platform/Paths.h"
#include "Render/Command/DrawCommandList.h"
#include "Render/Device/D3DDevice.h"
#include "Render/RenderPass/RenderPassBase.h"
#include "Render/Resource/RenderResources.h"
#include "Render/Shader/ShaderManager.h"
#include "Render/Types/FrameContext.h"
#include "UI/UserWidget.h"
#include "WICTextureLoader.h"

#ifdef GetNextSibling
#undef GetNextSibling
#endif
#ifdef GetFirstChild
#undef GetFirstChild
#endif
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>

namespace
{
	struct FRmlVertexD3D11
	{
		float X, Y;
		float R, G, B, A;
		float U, V;
	};

	struct FRmlGeometryD3D11
	{
		ID3D11Buffer* VertexBuffer = nullptr;
		ID3D11Buffer* IndexBuffer = nullptr;
		UINT IndexCount = 0;
	};

	struct FRmlTextureD3D11
	{
		ID3D11ShaderResourceView* SRV = nullptr;
	};

	struct FRmlPerFrameCB
	{
		float ViewportWidth = 1.0f;
		float ViewportHeight = 1.0f;
		float TranslationX = 0.0f;
		float TranslationY = 0.0f;
		float Transform[16] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
	};

	constexpr const char* UIShaderPath = "Shaders/UI/RmlUi.hlsl";

	std::filesystem::path ToProjectPath(const FString& Path)
	{
		std::filesystem::path Result(FPaths::ToWide(Path));
		if (Result.is_relative())
		{
			Result = std::filesystem::path(FPaths::RootDir()) / Result;
		}
		return Result;
	}

	Rml::String ToRmlPath(const std::filesystem::path& Path)
	{
		return FPaths::ToUtf8(Path.generic_wstring());
	}

	bool IsMouseVirtualKey(int VK)
	{
		return VK == VK_LBUTTON || VK == VK_RBUTTON || VK == VK_MBUTTON ||
			VK == VK_XBUTTON1 || VK == VK_XBUTTON2;
	}

	Rml::Input::KeyIdentifier MapVirtualKeyToRmlKey(int VK)
	{
		using namespace Rml::Input;

		if (VK >= '0' && VK <= '9')
		{
			return static_cast<KeyIdentifier>(KI_0 + (VK - '0'));
		}
		if (VK >= 'A' && VK <= 'Z')
		{
			return static_cast<KeyIdentifier>(KI_A + (VK - 'A'));
		}
		if (VK >= VK_F1 && VK <= VK_F24)
		{
			return static_cast<KeyIdentifier>(KI_F1 + (VK - VK_F1));
		}
		if (VK >= VK_NUMPAD0 && VK <= VK_NUMPAD9)
		{
			return static_cast<KeyIdentifier>(KI_NUMPAD0 + (VK - VK_NUMPAD0));
		}

		switch (VK)
		{
		case VK_SPACE: return KI_SPACE;
		case VK_BACK: return KI_BACK;
		case VK_TAB: return KI_TAB;
		case VK_RETURN: return KI_RETURN;
		case VK_ESCAPE: return KI_ESCAPE;
		case VK_PRIOR: return KI_PRIOR;
		case VK_NEXT: return KI_NEXT;
		case VK_END: return KI_END;
		case VK_HOME: return KI_HOME;
		case VK_LEFT: return KI_LEFT;
		case VK_UP: return KI_UP;
		case VK_RIGHT: return KI_RIGHT;
		case VK_DOWN: return KI_DOWN;
		case VK_INSERT: return KI_INSERT;
		case VK_DELETE: return KI_DELETE;
		case VK_SHIFT: return KI_LSHIFT;
		case VK_LSHIFT: return KI_LSHIFT;
		case VK_RSHIFT: return KI_RSHIFT;
		case VK_CONTROL: return KI_LCONTROL;
		case VK_LCONTROL: return KI_LCONTROL;
		case VK_RCONTROL: return KI_RCONTROL;
		case VK_MENU: return KI_LMENU;
		case VK_LMENU: return KI_LMENU;
		case VK_RMENU: return KI_RMENU;
		case VK_OEM_1: return KI_OEM_1;
		case VK_OEM_PLUS: return KI_OEM_PLUS;
		case VK_OEM_COMMA: return KI_OEM_COMMA;
		case VK_OEM_MINUS: return KI_OEM_MINUS;
		case VK_OEM_PERIOD: return KI_OEM_PERIOD;
		case VK_OEM_2: return KI_OEM_2;
		case VK_OEM_3: return KI_OEM_3;
		case VK_OEM_4: return KI_OEM_4;
		case VK_OEM_5: return KI_OEM_5;
		case VK_OEM_6: return KI_OEM_6;
		case VK_OEM_7: return KI_OEM_7;
		case VK_MULTIPLY: return KI_MULTIPLY;
		case VK_ADD: return KI_ADD;
		case VK_SEPARATOR: return KI_SEPARATOR;
		case VK_SUBTRACT: return KI_SUBTRACT;
		case VK_DECIMAL: return KI_DECIMAL;
		case VK_DIVIDE: return KI_DIVIDE;
		case VK_PAUSE: return KI_PAUSE;
		case VK_CAPITAL: return KI_CAPITAL;
		case VK_NUMLOCK: return KI_NUMLOCK;
		case VK_SCROLL: return KI_SCROLL;
		case VK_LWIN: return KI_LWIN;
		case VK_RWIN: return KI_RWIN;
		case VK_APPS: return KI_APPS;
		default: return KI_UNKNOWN;
		}
	}

	int GetRmlKeyModifierState(const InputSystem& Input)
	{
		using namespace Rml::Input;

		int Modifiers = 0;
		if (Input.GetKey(VK_CONTROL) || Input.GetKey(VK_LCONTROL) || Input.GetKey(VK_RCONTROL))
		{
			Modifiers |= KM_CTRL;
		}
		if (Input.GetKey(VK_SHIFT) || Input.GetKey(VK_LSHIFT) || Input.GetKey(VK_RSHIFT))
		{
			Modifiers |= KM_SHIFT;
		}
		if (Input.GetKey(VK_MENU) || Input.GetKey(VK_LMENU) || Input.GetKey(VK_RMENU))
		{
			Modifiers |= KM_ALT;
		}
		if (Input.GetKey(VK_LWIN) || Input.GetKey(VK_RWIN))
		{
			Modifiers |= KM_META;
		}
		if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
		{
			Modifiers |= KM_CAPSLOCK;
		}
		if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0)
		{
			Modifiers |= KM_NUMLOCK;
		}
		if ((GetKeyState(VK_SCROLL) & 0x0001) != 0)
		{
			Modifiers |= KM_SCROLLLOCK;
		}
		return Modifiers;
	}

	bool IsElementOrAncestorFormControl(Rml::Element* Element)
	{
		for (Rml::Element* Current = Element; Current != nullptr; Current = Current->GetParentNode())
		{
			if (rmlui_dynamic_cast<Rml::ElementFormControl*>(Current) != nullptr)
			{
				return true;
			}
		}
		return false;
	}
}

double FRmlSystemInterface::GetElapsedTime()
{
	using namespace std::chrono;
	const auto Now = steady_clock::now();
	return duration<double>(Now - StartTime).count();
}

void FRmlSystemInterface::JoinPath(Rml::String& TranslatedPath, const Rml::String& DocumentPath, const Rml::String& Path)
{
	std::filesystem::path ResourcePath(FPaths::ToWide(Path));
	if (!ResourcePath.is_relative())
	{
		TranslatedPath = ToRmlPath(ResourcePath);
		return;
	}

	std::filesystem::path BasePath(FPaths::ToWide(DocumentPath));
	TranslatedPath = ToRmlPath(BasePath.parent_path() / ResourcePath);
}

bool FRmlSystemInterface::LogMessage(Rml::Log::Type Type, const Rml::String& Message)
{
	UE_LOG("[RmlUi] %s", Message.c_str());
	return Type != Rml::Log::LT_ASSERT;
}

// FRmlFileInterfaceWide — 모든 RmlUi 파일 열기를 wide API 로 우회. 한글 경로의 디렉토리
// 에서 실행될 때 기본 fopen 경로가 ANSI 로 해석되며 깨지는 것을 방지.
Rml::FileHandle FRmlFileInterfaceWide::Open(const Rml::String& Path)
{
	const std::wstring WidePath = FPaths::ToWide(Path);
	FILE* Fp = nullptr;
	if (_wfopen_s(&Fp, WidePath.c_str(), L"rb") != 0 || !Fp)
	{
		return Rml::FileHandle{};
	}
	return reinterpret_cast<Rml::FileHandle>(Fp);
}

void FRmlFileInterfaceWide::Close(Rml::FileHandle FileHandle)
{
	if (FileHandle)
	{
		fclose(reinterpret_cast<FILE*>(FileHandle));
	}
}

size_t FRmlFileInterfaceWide::Read(void* Buffer, size_t Size, Rml::FileHandle FileHandle)
{
	if (!FileHandle) return 0;
	return fread(Buffer, 1, Size, reinterpret_cast<FILE*>(FileHandle));
}

bool FRmlFileInterfaceWide::Seek(Rml::FileHandle FileHandle, long Offset, int Origin)
{
	if (!FileHandle) return false;
	return fseek(reinterpret_cast<FILE*>(FileHandle), Offset, Origin) == 0;
}

size_t FRmlFileInterfaceWide::Tell(Rml::FileHandle FileHandle)
{
	if (!FileHandle) return 0;
	const long Pos = ftell(reinterpret_cast<FILE*>(FileHandle));
	return Pos < 0 ? 0 : static_cast<size_t>(Pos);
}

FRmlRenderInterfaceD3D11::FRmlRenderInterfaceD3D11(ID3D11Device* InDevice)
	: Device(InDevice)
	, CurrentTransform(Rml::Matrix4f::Identity())
{
	CreateConstantBuffer();
}

FRmlRenderInterfaceD3D11::~FRmlRenderInterfaceD3D11()
{
	ReleaseWhiteTexture();
	if (ScissorRasterizerState)
	{
		ScissorRasterizerState->Release();
		ScissorRasterizerState = nullptr;
	}
	if (PerFrameCB)
	{
		PerFrameCB->Release();
		PerFrameCB = nullptr;
	}
}

void FRmlRenderInterfaceD3D11::BeginFrame(const FPassContext& InCtx)
{
	Ctx = &InCtx;

	ID3D11DeviceContext* DC = Ctx->Device.GetDeviceContext();
	if (!DC)
	{
		return;
	}

	D3D11_VIEWPORT Viewport = {};
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = Ctx->Frame.ViewportWidth;
	Viewport.Height = Ctx->Frame.ViewportHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	DC->RSSetViewports(1, &Viewport);
}

void FRmlRenderInterfaceD3D11::EndFrame()
{
	Ctx = nullptr;
}

void FRmlRenderInterfaceD3D11::SetTransform(const Rml::Matrix4f* Transform)
{
	CurrentTransform = Transform ? *Transform : Rml::Matrix4f::Identity();
}

Rml::CompiledGeometryHandle FRmlRenderInterfaceD3D11::CompileGeometry(Rml::Span<const Rml::Vertex> Vertices, Rml::Span<const int> Indices)
{
	if (!Device || Vertices.empty() || Indices.empty())
	{
		return 0;
	}

	TArray<FRmlVertexD3D11> ConvertedVertices;
	ConvertedVertices.reserve(Vertices.size());
	for (const Rml::Vertex& Vertex : Vertices)
	{
		ConvertedVertices.push_back({
			Vertex.position.x,
			Vertex.position.y,
			Vertex.colour.red / 255.0f,
			Vertex.colour.green / 255.0f,
			Vertex.colour.blue / 255.0f,
			Vertex.colour.alpha / 255.0f,
			Vertex.tex_coord.x,
			Vertex.tex_coord.y,
		});
	}

	TArray<uint32> ConvertedIndices;
	ConvertedIndices.reserve(Indices.size());
	for (int Index : Indices)
	{
		ConvertedIndices.push_back(static_cast<uint32>(Index));
	}

	auto* Geometry = new FRmlGeometryD3D11();
	Geometry->IndexCount = static_cast<UINT>(ConvertedIndices.size());

	D3D11_BUFFER_DESC VBDesc = {};
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.ByteWidth = static_cast<UINT>(sizeof(FRmlVertexD3D11) * ConvertedVertices.size());
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VBData = {};
	VBData.pSysMem = ConvertedVertices.data();
	if (FAILED(Device->CreateBuffer(&VBDesc, &VBData, &Geometry->VertexBuffer)))
	{
		delete Geometry;
		return 0;
	}
	Geometry->VertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen("RmlGeometryVertexBuffer")), "RmlGeometryVertexBuffer");

	D3D11_BUFFER_DESC IBDesc = {};
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.ByteWidth = static_cast<UINT>(sizeof(uint32) * ConvertedIndices.size());
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IBData = {};
	IBData.pSysMem = ConvertedIndices.data();
	if (FAILED(Device->CreateBuffer(&IBDesc, &IBData, &Geometry->IndexBuffer)))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry));
		return 0;
	}
	Geometry->IndexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen("RmlGeometryIndexBuffer")), "RmlGeometryIndexBuffer");

	return reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry);
}

void FRmlRenderInterfaceD3D11::RenderGeometry(Rml::CompiledGeometryHandle GeometryHandle, Rml::Vector2f Translation, Rml::TextureHandle Texture)
{
	if (!Ctx || !GeometryHandle)
	{
		return;
	}

	auto* Geometry = reinterpret_cast<FRmlGeometryD3D11*>(GeometryHandle);
	ID3D11DeviceContext* DC = Ctx->Device.GetDeviceContext();
	if (!DC || !Geometry->VertexBuffer || !Geometry->IndexBuffer)
	{
		return;
	}

	FShader* Shader = FShaderManager::Get().GetOrCreate(UIShaderPath);
	if (!Shader || !Shader->IsValid())
	{
		return;
	}

	Ctx->Resources.SetDepthStencilState(Ctx->Device, EDepthStencilState::NoDepth);
	Ctx->Resources.SetBlendState(Ctx->Device, EBlendState::AlphaBlend);
	Ctx->Resources.SetRasterizerState(Ctx->Device, ERasterizerState::SolidNoCull);

	DC->OMSetRenderTargets(1, &Ctx->Cache.RTV, Ctx->Cache.DSV);
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Shader->Bind(DC);

	FRmlPerFrameCB CBData;
	CBData.ViewportWidth = Ctx->Frame.ViewportWidth;
	CBData.ViewportHeight = Ctx->Frame.ViewportHeight;
	CBData.TranslationX = Translation.x;
	CBData.TranslationY = Translation.y;
	const float* TransformData = CurrentTransform.data();
	std::copy(TransformData, TransformData + 16, CBData.Transform);
	DC->UpdateSubresource(PerFrameCB, 0, nullptr, &CBData, 0, 0);
	DC->VSSetConstantBuffers(0, 1, &PerFrameCB);

	ID3D11ShaderResourceView* SRV = WhiteTextureSRV;
	if (Texture)
	{
		auto* TextureResource = reinterpret_cast<FRmlTextureD3D11*>(Texture);
		SRV = TextureResource ? TextureResource->SRV : nullptr;
	}
	DC->PSSetShaderResources(0, 1, &SRV);

	UINT Stride = sizeof(FRmlVertexD3D11);
	UINT Offset = 0;
	DC->IASetVertexBuffers(0, 1, &Geometry->VertexBuffer, &Stride, &Offset);
	DC->IASetIndexBuffer(Geometry->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DC->DrawIndexed(Geometry->IndexCount, 0, 0);
}

void FRmlRenderInterfaceD3D11::ReleaseGeometry(Rml::CompiledGeometryHandle GeometryHandle)
{
	auto* Geometry = reinterpret_cast<FRmlGeometryD3D11*>(GeometryHandle);
	if (!Geometry)
	{
		return;
	}

	if (Geometry->VertexBuffer)
	{
		Geometry->VertexBuffer->Release();
	}
	if (Geometry->IndexBuffer)
	{
		Geometry->IndexBuffer->Release();
	}
	delete Geometry;
}

Rml::TextureHandle FRmlRenderInterfaceD3D11::LoadTexture(Rml::Vector2i& TextureDimensions, const Rml::String& Source)
{
	TextureDimensions = { 0, 0 };

	if (!Device || Source.empty())
	{
		return 0;
	}

	const std::wstring WidePath = FPaths::ToWide(Source);

	ID3D11Resource* Resource = nullptr;
	ID3D11ShaderResourceView* SRV = nullptr;
	const HRESULT HR = DirectX::CreateWICTextureFromFileEx(
		Device,
		WidePath.c_str(),
		0,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
		DirectX::WIC_LOADER_IGNORE_SRGB,
		&Resource,
		&SRV);

	if (FAILED(HR) || !SRV)
	{
		if (Resource)
		{
			Resource->Release();
		}
		UE_LOG("[RmlUi] Failed to load texture: %s", Source.c_str());
		return 0;
	}

	if (Resource)
	{
		ID3D11Texture2D* Texture2D = nullptr;
		if (SUCCEEDED(Resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&Texture2D))) && Texture2D)
		{
			D3D11_TEXTURE2D_DESC Desc = {};
			Texture2D->GetDesc(&Desc);
			TextureDimensions = {
				static_cast<int>(Desc.Width),
				static_cast<int>(Desc.Height)
			};
			Texture2D->Release();
		}
		Resource->Release();
	}

	auto* TextureResource = new FRmlTextureD3D11();
	TextureResource->SRV = SRV;
	return reinterpret_cast<Rml::TextureHandle>(TextureResource);
}

Rml::TextureHandle FRmlRenderInterfaceD3D11::GenerateTexture(Rml::Span<const Rml::byte> Source, Rml::Vector2i SourceDimensions)
{
	if (!Device || Source.empty() || SourceDimensions.x <= 0 || SourceDimensions.y <= 0)
	{
		return 0;
	}

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = static_cast<UINT>(SourceDimensions.x);
	TextureDesc.Height = static_cast<UINT>(SourceDimensions.y);
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA InitialData = {};
	InitialData.pSysMem = Source.data();
	InitialData.SysMemPitch = static_cast<UINT>(SourceDimensions.x * 4);

	ID3D11Texture2D* Texture = nullptr;
	if (FAILED(Device->CreateTexture2D(&TextureDesc, &InitialData, &Texture)))
	{
		return 0;
	}

	ID3D11ShaderResourceView* SRV = nullptr;
	HRESULT HR = Device->CreateShaderResourceView(Texture, nullptr, &SRV);
	Texture->Release();
	if (FAILED(HR))
	{
		return 0;
	}

	auto* TextureResource = new FRmlTextureD3D11();
	TextureResource->SRV = SRV;
	return reinterpret_cast<Rml::TextureHandle>(TextureResource);
}

void FRmlRenderInterfaceD3D11::ReleaseTexture(Rml::TextureHandle Texture)
{
	auto* TextureResource = reinterpret_cast<FRmlTextureD3D11*>(Texture);
	if (!TextureResource)
	{
		return;
	}
	if (TextureResource->SRV)
	{
		TextureResource->SRV->Release();
	}
	delete TextureResource;
}

void FRmlRenderInterfaceD3D11::EnableScissorRegion(bool Enable)
{
	if (!Ctx)
	{
		return;
	}

	ID3D11DeviceContext* DC = Ctx->Device.GetDeviceContext();
	if (Enable && ScissorRasterizerState)
	{
		DC->RSSetState(ScissorRasterizerState);
	}
	else
	{
		Ctx->Resources.SetRasterizerState(Ctx->Device, ERasterizerState::SolidNoCull);
	}

	if (!Enable)
	{
		DC->RSSetScissorRects(0, nullptr);
	}
}

void FRmlRenderInterfaceD3D11::SetScissorRegion(Rml::Rectanglei Region)
{
	if (!Ctx)
	{
		return;
	}

	D3D11_RECT Rect = {};
	Rect.left = Region.Left();
	Rect.top = Region.Top();
	Rect.right = Region.Right();
	Rect.bottom = Region.Bottom();
	Ctx->Device.GetDeviceContext()->RSSetScissorRects(1, &Rect);
}

void FRmlRenderInterfaceD3D11::CreateConstantBuffer()
{
	if (!Device)
	{
		return;
	}

	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.ByteWidth = sizeof(FRmlPerFrameCB);
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Device->CreateBuffer(&Desc, nullptr, &PerFrameCB);
	PerFrameCB->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen("RmlPerFrameCB")), "RmlPerFrameCB");

	CreateWhiteTexture();

	D3D11_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.CullMode = D3D11_CULL_NONE;
	RasterDesc.ScissorEnable = TRUE;
	Device->CreateRasterizerState(&RasterDesc, &ScissorRasterizerState);
}

void FRmlRenderInterfaceD3D11::CreateWhiteTexture()
{
	const uint32 WhitePixel = 0xffffffff;

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = 1;
	TextureDesc.Height = 1;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA InitialData = {};
	InitialData.pSysMem = &WhitePixel;
	InitialData.SysMemPitch = sizeof(uint32);

	ID3D11Texture2D* Texture = nullptr;
	if (SUCCEEDED(Device->CreateTexture2D(&TextureDesc, &InitialData, &Texture)))
	{
		Device->CreateShaderResourceView(Texture, nullptr, &WhiteTextureSRV);
		Texture->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen("RmlWhiteTexture")), "RmlWhiteTexture");
		WhiteTextureSRV->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen("RmlWhiteTextureSRV")), "RmlWhiteTextureSRV");
		Texture->Release();
	}
}

void FRmlRenderInterfaceD3D11::ReleaseWhiteTexture()
{
	if (WhiteTextureSRV)
	{
		WhiteTextureSRV->Release();
		WhiteTextureSRV = nullptr;
	}
}

void UUIManager::Initialize(ID3D11Device* InDevice)
{
	CachedDevice = InDevice;

	if (bRmlInitialized || !CachedDevice)
	{
		return;
	}

	SystemInterface = new FRmlSystemInterface();
	FileInterface = new FRmlFileInterfaceWide();
	RenderInterface = new FRmlRenderInterfaceD3D11(CachedDevice);

	Rml::SetSystemInterface(SystemInterface);
	// Initialise 전에 등록해야 RmlUi 가 default file 인터페이스 대신 우리 wide 버전을 쓴다.
	Rml::SetFileInterface(FileInterface);
	Rml::SetRenderInterface(RenderInterface);
	bRmlInitialized = Rml::Initialise();
	if (!bRmlInitialized)
	{
		UE_LOG("[RmlUi] Initialise failed.");
		return;
	}

	RmlContext = Rml::CreateContext("GameViewport", Rml::Vector2i(1, 1));
	if (!RmlContext)
	{
		UE_LOG("[RmlUi] Failed to create GameViewport context.");
	}

	const std::filesystem::path FontPath = ToProjectPath("Content/Font/Maplestory Bold.ttf");
	if (!Rml::LoadFontFace(ToRmlPath(FontPath), "Maplestory", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Bold))
	{
		UE_LOG("[RmlUi] Failed to load font: Content/Font/Maplestory Bold.ttf");
	}
}

void UUIManager::Shutdown()
{
	DestroyAllWidgets();

	if (RmlContext)
	{
		Rml::RemoveContext("GameViewport");
		RmlContext = nullptr;
	}

	if (bRmlInitialized)
	{
		Rml::Shutdown();
		bRmlInitialized = false;
	}

	delete RenderInterface;
	RenderInterface = nullptr;
	delete FileInterface;
	FileInterface = nullptr;
	delete SystemInterface;
	SystemInterface = nullptr;
	CachedDevice = nullptr;
}

UUserWidget* UUIManager::CreateWidget(APlayerController* OwningPlayer, const FString& DocumentPath)
{
	CompactInvalidWidgets();
	UUserWidget* Widget = UObjectManager::Get().CreateObject<UUserWidget>();
	Widget->Initialize(OwningPlayer, DocumentPath);
	CreatedWidgets.push_back(Widget);
	return Widget;
}

void UUIManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(CreatedWidgets, "UUIManager.CreatedWidgets");
	Collector.AddReferencedObjects(ViewportWidgets, "UUIManager.ViewportWidgets");
	Collector.AddReferencedObjects(PendingRemoveWidgets, "UUIManager.PendingRemoveWidgets");
}

void UUIManager::CompactInvalidWidgets()
{
	auto RemoveInvalid = [](TArray<UUserWidget*>& Widgets)
	{
		Widgets.erase(
			std::remove_if(
				Widgets.begin(),
				Widgets.end(),
				[](UUserWidget* Widget)
				{
					return !IsValid(Widget);
				}),
			Widgets.end());
	};

	RemoveInvalid(ViewportWidgets);
	RemoveInvalid(CreatedWidgets);
	RemoveInvalid(PendingRemoveWidgets);
}

void UUIManager::BeginInputFrame()
{
	bInputProcessedThisFrame = false;
	LastFrameInputCaptureState = {};

	InputSystem& Input = InputSystem::Get();
	Input.SetGuiMouseCapture(false);
	Input.SetGuiKeyboardCapture(false);
	Input.SetGuiTextInputCapture(false);
	Input.RefreshSnapshot();
}

bool UUIManager::PumpViewportInput(uint32 ViewportWidth, uint32 ViewportHeight,
	int32 ViewportClientX, int32 ViewportClientY,
	int32 ViewportClientWidth, int32 ViewportClientHeight)
{
	if (!RmlContext || bInputProcessedThisFrame || ViewportWidgets.empty())
	{
		return false;
	}

	ViewportWidth = (std::max)(ViewportWidth, static_cast<uint32>(1));
	ViewportHeight = (std::max)(ViewportHeight, static_cast<uint32>(1));
	ViewportClientWidth = (std::max)(ViewportClientWidth, static_cast<int32>(1));
	ViewportClientHeight = (std::max)(ViewportClientHeight, static_cast<int32>(1));

	RmlContext->SetDimensions({
		static_cast<int>(ViewportWidth),
		static_cast<int>(ViewportHeight)
	});

	InputSystem& Input = InputSystem::Get();
	const POINT ClientMousePos = Input.GetMouseClientPos();
	const bool bInsideViewport =
		ClientMousePos.x >= ViewportClientX &&
		ClientMousePos.y >= ViewportClientY &&
		ClientMousePos.x < ViewportClientX + ViewportClientWidth &&
		ClientMousePos.y < ViewportClientY + ViewportClientHeight;

	const FUIInputCaptureState CaptureState = GetViewportInputCaptureState();
	const bool bHasFocusedElement = RmlContext->GetFocusElement() != nullptr;
	if (!bInsideViewport &&
		!RmlContext->IsMouseInteracting() &&
		!bHasFocusedElement &&
		!CaptureState.bWantsKeyboard &&
		!CaptureState.bWantsTextInput)
	{
		bInputProcessedThisFrame = true;
		return false;
	}

	const int32 LocalMouseX = static_cast<int32>(
		static_cast<float>(ClientMousePos.x - ViewportClientX) * static_cast<float>(ViewportWidth) /
		static_cast<float>(ViewportClientWidth));
	const int32 LocalMouseY = static_cast<int32>(
		static_cast<float>(ClientMousePos.y - ViewportClientY) * static_cast<float>(ViewportHeight) /
		static_cast<float>(ViewportClientHeight));

	ProcessInputAtPosition(LocalMouseX, LocalMouseY, bInsideViewport);
	bInputProcessedThisFrame = true;
	return LastFrameInputCaptureState.bConsumedMouseThisFrame ||
		LastFrameInputCaptureState.bConsumedKeyboardThisFrame ||
		LastFrameInputCaptureState.bConsumedTextInputThisFrame;
}

FUIInputCaptureState UUIManager::GetViewportInputCaptureState() const
{
	FUIInputCaptureState State;
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (!IsValid(Widget))
		{
			continue;
		}

		State.bWantsMouse = State.bWantsMouse || Widget->WantsMouse();
		State.bWantsKeyboard = State.bWantsKeyboard || Widget->WantsKeyboard();
		State.bWantsTextInput = State.bWantsTextInput || Widget->WantsTextInput();
		State.bBlocksGameInput = State.bBlocksGameInput || Widget->BlocksGameInput();
		State.bBlocksGameKeyboard = State.bBlocksGameKeyboard || Widget->BlocksGameKeyboard();
		State.bBlocksGameMouseLook = State.bBlocksGameMouseLook || Widget->BlocksGameMouseLook();
	}

	if (RmlContext && IsElementOrAncestorFormControl(RmlContext->GetFocusElement()))
	{
		State.bWantsKeyboard = true;
		State.bWantsTextInput = true;
		State.bBlocksGameKeyboard = true;
	}

	State.bConsumedMouseThisFrame = LastFrameInputCaptureState.bConsumedMouseThisFrame;
	State.bConsumedKeyboardThisFrame = LastFrameInputCaptureState.bConsumedKeyboardThisFrame;
	State.bConsumedTextInputThisFrame = LastFrameInputCaptureState.bConsumedTextInputThisFrame;
	return State;
}

bool UUIManager::AnyViewportWidgetWantsMouse() const
{
	return GetViewportInputCaptureState().bWantsMouse;
}

FString UUIManager::GetElementText(const FString& ElementId) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->GetText(ElementId);
		}
	}
	return {};
}

bool UUIManager::SetElementText(const FString& ElementId, const FString& Text)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			Widget->SetText(ElementId, Text);
			return true;
		}
	}
	return false;
}

FString UUIManager::GetElementValue(const FString& ElementId) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->GetElementValue(ElementId);
		}
	}
	return {};
}

bool UUIManager::SetElementValue(const FString& ElementId, const FString& Value)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementValue(ElementId, Value))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::SetElementClass(const FString& ElementId, const FString& ClassName, bool bEnabled)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementClass(ElementId, ClassName, bEnabled))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::HasElementClass(const FString& ElementId, const FString& ClassName) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->HasElementClass(ElementId, ClassName);
		}
	}
	return false;
}

FString UUIManager::GetElementClassNames(const FString& ElementId) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->GetElementClassNames(ElementId);
		}
	}
	return {};
}

bool UUIManager::SetElementClassNames(const FString& ElementId, const FString& ClassNames)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementClassNames(ElementId, ClassNames))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::HasElementAttribute(const FString& ElementId, const FString& AttributeName) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->HasElementAttribute(ElementId, AttributeName);
		}
	}
	return false;
}

FString UUIManager::GetElementAttribute(const FString& ElementId, const FString& AttributeName) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->GetElementAttribute(ElementId, AttributeName);
		}
	}
	return {};
}

bool UUIManager::SetElementAttribute(const FString& ElementId, const FString& AttributeName, const FString& Value)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementAttribute(ElementId, AttributeName, Value))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::RemoveElementAttribute(const FString& ElementId, const FString& AttributeName)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->RemoveElementAttribute(ElementId, AttributeName))
		{
			return true;
		}
	}
	return false;
}

FString UUIManager::GetElementStyle(const FString& ElementId, const FString& StyleName) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->GetElementStyle(ElementId, StyleName);
		}
	}
	return {};
}

bool UUIManager::SetElementStyle(const FString& ElementId, const FString& StyleName, const FString& Value)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementStyle(ElementId, StyleName, Value))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::RemoveElementStyle(const FString& ElementId, const FString& StyleName)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->RemoveElementStyle(ElementId, StyleName))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::FocusElement(const FString& ElementId, bool bFocusVisible)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->FocusElement(ElementId, bFocusVisible))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::BlurElement(const FString& ElementId)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->BlurElement(ElementId))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::IsElementFocused(const FString& ElementId) const
{
	for (const UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->HasElement(ElementId))
		{
			return Widget->IsElementFocused(ElementId);
		}
	}
	return false;
}

bool UUIManager::ClickElement(const FString& ElementId)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->ClickElement(ElementId))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::SetElementVisible(const FString& ElementId, bool bVisible)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementVisible(ElementId, bVisible))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::SetElementEnabled(const FString& ElementId, bool bEnabled)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetElementEnabled(ElementId, bEnabled))
		{
			return true;
		}
	}
	return false;
}

bool UUIManager::SetActionEvent(const FString& ElementId, const FString& EventName)
{
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsValid(Widget) && Widget->SetActionEvent(ElementId, EventName))
		{
			return true;
		}
	}
	return false;
}

TArray<FString> UUIManager::PollActionEvents()
{
	TArray<FString> Events;
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (!IsValid(Widget))
		{
			continue;
		}

		for (const FString& EventName : Widget->PollActionEvents())
		{
			Events.push_back(EventName);
		}
	}
	return Events;
}

void UUIManager::AddToViewport(UUserWidget* Widget, int32 /*ZOrder*/)
{
	CompactInvalidWidgets();
	if (!IsValid(Widget))
	{
		return;
	}

	InputSystem::Get().ConsumeTextInput();

	if (!LoadDocument(Widget))
	{
		return;
	}

	auto It = std::find(ViewportWidgets.begin(), ViewportWidgets.end(), Widget);
	if (It == ViewportWidgets.end())
	{
		ViewportWidgets.push_back(Widget);
	}

	std::sort(ViewportWidgets.begin(), ViewportWidgets.end(),
		[](const UUserWidget* A, const UUserWidget* B)
		{
			return A->GetZOrder() < B->GetZOrder();
		});
}

void UUIManager::RemoveFromViewport(UUserWidget* Widget)
{
	CompactInvalidWidgets();
	if (!IsAliveObject(Widget))
	{
		return;
	}

	if (bDispatchingRmlEvents)
	{
		if (std::find(PendingRemoveWidgets.begin(), PendingRemoveWidgets.end(), Widget) == PendingRemoveWidgets.end())
		{
			PendingRemoveWidgets.push_back(Widget);
			Widget->MarkRemovedFromViewport();
		}
		return;
	}

	RemoveFromViewportImmediate(Widget);
}

void UUIManager::RemoveFromViewportImmediate(UUserWidget* Widget)
{
	ViewportWidgets.erase(std::remove(ViewportWidgets.begin(), ViewportWidgets.end(), Widget), ViewportWidgets.end());
	CloseDocument(Widget);
	if (IsAliveObject(Widget))
	{
		Widget->MarkRemovedFromViewport();
	}
}

void UUIManager::ClearViewport()
{
	// 위젯을 viewport 에서만 떼고 UObject 자체는 유지. UUIManager 는 widgets 의 owner —
	// 같은 Lua VM 안의 widgets[] 테이블이 그대로 살아있고, PIE 재시작 / TransitionToScene
	// 후 UIManager.Init re-entry 경로가 동일 위젯을 재사용한다 (위젯 destroy 시 Lua 측
	// 캐시가 dangling 이 되어 RemoveFromParent → CloseDocument 가 stale Rml::Document 를
	// 참조해 크래시). UObject 까지 파괴하는 건 Shutdown 만의 책임.
	PendingRemoveWidgets.clear();

	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (IsAliveObject(Widget))
		{
			CloseDocument(Widget);
			Widget->MarkRemovedFromViewport();
		}
	}
	ViewportWidgets.clear();

	if (RmlContext)
	{
		RmlContext->Update();
	}
}

void UUIManager::DestroyAllWidgets()
{
	ClearViewport();
	CompactInvalidWidgets();

	for (UUserWidget* Widget : CreatedWidgets)
	{
		if (IsAliveObject(Widget))
		{
			UObjectManager::Get().DestroyObject(Widget);
		}
	}
	CreatedWidgets.clear();
}

bool UUIManager::LoadDocument(UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return false;
	}
	if (Widget->IsDocumentLoaded())
	{
		return true;
	}
	if (!RmlContext)
	{
		return false;
	}

	const std::filesystem::path Path = ToProjectPath(Widget->GetDocumentPath());
	if (!std::filesystem::exists(Path))
	{
		UE_LOG("[RmlUi] Document not found: %s", Widget->GetDocumentPath().c_str());
		return false;
	}

	Rml::ElementDocument* Document = RmlContext->LoadDocument(ToRmlPath(Path));
	if (!Document)
	{
		UE_LOG("[RmlUi] Failed to load document: %s", Widget->GetDocumentPath().c_str());
		return false;
	}

	Document->Show();
	Widget->MarkDocumentLoaded(Document);
	Widget->RegisterEventListeners();
	return true;
}

void UUIManager::CloseDocument(UUserWidget* Widget)
{
	if (!IsAliveObject(Widget) || !Widget->GetDocument())
	{
		return;
	}

	Widget->ClearEventListeners();
	Widget->GetDocument()->Close();
	Widget->ClearDocument();
}

void UUIManager::Render(const FPassContext& Ctx)
{
	CompactInvalidWidgets();
	if (!RmlContext || !RenderInterface || ViewportWidgets.empty() || Ctx.Frame.ViewportWidth <= 0.0f || Ctx.Frame.ViewportHeight <= 0.0f)
	{
		return;
	}

	RmlContext->SetDimensions({
		static_cast<int>(Ctx.Frame.ViewportWidth),
		static_cast<int>(Ctx.Frame.ViewportHeight)
	});

	ProcessInput(Ctx.Frame);
	FlushDeferredViewportRemovals();
	if (ViewportWidgets.empty())
	{
		return;
	}

	RmlContext->Update();
	RenderInterface->BeginFrame(Ctx);
	RmlContext->Render();
	RenderInterface->EndFrame();
}

void UUIManager::ProcessInput(const FFrameContext& Frame)
{
	if (!RmlContext || bInputProcessedThisFrame)
	{
		return;
	}

	InputSystem& Input = InputSystem::Get();
	bool bMouseInsideViewport = false;
	int32 MouseX = 0;
	int32 MouseY = 0;
	if (Frame.CursorViewportX != UINT32_MAX && Frame.CursorViewportY != UINT32_MAX)
	{
		MouseX = static_cast<int32>(Frame.CursorViewportX);
		MouseY = static_cast<int32>(Frame.CursorViewportY);
		bMouseInsideViewport = true;
	}
	else
	{
		const POINT MousePos = Input.GetMouseClientPos();
		MouseX = MousePos.x;
		MouseY = MousePos.y;
	}

	ProcessInputAtPosition(MouseX, MouseY, bMouseInsideViewport);
	bInputProcessedThisFrame = true;
}

void UUIManager::ProcessInputAtPosition(int32 MouseX, int32 MouseY, bool bMouseInsideViewport)
{
	if (!RmlContext)
	{
		return;
	}

	LastFrameInputCaptureState = {};

	InputSystem& Input = InputSystem::Get();
	const int KeyModifierState = GetRmlKeyModifierState(Input);
	const FUIInputCaptureState CaptureState = GetViewportInputCaptureState();
	const bool bTextInputFocused = IsElementOrAncestorFormControl(RmlContext->GetFocusElement());
	const bool bShouldForwardMouse =
		CaptureState.bWantsMouse ||
		CaptureState.bBlocksGameInput ||
		CaptureState.bBlocksGameMouseLook;
	const bool bShouldForwardKeyboard = CaptureState.bWantsKeyboard || CaptureState.bWantsTextInput || bTextInputFocused;
	const bool bShouldForwardText = CaptureState.bWantsTextInput || bTextInputFocused;

	bDispatchingRmlEvents = true;
	if (bShouldForwardMouse)
	{
		if (bMouseInsideViewport)
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseMove(MouseX, MouseY, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame ||
				(!bMouseEventNotConsumed && RmlContext->IsMouseInteracting());
		}
		else
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseLeave();
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}

		if (Input.GetKeyDown(VK_LBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonDown(0, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		if (Input.GetKeyUp(VK_LBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonUp(0, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		if (Input.GetKeyDown(VK_RBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonDown(1, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		if (Input.GetKeyUp(VK_RBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonUp(1, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		if (Input.GetKeyDown(VK_MBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonDown(2, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		if (Input.GetKeyUp(VK_MBUTTON))
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseButtonUp(2, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
		const float WheelDelta = Input.GetScrollNotches();
		if (WheelDelta != 0.0f)
		{
			const bool bMouseEventNotConsumed = RmlContext->ProcessMouseWheel(WheelDelta, KeyModifierState);
			LastFrameInputCaptureState.bConsumedMouseThisFrame =
				LastFrameInputCaptureState.bConsumedMouseThisFrame || !bMouseEventNotConsumed;
		}
	}

	if (bShouldForwardKeyboard)
	{
		for (int VK = 0; VK < 256; ++VK)
		{
			if (IsMouseVirtualKey(VK))
			{
				continue;
			}

			const Rml::Input::KeyIdentifier Key = MapVirtualKeyToRmlKey(VK);
			if (Key == Rml::Input::KI_UNKNOWN)
			{
				continue;
			}

			if (Input.GetKeyDown(VK))
			{
				const bool bKeyEventNotConsumed = RmlContext->ProcessKeyDown(Key, KeyModifierState);
				LastFrameInputCaptureState.bConsumedKeyboardThisFrame =
					LastFrameInputCaptureState.bConsumedKeyboardThisFrame || !bKeyEventNotConsumed;
			}
			if (Input.GetKeyUp(VK))
			{
				const bool bKeyEventNotConsumed = RmlContext->ProcessKeyUp(Key, KeyModifierState);
				LastFrameInputCaptureState.bConsumedKeyboardThisFrame =
					LastFrameInputCaptureState.bConsumedKeyboardThisFrame || !bKeyEventNotConsumed;
			}
		}
	}

	TArray<uint32_t> TextInput = Input.ConsumeTextInput();
	if (bShouldForwardText)
	{
		for (uint32_t Codepoint : TextInput)
		{
			const bool bTextEventNotConsumed = RmlContext->ProcessTextInput(static_cast<Rml::Character>(Codepoint));
			LastFrameInputCaptureState.bConsumedTextInputThisFrame =
				LastFrameInputCaptureState.bConsumedTextInputThisFrame || !bTextEventNotConsumed;
			LastFrameInputCaptureState.bConsumedKeyboardThisFrame =
				LastFrameInputCaptureState.bConsumedKeyboardThisFrame || !bTextEventNotConsumed;
		}
	}
	bDispatchingRmlEvents = false;

	if (LastFrameInputCaptureState.bConsumedMouseThisFrame)
	{
		Input.SetGuiMouseCapture(true);
	}
	if (LastFrameInputCaptureState.bConsumedKeyboardThisFrame)
	{
		Input.SetGuiKeyboardCapture(true);
	}
	if (LastFrameInputCaptureState.bConsumedTextInputThisFrame || bTextInputFocused)
	{
		Input.SetGuiKeyboardCapture(true);
		Input.SetGuiTextInputCapture(true);
	}
	Input.RefreshSnapshot();
}

void UUIManager::FlushDeferredViewportRemovals()
{
	if (PendingRemoveWidgets.empty())
	{
		return;
	}

	TArray<UUserWidget*> WidgetsToRemove = PendingRemoveWidgets;
	PendingRemoveWidgets.clear();

	for (UUserWidget* Widget : WidgetsToRemove)
	{
		RemoveFromViewportImmediate(Widget);
	}
}
