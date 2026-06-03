# NvCloth World Collision Post-GPU Skinning Plan

## Context

이 문서는 GPU skinning cloth support 이후에 진행할 world collision 계획이다. GPU skinning support 이전에는 `Docs/NvCloth_PhysicsAssetCollision_PreGpuSkinning_Plan.md`에서 physics asset sphere/capsule/box collision을 먼저 구현한다.

world collision은 physics asset collision보다 입력 수집 비용과 API 영향이 크다. world static/dynamic shape는 cloth collision 전용 world shape query/snapshot API, broad query bounds, cloth section bounds scoring, debug view, dynamic shape 예외 처리가 함께 필요하다.

## Prerequisites

- CPU cloth runtime과 physics asset sphere/capsule/box collision이 동작한다.
- GPU skinning cloth support가 완료되어 cloth result가 주요 render pass, bounds, picking/selection path에 안정적으로 반영된다.
- physics asset collision에서 `FClothCollisionGatherer`, `FClothCollisionCandidate`, `FClothCollisionGatherResult`, `FClothCollisionPrimitiveSet`, NvCloth upload helper가 마련되어 있다.

## Goals

- world 안의 static box/sphere/capsule shape를 opt-in으로 cloth collision에 포함한다.
- moving dynamic world shape를 제한적으로 cloth collision에 포함한다.
- physics asset collision에서 만든 candidate/scoring/upload pipeline을 world source로 확장한다.
- component 전체 skinned world bounds로 broad query하고, cloth section world bounds 기준으로 scoring/filter/truncation한다.
- world collision 후보는 blocking shape만 대상으로 한다.
- world collision 도입 시점부터 selected/truncated/rejected primitive를 editor에서 확인할 수 있는 최소 debug view를 제공한다.
- dynamic collision은 current-only primitive upload로 시작하되, future moving collision overload를 위해 previous/current transform을 보존한다.

## No Goals

- GPU skinning cloth support 자체는 이 문서 범위에 포함하지 않는다.
- physics asset collision 구현은 이 문서 범위에 포함하지 않는다.
- self collision, cloth-to-cloth collision, tearing은 포함하지 않는다.
- triangle mesh collider나 arbitrary convex mesh collider는 초기 범위에 포함하지 않는다.
- gameplay physics response를 cloth가 rigidbody에 다시 전달하는 two-way coupling은 구현하지 않는다.
- collision event, hit callback, overlap callback을 cloth에서 발생시키지 않는다.
- Phase 2 초기 dynamic collision에서 NvCloth moving collision overload를 구현하지 않는다. current-only dynamic collision으로 시작하고, 실제 tunneling 문제가 확인될 때 후속 확장으로 붙인다.

## Implementation Policy

### Ownership

- world collision 후보 수집, scoring, budget truncation은 기존 `FClothCollisionGatherer` 계층이 담당한다.
- `USkeletalMeshComponent`는 skeletal mesh, component transform, world 접근 입력을 제공한다.
- `FSkeletalClothRuntime`은 gather 결과의 selected primitive를 NvCloth에 upload하고 simulation을 수행한다.
- `FSkeletalClothRuntime`은 world candidate 수집 로직을 직접 소유하지 않는다.
- `FClothCollisionCandidate`는 gather/scoring/debug 전용 frame-local transient 구조체다.
- `FClothCollisionPrimitiveSet`은 selected candidate만 변환한 순수 geometry 구조체다.

### Authoring And Config

- world static collision enable과 world dynamic collision enable은 cloth asset/section config로 저장한다.
- 저장 위치는 현재 cloth ownership 기준으로 `FSkeletalClothConfig` 또는 section-level cloth payload 확장이다.
- world static/dynamic collision은 opt-in이다.
- world collision 후보는 blocking shape만 대상으로 한다. trigger와 overlap shape는 cloth collision 후보에서 제외한다.

### World Query

