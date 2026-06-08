local GameState = require("Management/GameState")

local InGameManager = {}
InGameManager.__index = InGameManager

local ENEMY_KILL_SCORE = 100
local FRIENDLY_KILL_PENALTY = 150
local HEADSHOT_BONUS = 50
local PENETRATION_BONUS = 25
local DEFAULT_HIT_SCORE = 5

local PAUSE_CAMERA_NAME = "PauseMenu_Camera"
local PAUSE_RIFLE_NAME = "PauseMenu_Rifle"
local PAUSE_FADE_TIME = 0.080
local PAUSE_BLEND_TIME = 0.120
local PAUSE_TOGGLE_KEY_CODE = 81
local DEFAULT_MATCH_DURATION = 300.0

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[InGameManager] " .. message)
    else
        print("[InGameManager] " .. message)
    end
end

local function get_hit_region_score(hit)
    if hit == nil then
        return DEFAULT_HIT_SCORE
    end

    if SniperHitRegion ~= nil then
        if hit.HitRegion == SniperHitRegion.Head then
            return 50
        end
        if hit.HitRegion == SniperHitRegion.Torso then
            return 20
        end
        if hit.HitRegion == SniperHitRegion.Arm or hit.HitRegion == SniperHitRegion.Leg then
            return 10
        end
    end

    local regionName = hit.HitRegionName or ""
    if regionName == "Head" then
        return 50
    end
    if regionName == "Torso" then
        return 20
    end
    if regionName == "Arm" or regionName == "Leg" then
        return 10
    end
    return DEFAULT_HIT_SCORE
end

local function get_hit_body_name(hit)
    if hit == nil then
        return ""
    end
    if hit.HitBodyName ~= nil and hit.HitBodyName ~= "" then
        return hit.HitBodyName
    end
    if hit.HitBoneNameString ~= nil then
        return hit.HitBoneNameString
    end
    return ""
end

local function get_hit_region_name(hit)
    if hit == nil then
        return "Unknown"
    end
    if hit.HitRegionName ~= nil and hit.HitRegionName ~= "" then
        return hit.HitRegionName
    end
    return "Unknown"
end

local function get_hit_region_display_name(hit)
    if hit == nil then
        return "UNKNOWN"
    end
    if hit.HitRegionDisplayName ~= nil and hit.HitRegionDisplayName ~= "" then
        return hit.HitRegionDisplayName
    end
    return string.upper(get_hit_region_name(hit))
end

local function calculate_hit_score_delta(hit, isFriendly)
    if hit == nil then
        return 0
    end

    local rawScore = tonumber(hit.HitScoreValue)
    local score = 0
    if rawScore ~= nil and rawScore > 0 then
        score = math.floor(rawScore + 0.5)
    else
        local multiplier = tonumber(hit.HitScoreMultiplier or hit.RegionDamageMultiplier) or 1.0
        score = math.floor(get_hit_region_score(hit) * math.max(0.0, multiplier) + 0.5)
    end

    if isFriendly then
        return -score
    end
    return score
end

function InGameManager.new(general)
    return setmetatable({
        general = general,
        running = false,
        timer = 0.0,
        match_duration = DEFAULT_MATCH_DURATION,
        phase = "Idle",
        wave = 0,
        settings = {},
        last_timer_second = -1,
        result_requested = false,
        sniper_kills = 0,
        friendly_fire_kills = 0,
        paused = false,
        pause_transition = nil,
        pause_transition_time = 0.0,
        pause_previous_view_target = nil,
        pause_camera = nil,
        pause_rifle = nil,
        pause_rifle_sequence = nil
    }, InGameManager)
end

