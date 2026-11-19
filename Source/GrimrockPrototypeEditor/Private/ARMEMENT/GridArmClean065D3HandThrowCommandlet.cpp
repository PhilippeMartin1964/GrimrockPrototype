#include "ARMEMENT/GridArmClean065D3HandThrowCommandlet.h"
#include "GridEditorArmHandThrowPlan.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

namespace ArmHandThrow = GridEditorArmHandThrowPlan;
namespace
{
    const TCHAR* RootPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const TCHAR* Numeric = TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponCatalogue.DA_CanonicalWeaponCatalogue");
    const TCHAR* Visual = TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_CanonicalWeaponVisualCatalogue.DA_CanonicalWeaponVisualCatalogue");

    bool OutsideProject(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        FString Full = FPaths::ConvertRelativePathToFull(Path);
        FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FPaths::NormalizeFilename(Full); FPaths::CollapseRelativeDirectories(Full);
        FPaths::NormalizeDirectoryName(Project); FPaths::CollapseRelativeDirectories(Project);
        return !Full.Equals(Project, ESearchCase::IgnoreCase) && !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }

    FString Filename(const UObject& Obj)
    {
        return FPackageName::LongPackageNameToFilename(Obj.GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    }

    bool ExactBackup(const UObject& Obj, const FString& Backup)
    {
        TArray<uint8> A, B;
        return OutsideProject(Backup) && FFileHelper::LoadFileToArray(A, *Filename(Obj)) &&
            FFileHelper::LoadFileToArray(B, *Backup) && !A.IsEmpty() && A == B;
    }

    bool Save(UObject& Obj)
    {
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        Obj.GetOutermost()->MarkPackageDirty();
        return UPackage::SavePackage(Obj.GetOutermost(), &Obj, *Filename(Obj), Args);
    }

    bool Report(const FString& Output, const TCHAR* Mode, const ArmHandThrow::FPlan& Plan,
        int32 Saved, const FString& Manifest)
    {
        TSharedRef<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.5D3/arm-hand-throw/v1"));
        R->SetStringField(TEXT("mode"), Mode);
        R->SetNumberField(TEXT("ordinary"), Plan.Ordinary);
        R->SetNumberField(TEXT("specialized"), Plan.Specialized);
        R->SetNumberField(TEXT("authored"), Plan.Already);
        R->SetNumberField(TEXT("physically_throwable"), Plan.PhysicallyThrowable);
        R->SetNumberField(TEXT("packages_saved"), Saved);
        R->SetStringField(TEXT("normalized_manifest"), Manifest);
        R->SetBoolField(TEXT("savegames_touched"), false);
        FString Json;
        return FJsonSerializer::Serialize(R, TJsonWriterFactory<>::Create(&Json)) &&
            FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}
UGridArmClean065D3HandThrowCommandlet::UGridArmClean065D3HandThrowCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}
int32 UGridArmClean065D3HandThrowCommandlet::Main(const FString& Params)
{
    const bool bAudit = FParse::Param(*Params, TEXT("Audit"));
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString Output;
    if (int32(bAudit) + int32(bApply) + int32(bVerify) != 1 ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Output) || FPaths::FileExists(Output))
        return 1;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>(GetTransientPackage()));
    if (!Root.IsValid() || !Root->WeaponCatalogue || !Root->WeaponVisualCatalogue ||
        Root->WeaponCatalogue->GetPathName() != Numeric || Root->WeaponVisualCatalogue->GetPathName() != Visual ||
        Root->GetOutermost()->IsDirty() || Root->WeaponCatalogue->GetOutermost()->IsDirty() ||
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty())
        return 2;
    ArmHandThrow::FPlan Plan;
    FString Error;
    if (!ArmHandThrow::Build(*Root.Get(), Plan, Error) || !ArmHandThrow::AssignPreview(*Root.Get(), Plan, *Preview.Get()))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.5D3] projection refused: %s"), *Error);
        return 3;
    }
    const FString Manifest = ArmHandThrow::Manifest(*Root.Get(), *Preview.Get());
    if (bAudit)
        return Report(Output, TEXT("Audit"), Plan, 0, Manifest) ? 0 : 9;
    if (bVerify)
        return Report(Output, TEXT("Verify"), Plan, 0, Manifest) ?
            (Plan.Already == ArmHandThrow::OrdinaryCount ? 0 : 4) : 9;
    if (Plan.Already != 0) return 5;
    FString Head, Backup0, Backup1;
    if (!FParse::Value(*Params, TEXT("ExpectedHead="), Head) || Head.Len() != 40 ||
        !FParse::Value(*Params, TEXT("Backup0="), Backup0) ||
        !FParse::Value(*Params, TEXT("Backup1="), Backup1) || Backup0 == Backup1 ||
        !ExactBackup(*Root->WeaponCatalogue, Backup0) || !ExactBackup(*Root->WeaponVisualCatalogue, Backup1))
        return 5;
    for (TCHAR Ch : Head) if (!FChar::IsDigit(Ch) && (Ch < TEXT('a') || Ch > TEXT('f'))) return 5;
    Root->WeaponCatalogue->Archetypes = Plan.Archetypes;
    Root->WeaponVisualCatalogue->VisualProfiles = Plan.Visuals;
    ArmHandThrow::FPlan Rechecked;
    if (!ArmHandThrow::Build(*Root.Get(), Rechecked, Error) || Rechecked.Already != ArmHandThrow::OrdinaryCount ||
        ArmHandThrow::Export(*Root->WeaponCatalogue) != ArmHandThrow::Export(*Preview->WeaponCatalogue) ||
        ArmHandThrow::Export(*Root->WeaponVisualCatalogue) != ArmHandThrow::Export(*Preview->WeaponVisualCatalogue))
        return 6;
    if (!Save(*Root->WeaponCatalogue) || !Save(*Root->WeaponVisualCatalogue)) return 7;
    return Report(Output, TEXT("Apply"), Rechecked, 2, Manifest) ? 0 : 9;
}
