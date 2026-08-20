# week_14 빌드 주의사항

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
- PhysX PDB 관련 `LNK4099`가 표시될 수 있습니다. 현재 검증에서는 경고만 발생하고 링크는 성공했습니다.
- public 저장소는 저작권 및 재배포 정책에 따라 일부 런타임 모델·음원·종속 텍스처가 제거되어 있습니다. 컴파일 성공이 모든 장면의 실행 성공을 의미하지는 않습니다.
- DirectXTK 복원 중 `NU1902` 경고가 나타날 수 있으나 검증 빌드에는 영향을 주지 않았습니다.
