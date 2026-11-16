#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** X.16.8.4.1: During canonical key cutover, only these two real
 * definition packages may persist BOTH an old pointer and its exact
 * equivalent key ID. Every unknown, contradictory, duplicate or extra
 * AcceptedKeyIds remains a TD07 current-schema finding.
 */
namespace GridTD073ApprovedCanonicalLockAcceptance
{
    inline bool IsExactApprovedPair(const FString& AssetPath, const FGridLockBehaviorParams& Lock)
    {
        if (Lock.AcceptedKeyIds.Num() != 1 || Lock.AcceptedKeyItems.Num() != 1)
            return false;
        const UGridItemDefinitionAsset* Source = Lock.AcceptedKeyItems[0].Get();
        if (!IsValid(Source)) return false;
        if (AssetPath == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"))
            return Lock.AcceptedKeyIds[0] == FName(TEXT("Key_Copper")) &&
                Source->ItemDefinitionId == FName(TEXT("Key_Copper"));
        if (AssetPath == TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall"))
            return Lock.AcceptedKeyIds[0] == FName(TEXT("Key_Iron")) &&
                Source->ItemDefinitionId == FName(TEXT("Key_Iron"));
        return false;
    }

    inline bool IsHistoricalOrApproved(const FString& AssetPath, const FGridLockBehaviorParams& Lock)
    {
        return Lock.AcceptedKeyIds.IsEmpty() || IsExactApprovedPair(AssetPath, Lock);
    }
}
