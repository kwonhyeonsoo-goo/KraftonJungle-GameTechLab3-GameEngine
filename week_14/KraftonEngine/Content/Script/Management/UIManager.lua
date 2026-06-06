local GameState = require("Management/GameState")

local UIManager = {}
UIManager.__index = UIManager

local MAIN_HUD_MODE = "Main"
local PRE_INGAME_HUD_MODE = "PreInGame"
local LOADING_HUD_MODE = "Loading"
local IN_GAME_HUD_MODE = "InGame"
local DEFAULT_LOADING_TIP = "Tip: Hold your breath only when the shot really matters."
local MAIN_MENU_BUTTON_IDS = { "btnGameStart", "btnScoreBoard", "btnSettings", "btnCredits", "btnExit" }
local MAIN_MENU_BUTTON_TEXTS = {
    { button = "btnGameStart", label = "btnGameStartLabel", text = "Game Start" },
    { button = "btnScoreBoard", label = "btnScoreBoardLabel", text = "Score Board" },
    { button = "btnSettings", label = "btnSettingsLabel", text = "Settings" },
    { button = "btnCredits", label = "btnCreditsLabel", text = "Credits" },
    { button = "btnExit", label = "btnExitLabel", text = "Exit" }
}
local MAIN_MENU_BUTTON_LABEL_IDS = {
    "btnGameStartLabel",
    "btnScoreBoardLabel",
    "btnSettingsLabel",
    "btnCreditsLabel",
    "btnExitLabel"
}
local MAIN_MENU_FADE_IDS = {
    "mainMenu",
    "btnGameStart",
    "btnScoreBoard",
    "btnSettings",
    "btnCredits",
    "btnExit",
    "btnGameStartLabel",
    "btnScoreBoardLabel",
    "btnSettingsLabel",
    "btnCreditsLabel",
    "btnExitLabel"
}
local MAIN_MENU_BUTTON_TEXT_COLOR = { r = 226, g = 232, b = 235, a = 230 }
local MAIN_BUTTON_HOVER_SFX = "SFX/ButtonHovering.mp3"
local MAIN_BUTTON_CLICK_SFX = "SFX/ButtonClickMain.mp3"
local MAIN_GAME_START_SFX = "SFX/ButtonClickGameStart.mp3"
local LOADING_END_SFX = "SFX/LoadingEnd.mp3"

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[UIManager] " .. message)
    else
        print("[UIManager] " .. message)
    end
end

local function call_widget(widget, method_name, ...)
    if widget ~= nil and widget[method_name] ~= nil then
        return widget[method_name](widget, ...)
    end
    return nil
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

local function rgba255(color, alpha_scale)
    alpha_scale = clamp01(alpha_scale)
    local alpha = math.floor((color.a or 255) * alpha_scale + 0.5)
    return string.format("rgba(%d, %d, %d, %d)", color.r or 255, color.g or 255, color.b or 255, alpha)
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

local function smooth_heading(current_degrees, target_degrees, dt, speed)
    if current_degrees == nil then
        return target_degrees
    end

    local alpha = 1.0
    if dt ~= nil and dt > 0.0 then
        alpha = 1.0 - math.exp(-speed * dt)
    end
    return normalize_degrees(current_degrees + shortest_angle_delta(current_degrees, target_degrees) * clamp01(alpha))
end

local function enum_equals(value, expected)
    if value == nil or expected == nil then
        return false
    end

    local ok, result = pcall(function()
        return value == expected
    end)
    return ok and result == true
end

local function read_number_method(target, method_names)
    if target == nil then
        return nil
    end

    for _, method_name in ipairs(method_names) do
        if target[method_name] ~= nil then
            local ok, value = pcall(function()
                return target[method_name](target)
            end)
            if ok and value ~= nil then
                local number = tonumber(value)
                if number ~= nil then
                    return math.max(0, math.floor(number + 0.5))
                end
            end
        end
    end

    return nil
end

local function read_string_method(target, method_names)
    if target == nil then
        return nil
    end

    for _, method_name in ipairs(method_names) do
        if target[method_name] ~= nil then
            local ok, value = pcall(function()
                return target[method_name](target)
            end)
            if ok and value ~= nil then
                local text = tostring(value)
                if text ~= "" then
                    return text
                end
            end
        end
    end

    return nil
end

