#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"

namespace UITooltipClean01
{
    struct FCandidate
    {
        const TCHAR* Name;
        const TCHAR* Package;
        bool bLegacy;
    };

    // Exact package names: no binary deletion or mutation from the audit.
    static const FCandidate Candidates[] = {
        { TEXT("WBP_ItemTooltip"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltip"), false },
        { TEXT("WBP_ItemTooltipNativeLine"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipNativeLine"), false },
        { TEXT("WBP_ItemTooltipNativeComparison"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipNativeComparison"), false },
        { TEXT("WBP_ItemTooltipComparisonRow"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipComparisonRow"), true },
        { TEXT("WBP_ItemTooltipStatLine"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipStatLine"), true },
        // The pre-migration package paths must remain absent as well.
        { TEXT("WBP_ItemTooltipComparisonRow_OldPath"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltipComparisonRow"), true },
        { TEXT("WBP_ItemTooltipStatLine_OldPath"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltipStatLine"), true }
    };

    FString Escape(FString Value)
    {
        Value.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Value);
    }

    FString FormatRefs(TArray<FName> Names)
    {
        Names.Sort([](FName A, FName B)
        {
            return A.ToString() < B.ToString();
        });
        TArray<FString> Values;
        for (const FName& Name : Names)
        {
            Values.Add(Name.ToString());
        }
        return FString::Join(Values, TEXT("; "));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorUITooltipClean01ReferenceAudit,
    "Grimrock.Editor.UITooltipClean01.ReferenceAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorUITooltipClean01ReferenceAudit::RunTest(const FString&)
{
    using namespace UITooltipClean01;
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);

    TArray<FString> Report = {
        TEXT("Name,Package,Exists,ReferencerCount,Referencers,DependencyCount,Dependencies,Decision")
    };
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Audits/UI-TOOLTIP-CLEAN01"));
    for (const FCandidate& Candidate : Candidates)
    {
        const FName PackageName(Candidate.Package);
        TArray<FAssetData> PackageAssets;
        Registry.GetAssetsByPackageName(PackageName, PackageAssets, true, false);
        const bool bExists = !PackageAssets.IsEmpty();
        TArray<FName> Referencers;
        TArray<FName> Dependencies;
        const bool bRefQuery = bExists && Registry.GetReferencers(
            PackageName, Referencers, UE::AssetRegistry::EDependencyCategory::All,
            UE::AssetRegistry::FDependencyQuery());
        const bool bDepQuery = bExists && Registry.GetDependencies(
            PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::All,
            UE::AssetRegistry::FDependencyQuery());
        // CLEAN02.2 is a permanent regression guard, not an invitation to
        // repeat obsolete WBP reference audits.
        if (Candidate.bLegacy)
        {
            TestFalse(*FString::Printf(TEXT("Deleted tooltip WBP stays absent: %s"),
                Candidate.Package), bExists);
        }
        else
        {
            TestTrue(*FString::Printf(TEXT("Active tooltip WBP remains present: %s"),
                Candidate.Package), bExists);
        }
        const FString Decision = Candidate.bLegacy
            ? (bExists ? TEXT("REGRESSION_REINTRODUCED") : TEXT("REMOVED"))
            : (bExists ? TEXT("KEEP_ACTIVE") : TEXT("MISSING_ACTIVE"));
        AddInfo(FString::Printf(
            TEXT("[UI-TOOLTIP-CLEAN01] %s Exists=%s Referencers=%d Dependencies=%d Decision=%s"),
            Candidate.Name, bExists ? TEXT("true") : TEXT("false"),
            Referencers.Num(), Dependencies.Num(), *Decision));
        for (const FName& Ref : Referencers)
        {
            AddInfo(FString::Printf(TEXT("[UI-TOOLTIP-CLEAN01]   %s <- %s"),
                Candidate.Name, *Ref.ToString()));
        }
        const TArray<FString> Columns = {
            Escape(Candidate.Name), Escape(Candidate.Package),
            Escape(bExists ? TEXT("true") : TEXT("false")),
            Escape(FString::FromInt(Referencers.Num())),
            Escape(FormatRefs(Referencers)),
            Escape(FString::FromInt(Dependencies.Num())),
            Escape(bDepQuery ? FormatRefs(Dependencies) : FString()),
            Escape(Decision)
        };
        Report.Add(FString::Join(Columns, TEXT(",")));
    }
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Filename = FPaths::Combine(
        Directory, TEXT("UI-TOOLTIP-CLEAN01_Referencers.csv"));
    if (!TestTrue(TEXT("Read-only tooltip referencer report written"),
        FFileHelper::SaveStringArrayToFile(Report, *Filename)))
        return false;
    AddInfo(FString::Printf(TEXT("[UI-TOOLTIP-CLEAN01] Report=%s"), *Filename));
    AddInfo(TEXT("[UI-TOOLTIP-CLEAN02.2] Deleted WBPs must remain absent; native tooltip root and children must remain present."));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
