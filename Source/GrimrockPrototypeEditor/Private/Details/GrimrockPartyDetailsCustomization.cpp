#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

#include <initializer_list>

namespace
{
	void HideProperty(IDetailLayoutBuilder& DetailBuilder, UClass* OwnerClass, FName PropertyName)
	{
		const TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(PropertyName, OwnerClass);
		if (PropertyHandle->IsValidHandle())
		{
			DetailBuilder.HideProperty(PropertyHandle);
		}
	}

	void HideCategories(IDetailLayoutBuilder& DetailBuilder, std::initializer_list<const TCHAR*> CategoryNames)
	{
		for (const TCHAR* CategoryName : CategoryNames)
		{
			DetailBuilder.HideCategory(FName(CategoryName));
		}
	}

	class FGrimrockPartyPawnDetails final : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FGrimrockPartyPawnDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			UClass* PawnClass = AGrimrockPartyPawn::StaticClass();

			// Runtime/debug state does not belong in the authoring surface of
			// BP_GrimrockPartyPawn. Keep the C++ properties and Blueprint access;
			// only remove them from the Details panel.
			const FName RuntimeProperties[] = {
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, SceneRoot),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, SpringArm),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, Camera),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, HeldItemRoot),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, PartyInventoryComponent),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, MenuWidgetInstance),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, bInventoryWidgetVisible),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, CombatHudWidgetInstance),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, CharacterCreationWidgetInstance),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, bCharacterCreationModalActive),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, StoryCompanionRecruitmentWidgetInstance),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, bIsFreeLooking),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, HeldItemActor),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, HeldItemDefinitionId),
				GET_MEMBER_NAME_CHECKED(AGrimrockPartyPawn, bNativeMovementAudioPlaybackEnabled),
			};

			for (const FName PropertyName : RuntimeProperties)
			{
				HideProperty(DetailBuilder, PawnClass, PropertyName);
			}

			// Generic APawn/AActor implementation knobs are intentionally hidden
			// from this authored grid-party Blueprint. Gameplay-facing Grimrock
			// settings remain available in Grid, Movement, Camera, Held Item, Input,
			// UI and RPG categories.
			HideCategories(DetailBuilder,
				{TEXT("Actor"), TEXT("ActorTick"), TEXT("Actor Tick"), TEXT("Pawn"), TEXT("Replication"), TEXT("Rendering"), TEXT("Collision"),
					TEXT("HLOD"), TEXT("Physics"), TEXT("Navigation"), TEXT("Tags"), TEXT("Cooking"), TEXT("Events"), TEXT("LOD"), TEXT("Components")});

			DetailBuilder.EditCategory(TEXT("Grid"), FText::GetEmpty(), ECategoryPriority::Important);
			DetailBuilder.EditCategory(TEXT("Movement"), FText::GetEmpty(), ECategoryPriority::Important);
			DetailBuilder.EditCategory(TEXT("Camera"), FText::GetEmpty(), ECategoryPriority::Important);
			DetailBuilder.EditCategory(TEXT("Held Item"), FText::GetEmpty(), ECategoryPriority::Important);
		}
	};

	class FGridPartyIlluminationDetails final : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FGridPartyIlluminationDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			HideProperty(DetailBuilder, UGridLightEmitterComponent::StaticClass(),
				GET_MEMBER_NAME_CHECKED(UGridLightEmitterComponent, RuntimeConfig));
			HideProperty(DetailBuilder, UGridPartyIlluminationComponent::StaticClass(),
				GET_MEMBER_NAME_CHECKED(UGridPartyIlluminationComponent, ActiveSourceId));

			// The component is an ergonomic proxy. These inherited engine/runtime
			// sections are implementation details, not authored party-light data.
			HideCategories(DetailBuilder,
				{TEXT("Light"), TEXT("Variable"), TEXT("ComponentTick"), TEXT("Component Tick"), TEXT("Rendering"), TEXT("Sockets"), TEXT("Tags"),
					TEXT("ComponentReplication"), TEXT("Component Replication"), TEXT("Activation"), TEXT("Components|Activation"), TEXT("Components|Tick"),
					TEXT("Cooking"), TEXT("Events"), TEXT("Physics"), TEXT("LOD"), TEXT("AssetUserData"), TEXT("Asset User Data"), TEXT("Replication"),
					TEXT("Navigation")});

			DetailBuilder.EditCategory(TEXT("Transform"), FText::GetEmpty(), ECategoryPriority::Important);
			DetailBuilder.EditCategory(TEXT("Party Illumination"), FText::GetEmpty(), ECategoryPriority::Important);
		}
	};
}

namespace GrimrockPartyDetailsCustomization
{
	void Startup()
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.RegisterCustomClassLayout(
			AGrimrockPartyPawn::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FGrimrockPartyPawnDetails::MakeInstance));
		PropertyEditor.RegisterCustomClassLayout(UGridPartyIlluminationComponent::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FGridPartyIlluminationDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	void Shutdown()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}

		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.UnregisterCustomClassLayout(AGrimrockPartyPawn::StaticClass()->GetFName());
		PropertyEditor.UnregisterCustomClassLayout(UGridPartyIlluminationComponent::StaticClass()->GetFName());
		PropertyEditor.NotifyCustomizationModuleChanged();
	}
}
