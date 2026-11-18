#pragma once

#include "GridEditorAlchemyActionIdentity.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

/** X.64: explicit Editor-only conversion of a v27 byte snapshot.
 * The source byte array never changes. No live save slots or file I/O.
 */
namespace GridEditorAlchemySaveCopy
{
    namespace Identity = GridEditorAlchemyActionIdentity;

    inline bool CheckOtherActionReferences(const UGrimrockPartySaveGame& Save, FString& Error)
    {
        // These persisted references have no source ItemId. None of the
        // four elixirs creates a surface or a trap. Refuse unexpected old
        // identities instead of making up an element or dropping state.
        for (const auto& Level : Save.DungeonRuntimeState.LevelStates)
        {
            for (const auto& Surface : Level.Value.Surfaces)
                if (Surface.Value.SourceActionId == Identity::SharedLegacyAction())
                {
                    Error = TEXT("Ambiguous legacy elixir surface source in level ") + Level.Key.ToString();
                    return false;
                }
            for (const auto& Trap : Level.Value.CombatTraps)
                if (Trap.Value.SourceActionId == Identity::SharedLegacyAction())
                {
                    Error = TEXT("Ambiguous legacy elixir trap source in level ") + Level.Key.ToString();
                    return false;
                }
        }
        return true;
    }

    inline bool SameSaveProperties(const UGrimrockPartySaveGame& A,
        const UGrimrockPartySaveGame& B, FString& Error)
    {
        // The two objects originated from UE deserialization. Both have
        // the same reconstructed class/status caches. Comparing each
        // declared property also protects quests, world state, timers,
        // position and all nested item/character fields.
        for (TFieldIterator<FProperty> Field(UGrimrockPartySaveGame::StaticClass(),
            EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (!Field->HasAnyPropertyFlags(CPF_SaveGame)) continue;
            if (!Field->Identical_InContainer(&A, &B))
            {
                Error = TEXT("Unexpected save round-trip change: ") + Field->GetName();
                return false;
            }
        }
        return true;
    }

    inline UGrimrockPartySaveGame* LoadExact(const TArray<uint8>& Bytes, FString& Error)
    {
        if (Bytes.IsEmpty())
        {
            Error = TEXT("Empty save snapshot");
            return nullptr;
        }
        auto* Save = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if (!IsValid(Save) || Save->GetClass() != UGrimrockPartySaveGame::StaticClass() ||
            Save->SaveVersion != 27 || !Save->IsCompatible())
        {
            Error = TEXT("Input is not an accepted exact Grimrock v27 save");
            return nullptr;
        }
        FText Validation;
        if (!Save->ValidateCurrentState(Validation))
        {
            Error = Validation.ToString();
            return nullptr;
        }
        return Save;
    }

    inline bool Convert(const TArray<uint8>& Input, TArray<uint8>& Output,
        int32& Changed, FString& Error)
    {
        // An output alias must not erase the original byte snapshot.
        Changed = 0;
        Error.Reset();
        if (&Input == &Output)
        {
            Error = TEXT("Input/output arrays must not alias");
            return false;
        }
        Output.Reset();
        TStrongObjectPtr<UGrimrockPartySaveGame> Working(LoadExact(Input, Error));
        if (!Working.IsValid() || !CheckOtherActionReferences(*Working.Get(), Error)) return false;
        int32 Count = 0;
        if (!Identity::ConvertPartyCopy(Working->PartyInventoryState, Count, Error)) return false;
        if (Count == 0)
        {
            Output = Input; // Preserve unchanged files byte-for-byte.
            return true;
        }
        // Independent second decode serves as the expected post-conversion
        // state, not a mutable object potentially rewritten by Serialize.
        TStrongObjectPtr<UGrimrockPartySaveGame> Expected(LoadExact(Input, Error));
        int32 ExpectedCount = 0;
        if (!Expected.IsValid() ||
            !Identity::ConvertPartyCopy(Expected->PartyInventoryState, ExpectedCount, Error) ||
            ExpectedCount != Count || !SameSaveProperties(*Working.Get(), *Expected.Get(), Error))
            return false;
        TArray<uint8> Bytes;
        if (!UGameplayStatics::SaveGameToMemory(Working.Get(), Bytes) || Bytes.IsEmpty())
        {
            Error = TEXT("UE refused to serialize the converted copy");
            return false;
        }
        TStrongObjectPtr<UGrimrockPartySaveGame> Reloaded(LoadExact(Bytes, Error));
        if (!Reloaded.IsValid() || !SameSaveProperties(*Expected.Get(), *Reloaded.Get(), Error) ||
            !CheckOtherActionReferences(*Reloaded.Get(), Error)) return false;
        int32 Residual = INDEX_NONE;
        if (!Identity::ConvertPartyCopy(Reloaded->PartyInventoryState, Residual, Error) || Residual != 0)
        {
            if (Error.IsEmpty()) Error = TEXT("Reloaded copy still needs conversion");
            return false;
        }
        Output = MoveTemp(Bytes);
        Changed = Count;
        return true;
    }
}
