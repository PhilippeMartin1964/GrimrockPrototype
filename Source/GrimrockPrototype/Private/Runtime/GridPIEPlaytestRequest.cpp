#include "Runtime/GridPIEPlaytestRequest.h"

#include "EngineUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
#if WITH_EDITOR
    struct FGridFreshPIERequest
    {
        bool bActive = false;
        FString RuntimeActorName;
        FSoftObjectPath LevelAssetPath;
        FSoftObjectPath DungeonAssetPath;
        FName DungeonLevelId = NAME_None;
    };

    FGridFreshPIERequest FreshPIERequest;

    bool MatchesRequestIdentity (
        const AGridLevelRuntimeActor* RuntimeActor)
    {
        if (!RuntimeActor || !RuntimeActor->GetWorld () ||
            RuntimeActor->GetWorld ()->WorldType != EWorldType::PIE ||
            !FreshPIERequest.bActive)
        {
            return false;
        }

        return RuntimeActor->GetName () == FreshPIERequest.RuntimeActorName &&
            FSoftObjectPath (RuntimeActor->LevelAsset.Get ()) ==
                FreshPIERequest.LevelAssetPath &&
            FSoftObjectPath (RuntimeActor->DungeonAsset.Get ()) ==
                FreshPIERequest.DungeonAssetPath &&
            RuntimeActor->CurrentDungeonLevelId ==
                FreshPIERequest.DungeonLevelId;
    }
#endif
}

namespace GridPIEPlaytestRequest
{
    void BeginFreshPlaytest (
        const AGridLevelRuntimeActor* PreparedEditorActor)
    {
#if WITH_EDITOR
        Clear (TEXT ("ReplacedByPreBeginPIE"));
        if (!PreparedEditorActor || !PreparedEditorActor->GetWorld () ||
            PreparedEditorActor->GetWorld ()->WorldType != EWorldType::Editor ||
            !PreparedEditorActor->LevelAsset)
        {
            UE_LOG (LogTemp, Error,
                TEXT ("[GridPIEPlaytestRequest] Begin rejected: prepared editor runtime is invalid."));
            return;
        }

        FreshPIERequest.bActive = true;
        FreshPIERequest.RuntimeActorName = PreparedEditorActor->GetName ();
        FreshPIERequest.LevelAssetPath =
            FSoftObjectPath (PreparedEditorActor->LevelAsset.Get ());
        FreshPIERequest.DungeonAssetPath =
            FSoftObjectPath (PreparedEditorActor->DungeonAsset.Get ());
        FreshPIERequest.DungeonLevelId =
            PreparedEditorActor->CurrentDungeonLevelId;

        UE_LOG (LogTemp, Log,
            TEXT ("[GridPIEPlaytestRequest] Begin ActorName=%s LevelAsset=%s DungeonAsset=%s CurrentDungeonLevelId=%s"),
            *FreshPIERequest.RuntimeActorName,
            *FreshPIERequest.LevelAssetPath.ToString (),
            *FreshPIERequest.DungeonAssetPath.ToString (),
            *FreshPIERequest.DungeonLevelId.ToString ());
#endif
    }

    void Clear (const TCHAR* Reason)
    {
#if WITH_EDITOR
        if (FreshPIERequest.bActive)
        {
            UE_LOG (LogTemp, Log,
                TEXT ("[GridPIEPlaytestRequest] Clear Reason=%s ActorName=%s"),
                Reason ? Reason : TEXT ("Unknown"),
                *FreshPIERequest.RuntimeActorName);
        }
        FreshPIERequest = FGridFreshPIERequest ();
#endif
    }

    bool IsActiveForWorld (const UWorld* World)
    {
#if WITH_EDITOR
        return FreshPIERequest.bActive && World &&
            World->WorldType == EWorldType::PIE;
#else
        return false;
#endif
    }

    bool Matches (const AGridLevelRuntimeActor* RuntimeActor)
    {
#if WITH_EDITOR
        return MatchesRequestIdentity (RuntimeActor);
#else
        return false;
#endif
    }

    AGridLevelRuntimeActor* ResolveMatchingRuntimeActor (
        UWorld* World,
        int32* OutRuntimeActorCount,
        int32* OutMatchingActorCount)
    {
        int32 RuntimeActorCount = 0;
        int32 MatchingActorCount = 0;
        AGridLevelRuntimeActor* Match = nullptr;

#if WITH_EDITOR
        if (IsActiveForWorld (World))
        {
            for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
            {
                AGridLevelRuntimeActor* RuntimeActor = *It;
                ++RuntimeActorCount;
                const bool bMatches =
                    MatchesRequestIdentity (RuntimeActor);
                MatchingActorCount += bMatches ? 1 : 0;
                if (bMatches)
                {
                    Match = RuntimeActor;
                }
                UE_LOG (LogTemp, Log,
                    TEXT ("[GridPIEPlaytestRequest] Resolve ActorName=%s ActorPath=%s LevelAsset=%s DungeonAsset=%s CurrentDungeonLevelId=%s Matches=%s"),
                    *RuntimeActor->GetName (),
                    *RuntimeActor->GetPathName (),
                    RuntimeActor->LevelAsset
                        ? *RuntimeActor->LevelAsset->GetPathName ()
                        : TEXT ("None"),
                    RuntimeActor->DungeonAsset
                        ? *RuntimeActor->DungeonAsset->GetPathName ()
                        : TEXT ("None"),
                    *RuntimeActor->CurrentDungeonLevelId.ToString (),
                    bMatches ? TEXT ("true") : TEXT ("false"));
            }

            if (MatchingActorCount != 1)
            {
                UE_LOG (LogTemp, Error,
                    TEXT ("[GridPIEPlaytestRequest] Resolve failed RuntimeActorCount=%d MatchingActorCount=%d"),
                    RuntimeActorCount,
                    MatchingActorCount);
                Match = nullptr;
            }
        }
#endif

        if (OutRuntimeActorCount)
        {
            *OutRuntimeActorCount = RuntimeActorCount;
        }
        if (OutMatchingActorCount)
        {
            *OutMatchingActorCount = MatchingActorCount;
        }
        return Match;
    }
}
