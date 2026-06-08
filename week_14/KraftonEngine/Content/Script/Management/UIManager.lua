local GameState = require("Management/GameState")
local LoadingTips = require("Management/LoadingTips")

local UIManager = {}
UIManager.__index = UIManager

local MAIN_HUD_MODE = "Main"
local PRE_INGAME_HUD_MODE = "PreInGame"
local LOADING_HUD_MODE = "Loading"
local IN_GAME_HUD_MODE = "InGame"
local DEFAULT_LOADING_TIP = LoadingTips[1] or "Tip: Hold your breath only when the shot really matters."
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
local POPUP_LAYER_ID = "popupLayer"
local POPUP_BACKDROP_ID = "popupBackdrop"
local POPUP_IDS = {
    Settings = "settingsPopup",
    ScoreBoard = "scorePopup",
    Credits = "creditsPopup"
}
local POPUP_BUTTON_IDS = {
    "btnCloseSettings",
    "btnCloseScore",
    "btnCloseCredits",
    "btnBgmDown",
    "btnBgmUp",
    "btnSfxDown",
    "btnSfxUp",
    "btnZoomMode",
    "btnMouseDown",
    "btnMouseUp"
}
local SCORE_ROW_COUNT = 8
local PAUSE_LAYER_ID = "pauseLayer"
local PAUSE_PANEL_IDS = {
    Menu = "pauseMenuPanel",
    Settings = "pauseSettingsPanel",
    Controls = "pauseControlsPanel"
}
local PAUSE_MENU_BUTTON_IDS = {
    "btnPauseResume",
    "btnPauseMain",
    "btnPauseSettings",
    "btnPauseControls"
}
local PAUSE_SETTING_BUTTON_IDS = {
    "btnPauseSettingsBack",
    "btnPauseBgmDown",
    "btnPauseBgmUp",
    "btnPauseSfxDown",
    "btnPauseSfxUp",
    "btnPauseZoomMode",
    "btnPauseMouseDown",
    "btnPauseMouseUp"
}
local PAUSE_CONTROL_BUTTON_IDS = {
    "btnPauseControlsBack"
}
local CUTSCENE_LETTERBOX_TOP_ID = "cutsceneLetterboxTop"
local CUTSCENE_LETTERBOX_BOTTOM_ID = "cutsceneLetterboxBottom"
local CUTSCENE_LETTERBOX_THICKNESS = 130.0
local CUTSCENE_LETTERBOX_SCREEN_HEIGHT = 1080.0
local CUTSCENE_LETTERBOX_ENTER_SPEED = 18.0
local CUTSCENE_LETTERBOX_EXIT_SPEED = 14.0

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

local function approach01(current, target, dt, speed)
    current = clamp01(current or 0.0)
    target = clamp01(target or 0.0)
    if math.abs(target - current) <= 0.001 then
        return target
    end
    if dt == nil or dt <= 0.0 then
        return current
    end

    local alpha = clamp01(1.0 - math.exp(-(speed or 1.0) * dt))
    local result = current + (target - current) * alpha
    if math.abs(target - result) <= 0.001 then
        return target
    end
    return clamp01(result)
end

local function rgba255(color, alpha_scale)
    alpha_scale = clamp01(alpha_scale)
    local alpha = math.floor((color.a or 255) * alpha_scale + 0.5)
    return string.format("rgba(%d, %d, %d, %d)", color.r or 255, color.g or 255, color.b or 255, alpha)
end

local function random_int(min_value, max_value)
    if Random ~= nil and Random.RandomInt ~= nil then
        return Random.RandomInt(min_value, max_value)
    end
    return math.random(min_value, max_value)
end

