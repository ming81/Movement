
#include "packet_builder.h"
#include "mov_constants.h"
#include "opcodes.h"
#include "OutLog.h"

#include "Movement.h"
#include "SplineState.h"
#include <assert.h>

#include "WorldPacket_fake.h"

namespace Movement
{
    PacketBuilder::PacketBuilder(MovementState *const dat, MovControlType c)
        : mode(c), mov(*dat)
    {
    }

    PacketBuilder::~PacketBuilder()
    {
    }

    typedef void (PacketBuilder::*SpeedPtr)(SpeedType,WorldPacket&) const;
    typedef void (PacketBuilder::*MoveModePtr)(MoveMode,WorldPacket&) const;
    typedef void (PacketBuilder::*PathPtr)(WorldPacket&) const;

    void PacketBuilder::SpeedUpdate(SpeedType type, WorldPacket& p) const
    {
        static const SpeedPtr speed_ptrs[MovControlCount] =
        {
            &PacketBuilder::Client_SpeedUpdate,
            &PacketBuilder::Spline_SpeedUpdate,
        };

        (this->*speed_ptrs[mode])(type, p);
    }

    void PacketBuilder::MoveModeUpdate(MoveMode move_mode, WorldPacket& p) const
    {
        static const MoveModePtr move_mode_ptrs[MovControlCount] =
        {
            &PacketBuilder::Client_MoveModeUpdate,
            &PacketBuilder::Spline_MoveModeUpdate,
        };

        (this->*move_mode_ptrs[mode])(move_mode, p);
    }

    void PacketBuilder::PathUpdate(WorldPacket& p) const
    {
        static const PathPtr path_update_ptrs[MovControlCount] =
        {
            &PacketBuilder::Client_PathUpdate,
            &PacketBuilder::Spline_PathUpdate,
        };

        (this->*path_update_ptrs[mode])(p);
    }


    void PacketBuilder::Spline_SpeedUpdate(SpeedType type, WorldPacket& data) const
    {
        uint16 opcode = S_Speed2Opc_table[type];
        sLog.write("PacketBuilder:  created %s message", OpcodeName(opcode));

        data.Initialize(opcode, 8+4);
        data << mov.m_owner->GetPackGUID();
        data << mov.GetSpeed(type);
    }

    void PacketBuilder::Spline_MoveModeUpdate(MoveMode mode, WorldPacket& data) const
    {
        uint16 opcode = S_Mode2Opc_table[mode][mov.HasMode(mode)];
        sLog.write("PacketBuilder:  created %s message", OpcodeName(opcode));

        data.Initialize(opcode, 8+4);
        data << mov.m_owner->GetPackGUID();
    }

    void PacketBuilder::Spline_PathUpdate(WorldPacket& data) const
    {
        const MoveSpline& splineInfo = mov.splineInfo;
        const SplinePure::PointsArray& path = splineInfo.getPath();

        assert(splineInfo.nodes_count());

        uint16 opcode = SMSG_MONSTER_MOVE;
        sLog.write("PacketBuilder:  created %s message", OpcodeName(opcode));

        data.Initialize(opcode, 30);
        data << mov.m_owner->GetPackGUID();
        data << uint8(0);
        Vector3 start = mov.position.xyz();
        data << start;

        data << uint32(splineInfo.sequience_Id);

        uint32 nodes_count = path.size();
        uint32 splineflags = splineInfo.GetSplineFlags();  // spline flags are here? not sure...

        if(splineflags & SPLINE_MASK_FINAL_FACING)
        {
            if (splineflags & SPLINEFLAG_FINALTARGET)
            {
                data << uint8(SPLINETYPE_FACINGTARGET);
                data << splineInfo.facing_target;
            }
            else if(splineflags & SPLINETYPE_FACINGANGLE)
            {
                data << uint8(SPLINETYPE_FACINGANGLE);
                data << splineInfo.facing_angle;
            }
            else if(splineflags & SPLINEFLAG_FINALFACING)
            {
                data << uint8(SPLINETYPE_FACINGSPOT);
                data << splineInfo.facing_spot.x << splineInfo.facing_spot.y << splineInfo.facing_spot.z;
            }
            else
                assert(false);
        }
        else
            data << uint8(SPLINETYPE_NORMAL);

        data << uint32(splineflags & ~SPLINE_MASK_NO_MONSTER_MOVE);
        data << uint32(nodes_count);

        if (splineflags & SPLINEFLAG_ANIMATION)
        {
            data << splineInfo.animationType;
            data << splineInfo.animationTime;
        }

        data << splineInfo.duration;

        if (splineflags & SPLINEFLAG_TRAJECTORY)
        {
            data << splineInfo.z_acceleration;
            data << splineInfo.duration_mod;
        }

        if(splineflags & (SPLINEFLAG_FLYING | SPLINEFLAG_CATMULLROM))
        {
            for(uint32 i = 0; i < nodes_count; ++i)
                data << path[i];
        }
        else
        {
            const Vector3 &dest = path[nodes_count-1];
            data << dest;   // destination

            if(nodes_count > 1)
            {
                Vector3 vec = (start + dest) / 2;

                for(uint32 i = 0; i < nodes_count - 1; ++i)// "nodes_count-1" because destination point already appended
                {
                    Vector3 temp = vec - path[i];
                    data.appendPackXYZ(temp.x, temp.y, temp.z);
                }
            }
        }
    }



