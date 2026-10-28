#include "ARMEMENT/GridWeaponPipelineManifest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"

namespace
{
    constexpr int32 ManifestWidth = 16;
    const TCHAR* const Header[ManifestWidth] =
    {
        TEXT("ArchetypeId"), TEXT("ProductionState"), TEXT("BuildKind"),
        TEXT("ItemDefinitionId"), TEXT("ItemPath"), TEXT("AttackId"),
        TEXT("VisualId"), TEXT("VisualFamily"), TEXT("IconPath"),
        TEXT("MeshPath"), TEXT("MaterialPath"), TEXT("BaseColorPath"),
        TEXT("NormalPath"), TEXT("OrmPath"), TEXT("ClassId"), TEXT("EquipmentSlots")
    };

    bool IsSafeObjectPath(const FString& Path)
    {
        if (!Path.StartsWith(TEXT("/Game/GrimrockPrototype/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")))
        {
            return false;
        }
        FString Left, Right;
        return Path.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive,
                   ESearchDir::FromEnd) &&
            !Left.IsEmpty() && !Right.IsEmpty() &&
            Left.EndsWith(FString(TEXT("/")) + Right);
    }
}

FString FGridWeaponPipelineManifest::DefaultFile()
{
    return FPaths::Combine(FPaths::ProjectDir(),
        TEXT("Config/Armement/WeaponProductionManifest.csv"));
}

const FGridWeaponPipelineRow* FGridWeaponPipelineManifest::Find(
    const TArray<FGridWeaponPipelineRow>& Rows, const FString& ArchetypeId)
{
    return Rows.FindByPredicate([&ArchetypeId](const FGridWeaponPipelineRow& Row)
    {
        return Row.ArchetypeId == ArchetypeId;
    });
}

bool FGridWeaponPipelineManifest::Load(const FString& File,
    TArray<FGridWeaponPipelineRow>& Out, FString& Error)
{
    Out.Reset();
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *File))
    {
        Error = FString::Printf(TEXT("Cannot load manifest: %s"), *File);
        return false;
    }
    FCsvParser Parser(Content);
    const auto& Data = Parser.GetRows();
    if (Data.Num() < 2 || Data[0].Num() != ManifestWidth)
    {
        Error = TEXT("Manifest is empty or header width is not 16");
        return false;
    }
    for (int32 I = 0; I < ManifestWidth; ++I)
    {
        FString Actual(Data[0][I] ? Data[0][I] : TEXT(""));
        if (I == 0 && !Actual.IsEmpty() && Actual[0] == TCHAR(0xFEFF))
        {
            Actual.RightChopInline(1);
        }
        if (Actual != Header[I])
        {
            Error = FString::Printf(TEXT("Manifest column %d must be %s"), I, Header[I]);
            return false;
        }
    }

    TSet<FString> Archetypes, ItemIds, ItemPaths;
    for (int32 Line = 1; Line < Data.Num(); ++Line)
    {
        const TArray<const TCHAR*>& Cells = Data[Line];
        if (Cells.IsEmpty() || !Cells[0] || !*Cells[0])
        {
            continue;
        }
        if (Cells.Num() != ManifestWidth)
        {
            Error = FString::Printf(TEXT("Manifest row %d must contain exactly 16 fields"), Line + 1);
            return false;
        }
        TArray<FString> V;
        V.Reserve(ManifestWidth);
        for (int32 I = 0; I < ManifestWidth; ++I)
        {
            V.Emplace(Cells[I] ? Cells[I] : TEXT(""));
        }
        FGridWeaponPipelineRow& Row = Out.AddDefaulted_GetRef();
        Row.ArchetypeId = V[0]; Row.ProductionState = V[1]; Row.BuildKind = V[2];
        Row.ItemDefinitionId = V[3]; Row.ItemPath = V[4]; Row.AttackId = V[5];
        Row.VisualId = V[6]; Row.VisualFamily = V[7]; Row.IconPath = V[8];
        Row.MeshPath = V[9]; Row.MaterialPath = V[10]; Row.BaseColorPath = V[11];
        Row.NormalPath = V[12]; Row.OrmPath = V[13]; Row.ClassId = V[14];
        Row.EquipmentSlots = V[15];

        if (!UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(FName(Row.ArchetypeId)) ||
            (Row.ProductionState != TEXT("Active") && Row.ProductionState != TEXT("Planned")) ||
            !Row.IsSupportedBuildKind() || Row.ItemDefinitionId.IsEmpty() ||
            Row.VisualId.IsEmpty() || Row.VisualFamily.IsEmpty() ||
            Row.EquipmentSlots.IsEmpty() ||
            (Row.IsPassive() ? !Row.AttackId.IsEmpty() : Row.AttackId.IsEmpty()))
        {
            Error = FString::Printf(TEXT("Invalid identity/state/kind/slots at manifest row %d"),
                Line + 1);
            return false;
        }
        if (!IsSafeObjectPath(Row.ItemPath) || !IsSafeObjectPath(Row.IconPath) ||
            !IsSafeObjectPath(Row.MeshPath) || !IsSafeObjectPath(Row.MaterialPath) ||
            !IsSafeObjectPath(Row.BaseColorPath) || !IsSafeObjectPath(Row.NormalPath) ||
            !IsSafeObjectPath(Row.OrmPath))
        {
            Error = FString::Printf(TEXT("Invalid explicit object path at row %d"), Line + 1);
            return false;
        }
        if (Archetypes.Contains(Row.ArchetypeId) || ItemIds.Contains(Row.ItemDefinitionId) ||
            ItemPaths.Contains(Row.ItemPath))
        {
            Error = FString::Printf(TEXT("Duplicate archetype/item identity/path at row %d"),
                Line + 1);
            return false;
        }
        if ((Row.IsPassive() && Row.EquipmentSlots != TEXT("OffHand")) ||
            (!Row.IsPassive() && !Row.EquipmentSlots.StartsWith(TEXT("MainHand"))) ||
            (Row.IsTwoHanded() && Row.EquipmentSlots != TEXT("MainHand")))
        {
            Error = FString::Printf(TEXT("EquipmentSlots incompatible with BuildKind at row %d"),
                Line + 1);
            return false;
        }
        Archetypes.Add(Row.ArchetypeId);
        ItemIds.Add(Row.ItemDefinitionId);
        ItemPaths.Add(Row.ItemPath);
    }
    if (Out.Num() == 0)
    {
        Error = TEXT("No records in production manifest");
        return false;
    }
    return true;
}
