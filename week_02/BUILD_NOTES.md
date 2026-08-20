# week_02 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\KcraftonJungle_WEEK02\KcraftonJungle_WEEK02.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- `URenderer.h`와 `URenderer.cpp`의 pragma 영역명은 소스 인코딩 영향을 피하도록 ASCII로 유지해야 합니다. 한글 영역명을 다시 넣으면 환경에 따라 `C3872`가 발생할 수 있습니다.
- 별도 NuGet 또는 vcpkg 복원 없이 빌드됩니다.
