#include "Hud/HudRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include <SpriteBatch.h>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Picking/PickingSystem.h"
#include "Scene/Scene.h"
#include "Stats/StatsSystem.h"

namespace
{
	bool CreateWhiteTexture(ID3D11Device* InDevice, TComPtr<ID3D11ShaderResourceView>& OutTextureView)
	{
		const uint32 WhitePixel = 0xFFFFFFFFu;

		const D3D11_TEXTURE2D_DESC TextureDesc =
		{
			1,
			1,
			1,
			1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			{ 1, 0 },
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			0
		};

		const D3D11_SUBRESOURCE_DATA InitialData =
		{
			&WhitePixel,
			sizeof(uint32),
			0
		};

		TComPtr<ID3D11Texture2D> Texture;
		if (FAILED(InDevice->CreateTexture2D(&TextureDesc, &InitialData, Texture.GetAddressOf())))
		{
			return false;
		}

		return SUCCEEDED(InDevice->CreateShaderResourceView(Texture.Get(), nullptr, OutTextureView.GetAddressOf()));
	}

	struct FBitmapGlyph
	{
		char Character = ' ';
		std::array<uint8_t, 7> Rows = {};
	};

	const FBitmapGlyph* FindBitmapGlyph(char InCharacter)
	{
		static const std::array<FBitmapGlyph, 40> Glyphs =
		{{
			{ ' ', { 0x00,0x00,0x00,0x00,0x00,0x00,0x00 } },
			{ '-', { 0x00,0x00,0x00,0x1F,0x00,0x00,0x00 } },
			{ '.', { 0x00,0x00,0x00,0x00,0x00,0x06,0x06 } },
			{ ':', { 0x00,0x04,0x04,0x00,0x04,0x04,0x00 } },
			{ '0', { 0x0E,0x11,0x13,0x15,0x19,0x11,0x0E } },
			{ '1', { 0x04,0x0C,0x04,0x04,0x04,0x04,0x0E } },
			{ '2', { 0x0E,0x11,0x01,0x02,0x04,0x08,0x1F } },
			{ '3', { 0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E } },
			{ '4', { 0x02,0x06,0x0A,0x12,0x1F,0x02,0x02 } },
			{ '5', { 0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E } },
			{ '6', { 0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E } },
			{ '7', { 0x1F,0x01,0x02,0x04,0x08,0x08,0x08 } },
			{ '8', { 0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E } },
			{ '9', { 0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E } },
			{ 'A', { 0x0E,0x11,0x11,0x1F,0x11,0x11,0x11 } },
			{ 'B', { 0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E } },
			{ 'C', { 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E } },
			{ 'D', { 0x1C,0x12,0x11,0x11,0x11,0x12,0x1C } },
			{ 'E', { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F } },
			{ 'F', { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x10 } },
			{ 'G', { 0x0E,0x11,0x10,0x17,0x11,0x11,0x0F } },
			{ 'H', { 0x11,0x11,0x11,0x1F,0x11,0x11,0x11 } },
			{ 'I', { 0x1F,0x04,0x04,0x04,0x04,0x04,0x1F } },
			{ 'J', { 0x01,0x01,0x01,0x01,0x11,0x11,0x0E } },
			{ 'K', { 0x11,0x12,0x14,0x18,0x14,0x12,0x11 } },
			{ 'L', { 0x10,0x10,0x10,0x10,0x10,0x10,0x1F } },
			{ 'M', { 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 } },
			{ 'N', { 0x11,0x19,0x15,0x13,0x11,0x11,0x11 } },
			{ 'O', { 0x0E,0x11,0x11,0x11,0x11,0x11,0x0E } },
			{ 'P', { 0x1E,0x11,0x11,0x1E,0x10,0x10,0x10 } },
			{ 'Q', { 0x0E,0x11,0x11,0x11,0x15,0x12,0x0D } },
			{ 'R', { 0x1E,0x11,0x11,0x1E,0x14,0x12,0x11 } },
			{ 'S', { 0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E } },
			{ 'T', { 0x1F,0x04,0x04,0x04,0x04,0x04,0x04 } },
			{ 'U', { 0x11,0x11,0x11,0x11,0x11,0x11,0x0E } },
			{ 'V', { 0x11,0x11,0x11,0x11,0x11,0x0A,0x04 } },
			{ 'W', { 0x11,0x11,0x11,0x15,0x15,0x15,0x0A } },
			{ 'X', { 0x11,0x11,0x0A,0x04,0x0A,0x11,0x11 } },
			{ 'Y', { 0x11,0x11,0x0A,0x04,0x04,0x04,0x04 } },
			{ 'Z', { 0x1F,0x01,0x02,0x04,0x08,0x10,0x1F } },
		}};

		const char Character = static_cast<char>(std::toupper(static_cast<unsigned char>(InCharacter)));
		for (const FBitmapGlyph& Glyph : Glyphs)
		{
			if (Glyph.Character == Character)
			{
				return &Glyph;
			}
		}

		return &Glyphs[0];
	}