function InGameManager:Initialize()
    self.general:Subscribe("scene.entered", self, function(payload)
        if payload ~= nil and payload.to == "InGame" then
            self:Start(payload.settings)
        end
    end)

    self.general:Subscribe("scene.exiting", self, function(payload)
        if payload ~= nil and payload.from == "InGame" then
            local reason = payload.reason
            if reason == nil and type(payload.payload) == "table" then
                reason = payload.payload.reason
            end
            self:Stop(reason)
        end
    end)

    self.general:Subscribe("sniper.target_damaged", self, function(payload)
        if not self.running then
            return
        end

        local hit = payload ~= nil and payload.hit or nil
        local isFriendly = payload ~= nil and payload.friendly == true
        local scoreDelta = calculate_hit_score_delta(hit, isFriendly)
        if scoreDelta ~= 0 then
            self.general:AddScore(scoreDelta)
            if hit ~= nil then
                pcall(function()
                    hit.HitScoreValue = math.abs(scoreDelta)
                end)
            end
        end

        self.general:Publish("ingame.sniper_damaged", {
            timer = self.timer,
            wave = self.wave,
            phase = self.phase,
            payload = payload,
            score_delta = scoreDelta,
            total_score = self.general:GetScore(),
            hit_body_name = get_hit_body_name(hit),
            hit_region_name = get_hit_region_name(hit),
            hit_region_display_name = get_hit_region_display_name(hit)
        })

        self.general:Publish("ingame.sniper_hit_scored", {
            timer = self.timer,
            wave = self.wave,
            phase = self.phase,
            payload = payload,
            score_delta = scoreDelta,
            total_score = self.general:GetScore(),
            hit_body_name = get_hit_body_name(hit),
            hit_region_name = get_hit_region_name(hit),
            hit_region_display_name = get_hit_region_display_name(hit)
        })
    end)

    self.general:Subscribe("sniper.target_killed", self, function(payload)
        if not self.running then
            return
        end

        local hit = payload ~= nil and payload.hit or nil
        local isFriendly = payload ~= nil and payload.friendly == true
        local scoreDelta = 0

        if isFriendly then
            self.friendly_fire_kills = self.friendly_fire_kills + 1
            scoreDelta = -FRIENDLY_KILL_PENALTY
        else
            self.sniper_kills = self.sniper_kills + 1
            scoreDelta = ENEMY_KILL_SCORE

            if hit ~= nil and hit.bIsHeadshot == true then
                scoreDelta = scoreDelta + HEADSHOT_BONUS
            end
            if hit ~= nil and hit.HitOutcome == SniperHitOutcome.Penetrated then
                scoreDelta = scoreDelta + PENETRATION_BONUS
            end
        end

        self.general:AddScore(scoreDelta)
        self.general:Publish("ingame.sniper_killed", {
            timer = self.timer,
            wave = self.wave,
            phase = self.phase,
            payload = payload,
            score_delta = scoreDelta,
            total_score = self.general:GetScore(),
            sniper_kills = self.sniper_kills,
            friendly_fire_kills = self.friendly_fire_kills,
            hit_body_name = get_hit_body_name(hit),
            hit_region_name = get_hit_region_name(hit),
            hit_region_display_name = get_hit_region_display_name(hit)
        })
    end)

    self.general:Subscribe("ingame.pause_resume_requested", self, function()
        self:BeginResume()
    end)

    self.general:Subscribe("ingame.pause_toggle_requested", self, function(payload)
        self:TogglePause(payload and payload.reason or "ui")
    end)

    self.general:Subscribe("ingame.pause_main_requested", self, function(payload)
        self:GoToMain(payload and payload.reason or "pause_menu")
    end)

    self.general:Subscribe("general.initialized", self, function()
        self:EnsureRunningForCurrentState("general_initialized")
    end)
end

function InGameManager:Shutdown()
    self.general:UnsubscribeOwner(self)
end

function InGameManager:Start(settings)
    self.running = true
    self.timer = 0.0
    self.match_duration = self:ResolveMatchDuration(settings)
    self.phase = "Wave"
    self.wave = 1
    self.settings = settings or self.settings or {}
    self.last_timer_second = -1
    self.result_requested = false
    self.sniper_kills = 0
    self.friendly_fire_kills = 0
    self.general:SetScore(0)
    self.paused = false
    self.pause_transition = nil
    self.pause_transition_time = 0.0
    self.pause_previous_view_target = nil
    self.pause_camera = nil
    self.pause_rifle = nil
    self.pause_rifle_sequence = nil
    self.general:Publish("ingame.started", self:GetSnapshot())
    self.general:Publish("ingame.timer", self:GetSnapshot())
    self.general:Publish("ingame.pause_changed", { paused = false, reason = "start" })
