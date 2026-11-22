#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
// Frozen path inventory for historical X.37 backup receipts only.
// It must not include or load the retired projection/ItemDefinitionAsset code.

/** Pure validation of the X.37 allowlist receipt. No native files are touched. */
namespace ARMCLEAN064X371
{
inline constexpr const TCHAR* LegacySuffixes[] =
{
    TEXT("ArcaneOrb_01"), TEXT("ArcaneWand_01"),
    TEXT("AssassinDagger_01"), TEXT("Dagger_01"), TEXT("Dart_01"),
    TEXT("ElementalStaff_01"), TEXT("FireWand_01"), TEXT("GreatAxe_01"),
    TEXT("HolyAegis_01"), TEXT("HolyScepter_01"), TEXT("Javelin_01"),
    TEXT("LongSword_01"), TEXT("Mace_01"), TEXT("MainGauche_01"),
    TEXT("RecurveBow_01"), TEXT("RoundShield_01"), TEXT("ShortBow_01"),
    TEXT("ShortSpear_01"), TEXT("Stiletto_01"), TEXT("StraightSword_01"),
    TEXT("ThrowingKnife_01"), TEXT("WarPick_01")
};
inline bool IsLowercaseHex(const FString& Token, int32 Length)
{
    if (Token.Len() != Length) { return false; }
    for (TCHAR Character : Token)
    {
        if (!((Character >= TEXT('0') && Character <= TEXT('9')) ||
              (Character >= TEXT('a') && Character <= TEXT('f'))))
        {
            return false;
        }
    }
    return true;
}

inline TSet<FString> RequiredPaths()
{
    TSet<FString> Paths;
    for (const TCHAR* Suffix : LegacySuffixes)
    {
        Paths.Add(FString(TEXT("Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_"))
            + Suffix + TEXT(".uasset"));
    }
    Paths.Add(TEXT("Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponVisualCatalogue.uasset"));
    Paths.Add(TEXT("Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponCatalogue.uasset"));
    return Paths;
}

/** The commandlet checks all actual backup bytes after this exact shape test.
 * A boolean 'verified' and 24 arbitrary rows are never enough to authorize Apply. */
inline bool ValidateReceipt(const TSharedPtr<FJsonObject>& Manifest,
    const FString& ExpectedHead, TArray<FString>& OutPaths)
{
    OutPaths.Reset();
    if (!Manifest.IsValid() || !IsLowercaseHex(ExpectedHead, 40))
    {
        return false;
    }

    FString Schema, ActualHead;
    bool bVerified = false;
    int32 SourceCount = 0;
    int32 TargetCount = 0;
    const TArray<TSharedPtr<FJsonValue>>* Files = nullptr;
    if (!Manifest->TryGetStringField(TEXT("schema"), Schema) ||
        Schema != TEXT("ARM-CLEAN06.4X.37/native-backup/v1") ||
        !Manifest->TryGetStringField(TEXT("head"), ActualHead) ||
        ActualHead != ExpectedHead ||
        !Manifest->TryGetBoolField(TEXT("verified"), bVerified) || !bVerified ||
        !Manifest->TryGetNumberField(TEXT("source_count"), SourceCount) ||
        SourceCount != 22 ||
        !Manifest->TryGetNumberField(TEXT("output_count"), TargetCount) ||
        TargetCount != 2 ||
        !Manifest->TryGetArrayField(TEXT("files"), Files) ||
        !Files || Files->Num() != 24)
    {
        return false;
    }

    const TSet<FString> Required = RequiredPaths();
    if (Required.Num() != 24) { return false; }
    TSet<FString> Seen;
    for (const TSharedPtr<FJsonValue>& Value : *Files)
    {
        const TSharedPtr<FJsonObject> Row = Value.IsValid()
            ? Value->AsObject() : nullptr;
        FString Relative, Checksum;
        if (!Row.IsValid() ||
            !Row->TryGetStringField(TEXT("path"), Relative) ||
            !Row->TryGetStringField(TEXT("sha256"), Checksum) ||
            !Required.Contains(Relative) || Seen.Contains(Relative) ||
            !IsLowercaseHex(Checksum, 64))
        {
            OutPaths.Reset();
            return false;
        }
        Seen.Add(Relative);
        OutPaths.Add(Relative);
    }
    if (Seen.Num() != 24)
    {
        OutPaths.Reset();
        return false;
    }
    return true;
}
} // namespace ARMCLEAN064X371
