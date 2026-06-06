local GameState = require("Management/GameState")

local LoadingState = {}
LoadingState.__index = LoadingState

local SCENE_PATH = "Content/Scene/Loading.Scene"
local DEFAULT_TARGET_STATE = GameState.InGame
local DEFAULT_TARGET_SCENE = "Content/Scene/ScopeTest.Scene"
local DEFAULT_TIP = "Tip: Hold your breath only when the shot really matters."
local HUD = {
    name = "LoadingHUD",
    path = "Content/UI/LoadingHUD.rml",
    z_order = 10,
    mode = "Loading"
}

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[LoadingState] " .. message)
    else
        print("[LoadingState] " .. message)
    end
end

local function is_current_scene(path)
    if Scene == nil or Scene.GetCurrentPath == nil then
        return false
    end
    local current = string.lower(string.gsub(tostring(Scene.GetCurrentPath()), "\\", "/"))
    local target = string.lower(string.gsub(tostring(path), "\\", "/"))
    return current == target or string.sub(current, -string.len(target)) == target
end

local function transition_to_scene(path)
    if Scene == nil or path == nil or path == "" then
        log("transition skipped: Scene API or path unavailable")
        return false
    end
    if Scene.TransitionTo ~= nil then
        log("Scene.TransitionTo begin path=" .. tostring(path))
        local ok = Scene.TransitionTo(path)
        log("Scene.TransitionTo result=" .. tostring(ok) .. " path=" .. tostring(path))
        return ok
    end
    if Scene.Open ~= nil then
        log("Scene.Open fallback begin path=" .. tostring(path))
        Scene.Open(path)
        log("Scene.Open fallback completed path=" .. tostring(path))
        return true
    end
    log("transition skipped: no Scene.TransitionTo/Open function")
    return false
end

function LoadingState.new(general)
    return setmetatable({
        general = general,
        elapsed = 0.0,
        ready = false,
        loading_duration = 3.0,
        target_state = DEFAULT_TARGET_STATE,
        target_scene = DEFAULT_TARGET_SCENE,
        tip = DEFAULT_TIP,
        transition_requested = false
    }, LoadingState)
end

function LoadingState:GetHUD()
    return HUD
end

function LoadingState:Enter(payload)
    payload = payload or {}
    log("Enter reason=" .. tostring(payload.reason) .. " target_state=" .. tostring(payload.target_state) ..
        " target_scene=" .. tostring(payload.target_scene))
    self.elapsed = 0.0
    self.ready = false
    self.transition_requested = false
    self.loading_duration = payload.loading_duration or 3.0
    self.target_state = payload.target_state or DEFAULT_TARGET_STATE
    self.target_scene = payload.target_scene or DEFAULT_TARGET_SCENE
    self.tip = payload.tip or DEFAULT_TIP

    if not is_current_scene(SCENE_PATH) then
        transition_to_scene(SCENE_PATH)
    else
        log("Scene already current; no transition requested")
    end

    self.general:Publish("loading.started", {
        duration = self.loading_duration,
        target_state = self.target_state,
        target_scene = self.target_scene,
        tip = self.tip
    })
end

function LoadingState:Exit()
    self.elapsed = 0.0
    self.ready = false
    self.transition_requested = false
end

function LoadingState:Tick(dt)
    if self.transition_requested then
        return
    end

    self.elapsed = self.elapsed + (dt or 0.0)
    if not self.ready and self.elapsed >= self.loading_duration then
        self.ready = true
        log("Loading duration complete; Space enabled")
        self.general:Publish("loading.ready", {
            target_state = self.target_state,
            target_scene = self.target_scene,
            tip = self.tip
        })
    end

    if self.ready and Input ~= nil and Input.GetKeyDown ~= nil and Input.GetKeyDown("Space") then
        self.transition_requested = true
        log("Space accepted; requesting target_state=" .. tostring(self.target_state))
        if self.target_state ~= nil and self.general ~= nil and self.general.RequestState ~= nil then
            self.general:RequestState(self.target_state, {
                reason = "loading_completed",
                target_scene = self.target_scene
            })
        else
            transition_to_scene(self.target_scene)
        end
    end
end

return LoadingState
