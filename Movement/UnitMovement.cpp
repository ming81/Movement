
#include "UnitMovement.h"
#include "Client.h"
#include "MoveSplineInit.h"

#include <sstream>
#include <list>
#include <hash_map>
#include "typedefs_p.h"

#include "Object.h"
#include "opcodes.h"
#include "WorldPacket.h"
#include "Imports.h"
#include "MoveSpline.h"
#include "MoveUpdater.h"
#include "MoveListener.h"
#include "MovementBase.h"

#include "UnitMoveFlags.h"
#include "ClientMoveStatus.h"
#include "packet_builder.h"
#include "MovementMessage.h"
#include "UpdatableSpline.h"
#include "UnitMovementImpl.h"
#include "ClientImpl.h"

#include "MoveSplineInit.hpp"
#include "packet_builder.hpp"
#include "ClientImpl.hpp"
#include "UnitMovementImpl.hpp"
#include "UnitMovement.Requests.hpp"
#include "UpdatableSpline.hpp"

#include "UnitMovement.Tests.hpp"

namespace Movement
{
    UnitMovement* UnitMovement::create(WorldObjectType owner, uint64 ownerGuid, MoveUpdater& updater)
    {
        char * data = (char*)operator new(sizeof(UnitMovement) + sizeof(UnitMovementImpl));
        UnitMovementImpl* impl = new(data + sizeof(UnitMovement))UnitMovementImpl(owner, ownerGuid, updater);
        return new(data)UnitMovement(*impl);
    }

    void UnitMovement::dealloc()
    {
        m.~UnitMovementImpl();
        delete this;
    }

    void UnitMovement::CleanReferences()
    {
        m.CleanReferences();
    }

    void UnitMovement::SetPosition(const Location& position)
    {
        m.SetPosition(position);
    }

    const Location& UnitMovement::GetPosition() const
    {
        return m.GetPosition();
    }

    bool UnitMovement::IsWalking() const
    {
        return m.IsWalking();
    }

    bool UnitMovement::IsMoving() const
    {
        return m.IsMoving();
    }

    bool UnitMovement::IsTurning() const
    {
        return m.IsTurning();
    }

    bool UnitMovement::IsFlying() const
    {
        return m.IsFlying();
    }

    bool UnitMovement::IsFalling() const
    {
        return m.IsFalling();
    }

    bool UnitMovement::IsFallingFar() const
    {
        return m.IsFallingFar();
    }

    float UnitMovement::GetCollisionHeight() const
    {
        return m.GetParameter(Parameter_CollisionHeight);
    }

    float UnitMovement::GetSpeed(SpeedType type) const
    {
        return m.GetParameter((FloatParameter)(0 + type));
    }

    float UnitMovement::GetCurrentSpeed() const
    {
        return m.GetParameter(Parameter_SpeedMoveSpline);
    }

    std::string UnitMovement::ToString() const
    {
        return m.ToString();
    }

    void UnitMovement::BindOrientationTo(UnitMovement& target)
    {
        m.BindOrientationTo(target.Impl());
    }

    void UnitMovement::UnbindOrientation()
    {
        m.UnbindOrientation();
    }

    Vector3 UnitMovement::direction() const
    {
        return m.direction();
    }

    uint32 UnitMovement::MoveSplineId() const
    {
        return m.move_spline->getCurrentMoveId();
    }

    bool UnitMovement::HasMode(MoveMode mode) const
    {
        return m.HasMode(mode);
    }

    void UnitMovement::Teleport(const Location& loc)
    {
        m.Teleport(loc);
    }

    void UnitMovement::SetCollisionHeight(float value)
    {
        m.SetCollisionHeight(value);
    }

    void UnitMovement::ApplyMoveMode(MoveMode mode, bool apply)
    {
        m.ApplyMoveMode(mode, apply);
    }


    void UnitMovement::WriteCreate(ByteBuffer& buf) const
    {
        PacketBuilder::FullUpdate(Impl(), buf);
    }

    void UnitMovement::SetListener(class IListener * listener)
    {
        m.move_spline->SetListener(listener);
    }

    void UnitMovement::SetSpeed(SpeedType type, float speed)
    {
        m.SetSpeed(type, speed);
    }

    void UnitMovement::Initialize(const Location& position, MoveUpdater& updater)
    {
        m.Initialize(position, updater);
    }
}
