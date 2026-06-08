local CutSceneManager = {}
CutSceneManager.__index = CutSceneManager

local SNIPER_KILLCAM_DURATION = 5.0
local SNIPER_KILLCAM_SHOOT_SFX = "SFX/Sniper/Shoot1.wav"
local SNIPER_KILLCAM_SLOWDOWN_SFX = "SFX/Sniper/SlowDown.mp3"
local SNIPER_KILLCAM_SLOWDOWN_SFX_DELAY = 0.3
local SNIPER_KILLCAM_BULLET_CAM_SFX = "SFX/Sniper/BulletCam1.mp3"
local SNIPER_KILLCAM_BULLET_CAM_SFX_DELAY = 1.0

local function clamp01(value)
    value = tonumber(value) or 0.0
    if value < 0.0 then
        return 0.0
    end
    if value > 1.0 then
        return 1.0
    end
    return value
end

local function lerp(a, b, alpha)
    return a + (b - a) * alpha
end

local function merge_tables(base, override)
    local result = {}
    if type(base) == "table" then
        for key, value in pairs(base) do
            result[key] = value
        end
    end
    if type(override) == "table" then
        for key, value in pairs(override) do
            result[key] = value
        end
    end
    return result
end

local function play_sfx(general, path, volume)
    if general ~= nil and general.PlaySFX ~= nil then
        general:PlaySFX(path, volume or 1.0)
        return
    end

    if AudioManager ~= nil and AudioManager.PlaySFX ~= nil then
        AudioManager.PlaySFX(path, volume or 1.0)
    end
end

