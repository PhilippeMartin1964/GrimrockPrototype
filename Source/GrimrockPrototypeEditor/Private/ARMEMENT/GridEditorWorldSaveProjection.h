#pragma once

#include "Runtime/GridDungeonRuntimeState.h"
#include "UObject/UnrealType.h"

// Private, deterministic evidence across the two reflected schemas.
// The removed fields are never used as source values. Length prefixes
// protect text values with separators; sort rows, not container iteration.
namespace GridEditorWorldSaveProjection
{
    inline TArray<FString> Build(const FGridDungeonRuntimeState& Dungeon)
    {
        TArray<FString> Rows;
        auto Add = [&Rows](FName LevelId, const TCHAR* Container, FGuid Owner,
            FGuid Key, const FGridRuntimeItemState& Item)
        {
            FString Line;
            auto Append = [&Line](const FString& Value)
            {
                Line += FString::Printf(TEXT("%d:%s"), Value.Len(), *Value);
            };
            Append(LevelId.ToString());
            Append(Container);
            Append(Owner.ToString());
            Append(Key.ToString());
            for (TFieldIterator<FProperty> Field(FGridRuntimeItemState::StaticStruct()); Field; ++Field)
            {
                if (!Field->HasAnyPropertyFlags(CPF_SaveGame) ||
                    Field->GetFName() == FName(TEXT("ProjectileDefinitionAsset"))) continue;
                FString Value;
                Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(&Item),
                    nullptr, nullptr, PPF_None);
                Append(Field->GetName());
                Append(Value);
            }
            Rows.Add(MoveTemp(Line));
        };
        for (const auto& Level : Dungeon.LevelStates)
        {
            for (const auto& Pair : Level.Value.Items)
                Add(Level.Key, TEXT("World"), FGuid(), Pair.Key, Pair.Value);
            for (const auto& Pair : Level.Value.PendingInboundItems)
                Add(Level.Key, TEXT("Pending"), FGuid(), Pair.Key, Pair.Value.ItemState);
            for (const auto& Pair : Level.Value.Receptacles)
                for (const auto& Item : Pair.Value.ContainedItems)
                    Add(Level.Key, TEXT("Contained"), Pair.Key, Item.ObjectId, Item);
        }
        Rows.Sort();
        return Rows;
    }
}
