#pragma once

#include "GridEditorAlchemySaveCopy.h"
#include "GridEditorRetirementAudit.h"
#include "Runtime/GridReadableContentAsset.h"

// One-time Editor-only repair of the two paths established by 5A.
// The original archive is never mutated, and no redirect/fallback is
// installed in the runtime. Readable content is a separate authority,
// not an obsolete item definition.
namespace GridEditorReadableSaveRepair
{
    namespace Copy = GridEditorAlchemySaveCopy;

    struct FMapping
    {
        FString OldPath;
        FString NewPath;
        UGridReadableContentAsset* Asset = nullptr;
    };

    inline bool LoadMappings(TArray<FMapping>& Out, FString& Error)
    {
        Out.Reset();
        TSet<FName> Ids;
        for (const FString& Old : GridEditorRetirementAudit::MovedReadablePaths())
        {
            FString New = Old.Replace(TEXT("/DataAssets/Readables/"), TEXT("/DataAssets/Tests/Readables/"));
            auto* Asset = LoadObject<UGridReadableContentAsset>(nullptr, *New);
            if (New == Old || !IsValid(Asset) || Asset->GetPathName() != New ||
                Asset->ReadableContentId.IsNone() || Ids.Contains(Asset->ReadableContentId))
            {
                Error = TEXT("Moved readable identity is invalid or ambiguous: ") + New;
                return false;
            }
            Ids.Add(Asset->ReadableContentId);
            Out.Add({Old, New, Asset});
        }
        return Out.Num() == 2;
    }

    // Count the exact known path candidates; no arbitrary text substitution.
    // The count must agree with the independently decoded null-pointer IDs.
    inline int32 CountPath(const TArray<uint8>& Bytes, const FString& Path)
    {
        int32 Count = 0;
        for (int32 Width : {1, 2})
        {
            const int32 Length = Path.Len() * Width;
            if (Length <= 0) continue;
            for (int32 Start = 0; Start <= Bytes.Num() - Length; ++Start)
            {
                bool bMatch = true;
                for (int32 I = 0; I < Path.Len(); ++I)
                {
                    const int32 At = Start + I * Width;
                    if (TCHAR(Bytes[At]) != Path[I] || (Width == 2 && Bytes[At+1] != 0))
                    {
                        bMatch = false;
                        break;
                    }
                }
                if (bMatch)
                {
                    ++Count;
                    Start += Length - 1;
                }
            }
        }
        return Count;
    }

    template<typename Visitor>
    void Visit(UGrimrockPartySaveGame& Save, Visitor&& Read)
    {
        auto& Party = Save.PartyInventoryState;
        for (auto* Group : {&Party.ActiveCharacters, &Party.CharacterPool})
            for (auto& Character : *Group)
                for (auto& Slot : Character.InventorySlots)
                    Read(Slot.Item);
        for (auto& Equipment : Party.ActiveEquipment)
            for (TFieldIterator<FStructProperty> Field(FGridCharacterEquipmentState::StaticStruct()); Field; ++Field)
                if (Field->Struct == FGridItemInstance::StaticStruct())
                    Read(*Field->ContainerPtrToValuePtr<FGridItemInstance>(&Equipment));
        Read(Party.CursorItem);
        for (auto& Level : Save.DungeonRuntimeState.LevelStates)
        {
            for (auto& Entry : Level.Value.Items) Read(Entry.Value);
            for (auto& Entry : Level.Value.Receptacles)
                for (auto& Contained : Entry.Value.ContainedItems) Read(Contained);
            for (auto& Entry : Level.Value.PendingInboundItems) Read(Entry.Value.ItemState);
        }
    }

    struct FPatch
    {
        TObjectPtr<UGridReadableContentAsset>* Pointer = nullptr;
        UGridReadableContentAsset* Target = nullptr;
    };