- world collision은 debug snapshot 경로를 쓰지 않는다.
- physics runtime에 cloth collision 전용 world shape query/snapshot API를 바로 추가한다.
- 전용 query/snapshot API는 body transform뿐 아니라 shape geometry, filter data, owner id, component id, body type, previous/current transform을 제공해야 한다.
- `FPhysicsWorldSnapshot::DebugBodies`는 debug renderer용 자료로만 유지하고 cloth collision 기능의 입력으로 사용하지 않는다.
- world collision 후보 수집은 component 전체 skinned world bounds로 broad query한다.
- 최종 selection/truncation은 cloth section world bounds와 section expansion bounds 기준으로 수행한다.

### Dynamic Collision

- dynamic world collision은 current-only primitive upload로 시작한다.
- 전용 query/snapshot API와 `FClothCollisionCandidate`에는 previous/current transform을 보존한다.
- 빠르게 움직이는 collider가 cloth를 뚫는 문제가 실제로 확인되면 NvCloth moving collision overload를 후속 확장으로 붙인다.
- teleport로 판단되는 dynamic shape는 해당 frame에서 skip하거나 current-only로 snap한다.
- dynamic 후보 add/remove가 cloth popping을 만들면 hysteresis 또는 short-lived cache로 완화한다.

### Debug Policy

- immediate debug draw는 `FClothCollisionGatherResult::Candidates`를 소비한다.
- NvCloth upload는 `FClothCollisionGatherResult::SelectedPrimitives`만 소비한다.
- world static collision 도입 시 selected, truncated, rejected 후보를 구분하는 최소 debug view를 같이 구현한다.
- dynamic collision은 static/dynamic source, selected/truncated state, previous/current dynamic transform 상태를 debug view에 추가한다.
- final editor UX 단계에서 Mesh Editor workflow로 정리한다.

## Data Shape

이 문서는 pre-GPU physics asset collision 계획에서 만든 runtime data shape을 재사용한다.

world source에서 추가로 필요한 candidate fields:

```cpp
struct FClothCollisionSourceId
{
    EClothCollisionSource Source = EClothCollisionSource::WorldStatic;
    uint32 OwnerActorId = 0;
    uint32 OwnerComponentId = 0;
    FName BoneName = FName::None;
    int32 BodyIndex = -1;
    int32 ShapeIndex = -1;
    ECollisionChannel ObjectChannel = ECollisionChannel::WorldStatic;
};

struct FClothCollisionCandidate
{
    EClothCollisionPrimitiveType Type = EClothCollisionPrimitiveType::Sphere;
    FClothCollisionSourceId SourceId;

    FTransform LocalTransform;
    FTransform PreviousLocalTransform;
    FTransform CurrentLocalTransform;
    FVector HalfExtent = FVector::ZeroVector;
    float Radius = 0.0f;
    float CapsuleHalfHeight = 0.0f;

    FBoundingBox WorldBounds;

    int32 OverlapRank = 1;
    float DistanceScore = 0.0f;
    float CenterDistanceScore = 0.0f;
    int32 TypeCost = 1;
    uint64 StableTieBreaker = 0;

    EClothCollisionSelectState State = EClothCollisionSelectState::Gathered;
};
```

`PreviousLocalTransform` and `CurrentLocalTransform` are preserved for future moving collision overload support. The initial dynamic implementation uploads current-only primitives.

## World Bounds And Candidate Scoring

World collision은 component 전체 skinned world bounds로 broad query를 수행하되, 최종 upload 후보는 cloth section world bounds 기준으로 고른다.

각 candidate는 다음 값을 계산한다.

- `OverlapRank`: primitive world bounds가 cloth section world bounds와 겹치면 0, section expansion bounds 안에만 있으면 1이다.
- `DistanceScore`: primitive world bounds와 cloth section world bounds 사이의 squared distance. 두 bounds가 겹치면 0이다.
- `CenterDistanceScore`: primitive bounds center와 cloth section bounds center 사이의 squared distance. 겹친 후보끼리 tie-break하는 데 사용한다.
- `SourcePriority`: world static, world dynamic 순서의 source priority를 유지한다. physics asset collision은 pre-GPU plan에서 이미 처리된 source이며 overall upload priority에서는 world source보다 앞선다.
- `TypeCost`: box는 plane 6개를 쓰므로 sphere/capsule보다 budget cost가 크다.
- `StableTieBreaker`: source type, owner component id, body index, shape index를 묶은 deterministic key.

