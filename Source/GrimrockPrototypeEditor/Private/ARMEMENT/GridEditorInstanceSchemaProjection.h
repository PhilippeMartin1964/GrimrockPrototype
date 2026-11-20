#pragma once

#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"
#include "Misc/SecureHash.h"

// Private cross-schema evidence; never published. Preserve array order,
// occupancy, all durable root properties and all non-cache instance fields.
namespace GridEditorInstanceSchemaProjection
{
    inline void Append(FString& Out, const FString& Value)
    {
        Out += FString::Printf(TEXT("%d:%s"), Value.Len(), *Value);
    }

    inline bool IsInstanceContainer(const UScriptStruct* Type)
    {
        return Type == FGridPartyInventoryState::StaticStruct() ||
            Type == FGridCharacterInventoryState::StaticStruct() ||
            Type == FGridCharacterEquipmentState::StaticStruct() ||
            Type == FGridInventorySlot::StaticStruct() ||
            Type == FGridItemInstance::StaticStruct();
    }

    inline void AppendContainer(const UScriptStruct* Type, const void* Value, FString& Out);

    inline void AppendValue(const FProperty* Field, const void* Value, FString& Out)
    {
        if (const auto* Struct = CastField<FStructProperty>(Field))
        {
            if (IsInstanceContainer(Struct->Struct))
            {
                AppendContainer(Struct->Struct, Value, Out);
                return;
            }
        }
        if (const auto* Array = CastField<FArrayProperty>(Field))
        {
            if (const auto* Inner = CastField<FStructProperty>(Array->Inner))
            {
                if (IsInstanceContainer(Inner->Struct))
                {
                    FScriptArrayHelper Helper(Array, Value);
                    Append(Out, FString::FromInt(Helper.Num()));
                    for (int32 Index = 0; Index < Helper.Num(); ++Index)
                        AppendContainer(Inner->Struct, Helper.GetRawPtr(Index), Out);
                    return;
                }
            }
        }
        // Opaque fields use the same reflected export in both schemas.
        // They include non-item names, hotbar, quests, world, and resources.
        FString Text;
        Field->ExportTextItem_Direct(Text, Value, nullptr, nullptr, PPF_None);
        Append(Out, Text);
    }

    inline void AppendContainer(const UScriptStruct* Type, const void* Value, FString& Out)
    {
        Append(Out, Type->GetName());
        for (TFieldIterator<FProperty> Field(Type); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            if (Type == FGridItemInstance::StaticStruct() &&
                (Field->GetFName() == FName(TEXT("DisplayName")) || Field->GetFName() == FName(TEXT("Weight")))) continue;
            Append(Out, Field->GetName());
            Append(Out, FString::FromInt(Field->ArrayDim));
            for (int32 Index = 0; Index < Field->ArrayDim; ++Index)
                AppendValue(*Field, Field->ContainerPtrToValuePtr<void>(Value, Index), Out);
        }
    }

    inline TArray<FString> Build(const UGrimrockPartySaveGame& Save)
    {
        TArray<FString> Fields;
        for (TFieldIterator<FProperty> Field(UGrimrockPartySaveGame::StaticClass(),
            EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (!Field->HasAnyPropertyFlags(CPF_SaveGame) || Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            FString Row;
            Append(Row, Field->GetName());
            for (int32 Index = 0; Index < Field->ArrayDim; ++Index)
                AppendValue(*Field, Field->ContainerPtrToValuePtr<void>(&Save, Index), Row);
            // Keep the report bounded even for large explored dungeons. Hash
            // the complete length-framed UTF-8 property projection, not a prefix.
            FTCHARToUTF8 Utf8(*Row);
            FSHAHash Digest;
            FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Digest.Hash);
            Fields.Add(Field->GetName() + TEXT(":") + Digest.ToString());
        }
        return Fields;
    }
}