function UIManager.new(general)
    return setmetatable({
        general = general,
        widgets = {},
        current_hud = nil,
        active_hud_mode = nil,
        main_start_pending = false,
        main_start_elapsed = 0.0,
        main_start_duration = 2.0,
        main_state_requested = false,
        scope_visible = false,
        compass_frame_count = 360,
        compass_last_frame = -1,
        compass_smooth_speed = 18.0,
        smoothed_heading_degrees = nil,
        sniper_pawn = nil,
        breath_visible = false,
        breath_bar_width = 288.0,
        breath_last_width = -1.0,
        breath_hide_delay = 3.0,
        breath_hide_time_remaining = 0.0,
        breath_fade_out_duration = 0.3,
        breath_fade_out_time_remaining = 0.0,
        breath_fade_elements = { "breathPanel", "breathLabel", "breathBarTrack", "breathBarFill" },
        breath_missing_pawn_warned = false,
        weapon_last_name = nil,
        weapon_last_ammo_text = nil,
        weapon_last_ammo_type = nil
    }, UIManager)
end

function UIManager:Initialize()
    self.general:Subscribe("scene.hud_requested", self, function(payload)
        self:ApplySceneHUDRequest(payload)
    end)

    self.general:Subscribe("scene.exiting", self, function(payload)
        if payload ~= nil and payload.from == "InGame" then
            self:ResetInGameHUDRuntime(true)
        end
    end)

    self.general:Subscribe("loading.ready", self, function(payload)
        self:SetLoadingReady(payload)
    end)

    self.general:Subscribe("preingame.reset", self, function(payload)
        self:ResetPreInGameHUD(payload)
    end)

    self.general:Subscribe("preingame.sheet_update", self, function(payload)
        self:ApplyPreInGameSheet(payload)
    end)

    self.general:Subscribe("preingame.ready", self, function(payload)
        self:SetPreInGameReady(payload)
    end)

    self.general:Subscribe("preingame.approved", self, function(payload)
        self:ShowPreInGameApproved(payload)
    end)

    self.general:Subscribe("preingame.skip_prompt_alpha", self, function(payload)
        self:SetPreInGameSkipPromptAlpha(payload)
    end)

    self.general:Subscribe("preingame.subtitle", self, function(payload)
        self:SetPreInGameSubtitle(payload)
    end)
end

function UIManager:Shutdown()
    self.general:UnsubscribeOwner(self)
    self:Clear()
    self:ResetInGameHUDRuntime(true)
    if UI ~= nil and UI.ClearViewport ~= nil then
        UI.ClearViewport()
    end
end

function UIManager:Tick(dt)
    if self.active_hud_mode == MAIN_HUD_MODE then
        self:TickMainHUD(dt or 0.0)
    elseif self.active_hud_mode == IN_GAME_HUD_MODE then
        self:TickInGameHUD(dt or 0.0)
    end
end

function UIManager:CreateWidget(name, path, z_order)
    if UI == nil or UI.CreateWidget == nil then
        log("UI.CreateWidget is unavailable")
        return nil
    end

    local existing = self.widgets[name]
    if existing ~= nil then
        log("reuse widget name=" .. tostring(name) .. " path=" .. tostring(path))
        return existing
    end

    local widget = UI.CreateWidget(path)
    if widget == nil then
        log("failed to create widget: " .. tostring(path))
        return nil
    end

    if widget.AddToViewportZ ~= nil then
        widget:AddToViewportZ(z_order or 0)
    else
        widget:AddToViewport()
    end

    call_widget(widget, "SetWantsMouse", false)
    call_widget(widget, "SetWantsKeyboard", false)
    call_widget(widget, "SetWantsTextInput", false)
    call_widget(widget, "SetBlocksGameInput", false)
    call_widget(widget, "SetBlocksGameKeyboard", false)
    call_widget(widget, "SetBlocksGameMouseLook", false)

    self.widgets[name] = widget
    log("created widget name=" .. tostring(name) .. " path=" .. tostring(path))
    self.general:Publish("ui.widget_created", { name = name, path = path, widget = widget })
    return widget
end

function UIManager:GetWidget(name)
    return self.widgets[name]
end

function UIManager:RemoveWidget(name)
    local widget = self.widgets[name]
    if widget == nil then
        return false
    end

    call_widget(widget, "RemoveFromParent")
    self.widgets[name] = nil
    if self.current_hud ~= nil and self.current_hud.name == name then
        self.current_hud = nil
        self.active_hud_mode = nil
    end
    self.general:Publish("ui.widget_removed", { name = name })
    return true
end