local function select_loading_tip(payload)
    if payload ~= nil and payload.tip ~= nil and payload.tip ~= "" then
        return payload.tip
    end
    if type(LoadingTips) == "table" and #LoadingTips > 0 then
        local index = random_int(1, #LoadingTips)
        return LoadingTips[index] or DEFAULT_LOADING_TIP
    end
    return DEFAULT_LOADING_TIP
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

local function read_float_method(target, method_names)
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
                    return number
                end
            end
        end
    end

    return nil
end

local function read_bool_method(target, method_names)
    if target == nil then
        return false
    end

    for _, method_name in ipairs(method_names) do
        if target[method_name] ~= nil then
            local ok, value = pcall(function()
                return target[method_name](target)
            end)
            if ok then
                return value == true
            end
        end
    end

    return false
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
        breath_warning_time = 0.0,
        breath_warning_style_key = "",
        breath_missing_pawn_warned = false,
        weapon_last_name = nil,
        weapon_last_ammo_text = nil,
        weapon_last_ammo_type = nil,
        cutscene_active = false,
        cutscene_letterbox_alpha = 0.0,
        cutscene_letterbox_target = 0.0,
        cutscene_letterbox_last_alpha = -1.0,
        active_popup = nil,
        pause_visible = false,
        pause_panel = "Menu"
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

    self.general:Subscribe("ingame.pause_changed", self, function(payload)
        self:SetInGamePauseVisible(payload and payload.paused == true)
    end)

    self.general:Subscribe("ingame.started", self, function(payload)
        self:SetInGameTimer(payload)
    end)

    self.general:Subscribe("ingame.timer", self, function(payload)
        self:SetInGameTimer(payload)
    end)

    self.general:Subscribe("ingame.scope_telemetry", self, function(payload)
        self:SetScopeTelemetry(payload)
    end)

    self.general:Subscribe("cutscene.skip_prompt", self, function(payload)
        self:SetCutSceneSkipPrompt(payload)
    end)

    self.general:Subscribe("cutscene.presentation", self, function(payload)
        self:SetCutScenePresentation(payload)
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
    self.active_popup = nil
    self.pause_visible = false
    self.pause_panel = "Menu"
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

function UIManager:RemoveElementStyle(widget, element_id, property)
    call_widget(widget, "RemoveElementStyle", element_id, property)
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
    self:HideAllPopups(widget)

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
        local click_action = "MainButtonClick"
        if element_id == "btnGameStart" then
            click_action = "GameStart"
        elseif element_id == "btnScoreBoard" then
            click_action = "OpenScoreBoard"
        elseif element_id == "btnSettings" then
            click_action = "OpenSettings"
        elseif element_id == "btnCredits" then
            click_action = "OpenCredits"
        end
        call_widget(widget, "SetActionEvent", element_id, click_action)
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end

    call_widget(widget, "SetActionEvent", POPUP_BACKDROP_ID, "ModalBlock")
    for _, element_id in ipairs(POPUP_BUTTON_IDS) do
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end
    call_widget(widget, "SetActionEvent", "btnCloseSettings", "ClosePopup")
    call_widget(widget, "SetActionEvent", "btnCloseScore", "ClosePopup")
    call_widget(widget, "SetActionEvent", "btnCloseCredits", "ClosePopup")
    call_widget(widget, "SetActionEvent", "btnBgmDown", "BgmDown")
    call_widget(widget, "SetActionEvent", "btnBgmUp", "BgmUp")
    call_widget(widget, "SetActionEvent", "btnSfxDown", "SfxDown")
    call_widget(widget, "SetActionEvent", "btnSfxUp", "SfxUp")
    call_widget(widget, "SetActionEvent", "btnZoomMode", "ToggleZoomMode")
    call_widget(widget, "SetActionEvent", "btnMouseDown", "MouseDown")
    call_widget(widget, "SetActionEvent", "btnMouseUp", "MouseUp")
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
            if self.active_popup == nil then
                self:BeginMainStartTransition()
            end
        elseif action == "MainButtonClick" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 1.0)
        elseif action == "OpenSettings" then
            self:OpenPopup("Settings")
        elseif action == "OpenScoreBoard" then
            self:OpenPopup("ScoreBoard")
        elseif action == "OpenCredits" then
            self:OpenPopup("Credits")
        elseif action == "ClosePopup" then
            self:ClosePopup()
        elseif action == "BgmDown" then
            self:AdjustSetting("bgm_volume", -0.1)
        elseif action == "BgmUp" then
            self:AdjustSetting("bgm_volume", 0.1)
        elseif action == "SfxDown" then
            self:AdjustSetting("sfx_volume", -0.1)
        elseif action == "SfxUp" then
            self:AdjustSetting("sfx_volume", 0.1)
        elseif action == "ToggleZoomMode" then
            self:ToggleZoomMode()
        elseif action == "MouseDown" then
            self:AdjustSetting("mouse_sensitivity", -0.1)
        elseif action == "MouseUp" then
            self:AdjustSetting("mouse_sensitivity", 0.1)
        elseif action == "MainButtonHover" and not hover_played then
            hover_played = true
            self:PlayUISFX(MAIN_BUTTON_HOVER_SFX, 0.8)
        end
    end
end

function UIManager:TickMainHUD(dt)
    local widget = self:GetActiveHUDWidget()
    if not self.main_start_pending then
        self:PollMainActions(widget)
    end

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
        self:SetMainMenuButtonsEnabled(widget, false)
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
            call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
            self:RemoveElementStyle(widget, element_id, "transform")
        else
            call_widget(widget, "SetElementAttribute", element_id, "disabled", "true")
            call_widget(widget, "RemoveElementAttribute", element_id, "data-hover-action")
            self:SetElementStyle(widget, element_id, "transform", "scale(1.0)")
        end
    end
end

function UIManager:SetElementDisplay(widget, element_id, visible)
    self:SetElementVisible(widget, element_id, visible)
    self:SetElementStyle(widget, element_id, "display", visible and "block" or "none")
    self:SetElementAlpha(widget, element_id, visible and 1.0 or 0.0)
end

function UIManager:SetCutSceneSkipPrompt(payload)
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local visible = payload ~= nil and payload.visible == true
    local text = "Press Space to Skip"
    if payload ~= nil and payload.text ~= nil and payload.text ~= "" then
        text = tostring(payload.text)
    end

    call_widget(widget, "SetText", "cutsceneSkipPrompt", visible and text or "")
    self:SetElementDisplay(widget, "cutsceneSkipPrompt", visible)
end

function UIManager:ApplyCutSceneLetterbox(widget, alpha)
    if widget == nil then
        return
    end

    alpha = clamp01(alpha)
    local visible = alpha > 0.001 or self.cutscene_letterbox_target > 0.001
    local top_y = -CUTSCENE_LETTERBOX_THICKNESS + CUTSCENE_LETTERBOX_THICKNESS * alpha
    local bottom_y = CUTSCENE_LETTERBOX_SCREEN_HEIGHT - CUTSCENE_LETTERBOX_THICKNESS * alpha

    self:SetElementVisible(widget, CUTSCENE_LETTERBOX_TOP_ID, visible)
    self:SetElementVisible(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, visible)
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_TOP_ID, "display", visible and "block" or "none")
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, "display", visible and "block" or "none")
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_TOP_ID, "top", string.format("%.3fpx", top_y))
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, "top", string.format("%.3fpx", bottom_y))
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_TOP_ID, "height", string.format("%.3fpx", CUTSCENE_LETTERBOX_THICKNESS))
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, "height", string.format("%.3fpx", CUTSCENE_LETTERBOX_THICKNESS))
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_TOP_ID, "background-color", "rgba(0, 0, 0, 255)")
    self:SetElementStyle(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, "background-color", "rgba(0, 0, 0, 255)")
    self:SetElementAlpha(widget, CUTSCENE_LETTERBOX_TOP_ID, visible and 1.0 or 0.0)
    self:SetElementAlpha(widget, CUTSCENE_LETTERBOX_BOTTOM_ID, visible and 1.0 or 0.0)
