local hud_widget = nil
local scope_visible = false
local spawned_count = 0
local compass_frame_count = 360
local compass_last_frame = -1
local compass_smooth_speed = 18.0
local smoothed_heading_degrees = nil
local sniper_pawn = nil
local breath_visible = false
local breath_bar_width = 288.0
local breath_last_width = -1.0
local breath_hide_delay = 3.0
local breath_hide_time_remaining = 0.0
local breath_fade_out_duration = 0.3
local breath_fade_out_time_remaining = 0.0
local breath_fade_elements = { "breathPanel", "breathLabel", "breathBarTrack", "breathBarFill" }
local breath_missing_pawn_warned = false

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[ScopeTest_UIManager] " .. message)
    else
        print("[ScopeTest_UIManager] " .. message)
    end
end

local function set_breath_group_alpha(widget, alpha)
    if widget == nil then
        return
    end

    for _, element_id in ipairs(breath_fade_elements) do
        widget:SetAlpha(element_id, alpha)
    end
end

local function ensure_hud()
    if hud_widget ~= nil then
        return hud_widget
    end

    if UI == nil or UI.CreateWidget == nil then
        log("UI.CreateWidget is unavailable")
        return nil
    end

    hud_widget = UI.CreateWidget("Content/UI/InGameHUD.rml")
    if hud_widget == nil then
        log("failed to create InGameHUD")
        return nil
    end

    if hud_widget.AddToViewportZ ~= nil then
        hud_widget:AddToViewportZ(10)
    else
        hud_widget:AddToViewport()
    end
    hud_widget:SetWantsMouse(false)
    hud_widget:SetWantsKeyboard(false)
    hud_widget:SetWantsTextInput(false)
    hud_widget:SetBlocksGameInput(false)
    hud_widget:SetBlocksGameKeyboard(false)
    hud_widget:SetBlocksGameMouseLook(false)
    hud_widget:SetAlpha("scopeOverlay", 0.0)
    hud_widget:SetVisible("scopeOverlay", false)
    hud_widget:SetAlpha("crosshairImage", 1.0)
    hud_widget:SetVisible("crosshairImage", true)
    set_breath_group_alpha(hud_widget, 0.0)
    hud_widget:SetVisible("breathPanel", false)
    hud_widget:SetElementStyle("breathLabel", "font-family", "\"Nexon\"")
    hud_widget:SetElementStyle("breathLabel", "font-weight", "400")
    hud_widget:SetElementStyle("breathLabel", "color", "rgba(255, 255, 255, 255)")
    if hud_widget.SetText ~= nil then
        hud_widget:SetText("breathLabel", "&#49704;&#52280;&#44592;")
    end
    hud_widget:SetElementStyle("breathBarFill", "width", "0px")
    if hud_widget.SetElementValue ~= nil then
        hud_widget:SetElementValue("breathProgress", "0")
    end
    scope_visible = false
    breath_visible = false
    breath_last_width = -1.0
    breath_hide_time_remaining = 0.0
    breath_fade_out_time_remaining = 0.0
    if hud_widget.SetImage ~= nil then
        hud_widget:SetImage("compassImage", "Image/Hor-Compass/Window/Compass_Window_000.png")
    else
        hud_widget:SetElementAttribute("compassImage", "src", "Image/Hor-Compass/Window/Compass_Window_000.png")
    end
    return hud_widget
end

local function set_scope_hud_visible(visible)
    if scope_visible == visible then
        return
    end

    scope_visible = visible
    local widget = ensure_hud()
    if widget == nil then
        return
    end

    if visible then
        widget:SetVisible("scopeOverlay", true)
        widget:SetAlpha("scopeOverlay", 1.0)
        widget:SetAlpha("crosshairImage", 0.0)
        widget:SetVisible("crosshairImage", false)
    else
        widget:SetAlpha("scopeOverlay", 0.0)
        widget:SetVisible("scopeOverlay", false)
        widget:SetVisible("crosshairImage", true)
        widget:SetAlpha("crosshairImage", 1.0)
    end
end

local function clamp01(value)
    if value == nil then
        return 0.0
    end
    if value < 0.0 then
        return 0.0
    end
    if value > 1.0 then
        return 1.0
    end
    return value
end

local function set_breath_bar_ratio(widget, ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)
    local width = math.floor(breath_bar_width * ratio + 0.5)
    if width ~= breath_last_width then
        breath_last_width = width
        widget:SetElementStyle("breathBarFill", "width", string.format("%dpx", width))
    end
    if widget.SetElementValue ~= nil then
        widget:SetElementValue("breathProgress", string.format("%.3f", ratio))
    end
