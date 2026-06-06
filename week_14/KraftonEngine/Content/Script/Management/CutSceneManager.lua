local CutSceneManager = {}
CutSceneManager.__index = CutSceneManager

function CutSceneManager.new(general)
    return setmetatable({
        general = general,
        registry = {},
        current = nil
    }, CutSceneManager)
end

function CutSceneManager:Initialize()
end

function CutSceneManager:Shutdown()
    self:Stop("shutdown")
end

function CutSceneManager:Register(id, definition)
    if type(id) ~= "string" or type(definition) ~= "table" then
        return false
    end

    self.registry[id] = definition
    return true
end

function CutSceneManager:Play(id, payload)
    local definition = self.registry[id]
    if definition == nil then
        print("[CutSceneManager] missing cutscene: " .. tostring(id))
        return false
    end

    self:Stop("replace")
    self.current = {
        id = id,
        definition = definition,
        payload = payload or {},
        elapsed = 0.0,
        duration = tonumber(definition.duration) or 0.0
    }

    if definition.on_begin ~= nil then
        pcall(definition.on_begin, self.current)
    end
    self.general:Publish("cutscene.started", self.current)
    return true
end

function CutSceneManager:Stop(reason)
    if self.current == nil then
        return
    end

    local finished = self.current
    finished.reason = reason
    if finished.definition.on_end ~= nil then
        pcall(finished.definition.on_end, finished)
    end
    self.current = nil
    self.general:Publish("cutscene.stopped", finished)
end

function CutSceneManager:Tick(dt)
    if self.current == nil then
        return
    end

    local current = self.current
    current.elapsed = current.elapsed + (dt or 0.0)
    if current.definition.on_tick ~= nil then
        pcall(current.definition.on_tick, current, dt or 0.0)
    end

    if current.duration > 0.0 and current.elapsed >= current.duration then
        local next_state = current.definition.next_state
        self:Stop("finished")
        if next_state ~= nil then
            self.general:RequestState(next_state, { reason = "cutscene_finished", cutscene = current.id })
        end
    end
end

return CutSceneManager
