
/**
  file:         UnitMovement.h
  author:       SilverIce
  email:        slifeleaf@gmail.com
  created:      16:2:2011
*/

#pragma once

#include "MovementBase.h"
#include "mov_constants.h"
#include "ClientMoveStatus.h"
#include "MoveSpline.h"
#include <list>

namespace Movement
{
    class MoveSpline;
    class ClientImpl;
    struct MoveSplineInitArgs;

    // Manages by sequential set of client movement states
    class MoveStateSet
    {
        // TODO: more memory efficient storage
        std::list<ClientMoveState> m_state_queue;
        const ClientMoveState& LastQueuedState() const { return m_state_queue.front();}     // may crash in case no states queued
        const ClientMoveState& CurrentState() const { return m_state_queue.back();}     // may crash in case no states queued
    public:
        explicit MoveStateSet() {}

        void QueueState(const ClientMoveState& state) { m_state_queue.push_front(state);}
        bool Next(ClientMoveState& state, MSTime time_now);
        void Clear() { m_state_queue.clear();}
        size_t Size() const { return m_state_queue.size();}
    };

    struct TargetLink
    {
        TargetLink() : target(0), targeter(0) {}
        TargetLink(UnitMovementImpl* target_, UnitMovementImpl* targeter_)
            : target(target_), targeter(targeter_) {}

        UnitMovementImpl* target;
        UnitMovementImpl* targeter;
    };

    // class for unit's movement
    class UnitMovementImpl : public Transportable, public IUpdatable
    {
    public:

        explicit UnitMovementImpl(WorldObjectType owner);
        virtual ~UnitMovementImpl();

        virtual void CleanReferences();
        virtual void UpdateState();

        std::string ToString() const;

        void Initialize(const Location& position, MoveUpdater& updater);

        /* Needed for monster movement only*/
        void BindOrientationTo(UnitMovementImpl& target);
        void UnbindOrientation();
        bool IsOrientationBinded() const { return m_target_link.linked(); }
        const UnitMovementImpl* GetTarget() const { return m_target_link.Value.target;}

        const Location& GetPosition() const { return IsBoarded() ? m_local_position : world_position;}
        const Vector3& GetPosition3() const { return GetPosition();}
        Vector3 direction() const;

        virtual void BoardOn(Transport& transport, const Location& local_position, int8 seatId);
        virtual void Unboard();
        void UnboardAll() { m_transport.UnBoardAll();}
        void Board(Transportable& t, const Location& local_position, int8 seatId) { t.BoardOn(m_transport, local_position, seatId);}

    public:
        // Used by server side controlled movement
        void LaunchMoveSpline(MoveSplineInitArgs& args);
        uint32 MoveSplineId() const;
        const Vector3& MoveSplineDest() const;
        int32 MoveSplineTimeElapsed() const;
        void DisableSpline() { moveFlags &= ~(UnitMoveFlag::Mask_Moving | UnitMoveFlag::Spline_Enabled);}

        void SetListener(IListener * l) { m_listener = l;}
        void ResetLisener() { m_listener = NULL; }

    public:
        /** Seems it should be removed(or used for monster movement only), since it hard or impossible to get movement mode from incoming movement packets*/
        /// Move Modes
        bool HasMode(MoveMode m) const;
        void ApplyMoveMode(MoveMode mode, bool apply);

        /// Apply/remove modes
        void ApplyRootMode(bool apply);
        void ApplySwimMode(bool apply) { ApplyMoveMode(MoveModeSwim, apply); }
        void ApplyWalkMode(bool apply) { ApplyMoveMode(MoveModeWalk, apply); }
        void ApplyWaterWalkMode(bool apply) { ApplyMoveMode(MoveModeWaterwalk, apply); }
        void ApplySlowFallMode(bool apply) { ApplyMoveMode(MoveModeSlowfall, apply); }
        void ApplyFlyMode(bool apply) { ApplyMoveMode(MoveModeFly, apply); }
        void ApplyHoverMode(bool apply) { ApplyMoveMode(MoveModeHover, apply); }

        void Teleport(const Location& loc);

        bool IsWalking() const { return moveFlags.walk_mode;}
        bool IsFlying() const { return moveFlags & (UnitMoveFlag::Flying | UnitMoveFlag::GravityDisabled);}
        bool IsMoving() const { return moveFlags & UnitMoveFlag::Mask_Moving;}
        bool SplineEnabled() const { return moveFlags.spline_enabled; }
        bool IsTurning() const { return moveFlags & (UnitMoveFlag::Turn_Left | UnitMoveFlag::Turn_Right);}
        bool IsFalling() const { return moveFlags & (UnitMoveFlag::Falling);}
        bool IsFallingFar() const { return moveFlags & (UnitMoveFlag::Fallingfar);}

