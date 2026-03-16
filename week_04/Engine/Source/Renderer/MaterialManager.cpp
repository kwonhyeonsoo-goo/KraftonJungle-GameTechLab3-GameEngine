#include "MaterialManager.h"
#include "Material.h"
#include "Shader.h"
#include "ShaderMap.h"
#include "ThirdParty/nlohmann/json.hpp"
#include <fstream>

// ─── HLSL cbuffer 패킹 유틸 ───

namespace
{
	// 타입별 바이트 크기
	uint32 GetTypeSize(const FString& Type)
	{
		if (Type == "float")    return 4;
		if (Type == "float2")   return 8;
		if (Type == "float3")   return 12;
		if (Type == "float4")   return 16;
		if (Type == "float4x4") return 64;
		return 0;
	}

	// 타입별 정렬 (HLSL 패킹 룰: 16바이트 경계를 넘을 수 없음)
	uint32 AlignOffset(uint32 Offset, uint32 TypeSize)
	{
		// 16바이트 경계 내 남은 공간
		uint32 BoundaryStart = (Offset / 16) * 16;
		uint32 Remaining = BoundaryStart + 16 - Offset;

		// 남은 공간에 들어가지 않으면 다음 16바이트 경계로
		if (TypeSize > Remaining)
		{
			return BoundaryStart + 16;
		}
		return Offset;
	}

	// 전체 상수 버퍼 크기를 16바이트로 정렬
	uint32 AlignBufferSize(uint32 Size)
	{
		return (Size + 15) & ~15;
	}
}

// ─── FMaterialManager ───

FMaterialManager& FMaterialManager::Get()
{
	static FMaterialManager Instance;
	return Instance;
}

std::shared_ptr<FMaterial> FMaterialManager::GetOrLoad(ID3D11Device* Device, const FString& FilePath)
{
	// 경로 캐시 확인
	auto It = PathCache.find(FilePath);
	if (It != PathCache.end())
	{
		return It->second;
	}

	// JSON 파일 로드
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		return nullptr;
	}

	nlohmann::json Json;
	try
	{
		File >> Json;
	}
	catch (...)
	{
		return nullptr;
	}

	// Material 생성
	auto Mat = std::make_shared<FMaterial>();

	// 셰이더 로드
	if (Json.contains("VertexShader"))
	{
		FString VSPath = Json["VertexShader"].get<FString>();
		std::wstring WVSPath(VSPath.begin(), VSPath.end());
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, WVSPath.c_str());
		Mat->SetVertexShader(VS);
	}

	if (Json.contains("PixelShader"))
	{
		FString PSPath = Json["PixelShader"].get<FString>();
		std::wstring WPSPath(PSPath.begin(), PSPath.end());
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, WPSPath.c_str());
		Mat->SetPixelShader(PS);
	}

	// 상수 버퍼 로드
	if (Json.contains("ConstantBuffers"))
	{
		for (auto& CBJson : Json["ConstantBuffers"])
		{
			if (!CBJson.contains("Parameters"))
			{
				continue;
			}

			auto& Params = CBJson["Parameters"];

			// 1차: 오프셋 계산 + 총 크기 산출
			struct FParamInfo
			{
				FString Name;
				uint32 Offset;
				uint32 Size;
				FString Type;
				nlohmann::json Value;
			};
			TArray<FParamInfo> ParamList;
			uint32 CurrentOffset = 0;

			for (auto& P : Params)
			{
				FString Type = P.value("Type", "");
				uint32 TypeSize = GetTypeSize(Type);
				if (TypeSize == 0)
				{
					continue;
				}

				uint32 AlignedOffset = AlignOffset(CurrentOffset, TypeSize);

				FParamInfo Info;
				Info.Name = P.value("Name", "");
				Info.Offset = AlignedOffset;
				Info.Size = TypeSize;
				Info.Type = Type;
				Info.Value = P.contains("Value") ? P["Value"] : nlohmann::json();
				ParamList.push_back(Info);

				CurrentOffset = AlignedOffset + TypeSize;
			}

			if (CurrentOffset == 0)
			{
				continue;
			}

			uint32 BufferSize = AlignBufferSize(CurrentOffset);

			// 2차: 상수 버퍼 생성
			int32 SlotIndex = Mat->CreateConstantBuffer(Device, BufferSize);
			if (SlotIndex < 0)
			{
				continue;
			}

			FMaterialConstantBuffer* CB = Mat->GetConstantBuffer(SlotIndex);

			// 파라미터 이름 등록
			for (auto& Info : ParamList)
			{
				if (!Info.Name.empty())
				{
					Mat->RegisterParameter(Info.Name, SlotIndex, Info.Offset, Info.Size);
				}
			}

			// 3차: 초기값 기록
			for (auto& Info : ParamList)
			{
				if (Info.Value.is_null())
				{
					continue;
				}

				if (Info.Type == "float" && Info.Value.is_number())
				{
					float Val = Info.Value.get<float>();
					CB->SetData(&Val, sizeof(float), Info.Offset);
				}
				else if (Info.Type == "float2" && Info.Value.is_array() && Info.Value.size() >= 2)
				{
					float Val[2] = {
						Info.Value[0].get<float>(),
						Info.Value[1].get<float>()
					};
					CB->SetData(Val, sizeof(Val), Info.Offset);
				}
				else if (Info.Type == "float3" && Info.Value.is_array() && Info.Value.size() >= 3)
				{
					float Val[3] = {
						Info.Value[0].get<float>(),
						Info.Value[1].get<float>(),
						Info.Value[2].get<float>()
					};
					CB->SetData(Val, sizeof(Val), Info.Offset);
				}
				else if (Info.Type == "float4" && Info.Value.is_array() && Info.Value.size() >= 4)
				{
					float Val[4] = {
						Info.Value[0].get<float>(),
						Info.Value[1].get<float>(),
						Info.Value[2].get<float>(),
						Info.Value[3].get<float>()
					};
					CB->SetData(Val, sizeof(Val), Info.Offset);
				}
				else if (Info.Type == "float4x4" && Info.Value.is_array() && Info.Value.size() >= 16)
				{
					float Val[16];
					for (int32 i = 0; i < 16; ++i)
					{
						Val[i] = Info.Value[i].get<float>();
					}
					CB->SetData(Val, sizeof(Val), Info.Offset);
				}
			}
		}
	}

	// 캐시 등록
	PathCache[FilePath] = Mat;

	if (Json.contains("Name"))
	{
		FString Name = Json["Name"].get<FString>();
		Mat->SetName(Name);
		NameCache[Name] = Mat;
	}

	return Mat;
}

std::shared_ptr<FMaterial> FMaterialManager::FindByName(const FString& Name) const
{
	auto It = NameCache.find(Name);
	if (It != NameCache.end())
	{
		return It->second;
	}
	return nullptr;
}

void FMaterialManager::Register(const FString& Name, const std::shared_ptr<FMaterial>& InMaterial)
{
	if (InMaterial)
	{
		NameCache[Name] = InMaterial;
	}
}

void FMaterialManager::Clear()
{
	PathCache.clear();
	NameCache.clear();
}
