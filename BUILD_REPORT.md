# 주차별 빌드 검증 보고서

- 검증일: 2026-08-19 (Asia/Seoul)
- 환경: Visual Studio 2022 Community, MSBuild 17, Windows x64
- 구성: 각 대표 솔루션의 `Debug|x64`
- 범위: 컴파일 및 링크 검증. 실행 시나리오와 런타임 동작은 검증하지 않음.
- 에셋 정리: 빌드 검증 후 음원과 일부 런타임 모델·텍스처를 이력에서 제거했다. 소스 코드와 빌드용 라이브러리는 유지했지만, 제거한 런타임 콘텐츠를 사용하는 장면의 실행은 보장하지 않는다. 정리 후 `week_11`과 `week_14`를 다시 빌드해 성공을 확인했다.

## 요약

| 폴더 | 결과 | 대표 솔루션 | 비고 |
|---|---|---|---|
| `week_01` | 성공 | `KraftonJungle3_Week1_Team5.sln` | NuGet 복원 및 저장소의 `FMod/FMod.zip` 압축 해제 후 성공 |
| `week_02` | 실패 | `KcraftonJungle_WEEK02.sln` | `URenderer.h:84`의 비 ASCII pragma 영역명에서 C3872 발생 |
| `week_03` | 성공 | `Week3.sln` | 추가 준비 없음 |
| `week_04` | 성공 | `KraftonJungleEngine.sln` | NuGet 복원 후 성공 |
| `week_05` | 성공 | `PerformanceEngine.sln` | NuGet 복원 후 성공 |
| `week_05+` | 성공 | `DinoEngine.sln` | NuGet 복원 후 성공 |
| `week_06` | 성공 | `KraftonEngine.sln` | NuGet 복원 후 성공 |
| `week_07` | 성공 | `KraftonEngine.sln` | NuGet 복원 후 성공 |
| `week_08` | 성공 | `NipsEngine.sln` | NuGet 복원 후 성공 |
| `week_09` | 실패 | `CrashEngine.sln` | vcpkg LuaJIT 설치 후 컴파일 성공, 링크에서 Lua 심볼 72개 미해결 |
| `week_10` | 성공 | `KraftonEngine.sln` | vcpkg/NuGet 복원과 원본 로컬 폴더의 FBX SDK·SFML 런타임/임포트 라이브러리 사용 후 성공 |
| `week_11` | 성공 | `JSEngine.sln` | NuGet 복원, RmlUi 라이브러리 재생성 후 성공 |
| `week_12` | 성공 | `KraftonEngine.sln` | NuGet 복원 및 짧은 작업 경로에서 성공 |
| `week_13` | 성공 | `KraftonEngine.sln` | NuGet 복원 및 짧은 작업 경로에서 성공 |
| `week_14` | 성공 | `KraftonEngine.sln` | NuGet 복원 및 짧은 작업 경로에서 성공 |

총 15개 폴더 중 13개 성공, 2개 실패다.

## 실패 상세

### week_02

- 위치: `week_02/KcraftonJungle_WEEK02/Engine/Rendering/URenderer.h:84`
- 오류: `C3872` — 잘못된 문자 `0x2022`, `0x201c`, `0x00A4`
- 문제 줄: `#pragma region D3D11 Renderer 함수들`
- 판단: UTF-8로 보이는 한글 pragma 영역명이 현재 컴파일러 소스 문자 인코딩과 맞지 않는다. 프로젝트의 소스 문자 집합을 UTF-8로 통일하거나 pragma 영역명을 ASCII로 바꾸는 수정이 필요하다.

### week_09

- 준비: `vcpkg install --triplet x64-windows`로 LuaJIT 설치 후 `VcpkgTriplet=x64-windows`를 지정했다.
- 결과: 컴파일은 완료됐지만 `__imp_lua_*`, `__imp_luaL_*`, `__imp_luaopen_*` 등 72개 심볼이 미해결되어 `LNK1120`으로 실패했다.
- 원인: `CrashEngine.vcxproj`의 `Debug|x64` 링크 설정(288~292행)은 `lua51.lib`를 포함하지 않는다. 같은 프로젝트의 `Release|x64` 설정(313~317행)에는 `lua51.lib`와 vcpkg 라이브러리 경로가 들어 있다.
- 판단: `Debug|x64`의 `AdditionalDependencies`와 `AdditionalLibraryDirectories`에 LuaJIT 링크 설정을 맞추는 프로젝트 수정이 필요하다.

## 빌드 준비 및 주의사항

- `week_01`, `week_10`의 일부 바이너리 의존성은 Git 원격 이력에 없고 기존 로컬 폴더 또는 저장소 내 압축 파일에만 있었다. 빌드 검증에는 사용했지만 통합 저장소 커밋에는 추가하지 않았다.
- `week_11`의 대용량 RmlUi 라이브러리는 이력 정리 과정에서 제거됐으며, 프로젝트의 빌드 스크립트로 다시 생성해 검증했다. 생성 결과는 커밋하지 않았다.
- `week_12`~`week_14`는 현재 통합 저장소의 절대 경로가 길어 생성 파일 include를 찾지 못했다. 짧은 임시 경로에서 동일 소스를 빌드하면 성공했다.
- `directxtk_desktop_win10` 2025.10.28.2 복원 시 알려진 중간 심각도 취약점 `NU1902` 경고가 발생했다: `GHSA-c55g-rp4x-fx84`.
- `week_13`과 `week_14`에서는 PhysX PDB를 찾지 못한다는 `LNK4099` 경고가 있었지만 링크는 성공했다.
- 빌드 산출물, 복원된 패키지, vcpkg 설치 파일 및 로컬 전용 외부 바이너리는 이 커밋에 포함하지 않았다.

### week_11 RmlUi 라이브러리 재생성

`week_11/JSEngine/JSEngine.vcxproj`의 pre-build 단계가 필요한 RmlUi 및 SoLoud 정적 라이브러리를 자동으로 생성합니다. 따라서 Visual Studio에서 `JSEngine.sln`을 `Debug|x64`로 빌드하는 것이 기본 방법입니다.

RmlUi만 먼저 만들려면 `week_11/JSEngine`에서 다음 명령을 실행합니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\BuildTools\Scripts\BuildRmlUiLib.ps1 -ProjectDir "$PWD" -Configuration Debug -Platform x64
```

생성 파일은 `ThirdParty/RmlUi/Lib/x64/Debug/RmlUiCore.lib`에 놓이며 Git에는 포함되지 않습니다. Release 계열 구성은 `-Configuration Release`를 사용합니다. 자세한 재생성 조건은 `week_11/JSEngine/ThirdParty/README.md`에 정리돼 있습니다.

Windows PowerShell에서 `Get-FileHash`를 사용할 수 없는 환경도 지원하도록 RmlUi와 SoLoud 재생성 스크립트는 .NET SHA-256 API로 입력 파일 해시를 계산합니다. 이 경로로 `Debug|x64` 전체 빌드와 링크가 성공하는 것을 재검증했습니다.

## 대용량 파일 정책

통합 이력의 50 MiB 이상 일반 Git blob과 Git LFS 에셋 제거 정책은 그대로 유지했다. 빌드 확인 과정에서 복원하거나 생성한 외부 라이브러리와 산출물은 Git에 추가하지 않았다. 추가 에셋 정리 기준은 `ASSET_POLICY.md`를 따른다.
