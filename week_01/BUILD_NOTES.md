# week_01 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

처음 빌드하기 전에 FMOD 압축 파일을 같은 폴더에 풉니다.

```powershell
Expand-Archive .\KraftonJungle3_Week1_Team5\FMod\FMod.zip -DestinationPath .\KraftonJungle3_Week1_Team5\FMod -Force
```

```powershell
msbuild .\KraftonJungle3_Week1_Team5\KraftonJungle3_Week1_Team5.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- `packages.config`의 DirectXTK가 `/restore` 과정에서 복원되어야 합니다.
- FMOD 헤더만 있고 라이브러리 또는 DLL이 없으면 링크나 실행 단계에서 실패합니다. `FMod.zip`을 푼 뒤 빌드하세요.
- DirectXTK 복원 중 `NU1902`가 표시될 수 있습니다. 현재 검증에서는 경고이며 빌드를 막지는 않았습니다.