	void DrawBitmapText(
		DirectX::SpriteBatch* InSpriteBatch,
		ID3D11ShaderResourceView* InTexture,
		const std::string& InText,
		float InStartX,
		float InStartY,
		float InPixelScale,
		float InRed,
		float InGreen,
		float InBlue,
		float InAlpha)
	{
		if (InSpriteBatch == nullptr || InTexture == nullptr)
		{
			return;
		}

		const float GlyphAdvance = 6.0f * InPixelScale;
		const float LineAdvance = 9.0f * InPixelScale;
		float CursorX = InStartX;
		float CursorY = InStartY;

		for (char Character : InText)
		{
			if (Character == '\n')
			{
				CursorX = InStartX;
				CursorY += LineAdvance;
				continue;
			}

			const FBitmapGlyph* Glyph = FindBitmapGlyph(Character);
			for (size_t RowIndex = 0; RowIndex < Glyph->Rows.size(); ++RowIndex)
			{
				const uint8_t RowBits = Glyph->Rows[RowIndex];
				for (int32 ColumnIndex = 0; ColumnIndex < 5; ++ColumnIndex)
				{
					if ((RowBits & (1u << (4 - ColumnIndex))) == 0)
					{
						continue;
					}

					const RECT PixelRect =
					{
						static_cast<LONG>(CursorX + static_cast<float>(ColumnIndex) * InPixelScale),
						static_cast<LONG>(CursorY + static_cast<float>(RowIndex) * InPixelScale),
						static_cast<LONG>(CursorX + static_cast<float>(ColumnIndex + 1) * InPixelScale),
						static_cast<LONG>(CursorY + static_cast<float>(RowIndex + 1) * InPixelScale)
					};

					InSpriteBatch->Draw(InTexture, PixelRect, DirectX::XMVectorSet(InRed, InGreen, InBlue, InAlpha));
				}
			}

			CursorX += GlyphAdvance;
		}
	}

	struct FAxisGizmoVisual
	{
		char Label = '\0';
		FVector CameraSpaceDirection = FVector::ZeroVector;
		float Red = 1.0f;
		float Green = 1.0f;
		float Blue = 1.0f;
		float Alpha = 1.0f;
	};

	void DrawScreenLine(
		DirectX::SpriteBatch* InSpriteBatch,
		ID3D11ShaderResourceView* InTexture,
		const DirectX::XMFLOAT2& InStart,
		const DirectX::XMFLOAT2& InEnd,
		float InRed,
		float InGreen,
		float InBlue,
		float InAlpha,
		float InThickness)
	{
		if (InSpriteBatch == nullptr || InTexture == nullptr)
		{
			return;
		}

		const float DeltaX = InEnd.x - InStart.x;
		const float DeltaY = InEnd.y - InStart.y;
		const float Length = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
		if (Length <= 0.01f)
		{
			return;
		}

		InSpriteBatch->Draw(
			InTexture,
			InStart,
			nullptr,
			DirectX::XMVectorSet(InRed, InGreen, InBlue, InAlpha),
			std::atan2(DeltaY, DeltaX),
			DirectX::XMFLOAT2(0.0f, 0.5f),
			DirectX::XMFLOAT2(Length, InThickness));
	}

