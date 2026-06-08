local AudioManager = {}
AudioManager.__index = AudioManager

local function engine_audio()
    return rawget(_G, "AudioManager")
end

function AudioManager.new(general)
    return setmetatable({
        general = general,
        master_volume = 1.0,
        bgm_volume = 1.0,
        sfx_volume = 1.0
    }, AudioManager)
end

function AudioManager:Initialize()
    local data = self.general and self.general.managers and self.general.managers.Data
    if data ~= nil and data.GetSettings ~= nil then
        local settings = data:GetSettings()
        self.bgm_volume = tonumber(settings.bgm_volume) or self.bgm_volume
        self.sfx_volume = tonumber(settings.sfx_volume) or self.sfx_volume
    end
    self:SetBGMVolume(self.bgm_volume)
    self:SetSFXVolume(self.sfx_volume)
end

function AudioManager:Shutdown()
    self:StopAllLoops()
    self:StopBGM()
    self:StopAllSounds()
end

function AudioManager:SetMasterVolume(volume)
    self.master_volume = tonumber(volume) or self.master_volume
    local audio = engine_audio()
    if audio and audio.SetMasterVolume then
        audio.SetMasterVolume(self.master_volume)
    end
    self.general:Publish("audio.master_volume_changed", { volume = self.master_volume })
end

function AudioManager:SetBGMVolume(volume)
    self.bgm_volume = tonumber(volume) or self.bgm_volume
    if self.bgm_volume < 0.0 then
        self.bgm_volume = 0.0
    elseif self.bgm_volume > 1.0 then
        self.bgm_volume = 1.0
    end
    local audio = engine_audio()
    if audio and audio.SetBGMVolume then
        audio.SetBGMVolume(self.bgm_volume)
    end
    self.general:Publish("audio.bgm_volume_changed", { volume = self.bgm_volume })
end

function AudioManager:SetSFXVolume(volume)
    self.sfx_volume = tonumber(volume) or self.sfx_volume
    if self.sfx_volume < 0.0 then
        self.sfx_volume = 0.0
    elseif self.sfx_volume > 1.0 then
        self.sfx_volume = 1.0
    end
    local audio = engine_audio()
    if audio and audio.SetSFXVolume then
        audio.SetSFXVolume(self.sfx_volume)
    end
    self.general:Publish("audio.sfx_volume_changed", { volume = self.sfx_volume })
end

function AudioManager:Load(name, path, loop)
    local audio = engine_audio()
    if audio and audio.Load then
        return audio.Load(name, path, loop == true)
    end
    return nil
end

function AudioManager:PlaySFX(path_or_key, volume)
    local audio = engine_audio()
    if audio and audio.PlaySFX then
        return audio.PlaySFX(path_or_key, volume or 1.0)
    end
    return nil
end

function AudioManager:PlaySFXHandle(path_or_key, volume)
    local audio = engine_audio()
    if audio and audio.PlaySFXHandle then
        return audio.PlaySFXHandle(path_or_key, volume or 1.0)
    end
    if audio and audio.PlaySFX then
        audio.PlaySFX(path_or_key, volume or 1.0)
    end
    return 0
end

function AudioManager:FadeInSFX(handle, duration, target_volume)
    local audio = engine_audio()
    if audio and audio.FadeInSFX then
        return audio.FadeInSFX(handle, duration or 0.0, target_volume or 1.0)
    end
    return false
end

function AudioManager:FadeOutSFX(handle, duration)
    local audio = engine_audio()
    if audio and audio.FadeOutSFX then
        return audio.FadeOutSFX(handle, duration or 0.0)
    end
    if audio and audio.StopSound then
        audio.StopSound(handle)
        return true
    end
    return false
end

function AudioManager:PlayBGM(name, volume)
    local audio = engine_audio()
    if audio and audio.PlayBGM then
        audio.PlayBGM(name, volume or self.bgm_volume)
    end
end

function AudioManager:FadeInBGM(duration, target_volume)
    local audio = engine_audio()
    if audio and audio.FadeInBGM then
        return audio.FadeInBGM(duration or 0.0, target_volume or self.bgm_volume)
    end
    return false
end

function AudioManager:FadeOutBGM(duration)
    local audio = engine_audio()
    if audio and audio.FadeOutBGM then
        return audio.FadeOutBGM(duration or 0.0)
    end
    self:StopBGM()
    return false
end

function AudioManager:StopBGM()
    local audio = engine_audio()
    if audio and audio.StopBGM then
        audio.StopBGM()
    end
end

function AudioManager:StopSound(handle)
    local audio = engine_audio()
    if audio and audio.StopSound then
        audio.StopSound(handle)
    end
end

function AudioManager:StopAllSounds()
    local audio = engine_audio()
    if audio and audio.StopAllSounds then
        audio.StopAllSounds()
    end
end

function AudioManager:FadeInSound(handle, duration, target_volume)
    local audio = engine_audio()
    if audio and audio.FadeInSound then
        return audio.FadeInSound(handle, duration or 0.0, target_volume or 1.0)
    end
    return false
end

function AudioManager:FadeOutSound(handle, duration)
    local audio = engine_audio()
    if audio and audio.FadeOutSound then
        return audio.FadeOutSound(handle, duration or 0.0)
    end
    self:StopSound(handle)
    return false
end

function AudioManager:PlayLoop(sound_name, loop_name, volume, pitch)
    local audio = engine_audio()
    if audio and audio.PlayLoop then
        audio.PlayLoop(sound_name, loop_name, volume or 1.0, pitch or 1.0)
    end
end

function AudioManager:StopLoop(loop_name)
    local audio = engine_audio()
    if audio and audio.StopLoop then
        audio.StopLoop(loop_name)
    end
end

function AudioManager:StopAllLoops()
    local audio = engine_audio()
    if audio and audio.StopAllLoops then
        audio.StopAllLoops()
    end
end

return AudioManager
