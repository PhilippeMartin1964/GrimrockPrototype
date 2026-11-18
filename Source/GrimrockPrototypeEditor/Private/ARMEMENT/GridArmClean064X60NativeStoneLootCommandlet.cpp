#include "ARMEMENT/GridArmClean064X60NativeStoneLootCommandlet.h"

#include "GridEditorNativeStoneLootPlan.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace ARMCLEAN064X60
{
    using FPlan = FGridEditorNativeStoneLootPlan;
    const TCHAR* RootPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/"
                                 "CanonicalStage/DA_ItemSystem.DA_ItemSystem");

    bool ExactSha(const FString& Sha)
    {
        if (Sha.Len() != 40) return false;
        for (const TCHAR Ch : Sha)
            if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return false;
        return true;
    }

    FString NormalizedPath(const FString& Path)
    {
        FString Full = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(Full);
        FPaths::CollapseRelativeDirectories(Full);
        return Full;
    }

    bool OutsideProject(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        const FString Full = NormalizedPath(Path);
        FString Project = NormalizedPath(FPaths::ProjectDir());
        FPaths::NormalizeDirectoryName(Project);
        return !Full.Equals(Project, ESearchCase::IgnoreCase) &&
            !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }

    bool BackupIsExact(const FString& Backup, const FString& Live)
    {
        TArray<uint8> Original, Snapshot;
        return OutsideProject(Backup) && FPaths::FileExists(Backup) &&
            FPaths::FileExists(Live) && FFileHelper::LoadFileToArray(Original, *Live) &&
            FFileHelper::LoadFileToArray(Snapshot, *Backup) &&
            !Original.IsEmpty() && Original == Snapshot;
    }

    bool Report(const FString& Path, const FString& Mode,
        const FString& Manifest, const TArray<FString>& Sources, int32 Old, int32 Saved)
    {
        if (!OutsideProject(Path) || Sources.Num() != 5) return false;
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X60/stone-one/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("targets"), 1);
        Root->SetNumberField(TEXT("loot_rows"), 5);
        Root->SetNumberField(TEXT("legacy_pointers"), Old);
        Root->SetNumberField(TEXT("packages_saved"), Saved);
        Root->SetBoolField(TEXT("only_goblin_package_allowed"), true);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        Root->SetStringField(TEXT("normalized_monsters"), Manifest);
        TArray<TSharedPtr<FJsonValue>> Rows;
        for (int32 Index = 0; Index < 5; ++Index)
        {
            TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("item_id"), FPlan::ItemId(Index).ToString());
            Row->SetStringField(TEXT("source"), FPlan::SourcePath(Index));
            Row->SetStringField(TEXT("authored_properties"), Sources[Index]);
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        Root->SetArrayField(TEXT("source_evidence"), Rows);
        FString Text;
        return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text)) &&
            FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X60NativeStoneLootCommandlet::UGridArmClean064X60NativeStoneLootCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X60NativeStoneLootCommandlet::Main(const FString& Params)
{
    using namespace ARMCLEAN064X60;
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (static_cast<int32>(bPreflight) + static_cast<int32>(bVerify) + static_cast<int32>(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output))
        return 1;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Goblin(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FPlan::MonsterPath(0)));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Rat(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FPlan::MonsterPath(1)));
    if (!Root.IsValid() || Root->ItemDefinitions.Num() != 34 || !Goblin.IsValid() || !Rat.IsValid() ||
        Root->GetOutermost()->IsDirty() || Goblin->GetOutermost()->IsDirty() || Rat->GetOutermost()->IsDirty())
        return 2;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>(GetTransientPackage()));
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Registry.IsValid() || !Registry->RegisterItemSystem(Root.Get(), Errors)) return 2;
    TArray<FGridMonsterLootEntry> Plan;
    int32 Old = INDEX_NONE;
    FString Error;
    if (!FPlan::TryBuild(*Goblin.Get(), *Rat.Get(), Registry.Get(), Plan, Old, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X60] Plan refused: %s"), *Error);
        return 3;
    }
    const FString Manifest = FPlan::Export(*Goblin.Get(), &Plan) + FPlan::Export(*Rat.Get());
    TArray<FString> Sources;
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const UGridItemDefinitionAsset* Source = FPlan::LoadSource(Index);
        if (!IsValid(Source) || Source->GetOutermost()->IsDirty()) return 3;
        Sources.Add(FPlan::Export(*Source));
        if (bPreflight)
            UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X60] SOURCE %s\n%s"), *FPlan::ItemId(Index).ToString(), *Sources.Last());
    }
    if (Root->GetOutermost()->IsDirty() || Goblin->GetOutermost()->IsDirty() || Rat->GetOutermost()->IsDirty())
        return 3;
    const FString Mode = bApply ? TEXT("Apply") : (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight) return Report(Output, Mode, Manifest, Sources, Old, 0) ? 0 : 9;
    if (bVerify)
    {
        if (!Report(Output, Mode, Manifest, Sources, Old, 0)) return 9;
        return Old == 0 ? 0 : 4;
    }
    if (Old == 0) return Report(Output, Mode, Manifest, Sources, 0, 0) ? 0 : 9;
    FString Head, Backup;
    if (Old != 1 || !FParse::Value(*Params, TEXT("ExpectedHead="), Head) || !ExactSha(Head) ||
        !FParse::Value(*Params, TEXT("Backup="), Backup) || !OutsideProject(Backup) ||
        NormalizedPath(Backup).Equals(NormalizedPath(Output), ESearchCase::IgnoreCase))
        return 5;
    UPackage* Package = Goblin->GetOutermost();
    const FString Live = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    if (!BackupIsExact(Backup, Live) || Package->IsDirty()) return 5;

    Goblin->Modify();
    Goblin->LootTable[1].ItemDefinitionId = TEXT("Stone");
    Goblin->LootTable[1].ItemDefinitionAsset = nullptr;
    TArray<FGridMonsterLootEntry> Rechecked;
    int32 Remaining = INDEX_NONE;
    if (!FPlan::SameTable(Goblin->LootTable, Plan) ||
        !FPlan::TryBuild(*Goblin.Get(), *Rat.Get(), Registry.Get(), Rechecked, Remaining, Error) ||
        Remaining != 0 || !FPlan::SameTable(Goblin->LootTable, Rechecked) ||
        FPlan::Export(*Goblin.Get()) + FPlan::Export(*Rat.Get()) != Manifest ||
        Root->GetOutermost()->IsDirty() || Rat->GetOutermost()->IsDirty())
        return 7;
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Goblin.Get(), *Live, Args)) return 8;
    if (!Report(Output, Mode, Manifest, Sources, 0, 1)) return 9;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X60] Stone-only Goblin loot saved; verify in fresh UE."));
    return 0;
}