function UIManager:Clear()
    local names = {}
    for name, _ in pairs(self.widgets) do
        table.insert(names, name)
    end

    for _, name in ipairs(names) do
        self:RemoveWidget(name)
    end

    self.current_hud = nil
    self.active_hud_mode = nil
    self.main_start_pending = false
    self.main_start_elapsed = 0.0
    self.main_state_requested = false
end

function UIManager:ApplySceneHUDRequest(payload)
    local hud = payload and payload.hud
    if hud == nil then
        log("clear HUD request")
        self:Clear()
        return
    end

    local name = hud.name or "HUD"
    local path = hud.path
    log("HUD request state=" .. tostring(payload and payload.state) ..
        " name=" .. tostring(name) .. " path=" .. tostring(path) ..
        " mode=" .. tostring(hud.mode))
    if path == nil or path == "" then
        self:Clear()
        return
    end

    if self.current_hud ~= nil and self.current_hud.name ~= name then
        self:RemoveWidget(self.current_hud.name)
    end

    local widget = self:CreateWidget(name, path, hud.z_order or 0)
    if widget == nil then
        self.current_hud = nil
        self.active_hud_mode = nil
        return
    end

    self.current_hud = {
        name = name,
        path = path,
        widget = widget,
        mode = hud.mode
    }
    self.active_hud_mode = hud.mode

    if self.active_hud_mode == MAIN_HUD_MODE then
        self:ConfigureMainHUD(widget)
    end

    if self.active_hud_mode == PRE_INGAME_HUD_MODE then
        self:ConfigurePreInGameHUD(widget)
    end

    if self.active_hud_mode == LOADING_HUD_MODE then
        self:ConfigureLoadingHUD(widget, payload and payload.payload)
    end

    if self.active_hud_mode == IN_GAME_HUD_MODE then
        self:ConfigureInGameHUD(widget)
    end
end

function UIManager:GetActiveHUDWidget()
    if self.current_hud == nil then
        return nil
    end
    return self.current_hud.widget
end

function UIManager:SetElementAlpha(widget, element_id, alpha)
    call_widget(widget, "SetAlpha", element_id, alpha)
end

function UIManager:SetElementVisible(widget, element_id, visible)
    call_widget(widget, "SetVisible", element_id, visible)
end

function UIManager:SetElementStyle(widget, element_id, property, value)
    call_widget(widget, "SetElementStyle", element_id, property, value)
end

function UIManager:HasElement(widget, element_id)
    if widget == nil or widget.HasElement == nil then
        return false
    end
    local ok, result = pcall(function()
        return widget:HasElement(element_id)
    end)
    return ok and result == true
end

function UIManager:SetMainMenuTextVisible(widget, visible)
    if widget == nil then
        return
    end

    for _, item in ipairs(MAIN_MENU_BUTTON_TEXTS) do
        local text = visible and item.text or ""
        if self:HasElement(widget, item.label) then
            call_widget(widget, "SetText", item.label, text)
        else
            call_widget(widget, "SetText", item.button, text)
        end
    end
end

function UIManager:SetMainMenuAlpha(widget, alpha)
    alpha = clamp01(alpha)
    for _, element_id in ipairs(MAIN_MENU_FADE_IDS) do
        self:SetElementAlpha(widget, element_id, alpha)
    end
end

function UIManager:SetMainMenuVisible(widget, visible)
    self:SetElementVisible(widget, "mainMenu", visible)
    for _, element_id in ipairs(MAIN_MENU_BUTTON_IDS) do
        self:SetElementVisible(widget, element_id, visible)
    end
    for _, element_id in ipairs(MAIN_MENU_BUTTON_LABEL_IDS) do
        self:SetElementVisible(widget, element_id, visible)
    end
    self:SetMainMenuTextVisible(widget, visible)
end

function UIManager:SetElementImage(widget, element_id, path)
    if widget == nil then
        return
    end

    if widget.SetImage ~= nil then
        widget:SetImage(element_id, path)
    else
        call_widget(widget, "SetElementAttribute", element_id, "src", path)
    end
end

function UIManager:ConfigureMainHUD(widget)
    self.main_start_pending = false
    self.main_start_elapsed = 0.0
    self.main_state_requested = false

    call_widget(widget, "SetWantsMouse", true)
    call_widget(widget, "SetBlocksGameMouseLook", true)
    call_widget(widget, "SetBlocksGameInput", true)
    self:SetMainMenuVisible(widget, true)
    self:SetMainMenuTextVisible(widget, true)
    self:SetMainMenuAlpha(widget, 1.0)
    self:SetMainMenuButtonsEnabled(widget, true)

    self:ConfigureMainButtonActions(widget)
    if widget ~= nil and widget.bind_click ~= nil then
        widget:bind_click("btnGameStart", function()
            self:BeginMainStartTransition()
        end)
    end
