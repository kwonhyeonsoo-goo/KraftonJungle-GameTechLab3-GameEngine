local GameState = require("Management/GameState")
local MainState = require("Management/States/MainState")
local PreInGameState = require("Management/States/PreInGameState")
local LoadingState = require("Management/States/LoadingState")
local InGameState = require("Management/States/InGameState")

local SceneManager = {}
SceneManager.__index = SceneManager

local DEFAULT_START_STATE = GameState.Main

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[SceneManager] " .. message)
    else
        print("[SceneManager] " .. message)
    end
end

local function default_huds()
    return {}
end

function SceneManager.new(general)
    local self = setmetatable({
        general = general,
        current = nil,
        pending = nil,
        guard = nil,
        start_state = DEFAULT_START_STATE,
        hud_by_state = default_huds(),
        states_by_id = {}
    }, SceneManager)

    self:RegisterState(GameState.Main, MainState.new(general))
    self:RegisterState(GameState.PreInGame, PreInGameState.new(general))
    self:RegisterState(GameState.Loading, LoadingState.new(general))
    self:RegisterState(GameState.InGame, InGameState.new(general))
    return self
end

function SceneManager:Initialize()
    self.current = self:ResolveStartState(self.start_state)

    local scene_path = ""
    if Scene ~= nil and Scene.GetCurrentPath ~= nil then
        scene_path = tostring(Scene.GetCurrentPath())
    end
    log("initialized state=" .. tostring(self.current) .. " scene=" .. scene_path)

    self:EnterState(nil, self.current, { reason = "initial" })
    self.general:Publish("scene.initialized", { state = self.current })
    self:EmitEntered(nil, self.current, { reason = "initial" })
end

function SceneManager:Shutdown()
    self.pending = nil
    self:ExitState(self.current, nil, { reason = "shutdown" })
    self.general:Publish("scene.hud_requested", { state = nil, hud = nil, reason = "shutdown" })
end

function SceneManager:SetStartState(state)
    if not self:IsRegisteredState(state) then
        log("ignored unavailable start state=" .. tostring(state))
        return false
    end
    self.start_state = state
    return true
end

function SceneManager:RegisterState(state, state_object)
    if not GameState.IsValid(state) then
        print("[SceneManager] invalid registered state: " .. tostring(state))
        return false
    end

    self.states_by_id[state] = state_object
    return true
end

function SceneManager:GetStateObject(state)
    return self.states_by_id[state]
end

function SceneManager:IsRegisteredState(state)
    return GameState.IsValid(state) and self:GetStateObject(state) ~= nil
end

function SceneManager:ResolveStartState(state)
    if self:IsRegisteredState(state) then
        return state
    end

    if state ~= nil then
        log("fallback start state " .. tostring(state) .. " -> " .. tostring(DEFAULT_START_STATE))
    end
    return DEFAULT_START_STATE
end

function SceneManager:EnterState(from, state, payload)
    local state_object = self:GetStateObject(state)
    if state_object ~= nil and state_object.Enter ~= nil then
        state_object:Enter(payload or {}, from)
    end
end

function SceneManager:ExitState(state, next_state, payload)
    local state_object = self:GetStateObject(state)
    if state_object ~= nil and state_object.Exit ~= nil then
        state_object:Exit(payload or {}, next_state)
    end
end

function SceneManager:SetTransitionGuard(callback)
    self.guard = callback
end

function SceneManager:RegisterHUD(state, hud)
    if not GameState.IsValid(state) then
        print("[SceneManager] invalid HUD state: " .. tostring(state))
        return false
    end

    if hud == nil then
        self.hud_by_state[state] = false
    elseif type(hud) == "table" then
        self.hud_by_state[state] = hud
    else
        print("[SceneManager] invalid HUD payload for state: " .. tostring(state))
        return false
    end

    if self.current == state then
        self:PublishHUDForState(state, { reason = "hud_registered" })
    end
    return true
end

function SceneManager:GetState()
    return self.current
end

function SceneManager:GetHUDForState(state)
    local override = self.hud_by_state[state]
    if override ~= nil then
        if override == false then
            return nil
        end
        return override
    end

    local state_object = self:GetStateObject(state)
    if state_object ~= nil and state_object.GetHUD ~= nil then
        return state_object:GetHUD()
    end
    return nil
end

function SceneManager:RequestState(next_state, payload)
    if not self:IsRegisteredState(next_state) then
        print("[SceneManager] unavailable state: " .. tostring(next_state))
        return false
    end

    self.pending = {
        to = next_state,
        payload = payload or {}
    }
    log("queued transition " .. tostring(self.current) .. " -> " .. tostring(next_state))
    return true
end

function SceneManager:Tick(dt)
    if self.pending ~= nil then
        local request = self.pending
        self.pending = nil
        self:ApplyState(request.to, request.payload)
    end

    local state_object = self:GetStateObject(self.current)
    if state_object ~= nil and state_object.Tick ~= nil then
        state_object:Tick(dt or 0.0)
    end
end

function SceneManager:PublishHUDForState(state, payload)
    self.general:Publish("scene.hud_requested", {
        state = state,
        hud = self:GetHUDForState(state),
        payload = payload or {}
    })
end

function SceneManager:EmitEntered(from, next_state, payload)
    self.general:Publish("scene.entered", {
        from = from,
        to = next_state,
        payload = payload,
        settings = payload and payload.settings
    })
    self:PublishHUDForState(next_state, payload)
end

function SceneManager:ApplyState(next_state, payload)
    local from = self.current
    if from == next_state then
        self:PublishHUDForState(next_state, payload)
        return true
    end

    if self.guard ~= nil then
        local ok, allowed = pcall(self.guard, from, next_state, payload)
        if not ok or allowed == false then
            self.general:Publish("scene.rejected", { from = from, to = next_state, payload = payload })
            return false
        end
    end

    self.general:Publish("scene.exiting", { from = from, to = next_state, payload = payload })
    self:ExitState(from, next_state, payload)

    self.current = next_state
    log("apply transition " .. tostring(from) .. " -> " .. tostring(next_state))
    self.general:Publish("scene.changed", { from = from, to = next_state, payload = payload })
    self:EnterState(from, next_state, payload)
    self:EmitEntered(from, next_state, payload)
    return true
end

return SceneManager