	void DrawAxisGizmo(const FCamera& InCamera, const FD3D11RHI& InRHI, DirectX::SpriteBatch& InSpriteBatch, ID3D11ShaderResourceView* InWhiteTexture)
	{
		const D3D11_VIEWPORT Viewport = InRHI.GetViewport();
		const float CenterX = Viewport.Width - 78.0f;
		const float CenterY = 82.0f;
		const float AxisLength = 34.0f;
		const float AxisLabelOffset = 10.0f;
		const float AxisThickness = 3.0f;

		const RECT BackgroundRect =
		{
			static_cast<LONG>(CenterX - 46.0f),
			static_cast<LONG>(CenterY - 46.0f),
			static_cast<LONG>(CenterX + 46.0f),
			static_cast<LONG>(CenterY + 46.0f)
		};

		InSpriteBatch.Draw(InWhiteTexture, BackgroundRect, DirectX::XMVectorSet(0.05f, 0.05f, 0.05f, 0.28f));

		const FQuat CameraRotation = InCamera.GetRotation().GetNormalized();
		std::array<FAxisGizmoVisual, 3> Axes =
		{
			FAxisGizmoVisual{ 'X', CameraRotation.UnrotateVector(FVector::ForwardVector), 0.96f, 0.28f, 0.28f, 1.0f },
			FAxisGizmoVisual{ 'Y', CameraRotation.UnrotateVector(FVector::RightVector), 0.32f, 0.86f, 0.38f, 1.0f },
			FAxisGizmoVisual{ 'Z', CameraRotation.UnrotateVector(FVector::UpVector), 0.30f, 0.54f, 0.98f, 1.0f },
		};

		std::sort(Axes.begin(), Axes.end(), [](const FAxisGizmoVisual& A, const FAxisGizmoVisual& B)
		{
			return A.CameraSpaceDirection.X < B.CameraSpaceDirection.X;
		});

		const DirectX::XMFLOAT2 Center(CenterX, CenterY);
		for (const FAxisGizmoVisual& Axis : Axes)
		{
			const float EndX = CenterX + Axis.CameraSpaceDirection.Y * AxisLength;
			const float EndY = CenterY - Axis.CameraSpaceDirection.Z * AxisLength;
			const float DepthAlpha = 0.55f + 0.45f * ((Axis.CameraSpaceDirection.X + 1.0f) * 0.5f);
			const float FinalAlpha = Axis.Alpha * DepthAlpha;

			const DirectX::XMFLOAT2 EndPoint(EndX, EndY);
			DrawScreenLine(&InSpriteBatch, InWhiteTexture, Center, EndPoint, Axis.Red, Axis.Green, Axis.Blue, FinalAlpha, AxisThickness);

			const float LabelX = CenterX + Axis.CameraSpaceDirection.Y * (AxisLength + AxisLabelOffset) - 5.0f;
			const float LabelY = CenterY - Axis.CameraSpaceDirection.Z * (AxisLength + AxisLabelOffset) - 8.0f;
			DrawBitmapText(&InSpriteBatch, InWhiteTexture, std::string(1, Axis.Label), LabelX, LabelY, 2.0f, Axis.Red, Axis.Green, Axis.Blue, FinalAlpha);
		}

		const RECT CenterDotRect =
		{
			static_cast<LONG>(CenterX - 2.0f),
			static_cast<LONG>(CenterY - 2.0f),
			static_cast<LONG>(CenterX + 2.0f),
			static_cast<LONG>(CenterY + 2.0f)
		};
		InSpriteBatch.Draw(InWhiteTexture, CenterDotRect, DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.85f));
	}

	std::string SanitizeBitmapText(std::string InText)
	{
		for (char& Character : InText)
		{
			const unsigned char UnsignedCharacter = static_cast<unsigned char>(Character);
			if (std::isalnum(UnsignedCharacter) != 0 || Character == ' ' || Character == '-' || Character == '.' || Character == ':')
			{
				continue;
			}

			Character = ' ';
		}

		return InText;
	}
}