end

function InGameManager:Stop(reason)
    if not self.running then
        return
    end

    local snapshot = self:GetSnapshot()
    snapshot.reason = reason
    self:EndPausePresentation(false)
    self.running = false
    self.phase = "Idle"
    self.general:Publish("ingame.stopped", snapshot)
end

function InGameManager:Tick(dt)
    dt = dt or 0.0
    self:EnsureRunningForCurrentState("tick")
    if not self.running then
        if self:IsCurrentStateInGame() and self:IsPauseKeyPressed() then
            log("pause key detected while manager was not running; recovering InGame runtime")
            self:Start(self.settings)
            self:TogglePause("key_recovered")
        end
        return
    end

    self:PollPauseInput()
    self:TickPauseTransition(dt)
    if self.paused or self.pause_transition ~= nil then
        return
    end

    self.timer = self.timer + dt
    if self:GetRemainingTime() <= 0.0 then
        self.timer = self.match_duration
        self.general:Publish("ingame.timer", self:GetSnapshot())
        self:RequestVictory("air_support_arrived")
        return
    end

    local second = math.ceil(self:GetRemainingTime())
    if second ~= self.last_timer_second then
        self.last_timer_second = second
        self.general:Publish("ingame.timer", self:GetSnapshot())
    end
end

function InGameManager:ResolveMatchDuration(settings)
    local duration = nil
    if type(settings) == "table" then
        duration = settings.match_duration_seconds or settings.air_support_duration_seconds or settings.duration_seconds
    end
    duration = tonumber(duration) or self.match_duration or DEFAULT_MATCH_DURATION
    if duration <= 0.0 then
        duration = DEFAULT_MATCH_DURATION
    end
    return duration
end

function InGameManager:SetMatchDuration(seconds)
    if self.running then
        return false
    end
    seconds = tonumber(seconds) or DEFAULT_MATCH_DURATION
    self.match_duration = math.max(1.0, seconds)
    return true
end

function InGameManager:GetRemainingTime()
    return math.max(0.0, (self.match_duration or DEFAULT_MATCH_DURATION) - (self.timer or 0.0))
end

function InGameManager:RequestVictory(reason)
    return self:CompleteVictory(reason or "victory_requested")
end

function InGameManager:CompleteVictory(reason)
    if self.result_requested then
        return false
    end

    self.result_requested = true
    local snapshot = self:GetSnapshot()
    snapshot.result = "Victory"
    snapshot.reason = reason or "victory"
    self.general:Publish("ingame.victory_requested", snapshot)
    self.general:Publish("ingame.completed", snapshot)

    if self.general ~= nil and self.general.CommitRun ~= nil then
        self.general:CommitRun({
            nickname = "Player",
            result = "Victory",
            state = "Victory",
            score = self.general.GetScore ~= nil and self.general:GetScore() or 0,
            elapsed_time = snapshot.elapsed_time,
            remaining_time = snapshot.remaining_time,
            reason = snapshot.reason
        })
    end

    self:EndPausePresentation(false)
    self.running = false
    self.phase = "Result"
    local scene_manager = self.general ~= nil and self.general.managers ~= nil and self.general.managers.Scene or nil
    if scene_manager ~= nil and scene_manager.IsRegisteredState ~= nil and
        scene_manager:IsRegisteredState(GameState.Victory) and
        self.general.RequestState ~= nil then
        return self.general:RequestState(GameState.Victory, {
            reason = snapshot.reason,
            result = "Victory",
            elapsed_time = snapshot.elapsed_time
        })
    end

    log("victory completed without state transition; Victory state is not registered yet")
    return true
end

