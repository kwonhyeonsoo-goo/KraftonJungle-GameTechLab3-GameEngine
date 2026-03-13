# KraftonJungleEngine

크래프톤 정글 3기 Week2 Team1 — C++ 게임 엔진 구현 프로젝트

DirectX 기반의 3D 게임 엔진을 밑바닥부터 직접 구현한다.  
MVP 변환 파이프라인, UObject 시스템, 렌더링, 씬 편집 도구 등 게임 엔진의 핵심 요소를 다룬다.

---

## 프로젝트 구조

```
KraftonJungleEngine.sln
├── Engine/          # 엔진 코어 시스템
├── Editor/          # 에디터 UI 및 씬 편집 도구
└── Client/          # 게임 클라이언트 진입점
```

| 프로젝트 | 역할 | 비고 |
|----------|------|------|
| **Engine** | UObject, Component, Factory, 루프 구조 등 엔진 뼈대 | 주요 작업 영역 |
| **Editor** | ImGui 기반 씬 편집기, Gizmo, Property 창 | 주요 작업 영역 |
| **Client** | 게임 클라이언트 진입점 | 추후 확장 예정 |

> 현재 세 프로젝트 모두 `Application`(.exe) 타입으로 설정되어 있음.  
> Editor/Client가 Engine 코드를 공유하는 구조로 전환 시 Engine을 `StaticLibrary`로 변경 필요.

---

## 개발 환경

| 항목 | 내용 |
|------|------|
| IDE | Visual Studio 2022 |
| 언어 | C++20 |
| 플랫폼 | Windows x64 |
| Toolset | MSVC v143 |
| 문자 인코딩 | `/utf-8` |
| 문자 집합 | Unicode |
| Windows SDK | 10.0 |

---

## 빌드 방법

Visual Studio에서 `KraftonJungleEngine.sln`을 열고 빌드하거나, Developer Command Prompt에서 아래 명령어를 사용한다.

```bash
# 솔루션 전체 빌드
msbuild KraftonJungleEngine.sln /p:Configuration=Debug /p:Platform=x64

# 프로젝트 단위 빌드
msbuild Engine/Engine.vcxproj /p:Configuration=Debug /p:Platform=x64
msbuild Editor/Editor.vcxproj /p:Configuration=Debug /p:Platform=x64
msbuild Client/Client.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### Output 경로

| 구분 | 경로 |
|------|------|
| 실행 파일 (OutDir) | `{Project}/Bin/{Configuration}/` |
| 중간 파일 (IntDir) | `{Project}/Build/{Configuration}/` |

예시 — `Engine` Debug\|x64 빌드 시:
```
Engine/Bin/Debug/    ← Engine.exe
Engine/Build/Debug/  ← .obj 파일
```

---

## 구현 범위

### 렌더링 (Editor 팀)
- MVP 변환 파이프라인 (Model → View → Projection)
- FVector, FVector4, FMatrix 수학 구조체
- Vertex Shader / Pixel Shader (HLSL) 작성 및 MVP 행렬 적용
- Primitive 렌더링 (Plane, Cube, Sphere)
- 카메라 이동 (W/A/S/D) 및 회전 (마우스 우클릭 드래그)
- 오브젝트 피킹 (Ray Casting, Deprojection)
- Gizmo 컨트롤 (Location / Rotation / Scale)
- World / Local 좌표계 축 렌더링
- ImGui Property 창, Stat 창, Console 창

### 엔진 코어 (Engine 팀)
- 엔진 루프 구조: `Main → Core 초기화 → Update`
- UObject 시스템 (최상위 기반 클래스, GUObjectArray)
- Component 시스템 (USceneComponent, UPrimitiveComponent)
- Object Factory (`FObjectFactory::ConstructObject()`)
- UUID 발행 (`UEngineStatics::GenUUID()`)
- 힙 메모리 관리 (`new` / `delete` 오버로딩, 사용량 추적)
- UE 스타일 컨테이너 재정의 (TArray, TMap, TSet, FString 등)
- 씬 직렬화 — Save/Load (JSON, `.scene` 포맷)

---

## 팀 구성

4인 팀 / 2인씩 역할 분담

| 팀 | 담당 | 주요 프로젝트 |
|----|------|--------------| 
| 엔진 프레임워크 (2인) | 루프 구조, UObject, Component, Factory, 메모리 | `Engine` |
| 그래픽 프레임워크 (2인) | 셰이더, Primitive 렌더, Gizmo, Picking | `Editor` |

---

## 코딩 규약

| 접두사 | 대상 |
|--------|------|
| `C` | Generic Class |
| `F` | Structure |
| `U` | UObject 기반 클래스 |
| `T` | Template 클래스 |

- 네이밍: Upper Camel Case (Pascal Case)
- C++ 표준: C++20
- 커밋 메시지: 영문, Conventional Commits 기준

---

## 문서화

Doxygen 기반 API 문서: https://dding-ho.github.io/Jungle3_Week2_Team1/
