local hud_widget = nil
local scope_visible = false
local spawned_count = 0

local function log(message)
    if Debug and Debug.Log then
        Debug.Log("[ScopeTest_UIManager] " .. message)
    else
        print("[ScopeTest_UIManager] " .. message)
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
    else
        widget:SetAlpha("scopeOverlay", 0.0)
        widget:SetVisible("scopeOverlay", false)
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
end

function Tick(dt)
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
end
