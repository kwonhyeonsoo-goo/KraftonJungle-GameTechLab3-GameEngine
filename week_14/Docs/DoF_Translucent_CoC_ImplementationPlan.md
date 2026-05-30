# DoF / Translucent CoC Implementation Plan

## Context

현재 엔진의 반투명 렌더링은 `Translucent` 패스에서 `DepthReadOnly` 상태로 수행된다. 반투명 물체는 depth buffer를 갱신하지 않으므로, 일반적인 depth 기반 DoF post-process만으로는 반투명 물체가 초점 영역에 있는지 판단하기 어렵다.

이번 계획의 기본 타협안은 다음과 같다.

- Opaque는 기존 depth buffer를 기반으로 CoC를 계산한다.
- Opaque pass의 MRT 출력에는 CoC target을 추가하지 않는다.
- 별도 setup pass에서 depth를 읽어 CoC RT를 채운다.
- 반투명은 depth buffer를 쓰지 않고, 선택적으로 CoC/coverage 보정만 수행한다.
- `force depth` 방식은 채택하지 않는다.

최종 목표 패스 배치는 다음을 기준으로 한다.

```text
Opaque/Decal
Fog
DoFSetup      // depth -> opaque CoC
Translucent   // optional translucent CoC/coverage correction
DoFComposite  // scene color + CoC
PostProcess/FXAA/Gamma/UI/Editor overlays
```

`DoFSetup`은 translucent CoC 보정을 가능하게 하려면 `Translucent`보다 앞에 있어야 한다. 단순 opaque DoF만 구현하는 초기 Phase에서는 DoF 직전에 두어도 동작하지만, 이후 재배치를 줄이기 위해 처음부터 `Fog`와 `Translucent` 사이에 두는 것을 권장한다.

## Goals

- Depth 기반 `DoFSetupPass`와 CoC render target을 추가한다.
- Opaque 기준 DoF를 먼저 구현하고 빌드/화면 검증 가능한 상태로 만든다.
- Translucent CoC를 붙이기 전에 opaque DoF의 depth discontinuity artifact를 줄인다.
- Foreground/background blur를 분리해서 focus object silhouette가 뒤쪽 blur에 오염되지 않도록 한다.
- CoC debug 경로를 제공해서 focus distance/range 계산을 검증한다.
- 이후 `UberTranslucent` 분리를 통해 surface translucent 전용 shader path를 만든다.
- 선택된 translucent material만 CoC buffer를 보정하도록 확장한다.
- 모든 Phase는 `ReleaseBuild.bat < NUL` 기준으로 빌드 검증 가능해야 한다.

## Plan Boundary

이 문서는 DoF 품질, CoC encoding, translucent CoC 참여 정책만 다룬다.

렌더 패스 enum 정리, 패스 실행 순서 재배치, overlay/UI가 DoF 전후 어디에 위치해야 하는지 같은 구조 변경은 별도 문서인 `Docs/RenderPass_DoF_RefactoringPlan.md`에서 추적한다. DoF 구현 중 해당 구조 변경이 필요하면 이 문서에는 요구사항만 남기고, 실제 패스 리팩터링 작업 항목은 별도 계획서로 보낸다.

## No Goals

- 물리적으로 정확한 per-layer translucent bokeh는 구현하지 않는다.
- OIT, per-translucent-object blur accumulation, triangle-level translucent sorting은 하지 않는다.
- 첫 단계에서 `UberLit` 전체 해체를 완료하려고 하지 않는다.
- Opaque pass의 `UberPS_Output`에 CoC MRT 출력을 추가하지 않는다.
- 반투명 material의 depth write를 강제하지 않는다.
- 기존 depth buffer 의미를 바꾸지 않는다.

## Phase 1: DoF Resource and Setup Pass Skeleton

### Goal

CoC RT와 `DoFSetupPass`의 최소 골격을 추가한다. 이 Phase의 목적은 DoF 데이터 경로를 먼저 만들고, depth를 읽어서 CoC 값을 생성할 수 있음을 검증하는 것이다.