        void SetCollisionHeight(float value);
        float GetCollisionHeight() const { return GetParameter(Parameter_CollisionHeight);}

        void SetSpeed(SpeedType type, float s);
        float GetSpeed(SpeedType type) const { return GetParameter((FloatParameter)(0 + type)); }
        float GetCurrentSpeed() const { return GetParameter(Parameter_SpeedCurrent); }

        #pragma region Impl
    private:
        enum{
        /** Affects spline movement precision & performance,
            makes spline movement to be updated once per N milliseconds. */
            MoveSpline_UpdateDelay = 200,

        /** Upper limit for diff time values, milliseconds. Useful when movement wasn't get updated for a long time. */
            Maximum_update_difftime = 10000,
        };

        struct MoveSplineUpdater;
        class FloatValueChangeRequest;

        void setLastUpdate(MSTime time) { last_update_time = time;}
        MSTime getLastUpdate() const { return last_update_time;}

        void ApplyState(const ClientMoveState& state);
    public:
        enum FloatParameter
        {
            Parameter_SpeedWalk,
            Parameter_SpeedRun,
            Parameter_SpeedSwimBack,
            Parameter_SpeedSwim,
            Parameter_SpeedRunBack,
            Parameter_SpeedFlight,
            Parameter_SpeedFlightBack,
            Parameter_SpeedTurn,
            Parameter_SpeedPitch,
            Parameter_CollisionHeight,
            Parameter_SpeedCurrent,
            Parameter_End,
        };

        void SetParameter(FloatParameter p, float value) { m_float_values[p] = value;}
        float GetParameter(FloatParameter p) const { return m_float_values[p];}

        void ApplyMoveFlag(UnitMoveFlag::eUnitMoveFlags f, bool apply)
        {
            if (apply)
                SetMoveFlag(moveFlags | f);
            else
                SetMoveFlag(moveFlags & ~f);
        }

        void SetMoveFlag(const UnitMoveFlag& newFlags);

        void _QueueState(const ClientMoveState& state) { m_moveEvents.QueueState(state);}       // only for call from Client code
        ClientMoveState ClientState() const;

        bool HasUpdater() const { return updatable.HasUpdater();}
        MoveUpdater& GetUpdater() const { return updatable.GetUpdater();}
        void SetPosition(const Location& v);
        void SetPosition(const Vector3& v) { SetPosition(Location(v,GetPosition().orientation));}

        ClientImpl* client() const { return m_client;}
        void client(ClientImpl* c) { m_client = c;}
        bool IsClientControlled() const { return m_client && !SplineEnabled();}
        bool IsServerControlled() const { return !m_client;}
    private:
        static SpeedType SelectSpeedType(UnitMoveFlag moveFlags);

        void DisableSpline() { moveFlags &= ~(UnitMoveFlag::Mask_Directions | UnitMoveFlag::Spline_Enabled);}
        void PrepareMoveSplineArgs(MoveSplineInitArgs&, UnitMoveFlag&) const;

        void updateRotation();

    private:
        friend class PacketBuilder;
        friend class UnitMovement;

        UpdatableMovement updatable;
        MoveSpline move_spline;
        IListener* m_listener;
        ClientImpl* m_client;
        MSTime last_update_time;
        MoveStateSet m_moveEvents;
        Transport m_transport;

        UnitMoveFlag moveFlags;
        /** Data that cames from client. It affects nothing here but might be used in future. */
        _ClientMoveState m_unused; 

        float m_float_values[Parameter_End];
        LinkedListElement<TargetLink> m_target_link;
        LinkedList<TargetLink> m_targeter_references;
        #pragma endregion
    };






    struct OnEventArgs
    {
        enum EventType{
            PointDone,
            Arrived,
        };

        static OnEventArgs OnArrived(uint32 splineId)
        {
            OnEventArgs args = {Arrived, splineId, 0};
            return args;
        }

        static OnEventArgs OnPoint(uint32 splineId, int32 pointId)
        {
            OnEventArgs args = {PointDone, splineId, pointId};
            return args;
        }

        bool isArrived() const { return type == Arrived;}
        bool isPointDone() const { return type == PointDone;}

        EventType type;
        uint32 splineId;
        int32 data;
    };

}