function InGameManager:EnsureRunningForCurrentState(reason)
    if self.running or not self:IsCurrentStateInGame() then
        return
    end

    log("starting InGame runtime from current state reason=" .. tostring(reason))
    self:Start(self.settings)
end

function InGameManager:PollPauseInput()
    if self:IsPauseKeyPressed() then
        self:TogglePause("key")
    end
end

function InGameManager:IsPauseKeyPressed()
    if Input == nil then
        return false
    end

    if Input.WasPausePressed ~= nil then
        return Input.WasPausePressed() == true
    end

    if Input.GetKeyDown == nil then
        return false
    end

    if Input.GetKeyDown("Q") then
        return true
    end
    return Input.GetKeyDown(PAUSE_TOGGLE_KEY_CODE)
end

function InGameManager:IsCurrentStateInGame()
    if self.general == nil or self.general.GetState == nil then
        return false
    end
    return self.general:GetState() == GameState.InGame
end

function InGameManager:FindPauseCamera()
    if self.pause_camera ~= nil then
        return self.pause_camera
    end
    if World ~= nil and World.FindActorByName ~= nil then
        self.pause_camera = World.FindActorByName(PAUSE_CAMERA_NAME)
    end
    return self.pause_camera
end

function InGameManager:FindPauseRifle()
    if self.pause_rifle ~= nil then
        return self.pause_rifle
    end
    if World ~= nil and World.FindActorByName ~= nil then
        self.pause_rifle = World.FindActorByName(PAUSE_RIFLE_NAME)
    end
    return self.pause_rifle
end

function InGameManager:GetPauseRifleSequence()
    if self.pause_rifle_sequence ~= nil then
        return self.pause_rifle_sequence
    end
    local rifle = self:FindPauseRifle()
    if rifle ~= nil and rifle.GetActorSequenceComponent ~= nil then
        self.pause_rifle_sequence = rifle:GetActorSequenceComponent()
    end
    return self.pause_rifle_sequence
end

function InGameManager:TogglePause(reason)
    if self.pause_transition ~= nil then
        return
    end

    if self.paused then
        self:BeginResume(reason or "toggle")
    else
        self:BeginPause(reason or "toggle")
    end
end

function InGameManager:BeginPause(reason)
    if self.paused or self.pause_transition ~= nil then
        return
    end

    local camera = self:FindPauseCamera()
    if camera == nil then
        log("pause camera missing: " .. PAUSE_CAMERA_NAME)
        return
    end

    if CameraManager ~= nil and CameraManager.GetActiveCameraOwner ~= nil then
        self.pause_previous_view_target = CameraManager.GetActiveCameraOwner()
    elseif CameraManager ~= nil and CameraManager.GetPossessedCameraOwner ~= nil then
        self.pause_previous_view_target = CameraManager.GetPossessedCameraOwner()
    else
        self.pause_previous_view_target = nil
    end

    self.paused = true
    self.pause_transition = "enter_fade_out"
    self.pause_transition_time = 0.0
    if CameraManager ~= nil and CameraManager.FadeOut ~= nil then
        CameraManager.FadeOut(PAUSE_FADE_TIME)
    end
    self.general:Publish("ingame.pause_changed", { paused = true, reason = reason })
    log("pause begin reason=" .. tostring(reason))
end

function InGameManager:BeginResume(reason)
    if (not self.paused) or self.pause_transition ~= nil then
        return
    end

    self.pause_transition = "exit_fade_out"
    self.pause_transition_time = 0.0
    if CameraManager ~= nil and CameraManager.FadeOut ~= nil then
        CameraManager.FadeOut(PAUSE_FADE_TIME)
    end
    log("resume begin reason=" .. tostring(reason))
end