end

function UIManager:ConfigureMainButtonActions(widget)
    if widget == nil then
        return
    end

    for _, element_id in ipairs(MAIN_MENU_BUTTON_IDS) do
        local click_action = element_id == "btnGameStart" and "GameStart" or "MainButtonClick"
        call_widget(widget, "SetActionEvent", element_id, click_action)
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end
end

function UIManager:PlayUISFX(path, volume)
    if self.general ~= nil and self.general.PlaySFX ~= nil then
        self.general:PlaySFX(path, volume or 1.0)
        return
    end

    if AudioManager ~= nil and AudioManager.PlaySFX ~= nil then
        AudioManager.PlaySFX(path, volume or 1.0)
    end
end

function UIManager:PollMainActions(widget)
    if widget == nil or widget.PollActionEvents == nil then
        return
    end

    local ok, events = pcall(function()
        return widget:PollActionEvents()
    end)
    if not ok or events == nil then
        log("Main HUD action polling failed")
        return
    end

    local hover_played = false
    for _, action in ipairs(events) do
        if action == "GameStart" or action == "StartGame" then
            self:BeginMainStartTransition()
        elseif action == "MainButtonClick" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 1.0)
        elseif action == "MainButtonHover" and not hover_played then
            hover_played = true
            self:PlayUISFX(MAIN_BUTTON_HOVER_SFX, 0.8)
        end
    end
end

function UIManager:TickMainHUD(dt)
    local widget = self:GetActiveHUDWidget()
    self:PollMainActions(widget)

    if not self.main_start_pending then
        return
    end

    self.main_start_elapsed = self.main_start_elapsed + (dt or 0.0)
    local alpha = 1.0 - clamp01(self.main_start_elapsed / self.main_start_duration)
    if widget ~= nil then
        self:SetMainMenuAlpha(widget, alpha)
        if alpha <= 0.0 then
            self:SetMainMenuVisible(widget, false)
        end
    else
        log("Main fade tick has no active HUD widget")
    end

    if self.main_start_elapsed >= self.main_start_duration and not self.main_state_requested then
        self.main_state_requested = true
        log("Main fade completed; requesting PreInGame state")
        if self.general ~= nil and self.general.RequestState ~= nil then
            local ok = self.general:RequestState(GameState.PreInGame, { reason = "main_game_start" })
            log("PreInGame state request result=" .. tostring(ok))
        else
            log("PreInGame state request failed: GeneralManager.RequestState unavailable")
        end
    end
end

function UIManager:BeginMainStartTransition()
    if self.main_start_pending then
        return
    end

    log("Game Start clicked; fading main menu before PreInGame")
    self.main_start_pending = true
    self.main_start_elapsed = 0.0
    self.main_state_requested = false
    self:PlayUISFX(MAIN_GAME_START_SFX, 1.0)
    if self.general ~= nil and self.general.Publish ~= nil then
        self.general:Publish("main.game_start_requested", { reason = "button" })
    end

    local widget = self:GetActiveHUDWidget()
    if widget ~= nil then
        log("Main menu fade started")
    else
        log("Game Start transition has no active HUD widget")
    end
end

function UIManager:SetMainMenuButtonsEnabled(widget, enabled)
    for _, element_id in ipairs(MAIN_MENU_BUTTON_IDS) do
        call_widget(widget, "SetElementEnabled", element_id, enabled)
        if enabled then
            call_widget(widget, "RemoveElementAttribute", element_id, "disabled")
        else
            call_widget(widget, "SetElementAttribute", element_id, "disabled", "true")
        end
    end
end

function UIManager:ConfigurePreInGameHUD(widget)
    call_widget(widget, "SetWantsMouse", false)
    call_widget(widget, "SetBlocksGameMouseLook", false)
    call_widget(widget, "SetBlocksGameInput", false)
    self:ResetPreInGameHUD({ sheet_count = 6 })
end

