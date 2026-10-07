#include "UI/RPGTalentPresentationAsset.h"

bool FRPGTalentBranchPresentationDefinition::IsValidDefinition() const
{
	return !TalentBranchId.IsNone() && !DisplayName.IsEmpty();
}

bool FRPGClassPresentationDefinition::IsValidDefinition() const
{
	if (ClassId.IsNone() || Branches.Num() != 3)
	{
		return false;
	}

	TSet<FName> SeenBranchIds;
	for (const FRPGTalentBranchPresentationDefinition& Branch : Branches)
	{
		if (!Branch.IsValidDefinition() || SeenBranchIds.Contains(Branch.TalentBranchId))
		{
			return false;
		}
		SeenBranchIds.Add(Branch.TalentBranchId);
	}

	return true;
}

const FRPGTalentBranchPresentationDefinition* FRPGClassPresentationDefinition::FindBranch(FName TalentBranchId) const
{
	if (TalentBranchId.IsNone())
	{
		return nullptr;
	}

	return Branches.FindByPredicate(
		[TalentBranchId](const FRPGTalentBranchPresentationDefinition& Branch)
		{
			return Branch.TalentBranchId == TalentBranchId;
		});
}

bool URPGTalentPresentationAsset::IsValidDefinition() const
{
	if (Classes.IsEmpty())
	{
		return false;
	}

	TSet<FName> SeenClassIds;
	for (const FRPGClassPresentationDefinition& ClassPresentation : Classes)
	{
		if (!ClassPresentation.IsValidDefinition() || SeenClassIds.Contains(ClassPresentation.ClassId))
		{
			return false;
		}
		SeenClassIds.Add(ClassPresentation.ClassId);
	}

	return true;
}

const FRPGClassPresentationDefinition* URPGTalentPresentationAsset::FindClass(FName ClassId) const
{
	if (ClassId.IsNone())
	{
		return nullptr;
	}

	return Classes.FindByPredicate(
		[ClassId](const FRPGClassPresentationDefinition& ClassPresentation)
		{
			return ClassPresentation.ClassId == ClassId;
		});
}

bool URPGTalentPresentationAsset::GetClassPresentation(FName ClassId, FRPGClassPresentationDefinition& OutDefinition) const
{
	OutDefinition = FRPGClassPresentationDefinition();
	const FRPGClassPresentationDefinition* Found = FindClass(ClassId);
	if (!Found)
	{
		return false;
	}

	OutDefinition = *Found;
	return true;
}

bool URPGTalentPresentationAsset::GetBranchPresentation(
	FName ClassId, FName TalentBranchId, FRPGTalentBranchPresentationDefinition& OutDefinition) const
{
	OutDefinition = FRPGTalentBranchPresentationDefinition();
	const FRPGClassPresentationDefinition* ClassPresentation = FindClass(ClassId);
	if (!ClassPresentation)
	{
		return false;
	}

	const FRPGTalentBranchPresentationDefinition* BranchPresentation = ClassPresentation->FindBranch(TalentBranchId);
	if (!BranchPresentation)
	{
		return false;
	}

	OutDefinition = *BranchPresentation;
	return true;
}
