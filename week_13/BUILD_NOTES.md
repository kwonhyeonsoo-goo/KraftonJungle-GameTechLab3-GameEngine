# week_13 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\KraftonEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- `KraftonEngine\packages.config`의 DirectXTK가 `/restore`로 복원되어야 합니다.
- 긴 절대 경로에서는 생성 파일 include 문제가 발생할 수 있으므로 가능한 한 짧은 경로에서 빌드하세요.
- PhysX 관련 PDB를 찾지 못한다는 `LNK4099`가 표시될 수 있습니다. 현재 검증에서는 디버그 심볼 경고이며 링크는 성공했습니다.
- 저장소의 `ThirdParty\NvCloth` 관련 안내도 함께 확인하세요.
