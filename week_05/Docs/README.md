# 학습 내용

## 필수 구현 내용 (Implementation)

- 표준 StaticMesh OBJ  
  하위 폴더 `Data`를 만들고 압축을 푸세요.  
  `JungleApples.zip`

- 표준 씬 (Scene)  
  `DefaultScene.zip`

- 퍼포먼스 측정 정보를 화면 좌상단에 출력하세요.
  - FPS와 경과 시간을 ms 단위로 출력하세요.
  - 가장 마지막 Picking에 소요된 시간을 ms 단위로 출력하세요.
  - 누적된 Picking 횟수를 출력하세요.
  - 누적된 Picking 소요 시간을 ms 단위로 출력하세요.

## 대회 규정

### 우승팀 선발 기준

- FPS (ms) 부문 순위 + Picking (ms) 부문 순위로 결정
- 동률 시, 결승전을 별도로 실시한다.

### 우승팀 포상 내용

- 정글 티셔츠 4장
- 식권 4장

### 제한 사항 (Restrictions)

- Instanced Rendering 을 이용하지 마세요.
  - `ID3D11DeviceContext::DrawInstanced`, `ID3D11DeviceContext::DrawIndexedInstanced`와 같은 함수 사용 금지
  - 각각의 StaticMesh 마다 Draw Call을 하세요.
- 마우스 좌표에 맞는 StaticMesh가 Picking되지 않으면 0점 처리됨
- Picking의 퍼포먼스보다 정상 동작을 우선함
- 대회에서는 표준 씬, 표준 StaticMesh가 변경될 수 있습니다.
- 카메라 정보, 오브젝트의 수, 포함된 Mesh가 변경될 수 있습니다.
- 각 팀의 퍼포먼스는 시연용 Laptop Alienware (RTX 3070급)에서 측정됩니다.
  - Alienware m15 R5 Gaming Laptop with AMD Ryzen 5000 CPU | Dell Australia

### 팀간 무한 경쟁 촉진 정책

- 각 팀은 최신의 퍼포먼스 측정 정보 (FPS ms, Picking ms)를 매일 학습 종료 때 슬랙에 공유합니다. 하지만 의무는 아님
- 어떤 최적화 방법을 사용하고 있는지는 다른 팀에게 비밀로 할 수 있음
- 반드시 실시간 (RealTime) 연산으로 구현한다.
- 카메라가 움직이지 않을 때 이전에 계산된 (Cache) 데이터를 사용하여 렌더링하는 방식으로 구현하지마세요.
- 배치 되어 있는 오브젝트가 실시간으로 카메라 안과 밖으로 움직일 수 있는 상황을 가정 하세요.

---

# 학습 자료

## Picking 소요 시간 측정

다음과 같이 하세요.

```cpp
Handle WM_LBUTTONDOWN()
{
    // 1) 마우스 화면 좌표 획득
    int screenX = GetMouseScreenX();
    int screenY = GetMouseScreenY();

    // 2) 화면 좌표 -> 월드 좌표로의 픽 레이(Pick Ray) 계산
    Vector pickRay = CalculatePickRay(
        projectionMatrix,
        viewMatrix,
        cameraPosition,
        screenX,
        screenY
    );

    // 3) 퍼포먼스 측정용 카운터 시작
    FScopeCycleCounter pickCounter;

    // 4) 전체 Picking 횟수 누적
    ++TotalPickCount;

    // 5) 모든 오브젝트(프리미티브)에 대해 충돌 판정
    for (auto& primitive : primitives)
    {
        bool isHit = primitive.CheckHit(cameraPosition, pickRay);

        // 필요 시 'isHit' 결과를 활용해 추가 로직 처리
    }

    // 6) 퍼포먼스 측정 종료 및 시간 누적
    LastPickTime = pickCounter.Finish();
    TotalPickTime += LastPickTime;
}
```

## RAII (Resource Acquisition Is Initialization)

```cpp
class FWindowsPlatformTime
{
public:
    static double GSecondsPerCycle; // 0
    static bool bInitialized; // false

    static void InitTiming()
    {
        if (!bInitialized)
        {
            bInitialized = true;
            double Frequency = (double)GetFrequency();
            if (Frequency <= 0.0)
            {
                Frequency = 1.0;
            }
            GSecondsPerCycle = 1.0 / Frequency;
        }
    }

    static float GetSecondsPerCycle()
    {
        if (!bInitialized)
        {
            InitTiming();
        }
        return (float)GSecondsPerCycle;
    }

    static uint64 GetFrequency()
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        return Frequency.QuadPart;
    }

    static double ToMilliseconds(uint64 CycleDiff)
    {
        double Ms = static_cast<double>(CycleDiff)
            * GetSecondsPerCycle()
            * 1000.0;
        return Ms;
    }

    static uint64 Cycles64()
    {
        LARGE_INTEGER CycleCount;
        QueryPerformanceCounter(&CycleCount);
        return (uint64)CycleCount.QuadPart;
    }
};

struct TStatId
{
};

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
    FScopeCycleCounter(TStatId StatId)
        : StartCycles(FPlatformTime::Cycles64())
        , UsedStatId(StatId)
    {
    }

    ~FScopeCycleCounter()
    {
        Finish();
    }

    uint64 Finish()
    {
        const uint64 EndCycles = FPlatformTime::Cycles64();
        const uint64 CycleDiff = EndCycles - StartCycles;

        // FThreadStats::AddMessage(UsedStatId, EStatOperation::Add, CycleDiff);
        return CycleDiff;
    }

private:
    uint64 StartCycles;
    TStatId UsedStatId;
};
```

## Scene Graph (Management)

- Bounding volume hierarchy - Wikipedia
- K-D Tree
- Octree

## SIMD, Align

- SSE2 - 위키백과, 우리 모두의 백과사전

## Frustum Culling

- LearnOpenGL - Frustum Culling

## PIX

- Download - PIX on Windows

## 관련 서적

- *Introduction to 3D Game Programming with DirectX 11*
  - Chapter 15 - Frustums
- *Game Engine Architecture*
  - GEA 4.10 SIMD
  - GEA 11.2.7 Scene Graph