function InGameManager:TickPauseTransition(dt)
    if self.pause_transition == nil then
        return
    end

    self.pause_transition_time = self.pause_transition_time + (dt or 0.0)
    if self.pause_transition == "enter_fade_out" and self.pause_transition_time >= PAUSE_FADE_TIME then
        local camera = self:FindPauseCamera()
        if camera ~= nil and CameraManager ~= nil and CameraManager.SetViewTargetWithBlend ~= nil then
            CameraManager.SetViewTargetWithBlend(camera, PAUSE_BLEND_TIME)
        end
        local seq = self:GetPauseRifleSequence()
        if seq ~= nil and seq.Play ~= nil then
            seq:Play()
            log("pause rifle sequence play: " .. PAUSE_RIFLE_NAME)
        else
            log("pause rifle sequence missing: " .. PAUSE_RIFLE_NAME)
        end
        if CameraManager ~= nil and CameraManager.FadeIn ~= nil then
            CameraManager.FadeIn(PAUSE_FADE_TIME)
        end
        self.pause_transition = "enter_fade_in"
        self.pause_transition_time = 0.0
        return
    end

    if self.pause_transition == "enter_fade_in" and self.pause_transition_time >= PAUSE_FADE_TIME then
        self.pause_transition = nil
        self.pause_transition_time = 0.0
        return
    end

    if self.pause_transition == "exit_fade_out" and self.pause_transition_time >= PAUSE_FADE_TIME then
        self:EndPausePresentation(true)
        if CameraManager ~= nil and CameraManager.FadeIn ~= nil then
            CameraManager.FadeIn(PAUSE_FADE_TIME)
        end
        self.paused = false
        self.pause_transition = "exit_fade_in"
        self.pause_transition_time = 0.0
        self.general:Publish("ingame.pause_changed", { paused = false, reason = "resume" })
        return
    end

    if self.pause_transition == "exit_fade_in" and self.pause_transition_time >= PAUSE_FADE_TIME then
        self.pause_transition = nil
        self.pause_transition_time = 0.0
    end
end

function InGameManager:EndPausePresentation(restore_camera)
    local seq = self.pause_rifle_sequence
    if seq == nil and self.pause_rifle ~= nil then
        seq = self:GetPauseRifleSequence()
    end
    if seq ~= nil and seq.Stop ~= nil then
        seq:Stop()
    end

    if restore_camera and self.pause_previous_view_target ~= nil and
        CameraManager ~= nil and CameraManager.SetViewTargetWithBlend ~= nil then
        CameraManager.SetViewTargetWithBlend(self.pause_previous_view_target, PAUSE_BLEND_TIME)
    end

    self.pause_transition = nil
    self.pause_transition_time = 0.0
    self.pause_rifle_sequence = nil
end

function InGameManager:GoToMain(reason)
    self:EndPausePresentation(false)
    self.paused = false
    self.general:Publish("ingame.pause_changed", { paused = false, reason = reason or "go_main" })
    if self.general ~= nil and self.general.RequestState ~= nil then
        self.general:RequestState(GameState.Main, { reason = reason or "pause_menu" })
    end
end

function InGameManager:SetPhase(phase)
    if self.phase == phase then
        return
    end

    local from = self.phase
    self.phase = phase
    self.general:Publish("ingame.phase_changed", {
        from = from,
        to = phase,
        wave = self.wave,
        timer = self.timer
    })
end

function InGameManager:SetWave(wave)
    wave = math.max(0, math.floor(tonumber(wave) or 0))
    if self.wave == wave then
        return
    end

    local from = self.wave
    self.wave = wave
    self.general:Publish("ingame.wave_changed", {
        from = from,
        to = wave,
        phase = self.phase,
        timer = self.timer
    })
end

function InGameManager:NextWave()
    self:SetWave((self.wave or 0) + 1)
end

function InGameManager:GetSnapshot()
    return {
        running = self.running,
        timer = self.timer,
        elapsed_time = self.timer,
        remaining_time = self:GetRemainingTime(),
        match_duration = self.match_duration,
        phase = self.phase,
        wave = self.wave,
        settings = self.settings,
        sniper_kills = self.sniper_kills,
        friendly_fire_kills = self.friendly_fire_kills,
        score = self.general:GetScore(),
        paused = self.paused,
        result_requested = self.result_requested
    }
end

return InGameManager
