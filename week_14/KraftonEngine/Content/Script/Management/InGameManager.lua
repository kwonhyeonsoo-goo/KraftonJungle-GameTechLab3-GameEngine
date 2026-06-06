local InGameManager = {}
InGameManager.__index = InGameManager

function InGameManager.new(general)
    return setmetatable({
        general = general,
        running = false,
        timer = 0.0,
        phase = "Idle",
        wave = 0,
        settings = {},
        last_timer_second = -1
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
        settings = self.settings
    }
end

return InGameManager
