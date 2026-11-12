#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString FormatWeaponLevel(const FGridWeaponLevelProfile& Profile)
	{
		const FString Damage = Profile.DamageDice.bEnabled
			? FString::Printf(TEXT("%dd%d+%d"), Profile.DamageDice.DiceCount,
				Profile.DamageDice.DiceSides, Profile.DamageDice.FlatBonus)
			: TEXT("passif (aucune attaque)");
		return FString::Printf(TEXT("N%d | %s | Armure %d | Protection magique %d | PA %d | Portee %d"),
			Profile.ItemLevel, *Damage, Profile.PhysicalArmor, Profile.MagicalArmor,
			Profile.ActionPointCost, Profile.RangeCells);
	}

	void AddReadOnlyLine(IDetailCategoryBuilder& Category, const FText& Label, const FText& Value)
	{
		Category.AddCustomRow(Label)
		.NameContent()
		[
			SNew(STextBlock).Text(Label)
		]
		.ValueContent()
		.MinDesiredWidth(420.f)
		[
			SNew(STextBlock).Text(Value).AutoWrapText(true)
		];
	}

	class FGridWeaponItemDetails final : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FGridWeaponItemDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& Details) override
		{
			TArray<TWeakObjectPtr<UObject>> Objects;
			Details.GetObjectsBeingCustomized(Objects);
			if (Objects.Num() != 1)
			{
				return;
			}
			const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Objects[0].Get());
			if (!IsValid(Item) || !Item->bUseWeaponCatalogue)
			{
				return; // Non-weapon and Legacy item editors remain untouched.
			}

			IDetailCategoryBuilder& Overview = Details.EditCategory(
				TEXT("Weapon Quick View"), FText::FromString(TEXT("Arme — résumé (lecture seule)")),
				ECategoryPriority::Important);
			AddReadOnlyLine(Overview, FText::FromString(TEXT("Identité")),
				FText::FromString(FString::Printf(TEXT("%s | niveau par défaut %d | item %s"),
					*Item->WeaponArchetypeId.ToString(), Item->DefaultWeaponItemLevel,
					*Item->ItemDefinitionId.ToString())));

			const FGridWeaponLevelProfile* N1 = nullptr;
			const FGridWeaponLevelProfile* N20 = nullptr;
			if (Item->TryGetWeaponCatalogueProfile(1, N1) && N1)
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N1")),
					FText::FromString(FormatWeaponLevel(*N1)));
			}
			else
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N1")),
					FText::FromString(TEXT("ERREUR : catalogue ou profil indisponible")));
			}
			if (Item->TryGetWeaponCatalogueProfile(20, N20) && N20)
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N20")),
					FText::FromString(FormatWeaponLevel(*N20)));
			}
			else
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N20")),
					FText::FromString(TEXT("ERREUR : catalogue ou profil indisponible")));
			}
			AddReadOnlyLine(Overview, FText::FromString(TEXT("Où modifier ?")),
				FText::FromString(TEXT("Dégâts/protection/PA : DA_WeaponCatalogue. Sons/VFX/animations : PlayerAttackPresentationProfile. Les anciennes copies N1 de CombatActions et EquipmentStatBonus ne sont pas l'autorité des statistiques.")));

			Details.EditCategory(TEXT("Item"), FText::GetEmpty(), ECategoryPriority::Important);
			Details.EditCategory(TEXT("Equipment|Weapon Catalogue"), FText::GetEmpty(), ECategoryPriority::Important);
			Details.EditCategory(TEXT("Equipment|Offense|Presentation"), FText::GetEmpty(), ECategoryPriority::Important);
			Details.EditCategory(TEXT("Visual|Niagara"), FText::GetEmpty(), ECategoryPriority::Important);
			Details.EditCategory(TEXT("Visual|Weapon Catalogue"), FText::GetEmpty(), ECategoryPriority::Important);

			// Keep rare options accessible, but below the normal weapon workflow.
			// No UPROPERTY is removed or changed: existing .uasset remain intact.
			Details.EditCategory(TEXT("Equipment|Combat Actions"), FText::GetEmpty(), ECategoryPriority::Uncommon);
			Details.EditCategory(TEXT("Equipment|Stats"), FText::GetEmpty(), ECategoryPriority::Uncommon);
			Details.EditCategory(TEXT("Equipment|Resistances"), FText::GetEmpty(), ECategoryPriority::Uncommon);
			Details.EditCategory(TEXT("Throw"), FText::GetEmpty(), ECategoryPriority::Uncommon);

			if (Item->ReadText.IsEmpty())
			{
				Details.HideCategory(TEXT("Reading"));
			}
			if (!Item->bProvidesQuickItemCombatAction)
			{
				Details.HideCategory(TEXT("Combat|Quick Item"));
			}
		}
	};
}

namespace GrimrockWeaponDetailsCustomization
{
	void Startup()
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.RegisterCustomClassLayout(UGridItemDefinitionAsset::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FGridWeaponItemDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	void Shutdown()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.UnregisterCustomClassLayout(UGridItemDefinitionAsset::StaticClass()->GetFName());
		PropertyEditor.NotifyCustomizationModuleChanged();
	}
}
