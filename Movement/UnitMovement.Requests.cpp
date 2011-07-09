#include "UnitMovementImpl.h"
#include "WorldPacket.h"
#include "MovementMessage.h"
#include "ClientImpl.h"
#include "Object.h"
#include "MoveSplineInit.h"

namespace Movement
{
    struct ReqRespMsg
    {
        uint16 smsg_request;
        uint16 cmsg_response;
        uint16 msg;
        uint16 smsg_spline;
    };

    /* request-response-msg order*/
#define VALUE_CHANGE(mode)   {SMSG_FORCE_##mode##_CHANGE, CMSG_FORCE_##mode##_CHANGE_ACK, MSG_MOVE_SET_##mode,SMSG_SPLINE_SET_##mode},
    static const ReqRespMsg ValueChange2Opc_table[UnitMovementImpl::Parameter_End] =
    {
        VALUE_CHANGE(WALK_SPEED)
        VALUE_CHANGE(RUN_SPEED)
        VALUE_CHANGE(SWIM_BACK_SPEED)
        VALUE_CHANGE(SWIM_SPEED)
        VALUE_CHANGE(RUN_BACK_SPEED)
        VALUE_CHANGE(FLIGHT_SPEED)
        VALUE_CHANGE(FLIGHT_BACK_SPEED)
        VALUE_CHANGE(TURN_RATE)
        VALUE_CHANGE(PITCH_RATE)
        {SMSG_MOVE_SET_COLLISION_HGT,CMSG_MOVE_SET_COLLISION_HGT_ACK,MSG_MOVE_SET_COLLISION_HGT,0},
    };
#undef CLIENT_VALUE_CHANGE

    class UnitMovementImpl::FloatValueChangeRequest : public RespHandler
    {
        UnitMovementImpl::FloatParameter m_value_type;
        float m_value;

        FloatValueChangeRequest(ClientImpl * client, UnitMovementImpl::FloatParameter value_type, float value) :
            RespHandler(ValueChange2Opc_table[value_type].cmsg_response, client),
            m_value_type(value_type),
            m_value(value)
        { 
            if (uint16 opcode = ValueChange2Opc_table[value_type].smsg_request)
            {
                WorldPacket data(opcode, 32);
                data << client->controlled()->Owner.GetPackGUID();
                data << m_reqId;
                if (m_value_type == UnitMovementImpl::Parameter_SpeedRun)
                    data << int8(0);                               // new 2.1.0
                data << m_value;
                client->SendPacket(data);
            }
        }

    public:

        static void Launch(UnitMovementImpl * mov, UnitMovementImpl::FloatParameter value_type, float value)
        {
            if (mov->IsClientControlled())
            {
                new FloatValueChangeRequest(mov->client(), value_type, value);
            }
            else
            {
                mov->SetParameter(value_type, value);
                if (uint16 opcode = ValueChange2Opc_table[value_type].smsg_spline)
                {
                    WorldPacket data(opcode, 16);
                    data << mov->Owner.GetPackGUID();
                    data << value;
                    MaNGOS_API::BroadcastMessage(&mov->Owner, data);
                }

                // FIXME: currently there is no way to change speed of already moving server-side controlled unit (spline movement)
                // there is only one hacky way - launch new spline movement.. that's how blizz doing this
            }
        }

        virtual void OnReply(ClientImpl * client, WorldPacket& data) override
        {
            ClientMoveState client_state;
            ObjectGuid guid;
            uint32 client_req_id;
            float client_value;
            data >> guid.ReadAsPacked();
            data >> client_req_id;
            data >> client_state;
            data >> client_value;
            if (client_req_id != m_reqId)
            {
                log_write("FloatValueChangeRequest::OnReply: wrong counter value: %u and should be: %u",client_req_id,m_reqId);
                return;
            }
            if (client_value != m_value)
            {
                log_write("FloatValueChangeRequest::OnReply: wrong float value(type %u): %f and should be: %f",m_value_type,client_value,m_value);
                return;
            }
            client->QueueState(client_state);
            client->controlled()->SetParameter(m_value_type, m_value);
            if (uint16 opcode = ValueChange2Opc_table[m_value_type].msg)
            {
                MovementMessage msg(client->controlled(), opcode, 64);
                msg << guid.WriteAsPacked();
                msg << client_state;
                msg << m_value;
                client->BroadcastMessage(msg);
            }
        }
    };

    void UnitMovementImpl::SetCollisionHeight(float value)
    {
        FloatValueChangeRequest::Launch(this, Parameter_CollisionHeight, value);
    }

    void UnitMovementImpl::SetSpeed(SpeedType type, float value)
    {
        if (GetSpeed(type) != value)
            FloatValueChangeRequest::Launch(this, (FloatParameter)type, value);
    }

