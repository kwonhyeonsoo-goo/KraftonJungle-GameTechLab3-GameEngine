local DataManager = {}
DataManager.__index = DataManager

local SAVE_PATH = "GameData/player_profile.json"

local function default_data()
    return {
        nickname = "",
        score = 0,
        high_score = 0,
        runs = {}
    }
end

function DataManager.new(general)
    return setmetatable({
        general = general,
        data = default_data()
    }, DataManager)
end

function DataManager:Initialize()
    self:Load()
end

function DataManager:Shutdown()
    self:Save()
end

function DataManager:Load()
    if Save ~= nil and Save.ReadJson ~= nil then
        local loaded = Save.ReadJson(SAVE_PATH)
        if type(loaded) == "table" then
            self.data = loaded
            self.data.nickname = self.data.nickname or ""
            self.data.score = tonumber(self.data.score) or 0
            self.data.high_score = tonumber(self.data.high_score) or 0
            self.data.runs = self.data.runs or {}
            return true
        end
    end
    self.data = default_data()
    return false
end

function DataManager:Save()
    if Save ~= nil and Save.WriteJson ~= nil then
        return Save.WriteJson(SAVE_PATH, self.data)
    end
    return false
end

function DataManager:SetNickname(nickname)
    self.data.nickname = tostring(nickname or "")
    self:Save()
    self.general:Publish("data.nickname_changed", { nickname = self.data.nickname })
end

function DataManager:GetNickname()
    return self.data.nickname or ""
end

function DataManager:SetScore(score)
    self.data.score = math.max(0, math.floor(tonumber(score) or 0))
    if self.data.score > (self.data.high_score or 0) then
        self.data.high_score = self.data.score
    end
    self.general:Publish("data.score_changed", {
        score = self.data.score,
        high_score = self.data.high_score
    })
end

function DataManager:AddScore(delta)
    self:SetScore((self.data.score or 0) + (tonumber(delta) or 0))
end

function DataManager:GetScore()
    return self.data.score or 0
end

function DataManager:GetHighScore()
    return self.data.high_score or 0
end

function DataManager:CommitRun(result)
    result = result or {}
    result.score = tonumber(result.score) or self:GetScore()
    result.state = result.state or "Unknown"
    table.insert(self.data.runs, result)

    if result.score > (self.data.high_score or 0) then
        self.data.high_score = result.score
    end

    self:Save()
    self.general:Publish("data.run_committed", result)
end

return DataManager
