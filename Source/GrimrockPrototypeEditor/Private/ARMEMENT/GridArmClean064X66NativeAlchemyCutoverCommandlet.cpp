#include "ARMEMENT/GridArmClean064X66NativeAlchemyCutoverCommandlet.h"

#include "GridEditorAlchemyCutoverPlan.h"
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

namespace ARMCLEAN064X66
{
    namespace Plan = GridEditorAlchemyCutoverPlan;
    namespace Audit = GridEditorAlchemyMigrationAudit;
    using FCopy = FGridEditorLootDefinitionProjection;
    using FStone = FCopy::FStone;
    const TCHAR* ActionPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ActionCatalogue.DA_ActionCatalogue");
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

    FString Manifest(const UGridItemSystemAsset& Root, const UGridItemSystemAsset& Expected)
    {
        FString Result = FCopy::RootManifest(Root);
        for (int32 I = 0; I < Plan::BeforeCount; ++I)
            Result += Root.ItemDefinitions[I]->GetPathName() + TEXT("\n");
        Result += FStone::Export(*Root.WeaponCatalogue) + FStone::Export(*Root.WeaponVisualCatalogue) +
            FStone::Export(*Root.ActionPresentationCatalogue);
        // Normalize ONLY the appended detailed actions; preserve all other
        // catalogue fields, including the compact action catalogue.
        for (TFieldIterator<FProperty> Field(Root.ActionCatalogue->GetClass(), EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient) ||
                Field->GetFName() == GET_MEMBER_NAME_CHECKED(UGridActionCatalogueAsset, DetailedActions)) continue;
            FString Value;
            Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(Root.ActionCatalogue.Get()),
                nullptr, Root.ActionCatalogue.Get(), PPF_None);
            Result += Field->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        for (const auto& Action : Expected.ActionCatalogue->DetailedActions)
            Result += Audit::ActionExport(Action.Definition);
        for (int32 I = 0; I < Audit::LegacyCount; ++I)
        {
            Result += Plan::TargetPath(I) + TEXT("\n");
            const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *FRPGAlchemistAuthoring::GetItemObjectPath(Audit::Id(I)));
            if (!IsValid(Old)) return FString();
            Result += FStone::Export(*Old);
            const auto* New = Expected.ItemDefinitions[Plan::BeforeCount+I].Get();
            for (TFieldIterator<FProperty> Field(New->GetClass()); Field; ++Field)
            {
                if (!Field->GetOwnerStruct()->IsChildOf(UGridCanonicalItemDefinitionAsset::StaticClass()) ||
                    Field->HasAnyPropertyFlags(CPF_Transient)) continue;
                FString Value;
                Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(New), nullptr,
                    const_cast<UGridCanonicalItemDefinitionAsset*>(New), PPF_None);
                Result += Field->GetName() + TEXT("=") + Value + TEXT("\n");
            }
        }
        return Result;
    }

    bool Report(const FString& Path, const FString& Mode, const FString& Manifest, int32 Count, int32 Saved)
    {
        if (!OutsideProject(Path) || Manifest.IsEmpty()) return false;
        TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X66/ten-alchemy-twelve-packages/v1"));
        Root->SetStringField(TEXT("mode"), Mode);
        Root->SetNumberField(TEXT("registry_count"), Count);
        Root->SetNumberField(TEXT("unregistered_count"), Count == Plan::BeforeCount ? 10 : 0);
        Root->SetNumberField(TEXT("detailed_actions"), Count == Plan::BeforeCount ? 4 : 14);
        Root->SetNumberField(TEXT("packages_saved"), Saved);
        Root->SetNumberField(TEXT("targets"), 12);
        Root->SetBoolField(TEXT("savegames_touched"), false);
        Root->SetStringField(TEXT("normalized_manifest"), Manifest);
        TArray<TSharedPtr<FJsonValue>> Targets;
        for (int32 I = 0; I < Audit::LegacyCount; ++I)
            Targets.Add(MakeShared<FJsonValueString>(Plan::TargetPath(I)));
        Root->SetArrayField(TEXT("new_definitions"), Targets);
        FString Json;
        return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

UGridArmClean064X66NativeAlchemyCutoverCommandlet::UGridArmClean064X66NativeAlchemyCutoverCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X66NativeAlchemyCutoverCommandlet::Main(const FString& Params)
{
    using namespace ARMCLEAN064X66;
    const bool bPreflight = FParse::Param(*Params, TEXT("Preflight"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Output;
    if (int32(bPreflight) + int32(bVerify) + int32(bApply) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output) ||
        FPaths::FileExists(Output)) return 1;

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!Root.IsValid() || !Root->ActionCatalogue || Root->ActionCatalogue->GetPathName() != FString(ActionPath))
        return 2;
    const int32 Count = Root->ItemDefinitions.Num();
    const bool bMigrated = Count == Plan::AfterCount;
    FString Error;
    if (!Plan::BuildForCurrentState(*Root.Get(), *Preview.Get(), Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X66] Preflight refused: %s"), *Error);
        return 3;
    }
    for (int32 I = 0; I < Audit::LegacyCount; ++I)
        if (FPackageName::DoesPackageExist(Plan::TargetPackage(I)) != bMigrated)
            return 3;

    const FString Original = Manifest(*Root.Get(), *Preview.Get());
    if (Original.IsEmpty()) return 3;
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
    UObject* Existing[] = {Root.Get(), Root->ActionCatalogue.Get()};
    TSet<FString> Backups;
    for (int32 I = 0; I < 2; ++I)
    {
        FString Backup;
        const FString Key = FString::Printf(TEXT("Backup%d="), I);
        if (!FParse::Value(*Params, *Key, Backup) ||
            !BackupIsExact(Backup, LivePath(*Existing[I])) ||
            NormalizedPath(Backup).Equals(NormalizedPath(Output), ESearchCase::IgnoreCase) ||
            Backups.Contains(NormalizedPath(Backup).ToLower())) return 5;
        Backups.Add(NormalizedPath(Backup).ToLower());
    }

    TArray<UGridConsumableDefinitionAsset*> NewDefinitions;
    for (int32 I = 0; I < Audit::LegacyCount; ++I)
    {
        if (FPackageName::DoesPackageExist(Plan::TargetPackage(I))) return 5;
        UPackage* Package = CreatePackage(*Plan::TargetPackage(I));
        auto* New = NewObject<UGridConsumableDefinitionAsset>(Package, FName(*Plan::TargetName(I)),
            RF_Public | RF_Standalone | RF_Transactional);
        const auto* Expected = Cast<UGridConsumableDefinitionAsset>(Preview->ItemDefinitions[Plan::BeforeCount+I].Get());
        if (!IsValid(New) || !IsValid(Expected)) return 6;
        for (TFieldIterator<FProperty> Field(Expected->GetClass()); Field; ++Field)
        {
            if (!Field->GetOwnerStruct()->IsChildOf(UGridCanonicalItemDefinitionAsset::StaticClass()) ||
                Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            Field->CopyCompleteValue(Field->ContainerPtrToValuePtr<void>(New),
                Field->ContainerPtrToValuePtr<void>(Expected));
        }
        if (!FCopy::SameDefinition(*New, *Expected)) return 6;
        NewDefinitions.Add(New);
    }
    Root->Modify();
    for (auto* Definition : NewDefinitions) Root->ItemDefinitions.Add(Definition);
    Root->ActionCatalogue->Modify();
    Root->ActionCatalogue->DetailedActions = Preview->ActionCatalogue->DetailedActions;
    if (!Plan::MatchesProjected(*Root.Get(), *Preview.Get(), Error, true) ||
        Original != Manifest(*Root.Get(), *Preview.Get())) return 7;
    for (auto* Definition : NewDefinitions)
        if (!SaveExact(Definition)) return 8;
    // Persist the actions before the root that references their identities.
    if (!SaveExact(Existing[1]) || !SaveExact(Existing[0])) return 8;
    if (!Report(Output, Mode, Original, Plan::AfterCount, 12)) return 9;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X66] Twelve packages saved; reopen and verify before publication."));
    return 0;
}
