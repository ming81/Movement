
/**
  file:         mov_constants.h
  author:       SilverIce
  email:        slifeleaf@gmail.com
  created:      16:2:2011
*/

#pragma once

namespace Movement
{
    enum MonsterMoveType
    {
        MonsterMoveNormal       = 0,
        MonsterMoveStop         = 1,
        MonsterMoveFacingSpot   = 2,
        MonsterMoveFacingTarget = 3,
        MonsterMoveFacingAngle  = 4
    };

    // used with 0x00200000 flag (SPLINEFLAG_ANIMATION) in monster move packet
    enum AnimType
    {
        UNK0 = 0, // 460 = ToGround, index of AnimationData.dbc
        UNK1 = 1, // 461 = FlyToFly?
        UNK2 = 2, // 458 = ToFly
        UNK3 = 3, // 463 = FlyToGround
    };

    enum MoveMode
    {
        MoveModeWalk,
        MoveModeRoot,
        MoveModeSwim,
        MoveModeWaterwalk,
        MoveModeSlowfall,
        MoveModeHover,
        MoveModeFly,
        MoveModeLevitation,
        MoveModeMaxCount
    };

    enum SpeedType
    {
        SpeedNotStandart    =-1,

        SpeedWalk           = 0,
        SpeedRun            = 1,
        SpeedSwimBack       = 2,
        SpeedSwim           = 3,
        SpeedRunBack        = 4,
        SpeedFlight         = 5,
        SpeedFlightBack     = 6,
        SpeedTurn           = 7,
        SpeedPitch          = 8,

        SpeedMaxCount       = 9,
    };

    enum MovControlType
    {
        MovControlClient,
        MovControlServer,
        MovControlCount,
    };

    extern const uint32 Mode2Flag_table[];
    extern const uint16 S_Speed2Opc_table[];
    extern const uint16 S_Mode2Opc_table[MoveModeMaxCount][2];
    extern const uint16 SetSpeed2Opc_table[][2];
    extern const float  BaseSpeed[SpeedMaxCount];

    float computeFallTime(float path_length, bool isSafeFall);
    float computeFallElevation(float t_passed, bool isSafeFall, float start_velocy);
}