### Tasks

1. Viewport 또는 render resource 계층에 `CoCTexture`, `CoCRTV`, `CoCSRV`를 추가한다.
2. viewport resize 시 CoC RT도 함께 재생성되도록 한다.
3. `FFrameContext::SetViewportInfo()`와 `ClearViewportResources()`에 CoC D3D 포인터를 연결한다.
4. camera/post-process 파라미터 구조에 DoF enable, focus distance, focus range 또는 aperture scale을 추가한다.
5. `DoFSetupPass`를 render pass registry에 등록한다.
6. `DoFSetupPass`는 depth copy 갱신을 자기 책임으로 수행한다. Fog pass의 depth copy side effect에 의존하지 않는다.
7. `Shaders/PostProcess/DoFSetup.hlsl`을 추가하고 `ShaderManager`의 shader path/lookup에 등록한다.
8. 새 C++/HLSL 파일은 `KraftonEngine.vcxproj`와 `.filters`에도 반영한다.
9. 첫 출력은 grayscale CoC debug 값으로 단순화한다.

### Validation

- `ReleaseBuild.bat < NUL`
- DoF off 상태에서 기존 화면 변화가 없어야 한다.
- 임시 debug view 또는 blit으로 CoC RT가 depth에 따라 달라지는지 확인한다.

### No Goal

- blur 적용은 하지 않는다.
- translucent 반영은 하지 않는다.
- material/shader routing 변경은 하지 않는다.

## Phase 2: Opaque DoF Composite

### Goal

SceneColor와 CoC RT를 사용해서 opaque 기준 DoF blur/composite를 구현한다.

### Tasks

1. `DoFPass`를 추가하거나 기존 post-process 체인에 DoF composite pass를 추가한다.
2. DoF pass는 SceneColor를 읽으면서 같은 RTV에 쓰지 않는다. 기존 FXAA/Gamma처럼 `SceneColorCopyTexture`에 복사한 뒤 SRV로 읽거나, 별도 ping-pong RT를 사용한다.
3. Pass 종료 시 DoF가 바인딩한 SceneColor/CoC SRV를 명시적으로 unbind해서 이후 RTV 바인딩 hazard를 피한다.
4. 초기 구현은 full-res small kernel 또는 간단한 separable blur로 시작한다.
5. CoC 값 기반 near/far blur 강도를 계산한다.
6. editor/game camera 설정에서 focus distance를 조정할 수 있는 최소 입력 경로를 만든다.
7. DoF on/off 토글을 추가한다.

### Validation

- `ReleaseBuild.bat < NUL`
- opaque 물체 기준으로 focus plane 근처는 선명하고 앞/뒤는 blur되어야 한다.
- DoF off 상태에서 기존 post-process 결과와 동일해야 한다.

### No Goal

- 고품질 bokeh shape 구현은 하지 않는다.
- translucent 물체의 별도 CoC 처리는 하지 않는다.
- `UberLit` 해체는 하지 않는다.
- editor line/gizmo/UI가 DoF에 포함될지 여부를 이 Phase에서 최종 해결하지 않는다. 기본은 DoF를 scene geometry post-process로 보고 UI/overlay는 DoF 이후에 두는 방향이다.

## Phase 2.5: Opaque DoF Stability - Foreground/Background Split

### Problem

초점 물체 A는 내부가 선명하지만, A 뒤쪽 배경 blur가 A의 silhouette 근처에 섞이면 외곽선만 초점이 어긋난 것처럼 보인다. 이는 current single-layer full-res gather blur가 depth discontinuity를 존중하지 않기 때문이다.

이 Phase는 translucent CoC를 붙이기 전에 opaque DoF 자체를 안정화하는 단계다.

### Goal

