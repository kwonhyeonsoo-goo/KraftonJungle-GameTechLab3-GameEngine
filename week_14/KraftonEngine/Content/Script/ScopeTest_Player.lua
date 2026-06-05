local scoped = false

local scope_radius = 0.42
local scope_outer_blur = 4.0
local scope_zoom_fov = 0.30
local scope_feather = 0.08
local scope_edge_blur = 1.5
local scope_intensity = 1.0
local scope_look_sensitivity_scale = 0.275
local scope_blend_time = 0.08

local function set_scope_enabled(enabled)
    scoped = enabled
    CameraManager.SetScopeZoomEnabled(scoped)
end

function BeginPlay()
    print("[ScopeTest] BeginPlay " .. obj.UUID)

    Input.SetInputModeGameOnly()
    Input.SetMouseCaptured(true)
    CameraManager.SetScopeLensProfile(
        scope_radius,
        scope_outer_blur,
        scope_zoom_fov,
        scope_feather,
        scope_edge_blur,
        scope_intensity,
        scope_look_sensitivity_scale,
        scope_blend_time
    )
    CameraManager.SetScopeZoomEnabled(false)
end

function EndPlay()
    CameraManager.ClearScopeLens()
    Input.ReleaseMouseCapture()
end

function Tick(dt)
    if Input.GetKeyDown("RightMouseButton") then
        set_scope_enabled(not scoped)
    end
end