end

function UIManager:SetCutSceneLetterboxTarget(widget, active)
    self.cutscene_letterbox_target = active and 1.0 or 0.0
    if active then
        self:ApplyCutSceneLetterbox(widget, self.cutscene_letterbox_alpha)
    end
end

function UIManager:TickCutSceneLetterbox(widget, dt)
    if widget == nil then
        return
    end

    local speed = self.cutscene_letterbox_target > self.cutscene_letterbox_alpha and
        CUTSCENE_LETTERBOX_ENTER_SPEED or CUTSCENE_LETTERBOX_EXIT_SPEED
    self.cutscene_letterbox_alpha = approach01(
        self.cutscene_letterbox_alpha,
        self.cutscene_letterbox_target,
        dt,
        speed)

    if math.abs(self.cutscene_letterbox_alpha - self.cutscene_letterbox_last_alpha) > 0.0005 then
        self.cutscene_letterbox_last_alpha = self.cutscene_letterbox_alpha
        self:ApplyCutSceneLetterbox(widget, self.cutscene_letterbox_alpha)
    end
end

function UIManager:SetCutScenePresentation(payload)
    local active = payload ~= nil and payload.active == true
    self.cutscene_active = active

    local widget = self:GetActiveHUDWidget()
    if widget == nil or self.active_hud_mode ~= IN_GAME_HUD_MODE then
        return
    end

    self:SetCutSceneLetterboxTarget(widget, active)
    if active then
        self:SetInGameHUDSuppressed(widget, true)
        self:SetElementDisplay(widget, PAUSE_LAYER_ID, false)
    elseif not self.pause_visible then
        self:SetInGameHUDSuppressed(widget, false)
    end
end

function UIManager:HideAllPopups(widget)
    widget = widget or self:GetActiveHUDWidget()
    if widget == nil then
        self.active_popup = nil
        return
    end

    self:SetElementDisplay(widget, POPUP_LAYER_ID, false)
    for _, popup_id in pairs(POPUP_IDS) do
        self:SetElementDisplay(widget, popup_id, false)
    end
    self.active_popup = nil
    self:SetMainMenuButtonsEnabled(widget, true)
end

function UIManager:OpenPopup(popup_name)
    local widget = self:GetActiveHUDWidget()
    local popup_id = POPUP_IDS[popup_name]
    if widget == nil or popup_id == nil or self.main_start_pending then
        return
    end

    self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 1.0)
    self:SetMainMenuButtonsEnabled(widget, false)
    self:SetElementDisplay(widget, POPUP_LAYER_ID, true)
    self:SetElementDisplay(widget, POPUP_BACKDROP_ID, true)

    for name, id in pairs(POPUP_IDS) do
        self:SetElementDisplay(widget, id, name == popup_name)
    end

    self.active_popup = popup_name
    if popup_name == "Settings" then
        self:RefreshSettingsPopup(widget)
    elseif popup_name == "ScoreBoard" then
        self:RefreshScoreBoardPopup(widget)
    end
