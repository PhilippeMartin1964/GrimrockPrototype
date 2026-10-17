#include "UI/UIRPGTalentPresentationAuthoring.h"

#include "UI/RPGTalentPresentationAsset.h"

namespace
{
	FLinearColor Color(float R, float G, float B)
	{
		return FLinearColor(R, G, B, 1.0f);
	}

	void SetBootstrapPresentationText(FName StableId, FText& DisplayName, FText& Description)
	{
		DisplayName = FText::FromName(StableId);
		Description = FText::FromString(FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableId.ToString()));
	}

	FRPGTalentNodePresentationDefinition MakeNodeOverride(
		FName NodeId)
	{
		FRPGTalentNodePresentationDefinition Node;
		Node.TalentNodeId = NodeId;
		return Node;
	}

	FRPGTalentBranchPresentationDefinition MakeBranch(
		FName BranchId,
		const FLinearColor& AccentColor,
		TArray<FRPGTalentNodePresentationDefinition> NodeOverrides = {})
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		Branch.AccentColor = AccentColor;
		Branch.NodePresentationOverrides = MoveTemp(NodeOverrides);
		return Branch;
	}

	FRPGClassPresentationDefinition MakeClass(
		FName ClassId,
		const FLinearColor& Primary,
		const FLinearColor& Secondary,
		const FLinearColor& Accent,
		const FLinearColor& Glow,
		TArray<FRPGTalentBranchPresentationDefinition> Branches)
	{
		FRPGClassPresentationDefinition ClassPresentation;
		ClassPresentation.ClassId = ClassId;
		ClassPresentation.PrimaryColor = Primary;
		ClassPresentation.SecondaryColor = Secondary;
		ClassPresentation.AccentColor = Accent;
		ClassPresentation.GlowColor = Glow;
		ClassPresentation.Branches = MoveTemp(Branches);
		return ClassPresentation;
	}
}

const TCHAR* FUIRPGTalentPresentationAuthoring::PackageName()
{
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation");
}

const TCHAR* FUIRPGTalentPresentationAuthoring::AssetName()
{
	return TEXT("DA_RPGTalentPresentation");
}

const TCHAR* FUIRPGTalentPresentationAuthoring::ObjectPath()
{
	return URPGTalentPresentationAsset::CanonicalObjectPath();
}

