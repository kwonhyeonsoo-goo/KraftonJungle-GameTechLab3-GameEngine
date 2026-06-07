local InGameManager = {}
InGameManager.__index = InGameManager

local ENEMY_KILL_SCORE = 100
local FRIENDLY_KILL_PENALTY = 150
local HEADSHOT_BONUS = 50
local PENETRATION_BONUS = 25

function InGameManager.new(general)
    return setmetatable({
        general = general,
        running = false,
        timer = 0.0,
        phase = "Idle",
        wave = 0,
        settings = {},
        last_timer_second = -1,
        sniper_kills = 0,
        friendly_fire_kills = 0
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

        self.general:Publish("ingame.sniper_damaged", {
            timer = self.timer,
            wave = self.wave,
            phase = self.phase,
            payload = payload
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
            friendly_fire_kills = self.friendly_fire_kills
        })
    end)
end

function InGameManager:Shutdown()
    self.general:UnsubscribeOwner(self)
end

function InGameManager:Start(settings)
    self.running = true
    self.timer = 0.0
    self.phase = "Wave"
    self.wave = 1
    self.settings = settings or self.settings or {}
    self.last_timer_second = -1
    self.sniper_kills = 0
    self.friendly_fire_kills = 0
    self.general:SetScore(0)
    self.general:Publish("ingame.started", self:GetSnapshot())
end

function InGameManager:Stop(reason)
    if not self.running then
        return
    end

    local snapshot = self:GetSnapshot()
    snapshot.reason = reason
    self.running = false
    self.phase = "Idle"
    self.general:Publish("ingame.stopped", snapshot)
end

function InGameManager:Tick(dt)
    if not self.running then
        return
    end

    self.timer = self.timer + (dt or 0.0)
    local second = math.floor(self.timer)
    if second ~= self.last_timer_second then
        self.last_timer_second = second
        self.general:Publish("ingame.timer", self:GetSnapshot())
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
        phase = self.phase,
        wave = self.wave,
        settings = self.settings,
        sniper_kills = self.sniper_kills,
        friendly_fire_kills = self.friendly_fire_kills,
        score = self.general:GetScore()
    }
end

return InGameManager
