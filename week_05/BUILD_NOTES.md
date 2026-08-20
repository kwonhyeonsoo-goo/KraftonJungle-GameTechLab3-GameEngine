# week_05 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\PerformanceEngine.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- `PerformanceEngine\packages.config`의 DirectXTK가 `/restore`로 복원되어야 합니다.
- 성능 측정용 실행 결과는 Debug와 Release에서 크게 다를 수 있습니다. 성능 비교는 Release를 별도로 빌드해 진행하세요.
- 이 저장소에서 검증된 구성은 `Debug|x64`입니다.