정렬 정책:

1. source priority가 높은 후보를 먼저 본다.
2. 같은 source 안에서는 `OverlapRank`가 낮은 후보를 먼저 본다.
3. 같은 overlap rank에서는 `DistanceScore`가 낮은 후보를 먼저 본다.
4. 같은 distance면 `CenterDistanceScore`가 낮은 후보를 먼저 본다.
5. 그래도 같으면 더 낮은 `TypeCost`를 먼저 본다.
6. 마지막으로 `StableTieBreaker`로 정렬한다.

선택 정책:

- sphere/capsule/box authoring budget과 NvCloth sphere/plane/convex upload budget을 동시에 만족하는 후보만 selected로 표시한다.
- budget을 초과한 후보는 uploaded primitive set에 넣지 않고 `TruncatedByBudget` reason을 metadata에 남긴다.
- broad query에는 들어왔지만 section bounds expansion 바깥인 후보는 `RejectedBySectionBounds` reason을 남긴다.
- 각 cloth section은 독립적으로 scoring/truncation한다. 한 section의 먼 collider가 다른 section의 가까운 collider budget을 밀어내면 안 된다.

## Phase 0: Post-GPU World Collision Contract

### Goal

GPU skinning cloth support 완료 후 world collision 입력 계약과 physics runtime query API를 고정한다.

### Tasks

- GPU skinning cloth support 이후 cloth section world bounds를 안정적으로 계산할 수 있는지 확인한다.
- `FClothCollisionGatherer`에 world static/dynamic source gather entry point를 추가한다.
- physics runtime에 cloth collision query 전용 shape snapshot/query API를 추가한다.
- 전용 query/snapshot API가 shape geometry, filter data, owner ids, body type, previous/current transform을 제공하게 한다.
- world query result를 `FClothCollisionCandidate`로 변환한다.
- debug stat에 gathered/selected/rejected/truncated world static/dynamic count를 추가한다.

### Validation

- debug snapshot setting과 무관하게 cloth collision 전용 query/snapshot API path가 동작한다.
- world source가 disabled 상태면 pre-GPU physics asset collision 결과만 유지된다.
- world candidate debug draw가 selected/rejected/truncated 상태를 표시한다.

## Phase 1: World Static Shape Collision

### Goal

world 안의 static box/sphere/capsule shape를 opt-in으로 cloth collision에 포함한다.

### Tasks

- cloth section config에 `bEnableWorldStaticClothCollision` option을 추가한다.
- component 전체 skinned world bounds로 broad query bounds를 만든다.
- cloth section world bounds와 section expansion margin으로 scoring/filter 기준 bounds를 만든다.
- WorldStatic channel의 blocking shape만 대상으로 제한한다.
- owner actor/component, trigger shape, overlap-only shape는 제외한다.
- world static shape를 component-local candidate로 변환하고 section bounds scoring과 budget 우선순위에 따라 selected primitive를 만든다.
- 최소 debug view를 Phase 1에 같이 넣는다. gathered 후보, budget 이후 selected 후보, truncated/rejected 후보 수를 source/type별로 확인할 수 있어야 한다.
- viewport overlay는 최소한 selected world collision primitive와 truncated/rejected candidate bounds를 서로 다른 색으로 표시한다.

### Validation

- level에 놓인 static sphere/capsule/box collider가 cloth를 막는다.
- cloth section bounds expansion 밖의 static collider는 selected 후보에 들어오지 않는다.
- 어떤 static primitive가 NvCloth에 업로드됐고 어떤 primitive가 budget 때문에 truncate됐는지 editor에서 확인할 수 있다.
- world static collision option이 off이면 physics asset collision만 유지된다.