    void PacketBuilder::Client_MoveModeUpdate(MoveMode /*type*/, WorldPacket& data) const
    {
        //mov.wow_object->BuildHeartBeatMsg(&mov.wow_object->PrepareSharedMessage());
    }

    void PacketBuilder::Client_SpeedUpdate(SpeedType ty, WorldPacket& data) const
    {
        bool forced = true;

        //WorldObject *m = mov.wow_object;
        uint16 opcode = SetSpeed2Opc_table[ty][forced];
        sLog.write("PacketBuilder:  created %s message", OpcodeName(opcode));

        //WorldPacket& data = m->PrepareSharedMessage(opcode, 10); 
        //data << mov.m_owner->GetPackGUID();

        //if(!forced)
        //{
        //    m->WriteMovementBlock(data);
        //}
        //else
        //{
        //    if(m->GetTypeId() == TYPEID_PLAYER)
        //        ++((Player*)this)->m_forced_speed_changes[ty];
        //    data << (uint32)0;                                  // moveEvent, NUM_PMOVE_EVTS = 0x39
        //    if (ty == MOVE_RUN)
        //        data << uint8(0);                               // new 2.1.0
        //}

        //data << mov.GetSpeed(ty);
    }

    void PacketBuilder::Client_PathUpdate(WorldPacket& data) const
    {
        // do nothing
    }

    void PacketBuilder::FullUpdate( ByteBuffer& data) const
    {
        data << mov.moveFlags;
        data << mov.move_flags2;

        data << mov.last_ms_time_fake;
        data << mov.position;

        if (mov.HasMovementFlag(MOVEFLAG_ONTRANSPORT))
        {
            data << mov.m_transport.t_guid;
            data << mov.m_transport.t_offset;
            data << mov.m_transport.t_time;
            data << mov.m_transport.t_seat;

            if (mov.move_flags2 & MOVEFLAG2_INTERP_MOVE)
                data << mov.m_transport.t_time2;
        }
        
        if (mov.HasMovementFlag(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING) || (mov.move_flags2 & MOVEFLAG2_ALLOW_PITCHING))
        {
            data << mov.s_pitch;
        }

        data << mov.fallTime;

        if (mov.HasMovementFlag(MOVEFLAG_FALLING))
        {
            data << mov.j_velocity;
            data << mov.j_sinAngle;
            data << mov.j_cosAngle;
            data << mov.j_xy_velocy;
        }

        if (mov.HasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        {
            data << mov.u_unk1;
        }

        for (int i = SpeedWalk; i < SpeedMaxCount; ++i)
            data << mov.GetSpeed((SpeedType)i);
        
        if (mov.HasMovementFlag(MOVEFLAG_SPLINE_ENABLED))
        {
            // for debugging
            static float unkf1 = 1.f;
            static float unkf2 = 1.f;
            static float unkf3 = 0.f;
            static float dur_multiplier = 1.f;

            static uint32 addit_flags = 0;

            const MoveSpline& splineInfo = mov.splineInfo;
            uint32 splineFlags = mov.splineInfo.splineflags | addit_flags;

            data << splineFlags;

            if(splineFlags & SPLINEFLAG_FINALFACING)             // may be orientation
            {
                data << splineInfo.facing_angle;
            }
            else
            {
                if(splineFlags & SPLINEFLAG_FINALTARGET)         // probably guid there
                {
                    data << splineInfo.facing_target;
                }
                else
                {
                    if(splineFlags & SPLINEFLAG_FINALPOINT)      // probably x,y,z coords there
                    {
                        data << splineInfo.facing_spot.x << splineInfo.facing_spot.y << splineInfo.facing_spot.z;
                    }
                }
            }

            data << uint32(splineInfo.time_passed);
            data << uint32(splineInfo.duration * dur_multiplier);
            data << splineInfo.sequience_Id;

            data << unkf1;                              // added in 3.1
            data << unkf2;                              // added in 3.1
            data << unkf3;                                // added in 3.1

            data << uint32(0);                               // added in 3.1

            uint32 nodes = splineInfo.nodes_count();
            data << nodes;
            for (uint32 i = 0; i < nodes; ++i)
            {
                data << splineInfo.getNode(i);
            }

            data << uint8(splineInfo.mode());

            data << splineInfo.finalDestination;
        }



    }
}