end

function UIManager:ClosePopup()
    local widget = self:GetActiveHUDWidget()
    if widget == nil or self.active_popup == nil then
        return
    end

    self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.9)
    self:HideAllPopups(widget)
end

function UIManager:GetDataManager()
    if self.general ~= nil and self.general.managers ~= nil then
        return self.general.managers.Data
    end
    return nil
end

function UIManager:GetAudioManager()
    if self.general ~= nil and self.general.managers ~= nil then
        return self.general.managers.Audio
    end
    return nil
end

function UIManager:GetSettings()
    local data = self:GetDataManager()
    if data ~= nil and data.GetSettings ~= nil then
        return data:GetSettings()
    end
    return {
        bgm_volume = 1.0,
        sfx_volume = 1.0,
        zoom_toggle = true,
        mouse_sensitivity = 1.0
    }
end

function UIManager:SetSetting(key, value)
    local data = self:GetDataManager()
    if data ~= nil and data.SetSetting ~= nil then
        data:SetSetting(key, value)
    end

    local audio = self:GetAudioManager()
    if audio ~= nil then
        if key == "bgm_volume" and audio.SetBGMVolume ~= nil then
            audio:SetBGMVolume(value)
        elseif key == "sfx_volume" and audio.SetSFXVolume ~= nil then
            audio:SetSFXVolume(value)
        end
    end

    self:RefreshSettingsPopup()
end

function UIManager:AdjustSetting(key, delta)
    local settings = self:GetSettings()
    local current = tonumber(settings[key]) or 0.0
    local min_value = key == "mouse_sensitivity" and 0.1 or 0.0
    local max_value = key == "mouse_sensitivity" and 5.0 or 1.0
    local next_value = current + delta
    if next_value < min_value then
        next_value = min_value
    elseif next_value > max_value then
        next_value = max_value
    end
    self:SetSetting(key, next_value)
    self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.65)
end

function UIManager:ToggleZoomMode()
    local settings = self:GetSettings()
    self:SetSetting("zoom_toggle", not (settings.zoom_toggle == true))
    self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.65)
end

function UIManager:RefreshSettingsPopup(widget)
    widget = widget or self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local settings = self:GetSettings()
    call_widget(widget, "SetText", "bgmValue", string.format("%d%%", math.floor((settings.bgm_volume or 1.0) * 100.0 + 0.5)))
    call_widget(widget, "SetText", "sfxValue", string.format("%d%%", math.floor((settings.sfx_volume or 1.0) * 100.0 + 0.5)))
    call_widget(widget, "SetText", "mouseValue", string.format("%.2fx", settings.mouse_sensitivity or 1.0))
    call_widget(widget, "SetText", "zoomModeValue", settings.zoom_toggle and "Toggle" or "Hold")

    call_widget(widget, "SetText", "pauseBgmValue", string.format("%d%%", math.floor((settings.bgm_volume or 1.0) * 100.0 + 0.5)))
    call_widget(widget, "SetText", "pauseSfxValue", string.format("%d%%", math.floor((settings.sfx_volume or 1.0) * 100.0 + 0.5)))
    call_widget(widget, "SetText", "pauseMouseValue", string.format("%.2fx", settings.mouse_sensitivity or 1.0))
    call_widget(widget, "SetText", "pauseZoomModeValue", settings.zoom_toggle and "Toggle" or "Hold")
end

function UIManager:NormalizeRunResult(result)
    local text = string.lower(tostring(result or "Unknown"))
    if text == "victory" or text == "win" or text == "success" then
        return "Victory"
    end
    if text == "defeat" or text == "lose" or text == "loss" or text == "fail" then
        return "Defeat"
    end
    return tostring(result or "Unknown")
end

function UIManager:RefreshScoreBoardPopup(widget)
    widget = widget or self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local entries = {}
    local data = self:GetDataManager()
    if data ~= nil and data.GetScoreEntries ~= nil then
        entries = data:GetScoreEntries()
    end

    if #entries <= 0 then
        entries = {
            { nickname = "Player", result = "Defeat", score = 0 }
        }
    end

    for index = 1, SCORE_ROW_COUNT do
        local entry = entries[index]
        local visible = entry ~= nil
        self:SetElementVisible(widget, "scoreRow" .. tostring(index), visible)
        if visible then
            call_widget(widget, "SetText", "scoreRank" .. tostring(index), tostring(index))
            call_widget(widget, "SetText", "scoreName" .. tostring(index), tostring(entry.nickname or "Player"))
            call_widget(widget, "SetText", "scoreResult" .. tostring(index), self:NormalizeRunResult(entry.result))
            call_widget(widget, "SetText", "scoreValue" .. tostring(index), tostring(math.floor(tonumber(entry.score) or 0)))
        end
    end

    local thumb_height = #entries > SCORE_ROW_COUNT and 96 or 406
    self:SetElementStyle(widget, "scoreScrollThumb", "height", tostring(thumb_height) .. "px")
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
    call_widget(widget, "SetText", "loadingTip", select_loading_tip(payload))
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