Signed CoC와 foreground/background 분리 blur를 도입해 focus object silhouette bleeding을 줄인다. A가 focus plane에 있을 때 A의 edge가 뒤쪽 background blur에 오염되지 않아야 한다.

### Proposed Rendering Model

```text
DoFSetup
  depth -> signed CoC
  negative: foreground, positive: background, near zero: focus

DoFBackgroundBlur
  scene color + depth + positive CoC
  far/background blur only
  nearer/focus samples reject or strongly down-weight

DoFForegroundBlur
  scene color + depth + negative CoC
  foreground blur/mask only
  optional dilation to avoid thin foreground holes

DoFComposite
  sharp scene + background blur + foreground blur + signed CoC
  focus pixels prefer sharp scene
  foreground blur overlays background when foreground CoC is significant
```

초기 구현은 고품질 bokeh가 아니라 안정적인 edge 정책을 우선한다. pass 수는 늘어나도 된다. 단, 패스 enum/실행 순서 정리 자체는 `RenderPass_DoF_RefactoringPlan.md`의 범위다.

### Tasks

1. CoC encoding을 signed로 변경한다.
   - `0`: focus plane
   - `> 0`: background/far blur
   - `< 0`: foreground/near blur
   - 권장 포맷은 우선 `R16_FLOAT` 유지
2. `DoFSetup.hlsl`에서 focus distance 대비 signed distance를 계산한다.
3. `DoFComposite.hlsl`의 단일 abs-CoC blur를 제거하고, background/foreground 경로로 분리한다.
4. background blur에서 sample depth가 current pixel보다 더 가까운 경우 contribution을 줄인다.
5. focus 또는 foreground object pixel은 background blur 결과를 직접 받지 않도록 한다.
6. foreground blur는 별도 mask/blur로 계산하고 composite에서 sharp/background 위에 합성한다.
7. 필요한 중간 RT를 추가한다.
   - 최소 후보: `DoFBackgroundTexture`, `DoFForegroundTexture`
   - foreground alpha/mask가 필요하면 `R16_FLOAT` 또는 `RG16_FLOAT` 후보를 검토
8. SceneColor를 SRV로 읽고 같은 RTV에 쓰는 hazard를 피하기 위해 copy 또는 ping-pong 규칙을 명시한다.
9. CoC debug view에서 signed CoC를 확인할 수 있게 한다.
   - background: 한 색상 계열
   - foreground: 다른 색상 계열
   - focus: black 또는 neutral
10. DoF off 상태에서는 기존 post-process 결과와 동일해야 한다.

### Acceptance Criteria

- focus object A 내부와 silhouette가 모두 선명하게 유지된다.
- A 뒤쪽 background는 blur되지만 A edge 안쪽으로 번지지 않는다.
- A 앞쪽 foreground object는 focus plane과 분리되어 blur된다.
- foreground object 주변에 명백한 unblurred halo가 없어야 한다.
- sky/background depth가 없는 영역의 signed CoC 정책이 debug view에서 예측 가능하다.
- `ReleaseBuild.bat < NUL` 통과.

### No Goal

- physically correct multi-layer DoF는 하지 않는다.
- polygonal bokeh shape, aperture blade, high quality scatter bokeh는 하지 않는다.
- translucent CoC write는 하지 않는다.
- render pass framework 전반을 정리하지 않는다. 필요한 구조 변경은 별도 계획서로 분리한다.

## Phase 3: CoC Debug and Encoding Policy

### Goal

Translucent 보정 작업 전에 CoC 데이터가 신뢰 가능한지 확인할 수 있는 debug/정책 경로를 정리한다.

### Tasks

