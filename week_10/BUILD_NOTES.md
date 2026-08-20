# week_10 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

LuaJIT, Sol2, RmlUi, SFML을 vcpkg 매니페스트로 먼저 설치합니다.

```powershell
& "$env:VCPKG_ROOT\vcpkg.exe" install --x-manifest-root=. --triplet x64-windows
```

```powershell
msbuild .\KraftonEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64 /p:VcpkgTriplet=x64-windows /p:WithFbxSdk=false
```

## 주의사항

- 깨끗한 환경에서는 Autodesk FBX SDK 없이 빌드되며 FBX 임포터 관련 소스만 제외됩니다. 캐시된 엔진 에셋 로드는 유지됩니다.
- FBX 임포터가 필요하면 로컬 `ThirdParty\FBXSDK`에 SDK를 준비하고 `/p:WithFbxSdk=true`로 빌드하세요. `libfbxsdk.lib` 및 DLL의 재배포 권한은 별도로 확인해야 합니다.
- SFML은 저장소의 로컬 바이너리가 아니라 vcpkg 매니페스트로 복원할 수 있습니다.
- DirectXTK 복원 중 `NU1902` 경고가 나타날 수 있으나 검증 빌드에는 영향을 주지 않았습니다.
