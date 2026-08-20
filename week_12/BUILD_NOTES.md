# week_12 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\KraftonEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- `KraftonEngine\packages.config`에서 DirectXTK와 NVIDIA PhysX를 복원합니다.
- 소스의 절대 경로가 길면 생성 파일 include를 찾지 못할 수 있습니다. `C:\src\week12`처럼 짧은 경로에 클론하거나 작업 드라이브를 매핑해 빌드하세요.
- `Demo`, `Game`, `ObjViewDebug` 등의 구성은 별도 런타임 데이터가 필요할 수 있으며 검증 기준은 `Debug|x64`입니다.
- DirectXTK 복원 중 `NU1902` 경고가 나타날 수 있으나 검증 빌드에는 영향을 주지 않았습니다.
