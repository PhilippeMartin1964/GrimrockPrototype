#include "Runtime/GrimrockGameMode.h"

#include "Runtime/GrimrockPartyPawn.h"

AGrimrockGameMode::AGrimrockGameMode ()
{
    DefaultPawnClass = AGrimrockPartyPawn::StaticClass ();
}