1. CoC RT debug view mode 또는 post-process debug output을 추가한다.
2. focus distance/range 값이 화면에서 예측 가능하게 보이도록 조정한다.
3. Phase 2.5에서 도입한 signed CoC 정책을 문서화하고 debug view와 일치시킨다.
4. CoC RT 포맷을 확정한다. opaque signed CoC에는 우선 `R16_FLOAT`를 유지한다.
5. sky/background처럼 depth가 없는 영역의 CoC 정책을 확정한다.
6. Translucent CoC 보정 단계에서 `R16_FLOAT`만으로 충분한지 재검토한다. Opaque-only CoC에는 `R16_FLOAT`가 적합하지만, alpha/coverage까지 고정함수 blend로 합성하려면 별도 coverage RT 또는 `RG/RGBA` 계열 포맷이 필요할 수 있다.

### Validation

- `ReleaseBuild.bat < NUL`
- CoC debug 화면에서 focus plane 값이 0 근처여야 한다.
- foreground/background 영역의 CoC 부호와 강도가 예측 가능해야 한다.

### No Goal

- translucent CoC write는 하지 않는다.
- `UberLit` 해체는 하지 않는다.

## Phase 4: UberTranslucent Split

### Goal

Surface translucent mesh가 `UberLit`에서 분리된 전용 shader path를 타게 한다. 이 Phase는 DoF translucent 보정을 위한 선행 리팩터이며, opaque DoF 기능 자체의 필수 조건은 아니다.

### Tasks

1. `Shaders/Geometry/UberTranslucent.hlsl`을 추가한다.
2. static/skeletal vertex factory 입력을 지원한다.
3. 기존 `UberLit`의 translucent-only `FORWARD_FOG` 로직을 `UberTranslucent`로 이동한다.
4. `ShaderManager`에 `UberTranslucent` shader path와 필요한 permutation/entry point 정책을 추가한다.
5. `DrawCommandBuilder::ResolveSectionShader()`에서 `SecPass == ERenderPass::Translucent`인 surface mesh는 `UberTranslucent`를 선택하도록 한다.
6. `UberTranslucent`는 우선 color alpha만 출력한다.
7. Normal/Culling MRT 출력은 하지 않거나 명확한 no-op 정책으로 둔다.
8. particle shader는 이 Phase에서 변경하지 않는다.
9. 새 shader 파일은 `KraftonEngine.vcxproj`와 `.filters`에도 반영한다.

### Validation

- `ReleaseBuild.bat < NUL`
- 기존 translucent surface material 렌더링이 유지되어야 한다.
- fog 뒤에 그려지는 translucent self-fog 동작이 유지되어야 한다.
- opaque/debug view mode가 깨지지 않아야 한다.

### No Goal

- Debug viewmode 전체 분리는 하지 않는다.
- particle shader 통합은 하지 않는다.
- CoC write 연결은 하지 않는다.

## Phase 5: Surface Translucent CoC MVP

### Goal

선택된 surface translucent material만 CoC RT를 보정하도록 한다. Depth buffer는 계속 write하지 않는다.

### Tasks

1. material에 `DoFParticipation` 또는 `bWriteTranslucentCoC` 플래그를 추가한다.
2. material serialization, material instance override, editor material inspector에 해당 플래그를 반영한다.
3. `TranslucentPass`에서 CoC RTV를 함께 바인딩할지, 별도 `TranslucentCoCPass`를 둘지 결정한다.
4. 현재 `BlendStateManager`는 `IndependentBlendEnable = FALSE`이고 `RenderTarget[0]` 중심의 blend state만 제공한다. MRT로 SceneColor와 CoC를 동시에 갱신할 경우 CoC target의 blend/coverage 정책을 별도로 설계해야 한다.
5. 첫 구현은 `UberTranslucent`만 CoC write를 지원한다.
6. CoC write는 alpha/coverage 기반으로 제한한다.
7. DoF composite에서 opaque CoC와 translucent CoC를 합성한다.

### Recommended Policy

```text
finalCoC = lerp(opaqueCoC, translucentCoC, translucentCoverage)
```

위 식은 목표 의미다. 실제 구현은 다음 중 하나로 결정한다.