local function sample_keyframes(frames, alpha)
    if type(frames) ~= "table" or #frames == 0 then
        return {}
    end

    alpha = clamp01(alpha)
    local first = frames[1]
    if alpha <= (tonumber(first.time) or 0.0) then
        local sample = {}
        for key, value in pairs(first) do
            if key ~= "time" then
                sample[key] = value
            end
        end
        return sample
    end

    for index = 1, #frames - 1 do
        local from = frames[index]
        local to = frames[index + 1]
        local from_time = tonumber(from.time) or 0.0
        local to_time = tonumber(to.time) or 1.0
        if alpha <= to_time then
            local range = to_time - from_time
            local local_alpha = range > 0.0001 and ((alpha - from_time) / range) or 1.0
            local eased = local_alpha
            local sample = {}
            for key, to_value in pairs(to) do
                if key ~= "time" then
                    local from_value = from[key]
                    if type(from_value) == "number" and type(to_value) == "number" then
                        sample[key] = lerp(from_value, to_value, eased)
                    else
                        sample[key] = to_value
                    end
                end
            end
            for key, from_value in pairs(from) do
                if key ~= "time" and sample[key] == nil then
                    sample[key] = from_value
                end
            end
            return sample
        end
    end

    local last = frames[#frames]
    local sample = {}
    for key, value in pairs(last) do
        if key ~= "time" then
            sample[key] = value
        end
    end
    return sample
end

local SNIPER_KILLCAM_PROFILES = {
    front_pass_tail = {
        bullet = {
            spinRevolutions = 64.0,
            spinPhase = 0.0,
            scale = 1.0
        },
        shockwave = {
            enabled = true,
            forwardOffset = -0.34,
            sideOffset = 0.0,
            upOffset = 0.0,
            radius = 0.110,
            startRadiusBoost = 0.16,
            width = 0.040,
            strength = 0.018,
            startStrengthBoost = 0.040,
            falloff = 1.25,
            directionalStretch = 4.4,
            decay = 4.6
        },
        rig_frames = {
            {
                time = 0.00,
                bAllowRailExtrapolation = 1.0,
                bClampAuthoredRailAlpha = 1.0,
                RailAlphaClampMin = -0.35,
                RailAlphaClampMax = 1.15,
                CameraRailAlphaOverride = 0.000,
                LookRailAlphaOverride = 0.000,
                ForwardOffset = 7.40,
                SideOffset = -3.55,
                UpOffset = 0.34,
                LookAhead = 0.00,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 0.0,
                FOV = 0.72,
                Roll = 0.0,
                CameraLagSpeed = 14.0,
                LookLagSpeed = 54.0,
                CameraShakeAmplitude = 0.0015,
                CameraShakeFrequency = 7.5,
                DOFFocusRange = 1.4,
                DOFBlurRadius = 4.2,
                OrbitBlend = 0.0,
                BulletRailAlphaOverride = 0.0,
                BulletSpinRevolutions = 72.0
            },
            {
                time = 0.30,
                CameraRailAlphaOverride = 0.000,
                LookRailAlphaOverride = 0.000,
                ForwardOffset = 7.10,
                SideOffset = -3.35,
                UpOffset = 0.28,
                LookAhead = 0.00,
                bLookAtBulletVisual = 0.0,
                FOV = 0.70,
                Roll = 0.0,
                CameraLagSpeed = 14.0,
                LookLagSpeed = 54.0,
                CameraShakeAmplitude = 0.0015,
                CameraShakeFrequency = 7.5,
                DOFFocusRange = 1.2,
                DOFBlurRadius = 4.8,
                BulletRailAlphaOverride = 0.120,
                BulletSpinRevolutions = 72.0
            },
            {
                time = 0.44,
                CameraRailAlphaOverride = 0.260,
                LookRailAlphaOverride = 0.360,
                ForwardOffset = 3.05,
                SideOffset = -2.05,
                UpOffset = 0.82,
                LookAhead = -0.08,
                bLookAtBulletVisual = 1.0,
                FOV = 0.56,
                Roll = 0.0,
                CameraLagSpeed = 18.0,
                LookLagSpeed = 56.0,
                CameraShakeAmplitude = 0.0012,
                CameraShakeFrequency = 8.5,
                DOFFocusRange = 1.0,
                DOFBlurRadius = 5.0,
                BulletRailAlphaOverride = 0.440,
                BulletSpinRevolutions = 58.0
            },
            {
                time = 0.60,
                CameraRailAlphaOverride = 0.520,
                LookRailAlphaOverride = 0.660,
                ForwardOffset = 1.10,
                SideOffset = -1.45,
                UpOffset = 0.55,
                LookAhead = 0.18,
                FOV = 0.49,
                Roll = 0.0,
                CameraLagSpeed = 26.0,
                LookLagSpeed = 58.0,
                CameraShakeAmplitude = 0.0010,
                CameraShakeFrequency = 9.5,
                DOFFocusRange = 0.65,
                DOFBlurRadius = 4.6,
                BulletRailAlphaOverride = 0.620,
                BulletSpinRevolutions = 50.0
            },
            {
                time = 0.78,
                CameraRailAlphaOverride = 0.800,
                LookRailAlphaOverride = 0.900,
                ForwardOffset = -0.82,
                SideOffset = -0.78,
                UpOffset = 0.32,
                LookAhead = 0.40,
                FOV = 0.42,
                Roll = 0.0,
                CameraLagSpeed = 34.0,
                LookLagSpeed = 60.0,
                CameraShakeAmplitude = 0.0008,
                CameraShakeFrequency = 10.5,
                DOFFocusRange = 0.95,
                DOFBlurRadius = 3.8,
                BulletRailAlphaOverride = 0.840,
                BulletSpinRevolutions = 44.0
            },
            {
                time = 1.00,
                CameraRailAlphaOverride = 1.000,
                LookRailAlphaOverride = 1.000,
                ForwardOffset = -1.35,
                SideOffset = -0.48,
                UpOffset = 0.18,
                LookAhead = 0.58,
                FOV = 0.39,
                Roll = 0.0,
                CameraLagSpeed = 40.0,
                LookLagSpeed = 64.0,
                CameraShakeAmplitude = 0.0005,
                CameraShakeFrequency = 11.0,
                DOFFocusRange = 1.2,
                DOFBlurRadius = 3.2,
                BulletRailAlphaOverride = 1.000,
                BulletSpinRevolutions = 42.0
            }
        }
    },
    right_high_center_rush = {
        bullet = {
            spinRevolutions = 78.0,
            spinPhase = 0.0,
            scale = 1.0
        },
        shockwave = {
            enabled = true,
            forwardOffset = -0.42,
            sideOffset = 0.0,
            upOffset = 0.0,
            radius = 0.100,
            startRadiusBoost = 0.14,
            width = 0.034,
            strength = 0.016,
            startStrengthBoost = 0.034,
            falloff = 1.25,
            directionalStretch = 4.8,
            decay = 4.8
        },
        rig_frames = {
            {
                time = 0.00,
                bAllowRailExtrapolation = 1.0,
                bClampAuthoredRailAlpha = 1.0,
                RailAlphaClampMin = -0.20,
                RailAlphaClampMax = 1.10,
                CameraRailAlphaOverride = 0.000,
                LookRailAlphaOverride = 0.030,
                BulletRailAlphaOverride = 0.000,
                ForwardOffset = -0.18,
                SideOffset = 1.45,
                UpOffset = 1.02,
                LookAhead = 1.15,
                LookSideOffset = 1.70,
                LookUpOffset = 0.92,
                bLookAtBulletVisual = 0.0,
                FOV = 0.44,
                Roll = 0.0,
                CameraLagSpeed = 42.0,
                LookLagSpeed = 54.0,
                CameraShakeAmplitude = 0.0010,
                CameraShakeFrequency = 8.0,
                DOFFocusRange = 0.95,
                DOFBlurRadius = 3.4,
                OrbitBlend = 0.0,
                BulletScaleMultiplier = 0.0,
                BulletSpinRevolutions = 88.0
            },
            {
                time = 0.30,
                CameraRailAlphaOverride = 0.300,
                LookRailAlphaOverride = 0.330,
                BulletRailAlphaOverride = 0.300,
                ForwardOffset = -0.18,
                SideOffset = 1.36,
                UpOffset = 0.96,
                LookAhead = 0.72,
                LookSideOffset = 1.06,
                LookUpOffset = 0.58,
                bLookAtBulletVisual = 0.0,
                FOV = 0.46,
                CameraLagSpeed = 42.0,
                LookLagSpeed = 54.0,
                CameraShakeAmplitude = 0.0010,
                CameraShakeFrequency = 8.0,
                DOFFocusRange = 0.82,
                DOFBlurRadius = 3.9,
                BulletScaleMultiplier = 0.0,
                BulletSpinRevolutions = 88.0
            },
            {
                time = 0.50,
                CameraRailAlphaOverride = 0.500,
                LookRailAlphaOverride = 0.500,
                BulletRailAlphaOverride = 0.500,
                ForwardOffset = 0.88,
                SideOffset = 0.44,
                UpOffset = 0.68,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.50,
                CameraLagSpeed = 40.0,
                LookLagSpeed = 64.0,
                CameraShakeAmplitude = 0.0009,
                CameraShakeFrequency = 8.5,
                DOFFocusRange = 0.55,
                DOFBlurRadius = 4.5,
                BulletScaleMultiplier = 1.0,
                BulletSpinRevolutions = 58.0
            },
            {
                time = 0.72,
                CameraRailAlphaOverride = 0.720,
                LookRailAlphaOverride = 0.735,
                BulletRailAlphaOverride = 0.720,
                ForwardOffset = 0.78,
                SideOffset = 0.34,
                UpOffset = 0.54,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.64,
                CameraLagSpeed = 44.0,
                LookLagSpeed = 66.0,
                CameraShakeAmplitude = 0.0008,
                CameraShakeFrequency = 9.0,
                DOFFocusRange = 0.62,
                DOFBlurRadius = 4.2,
                BulletScaleMultiplier = 1.0,
                BulletSpinRevolutions = 52.0
            },
            {
                time = 0.90,
                CameraRailAlphaOverride = 0.860,
                LookRailAlphaOverride = 0.900,
                BulletRailAlphaOverride = 0.900,
                ForwardOffset = 0.70,
                SideOffset = 0.28,
                UpOffset = 0.50,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.82,
                CameraLagSpeed = 48.0,
                LookLagSpeed = 68.0,
                CameraShakeAmplitude = 0.0006,
                CameraShakeFrequency = 9.5,
                DOFFocusRange = 0.72,
                DOFBlurRadius = 3.5,
                BulletScaleMultiplier = 1.0,
                BulletSpinRevolutions = 46.0
            },
            {
                time = 0.925,
                CameraRailAlphaOverride = 0.860,
                LookRailAlphaOverride = 0.925,
                BulletRailAlphaOverride = 0.925,
                ForwardOffset = 0.70,
                SideOffset = 0.28,
                UpOffset = 0.50,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.38,
                CameraLagSpeed = 48.0,
                LookLagSpeed = 68.0,
                CameraShakeAmplitude = 0.0004,
                CameraShakeFrequency = 9.5,
                DOFFocusRange = 0.95,
                DOFBlurRadius = 2.6,
                BulletScaleMultiplier = 1.0,
                BulletSpinRevolutions = 46.0
            },
            {
                time = 1.00,
                CameraRailAlphaOverride = 0.860,
                LookRailAlphaOverride = 1.000,
                BulletRailAlphaOverride = 1.000,
                ForwardOffset = 0.70,
                SideOffset = 0.28,
                UpOffset = 0.50,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.36,
                CameraLagSpeed = 48.0,
                LookLagSpeed = 68.0,
                CameraShakeAmplitude = 0.0002,
                CameraShakeFrequency = 9.5,
                DOFFocusRange = 1.1,
                DOFBlurRadius = 2.2,
                BulletScaleMultiplier = 1.0,
                BulletSpinRevolutions = 46.0
            }
        }
    },
    near_bullet_sway_drop = {
        bullet = {
            spinRevolutions = 54.0,
            spinPhase = 0.0,
            scale = 1.0
        },
        shockwave = {
            enabled = true,
            forwardOffset = -0.30,
            sideOffset = 0.0,
            upOffset = 0.0,
            radius = 0.092,
            startRadiusBoost = 0.12,
            width = 0.032,
            strength = 0.014,
            startStrengthBoost = 0.030,
            falloff = 1.30,
            directionalStretch = 4.4,
            decay = 5.0
        },
        rig_frames = {
            {
                time = 0.00,
                bAllowRailExtrapolation = 1.0,
                bClampAuthoredRailAlpha = 1.0,
                RailAlphaClampMin = -0.10,
                RailAlphaClampMax = 1.10,
                CameraRailAlphaOverride = 0.000,
                LookRailAlphaOverride = 0.020,
                BulletRailAlphaOverride = 0.020,
                ForwardOffset = 1.75,
                SideOffset = -0.22,
                UpOffset = 0.30,
                LookAhead = -0.02,
                LookSideOffset = 0.0,
                LookUpOffset = -0.50,
                bLookAtBulletVisual = 1.0,
                FOV = 0.72,
                Roll = 0.0,
                CameraLagSpeed = 34.0,
                LookLagSpeed = 70.0,
                CameraShakeAmplitude = 0.0014,
                CameraShakeFrequency = 10.0,
                DOFFocusRange = 0.70,
                DOFBlurRadius = 4.7,
                OrbitBlend = 0.0,
                BulletScaleMultiplier = 0.96,
                BulletSpinRevolutions = 30.0
            },
            {
                time = 0.12,
                CameraRailAlphaOverride = 0.055,
                LookRailAlphaOverride = 0.085,
                BulletRailAlphaOverride = 0.120,
                ForwardOffset = 1.68,
                SideOffset = 0.24,
                UpOffset = 0.24,
                LookAhead = -0.02,
                LookSideOffset = 0.0,
                LookUpOffset = -0.52,
                bLookAtBulletVisual = 1.0,
                FOV = 0.70,
                Roll = 0.0,
                CameraLagSpeed = 36.0,
                LookLagSpeed = 72.0,
                CameraShakeAmplitude = 0.0015,
                CameraShakeFrequency = 10.5,
                DOFFocusRange = 0.66,
                DOFBlurRadius = 5.0,
                BulletScaleMultiplier = 1.00,
                BulletSpinRevolutions = 30.0
            },
            {
                time = 0.24,
                CameraRailAlphaOverride = 0.125,
                LookRailAlphaOverride = 0.165,
                BulletRailAlphaOverride = 0.240,
                ForwardOffset = 1.58,
                SideOffset = -0.18,
                UpOffset = 0.15,
                LookAhead = -0.01,
                LookSideOffset = 0.0,
                LookUpOffset = -0.50,
                bLookAtBulletVisual = 1.0,
                FOV = 0.68,
                Roll = 0.0,
                CameraLagSpeed = 38.0,
                LookLagSpeed = 72.0,
                CameraShakeAmplitude = 0.0014,
                CameraShakeFrequency = 10.5,
                DOFFocusRange = 0.62,
                DOFBlurRadius = 5.0,
                BulletScaleMultiplier = 1.00,
                BulletSpinRevolutions = 31.0
            },
            {
                time = 0.40,
                CameraRailAlphaOverride = 0.270,
                LookRailAlphaOverride = 0.330,
                BulletRailAlphaOverride = 0.400,
                ForwardOffset = 1.42,
                SideOffset = 0.10,
                UpOffset = 0.00,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = -0.46,
                bLookAtBulletVisual = 1.0,
                FOV = 0.64,
                Roll = 0.0,
                CameraLagSpeed = 40.0,
                LookLagSpeed = 74.0,
                CameraShakeAmplitude = 0.0010,
                CameraShakeFrequency = 9.5,
                DOFFocusRange = 0.58,
                DOFBlurRadius = 4.8,
                BulletScaleMultiplier = 0.98,
                BulletSpinRevolutions = 32.0
            },
            {
                time = 0.62,
                CameraRailAlphaOverride = 0.500,
                LookRailAlphaOverride = 0.580,
                BulletRailAlphaOverride = 0.620,
                ForwardOffset = 1.18,
                SideOffset = 0.04,
                UpOffset = -0.18,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = -0.38,
                bLookAtBulletVisual = 1.0,
                FOV = 0.58,
                Roll = 0.0,
                CameraLagSpeed = 42.0,
                LookLagSpeed = 74.0,
                CameraShakeAmplitude = 0.0008,
                CameraShakeFrequency = 8.8,
                DOFFocusRange = 0.62,
                DOFBlurRadius = 4.3,
                BulletScaleMultiplier = 0.94,
                BulletSpinRevolutions = 34.0
            },
            {
                time = 0.80,
                CameraRailAlphaOverride = 0.700,
                LookRailAlphaOverride = 0.790,
                BulletRailAlphaOverride = 0.800,
                ForwardOffset = 0.92,
                SideOffset = 0.02,
                UpOffset = -0.34,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = -0.28,
                bLookAtBulletVisual = 1.0,
                FOV = 0.52,
                Roll = 0.0,
                CameraLagSpeed = 44.0,
                LookLagSpeed = 74.0,
                CameraShakeAmplitude = 0.0005,
                CameraShakeFrequency = 8.0,
                DOFFocusRange = 0.68,
                DOFBlurRadius = 3.7,
                BulletScaleMultiplier = 0.90,
                BulletSpinRevolutions = 36.0
            },
            {
                time = 0.90,
                CameraRailAlphaOverride = 0.760,
                LookRailAlphaOverride = 0.900,
                BulletRailAlphaOverride = 0.900,
                ForwardOffset = 0.76,
                SideOffset = 0.0,
                UpOffset = -0.42,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = -0.18,
                bLookAtBulletVisual = 1.0,
                FOV = 0.48,
                Roll = 0.0,
                CameraLagSpeed = 28.0,
                LookLagSpeed = 76.0,
                CameraShakeAmplitude = 0.00025,
                CameraShakeFrequency = 7.5,
                DOFFocusRange = 0.74,
                DOFBlurRadius = 3.2,
                BulletScaleMultiplier = 0.86,
                BulletSpinRevolutions = 38.0
            },
            {
                time = 1.00,
                CameraRailAlphaOverride = 0.760,
                LookRailAlphaOverride = 0.980,
                BulletRailAlphaOverride = 1.000,
                ForwardOffset = 0.76,
                SideOffset = 0.0,
                UpOffset = -0.42,
                LookAhead = 0.10,
                LookSideOffset = 0.0,
                LookUpOffset = -0.10,
                bLookAtBulletVisual = 1.0,
                FOV = 0.46,
                Roll = 0.0,
                CameraLagSpeed = 24.0,
                LookLagSpeed = 76.0,
                CameraShakeAmplitude = 0.0001,
                CameraShakeFrequency = 7.0,
                DOFFocusRange = 1.0,
                DOFBlurRadius = 2.4,
                BulletScaleMultiplier = 0.82,
                BulletSpinRevolutions = 40.0
            }
        }
    },
    side_close_orbit_impact = {
        bullet = {
            spinRevolutions = 104.0,
            spinPhase = 0.0,
            scale = 1.0
        },
        shockwave = {
            enabled = true,
            forwardOffset = -0.36,
            sideOffset = 0.0,
            upOffset = 0.0,
            radius = 0.096,
            startRadiusBoost = 0.13,
            width = 0.034,
            strength = 0.015,
            startStrengthBoost = 0.032,
            falloff = 1.28,
            directionalStretch = 4.5,
            decay = 4.9
        },
        rig_frames = {
            {
                time = 0.00,
                bAllowRailExtrapolation = 1.0,
                bClampAuthoredRailAlpha = 1.0,
                RailAlphaClampMin = -0.12,
                RailAlphaClampMax = 1.08,
                CameraRailAlphaOverride = 0.020,
                LookRailAlphaOverride = 0.060,
                BulletRailAlphaOverride = 0.020,
                ForwardOffset = -1.42,
                SideOffset = -1.30,
                UpOffset = 0.36,
                LookAhead = 0.24,
                LookSideOffset = -0.30,
                LookUpOffset = 0.05,
                bLookAtBulletVisual = 1.0,
                FOV = 0.58,
                Roll = 0.0,
                CameraLagSpeed = 42.0,
                LookLagSpeed = 58.0,
                DOFFocusRange = 0.72,
                DOFBlurRadius = 4.8,
                OrbitBlend = 0.0,
                OrbitYaw = -72.0,
                OrbitPitch = 10.0,
                OrbitRadius = 1.70,
                OrbitPivotForwardOffset = 0.0,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 1.18
            },
            {
                time = 0.18,
                CameraRailAlphaOverride = 0.170,
                LookRailAlphaOverride = 0.210,
                BulletRailAlphaOverride = 0.180,
                ForwardOffset = -1.02,
                SideOffset = -1.00,
                UpOffset = 0.24,
                LookAhead = 0.14,
                LookSideOffset = -0.18,
                LookUpOffset = 0.02,
                bLookAtBulletVisual = 1.0,
                FOV = 0.52,
                CameraLagSpeed = 48.0,
                LookLagSpeed = 62.0,
                DOFFocusRange = 0.56,
                DOFBlurRadius = 5.0,
                OrbitBlend = 0.0,
                OrbitYaw = -64.0,
                OrbitPitch = 8.0,
                OrbitRadius = 1.42,
                OrbitPivotForwardOffset = 0.0,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 1.12
            },
            {
                time = 0.30,
                CameraRailAlphaOverride = 0.300,
                LookRailAlphaOverride = 0.335,
                BulletRailAlphaOverride = 0.300,
                ForwardOffset = -0.66,
                SideOffset = -0.54,
                UpOffset = 0.12,
                LookAhead = 0.04,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.46,
                CameraLagSpeed = 56.0,
                LookLagSpeed = 70.0,
                DOFFocusRange = 0.40,
                DOFBlurRadius = 5.3,
                OrbitBlend = 0.0,
                OrbitYaw = -52.0,
                OrbitPitch = 5.0,
                OrbitRadius = 1.08,
                OrbitPivotForwardOffset = 0.0,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 1.06
            },
            {
                time = 0.48,
                CameraRailAlphaOverride = 0.480,
                LookRailAlphaOverride = 0.510,
                BulletRailAlphaOverride = 0.480,
                ForwardOffset = -0.46,
                SideOffset = 0.18,
                UpOffset = 0.02,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.44,
                CameraLagSpeed = 58.0,
                LookLagSpeed = 72.0,
                DOFFocusRange = 0.42,
                DOFBlurRadius = 4.8,
                OrbitBlend = 0.0,
                OrbitYaw = -32.0,
                OrbitPitch = 0.0,
                OrbitRadius = 0.95,
                OrbitPivotForwardOffset = 0.0,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 1.0
            },
            {
                time = 0.68,
                CameraRailAlphaOverride = 0.680,
                LookRailAlphaOverride = 0.710,
                BulletRailAlphaOverride = 0.680,
                ForwardOffset = -0.42,
                SideOffset = 0.46,
                UpOffset = -0.04,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.42,
                CameraLagSpeed = 58.0,
                LookLagSpeed = 72.0,
                DOFFocusRange = 0.48,
                DOFBlurRadius = 4.1,
                OrbitBlend = 0.0,
                OrbitYaw = -18.0,
                OrbitPitch = -3.0,
                OrbitRadius = 0.92,
                OrbitPivotForwardOffset = 0.0,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 0.96
            },
            {
                time = 0.80,
                CameraRailAlphaOverride = 0.800,
                LookRailAlphaOverride = 0.830,
                BulletRailAlphaOverride = 0.800,
                ForwardOffset = -0.36,
                SideOffset = 0.52,
                UpOffset = -0.06,
                LookAhead = 0.0,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.40,
                CameraLagSpeed = 54.0,
                LookLagSpeed = 72.0,
                DOFFocusRange = 0.58,
                DOFBlurRadius = 3.6,
                OrbitBlend = 0.20,
                OrbitYaw = -5.0,
                OrbitPitch = -7.0,
                OrbitRadius = 0.88,
                OrbitPivotForwardOffset = 0.10,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = 0.0,
                BulletScaleMultiplier = 0.92
            },
            {
                time = 0.90,
                CameraRailAlphaOverride = 0.900,
                LookRailAlphaOverride = 0.930,
                BulletRailAlphaOverride = 0.900,
                ForwardOffset = -0.32,
                SideOffset = 0.28,
                UpOffset = -0.10,
                LookAhead = 0.04,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.38,
                CameraLagSpeed = 48.0,
                LookLagSpeed = 72.0,
                DOFFocusRange = 0.78,
                DOFBlurRadius = 2.9,
                OrbitBlend = 0.72,
                OrbitYaw = 32.0,
                OrbitPitch = -11.0,
                OrbitRadius = 0.82,
                OrbitPivotForwardOffset = 0.18,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = -0.02,
                BulletScaleMultiplier = 0.88
            },
            {
                time = 1.00,
                CameraRailAlphaOverride = 0.985,
                LookRailAlphaOverride = 1.000,
                BulletRailAlphaOverride = 1.000,
                ForwardOffset = -0.20,
                SideOffset = -0.10,
                UpOffset = -0.12,
                LookAhead = 0.10,
                LookSideOffset = 0.0,
                LookUpOffset = 0.0,
                bLookAtBulletVisual = 1.0,
                FOV = 0.36,
                CameraLagSpeed = 42.0,
                LookLagSpeed = 72.0,
                DOFFocusRange = 1.0,
                DOFBlurRadius = 2.2,
                OrbitBlend = 1.0,
                OrbitYaw = 72.0,
                OrbitPitch = -14.0,
                OrbitRadius = 0.76,
                OrbitPivotForwardOffset = 0.26,
                OrbitPivotSideOffset = 0.0,
                OrbitPivotUpOffset = -0.04,
                BulletScaleMultiplier = 0.82
            }
        }
    }
}

local SNIPER_KILLCAM_PROFILE_CYCLE = {
    "front_pass_tail",
    "right_high_center_rush",
    "near_bullet_sway_drop",
    "side_close_orbit_impact"
}

local next_sniper_killcam_profile_cycle_index = 1

local function take_next_sniper_killcam_profile_id()
    local profile_id = SNIPER_KILLCAM_PROFILE_CYCLE[next_sniper_killcam_profile_cycle_index]
    next_sniper_killcam_profile_cycle_index = next_sniper_killcam_profile_cycle_index + 1
    if next_sniper_killcam_profile_cycle_index > #SNIPER_KILLCAM_PROFILE_CYCLE then
        next_sniper_killcam_profile_cycle_index = 1
    end
    return profile_id or "front_pass_tail"
end

local function resolve_sniper_killcam_profile(payload)
    local profile_id = "front_pass_tail"
    if type(payload) == "table" and type(payload.profile) == "string" then
        profile_id = payload.profile
    end
    return SNIPER_KILLCAM_PROFILES[profile_id] or SNIPER_KILLCAM_PROFILES.front_pass_tail
end

local function apply_sniper_killcam_profile(current)
    if SniperKillCam == nil or SniperKillCam.SetRigScalars == nil then
        return
    end

    local profile = current.killcam_profile
    if type(profile) ~= "table" then
        return
    end

    local duration = tonumber(current.duration) or 0.0
    local alpha = duration > 0.0001 and ((tonumber(current.elapsed) or 0.0) / duration) or 0.0
    local rig = sample_keyframes(profile.rig_frames, alpha)
    rig = merge_tables(rig, current.rig_overrides)
    SniperKillCam.SetRigScalars(rig)
end

function CutSceneManager.new(general)
    return setmetatable({
        general = general,
        registry = {},
        current = nil
    }, CutSceneManager)
end

function CutSceneManager:Initialize()
    self:Register("sniper_killcam", {
        duration = SNIPER_KILLCAM_DURATION,
        skippable = true,
        on_begin = function(current)
            local payload = current.payload or {}
            local bullet_id = tonumber(payload.bullet_id) or 0
            local duration = tonumber(payload.duration) or current.duration or SNIPER_KILLCAM_DURATION
            local camera_mode = tonumber(payload.camera_mode) or 0
            current.duration = duration
            current.killcam_profile = resolve_sniper_killcam_profile(payload)
            current.rig_overrides = type(payload.rig) == "table" and payload.rig or nil
            current.slowdown_sfx_played = false
            current.bullet_cam_sfx_played = false
            play_sfx(self.general, SNIPER_KILLCAM_SHOOT_SFX, 1.0)
            if SniperKillCam ~= nil and SniperKillCam.Start ~= nil then
                if SniperKillCam.ConfigureBullet ~= nil then
                    SniperKillCam.ConfigureBullet(merge_tables(current.killcam_profile.bullet, payload.bullet))
                end
                if SniperKillCam.ConfigureShockWave ~= nil then
                    SniperKillCam.ConfigureShockWave(merge_tables(current.killcam_profile.shockwave, payload.shockwave))
                end
                apply_sniper_killcam_profile(current)
                local started = SniperKillCam.Start(bullet_id, duration, camera_mode)
                current.director_started = started == true
            end
            if self.general ~= nil and self.general.Publish ~= nil then
                self.general:Publish("cutscene.presentation", {
                    active = true
                })
                self.general:Publish("cutscene.skip_prompt", {
                    visible = true,
                    text = "Press Space to Skip"
                })
            end
        end,
        on_tick = function(current, dt)
            if current.skipped == true then
                return
            end
            apply_sniper_killcam_profile(current)
            if current.slowdown_sfx_played ~= true and
                (tonumber(current.elapsed) or 0.0) >= SNIPER_KILLCAM_SLOWDOWN_SFX_DELAY then
                current.slowdown_sfx_played = true
                play_sfx(self.general, SNIPER_KILLCAM_SLOWDOWN_SFX, 1.0)
            end
            if current.bullet_cam_sfx_played ~= true and
                (tonumber(current.elapsed) or 0.0) >= SNIPER_KILLCAM_BULLET_CAM_SFX_DELAY then
                current.bullet_cam_sfx_played = true
                play_sfx(self.general, SNIPER_KILLCAM_BULLET_CAM_SFX, 1.0)
            end
            if Input ~= nil and Input.GetKeyDown ~= nil and Input.GetKeyDown("Space") then
                current.skipped = true
                self:Stop("skipped")
            end
        end,
        on_end = function(current)
            if SniperKillCam ~= nil and SniperKillCam.Stop ~= nil then
                SniperKillCam.Stop()
            end
            if SniperKillCam ~= nil and SniperKillCam.ClearPendingBullets ~= nil then
                SniperKillCam.ClearPendingBullets()
            end
            if self.general ~= nil and self.general.Publish ~= nil then
                self.general:Publish("cutscene.presentation", {
                    active = false
                })
                self.general:Publish("cutscene.skip_prompt", {
                    visible = false,
                    text = ""
                })
            end
        end
    })
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
        duration = tonumber((payload or {}).duration) or tonumber(definition.duration) or 0.0
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
        self:PollSniperKillCam()
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

function CutSceneManager:PollSniperKillCam()
    if SniperKillCam == nil or SniperKillCam.ConsumePendingBulletId == nil then
        return
    end

    local bullet_id = SniperKillCam.ConsumePendingBulletId()
    if bullet_id == nil or bullet_id == 0 then
        return
    end

    self:Play("sniper_killcam", {
        bullet_id = bullet_id,
        duration = SNIPER_KILLCAM_DURATION,
        camera_mode = 0,
        profile = take_next_sniper_killcam_profile_id()
    })
end

return CutSceneManager