    struct ModeInfo
    {
        UnitMoveFlag::eUnitMoveFlags moveFlag;
        uint16 smsg_apply[2];   // 0 is apply, 1 - unapply
        uint16 cmsg_ack[2];
        uint16 msg_apply[2];   // 0 is apply, 1 - unapply
        uint16 smsg_spline_apply[2];   // 0 is apply, 1 - unapply
    };

#define CLIENT_MODE_CHANGE(apply,unapply,ack,msg_apply)\
    {SMSG_MOVE_##apply,SMSG_MOVE_##unapply,CMSG_MOVE_##ack##_ACK,MSG_MOVE_##msg_apply,MSG_MOVE_##msg_apply},\

    const ModeInfo modeInfo[MoveModeMaxCount]=
    {
        {
            UnitMoveFlag::Walk_Mode, 0, 0, 0, 0, 0, 0,
                SMSG_SPLINE_MOVE_SET_WALK_MODE, SMSG_SPLINE_MOVE_SET_RUN_MODE
        },
        {
            UnitMoveFlag::Root, SMSG_FORCE_MOVE_ROOT, SMSG_FORCE_MOVE_UNROOT,
                CMSG_FORCE_MOVE_ROOT_ACK, CMSG_FORCE_MOVE_ROOT_ACK,
                MSG_MOVE_ROOT, MSG_MOVE_UNROOT,
                SMSG_SPLINE_MOVE_ROOT, SMSG_SPLINE_MOVE_UNROOT
            },
            {
                UnitMoveFlag::Swimming, 0, 0, 0, 0, 0, 0,
                    SMSG_SPLINE_MOVE_START_SWIM, SMSG_SPLINE_MOVE_STOP_SWIM
            },
            {
                UnitMoveFlag::Waterwalking, SMSG_MOVE_WATER_WALK, SMSG_MOVE_LAND_WALK,
                    CMSG_MOVE_WATER_WALK_ACK, CMSG_MOVE_WATER_WALK_ACK,
                    MSG_MOVE_WATER_WALK, MSG_MOVE_WATER_WALK,
                    SMSG_SPLINE_MOVE_WATER_WALK, SMSG_SPLINE_MOVE_LAND_WALK
                },
                {
                    UnitMoveFlag::Can_Safe_Fall, SMSG_MOVE_FEATHER_FALL, SMSG_MOVE_NORMAL_FALL,
                        CMSG_MOVE_FEATHER_FALL_ACK, CMSG_MOVE_FEATHER_FALL_ACK,
                        MSG_MOVE_FEATHER_FALL, MSG_MOVE_FEATHER_FALL,
                        SMSG_SPLINE_MOVE_FEATHER_FALL, SMSG_SPLINE_MOVE_NORMAL_FALL
                },
                {
                    UnitMoveFlag::Hover, SMSG_MOVE_SET_HOVER, SMSG_MOVE_UNSET_HOVER,
                        CMSG_MOVE_HOVER_ACK, CMSG_MOVE_HOVER_ACK,
                        MSG_MOVE_HOVER, MSG_MOVE_HOVER,
                        SMSG_SPLINE_MOVE_SET_HOVER, SMSG_SPLINE_MOVE_UNSET_HOVER
                    },
                    {
                        UnitMoveFlag::Flying, 0, 0, 0, 0, 0,
                            SMSG_SPLINE_MOVE_SET_FLYING, SMSG_SPLINE_MOVE_UNSET_FLYING
                    },
                    {
                        UnitMoveFlag::GravityDisabled, SMSG_MOVE_GRAVITY_DISABLE, SMSG_MOVE_GRAVITY_ENABLE,
                            CMSG_MOVE_GRAVITY_DISABLE_ACK, CMSG_MOVE_GRAVITY_ENABLE_ACK,
                            MSG_MOVE_GRAVITY_CHNG, MSG_MOVE_GRAVITY_CHNG,
                            SMSG_SPLINE_MOVE_GRAVITY_DISABLE, SMSG_SPLINE_MOVE_GRAVITY_ENABLE
                        },
                        {
                            UnitMoveFlag::Can_Fly, SMSG_MOVE_SET_CAN_FLY, SMSG_MOVE_UNSET_CAN_FLY,
                                CMSG_MOVE_SET_CAN_FLY_ACK, CMSG_MOVE_SET_CAN_FLY_ACK,
                                MSG_MOVE_UPDATE_CAN_FLY, MSG_MOVE_UPDATE_CAN_FLY,
                                SMSG_SPLINE_MOVE_SET_FLYING, SMSG_SPLINE_MOVE_UNSET_FLYING
                        },
    };

    class ModeChangeRequest : public RespHandler
    {
        MoveMode m_mode;
        bool m_apply;

        ModeChangeRequest(ClientImpl * client, MoveMode mode, bool apply) :
            RespHandler(modeInfo[mode].cmsg_ack[!apply],client), m_mode(mode), m_apply(apply)
        {
            if (uint16 opcode = modeInfo[mode].smsg_apply[!apply])
            {
                MovementMessage msg(NULL, opcode, 16);
                msg << client->controlled()->Owner.GetPackGUID();
                msg << m_reqId;
                client->SendMoveMessage(msg);
            }
        }

    public:

        static void Launch(UnitMovementImpl * mov, MoveMode mode, bool apply)
        {
            if (mov->IsClientControlled())
                new ModeChangeRequest(mov->client(), mode, apply);
            else
            {
                mov->ApplyMoveFlag(modeInfo[mode].moveFlag, apply);
                if (uint16 opcode = modeInfo[mode].smsg_spline_apply[!apply])
                {
                    WorldPacket data(opcode, 12);
                    data << mov->Owner.GetPackGUID();
                    MaNGOS_API::BroadcastMessage(&mov->Owner, data);
                }
            }
        }

        virtual void OnReply(ClientImpl * client, WorldPacket& data) override
        {
            ClientMoveState client_state;
            ObjectGuid guid;
            uint32 client_req_id;

            data >> guid.ReadAsPacked();
            data >> client_req_id;
            data >> client_state;
            if (data.rpos() != data.size())
                data >> Unused<float>();          // 0 or 1, unused

            if (client_req_id != m_reqId)
            {
                log_write("FloatValueChangeRequest::OnReply: wrong counter value: %u and should be: %u",client_req_id,m_reqId);
                return;
            }
            if (modeInfo[m_mode].moveFlag != 0 && m_apply != (bool)(client_state.moveFlags & modeInfo[m_mode].moveFlag))
            {
                log_write("ModeChangeRequest::OnReply: wrong client's flag");
                return;
            }

            // Should i queue state or apply it immediately?
            // very often incoming client state is  from past time..
            client->QueueState(client_state);

            if (uint16 opcode = modeInfo[m_mode].msg_apply[!m_apply])
            {
                MovementMessage msg(client->controlled(), opcode, 64);
                msg << guid.WriteAsPacked();
                msg << client_state;
                client->BroadcastMessage(msg);
            }
        }
    };

    void UnitMovementImpl::ApplyMoveMode(MoveMode mode, bool apply)
    {
        ModeChangeRequest::Launch(this, mode, apply);
    }

    bool UnitMovementImpl::HasMode(MoveMode m) const
    {
        return moveFlags & modeInfo[m].moveFlag;
    }

    class TeleportRequest : public RespHandler
    {
        Location m_location;

        TeleportRequest(ClientImpl * client, const Location& loc) :
            RespHandler(MSG_MOVE_TELEPORT_ACK, client), m_location(loc)
        {
            ClientMoveState state(client->controlled()->ClientState());
            // TODO: add set of functions for state modifying
            if (state.moveFlags.ontransport)
                state.transport_position = m_location;
            else
                state.world_position = m_location;

            MovementMessage msg(NULL, MSG_MOVE_TELEPORT_ACK, 64);   // message source is null - client shouldn't skip that message
            msg << client->controlled()->Owner.GetPackGUID();
            msg << m_reqId;
            msg << state;
            client->SendMoveMessage(msg);
        }

    public:

        static void Launch(UnitMovementImpl * mov, const Location& loc)
        {
            if (mov->client())
            {
                new TeleportRequest(mov->client(), loc);
            }
            else
            {
                mov->SetPosition(loc);
                MovementMessage msg(mov, MSG_MOVE_TELEPORT, 64);
                msg << mov->Owner.GetPackGUID();
                msg << mov->ClientState();
                MaNGOS_API::BroadcastMessage(&mov->Owner, msg);
            }
        }

        virtual void OnReply(ClientImpl * client, WorldPacket& data) override
        {
            ObjectGuid guid;
            uint32 client_req_id;
            MSTime client_time;
            data >> guid.ReadAsPacked();
            data >> client_req_id;
            data >> client_time;

            if (client_req_id != m_reqId)
            {
                log_write("FloatValueChangeRequest::OnReply: wrong counter value: %u and should be: %u",client_req_id,m_reqId);
                return;
            }

            client->controlled()->SetPosition(m_location);

            MovementMessage msg(client->controlled(), MSG_MOVE_TELEPORT, 64);
            msg << guid.WriteAsPacked();
            msg << client->controlled()->ClientState();
            client->BroadcastMessage(msg);
        }
    };

    void UnitMovementImpl::Teleport(const Location& loc)
    {
        TeleportRequest::Launch(this, loc);
    }
}