end

local function get_sniper_pawn()
    if sniper_pawn ~= nil then
        local ok, is_valid = pcall(function()
            if sniper_pawn.IsValid ~= nil then
                return sniper_pawn:IsValid()
            end
            return true
        end)
        if ok and is_valid and sniper_pawn.GetHoldBreathGaugeRatio ~= nil then
            return sniper_pawn
        end
    end

    sniper_pawn = nil
    if World == nil or World.FindFirstActorByClass == nil then
        return nil
    end

    local actor = World.FindFirstActorByClass("ASniperPawn")
    if actor == nil then
        actor = World.FindFirstActorByClass("SniperPawn")
    end
    if actor == nil and World.FindActorByName ~= nil then
        actor = World.FindActorByName("ScopeTest_Player")
    end
    if actor == nil then
        return nil
    end

    if actor.AsSniperPawn ~= nil then
        local ok, casted = pcall(function()
            return actor:AsSniperPawn()
        end)
        if ok and casted ~= nil then
            sniper_pawn = casted
            return sniper_pawn
        end
    end

    if actor.GetHoldBreathGaugeRatio ~= nil then
        sniper_pawn = actor
        return sniper_pawn
    end

    return nil
end

local function set_breath_hud_visible(visible)
    local widget = ensure_hud()
    if widget == nil then
        breath_visible = visible
        return
    end

    if visible then
        breath_visible = true
        breath_fade_out_time_remaining = 0.0
        widget:SetVisible("breathPanel", true)
        set_breath_group_alpha(widget, 1.0)
    else
        if breath_visible then
            breath_visible = false
            breath_fade_out_time_remaining = breath_fade_out_duration
            return
        end

        breath_visible = false
        if breath_fade_out_time_remaining <= 0.0 then
            set_breath_group_alpha(widget, 0.0)
            widget:SetVisible("breathPanel", false)
        end
    end
end

local function update_breath_fade(dt)
    if breath_fade_out_time_remaining <= 0.0 then
        return
    end

    breath_fade_out_time_remaining = breath_fade_out_time_remaining - (dt or 0.0)
    if breath_fade_out_time_remaining <= 0.0 then
        breath_fade_out_time_remaining = 0.0
        local widget = ensure_hud()
        if widget ~= nil and not breath_visible then
            set_breath_group_alpha(widget, 0.0)
            widget:SetVisible("breathPanel", false)
        end
        return
    end

    local widget = ensure_hud()
    if widget ~= nil and not breath_visible then
        set_breath_group_alpha(widget, breath_fade_out_time_remaining / breath_fade_out_duration)
    end
end

local function is_raw_hold_breath_requested()
    if Input == nil or Input.GetKey == nil then
        return false
    end

    local scope_down = Input.GetKey("RightMouseButton")
    local shift_down =
        Input.GetKey("Shift") or
        Input.GetKey("LeftShift") or
        Input.GetKey("RightShift")
    return scope_down and shift_down
end

local function update_breath_hud(dt)
    local widget = ensure_hud()
    if widget == nil then
        return
    end

    local pawn = get_sniper_pawn()
    local requested = false
    local ratio = 0.0
    if pawn ~= nil then
        local active = pawn.IsHoldBreathActive ~= nil and pawn:IsHoldBreathActive()
        local scoped = pawn.IsScoped ~= nil and pawn:IsScoped()
        local held = pawn.IsHoldBreathInputHeld ~= nil and pawn:IsHoldBreathInputHeld()
        requested = active or (scoped and held)

        if pawn.GetHoldBreathGaugeRatio ~= nil then
            ratio = clamp01(pawn:GetHoldBreathGaugeRatio())
        end
    elseif is_raw_hold_breath_requested() then
        requested = true
        ratio = 1.0
        if not breath_missing_pawn_warned then
            breath_missing_pawn_warned = true
            log("SniperPawn binding unavailable; showing fallback hold-breath HUD")
        end
    end

    if not requested then
        if breath_visible and breath_hide_time_remaining > 0.0 then
            breath_hide_time_remaining = breath_hide_time_remaining - (dt or 0.0)
            if breath_hide_time_remaining > 0.0 then
                set_breath_hud_visible(true)
                set_breath_bar_ratio(widget, ratio)
                update_breath_fade(dt)
                return
            end
        end

        breath_hide_time_remaining = 0.0
        breath_last_width = -1.0
        set_breath_hud_visible(false)
        update_breath_fade(dt)
        return
    end

    breath_hide_time_remaining = breath_hide_delay
    set_breath_hud_visible(true)
    set_breath_bar_ratio(widget, ratio)