function UIManager:ResetPreInGameHUD(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local count = payload and payload.sheet_count or 6
    for index = 1, count do
        local element_id = "sheet" .. tostring(index)
        self:SetElementVisible(widget, element_id, false)
        self:SetElementAlpha(widget, element_id, 0.0)
        self:SetElementStyle(widget, element_id, "left", "600px")
        self:SetElementStyle(widget, element_id, "top", "-420px")
        self:SetElementStyle(widget, element_id, "width", "720px")
        self:SetElementStyle(widget, element_id, "height", "540px")
        self:SetElementStyle(widget, element_id, "transform", "scale(2.325)")
    end

    self:SetElementVisible(widget, "approvedStamp", false)
    self:SetElementAlpha(widget, "approvedStamp", 0.0)
    self:SetElementVisible(widget, "newsSubtitle", false)
    self:SetElementAlpha(widget, "newsSubtitle", 0.0)
    call_widget(widget, "SetText", "newsSubtitle", "")
    self:SetElementVisible(widget, "skipPrompt", false)
    self:SetElementAlpha(widget, "skipPrompt", 0.0)
end

function UIManager:ApplyPreInGameSheet(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE or payload == nil or payload.element_id == nil then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local element_id = payload.element_id
    self:SetElementVisible(widget, element_id, true)
    self:SetElementAlpha(widget, element_id, clamp01(payload.alpha or 1.0))
    self:SetElementStyle(widget, element_id, "left", string.format("%.3fpx", payload.left or 600.0))
    self:SetElementStyle(widget, element_id, "top", string.format("%.3fpx", payload.top or 270.0))
    self:SetElementStyle(widget, element_id, "width", string.format("%.3fpx", payload.width or 720.0))
    self:SetElementStyle(widget, element_id, "height", string.format("%.3fpx", payload.height or 540.0))
    self:SetElementStyle(
        widget,
        element_id,
        "transform",
        string.format("rotate(%.3fdeg) scale(%.3f)", payload.rotation or 0.0, payload.scale or 1.0))
end

function UIManager:SetPreInGameReady(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    self:SetElementVisible(widget, "skipPrompt", true)
    self:SetElementAlpha(widget, "skipPrompt", 1.0)
end

function UIManager:SetPreInGameSubtitle(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local text = payload and payload.text or ""
    local visible = text ~= ""
    local text_length = string.len(text)
    local font_size = 24
    if text_length > 260 then
        font_size = 17
    elseif text_length > 190 then
        font_size = 19
    elseif text_length > 130 then
        font_size = 21
    end
    call_widget(widget, "SetText", "newsSubtitle", text)
    self:SetElementStyle(widget, "newsSubtitle", "font-size", tostring(font_size) .. "px")
    self:SetElementVisible(widget, "newsSubtitle", visible)
    self:SetElementAlpha(widget, "newsSubtitle", visible and 1.0 or 0.0)
end

function UIManager:ShowPreInGameApproved(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    self:SetElementVisible(widget, "approvedStamp", true)
    self:SetElementAlpha(widget, "approvedStamp", 1.0)
    self:SetElementStyle(widget, "approvedStamp", "left", "540px")
    self:SetElementStyle(widget, "approvedStamp", "top", "453px")
    self:SetElementVisible(widget, "skipPrompt", true)
end

function UIManager:SetPreInGameSkipPromptAlpha(payload)
    if self.active_hud_mode ~= PRE_INGAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local alpha = clamp01(payload and payload.alpha or 0.0)
    self:SetElementVisible(widget, "skipPrompt", alpha > 0.001)
    self:SetElementAlpha(widget, "skipPrompt", alpha)
end

function UIManager:ConfigureLoadingHUD(widget, payload)
    payload = payload or {}
    call_widget(widget, "SetWantsMouse", false)
    call_widget(widget, "SetBlocksGameMouseLook", false)
    call_widget(widget, "SetBlocksGameInput", false)

    call_widget(widget, "SetText", "loadingTitle", "Loading")
    call_widget(widget, "SetText", "loadingTip", payload.tip or DEFAULT_LOADING_TIP)
    call_widget(widget, "SetText", "pressPrompt", "Press Space to Play")
    self:SetElementAlpha(widget, "pressPrompt", 0.0)
    self:SetElementVisible(widget, "pressPrompt", false)
end

function UIManager:SetLoadingReady(payload)
    if self.active_hud_mode ~= LOADING_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    if payload ~= nil and payload.tip ~= nil then
        call_widget(widget, "SetText", "loadingTip", payload.tip)
    end
    self:SetElementVisible(widget, "pressPrompt", true)
    self:SetElementAlpha(widget, "pressPrompt", 1.0)
    self:PlayUISFX(LOADING_END_SFX, 1.0)
end

function UIManager:SetBreathGroupAlpha(widget, alpha)
    if widget == nil then
        return
    end

    for _, element_id in ipairs(self.breath_fade_elements) do
        self:SetElementAlpha(widget, element_id, alpha)
    end
end

function UIManager:ConfigureInGameHUD(widget)
    self:ResetInGameHUDRuntime(true)

    self:SetElementAlpha(widget, "scopeOverlay", 0.0)
    self:SetElementVisible(widget, "scopeOverlay", false)
    self:SetElementAlpha(widget, "crosshairImage", 1.0)
    self:SetElementVisible(widget, "crosshairImage", true)

    self:SetBreathGroupAlpha(widget, 0.0)
    self:SetElementVisible(widget, "breathPanel", false)
    self:SetElementStyle(widget, "breathLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "breathLabel", "font-weight", "400")
    self:SetElementStyle(widget, "breathLabel", "color", "rgba(255, 255, 255, 255)")
    call_widget(widget, "SetText", "breathLabel", "&#49704;&#52280;&#44592;")
    self:SetElementStyle(widget, "breathBarFill", "width", "0px")
    call_widget(widget, "SetElementValue", "breathProgress", "0")

    self:SetElementVisible(widget, "weaponInfoPanel", true)
    self:SetElementAlpha(widget, "weaponInfoPanel", 1.0)
    self:SetElementStyle(widget, "weaponNameLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "weaponNameLabel", "font-weight", "bold")
    self:SetElementStyle(widget, "ammoCountLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "ammoCountLabel", "font-weight", "bold")
    self:SetElementStyle(widget, "ammoTypeLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "ammoTypeLabel", "font-weight", "bold")
    self:SetElementStyle(widget, "weaponNameLabel", "color", "rgba(255, 255, 255, 255)")
    self:SetElementStyle(widget, "ammoTypeLabel", "color", "rgba(255, 255, 255, 255)")
    self:UpdateWeaponHUD(true)

    self:SetElementImage(widget, "compassImage", "Image/Hor-Compass/Window/Compass_Window_000.png")
end

function UIManager:ResetInGameHUDRuntime(clear_pawn)
    self.scope_visible = false
    self.compass_last_frame = -1
    self.smoothed_heading_degrees = nil
    self.breath_visible = false
    self.breath_last_width = -1.0
    self.breath_hide_time_remaining = 0.0
    self.breath_fade_out_time_remaining = 0.0
    self.breath_missing_pawn_warned = false
    self.weapon_last_name = nil
    self.weapon_last_ammo_text = nil
    self.weapon_last_ammo_type = nil
    if clear_pawn then
        self.sniper_pawn = nil
    end
end

function UIManager:GetSniperPawn()
    if self.sniper_pawn ~= nil then
        local ok, is_valid = pcall(function()
            if self.sniper_pawn.IsValid ~= nil then
                return self.sniper_pawn:IsValid()
            end
            return true
        end)
        if ok and is_valid and self.sniper_pawn.GetHoldBreathGaugeRatio ~= nil then
            return self.sniper_pawn
        end
    end

    self.sniper_pawn = nil
    if World == nil then
        return nil
    end

    local actor = nil
    if World.FindFirstActorByClass ~= nil then
        actor = World.FindFirstActorByClass("ASniperPawn")
        if actor == nil then
            actor = World.FindFirstActorByClass("SniperPawn")
        end
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
            self.sniper_pawn = casted
            return self.sniper_pawn
        end
    end

    if actor.GetHoldBreathGaugeRatio ~= nil then
        self.sniper_pawn = actor
        return self.sniper_pawn
    end

    return nil
end

function UIManager:GetSniperWeaponComponent()
    local pawn = self:GetSniperPawn()
    if pawn == nil or pawn.GetSniperWeaponComponent == nil then
        return nil
    end

    local ok, weapon = pcall(function()
        return pawn:GetSniperWeaponComponent()
    end)
    if ok then
        return weapon
    end
    return nil
end

function UIManager:FormatAmmoTypeName(ammo_type)
    if SniperAmmoType ~= nil then
        if enum_equals(ammo_type, SniperAmmoType.AntiMaterial) then
            return "ANTI-MATERIAL"
        end
        if enum_equals(ammo_type, SniperAmmoType.Normal) then
            return "NORMAL"
        end
    end

    local numeric = tonumber(ammo_type)
    if numeric == 1 then
        return "ANTI-MATERIAL"
    end
    if numeric == 0 then
        return "NORMAL"
    end

    local text = string.lower(tostring(ammo_type or ""))
    if string.find(text, "anti", 1, true) ~= nil then
        return "ANTI-MATERIAL"
    end
    if string.find(text, "normal", 1, true) ~= nil then
        return "NORMAL"
    end
    return "NORMAL"
end

function UIManager:GetWeaponHUDSnapshot()
    local weapon = self:GetSniperWeaponComponent()
    local weapon_name = "SNIPER RIFLE"
    local ammo_type_name = "NORMAL"
    local current_ammo = nil
    local total_ammo = nil

    if weapon ~= nil then
        weapon_name = read_string_method(weapon, {
            "GetWeaponDisplayName",
            "GetDisplayName",
            "GetCurrentWeaponName",
            "GetWeaponName"
        }) or weapon_name

        if weapon.GetCurrentAmmoType ~= nil then
            local ok, ammo_type = pcall(function()
                return weapon:GetCurrentAmmoType()
            end)
            if ok then
                ammo_type_name = self:FormatAmmoTypeName(ammo_type)
            end
        end

        current_ammo = read_number_method(weapon, {
            "GetCurrentAmmoCount",
            "GetRemainingAmmo",
            "GetAmmoInMagazine",
            "GetCurrentAmmo",
            "GetClipAmmo"
        })
        total_ammo = read_number_method(weapon, {
            "GetTotalAmmoCount",
            "GetTotalAmmo",
            "GetReserveAmmo",
            "GetMaxAmmoCount",
            "GetMaxAmmo"
        })
    end

    local ammo_text = "00 / 00"
    if current_ammo ~= nil or total_ammo ~= nil then
        ammo_text = string.format("%02d / %02d", current_ammo or 0, total_ammo or 0)
    end

    return {
        weapon_name = weapon_name,
        ammo_text = ammo_text,
        ammo_type_name = "AMMO TYPE  " .. ammo_type_name
    }
end

function UIManager:UpdateWeaponHUD(force)
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local snapshot = self:GetWeaponHUDSnapshot()
    if force or snapshot.weapon_name ~= self.weapon_last_name then
        self.weapon_last_name = snapshot.weapon_name
        call_widget(widget, "SetText", "weaponNameLabel", snapshot.weapon_name)
    end

    if force or snapshot.ammo_text ~= self.weapon_last_ammo_text then
        self.weapon_last_ammo_text = snapshot.ammo_text
        call_widget(widget, "SetText", "ammoCountLabel", snapshot.ammo_text)
    end

    if force or snapshot.ammo_type_name ~= self.weapon_last_ammo_type then
        self.weapon_last_ammo_type = snapshot.ammo_type_name
        call_widget(widget, "SetText", "ammoTypeLabel", snapshot.ammo_type_name)
    end
end

function UIManager:IsRawHoldBreathRequested()
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

function UIManager:GetScopeVisibleFromInputOrPawn()
    local pawn = self:GetSniperPawn()
    if pawn ~= nil and pawn.IsScoped ~= nil then
        return pawn:IsScoped()
    end

    return Input ~= nil and Input.GetKey ~= nil and Input.GetKey("RightMouseButton") == true
end

function UIManager:SetScopeHUDVisible(visible)
    local widget = self:GetActiveHUDWidget()
    if widget == nil or self.scope_visible == visible then
        return
    end

    self.scope_visible = visible
    if visible then
        self:SetElementVisible(widget, "scopeOverlay", true)
        self:SetElementAlpha(widget, "scopeOverlay", 1.0)
        self:SetElementAlpha(widget, "crosshairImage", 0.0)
        self:SetElementVisible(widget, "crosshairImage", false)
    else
        self:SetElementAlpha(widget, "scopeOverlay", 0.0)
        self:SetElementVisible(widget, "scopeOverlay", false)
        self:SetElementVisible(widget, "crosshairImage", true)
        self:SetElementAlpha(widget, "crosshairImage", 1.0)
    end
end

function UIManager:SetBreathBarRatio(widget, ratio)
    if widget == nil then
        return
    end

    ratio = clamp01(ratio)
    local width = math.floor(self.breath_bar_width * ratio + 0.5)
    if width ~= self.breath_last_width then
        self.breath_last_width = width
        self:SetElementStyle(widget, "breathBarFill", "width", string.format("%dpx", width))
    end
    call_widget(widget, "SetElementValue", "breathProgress", string.format("%.3f", ratio))
end

function UIManager:SetBreathHUDVisible(visible)
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        self.breath_visible = visible
        return
    end

    if visible then
        self.breath_visible = true
        self.breath_fade_out_time_remaining = 0.0
        self:SetElementVisible(widget, "breathPanel", true)
        self:SetBreathGroupAlpha(widget, 1.0)
    else
        if self.breath_visible then
            self.breath_visible = false
            self.breath_fade_out_time_remaining = self.breath_fade_out_duration
            return
        end

        self.breath_visible = false
        if self.breath_fade_out_time_remaining <= 0.0 then
            self:SetBreathGroupAlpha(widget, 0.0)
            self:SetElementVisible(widget, "breathPanel", false)
        end
    end
end

function UIManager:UpdateBreathFade(dt)
    if self.breath_fade_out_time_remaining <= 0.0 then
        return
    end

    self.breath_fade_out_time_remaining = self.breath_fade_out_time_remaining - (dt or 0.0)
    local widget = self:GetActiveHUDWidget()
    if self.breath_fade_out_time_remaining <= 0.0 then
        self.breath_fade_out_time_remaining = 0.0
        if widget ~= nil and not self.breath_visible then
            self:SetBreathGroupAlpha(widget, 0.0)
            self:SetElementVisible(widget, "breathPanel", false)
        end
        return
    end

    if widget ~= nil and not self.breath_visible then
        self:SetBreathGroupAlpha(widget, self.breath_fade_out_time_remaining / self.breath_fade_out_duration)
    end
end

function UIManager:UpdateBreathHUD(dt)
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local pawn = self:GetSniperPawn()
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
    elseif self:IsRawHoldBreathRequested() then
        requested = true
        ratio = 1.0
        if not self.breath_missing_pawn_warned then
            self.breath_missing_pawn_warned = true
            log("SniperPawn binding unavailable; showing fallback hold-breath HUD")
        end
    end

    if not requested then
        if self.breath_visible and self.breath_hide_time_remaining > 0.0 then
            self.breath_hide_time_remaining = self.breath_hide_time_remaining - (dt or 0.0)
            if self.breath_hide_time_remaining > 0.0 then
                self:SetBreathHUDVisible(true)
                self:SetBreathBarRatio(widget, ratio)
                self:UpdateBreathFade(dt)
                return
            end
        end

        self.breath_hide_time_remaining = 0.0
        self.breath_last_width = -1.0
        self:SetBreathHUDVisible(false)
        self:UpdateBreathFade(dt)
        return
    end

    self.breath_hide_time_remaining = self.breath_hide_delay
    self:SetBreathHUDVisible(true)
    self:SetBreathBarRatio(widget, ratio)
end

function UIManager:GetHeadingSource()
    local pawn = self:GetSniperPawn()
    if pawn ~= nil then
        return pawn
    end

    if World ~= nil and World.FindActorByName ~= nil then
        local player = World.FindActorByName("ScopeTest_Player")
        if player ~= nil then
            return player
        end
    end

    if self.general ~= nil and self.general.context ~= nil then
        return self.general.context.actor
    end
    return obj
end

function UIManager:GetHeadingDegrees()
    local source = self:GetHeadingSource()
    if source == nil or source.Forward == nil then
        return 0.0
    end

    local forward = source.Forward
    local x = forward.X or forward.x or 1.0
    local y = forward.Y or forward.y or 0.0
    if math.abs(x) < 0.0001 and math.abs(y) < 0.0001 then
        return 0.0
    end

    return normalize_degrees(atan2_degrees(y, x))
end

function UIManager:UpdateCompass(dt)
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    self.smoothed_heading_degrees = smooth_heading(
        self.smoothed_heading_degrees,
        self:GetHeadingDegrees(),
        dt,
        self.compass_smooth_speed)

    local frame = math.floor(normalize_degrees(self.smoothed_heading_degrees) + 0.5) % self.compass_frame_count
    if frame ~= self.compass_last_frame then
        self.compass_last_frame = frame
        self:SetElementImage(widget, "compassImage", string.format("Image/Hor-Compass/Window/Compass_Window_%03d.png", frame))
    end
end

function UIManager:TickInGameHUD(dt)
    self:UpdateCompass(dt)
    self:SetScopeHUDVisible(self:GetScopeVisibleFromInputOrPawn())
    self:UpdateBreathHUD(dt)
    self:UpdateWeaponHUD(false)
end

return UIManager
