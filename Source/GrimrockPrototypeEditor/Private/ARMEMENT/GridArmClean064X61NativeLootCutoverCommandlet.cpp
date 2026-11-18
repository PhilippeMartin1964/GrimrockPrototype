#include "ARMEMENT/GridArmClean064X61NativeLootCutoverCommandlet.h"

#include "GridEditorLootDefinitionProjection.h"
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

namespace ARMCLEAN064X61
{
    using FPlan = FGridEditorLootDefinitionProjection;
    using FStone = FPlan::FStone;
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

    FString LivePath(const UObject& Asset)
    {
        return FPackageName::LongPackageNameToFilename(Asset.GetOutermost()->GetName(),
            FPackageName::GetAssetPackageExtension());
    }

    bool SaveExact(UObject* Asset)
    {
        if (!IsValid(Asset)) return false;
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        Asset->GetOutermost()->MarkPackageDirty();
        return UPackage::SavePackage(Asset->GetOutermost(), Asset, *LivePath(*Asset), Args);
    }

    FString Manifest(const UGridItemSystemAsset& Root, const UGridMonsterDefinitionAsset& Goblin,
        const UGridMonsterDefinitionAsset& Rat, const TArray<FGridMonsterLootEntry>& GoblinPlan,
        const TArray<FGridMonsterLootEntry>& RatPlan)
    {
        FString Result = FPlan::RootManifest(Root) + FStone::Export(Goblin, &GoblinPlan) +
            FStone::Export(Rat, &RatPlan);
        // Exact legacy evidence stays in the manifest for this single
        // conversion. The persistent new definitions are independently
        // compared against fresh projections by BuildPreview.
        for (int32 I = 0; I < FPlan::AddedCount; ++I)
            Result += FStone::Export(*FStone::LoadSource(FPlan::SourceIndex(I)));
        return Result;
    }

