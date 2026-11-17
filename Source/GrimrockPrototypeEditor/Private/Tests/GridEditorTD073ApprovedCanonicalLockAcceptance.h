#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** X.52: only these two native lock packages may have the exact
 * approved Copper/Iron ItemId in either of TWO safe serialized states:
 *   a single matching old pointer + ID, or a canonical-only ID.
 * Duplicate, unknown, contradictory or stray pointers fail closed.
 */
namespace GridTD073ApprovedCanonicalLockAcceptance
{
    inline bool IsExactApprovedPair(const FString& AssetPath, const FGridLockBehaviorParams& Lock)
    {
        if (Lock.AcceptedKeyIds.Num() != 1 || Lock.AcceptedKeyItems.Num() > 1)
            return false;
        FName Required = NAME_None;
        if (AssetPath == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"))
            Required = TEXT("Key_Copper");
        else if (AssetPath == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall"))
            Required = TEXT("Key_Iron");
        if (Required.IsNone() || Lock.AcceptedKeyIds[0] != Required)
            return false;
        if (Lock.AcceptedKeyItems.IsEmpty())
            return true; // Verified post-conversion canonical-only form.
        const UGridItemDefinitionAsset* Source = Lock.AcceptedKeyItems[0].Get();
        return IsValid(Source) && Source->ItemDefinitionId == Required;
    }

    inline bool IsHistoricalOrApproved(const FString& AssetPath, const FGridLockBehaviorParams& Lock)
    {
        return Lock.AcceptedKeyIds.IsEmpty() || IsExactApprovedPair(AssetPath, Lock);
    }
}