end

local function normalize_degrees(degrees)
    local result = degrees % 360.0
    if result < 0.0 then
        result = result + 360.0
    end
    return result
end

local function shortest_angle_delta(from_degrees, to_degrees)
    local delta = normalize_degrees(to_degrees - from_degrees)
    if delta > 180.0 then
        delta = delta - 360.0
    end
    return delta
end

local function smooth_heading(current_degrees, target_degrees, dt)
    if current_degrees == nil then
        return target_degrees
    end

    local alpha = 1.0
    if dt ~= nil and dt > 0.0 then
        alpha = 1.0 - math.exp(-compass_smooth_speed * dt)
    end
    if alpha < 0.0 then
        alpha = 0.0
    elseif alpha > 1.0 then
        alpha = 1.0
    end

    return normalize_degrees(current_degrees + shortest_angle_delta(current_degrees, target_degrees) * alpha)
end

local function atan2_degrees(y, x)
    if math.atan2 ~= nil then
        return math.deg(math.atan2(y, x))
    end

    if x > 0.0 then
        return math.deg(math.atan(y / x))
    end
    if x < 0.0 and y >= 0.0 then
        return math.deg(math.atan(y / x)) + 180.0
    end
    if x < 0.0 and y < 0.0 then
        return math.deg(math.atan(y / x)) - 180.0
    end
    if y > 0.0 then
        return 90.0
    end
    if y < 0.0 then
        return -90.0
    end
    return 0.0
end

local function get_heading_degrees()
    if obj == nil or obj.Forward == nil then
        return 0.0
    end

    local forward = obj.Forward
    local x = forward.X or forward.x or 1.0
    local y = forward.Y or forward.y or 0.0
    if math.abs(x) < 0.0001 and math.abs(y) < 0.0001 then
        return 0.0
    end

    return normalize_degrees(atan2_degrees(y, x))
end

local function update_compass(dt)
    local widget = ensure_hud()
    if widget == nil then
        return
    end

    smoothed_heading_degrees = smooth_heading(smoothed_heading_degrees, get_heading_degrees(), dt)
    local heading = smoothed_heading_degrees
    local frame = math.floor(normalize_degrees(heading) + 0.5) % compass_frame_count
    if frame ~= compass_last_frame then
        compass_last_frame = frame
        local path = string.format("Image/Hor-Compass/Window/Compass_Window_%03d.png", frame)
        if widget.SetImage ~= nil then
            widget:SetImage("compassImage", path)
        else
            widget:SetElementAttribute("compassImage", "src", path)
        end
    end
end

local function spawn_empty_marker()
    if World == nil or World.SpawnActor == nil then
        log("World.SpawnActor is unavailable")
        return
    end

    local base_location = obj.Location
    local forward = obj.Forward
    local spawn_location = base_location + forward * (2.0 + spawned_count * 0.5)
    spawned_count = spawned_count + 1

    local actor = World.SpawnActor("AActor", spawn_location, Vec3(0.0, 0.0, 0.0), Vec3(1.0, 1.0, 1.0))
    if actor ~= nil then
        if actor.AddTag ~= nil then
            actor:AddTag("ScopeTest_EmptySpawn")
        end
        log("spawned empty actor " .. tostring(spawned_count))
    end
end

function BeginPlay()
    log("BeginPlay " .. tostring(obj and obj.UUID or ""))
    ensure_hud()
end

function EndPlay()
    if hud_widget ~= nil then
        hud_widget:RemoveFromParent()
        hud_widget = nil
    end
    scope_visible = false
    breath_visible = false
    breath_last_width = -1.0
    breath_hide_time_remaining = 0.0
    breath_fade_out_time_remaining = 0.0
    sniper_pawn = nil
end

function Tick(dt)
    update_compass(dt)

    if Input and Input.GetKeyDown and Input.GetKeyDown("RightMouseButton") then
        local ok, err = pcall(spawn_empty_marker)
        if not ok then
            log("spawn_empty_marker failed: " .. tostring(err))
        end
    end

    local scope_down = false
    if Input and Input.GetKey and Input.GetKey("RightMouseButton") then
        scope_down = true
    end

    set_scope_hud_visible(scope_down)
    update_breath_hud(dt)
end