void FUIRPGTalentPresentationAuthoring::ConfigureCatalog(URPGTalentPresentationAsset& Catalog)
{
	const TArray<FRPGClassPresentationDefinition> PreviousClasses = Catalog.Classes;
	Catalog.Classes.Reset(6);

	Catalog.Classes.Add(MakeClass(
		TEXT("Warrior"),
		Color(0.42f, 0.07f, 0.05f), Color(0.34f, 0.24f, 0.12f), Color(0.72f, 0.48f, 0.16f), Color(0.86f, 0.62f, 0.24f),
		{
			MakeBranch(TEXT("Guardian"), Color(0.72f, 0.52f, 0.18f)),
			MakeBranch(TEXT("Breaker"), Color(0.72f, 0.20f, 0.10f)),
			MakeBranch(
				TEXT("WeaponMaster"),
				Color(0.58f, 0.60f, 0.64f),
				{
					MakeNodeOverride(
						TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"))
				})
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Rogue"),
		Color(0.08f, 0.08f, 0.09f), Color(0.32f, 0.32f, 0.35f), Color(0.34f, 0.12f, 0.46f), Color(0.48f, 0.22f, 0.68f),
		{
			MakeBranch(TEXT("Assassin"), Color(0.66f, 0.68f, 0.72f)),
			MakeBranch(TEXT("Shadow"), Color(0.38f, 0.18f, 0.52f)),
			MakeBranch(TEXT("Saboteur"), Color(0.48f, 0.34f, 0.20f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Ranger"),
		Color(0.12f, 0.28f, 0.12f), Color(0.34f, 0.22f, 0.10f), Color(0.68f, 0.42f, 0.10f), Color(0.76f, 0.58f, 0.20f),
		{
			MakeBranch(TEXT("Marksman"), Color(0.32f, 0.48f, 0.16f)),
			MakeBranch(
				TEXT("Hunter"),
				Color(0.68f, 0.22f, 0.08f),
				{
					MakeNodeOverride(
						TEXT("Talent_Ranger_Hunter_FavoredEnemy"))
				}),
			MakeBranch(TEXT("Scout"), Color(0.10f, 0.38f, 0.44f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Mage"),
		Color(0.06f, 0.14f, 0.34f), Color(0.06f, 0.46f, 0.62f), Color(0.34f, 0.12f, 0.62f), Color(0.46f, 0.34f, 0.92f),
		{
			MakeBranch(
				TEXT("Evoker"),
				Color(0.08f, 0.58f, 0.86f),
				{
					MakeNodeOverride(
						TEXT("Talent_Mage_Evoker_ElementalAffinity"))
				}),
			MakeBranch(TEXT("Arcanist"), Color(0.50f, 0.16f, 0.72f)),
			MakeBranch(
				TEXT("SurfaceWeaver"),
				Color(0.28f, 0.38f, 0.82f),
				{
					MakeNodeOverride(
						TEXT("Talent_Mage_SurfaceWeaver_Imbuement"))
				})
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Priest"),
		Color(0.82f, 0.78f, 0.64f), Color(0.76f, 0.54f, 0.16f), Color(0.42f, 0.58f, 0.72f), Color(0.92f, 0.78f, 0.34f),
		{
			MakeBranch(TEXT("Restoration"), Color(0.90f, 0.68f, 0.30f)),
			MakeBranch(TEXT("Protection"), Color(0.48f, 0.64f, 0.78f)),
			MakeBranch(TEXT("Exorcism"), Color(0.92f, 0.74f, 0.22f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Alchemist"),
		Color(0.46f, 0.20f, 0.08f), Color(0.72f, 0.42f, 0.08f), Color(0.12f, 0.48f, 0.20f), Color(0.34f, 0.82f, 0.28f),
		{
			MakeBranch(TEXT("Grenadier"), Color(0.82f, 0.34f, 0.08f)),
			MakeBranch(TEXT("Apothecary"), Color(0.30f, 0.62f, 0.22f)),
			MakeBranch(TEXT("Transmuter"), Color(0.70f, 0.54f, 0.12f))
		}));

	for (FRPGClassPresentationDefinition& ClassPresentation : Catalog.Classes)
	{
		const FRPGClassPresentationDefinition* PreviousClass = PreviousClasses.FindByPredicate(
			[&ClassPresentation](const FRPGClassPresentationDefinition& Candidate)
			{
				return Candidate.ClassId == ClassPresentation.ClassId;
			});

		for (FRPGTalentBranchPresentationDefinition& Branch : ClassPresentation.Branches)
		{
			const FRPGTalentBranchPresentationDefinition* PreviousBranch =
				PreviousClass ? PreviousClass->FindBranch(Branch.TalentBranchId) : nullptr;
			if (PreviousBranch)
			{
				Branch.DisplayName = PreviousBranch->DisplayName;
				Branch.ShortDescription = PreviousBranch->ShortDescription;
			}
			else
			{
				SetBootstrapPresentationText(Branch.TalentBranchId, Branch.DisplayName, Branch.ShortDescription);
			}

			for (FRPGTalentNodePresentationDefinition& Node : Branch.NodePresentationOverrides)
			{
				const FRPGTalentNodePresentationDefinition* PreviousNode =
					PreviousBranch ? PreviousBranch->FindNodeOverride(Node.TalentNodeId) : nullptr;
				if (PreviousNode)
				{
					Node.DisplayName = PreviousNode->DisplayName;
					Node.Description = PreviousNode->Description;
				}
				else
				{
					SetBootstrapPresentationText(Node.TalentNodeId, Node.DisplayName, Node.Description);
				}
			}
		}
	}
}
