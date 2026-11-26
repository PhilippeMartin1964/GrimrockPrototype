#include "GridItemWorkshopSession.h"

bool FGridItemWorkshopSession::Rebuild()
{
	const FName PreviousId = SelectedId;
	const int32 PreviousLevel = SelectedLevel;
	View = FGridItemInspectionView();
	Entries.Reset();
	Error.Reset();
	Registry.Reset();
	GameInstance.Reset();

	UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
	if (!IsValid(Root))
	{
		Error = TEXT("DA_ItemSystem indisponible.");
		return false;
	}

	GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackage(), NAME_None, RF_Transient));
	Registry.Reset(NewObject<UGridItemSubsystem>(GameInstance.Get(), NAME_None, RF_Transient));
	TArray<FString> Errors;
	if (!Registry->RegisterItemSystem(Root, Errors))
	{
		Error = FString::Join(Errors, TEXT("\n"));
		if (Error.IsEmpty())
		{
			Error = TEXT("Registre canonique invalide.");
		}
		Registry.Reset();
		return false;
	}

	TArray<FName> Ids;
	Registry->GetItemIds(Ids);
	for (FName Id : Ids)
	{
		TSharedPtr<FGridWorkshopEntry> Entry = MakeShared<FGridWorkshopEntry>();
		Entry->Id = Id;
		int32 DefaultLevel = INDEX_NONE;
		Entry->bArmament = Registry->TryGetDefaultItemLevel(Id, DefaultLevel) && DefaultLevel == 1;
		FGridItemInspectionView Default;
		Entry->bResolved = FGridItemInspectionReadModel::ResolveDefault(Registry.Get(), Id, Default);
		Entry->Name = Entry->bResolved ? Default.Item.DisplayName : FText::FromName(Id);
		Entry->ArchetypeId = Default.Item.ArchetypeId;
		Entry->bEquipment = Default.HasEquipment() && !Entry->bArmament;
		Entries.Add(Entry);
	}

	if (!PreviousId.IsNone() && Registry->ContainsItem(PreviousId))
	{
		Select(PreviousId);
		if (View.HasLevel() && PreviousLevel >= 1 && PreviousLevel <= 20)
		{
			SelectLevel(PreviousLevel);
		}
	}
	else
	{
		ClearSelection();
	}
	return true;
}

bool FGridItemWorkshopSession::Select(FName Id)
{
	ClearSelection();
	SelectedId = Id;
	const bool bResolved = FGridItemInspectionReadModel::ResolveDefault(Registry.Get(), Id, View);
	SelectedLevel = bResolved ? View.Item.ItemLevel : INDEX_NONE;
	return bResolved;
}

bool FGridItemWorkshopSession::SelectLevel(int32 Level)
{
	if (!View.HasLevel() || SelectedId.IsNone() || Level < 1 || Level > 20)
	{
		return false;
	}
	SelectedLevel = Level;
	return FGridItemInspectionReadModel::ResolveExact(Registry.Get(), SelectedId, Level, View);
}

void FGridItemWorkshopSession::ClearSelection()
{
	SelectedId = NAME_None;
	SelectedLevel = INDEX_NONE;
	View = FGridItemInspectionView();
}

void FGridItemWorkshopSession::Filter(const FString& Search, EGridWorkshopFilter Mode,
	TArray<TSharedPtr<FGridWorkshopEntry>>& Out) const
{
	Out.Reset();
	const FString Query = Search.TrimStartAndEnd();
	for (const TSharedPtr<FGridWorkshopEntry>& Entry : Entries)
	{
		const bool bCategoryMatches = Mode == EGridWorkshopFilter::All ||
			(Mode == EGridWorkshopFilter::Armaments && Entry->bArmament) ||
			(Mode == EGridWorkshopFilter::Equipment && Entry->bEquipment) ||
			(Mode == EGridWorkshopFilter::Other && !Entry->bArmament && !Entry->bEquipment);
		const bool bSearchMatches = Query.IsEmpty() || Entry->Id.ToString().Contains(Query) ||
			Entry->Name.ToString().Contains(Query) || Entry->ArchetypeId.ToString().Contains(Query);
		if (bCategoryMatches && bSearchMatches)
		{
			Out.Add(Entry);
		}
	}
}
