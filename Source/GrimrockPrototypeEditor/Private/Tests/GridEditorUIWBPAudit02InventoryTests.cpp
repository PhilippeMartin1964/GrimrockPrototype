#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace UIWBPAudit02
{
    const TCHAR* UiRoot = TEXT("/Game/GrimrockPrototype/Blueprints/UI");
    constexpr int32 ExpectedCount = 46;

    struct FTextFile
    {
        FString Name;
        FString Body;
    };

    FString CsvCell(FString Value)
    {
        Value.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Value);
    }

    void NormalizeNames(TArray<FName>& Names)
    {
        Names.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
        for (int32 I = Names.Num() - 1; I > 0; --I)
        {
            if (Names[I] == Names[I - 1])
            {
                Names.RemoveAt(I);
            }
        }
    }

    FString NameList(const TArray<FName>& Names)
    {
        TArray<FString> Values;
        for (FName Name : Names)
        {
            Values.Add(Name.ToString());
        }
        return FString::Join(Values, TEXT(" ; "));
    }

    void ScanSourceFiles(TArray<FTextFile>& OutFiles)
    {
        const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        TArray<FString> Files;
        for (const TCHAR* Pattern : { TEXT("*.cpp"), TEXT("*.h"), TEXT("*.cs") })
        {
            IFileManager::Get().FindFilesRecursive(
                Files, *FPaths::Combine(Root, TEXT("Source")), Pattern, true, false);
        }
        IFileManager::Get().FindFilesRecursive(
            Files, *FPaths::Combine(Root, TEXT("Config")), TEXT("*.ini"), true, false);
        Files.Sort();
        for (const FString& Filename : Files)
        {
            FString Normalized = Filename;
            FPaths::NormalizeFilename(Normalized);
            if (Normalized.Contains(TEXT("/Tests/"), ESearchCase::IgnoreCase))
            {
                continue;
            }
            FTextFile File;
            File.Name = Normalized;
            FString NormalizedRoot = Root;
            FPaths::NormalizeFilename(NormalizedRoot);
            if (File.Name.StartsWith(NormalizedRoot, ESearchCase::IgnoreCase))
            {
                File.Name.RightChopInline(NormalizedRoot.Len());
                File.Name.RemoveFromStart(TEXT("/"));
            }
            if (FFileHelper::LoadFileToString(File.Body, *Filename))
            {
                OutFiles.Add(MoveTemp(File));
            }
        }
    }

    bool IsKnownRoot(const FString& Name)
    {
        const TCHAR* Names[] = {
            TEXT("WBP_MainMenu"), TEXT("WBP_GridPersistentHud"),
            TEXT("WBP_GridCombatHud"), TEXT("WBP_GridEventFeed"),
            TEXT("WBP_CharacterCreationWizard"), TEXT("WBP_CharacterSheet"),
            TEXT("WBP_InventoryBag"), TEXT("WBP_GrimrockMenu"),
            TEXT("WBP_GridSkills"), TEXT("WBP_GridMap"),
            TEXT("WBP_RPGNotification"), TEXT("WBP_PartyPortraitHud"),
            TEXT("WBP_GridMouseCursor")
        };
        for (const TCHAR* Known : Names)
        {
            if (Name == Known)
            {
                return true;
            }
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FUIWBPAudit02Inventory,
    "Grimrock.Editor.UIWBPAudit02.Inventory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIWBPAudit02Inventory::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace UIWBPAudit02;
    using namespace UE::AssetRegistry;

    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    TArray<FAssetData> AllUiAssets;
    if (!TestTrue(TEXT("AssetRegistry loads the production UI subtree"),
        Registry.GetAssetsByPath(FName(UiRoot), AllUiAssets, true, true)))
    {
        return false;
    }

    TArray<FAssetData> Widgets;
    for (const FAssetData& Data : AllUiAssets)
    {
        if (Data.AssetName.ToString().StartsWith(TEXT("WBP_")))
        {
            Widgets.Add(Data);
        }
    }
    Widgets.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.PackageName.ToString() < B.PackageName.ToString();
    });

    TArray<FTextFile> SourceFiles;
    ScanSourceFiles(SourceFiles);

    FString Csv = TEXT("Name,Package,Class,HardCount,SoftCount,OtherCount,HardReferencers,SoftReferencers,OtherReferencers,HardDependencies,SoftDependencies,SourceConfigCount,SourceConfigFiles,Decision,Reason\n");
    FString Md = TEXT("# UI-WBP-AUDIT02 — Rapport AssetRegistry UE5.5.4\n\n");
    Md += TEXT("Audit en lecture seule des dependances on-disk. Les mentions Source/Config incluent potentiellement des commentaires et ne prouvent pas une utilisation runtime.\n\n");
    Md += TEXT("| Widget | Hard | Soft | Other | Source/Config | Etat |\n|---|---:|---:|---:|---:|---|\n");
    FString Detail;
    int32 Referenced = 0;
    int32 SourceOnly = 0;
    int32 RootOnly = 0;
    int32 Zero = 0;
    int32 Redirectors = 0;

    for (const FAssetData& Data : Widgets)
    {
        const FString Name = Data.AssetName.ToString();
        const FString Class = Data.AssetClassPath.GetAssetName().ToString();
        const FName Package = Data.PackageName;
        TArray<FName> AllRefs, HardRefs, SoftRefs, OtherRefs, HardDeps, SoftDeps;
        Registry.GetReferencers(Package, AllRefs, EDependencyCategory::All, FDependencyQuery());
        Registry.GetReferencers(Package, HardRefs, EDependencyCategory::Package,
            FDependencyQuery(EDependencyQuery::Hard));
        Registry.GetReferencers(Package, SoftRefs, EDependencyCategory::Package,
            FDependencyQuery(EDependencyQuery::Soft));
        Registry.GetDependencies(Package, HardDeps, EDependencyCategory::Package,
            FDependencyQuery(EDependencyQuery::Hard));
        Registry.GetDependencies(Package, SoftDeps, EDependencyCategory::Package,
            FDependencyQuery(EDependencyQuery::Soft));
        NormalizeNames(AllRefs);
        NormalizeNames(HardRefs);
        NormalizeNames(SoftRefs);
        NormalizeNames(HardDeps);
        NormalizeNames(SoftDeps);
        for (FName Referencer : AllRefs)
        {
            if (!HardRefs.Contains(Referencer) && !SoftRefs.Contains(Referencer))
            {
                OtherRefs.Add(Referencer);
            }
        }
        NormalizeNames(OtherRefs);

        TArray<FString> Mentions;
        for (const FTextFile& File : SourceFiles)
        {
            if (File.Body.Contains(Name, ESearchCase::CaseSensitive))
            {
                Mentions.Add(File.Name);
            }
        }

        FString Decision;
        FString Reason;
        if (Class == TEXT("ObjectRedirector"))
        {
            ++Redirectors;
            Decision = TEXT("REDIRECTOR_REVIEW");
            Reason = TEXT("Old redirector; inspect and Fix Up Redirectors in Unreal");
        }
        else if (!AllRefs.IsEmpty())
        {
            ++Referenced;
            Decision = TEXT("REFERENCED_KEEP");
            Reason = TEXT("On-disk AssetRegistry referencers exist");
        }
        else if (!Mentions.IsEmpty())
        {
            ++SourceOnly;
            Decision = TEXT("SOURCE_CONFIG_REVIEW");
            Reason = TEXT("Only production Source/Config text hints; may be loaded dynamically");
        }
        else if (IsKnownRoot(Name))
        {
            ++RootOnly;
            Decision = TEXT("DYNAMIC_ROOT_REVIEW");
            Reason = TEXT("Known UI entry point may be assigned in BP defaults or maps");
        }
        else
        {
            ++Zero;
            Decision = TEXT("ZERO_REFERENCER_REVIEW");
            Reason = TEXT("No on-disk or production text referencer found; NOT deletion approval");
        }

        const TArray<FString> Fields = {
            Name, Package.ToString(), Class, FString::FromInt(HardRefs.Num()),
            FString::FromInt(SoftRefs.Num()), FString::FromInt(OtherRefs.Num()),
            NameList(HardRefs), NameList(SoftRefs), NameList(OtherRefs),
            NameList(HardDeps), NameList(SoftDeps), FString::FromInt(Mentions.Num()),
            FString::Join(Mentions, TEXT(" ; ")), Decision, Reason
        };
        for (int32 I = 0; I < Fields.Num(); ++I)
        {
            if (I > 0) Csv += TEXT(",");
            Csv += CsvCell(Fields[I]);
        }
        Csv += TEXT("\n");

        Md += FString::Printf(TEXT("| %s | %d | %d | %d | %d | %s |\n"),
            *Name, HardRefs.Num(), SoftRefs.Num(), OtherRefs.Num(), Mentions.Num(), *Decision);
        Detail += FString::Printf(TEXT("\n### %s\n\n- Package: %s\n- Class: %s\n- Decision: %s — %s\n"),
            *Name, *Package.ToString(), *Class, *Decision, *Reason);
        Detail += FString::Printf(TEXT("- Hard referencers: %s\n- Soft referencers: %s\n- Other referencers: %s\n"),
            HardRefs.IsEmpty() ? TEXT("(none)") : *NameList(HardRefs),
            SoftRefs.IsEmpty() ? TEXT("(none)") : *NameList(SoftRefs),
            OtherRefs.IsEmpty() ? TEXT("(none)") : *NameList(OtherRefs));
        Detail += FString::Printf(TEXT("- Hard dependencies: %s\n- Soft dependencies: %s\n- Source/Config mentions: %s\n"),
            HardDeps.IsEmpty() ? TEXT("(none)") : *NameList(HardDeps),
            SoftDeps.IsEmpty() ? TEXT("(none)") : *NameList(SoftDeps),
            Mentions.IsEmpty() ? TEXT("(none)") : *FString::Join(Mentions, TEXT(" ; ")));
    }

    Md += FString::Printf(
        TEXT("\nTotal %d / baseline %d. Referenced=%d; SourceOnly=%d; KnownRoots=%d; ZeroRefs=%d; Redirectors=%d; SourceFiles=%d.\n\n"),
        Widgets.Num(), ExpectedCount, Referenced, SourceOnly, RootOnly, Zero, Redirectors, SourceFiles.Num());
    Md += TEXT("Zero references NEVER authorizes deletion. Check Reference Viewer (hard/soft), serialized Class Defaults and levels, runtime dynamic loading, then PIE. This report does not touch any uasset.\n\n");
    Md += TEXT("## Individual asset dependencies\n");
    Md += Detail;

    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audits/UI-WBP-AUDIT02"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString CsvFile = FPaths::Combine(Folder, TEXT("UI-WBP-AUDIT02.csv"));
    const FString MdFile = FPaths::Combine(Folder, TEXT("UI-WBP-AUDIT02.md"));
    const bool bCsvOK = FFileHelper::SaveStringToFile(
        Csv, *CsvFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const bool bMdOK = FFileHelper::SaveStringToFile(
        Md, *MdFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    AddInfo(FString::Printf(TEXT("[UI-WBP-AUDIT02] Widgets=%d Referenced=%d SourceOnly=%d KnownRoots=%d ZeroRefs=%d Redirectors=%d"),
        Widgets.Num(), Referenced, SourceOnly, RootOnly, Zero, Redirectors));
    AddInfo(FString::Printf(TEXT("[UI-WBP-AUDIT02] CSV: %s"), *CsvFile));
    AddInfo(FString::Printf(TEXT("[UI-WBP-AUDIT02] Markdown: %s"), *MdFile));
    TestTrue(TEXT("Complete CSV report generated under Saved"), bCsvOK);
    TestTrue(TEXT("Complete Markdown report generated under Saved"), bMdOK);
    TestEqual(TEXT("Expected 46 UI Widget Blueprints"), Widgets.Num(), ExpectedCount);
    TestEqual(TEXT("No WBP redirectors are left in UI folder"), Redirectors, 0);
    return bCsvOK && bMdOK && Widgets.Num() == ExpectedCount && Redirectors == 0;
}

#endif
