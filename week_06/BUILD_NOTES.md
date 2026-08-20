# week_06 빌드 주의사항

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
- `Demo`, `ObjViewDebug`, `Release` 등 여러 구성이 있지만 현재 확인된 구성은 `Debug|x64`입니다.
- DirectXTK 복원 중 `NU1902` 경고가 나타날 수 있으나 검증 빌드에는 영향을 주지 않았습니다.
