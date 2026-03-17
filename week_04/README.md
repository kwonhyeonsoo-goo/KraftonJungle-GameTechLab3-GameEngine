# KraftonJungleEngine

크래프톤 정글 3기 Week2 Team1 — C++ 게임 엔진 구현 프로젝트

DirectX 11 기반의 3D 게임 엔진을 밑바닥부터 직접 구현한다.
UObject 시스템, Material 기반 렌더링 파이프라인, 씬 직렬화, ImGui 에디터 등 게임 엔진의 핵심 요소를 다룬다.

---

## 프로젝트 구조

```
KraftonJungleEngine.sln
├── Engine/              # 엔진 코어 시스템
│   ├── Source/
│   │   ├── Core/        # 엔진 초기화, 루프, FPaths, FTimer
│   │   ├── Object/      # UObject, ObjectFactory, ObjectManager
│   │   │   ├── Actor/   # AActor, ACubeActor, ASphereActor
│   │   │   └── Scene/   # Scene 관리, JSON 직렬화
│   │   ├── Component/   # ActorComponent, SceneComponent, PrimitiveComponent 등
│   │   ├── Math/        # FVector, FMatrix, FQuat, FTransform, FFrustum 등
│   │   ├── Renderer/    # DX11 렌더러, Material, Shader, RenderCommand
│   │   ├── Primitive/   # 메시 생성/캐싱 (Cube, Plane, Sphere)
│   │   ├── Camera/      # 카메라 이동/회전
│   │   ├── Input/       # Windows 입력 처리
│   │   ├── Platform/    # 윈도우 생성, 플랫폼 전역
│   │   ├── Debug/       # 엔진 로그
│   │   └── Types/       # TArray, TMap, TSet, TQueue, FString 등
│   └── Shaders/         # HLSL 셰이더 파일
├── Editor/              # ImGui 기반 에디터
│   └── Source/
│       ├── UI/          # PropertyWindow, ControlPanel, Console, Stat
│       ├── Gizmo/       # Transform Gizmo (Location/Rotation/Scale)
│       ├── Picking/     # Ray Casting 오브젝트 피킹
│       └── Axis/        # 좌표축 렌더링
├── Client/              # 게임 클라이언트 진입점
└── Assets/              # 에셋 파일
    ├── Materials/       # JSON Material 정의
    └── Scenes/          # JSON Scene 정의
```

| 프로젝트 | 역할 | 비고 |
|----------|------|------|
| **Engine** | UObject, Component, Material, 렌더링, 수학 라이브러리 등 엔진 뼈대 | 주요 작업 영역 |
| **Editor** | ImGui 기반 씬 편집기, Gizmo, Picking, Property 창 | 주요 작업 영역 |
| **Client** | 게임 클라이언트 진입점 | 추후 확장 예정 |

> 현재 세 프로젝트 모두 `Application`(.exe) 타입으로 설정되어 있음.
> Editor/Client가 Engine 코드를 공유하는 구조로 전환 시 Engine을 `StaticLibrary`로 변경 필요.

---

## 개발 환경

| 항목 | 내용 |
|------|------|
| IDE | Visual Studio 2022 |
| 언어 | C++20 |
| 그래픽 API | DirectX 11 |
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

---

## 구현 범위

### 렌더링 파이프라인
- DirectX 11 기반 렌더링
- Material 시스템: JSON 기반 Material 정의, 이름 기반 파라미터 API (`SetScalarParameter`, `SetVectorParameter`)
- Constant Buffer 레이아웃: b0=Frame (View/Projection), b1=Object (World), b2+=Material
- RenderCommand 정렬/배칭 (Material 기준)
- HLSL 셰이더 (Vertex/Pixel/Outline/Color) + 공유 헤더 (`ShaderCommon.hlsli`)
- Frustum Culling (FFrustum)
- Outline 렌더링 (Stencil Buffer)
- Primitive 렌더링 (Plane, Cube, Sphere) — 메시 캐싱 지원

### 수학 라이브러리
- FVector, FVector4, FMatrix (4x4), FQuat (Quaternion)
- FRotator, FTransform
- FFrustum (절두체 추출 및 가시성 판정)
- MathUtility (공용 수학 함수)

### 엔진 코어
- 엔진 루프: `ProcessInput → Tick(DeltaTime) → Physics → GameLogic → Render`
- UObject 시스템 (UClass 리플렉션, IsA() 타입 체크, GUObjectArray)
- Component 시스템 (USceneComponent → UPrimitiveComponent, 라이프사이클 관리)
- Object Factory (`FObjectFactory::ConstructObject()`)
- UUID 발행 (`UEngineStatics::GenUUID()`)
- 힙 메모리 관리 (`new` / `delete` 오버로딩, 사용량 추적)
- UE 스타일 컨테이너 (TArray, TMap, TSet, TQueue, TLinkedList, FString)
- FPaths 유틸리티 — 프로젝트 루트 자동 탐지, 중앙 집중식 경로 관리

### Actor / Component
- AActor 기반 클래스 + PostSpawnInitialize 훅
- ACubeActor, ASphereActor — 자동 PrimitiveComponent 생성 및 Material 할당
- URandomColorComponent — 설정 간격마다 랜덤 색상 업데이트
- `SpawnActor<T>()` 템플릿 팩토리
- `GetComponentByClass<T>()` 컴포넌트 검색

### 씬 관리
- JSON 기반 씬 직렬화 (Save/Load, `.scene` 포맷)
- Actor 라이프사이클: Register → BeginPlay → Tick → PendingKill 정리
- DefaultScene.json 기본 레벨

### 에디터 (Editor)
- ImGui 기반 도킹 레이아웃
- Property 창 (선택된 Actor 속성 표시)
- Control Panel (씬 계층 구조)
- Console 창 (디버그 로그)
- Stat 창 (성능 통계)
- 오브젝트 피킹 (Ray Casting, Möller-Trumbore 알고리즘)
- Gizmo 컨트롤 (Location / Rotation / Scale)
- World / Local 좌표계 축 렌더링
- 카메라: W/A/S/D 이동, Q/E 상하, 마우스 우클릭 회전

### 에셋
- Material JSON 포맷: 셰이더 경로, ConstantBuffer 정의, 타입별 파라미터
- 기본 제공 Material: M_RedColor, M_GreenColor, M_BlueColor, M_TintYellow

---

## 팀 구성

4인 팀 / 2인씩 역할 분담

| 팀 | 담당 | 주요 프로젝트 |
|----|------|--------------|
| 엔진 프레임워크 (2인) | 루프 구조, UObject, Component, Factory, 메모리, 씬 직렬화 | `Engine` |
| 그래픽 프레임워크 (2인) | 셰이더, Material, Primitive 렌더, Gizmo, Picking | `Editor` |

---

## 코딩 규약

| 접두사 | 대상 |
|--------|------|
| `A` | Actor 클래스 |
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
