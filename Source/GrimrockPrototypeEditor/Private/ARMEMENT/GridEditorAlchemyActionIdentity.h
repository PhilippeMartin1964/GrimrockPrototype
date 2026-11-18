#pragma once

#include "Runtime/GridInventoryTypes.h"

/** Explicit editor-only migration of the old shared defensive-elixir ActionId.
 * Never a runtime alias and never an inferred choice of element.
 * The caller of ConvertPartyCopy owns the SaveGame version/files transaction.
 */
namespace GridEditorAlchemyActionIdentity
{
    inline FName SharedLegacyAction()
    {
        return TEXT("Action_Alchemist_DefensiveElixir");
    }

    inline FName Item(int32 Index)
    {
        static const FName Ids[] = {
            TEXT("Item_DefensiveElixir_Fire"),
            TEXT("Item_DefensiveElixir_Ice"),
            TEXT("Item_DefensiveElixir_Lightning"),
            TEXT("Item_DefensiveElixir_Poison")
        };
        check(Index >= 0 && Index < 4);
        return Ids[Index];
    }

    inline FName ActionForItem(FName ItemId)
    {
        static const FName Actions[] = {
            TEXT("Action_Alchemist_DefensiveElixir_Fire"),
            TEXT("Action_Alchemist_DefensiveElixir_Ice"),
            TEXT("Action_Alchemist_DefensiveElixir_Lightning"),
            TEXT("Action_Alchemist_DefensiveElixir_Poison")
        };
        for (int32 I = 0; I < 4; ++I)
            if (ItemId == Item(I)) return Actions[I];
        return NAME_None;
    }

    inline bool IsNewElixirAction(FName ActionId)
    {
        for (int32 I = 0; I < 4; ++I)
            if (ActionId == ActionForItem(Item(I))) return true;
        return false;
    }

    /** Copy first, commit only a completely valid transformation. Preserve
     * every other field, character, inventory item, slot and preferred GUID.
     * Active and reserve pools are separate durable arrays in v27.
     */
    inline bool ConvertPartyCopy(FGridPartyInventoryState& Party, int32& Changed, FString& Error)
    {
        Changed = 0;
        Error.Reset();
        FGridPartyInventoryState Candidate = Party;
        int32 Count = 0;
        auto Visit = [&](TArray<FGridCharacterInventoryState>& Characters)
        {
            for (auto& Character : Characters)
            {
                for (int32 Index = 0; Index < Character.CombatHotbarSlots.Num(); ++Index)
                {
                    auto& Binding = Character.CombatHotbarSlots[Index];
                    const FName NewId = ActionForItem(Binding.SourceDefinitionId);
                    const bool bOldAction = Binding.ActionId == SharedLegacyAction();
                    const bool bNewAction = IsNewElixirAction(Binding.ActionId);
                    if (!bOldAction && !bNewAction && NewId.IsNone()) continue;
                    if (NewId.IsNone() || Binding.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
                        Binding.SlotIndex != Index ||
                        Index == FGridCombatHotbarBinding::PrimaryAttackSlotIndex ||
                        !Binding.IsValid() || (!bOldAction && Binding.ActionId != NewId))
                    {
                        Error = FString::Printf(TEXT("Ambiguous or invalid elixir binding at slot %d, source %s, action %s"),
                            Index, *Binding.SourceDefinitionId.ToString(), *Binding.ActionId.ToString());
                        return false;
                    }
                    if (bOldAction)
                    {
                        Binding.ActionId = NewId;
                        ++Count;
                    }
                }
            }
            return true;
        };
        if (!Visit(Candidate.ActiveCharacters) || !Visit(Candidate.CharacterPool)) return false;
        if (Count != 0) Party = MoveTemp(Candidate);
        Changed = Count;
        return true;
    }
}