- CoC/coverage를 저장할 별도 RT 또는 `RG/RGBA` 포맷을 사용하고, pass에서 명시적으로 합성한다.
- `TranslucentCoCPass`에서 opaque CoC SRV를 읽고 별도 CoC output RT에 쓴다. 같은 텍스처를 SRV와 RTV로 동시에 바인딩하지 않도록 ping-pong한다.
- MRT 동시 갱신을 선택한다면 `BlendStateManager`에 CoC target에 맞는 blend state를 추가하고, 필요 시 independent blend를 지원한다.

`R16_FLOAT` 단일 CoC RT에 단순히 alpha blend를 기대하는 방식은 coverage 정보가 부족할 수 있으므로, Phase 5 착수 전에 확정한다.

### Validation

- `ReleaseBuild.bat < NUL`
- material flag off 상태에서는 기존 DoF와 동일해야 한다.
- material flag on 상태에서는 해당 translucent가 focus 근처일 때 과도하게 blur되지 않아야 한다.
- 뒤쪽 opaque depth가 반투명 depth로 오염되지 않아야 한다.

### No Goal

- 여러 겹 translucent의 정확한 layer 처리는 하지 않는다.
- particle 전체 타입 지원은 하지 않는다.
- per-object bokeh는 구현하지 않는다.

## Phase 6: Particle Translucent CoC Extension

### Goal

Sprite/Mesh/BeamTrail particle shader에도 선택적 CoC 보정을 추가한다.

### Tasks

1. particle material flag를 읽어 CoC 참여 여부를 결정한다.
2. `Shaders/Particle/Sprite.hlsl`, `Shaders/Particle/Mesh.hlsl`, `Shaders/Particle/BeamTrail.hlsl`에 optional CoC output을 추가한다.
3. Sprite/Mesh는 instance world position 기준으로 CoC를 계산한다.
4. Beam/Ribbon은 vertex world position 또는 interpolated position 기준으로 근사한다.
5. Additive particle은 기본 off로 두고, alpha-blend particle부터 지원한다.

### Validation

- `ReleaseBuild.bat < NUL`
- alpha particle은 focus plane 근처에서 선명도 보정이 가능해야 한다.
- additive 효과는 기본값에서 기존 비주얼을 유지해야 한다.

### No Goal

- particle 내부 per-pixel correct depth ordering은 하지 않는다.
- emitter 간 완전한 DoF interleave는 하지 않는다.

## Recommended Commit Boundaries

1. `DoFSetupPass + CoC RT`
2. `Opaque DoF composite`
3. `Opaque DoF stability - foreground/background split`
4. `CoC debug/view controls`
5. `UberTranslucent split`
6. `Surface translucent CoC write`
7. `Particle CoC write`

## Implementation Notes

- `force depth`는 구현이 쉽지만 depth buffer의 의미를 깨뜨리므로 제외한다.
- Opaque pass에 CoC MRT를 추가하면 모든 opaque draw가 불필요한 target을 부담하므로 제외한다.
- CoC RT는 DoF가 켜진 경우에만 준비/사용하는 방향이 바람직하다.
- `UberTranslucent` 분리는 최종 구조상 맞지만, DoF 데이터 경로 검증과 동시에 진행하면 원인 분리가 어려워진다.
- 따라서 먼저 depth 기반 CoC 생성과 opaque DoF를 닫고, 이후 translucent shader path를 분리해 CoC 보정을 연결한다.
- Focus object silhouette bleeding은 kernel 크기만 줄여서는 해결하지 않는다. signed CoC와 depth-aware foreground/background 분리로 해결한다.
- D3D11에서 같은 리소스를 SRV와 RTV로 동시에 사용하는 경로는 피한다. SceneColor와 CoC 모두 copy 또는 ping-pong 정책을 명시한다.
- Editor overlay, gizmo, UI는 기본적으로 DoF 이후에 그리는 방향을 유지한다. 기존 `EditorLines`처럼 현재 post-process 이전에 있는 요소는 DoF 적용 여부를 별도 정책으로 판단한다.
