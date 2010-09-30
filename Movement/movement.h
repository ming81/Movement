
#pragma once

#include "typedefs.h"
#include "mov_constants.h"
#include "packet_builder.h"
#include "SplineState.h"


namespace Movement
{
    class WorldObject;


    class MovementState
    {

    public:
        MovementState() :  msg_builder(this, MovControlServer)
        {
        }

        ~MovementState()
        {
        }


        PacketBuilder& GetBuilder() { return msg_builder; }
        PacketBuilder msg_builder;

        //WorldObject* wow_object;

        /// Get-Set methtods

        void SetSpeed(UnitMoveType type, float s) { speed[type] = s; }
        float GetSpeed(UnitMoveType type) const { return speed[type]; }
        float GetCurrentSpeed() const { return speed[mt]; }
        void SetMoveType(UnitMoveType type) { mt = type; }

        void AddMovementFlag(uint32 f) { moveFlags |= f; }
        void RemoveMovementFlag(uint32 f) { moveFlags &= ~f; }
        bool HasMovementFlag(uint32 f) const { return moveFlags & f; }
        uint32 GetMovementFlags() const { return moveFlags; }
        void SetMovementFlags(uint32 f) { moveFlags = f; }

        /// Direction info
        void AddDirectionFlag(uint8 f) { direction_flags |= f; }
        void RemoveDirectionFlag(uint8 f) { direction_flags &= ~f; }
        bool HasDirectionFlag(uint8 f) const { return direction_flags & f; }
        uint8 GetDirectionFlags() const { return direction_flags; }
        void SetDirectionFlags(uint8 f) { direction_flags = f; }

        bool HasDest() const { return direction_flags & DIRECTIONS_MASK; }
        void Stop()  { direction_flags &= ~DIRECTIONS_MASK; }

        /// Move Modes
        bool HasMode(MoveMode m) const { return move_mode & (1 << m);}

        void SetMoveMode(MoveMode mode, bool began)
        {
            if (began)
            {
                AddMovementFlag(Mode2Flag_table[mode]);
                move_mode |= 1 << mode;
            }
            else
            {
                RemoveMovementFlag(Mode2Flag_table[mode]);
                move_mode &= ~(1 << mode);
            }
        }

        /// Apply/remove modes
        void Root(bool apply) { SetMoveMode(MoveModeRoot, apply); }
        void Swim(bool apply) { SetMoveMode(MoveModeSwim, apply); }
        void Walk(bool apply) { SetMoveMode(MoveModeWalk, apply); }
        void WaterWalk(bool apply) { SetMoveMode(MoveModeWaterwalk, apply); }
        void SlowFall(bool apply) { SetMoveMode(MoveModeSlow_fall, apply); }
        void Fly(bool apply) { SetMoveMode(MoveModeFly, apply); }
        void Hover(bool apply) { SetMoveMode(MoveModeHover, apply); }

        /// end of Get-Set methtods

        uint32 move_mode;

        // position-time pair
        Vector3 position;// current position
        uint32 time_stamp;

        union
        {
            uint8 direction_flags;
            uint32 moveFlags;
        };
        uint16 move_flags2;

        /// Transport info
        struct TransportData
        {
            uint64 t_guid;
            Vector3 t_offset;
            uint32 t_time;
        } m_transport;

        struct 
        {
            float current;
            float walk;
            float run;
            float run_back;
            float swim;
            float swim_back;
            float flight;
            float flight_back;
            float turn;
            float pitch;
            //float speed_14;

        } speed_block;
        
        float speed[MAX_MOVE_TYPE];

        UnitMoveType mt;
        /// end generic data

        /// Spline move data

        SplineState spline;

        /// end spline data


        /// Some client's formulas:

        float CalculateCurrentSpeed(bool is_walking = false) const
        {
            uint32 splineflags = spline.splineflags;

            // g_moveFlags_mask - some global client's moveflag mask
            // TODO: get real value
            static uint32 g_moveFlags_mask = 0xFFFFFFFF;
            float speed = 0.0f;

            if ( !(g_moveFlags_mask & moveFlags) )
                return 0.0f;

            if ( /*!spline ||*/ splineflags & SPLINEFLAG_NO_SPLINE )
            {
                if ( moveFlags & MOVEFLAG_FLYING )
                {
                    if ( moveFlags & MOVEFLAG_BACKWARD && speed_block.flight >= speed_block.flight_back )
                        return speed_block.flight_back;
                    else
                        return speed_block.flight;
                }
                else if ( moveFlags & MOVEFLAG_SWIMMING )
                {
                    if ( moveFlags & MOVEFLAG_BACKWARD && speed_block.swim >= speed_block.swim_back )
                        return speed_block.swim_back;
                    else
                        return speed_block.swim;
                }
                else
                {
                    if ( moveFlags & MOVEFLAG_WALK_MODE || is_walking )
                    {
                        if ( speed_block.run > speed_block.walk )
                            return speed_block.walk;
                    }
                    else
                    {
                        if ( moveFlags & MOVEFLAG_BACKWARD && speed_block.run >= speed_block.run_back )
                            return speed_block.run_back;
                    }
                    return speed_block.run;
               }
            }
            else
            {
                if ( !spline.move_time_full )
                    return 0.0f;
                speed = spline.total_lenght / spline.move_time_full * 1000.0f;
            }
            return speed;
        }

    };



}