    // All candidates must match before a single in-memory field is changed.
    // A known path without a matching ID (or vice versa in a changing file)
    // is a blocker, not an invitation to choose an arbitrary readable.
    inline bool Plan(UGrimrockPartySaveGame& Save, const TArray<FMapping>& Mappings,
        const TArray<uint8>& Evidence, TArray<FPatch>& Out, FString& Error)
    {
        Out.Reset();
        Error.Reset();
        TArray<int32> Expected, Observed;
        for (const auto& Map : Mappings) Expected.Add(CountPath(Evidence, Map.OldPath));
        Observed.Init(0, Mappings.Num());
        int32 Total = 0;
        for (int32 Count : Expected) Total += Count;
        if (Total == 0) return true;
        bool bConflict = false;
        TArray<FPatch> Candidate;
        Visit(Save, [&](auto& Item)
        {
            if (Item.ReadableContentId.IsNone())
            {
                if (IsValid(Item.ReadableContentAsset.Get())) bConflict = true;
                return;
            }
            const int32 Index = Mappings.IndexOfByPredicate([&](const FMapping& Map)
                { return Map.Asset->ReadableContentId == Item.ReadableContentId; });
            if (IsValid(Item.ReadableContentAsset.Get()))
            {
                if (Item.ReadableContentAsset->ReadableContentId != Item.ReadableContentId ||
                    (Index != INDEX_NONE && Item.ReadableContentAsset.Get() != Mappings[Index].Asset))
                    bConflict = true;
                return;
            }
            if (Index == INDEX_NONE)
            {
                // Refuse unknown dangling identities in a file being rewritten.
                bConflict = true;
                return;
            }
            ++Observed[Index];
            Candidate.Add({&Item.ReadableContentAsset, Mappings[Index].Asset});
        });
        if (bConflict || Observed != Expected || Candidate.Num() != Total)
        {
            Error = TEXT("Old readable paths and decoded identities disagree; no candidate returned");
            return false;
        }
        Out = MoveTemp(Candidate);
        return true;
    }

    inline void Apply(const TArray<FPatch>& Patches)
    {
        for (const auto& Patch : Patches) *Patch.Pointer = Patch.Target;
    }

    inline bool Convert(const TArray<uint8>& Input, TArray<uint8>& Output, int32& Changed, FString& Error)
    {
        Changed = 0;
        Error.Reset();
        if (&Input == &Output)
        {
            Error = TEXT("Input/output arrays must not alias");
            return false;
        }
        Output.Reset();
        TStrongObjectPtr<UGrimrockPartySaveGame> Working(Copy::LoadExact(Input, Error));
        if (!Working.IsValid()) return false;
        TArray<FMapping> Mappings;
        if (!LoadMappings(Mappings, Error)) return false;
        TArray<FPatch> Patches;
        if (!Plan(*Working.Get(), Mappings, Input, Patches, Error)) return false;
        if (Patches.IsEmpty())
        {
            Output = Input;
            return true;
        }
        TStrongObjectPtr<UGrimrockPartySaveGame> Expected(Copy::LoadExact(Input, Error));
        TArray<FPatch> ExpectedPatches;
        if (!Expected.IsValid() || !Plan(*Expected.Get(), Mappings, Input, ExpectedPatches, Error) ||
            ExpectedPatches.Num() != Patches.Num() ||
            !Copy::SameSaveProperties(*Working.Get(), *Expected.Get(), Error)) return false;

        Apply(Patches);
        Apply(ExpectedPatches);
        if (!Copy::SameSaveProperties(*Working.Get(), *Expected.Get(), Error)) return false;
        TArray<uint8> Bytes;
        if (!Copy::EncodeChecked(*Working.Get(), Bytes, Error)) return false;
        for (const auto& Map : Mappings)
            if (CountPath(Bytes, Map.OldPath) != 0)
            {
                Error = TEXT("Old readable path survived reserialization");
                return false;
            }
        TStrongObjectPtr<UGrimrockPartySaveGame> Reloaded(Copy::LoadExact(Bytes, Error));
        if (!Reloaded.IsValid() || !Copy::SameSaveProperties(*Expected.Get(), *Reloaded.Get(), Error))
            return false;
        Changed = Patches.Num();
        Output = MoveTemp(Bytes);
        return true;
    }
}
