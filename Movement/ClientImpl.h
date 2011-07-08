
/**
  file:         Session.h
  author:       SilverIce
  email:        slifeleaf@gmail.com
  created:      20:2:2011
*/

#pragma once

#include "typedefs.h"
#include "MaNGOS_API.h"
#include "ClientMoveStatus.h"
#include "UnitMovementImpl.h"

class ByteBuffer;
class WorldPacket;

namespace Movement
{
    class MovementMessage;
    class UnitMovementImpl;
    class ClientImpl;

    class RespHandler
    {
    protected:
        uint32 opcode;
        //MSTime time;
    public:

        explicit RespHandler(uint32 _opcode) : opcode(_opcode) {}

        bool CanHandle(uint32 _opcode) const { return opcode == _opcode;}
        //virtual void OnTimeout() {}
        virtual void OnReply(ClientImpl * client, WorldPacket& data) = 0;
    };

    class ClientImpl
    {
        #pragma region Impl
    private:
        void * m_socket;
        UnitMovementImpl * m_controlled;
        MSTime m_time_diff;             // difference between client and server time: diff = client_ticks - server_ticks
        MSTime m_last_sync_time;
        UInt32Counter request_counter;
        //int32 time_skipped;
        typedef std::list<RespHandler*> RespHdlContainer;
        RespHdlContainer m_resp_handlers;

        static MSTime ServerTime() { return MSTime(getMSTime());}
        MSTime ServerToClientTime(const MSTime& server_time) const { return server_time + m_time_diff;}
        MSTime ClientTime() const {return ServerToClientTime(ServerTime());}
        MSTime ClientToServerTime(const MSTime& client_time) const { return client_time - m_time_diff;}

    public:

        UnitMovementImpl* controlled() const { return m_controlled;}
        void SetClientTime(const MSTime& client_time) { m_time_diff = client_time - ServerTime();}

        void QueueState(ClientMoveState& client_state)
        {
            client_state.ms_time = ClientToServerTime(client_state.ms_time);
            m_controlled->_QueueState(client_state);
        }

        uint32 AddRespHandler(RespHandler* req)
        {
            m_resp_handlers.push_back(req);
            return request_counter.NewId();
        }

        inline void BroadcastMessage(MovementMessage& msg) const { MaNGOS_API::BroadcastMessage(&m_controlled->Owner, msg);}
        inline void BroadcastMessage(WorldPacket& data) const { MaNGOS_API::BroadcastMessage(&m_controlled->Owner, data);}
        inline void SendPacket(const WorldPacket& data) const { MaNGOS_API::SendPacket(m_socket, data);}

        void CleanReferences();
        void Dereference(const UnitMovementImpl * m);
        void _OnUpdate();
        #pragma endregion
    public:

        /** Client's lifetime bounded to WorldSession lifetime */
        ClientImpl(HANDLE socket);

        ~ClientImpl()
        {
            CleanReferences();
        }

        std::string ToString() const;

        void LostControl();
        void SetControl(UnitMovementImpl * mov);

        void HandleResponse(WorldPacket& data);

        /** Handles messages from another clients */
        void SendMoveMessage(MovementMessage& msg) const;
        /** Handles messages from that client */
        void HandleOutcomingMessage(WorldPacket& recv_data);

        void HandleMoveTimeSkipped(WorldPacket & recv_data);
    };
}