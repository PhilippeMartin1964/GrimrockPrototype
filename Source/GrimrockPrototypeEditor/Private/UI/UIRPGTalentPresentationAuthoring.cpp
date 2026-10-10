#include "UI/UIRPGTalentPresentationAuthoring.h"

#include "UI/RPGTalentPresentationAsset.h"

namespace
{
	FLinearColor Color(float R, float G, float B)
	{
		return FLinearColor(R, G, B, 1.0f);
	}

	FRPGTalentNodePresentationDefinition MakeNodeOverride(
		FName NodeId, const TCHAR* DisplayName, const TCHAR* Description)
	{
		FRPGTalentNodePresentationDefinition Node;
		Node.TalentNodeId = NodeId;
		Node.DisplayName = FText::FromString(DisplayName);
		Node.Description = FText::FromString(Description);
		return Node;
	}

	FRPGTalentBranchPresentationDefinition MakeBranch(
		FName BranchId,
		const TCHAR* DisplayName,
		const FLinearColor& AccentColor,
		TArray<FRPGTalentNodePresentationDefinition> NodeOverrides = {})
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		Branch.DisplayName = FText::FromString(DisplayName);
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
	return TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");
}

void FUIRPGTalentPresentationAuthoring::ConfigureCatalog(URPGTalentPresentationAsset& Catalog)
{
	Catalog.Classes.Reset(6);

	Catalog.Classes.Add(MakeClass(
		TEXT("Warrior"),
		Color(0.42f, 0.07f, 0.05f), Color(0.34f, 0.24f, 0.12f), Color(0.72f, 0.48f, 0.16f), Color(0.86f, 0.62f, 0.24f),
		{
			MakeBranch(TEXT("Guardian"), TEXT("Gardien"), Color(0.72f, 0.52f, 0.18f)),
			MakeBranch(TEXT("Breaker"), TEXT("Brise-ligne"), Color(0.72f, 0.20f, 0.10f)),
			MakeBranch(
				TEXT("WeaponMaster"),
				TEXT("Maître d'armes"),
				Color(0.58f, 0.60f, 0.64f),
				{
					MakeNodeOverride(
						TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"),
						TEXT("Spécialisation martiale"),
						TEXT("Choisissez Tranchant, Perforant ou Contondant. La variante sélectionnée devient la spécialisation martiale du Guerrier."))
				})
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Rogue"),
		Color(0.08f, 0.08f, 0.09f), Color(0.32f, 0.32f, 0.35f), Color(0.34f, 0.12f, 0.46f), Color(0.48f, 0.22f, 0.68f),
		{
			MakeBranch(TEXT("Assassin"), TEXT("Assassin"), Color(0.66f, 0.68f, 0.72f)),
			MakeBranch(TEXT("Shadow"), TEXT("Ombre"), Color(0.38f, 0.18f, 0.52f)),
			MakeBranch(TEXT("Saboteur"), TEXT("Saboteur"), Color(0.48f, 0.34f, 0.20f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Ranger"),
		Color(0.12f, 0.28f, 0.12f), Color(0.34f, 0.22f, 0.10f), Color(0.68f, 0.42f, 0.10f), Color(0.76f, 0.58f, 0.20f),
		{
			MakeBranch(TEXT("Marksman"), TEXT("Tireur"), Color(0.32f, 0.48f, 0.16f)),
			MakeBranch(
				TEXT("Hunter"),
				TEXT("Chasseur"),
				Color(0.68f, 0.22f, 0.08f),
				{
					MakeNodeOverride(
						TEXT("Talent_Ranger_Hunter_FavoredEnemy"),
						TEXT("Ennemi juré"),
						TEXT("Choisissez une catégorie de créatures comme ennemi juré. Les variantes sont mutuellement exclusives."))
				}),
			MakeBranch(TEXT("Scout"), TEXT("Éclaireur"), Color(0.10f, 0.38f, 0.44f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Mage"),
		Color(0.06f, 0.14f, 0.34f), Color(0.06f, 0.46f, 0.62f), Color(0.34f, 0.12f, 0.62f), Color(0.46f, 0.34f, 0.92f),
		{
			MakeBranch(
				TEXT("Evoker"),
				TEXT("Évocateur"),
				Color(0.08f, 0.58f, 0.86f),
				{
					MakeNodeOverride(
						TEXT("Talent_Mage_Evoker_ElementalAffinity"),
						TEXT("Affinité élémentaire"),
						TEXT("Choisissez Feu, Glace, Air ou Terre comme affinité élémentaire."))
				}),
			MakeBranch(TEXT("Arcanist"), TEXT("Arcaniste"), Color(0.50f, 0.16f, 0.72f)),
			MakeBranch(
				TEXT("SurfaceWeaver"),
				TEXT("Tisseur de surfaces"),
				Color(0.28f, 0.38f, 0.82f),
				{
					MakeNodeOverride(
						TEXT("Talent_Mage_SurfaceWeaver_Imbuement"),
						TEXT("Imprégnation"),
						TEXT("Choisissez l'élément utilisé par l'imprégnation. Les variantes partagent le même nœud conceptuel."))
				})
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Priest"),
		Color(0.82f, 0.78f, 0.64f), Color(0.76f, 0.54f, 0.16f), Color(0.42f, 0.58f, 0.72f), Color(0.92f, 0.78f, 0.34f),
		{
			MakeBranch(TEXT("Restoration"), TEXT("Restauration"), Color(0.90f, 0.68f, 0.30f)),
			MakeBranch(TEXT("Protection"), TEXT("Protection"), Color(0.48f, 0.64f, 0.78f)),
			MakeBranch(TEXT("Exorcism"), TEXT("Exorcisme"), Color(0.92f, 0.74f, 0.22f))
		}));

	Catalog.Classes.Add(MakeClass(
		TEXT("Alchemist"),
		Color(0.46f, 0.20f, 0.08f), Color(0.72f, 0.42f, 0.08f), Color(0.12f, 0.48f, 0.20f), Color(0.34f, 0.82f, 0.28f),
		{
			MakeBranch(TEXT("Grenadier"), TEXT("Grenadier"), Color(0.82f, 0.34f, 0.08f)),
			MakeBranch(TEXT("Apothecary"), TEXT("Apothicaire"), Color(0.30f, 0.62f, 0.22f)),
			MakeBranch(TEXT("Transmuter"), TEXT("Transmutateur"), Color(0.70f, 0.54f, 0.12f))
		}));
}
