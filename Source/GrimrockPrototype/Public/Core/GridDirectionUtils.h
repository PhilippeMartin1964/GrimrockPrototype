#pragma once

#include "CoreMinimal.h"
#include "GridTypes.h"

namespace GridDirectionUtils
{
    /*
    ============================================================
    CONVENTION OFFICIELLE DU PROJET
    ============================================================

    Grille :
        North = +Y
        East  = +X
        South = -Y
        West  = -X

    Unreal Yaw :
        East  =   0°
        North =  90°
        South = -90°
        West  = 180°

    IMPORTANT :
        - Tourner à gauche = RotateLeft = sens anti-horaire visuel
        - Tourner à droite = RotateRight = sens horaire visuel
        - MAIS selon cette convention :
            RotateLeft  = math "inverse"
            RotateRight = math "inverse"

    => Ne jamais modifier sans tout recalibrer
    ============================================================
    */

    FORCEINLINE EGridEdge RotateLeft(EGridEdge Dir)
    {
        switch (Dir)
        {
        case EGridEdge::North: return EGridEdge::East;
        case EGridEdge::East:  return EGridEdge::South;
        case EGridEdge::South: return EGridEdge::West;
        case EGridEdge::West:  return EGridEdge::North;
        default:               return EGridEdge::North;
        }
    }

    FORCEINLINE EGridEdge RotateRight(EGridEdge Dir)
    {
        switch (Dir)
        {
        case EGridEdge::North: return EGridEdge::West;
        case EGridEdge::West:  return EGridEdge::South;
        case EGridEdge::South: return EGridEdge::East;
        case EGridEdge::East:  return EGridEdge::North;
        default:               return EGridEdge::North;
        }
    }

    FORCEINLINE float ToYaw(EGridEdge Dir)
    {
        switch (Dir)
        {
        case EGridEdge::North: return 90.f;
        case EGridEdge::East:  return 0.f;
        case EGridEdge::South: return -90.f;
        case EGridEdge::West:  return 180.f;
        default:               return 0.f;
        }
    }

    FORCEINLINE EGridEdge GetForward(EGridEdge Facing)
    {
        return Facing;
    }

    FORCEINLINE EGridEdge GetBackward(EGridEdge Facing)
    {
        return RotateRight(RotateRight(Facing));
    }

    FORCEINLINE EGridEdge GetLeft(EGridEdge Facing)
    {
        return RotateLeft (Facing);
    }

    FORCEINLINE EGridEdge GetRight(EGridEdge Facing)
    {
        return RotateRight (Facing);
    }
}