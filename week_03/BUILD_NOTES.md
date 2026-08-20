# week_03 빌드 주의사항

## 검증 기준

- Windows 11, Visual Studio 2022 Community의 **Desktop development with C++** 워크로드를 기준으로 합니다.
- 확인된 구성은 `Debug|x64`이며 컴파일과 링크까지 검증했습니다. 실행 시나리오와 다른 구성은 별도 확인이 필요합니다.
- 아래 명령은 **Developer PowerShell for VS 2022**에서 이 주차 폴더를 현재 위치로 두고 실행합니다.

## 빌드

```powershell
msbuild .\Week3.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

## 주의사항

- 별도 패키지 복원 단계가 필요하지 않습니다.
- 저장소의 상대 경로 구조를 유지해야 셰이더와 프로젝트 참조를 정상적으로 찾습니다.
