#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "GridEditorArmamentPreview.h"

namespace
{
	FString FormatWeaponLevel(const FGridResolvedItem& Item)
	{
		const auto& Profile = Item.Weapon;
		const FString Damage = Profile.Kind == EGridCanonicalArmamentKind::Weapon && Profile.DamageDice.bEnabled
			? FString::Printf(TEXT("%dd%d+%d"), Profile.DamageDice.DiceCount,
				Profile.DamageDice.DiceSides, Profile.DamageDice.FlatBonus)
			: TEXT("passif (aucune attaque)");
		return FString::Printf(TEXT("N%d | %s | Armure %d | Protection magique %d | PA %d | Portee %d"),
			Item.ItemLevel, *Damage, Profile.PhysicalArmor, Profile.MagicalArmor,
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

	void AddCatalogueLink(IDetailCategoryBuilder& Category, const FText& Label, UObject* Asset)
	{
		if (!IsValid(Asset)) return;
		const FSoftObjectPath AssetPath(Asset);
		Category.AddCustomRow(Label)
		.NameContent()
		[
			SNew(STextBlock).Text(Label)
		]
		.ValueContent()
		.MinDesiredWidth(420.f)
		[
			SNew(SButton)
			.Text(FText::FromString(Asset->GetName()))
			.OnClicked_Lambda([AssetPath]()
			{
				if (GEditor)
				{
					if (UObject* Target = AssetPath.TryLoad())
					{
						if (UAssetEditorSubsystem* Editor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
							Editor->OpenEditorForAsset(Target);
					}
				}
				return FReply::Handled();
			})
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
			if (Objects.Num() != 1) return;
			const auto* Item = Cast<UGridArmamentDefinitionAsset>(Objects[0].Get());
			if (!IsValid(Item)) return;

			IDetailCategoryBuilder& Overview = Details.EditCategory(
				TEXT("Weapon Quick View"), FText::FromString(TEXT("Arme — résumé (lecture seule)")),
				ECategoryPriority::Important);
			AddReadOnlyLine(Overview, FText::FromString(TEXT("Identité")),
				FText::FromString(FString::Printf(TEXT("%s | %s"), *Item->ArchetypeId.ToString(), *Item->ItemId.ToString())));
			AddReadOnlyLine(Overview, FText::FromString(TEXT("Poids (kg)")),
				FText::AsNumber(Item->WeightKg));
			Details.EditCategory(TEXT("Item"), FText::GetEmpty(), ECategoryPriority::Important);
			Details.EditCategory(TEXT("Armament"), FText::GetEmpty(), ECategoryPriority::Important);

			UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
			FGridResolvedItem N1, N20;
			FString Error;
			if (GridEditorArmamentPreview::Resolve(Root, Item, N1, N20, Error))
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N1")), FText::FromString(FormatWeaponLevel(N1)));
				AddReadOnlyLine(Overview, FText::FromString(TEXT("Profil N20")), FText::FromString(FormatWeaponLevel(N20)));
			}
			else
			{
				AddReadOnlyLine(Overview, FText::FromString(TEXT("État du registre")), FText::FromString(Error));
			}
			if (IsValid(Root))
			{
				AddCatalogueLink(Overview, FText::FromString(TEXT("Registre des objets")), Root);
				AddCatalogueLink(Overview, FText::FromString(TEXT("Dégâts, protection, PA, portée et lancer")), Root->WeaponCatalogue.Get());
				AddCatalogueLink(Overview, FText::FromString(TEXT("Meshes, icônes et visuels")), Root->WeaponVisualCatalogue.Get());
				AddCatalogueLink(Overview, FText::FromString(TEXT("Actions")), Root->ActionCatalogue.Get());
				AddCatalogueLink(Overview, FText::FromString(TEXT("Sons, VFX et animations")), Root->ActionPresentationCatalogue.Get());
			}
		}
	};
}

namespace GrimrockWeaponDetailsCustomization
{
	void Startup()
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.RegisterCustomClassLayout(UGridArmamentDefinitionAsset::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FGridWeaponItemDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	void Shutdown()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor"))) return;
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.UnregisterCustomClassLayout(UGridArmamentDefinitionAsset::StaticClass()->GetFName());
		PropertyEditor.NotifyCustomizationModuleChanged();
	}
}
