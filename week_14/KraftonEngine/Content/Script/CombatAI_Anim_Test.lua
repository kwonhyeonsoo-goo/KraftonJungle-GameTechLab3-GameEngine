local animInstance = nil
local combatAgent = nil
local fireElapsed = 0.0
local lastMoveState = -1.0
local lastDeath = false
local lastEngaging = false

local FIRE_TRIGGER_INTERVAL = 0.45

local function find_anim_instance()
    if obj == nil then
        return nil
    end

    local mesh = nil
    if obj.GetSkeletalMeshComponent ~= nil then
        mesh = obj:GetSkeletalMeshComponent()
    end
    if mesh == nil and obj.GetMesh ~= nil then
        mesh = obj:GetMesh()
    end
    if mesh == nil or mesh.GetAnimInstance == nil then
        return nil
    end

    return mesh:GetAnimInstance()
end

local function get_anim_instance()
    if animInstance == nil then
        animInstance = find_anim_instance()
    end
    return animInstance
end

local function find_combat_agent()
    if obj == nil then
        return nil
    end

    if obj.GetCombatCoverAgentComponent ~= nil then
        local agent = obj:GetCombatCoverAgentComponent()
        if agent ~= nil then
            return agent
        end
    end

    if obj.GetComponents == nil then
        return nil
    end

    local components = obj:GetComponents()
    for _, component in pairs(components) do
        if component ~= nil and component.IsA ~= nil and component:IsA("UCombatCoverAgentComponent") then
            return component
        end
    end

    return nil
end

local function get_combat_agent()
    if combatAgent == nil then
        combatAgent = find_combat_agent()
    end
    return combatAgent
end

local function set_initial_variables()
    local anim = get_anim_instance()
    if anim == nil then
        return false
    end

    anim:SetGraphVariableFloat("MoveState", 0.0)
    anim:SetGraphVariableBool("Death", false)
    return true
end

local function current_move_state(agent)
    if agent == nil then
        return 0.0
    end
    if agent:IsMovingForCombatRange() then
        return 2.0
    end
    if agent:IsInCover() or agent:IsEngaging() or agent:IsSuppressed() or agent:GetIncomingFireCount() > 0 then
        return 1.0
    end
    return 0.0
end

function BeginPlay()
    animInstance = find_anim_instance()
    combatAgent = find_combat_agent()
    fireElapsed = 0.0
    lastMoveState = -1.0
    lastDeath = false
    lastEngaging = false
    set_initial_variables()
end

function Tick(dt)
    local anim = get_anim_instance()
    if anim == nil then
        return
    end

    local agent = get_combat_agent()
    local isDead = agent ~= nil and not agent:IsAlive()
    local isEngaging = agent ~= nil and agent:IsEngaging()
    local moveState = current_move_state(agent)

    anim:SetGraphVariableFloat("MoveState", moveState)
    anim:SetGraphVariableBool("Death", isDead)

    if moveState ~= lastMoveState or isDead ~= lastDeath then
        fireElapsed = 0.0
        lastMoveState = moveState
        lastDeath = isDead
    end

    if isEngaging and not lastEngaging then
        fireElapsed = FIRE_TRIGGER_INTERVAL
    end
    lastEngaging = isEngaging

    if isDead then
        return
    end

    if isEngaging then
        fireElapsed = fireElapsed + dt
        while fireElapsed >= FIRE_TRIGGER_INTERVAL do
            fireElapsed = fireElapsed - FIRE_TRIGGER_INTERVAL
            anim:SetGraphVariableTrigger("Fire")
        end
    else
        fireElapsed = 0.0
    end
end
