#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Inspection/GridItemInspectionReadModel.h"
#include "UObject/StrongObjectPtr.h"

enum class EGridWorkshopFilter : uint8
{
	All, Armaments, Equipment, Other
};

struct FGridWorkshopEntry
{
	FName Id;
	FText Name;
	FName ArchetypeId;
	bool bArmament = false;
	bool bEquipment = false;
	bool bResolved = false;
};

/** Session-only registry projection; no world, physical instance or asset writes. */
class FGridItemWorkshopSession
{
public:
	bool Rebuild();
	bool Select(FName Id);
	bool SelectLevel(int32 Level);
	bool ResolveComparison(int32 Level, FGridItemInspectionView& Out) const;
	void ClearSelection();
	void Filter(const FString& Search, EGridWorkshopFilter Mode, TArray<TSharedPtr<FGridWorkshopEntry>>& Out) const;

	const FGridItemInspectionView& GetView() const { return View; }
	FName GetSelectedId() const { return SelectedId; }
	int32 GetSelectedLevel() const { return SelectedLevel; }
	const TArray<TSharedPtr<FGridWorkshopEntry>>& GetEntries() const { return Entries; }
	const FString& GetError() const { return Error; }

private:
	TStrongObjectPtr<UGameInstance> GameInstance;
	TStrongObjectPtr<UGridItemSubsystem> Registry;
	TArray<TSharedPtr<FGridWorkshopEntry>> Entries;
	FGridItemInspectionView View;
	FName SelectedId = NAME_None;
	int32 SelectedLevel = INDEX_NONE;
	FString Error;
};
