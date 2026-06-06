local animInstance = nil
local elapsed = 0.0
local fireElapsed = 0.0
local lastMoveState = -1
local deathSet = false

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

local function set_initial_variables()
    local anim = get_anim_instance()
    if anim == nil then
        return false
    end

    anim:SetGraphVariableFloat("MoveState", 0.0)
    anim:SetGraphVariableBool("Death", false)
    return true
end

local function current_move_state()
    if elapsed < 10.0 then
        return 0.0
    end
    if elapsed < 20.0 then
        return 2.0
    end
    return 1.0
end

function BeginPlay()
    elapsed = 0.0
    fireElapsed = 0.0
    lastMoveState = -1
    deathSet = false
    animInstance = find_anim_instance()
    set_initial_variables()
end

function Tick(dt)
    local anim = get_anim_instance()
    if anim == nil then
        return
    end

    elapsed = elapsed + dt

    local moveState = current_move_state()
    anim:SetGraphVariableFloat("MoveState", moveState)

    if moveState ~= lastMoveState then
        fireElapsed = 0.0
        lastMoveState = moveState
    end

    if moveState == 1.0 and not deathSet then
        fireElapsed = fireElapsed + dt
        while fireElapsed >= 2.0 do
            fireElapsed = fireElapsed - 2.0
            anim:SetGraphVariableTrigger("Fire")
        end
    end

    if elapsed >= 30.0 and not deathSet then
        anim:SetGraphVariableBool("Death", true)
        deathSet = true
    end
end