    bool Report(const FString& Path, const FString& Mode, const FString& Manifest, int32 Count, int32 Saved)
    {
        if (!OutsideProject(Path)) return false;
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X61/four-loots-seven-packages/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("registry_count"), Count);
        Root->SetNumberField(TEXT("legacy_pointers"), Count == FPlan::BaseCount ? 4 : 0);
        Root->SetNumberField(TEXT("packages_saved"), Saved);
        Root->SetNumberField(TEXT("targets"), 7);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        Root->SetStringField(TEXT("normalized_manifest"), Manifest);
        TArray<TSharedPtr<FJsonValue>> Targets;
        for (int32 I = 0; I < FPlan::AddedCount; ++I)
            Targets.Add(MakeShared<FJsonValueString>(FPlan::TargetPath(I)));
        Root->SetArrayField(TEXT("new_definitions"), Targets);
        FString Json;
        return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X61NativeLootCutoverCommandlet::UGridArmClean064X61NativeLootCutoverCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X61NativeLootCutoverCommandlet::Main(const FString& Params)
{
    using namespace ARMCLEAN064X61;
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (int32(bPreflight) + int32(bVerify) + int32(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output)) return 1;

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Goblin(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FStone::MonsterPath(0)));
    TStrongObjectPtr<UGridMonsterDefinitionAsset> Rat(LoadObject<UGridMonsterDefinitionAsset>(nullptr, FStone::MonsterPath(1)));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!Root.IsValid() || !Goblin.IsValid() || !Rat.IsValid() || !Preview.IsValid() ||
        Root->GetPathName() != FString(RootPath)) return 2;

    const int32 Count = Root->ItemDefinitions.Num();
    const bool bMigrated = Count == FPlan::FinalCount;
    FString Error;
    TArray<FGridMonsterLootEntry> GoblinPlan, RatPlan;
    if (!FPlan::BuildPreview(*Root.Get(), *Preview.Get(), Error) ||
        !FPlan::BuildLootTables(*Goblin.Get(), *Rat.Get(), bMigrated, GoblinPlan, RatPlan, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X61] Preflight refused: %s"), *Error);
        return 3;
    }
    for (int32 I = 0; I < FPlan::AddedCount; ++I)
    {
        if (FPackageName::DoesPackageExist(FPlan::TargetPackage(I)) != bMigrated)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X61] Unexpected package presence: %s"), *FPlan::TargetPackage(I));
            return 3;
        }
    }
    const FString Original = Manifest(*Root.Get(), *Goblin.Get(), *Rat.Get(), GoblinPlan, RatPlan);
    const FString Mode = bApply ? TEXT("Apply") : (bVerify ? TEXT("Verify") : TEXT("Preflight"));
    if (bPreflight) return Report(Output, Mode, Original, Count, 0) ? 0 : 9;
    if (bVerify)
    {
        if (!Report(Output, Mode, Original, Count, 0)) return 9;
        return bMigrated ? 0 : 4;
    }
    if (bMigrated) return Report(Output, Mode, Original, Count, 0) ? 0 : 9;
    FString Head;
    if (!FParse::Value(*Params, TEXT("ExpectedHead="), Head) || !ExactSha(Head)) return 5;
    UObject* Existing[] = {Root.Get(), Goblin.Get(), Rat.Get()};
    TSet<FString> Backups;
    for (int32 I = 0; I < 3; ++I)
    {
        FString Backup;
        const FString Key = FString::Printf(TEXT("Backup%d="), I);
        if (!FParse::Value(*Params, *Key, Backup) ||
            !BackupIsExact(Backup, LivePath(*Existing[I])) ||
            NormalizedPath(Backup).Equals(NormalizedPath(Output), ESearchCase::IgnoreCase) ||
            Backups.Contains(NormalizedPath(Backup).ToLower())) return 5;
        Backups.Add(NormalizedPath(Backup).ToLower());
    }

    // Create the four exact native assets without touching the old sources.
    // All seven packages belong to one transaction: no partly registered loot.
    TArray<UGridNonArmamentDefinitionAsset*> NewDefinitions;
    for (int32 I = 0; I < FPlan::AddedCount; ++I)
    {
        const auto* Old = FStone::LoadSource(FPlan::SourceIndex(I));
        if (!IsValid(Old) || FPackageName::DoesPackageExist(FPlan::TargetPackage(I))) return 5;
        UPackage* Package = CreatePackage(*FPlan::TargetPackage(I));
        auto* New = Cast<UGridNonArmamentDefinitionAsset>(NewObject<UObject>(
            Package, FPlan::ClassFor(I), FName(*FPlan::TargetName(I)),
            RF_Public | RF_Standalone | RF_Transactional));
        const auto* Expected = Cast<UGridNonArmamentDefinitionAsset>(Preview->ItemDefinitions[FPlan::BaseCount + I].Get());
        if (!IsValid(New) || !IsValid(Expected) || !FPlan::Project(*Old, I, *New) ||
            !FPlan::SameDefinition(*New, *Expected)) return 6;
        NewDefinitions.Add(New);
    }

    Root->Modify();
    for (auto* Definition : NewDefinitions) Root->ItemDefinitions.Add(Definition);
    Goblin->Modify();
    Rat->Modify();
    Goblin->LootTable = GoblinPlan;
    Rat->LootTable = RatPlan;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Registry->RegisterItemSystem(Root.Get(), Errors) ||
        !FStone::SameTable(Goblin->LootTable, GoblinPlan) ||
        !FStone::SameTable(Rat->LootTable, RatPlan) ||
        Manifest(*Root.Get(), *Goblin.Get(), *Rat.Get(), Goblin->LootTable, Rat->LootTable) != Original)
        return 7;
    for (int32 I = 0; I < FPlan::AddedCount; ++I)
    {
        const auto* Old = FStone::LoadSource(FPlan::SourceIndex(I));
        if (!IsValid(Old) || !FPlan::ResolveParity(*Registry.Get(), *Old, I)) return 7;
    }
    for (auto* Definition : NewDefinitions)
        if (!SaveExact(Definition)) return 8;
    for (UObject* Asset : Existing)
        if (!SaveExact(Asset)) return 8;
    if (!Report(Output, Mode, Original, FPlan::FinalCount, 7)) return 9;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X61] Seven packages saved; reopen and verify before publication."));
    return 0;
}
