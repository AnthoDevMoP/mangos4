/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "packet_builder.h"
#include "../Unit.h"
#include "../GameObject.h"
#include "../TransportSystem.h"

namespace Movement
{
    UnitMoveType SelectSpeedType(uint32 moveFlags)
    {
        if (moveFlags & MOVEFLAG_FLYING)
        {
            if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.flight >= speed_obj.flight_back*/)
                return MOVE_FLIGHT_BACK;
            else
                return MOVE_FLIGHT;
        }
        else if (moveFlags & MOVEFLAG_SWIMMING)
        {
            if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.swim >= speed_obj.swim_back*/)
                return MOVE_SWIM_BACK;
            else
                return MOVE_SWIM;
        }
        else if (moveFlags & MOVEFLAG_WALK_MODE)
        {
            // if ( speed_obj.run > speed_obj.walk )
            return MOVE_WALK;
        }
        else if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.run >= speed_obj.run_back*/)
            return MOVE_RUN_BACK;

        return MOVE_RUN;
    }

    int32 MoveSplineInit<Unit*>::Launch()
    {
        MoveSpline& move_spline = *unit.movespline;
        TransportInfo* transportInfo = unit.GetTransportInfo();

        Position real_position = transportInfo ?
                                    transportInfo->GetLocalPosition() :
                                    unit.GetPosition();

        // there is a big chane that current position is unknown if current state is not finalized, need compute it
        // this also allows calculate spline position and update map position in much greater intervals
        if (!move_spline.Finalized() && !transportInfo)
            real_position = move_spline.ComputePosition();

        if (args.path.empty())
        {
            // form final position only for rotate
            if (!args.flags.isFacing())
                return 0;
            MoveTo(real_position);
        }
        else
        {
            // check path equivalence
            if (!args.flags.isFacing() && args.path.size() == 2 && args.path[0] == args.path[1])
                return 0;
        }

        // corrent first vertex
        args.path[0] = real_position;
        args.initialOrientation = real_position.orientation;

        uint32 moveFlags = unit.m_movementInfo.GetMovementFlags();
        moveFlags |= (MOVEFLAG_SPLINE_ENABLED | MOVEFLAG_FORWARD);

        if (moveFlags & MOVEFLAG_ROOT)
            moveFlags &= ~MOVEFLAG_MASK_MOVING;

        if (fabs(args.velocity) < M_NULL_F)
        {
            // Add or remove walk mode flag for select speed depending from call SetWalk()
            // in spline initialization. Not need change the real unit move flags.
            uint32 moveFlagsForSpeed = moveFlags;
            if (args.flags.walkmode)
                moveFlagsForSpeed |= MOVEFLAG_WALK_MODE;
            else
                moveFlagsForSpeed &= ~MOVEFLAG_WALK_MODE;

            args.velocity = unit.GetSpeed(SelectSpeedType(moveFlagsForSpeed));
        }

        if (!args.Validate(&unit))
            return 0;

        unit.m_movementInfo.SetMovementFlags((MovementFlags)moveFlags);
        move_spline.Initialize(args);

        WorldPacket data(SMSG_MONSTER_MOVE, 64);
        data << unit.GetPackGUID();

        if (transportInfo)
        {
            data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
            data << transportInfo->GetTransportGuid().WriteAsPacked();
            data << int8(transportInfo->GetTransportSeat());
        }

        PacketBuilder::WriteMonsterMove(move_spline, data);
        unit.SendMessageToSet(&data, true);

        return move_spline.Duration();
    }

    MoveSplineInit<Unit*>::MoveSplineInit(Unit& m) : unit(m)
    {
        // mix existing state into new
        args.flags.walkmode = unit.m_movementInfo.HasMovementFlag(MOVEFLAG_WALK_MODE);
        args.flags.flying = unit.m_movementInfo.HasMovementFlag((MovementFlags)(MOVEFLAG_FLYING | MOVEFLAG_LEVITATING));
    }

    void MoveSplineInit<Unit*>::SetFacing(const Unit* target)
    {
        args.flags.EnableFacingTarget();
        args.facing.target = target->GetObjectGuid().GetRawValue();
    }

    void MoveSplineInit<Unit*>::SetFacing(float angle)
    {
        args.facing.angle = G3D::wrap(angle, 0.f, (float)G3D::twoPi());
        args.flags.EnableFacingAngle();
    }

    MoveSplineInit<GameObject*>::MoveSplineInit(GameObject& go) : gameobject(go)
    {
    }

    int32 MoveSplineInit<GameObject*>::Launch()
    {
        MoveSpline& move_spline = *gameobject.movespline;
        TransportInfo* transportInfo = gameobject.GetTransportInfo();

        Position real_position = transportInfo ?
                                    transportInfo->GetLocalPosition() :
                                    gameobject.GetPosition();

        // there is a big chane that current position is unknown if current state is not finalized, need compute it
        // this also allows calculate spline position and update map position in much greater intervals
        if (!move_spline.Finalized())
            real_position = move_spline.ComputePosition();

        if (args.path.empty())
        {
            // form final position only for rotate
            if (!args.flags.isFacing())
                return 0;
            MoveTo(real_position);
        }
        else
        {
            // check path equivalence
            if (!args.flags.isFacing() && args.path.size() == 2 && args.path[0] == args.path[1])
                return 0;
        }

        // corrent first vertex
        args.path[0] = real_position;
        args.initialOrientation = real_position.orientation;

        if (fabs(args.velocity) < M_NULL_F)
            return 0;
//            args.velocity = gameobject.GetSpeed();

        move_spline.Initialize(args);
        return move_spline.Duration();
    }
}