## Phase 2: World Dynamic Shape Collision

### Goal

moving dynamic world shape를 제한적으로 cloth collision에 포함한다.

### Tasks

- cloth section config에 `bEnableWorldDynamicClothCollision` option을 static과 별도로 둔다.
- dynamic 후보는 cloth bounds 주변 + velocity-expanded bounds로 제한한다.
- initial implementation은 dynamic shape도 current-only primitive로 업로드한다.
- moving collision overload를 나중에 붙일 수 있도록 전용 query/snapshot API와 `FClothCollisionCandidate`에는 previous/current transform을 보존한다.
- 후보가 갑자기 추가/제거될 때 popping을 줄이기 위한 hysteresis 또는 short-lived cache를 둔다.
- teleport로 판단되는 dynamic shape는 해당 frame에서 skip하거나 current-only로 snap한다.
- Phase 1 최소 debug view를 확장해 static/dynamic source, selected/truncated state, previous/current dynamic transform 상태를 구분 표시한다.

### Validation

- 움직이는 sphere/capsule/box가 cloth를 뚫고 지나가지 않고 제한적으로 상호작용한다.
- 빠르게 teleport한 collider 때문에 cloth가 장면 밖으로 튀지 않는다.
- dynamic primitive가 current-only로 처리됐는지 또는 teleport/skip됐는지 editor debug에서 확인할 수 있다.
- dynamic collision off 상태에서는 Phase 1 static 결과가 유지된다.

## Phase 3: Full Editor Preview And Debug UX

### Goal

World collision 최소 debug view를 정식 Mesh Editor workflow로 정리한다.

### Tasks

- Mesh Editor preview에서 world static/dynamic collision enable flags를 노출한다.
- gathered/selected/uploaded/skipped/rejected/truncated primitive count 표시를 source/type별로 정리한다.
- world collision 후보 bounds, selected candidate, truncated candidate, rejected candidate 표시를 필터링할 수 있게 한다.
- reset cloth simulation control과 world collision option 변경 시 reset 정책을 연결한다.

### Validation

- budget truncation이 발생하면 어떤 world source가 잘렸는지 보인다.
- world collision option 변경이 asset dirty와 preview-only runtime state를 혼동하지 않는다.
- static/dynamic debug state가 current-only dynamic policy와 일치한다.

## Risks

- world collision 전용 query/snapshot API가 debug snapshot과 같은 shape transform 보정 규칙을 공유하지 않으면 debug draw와 cloth collision 위치가 어긋날 수 있다.
- world collision이 최신 physics step snapshot을 읽는지, 이전 publish snapshot을 읽는지에 따라 moving collider와 cloth 사이에 1-frame latency가 생길 수 있다.
- world dynamic collider add/remove가 매 frame 크게 바뀌면 cloth popping이 발생한다.
- non-uniform scale을 조용히 받아들이면 sphere/capsule radius와 box plane이 실제 physics shape와 달라진다.
- `FClothCollisionCandidate` population에서 source id와 skip/truncate reason을 누락하면 debug view에서 어떤 collider가 실제로 업로드됐는지 설명할 수 없다.
- all-sections shared collision set을 그대로 쓰면 여러 cloth section이 있는 mesh에서 먼 section 때문에 가까운 collider가 budget에서 밀릴 수 있다. 이 계획은 section별 scoring/truncation으로 이를 방지한다.

## Recommended First Implementation Slice

첫 구현은 Phase 0과 Phase 1만 묶는다.

- cloth collision 전용 world shape query/snapshot API 추가
- `FClothCollisionGatherer` world static source 추가
- cloth section config에 world static collision enable 추가
- component broad bounds + section scoring bounds 계산
- blocking WorldStatic shape만 candidate로 변환
- selected world static primitive upload
- selected/truncated/rejected debug overlay

이 slice는 dynamic world collision과 moving overload를 건드리지 않고도 world collision query API, section bounds scoring, debug view를 검증할 수 있다.
