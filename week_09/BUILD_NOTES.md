# week_09 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

LuaJIT을 vcpkg 매니페스트로 먼저 설치합니다.

```powershell
& "$env:VCPKG_ROOT\vcpkg.exe" install --x-manifest-root=. --triplet x64-windows
```

```powershell
msbuild .\CrashEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64 /p:VcpkgTriplet=x64-windows
```

## 주의사항

- `VCPKG_ROOT`는 vcpkg 설치 폴더를 가리켜야 합니다.
- `Debug|x64`는 `lua51.lib`와 `vcpkg_installed\x64-windows\lib`를 사용합니다. 의존성 복원 없이 빌드하면 Lua 심볼 링크 오류가 발생합니다.
- 후처리 단계에서 `lua51.dll`과 저장소의 FMOD DLL을 출력 폴더로 복사합니다.
- DirectXTK 복원 중 `NU1902` 경고가 나타날 수 있으나 검증 빌드에는 영향을 주지 않았습니다.
