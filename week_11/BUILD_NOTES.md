# week_11 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\JSEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- 첫 빌드는 pre-build 단계에서 RmlUi와 SoLoud 정적 라이브러리를 소스에서 생성하므로 시간이 더 걸립니다.
- 생성 파일은 `JSEngine\ThirdParty\RmlUi\Lib`와 `JSEngine\ThirdParty\SoLoud\Lib` 아래에 생기며 커밋 대상이 아닙니다.
- 재생성 스크립트는 Windows PowerShell 호환성을 위해 .NET SHA-256 API를 사용합니다. 실행 정책으로 스크립트가 차단되면 현재 프로세스에서 PowerShell 스크립트 실행을 허용해야 합니다.
- FBX SDK 라이브러리와 DLL은 `JSEngine\ThirdParty\FBX\lib\debug` 또는 `release` 경로에서 참조됩니다.
- 자세한 재생성 규칙은 `JSEngine\ThirdParty\README.md`를 참고하세요.