struct FHudRenderer::FResources
{
	TComPtr<ID3D11ShaderResourceView> WhiteTextureView;
	std::unique_ptr<DirectX::SpriteBatch> SpriteBatch;
};

FHudRenderer::FHudRenderer() = default;
FHudRenderer::~FHudRenderer() = default;

bool FHudRenderer::Initialize(FD3D11RHI& InRHI)
{
	ID3D11Device* Device = InRHI.GetDevice();
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	if (Device == nullptr || DeviceContext == nullptr)
	{
		return false;
	}

	Resources = std::make_unique<FResources>();
	if (!Resources)
	{
		return false;
	}

	if (!CreateWhiteTexture(Device, Resources->WhiteTextureView))
	{
		Resources.reset();
		return false;
	}

	Resources->SpriteBatch = std::make_unique<DirectX::SpriteBatch>(DeviceContext);
	if (!Resources->SpriteBatch)
	{
		Resources.reset();
		return false;
	}

	return true;
}

void FHudRenderer::Shutdown()
{
	Resources.reset();
}

void FHudRenderer::Render(
	const FD3D11RHI& InRHI,
	const FCamera& InCamera,
	const FScene& InScene,
	const FStatsSystem& InStatsSystem,
	const FPickState& InPickState)
{
	if (!Resources || !Resources->SpriteBatch || !Resources->WhiteTextureView)
	{
		return;
	}

	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	if (DeviceContext == nullptr)
	{
		return;
	}

	std::ostringstream TextStream;
	TextStream << std::fixed << std::setprecision(2);
	TextStream
		<< "FPS: " << InStatsSystem.GetFramesPerSecond() << '\n'
		<< "FRAME: " << InStatsSystem.GetFrameTimeMs() << " MS\n"
		<< "PICK LAST: " << InStatsSystem.GetLastPickTimeMs() << " MS\n"
		<< "PICK COUNT: " << InStatsSystem.GetTotalPickCount() << '\n'
		<< "PICK TOTAL: " << InStatsSystem.GetTotalPickTimeMs() << " MS\n"
		<< "GPU: " << SanitizeBitmapText(InRHI.GetAdapterName().empty() ? std::string("UNKNOWN") : InRHI.GetAdapterName());

	if (InRHI.GetAdapterDedicatedVideoMemoryMB() > 0)
	{
		TextStream << ' ' << InRHI.GetAdapterDedicatedVideoMemoryMB() << " MB";
	}

	TextStream
		<< '\n'
		<< "PRIMS: " << InScene.GetRenderItems().size() << '\n'
		<< "SELECTED: " << InPickState.SelectedPrimitiveId;

	DirectX::SpriteBatch& SpriteBatch = *Resources->SpriteBatch;
	ID3D11ShaderResourceView* WhiteTexture = Resources->WhiteTextureView.Get();
	ID3D11RenderTargetView* RenderTargets[] = { InRHI.GetBackBufferRTV() };
	const D3D11_VIEWPORT Viewport = InRHI.GetViewport();
	DeviceContext->OMSetRenderTargets(1, RenderTargets, InRHI.GetDepthStencilView());
	DeviceContext->RSSetViewports(1, &Viewport);
	SpriteBatch.SetViewport(InRHI.GetViewport());
	SpriteBatch.Begin();
	DrawAxisGizmo(InCamera, InRHI, SpriteBatch, WhiteTexture);
	DrawBitmapText(&SpriteBatch, WhiteTexture, TextStream.str(), 16.0f, 16.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	SpriteBatch.End();
}