function UIManager:FormatTimerSeconds(seconds)
    seconds = math.max(0, math.ceil(tonumber(seconds) or 0.0))
    local minutes = math.floor(seconds / 60)
    local rest = seconds % 60
    return string.format("%02d:%02d", minutes, rest)
end

function UIManager:SetInGameTimer(payload)
    payload = payload or {}
    local widget = payload.widget or self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local remaining = payload.remaining_time
    if remaining == nil and payload.match_duration ~= nil and payload.elapsed_time ~= nil then
        remaining = (tonumber(payload.match_duration) or 0.0) - (tonumber(payload.elapsed_time) or 0.0)
    elseif remaining == nil and payload.match_duration ~= nil and payload.timer ~= nil then
        remaining = (tonumber(payload.match_duration) or 0.0) - (tonumber(payload.timer) or 0.0)
    elseif remaining == nil then
        remaining = 300.0
    end

    self:SetElementVisible(widget, "airSupportTimerPanel", true)
    self:SetElementAlpha(widget, "airSupportTimerPanel", 1.0)
    call_widget(widget, "SetText", "airSupportTimerValue", self:FormatTimerSeconds(remaining))
end

function UIManager:ConfigureInGameHUD(widget)
    self:ResetInGameHUDRuntime(true)
    self.pause_visible = false
    self.pause_panel = "Menu"

    call_widget(widget, "SetWantsMouse", false)
    call_widget(widget, "SetBlocksGameMouseLook", false)
    call_widget(widget, "SetBlocksGameInput", false)

    self:SetElementAlpha(widget, "scopeOverlay", 0.0)
    self:SetElementVisible(widget, "scopeOverlay", false)
    self:ConfigureScopeTelemetry(widget)
    self:SetElementAlpha(widget, "crosshairImage", 1.0)
    self:SetElementVisible(widget, "crosshairImage", true)
    self:SetElementStyle(widget, "airSupportTimerLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "airSupportTimerLabel", "font-weight", "bold")
    self:SetElementStyle(widget, "airSupportTimerValue", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "airSupportTimerValue", "font-weight", "bold")
    call_widget(widget, "SetText", "airSupportTimerLabel", "&#54637;&#44277; &#51648;&#50896; &#46020;&#52265; &#50696;&#51221;")
    self:SetInGameTimer({ widget = widget, remaining_time = 300.0, match_duration = 300.0, elapsed_time = 0.0 })

    self:SetBreathGroupAlpha(widget, 0.0)
    self:SetElementVisible(widget, "breathPanel", false)
    self:SetElementStyle(widget, "breathLabel", "font-family", "\"Nexon\"")
    self:SetElementStyle(widget, "breathLabel", "font-weight", "400")
    self:SetElementStyle(widget, "breathLabel", "color", "rgba(255, 255, 255, 255)")
    call_widget(widget, "SetText", "breathLabel", "&#49704;&#52280;&#44592;")
    self:SetElementStyle(widget, "breathBarFill", "width", "0px")
    self:SetBreathWarning(widget, false, 0.0)
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
    self:SetCutSceneSkipPrompt({ visible = false })
    self:ApplyCutSceneLetterbox(widget, self.cutscene_letterbox_alpha)

    self:SetElementImage(widget, "compassImage", "Image/Hor-Compass/Window/Compass_Window_000.png")
    self:ConfigurePauseMenuActions(widget)
    self:SetInGamePauseVisible(false)
end

function UIManager:ConfigurePauseMenuActions(widget)
    if widget == nil then
        return
    end

    call_widget(widget, "SetActionEvent", "btnPauseResume", "PauseResume")
    call_widget(widget, "SetActionEvent", "btnPauseMain", "PauseGoMain")
    call_widget(widget, "SetActionEvent", "btnPauseSettings", "PauseOpenSettings")
    call_widget(widget, "SetActionEvent", "btnPauseControls", "PauseOpenControls")
    call_widget(widget, "SetActionEvent", "btnPauseSettingsBack", "PauseBackMenu")
    call_widget(widget, "SetActionEvent", "btnPauseControlsBack", "PauseBackMenu")
    call_widget(widget, "SetActionEvent", "btnPauseBgmDown", "BgmDown")
    call_widget(widget, "SetActionEvent", "btnPauseBgmUp", "BgmUp")
    call_widget(widget, "SetActionEvent", "btnPauseSfxDown", "SfxDown")
    call_widget(widget, "SetActionEvent", "btnPauseSfxUp", "SfxUp")
    call_widget(widget, "SetActionEvent", "btnPauseZoomMode", "ToggleZoomMode")
    call_widget(widget, "SetActionEvent", "btnPauseMouseDown", "MouseDown")
    call_widget(widget, "SetActionEvent", "btnPauseMouseUp", "MouseUp")

    for _, element_id in ipairs(PAUSE_MENU_BUTTON_IDS) do
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end
    for _, element_id in ipairs(PAUSE_SETTING_BUTTON_IDS) do
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end
    for _, element_id in ipairs(PAUSE_CONTROL_BUTTON_IDS) do
        call_widget(widget, "SetElementAttribute", element_id, "data-hover-action", "MainButtonHover")
    end
end

function UIManager:SetInGameHUDSuppressed(widget, suppressed)
    local visible = not suppressed
    self:SetElementVisible(widget, "compassImage", visible)
    self:SetElementVisible(widget, "CompassArrow", visible)
    self:SetElementVisible(widget, "crosshairImage", visible and not self.scope_visible)
    self:SetElementVisible(widget, "scopeOverlay", visible and self.scope_visible)
    self:SetElementVisible(widget, "airSupportTimerPanel", visible)
    self:SetElementVisible(widget, "breathPanel", visible and self.breath_visible)
    self:SetElementVisible(widget, "weaponInfoPanel", visible)
end

function UIManager:SetPausePanel(panel_name)
    self.pause_panel = panel_name or "Menu"
    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    for name, element_id in pairs(PAUSE_PANEL_IDS) do
        self:SetElementDisplay(widget, element_id, name == self.pause_panel)
    end
    if self.pause_panel == "Settings" then
        self:RefreshSettingsPopup(widget)
    end
end

function UIManager:SetInGamePauseVisible(visible)
    if self.active_hud_mode ~= IN_GAME_HUD_MODE then
        return
    end

    local widget = self:GetActiveHUDWidget()
    if widget == nil then
        self.pause_visible = visible
        return
    end

    self.pause_visible = visible
    if visible then
        if Input ~= nil and Input.SetInputModeGameAndUI ~= nil then
            Input.SetInputModeGameAndUI()
        end
        if Input ~= nil and Input.SetCursorVisible ~= nil then
            Input.SetCursorVisible(true)
        end
        if Input ~= nil and Input.ReleaseMouseCapture ~= nil then
            Input.ReleaseMouseCapture()
        end
        call_widget(widget, "SetWantsMouse", true)
        call_widget(widget, "SetBlocksGameMouseLook", true)
        call_widget(widget, "SetBlocksGameInput", false)
        call_widget(widget, "SetBlocksGameKeyboard", false)
        self:SetElementDisplay(widget, PAUSE_LAYER_ID, true)
        self:SetInGameHUDSuppressed(widget, true)
        self:SetPausePanel(self.pause_panel or "Menu")
        self:RefreshSettingsPopup(widget)
    else
        if Input ~= nil and Input.SetInputModeGameOnly ~= nil then
            Input.SetInputModeGameOnly()
        end
        if Input ~= nil and Input.SetCursorVisible ~= nil then
            Input.SetCursorVisible(false)
        end
        if Input ~= nil and Input.SetMouseCaptured ~= nil then
            Input.SetMouseCaptured(true)
        end
        call_widget(widget, "SetWantsMouse", false)
        call_widget(widget, "SetBlocksGameMouseLook", false)
        call_widget(widget, "SetBlocksGameInput", false)
        call_widget(widget, "SetBlocksGameKeyboard", false)
        self.pause_panel = "Menu"
        self:SetElementDisplay(widget, PAUSE_LAYER_ID, false)
        for _, element_id in pairs(PAUSE_PANEL_IDS) do
            self:SetElementDisplay(widget, element_id, false)
        end
        self:SetInGameHUDSuppressed(widget, false)
    end
end

function UIManager:ConfigureScopeTelemetry(widget)
    if widget == nil then
        return
    end

    self:SetScopeTelemetry({
        widget = widget,
        distance_text = "-- m",
        wind_text = "000 deg  0.0 m/s",
        zoom_text = "4x"
    })
end

function UIManager:SetScopeTelemetry(payload)
    payload = payload or {}
    local widget = payload.widget or self:GetActiveHUDWidget()
    if widget == nil then
        return
    end

    local distance_text = payload.distance_text or payload.distance or "-- m"
    local wind_text = payload.wind_text or payload.wind or "000 deg  0.0 m/s"
    local wind_direction_text = "000 deg"
    local wind_speed_text = "0.0 m/s"
    local zoom_text = payload.zoom_text or payload.zoom or "4x"
    if type(payload.distance_meters) == "number" then
        distance_text = string.format("%dm", math.floor(payload.distance_meters + 0.5))
    end
    if type(payload.wind_degrees) == "number" and type(payload.wind_mps) == "number" then
        wind_direction_text = string.format("%03d deg", math.floor(payload.wind_degrees + 0.5) % 360)
        wind_speed_text = string.format("%.1f m/s", payload.wind_mps)
        wind_text = wind_direction_text .. "  " .. wind_speed_text
    else
        local parsed_direction, parsed_speed = tostring(wind_text):match("^%s*([%+%-]?%d+%s*deg)%s+([%+%-]?[%d%.]+%s*m/s)%s*$")
        if parsed_direction ~= nil and parsed_speed ~= nil then
            wind_direction_text = parsed_direction
            wind_speed_text = parsed_speed
        else
            wind_direction_text = tostring(wind_text)
            wind_speed_text = ""
        end
    end
    if type(payload.zoom_multiplier) == "number" then
        local clamped_zoom = payload.zoom_multiplier
        if clamped_zoom < 4 then
            clamped_zoom = 4
        elseif clamped_zoom > 32 then
            clamped_zoom = 32
        end
        zoom_text = string.format("%dx", math.floor(clamped_zoom + 0.5))
    end

    local telemetry_values = {
        scopeDistanceValue = tostring(distance_text),
        scopeWindValue = tostring(wind_direction_text),
        scopeWindSpeedValue = tostring(wind_speed_text),
        scopeZoomValue = tostring(zoom_text)
    }

    for element_id, text in pairs(telemetry_values) do
        self:SetElementStyle(widget, element_id, "font-family", "\"Nexon\"")
        self:SetElementStyle(widget, element_id, "font-weight", "bold")
        self:SetElementStyle(widget, element_id, "color", "rgba(255, 255, 255, 255)")
        call_widget(widget, "SetText", element_id, text)
    end
end

function UIManager:ResetInGameHUDRuntime(clear_pawn)
    self.scope_visible = false
    self.compass_last_frame = -1
    self.smoothed_heading_degrees = nil
    self.breath_visible = false
    self.breath_last_width = -1.0
    self.breath_hide_time_remaining = 0.0
    self.breath_fade_out_time_remaining = 0.0
    self.breath_warning_time = 0.0
    self.breath_warning_style_key = ""
    self.breath_missing_pawn_warned = false
    self.weapon_last_name = nil
    self.weapon_last_ammo_text = nil
    self.weapon_last_ammo_type = nil
    self.cutscene_active = false
    self.cutscene_letterbox_alpha = 0.0
    self.cutscene_letterbox_target = 0.0
    self.cutscene_letterbox_last_alpha = -1.0
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
        if ok and is_valid and (
            self.sniper_pawn.GetHoldBreathGaugeRatio ~= nil or
            self.sniper_pawn.GetHoldBreathGauge ~= nil or
            self.sniper_pawn.IsHoldBreathActive ~= nil
        ) then
            return self.sniper_pawn
        end
    end

    self.sniper_pawn = nil
    if World == nil then
        return nil
    end

    local actor = nil
    if World.FindFirstSniperPawn ~= nil then
        actor = World.FindFirstSniperPawn()
    end
    if actor == nil and World.FindFirstActorByClass ~= nil then
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

    if actor.GetHoldBreathGaugeRatio ~= nil or actor.GetHoldBreathGauge ~= nil or actor.IsHoldBreathActive ~= nil then
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

function UIManager:GetHoldBreathGaugeRatio(pawn)
    if pawn == nil then
        return 0.0
    end

    local ratio = read_float_method(pawn, { "GetHoldBreathGaugeRatio" })
    if ratio ~= nil then
        return clamp01(ratio)
    end

    local gauge = read_float_method(pawn, { "GetHoldBreathGauge" })
    local max_gauge = read_float_method(pawn, { "GetMaxHoldBreathGauge" })
    if gauge ~= nil and max_gauge ~= nil and max_gauge > 0.0 then
        return clamp01(gauge / max_gauge)
    end

    return 0.0
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

function UIManager:SetBreathWarning(widget, warning, dt)
    if widget == nil then
        return
    end

    local fill_color = "rgba(255, 255, 255, 255)"
    local track_background_color = "rgba(12, 18, 26, 220)"
    local track_color = "rgba(236, 242, 255, 175)"
    if warning then
        self.breath_warning_time = (self.breath_warning_time or 0.0) + (dt or 0.0)
        local pulse = 0.5 + 0.5 * math.sin(self.breath_warning_time * 13.0)
        local green = math.floor(34 + 92 * (1.0 - pulse))
        local blue = math.floor(30 + 58 * (1.0 - pulse))
        local track_alpha = math.floor(90 + 120 * pulse)
        fill_color = string.format("rgba(255, %d, %d, 255)", green, blue)
        track_background_color = string.format("rgba(120, %d, %d, %d)", math.floor(green * 0.35), math.floor(blue * 0.35), track_alpha)
        track_color = string.format("rgba(255, %d, %d, 230)", math.floor(green * 0.85), math.floor(blue * 0.85))
    else
        self.breath_warning_time = 0.0
    end

    local style_key = fill_color .. "|" .. track_background_color .. "|" .. track_color
    if self.breath_warning_style_key == style_key then
        return
    end

    self.breath_warning_style_key = style_key
    self:SetElementStyle(widget, "breathBarFill", "background-color", fill_color)
    self:SetElementStyle(widget, "breathBarTrack", "background-color", track_background_color)
    self:SetElementStyle(widget, "breathBarTrack", "border-color", track_color)
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
            self:SetBreathWarning(widget, false, 0.0)
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
            self:SetBreathWarning(widget, false, 0.0)
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
    local warning = false
    if pawn ~= nil then
        local active = read_bool_method(pawn, { "IsHoldBreathActive" })
        local scoped = read_bool_method(pawn, { "IsScoped" })
        local held = read_bool_method(pawn, { "IsHoldBreathInputHeld" })
        if pawn.IsHoldBreathInputHeld == nil then
            held = self:IsRawHoldBreathRequested()
        end
        requested = active or (scoped and held)

        ratio = self:GetHoldBreathGaugeRatio(pawn)

        local recovering = read_bool_method(pawn, { "IsHoldBreathRecovering", "IsHoldBreathInForcedRecovery" })
        local release_required = read_bool_method(pawn, { "IsHoldBreathReleaseRequired", "IsHoldBreathReleasePending" })
        local exhausted = ratio <= 0.001
        warning = (requested and exhausted) or recovering or release_required
        if scoped and held and not active and ratio < 0.999 then
            warning = true
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
                self:SetBreathWarning(widget, warning, dt)
                self:UpdateBreathFade(dt)
                return
            end
        end

        self.breath_hide_time_remaining = 0.0
        self.breath_last_width = -1.0
        self:SetBreathWarning(widget, false, 0.0)
        self:SetBreathHUDVisible(false)
        self:UpdateBreathFade(dt)
        return
    end

    self.breath_hide_time_remaining = self.breath_hide_delay
    self:SetBreathHUDVisible(true)
    self:SetBreathBarRatio(widget, ratio)
    self:SetBreathWarning(widget, warning, dt)
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

function UIManager:PollInGameActions(widget)
    if widget == nil or widget.PollActionEvents == nil then
        return
    end

    local ok, events = pcall(function()
        return widget:PollActionEvents()
    end)
    if not ok or events == nil then
        log("InGame HUD action polling failed")
        return
    end

    local hover_played = false
    for _, action in ipairs(events) do
        if action == "PauseResume" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 1.0)
            if self.general ~= nil then
                self.general:Publish("ingame.pause_resume_requested", { reason = "ui" })
            end
        elseif action == "PauseGoMain" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 1.0)
            if self.general ~= nil then
                self.general:Publish("ingame.pause_main_requested", { reason = "ui" })
            end
        elseif action == "PauseOpenSettings" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.9)
            self:SetPausePanel("Settings")
        elseif action == "PauseOpenControls" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.9)
            self:SetPausePanel("Controls")
        elseif action == "PauseBackMenu" then
            self:PlayUISFX(MAIN_BUTTON_CLICK_SFX, 0.8)
            self:SetPausePanel("Menu")
        elseif action == "BgmDown" then
            self:AdjustSetting("bgm_volume", -0.1)
        elseif action == "BgmUp" then
            self:AdjustSetting("bgm_volume", 0.1)
        elseif action == "SfxDown" then
            self:AdjustSetting("sfx_volume", -0.1)
        elseif action == "SfxUp" then
            self:AdjustSetting("sfx_volume", 0.1)
        elseif action == "ToggleZoomMode" then
            self:ToggleZoomMode()
        elseif action == "MouseDown" then
            self:AdjustSetting("mouse_sensitivity", -0.1)
        elseif action == "MouseUp" then
            self:AdjustSetting("mouse_sensitivity", 0.1)
        elseif action == "MainButtonHover" and not hover_played then
            hover_played = true
            self:PlayUISFX(MAIN_BUTTON_HOVER_SFX, 0.8)
        end
    end
end

function UIManager:TickInGameHUD(dt)
    local widget = self:GetActiveHUDWidget()
    self:PollInGameActions(widget)
    self:TickCutSceneLetterbox(widget, dt)
    if self.cutscene_active then
        if widget ~= nil then
            self:SetInGameHUDSuppressed(widget, true)
        end
        return
    end

    if self.pause_visible then
        return
    end

    self:UpdateCompass(dt)
    self:SetScopeHUDVisible(self:GetScopeVisibleFromInputOrPawn())
    self:UpdateBreathHUD(dt)
    self:UpdateWeaponHUD(false)
end

return UIManager
