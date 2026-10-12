#include "UI/RPGTalentDetailTextSet.h"

bool FRPGTalentDetailTextEntry::IsValidDefinition() const
{
	return !TextId.IsNone() && !Text.IsEmpty();
}

bool URPGTalentDetailTextSet::IsValidDefinition() const
{
	if (Entries.IsEmpty())
	{
		return false;
	}

	TSet<FName> SeenIds;
	for (const FRPGTalentDetailTextEntry& Entry : Entries)
	{
		if (!Entry.IsValidDefinition() || SeenIds.Contains(Entry.TextId))
		{
			return false;
		}
		SeenIds.Add(Entry.TextId);
	}

	return true;
}

FText URPGTalentDetailTextSet::ResolveText(FName TextId) const
{
	if (TextId.IsNone())
	{
		return FText::GetEmpty();
	}

	const FRPGTalentDetailTextEntry* Found = Entries.FindByPredicate(
		[TextId](const FRPGTalentDetailTextEntry& Entry)
		{
			return Entry.TextId == TextId;
		});
	return Found ? Found->Text : FText::GetEmpty();
}


namespace
{
	const TCHAR* ProductionTalentDetailTextSetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet");
}

const URPGTalentDetailTextSet* URPGTalentDetailTextSet::LoadProduction()
{
	return LoadObject<URPGTalentDetailTextSet>(nullptr, ProductionTalentDetailTextSetPath);
}

FText URPGTalentDetailTextSet::ResolveProductionText(FName TextId)
{
	const URPGTalentDetailTextSet* TextSet = LoadProduction();
	return TextSet ? TextSet->ResolveText(TextId) : FText::GetEmpty();
}
