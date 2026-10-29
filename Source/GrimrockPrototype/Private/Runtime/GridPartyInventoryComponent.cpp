#include "Runtime/GridPartyInventoryComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#include "GridEquipmentSlotUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "UI/UILocalizationText.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGAttributePointService.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGRaceAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridInventory, Log, All);

namespace
{
	int32 CountOccupiedSlots(const FGridCharacterInventoryState& CharacterState)
	{
		int32 OccupiedCount = 0;
		for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
		{
			if (!Slot.IsEmpty())
			{
				++OccupiedCount;
			}
		}
		return OccupiedCount;
	}

	float GetItemTotalWeight(const FGridItemInstance& Item)
	{
		return Item.IsValid() ? Item.Weight * FMath::Max(1, Item.Quantity) : 0.0f;
	}

	constexpr float InventoryHeavyWeightRatio = 0.8f;

	float ResolvePartyPortraitRatio(int32 CurrentValue, int32 ReferenceValue)
	{
		if (ReferenceValue <= 0)
		{
			return 0.0f;
		}
		return FMath::Clamp(static_cast<float>(FMath::Max(0, CurrentValue)) / static_cast<float>(ReferenceValue), 0.0f, 1.0f);
	}

	URPGClassAsset* ResolvePartyPortraitClassDefinition(const FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
		}
		return FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition) ? ClassDefinition : nullptr;
	}

	/** Read-only query against canonical progression balances; invalid balances never invent points. */
	bool HasUnspentPartyPortraitProgressionPoints(const FGridCharacterInventoryState& Character)
	{
		FRPGSkillPointBalance SkillBalance;
		if (FRPGSkillPointService::TryGetBalance(Character, SkillBalance) && SkillBalance.RemainingPoints > 0)
		{
			return true;
		}

		FRPGAttributePointBalance AttributeBalance;
		if (FRPGAttributePointService::TryGetBalance(Character, AttributeBalance) && AttributeBalance.RemainingPoints > 0)
		{
			return true;
		}

		const URPGClassAsset* ClassDefinition = ResolvePartyPortraitClassDefinition(Character);
		if (!ClassDefinition)
		{
			return false;
		}

		TSet<FName> SelectedChoices;
		for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
		{
			if (ChoiceId.IsNone() || SelectedChoices.Contains(ChoiceId))
			{
				return false;
			}
			SelectedChoices.Add(ChoiceId);
		}

		int32 GrantedPoints = 0;
		int32 SpentPoints = 0;
		int32 RemainingPoints = 0;
		return FRPGClassProgressionService::TryGetChoicePointBalance(
			ClassDefinition, Character.Level, SelectedChoices, GrantedPoints, SpentPoints, RemainingPoints) &&
			RemainingPoints > 0;
	}

	void ResolvePartyPortraitArmorReferences(
		const FGridCharacterInventoryState& Character,
		const FGridInventoryCharacterSummary& Summary,
		int32& OutPhysicalArmor,
		int32& OutMagicalArmor)
	{
		OutPhysicalArmor = 0;
		OutMagicalArmor = 0;

		URPGClassAsset* ClassDefinition = ResolvePartyPortraitClassDefinition(Character);
		if (!ClassDefinition)
		{
			return;
		}

		FGridCombatArmorPoolSnapshot Snapshot;
		Snapshot.ReferencePhysicalArmor =
			FMath::Max(0, ClassDefinition->BasePhysicalArmor + Summary.EquipmentStatBonus.ArmorBonus);
		Snapshot.ReferenceMagicalArmor = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(ClassDefinition->BaseMagicalArmor) +
			Summary.EquipmentStatBonus.MagicalArmorBonus, 0, MAX_int32));

		// Identity caches are normally rehydrated before UI projection. Keep this
		// helper robust when only ClassId can resolve the canonical class asset.
		FGridCharacterInventoryState ModifierCharacter = Character;
		ModifierCharacter.ClassDefinition = ClassDefinition;

		TArray<FGridCombatModifierProfile> Profiles;
		if (FGridCombatModifierResolver::CollectCharacterModifiers(ModifierCharacter, Profiles))
		{
			FGridResolvedCombatModifiers Modifiers;
			FGridCombatModifierResolver::Resolve(Profiles, FGridCombatModifierContext(), Modifiers);
			FGridCombatArmorEffectResolver::ApplyReferenceModifiers(Snapshot, Modifiers);
		}

		OutPhysicalArmor = FMath::Max(0, Snapshot.ReferencePhysicalArmor);
		OutMagicalArmor = FMath::Max(0, Snapshot.ReferenceMagicalArmor);
	}

	EGridInventoryWeightState ResolveInventoryWeightState(float CurrentWeight, float MaxWeight)
	{
		if (CurrentWeight > MaxWeight)
		{
			return EGridInventoryWeightState::Overloaded;
		}
		if (MaxWeight > 0.0f && CurrentWeight >= MaxWeight * InventoryHeavyWeightRatio)
		{
			return EGridInventoryWeightState::Heavy;
		}
		return EGridInventoryWeightState::Normal;
	}

	bool HaveMatchingReadableContent(const FGridItemInstance& ExistingItem, const FGridItemInstance& IncomingItem)
	{
		return ExistingItem.ReadableContentAsset == IncomingItem.ReadableContentAsset && ExistingItem.ReadableContentId == IncomingItem.ReadableContentId &&
			ExistingItem.ReadTitleOverride.EqualTo(IncomingItem.ReadTitleOverride) && ExistingItem.ReadTextOverride.EqualTo(IncomingItem.ReadTextOverride);
	}

	const TCHAR* GetOwnerTypeName(EGridItemOwnerType OwnerType)
	{
		switch (OwnerType)
		{
			case EGridItemOwnerType::World:
				return TEXT("World");
			case EGridItemOwnerType::Receptacle:
				return TEXT("Receptacle");
			case EGridItemOwnerType::CharacterInventory:
				return TEXT("CharacterInventory");
			case EGridItemOwnerType::EquipmentSlot:
				return TEXT("EquipmentSlot");
			case EGridItemOwnerType::Cursor:
				return TEXT("Cursor");
			case EGridItemOwnerType::HeldBySelectedCharacter:
				return TEXT("HeldBySelectedCharacter");
			case EGridItemOwnerType::Removed:
				return TEXT("Removed");
			default:
				return TEXT("None");
		}
	}

	bool CharacterHasInventoryItemDefinition(const FGridCharacterInventoryState& CharacterState, FName ItemDefinitionId)
	{
		return !ItemDefinitionId.IsNone() &&
			CharacterState.InventorySlots.ContainsByPredicate(
				[ItemDefinitionId](const FGridInventorySlot& Slot)
				{
					return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemDefinitionId && Slot.Item.Quantity > 0;
				});
	}

	void ClearQuickItemHotbarBindings(FGridCharacterInventoryState& CharacterState, FName ItemDefinitionId)
	{
		for (int32 SlotIndex = 0; SlotIndex < CharacterState.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			FGridCombatHotbarBinding& Binding = CharacterState.CombatHotbarSlots[SlotIndex];
			if (Binding.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem || Binding.SourceDefinitionId != ItemDefinitionId)
			{
				continue;
			}

			Binding.Reset(SlotIndex);
		}
	}

	void EnsurePrimaryAttackHotbarBinding(FGridCharacterInventoryState& CharacterState)
	{
		if (CharacterState.CombatHotbarSlots.Num() < FGridCombatHotbarBinding::MinimumSlotCount)
		{
			return;
		}

		FGridCombatHotbarBinding& PrimarySlot = CharacterState.CombatHotbarSlots[FGridCombatHotbarBinding::PrimaryAttackSlotIndex];
		if (!PrimarySlot.IsPrimaryAttackBinding() && !PrimarySlot.IsEmpty() && PrimarySlot.IsValid())
		{
			for (int32 TargetSlotIndex = 1; TargetSlotIndex < CharacterState.CombatHotbarSlots.Num(); ++TargetSlotIndex)
			{
				if (!CharacterState.CombatHotbarSlots[TargetSlotIndex].IsEmpty())
				{
					continue;
				}
				FGridCombatHotbarBinding DisplacedBinding = PrimarySlot;
				DisplacedBinding.SlotIndex = TargetSlotIndex;
				CharacterState.CombatHotbarSlots[TargetSlotIndex] = MoveTemp(DisplacedBinding);
				break;
			}
		}

		PrimarySlot.Reset(FGridCombatHotbarBinding::PrimaryAttackSlotIndex);
		PrimarySlot.ActionId = FGridCombatHotbarBinding::MakePrimaryAttackActionId();
		PrimarySlot.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	}

	void SanitizeCombatHotbarBindings(FGridCharacterInventoryState& CharacterState)
	{
		EnsurePrimaryAttackHotbarBinding(CharacterState);
		TSet<FGuid> AssignedEquipmentRuntimeIds;
		TSet<FName> AssignedQuickItemDefinitionIds;
		for (int32 SlotIndex = 0; SlotIndex < CharacterState.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			FGridCombatHotbarBinding& Binding = CharacterState.CombatHotbarSlots[SlotIndex];
			if (!Binding.IsValid() || Binding.IsEmpty())
			{
				continue;
			}
			if (SlotIndex != FGridCombatHotbarBinding::PrimaryAttackSlotIndex && Binding.IsPrimaryAttackBinding())
			{
				Binding.Reset(SlotIndex);
				continue;
			}

			if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
			{
				if (AssignedEquipmentRuntimeIds.Contains(Binding.PreferredSourceRuntimeId))
				{
					Binding.Reset(SlotIndex);
				}
				else
				{
					AssignedEquipmentRuntimeIds.Add(Binding.PreferredSourceRuntimeId);
				}
			}
			else if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
			{
				if (!CharacterHasInventoryItemDefinition(CharacterState, Binding.SourceDefinitionId) ||
					AssignedQuickItemDefinitionIds.Contains(Binding.SourceDefinitionId))
				{
					Binding.Reset(SlotIndex);
				}
				else
				{
					AssignedQuickItemDefinitionIds.Add(Binding.SourceDefinitionId);
				}
			}
		}
	}
}

UGridPartyInventoryComponent::UGridPartyInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PartyInventoryState.MaxActiveCharacters = DefaultMaxActiveCharacters;
}

void UGridPartyInventoryComponent::NotifyPartyInventoryChanged(int32 CharacterIndex)
{
	SanitizePartyPortraitOrder();
	OnPartyInventoryChanged.Broadcast(CharacterIndex);
}

void UGridPartyInventoryComponent::InitializeDefaultPartyIfNeeded()
{
	PartyInventoryState.MaxActiveCharacters = FMath::Max(1, DefaultMaxActiveCharacters);

	if (PartyInventoryState.ActiveCharacters.Num() == 0)
	{
		FGridCharacterInventoryState DefaultCharacter;
		InitializeCharacterDefaults(DefaultCharacter, 0);
		PartyInventoryState.ActiveCharacters.Add(DefaultCharacter);
	}

	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		InitializeCharacterDefaults(PartyInventoryState.ActiveCharacters[CharacterIndex], CharacterIndex);
	}

	for (FGridCharacterInventoryState& CharacterState : PartyInventoryState.CharacterPool)
	{
		InitializeCombatHotbarDefaults(CharacterState);
	}

	EnsureEquipmentCountMatchesActiveCharacters();

	if (!IsValidCharacterIndex(PartyInventoryState.SelectedCharacterIndex))
	{
		PartyInventoryState.SelectedCharacterIndex = PartyInventoryState.ActiveCharacters.Num() > 0 ? 0 : INDEX_NONE;
	}

	NotifyPartyInventoryChanged(INDEX_NONE);
}

bool UGridPartyInventoryComponent::HasCompletedInitialCharacterCreation() const
{
	return PartyInventoryState.bInitialCharacterCreationCompleted;
}

void UGridPartyInventoryComponent::ResetPartyForNewGame()
{
	PartyPortraitOrder.Reset();
	PartyInventoryState = FGridPartyInventoryState();
	InitializeDefaultPartyIfNeeded();
}

bool UGridPartyInventoryComponent::RestorePartyInventoryState(const FGridPartyInventoryState& SavedState, FText& OutError)
{
	OutError = FText::GetEmpty();

	if (!SavedState.bInitialCharacterCreationCompleted)
	{
		OutError = FText::FromString(TEXT("La sauvegarde ne contient aucun personnage finalisé."));
		return false;
	}

	if (SavedState.ActiveCharacters.Num() < 1 || SavedState.MaxActiveCharacters < SavedState.ActiveCharacters.Num())
	{
		OutError = FText::FromString(TEXT("Le groupe sauvegardé possède un nombre de personnages invalide."));
		return false;
	}

	if (SavedState.ActiveEquipment.Num() != SavedState.ActiveCharacters.Num())
	{
		OutError = FText::FromString(TEXT("Les personnages et leurs équipements sauvegardés ne sont pas alignés."));
		return false;
	}

	if (!SavedState.ActiveCharacters.IsValidIndex(SavedState.SelectedCharacterIndex))
	{
		OutError = FText::FromString(TEXT("Le personnage sélectionné dans la sauvegarde est invalide."));
		return false;
	}

	FGridPartyInventoryState RestoredState = SavedState;
	const int32 ConfiguredInventorySlotCount = FMath::Max(1, DefaultInventorySlotCountPerCharacter);
	auto ApplyConfiguredInventorySlotCount =
		[ConfiguredInventorySlotCount, &OutError](FGridCharacterInventoryState& Character, const TCHAR* CharacterKind, int32 CharacterIndex)
		{
			for (int32 SlotIndex = ConfiguredInventorySlotCount; SlotIndex < Character.InventorySlots.Num(); ++SlotIndex)
			{
				if (!Character.InventorySlots[SlotIndex].IsEmpty())
				{
					OutError = FText::FromString(FString::Printf(
						TEXT("%s %d utilise le slot %d, au-delà de la capacité configurée de %d slots."),
						CharacterKind, CharacterIndex, SlotIndex, ConfiguredInventorySlotCount));
					return false;
				}
			}

			Character.InventorySlots.SetNum(ConfiguredInventorySlotCount);
			return true;
		};

	for (int32 CharacterIndex = 0; CharacterIndex < RestoredState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		FGridCharacterInventoryState& Character = RestoredState.ActiveCharacters[CharacterIndex];
		if (!ApplyConfiguredInventorySlotCount(Character, TEXT("Personnage"), CharacterIndex))
		{
			return false;
		}
		if (!Character.CharacterId.IsValid())
		{
			OutError = FText::FromString(TEXT("Un personnage sauvegardé ne possède pas d'identifiant valide."));
			return false;
		}

		SanitizeCombatHotbarBindings(Character);

		FString HotbarError;
		if (!ValidateCombatHotbar(Character, HotbarError))
		{
			OutError = FText::FromString(FString::Printf(TEXT("La barre de raccourcis d'un personnage sauvegardé est invalide : %s"), *HotbarError));
			return false;
		}
	}

	for (int32 PoolIndex = 0; PoolIndex < RestoredState.CharacterPool.Num(); ++PoolIndex)
	{
		FGridCharacterInventoryState& Character = RestoredState.CharacterPool[PoolIndex];
		if (!ApplyConfiguredInventorySlotCount(Character, TEXT("Personnage en réserve"), PoolIndex))
		{
			return false;
		}

		SanitizeCombatHotbarBindings(Character);

		FString HotbarError;
		if (!ValidateCombatHotbar(Character, HotbarError))
		{
			OutError = FText::FromString(FString::Printf(TEXT("La barre de raccourcis d'un personnage en réserve est invalide : %s"), *HotbarError));
			return false;
		}
	}

	const FGridPartyInventoryState PreviousState = PartyInventoryState;
	const TArray<FGuid> PreviousPortraitOrder = PartyPortraitOrder;
	PartyInventoryState = MoveTemp(RestoredState);
	NotifyPartyInventoryChanged(INDEX_NONE);

	FString OwnershipError;
	if (!ValidateInventoryOwnership(OwnershipError))
	{
		PartyInventoryState = PreviousState;
		PartyPortraitOrder = PreviousPortraitOrder;
		NotifyPartyInventoryChanged(INDEX_NONE);
		OutError = FText::FromString(FString::Printf(TEXT("L'ownership de la sauvegarde est invalide : %s"), *OwnershipError));
		return false;
	}

	return true;
}

bool UGridPartyInventoryComponent::CreateInitialCharacter(const FRPGCharacterCreationRequest& Request, FText& OutError)
{
	OutError = FText::GetEmpty();

	if (HasCompletedInitialCharacterCreation())
	{
		OutError = FText::FromString(TEXT("La création initiale du personnage est déjà terminée."));
		return false;
	}

	FString NormalizedName = Request.DisplayName.ToString();
	NormalizedName.TrimStartAndEndInline();
	if (NormalizedName.Len() < 1 || NormalizedName.Len() > 24)
	{
		OutError = FText::FromString(TEXT("Le nom du personnage doit contenir entre 1 et 24 caractères."));
		return false;
	}

	if (!Request.RaceDefinition || !Request.RaceDefinition->IsValidDefinition())
	{
		OutError = FText::FromString(TEXT("Une définition de race valide est requise."));
		return false;
	}

	if (!Request.ClassDefinition || !Request.ClassDefinition->IsValidDefinition())
	{
		OutError = FText::FromString(TEXT("Une définition de classe valide est requise."));
		return false;
	}

	URPGClassAsset* CombatActionSourceClass =
		Request.CombatActionSourceClassDefinition ? Request.CombatActionSourceClassDefinition.Get() : Request.ClassDefinition.Get();
	if (!CombatActionSourceClass || !CombatActionSourceClass->IsValidDefinition() || CombatActionSourceClass->ClassId != Request.ClassDefinition->ClassId)
	{
		OutError = FText::FromString(TEXT("La source des actions de classe est invalide."));
		return false;
	}

	FRPGAuthoringIdentityResolver::RememberRaceDefinition(Request.RaceDefinition);
	FRPGAuthoringIdentityResolver::RememberClassDefinition(CombatActionSourceClass);
	FRPGAuthoringIdentityResolver::RememberPortraitVisual(Request.RaceDefinition->RaceId, Request.PortraitGender, Request.PortraitVariantId, Request.Portrait);
	FRPGAuthoringIdentityResolver::RememberClassIcon(Request.ClassDefinition->ClassId, Request.ClassIcon);

	const FRPGAttributes FinalAttributes =
		URPGCharacterRulesLibrary::AddAttributes(Request.ClassDefinition->BaseAttributes, Request.RaceDefinition->AttributeBonuses);
	if (!URPGCharacterRulesLibrary::AreAttributesInRange(FinalAttributes))
	{
		OutError = FText::FromString(TEXT("Toutes les caractéristiques initiales doivent être comprises entre 6 et 20."));
		return false;
	}

	FGridCharacterInventoryState NewCharacter;
	NewCharacter.CharacterId = FGuid::NewGuid();
	NewCharacter.DisplayName = FText::FromString(NormalizedName);
	NewCharacter.RaceId = Request.RaceDefinition->RaceId;
	NewCharacter.RaceDisplayName = Request.RaceDefinition->DisplayName;
	NewCharacter.ClassId = Request.ClassDefinition->ClassId;
	NewCharacter.ClassDisplayName = Request.ClassDefinition->DisplayName;
	NewCharacter.ClassDefinition = CombatActionSourceClass;
	NewCharacter.Level = 1;
	NewCharacter.Experience = 0;
	NewCharacter.Attributes = FinalAttributes;
	NewCharacter.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(FinalAttributes, Request.ClassDefinition, NewCharacter.Level);
	NewCharacter.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(NewCharacter.DerivedStats, Request.ClassDefinition);
	NewCharacter.PortraitGender = Request.PortraitGender;
	NewCharacter.PortraitVariantId = Request.PortraitVariantId;
	NewCharacter.Portrait =
		FRPGAuthoringIdentityResolver::ResolvePortraitVisual(NewCharacter.RaceId, NewCharacter.PortraitGender, NewCharacter.PortraitVariantId);
	NewCharacter.ClassIcon = FRPGAuthoringIdentityResolver::ResolveClassIcon(NewCharacter.ClassId);
	NewCharacter.InventorySlots.SetNum(FMath::Max(1, DefaultInventorySlotCountPerCharacter));
	InitializeCombatHotbarDefaults(NewCharacter);

	FGridPartyInventoryState NewPartyState;
	NewPartyState.SelectedCharacterIndex = 0;
	NewPartyState.MaxActiveCharacters = FMath::Max(1, DefaultMaxActiveCharacters);
	NewPartyState.bInitialCharacterCreationCompleted = true;
	NewPartyState.ActiveCharacters.Add(MoveTemp(NewCharacter));
	NewPartyState.ActiveEquipment.SetNum(1);
	NewPartyState.bHasCursorItem = false;
	NewPartyState.CursorItem = FGridItemInstance();

	// ARMEMENT01.9.2: source-controlled presets are provided only for the six
	// real production class assets. Materialize on a transient duplicate:
	// no cooked DataAsset is mutated and explicit user-authored kits win.
	URPGClassAsset* StartingKitClass = CombatActionSourceClass;
	if (FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(CombatActionSourceClass) &&
		CombatActionSourceClass->StartingEquipmentKits.IsEmpty() &&
		CombatActionSourceClass->DefaultStartingEquipmentKitId.IsNone())
	{
		StartingKitClass = DuplicateObject<URPGClassAsset>(CombatActionSourceClass, this);
		FString PresetError;
		if (!StartingKitClass ||
			!FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*StartingKitClass, PresetError))
		{
			UE_LOG(LogTemp, Warning, TEXT("StartingEquipment PresetUnavailable Reason=%s"), *PresetError);
			OutError = FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.Unavailable"));
			return false;
		}
	}

	// Explicit wizard choice wins over the canonical default; invalid IDs
	// fail inside the transactional kit builder, before touching live inventory.
	const FName SelectedStartingKitId = Request.StartingEquipmentKitId.IsNone()
		? StartingKitClass->DefaultStartingEquipmentKitId : Request.StartingEquipmentKitId;

	// Build the selected class's default kit into the uncommitted party.
	// Failed or incompatible kits cannot create a partially equipped hero.
	TArray<UGridItemDefinitionAsset*> StartingDefinitions;
	FString KitError;
	if (!FRPGStartingEquipmentService::TryBuildKit(
		StartingKitClass, SelectedStartingKitId,
		NewPartyState.ActiveCharacters[0], NewPartyState.ActiveEquipment[0], StartingDefinitions, KitError))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartingEquipment InvalidKit KitId=%s Reason=%s"), *SelectedStartingKitId.ToString(), *KitError);
		OutError = FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.InvalidKit"));
		return false;
	}
	for (UGridItemDefinitionAsset* Definition : StartingDefinitions)
	{
		if (UGridItemDefinitionAsset* Existing = FindItemDefinition(Definition->ItemDefinitionId))
		{
			if (Existing != Definition)
			{
				UE_LOG(LogTemp, Warning, TEXT("StartingEquipment ConflictingDefinition ItemId=%s"), *Definition->ItemDefinitionId.ToString());
				OutError = FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.ConflictingDefinition"));
				return false;
			}
		}
	}

	const FGridPartyInventoryState PreviousPartyState = PartyInventoryState;
	const TArray<FGuid> PreviousPortraitOrder = PartyPortraitOrder;
	PartyInventoryState = MoveTemp(NewPartyState);
	TArray<FName> AddedDefinitionIds;
	for (UGridItemDefinitionAsset* Definition : StartingDefinitions)
	{
		if (!RuntimeItemDefinitionsById.Contains(Definition->ItemDefinitionId))
		{
			RuntimeItemDefinitionsById.Add(Definition->ItemDefinitionId, Definition);
			AddedDefinitionIds.Add(Definition->ItemDefinitionId);
		}
	}
	NotifyPartyInventoryChanged(INDEX_NONE);

	FString OwnershipError;
	if (!ValidateInventoryOwnership(OwnershipError))
	{
		PartyInventoryState = PreviousPartyState;
		PartyPortraitOrder = PreviousPortraitOrder;
		for (const FName DefinitionId : AddedDefinitionIds)
		{
			RuntimeItemDefinitionsById.Remove(DefinitionId);
		}
		NotifyPartyInventoryChanged(INDEX_NONE);
		OutError = FText::FromString(FString::Printf(TEXT("La création du personnage a échoué lors de la validation de l'ownership : %s"), *OwnershipError));
		return false;
	}

	return true;
}

int32 UGridPartyInventoryComponent::GetActiveCharacterCount() const
{
	return PartyInventoryState.ActiveCharacters.Num();
}

int32 UGridPartyInventoryComponent::GetMaxActiveCharacters() const
{
	return PartyInventoryState.MaxActiveCharacters;
}

int32 UGridPartyInventoryComponent::GetMaxActiveCharacterCount() const
{
	return GetMaxActiveCharacters();
}

int32 UGridPartyInventoryComponent::GetSelectedCharacterIndex() const
{
	return PartyInventoryState.SelectedCharacterIndex;
}

bool UGridPartyInventoryComponent::SetSelectedCharacterIndex(int32 NewIndex)
{
	const int32 OldIndex = PartyInventoryState.SelectedCharacterIndex;
	if (!IsValidCharacterIndex(NewIndex))
	{
		UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory SelectedCharacter Changed Old=%d New=%d Result=false"), OldIndex, NewIndex);
		return false;
	}

	PartyInventoryState.SelectedCharacterIndex = NewIndex;
	if (OldIndex != NewIndex)
	{
		NotifyPartyInventoryChanged(INDEX_NONE);
	}
	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory SelectedCharacter Changed Old=%d New=%d Result=true"), OldIndex, NewIndex);
	return true;
}

FText UGridPartyInventoryComponent::GetActiveCharacterDisplayName(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return FText::GetEmpty();
	}

	const FText& DisplayName = PartyInventoryState.ActiveCharacters[CharacterIndex].DisplayName;
	return DisplayName.IsEmpty()
		? FText::FromString(CharacterIndex == 0 ? TEXT("Hero_01") : FString::Printf(TEXT("Hero_%02d"), CharacterIndex + 1))
		: DisplayName;
}

bool UGridPartyInventoryComponent::HasUnspentCharacterProgressionPoints(int32 CharacterIndex) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PERF_PROFILE01_RPG_ProgressionPointsRead);
	return IsValidCharacterIndex(CharacterIndex) &&
		HasUnspentPartyPortraitProgressionPoints(PartyInventoryState.ActiveCharacters[CharacterIndex]);
}


int32 UGridPartyInventoryComponent::RefillArmorReservesAfterVictory()
{
	return RefillArmorReservesForRecovery();
}

int32 UGridPartyInventoryComponent::RefillArmorReservesAfterRest()
{
	return RefillArmorReservesForRecovery();
}

int32 UGridPartyInventoryComponent::RefillArmorReservesForRecovery()
{
	// Two authoritative callers: committed normal Victory and accepted explicit Rest.
	// Never refill on load, Tick, combat start, abort or equipment transfer.
	int32 ChangedCharacters = 0;
	bool bAnyLivingCharacter = false;
	TSet<FGuid> VisitedItems;

	auto RefillItem = [&VisitedItems](FGridItemInstance& Item) -> bool
	{
		if (!Item.IsValid() || VisitedItems.Contains(Item.RuntimeObjectId))
		{
			return false;
		}
		VisitedItems.Add(Item.RuntimeObjectId);
		if (Item.AbsorbedEquipmentPhysicalArmor <= 0 &&
			Item.AbsorbedEquipmentMagicalArmor <= 0)
		{
			return false;
		}
		Item.AbsorbedEquipmentPhysicalArmor = 0;
		Item.AbsorbedEquipmentMagicalArmor = 0;
		return true;
	};

	for (int32 Index = 0; Index < PartyInventoryState.ActiveCharacters.Num(); ++Index)
	{
		FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[Index];
		if (Character.Resources.CurrentHealth <= 0)
		{
			continue;
		}
		bAnyLivingCharacter = true;
		if (!ResolvePartyPortraitClassDefinition(Character))
		{
			UE_LOG(LogGridInventory, Warning,
				TEXT("[RPG-SHIELD02.2] Victory refill skipped Character=%d ClassId=%s Reason=UnresolvedClass"),
				Index, *Character.ClassId.ToString());
			continue;
		}

		FGridInventoryCharacterSummary Summary;
		if (!GetCharacterSummary(Index, Summary))
		{
			continue;
		}
		int32 PhysicalReference = 0;
		int32 MagicalReference = 0;
		ResolvePartyPortraitArmorReferences(Character, Summary, PhysicalReference, MagicalReference);

		// The effective armor read model separately adds equipped protection.
		// Only the remainder belongs in the character's stored base reserve.
		const int32 BaseReference = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(PhysicalReference) -
				static_cast<int64>(Summary.EquipmentStatBonus.ArmorBonus), 0, MAX_int32));
		const int32 BaseMagicalReference = static_cast<int32>(FMath::Clamp<int64>(
			static_cast<int64>(MagicalReference) -
				static_cast<int64>(Summary.EquipmentStatBonus.MagicalArmorBonus), 0, MAX_int32));
		bool bChanged = Character.Resources.CurrentPhysicalArmor != BaseReference ||
			Character.Resources.CurrentMagicalArmor != BaseMagicalReference;
		Character.Resources.CurrentPhysicalArmor = BaseReference;
		Character.Resources.CurrentMagicalArmor = BaseMagicalReference;

		if (PartyInventoryState.ActiveEquipment.IsValidIndex(Index))
		{
			FGridCharacterEquipmentState& Equipment = PartyInventoryState.ActiveEquipment[Index];
			for (uint8 SlotValue = GridEquipmentSlotUtils::FirstSlotValue;
				SlotValue <= GridEquipmentSlotUtils::LastSlotValue; ++SlotValue)
			{
				if (FGridItemInstance* Item = Equipment.GetMutableSlot(static_cast<EGridEquipmentSlot>(SlotValue)))
				{
					bChanged |= RefillItem(*Item);
				}
			}
		}
		for (FGridInventorySlot& Slot : Character.InventorySlots)
		{
			if (!Slot.IsEmpty())
			{
				bChanged |= RefillItem(Slot.Item);
			}
		}
		if (bChanged)
		{
			++ChangedCharacters;
			NotifyPartyInventoryChanged(Index);
		}
	}

	// Cursor has party ownership (OwnerCharacterIndex == INDEX_NONE).
	if (bAnyLivingCharacter && PartyInventoryState.bHasCursorItem &&
		RefillItem(PartyInventoryState.CursorItem))
	{
		NotifyPartyInventoryChanged(INDEX_NONE);
	}
	return ChangedCharacters;
}

bool UGridPartyInventoryComponent::GetCharacterSummary(int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PERF_PROFILE01_RPG_CharacterSummary);
	OutSummary = FGridInventoryCharacterSummary();
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	OutSummary.CharacterIndex = CharacterIndex;
	OutSummary.CharacterId = CharacterState.CharacterId.IsValid() ? FName(*CharacterState.CharacterId.ToString(EGuidFormats::DigitsWithHyphens)) : NAME_None;
	OutSummary.DisplayName = GetActiveCharacterDisplayName(CharacterIndex);
	OutSummary.ClassId = CharacterState.ClassId;
	OutSummary.ClassDisplayName = CharacterState.ClassDisplayName.IsEmpty() ? FText::FromName(CharacterState.ClassId) : CharacterState.ClassDisplayName;
	OutSummary.RaceId = CharacterState.RaceId;
	OutSummary.RaceDisplayName = CharacterState.RaceDisplayName.IsEmpty() ? FText::FromName(CharacterState.RaceId) : CharacterState.RaceDisplayName;
	OutSummary.Level = CharacterState.Level;
	OutSummary.Experience = CharacterState.Experience;
	OutSummary.BaseAttributes = CharacterState.Attributes;
	OutSummary.BaseDerivedStats = CharacterState.DerivedStats;
	OutSummary.BaseMaxWeight = CalculateCharacterBaseMaxWeight(CharacterIndex);
	OutSummary.EquipmentStatBonus = ComputeCharacterEquipmentStatBonus(CharacterIndex);
	OutSummary.EquipmentResistances = ComputeCharacterEquipmentResistances(CharacterIndex);
	OutSummary.FinalResistances = OutSummary.EquipmentResistances;
	OutSummary.Attributes = OutSummary.BaseAttributes;
	OutSummary.Attributes.Strength = FMath::Max(0, OutSummary.Attributes.Strength + OutSummary.EquipmentStatBonus.StrengthBonus);
	OutSummary.Attributes.Dexterity = FMath::Max(0, OutSummary.Attributes.Dexterity + OutSummary.EquipmentStatBonus.DexterityBonus);
	OutSummary.Attributes.Constitution = FMath::Max(0, OutSummary.Attributes.Constitution + OutSummary.EquipmentStatBonus.ConstitutionBonus);
	OutSummary.Attributes.Intelligence = FMath::Max(0, OutSummary.Attributes.Intelligence + OutSummary.EquipmentStatBonus.IntelligenceBonus);
	OutSummary.Attributes.Wisdom = FMath::Max(0, OutSummary.Attributes.Wisdom + OutSummary.EquipmentStatBonus.WisdomBonus);
	OutSummary.Attributes.Charisma = FMath::Max(0, OutSummary.Attributes.Charisma + OutSummary.EquipmentStatBonus.CharismaBonus);
	OutSummary.DerivedStats = OutSummary.BaseDerivedStats;
	OutSummary.Resources = CharacterState.Resources;
	OutSummary.DerivedStats.MaxHealth = FMath::Max(1, OutSummary.DerivedStats.MaxHealth + OutSummary.EquipmentStatBonus.MaxHealthBonus);
	OutSummary.Resources.CurrentHealth = FMath::Clamp(OutSummary.Resources.CurrentHealth, 0, OutSummary.DerivedStats.MaxHealth);
	OutSummary.DerivedStats.MaxMana = FMath::Max(0, OutSummary.DerivedStats.MaxMana + OutSummary.EquipmentStatBonus.MaxManaBonus);
	OutSummary.Resources.CurrentMana = FMath::Clamp(OutSummary.Resources.CurrentMana, 0, OutSummary.DerivedStats.MaxMana);
	OutSummary.Resources.CurrentPhysicalArmor = GetEffectiveCurrentPhysicalArmor(CharacterIndex);
	OutSummary.Resources.CurrentMagicalArmor = GetEffectiveCurrentMagicalArmor(CharacterIndex);
	OutSummary.Portrait =
		FRPGAuthoringIdentityResolver::ResolvePortraitVisual(CharacterState.RaceId, CharacterState.PortraitGender, CharacterState.PortraitVariantId);
	OutSummary.UsedInventorySlots = CountOccupiedSlots(CharacterState);
	OutSummary.MaxInventorySlots = CharacterState.InventorySlots.Num();
	OutSummary.CurrentWeight = CalculateCharacterCurrentWeight(CharacterIndex);
	OutSummary.MaxWeight = FMath::Max(0.0f, OutSummary.BaseMaxWeight + OutSummary.EquipmentStatBonus.CarryWeightBonus);
	OutSummary.WeightState = ResolveInventoryWeightState(OutSummary.CurrentWeight, OutSummary.MaxWeight);
	OutSummary.bOverloaded = OutSummary.WeightState == EGridInventoryWeightState::Overloaded;
	OutSummary.bHasUnspentProgressionPoints = HasUnspentCharacterProgressionPoints(CharacterIndex);
	OutSummary.bIsSelected = CharacterIndex == PartyInventoryState.SelectedCharacterIndex;
	return true;
}


void UGridPartyInventoryComponent::SanitizePartyPortraitOrder()
{
	TArray<FGuid> SanitizedOrder;
	SanitizedOrder.Reserve(PartyInventoryState.ActiveCharacters.Num());
	TSet<FGuid> SeenIds;

	for (const FGuid& CharacterId : PartyPortraitOrder)
	{
		if (CharacterId.IsValid() && ResolveActiveCharacterIndexById(CharacterId) != INDEX_NONE && !SeenIds.Contains(CharacterId))
		{
			SanitizedOrder.Add(CharacterId);
			SeenIds.Add(CharacterId);
		}
	}

	for (const FGridCharacterInventoryState& Character : PartyInventoryState.ActiveCharacters)
	{
		if (Character.CharacterId.IsValid() && !SeenIds.Contains(Character.CharacterId))
		{
			SanitizedOrder.Add(Character.CharacterId);
			SeenIds.Add(Character.CharacterId);
		}
	}

	PartyPortraitOrder = MoveTemp(SanitizedOrder);
}

void UGridPartyInventoryComponent::RestorePartyPortraitOrder(const TArray<FGuid>& SavedOrder)
{
	PartyPortraitOrder = SavedOrder;
	NotifyPartyInventoryChanged(INDEX_NONE);
}

int32 UGridPartyInventoryComponent::ResolveActiveCharacterIndexById(FGuid CharacterId) const
{
	if (!CharacterId.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		if (PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId == CharacterId)
		{
			return CharacterIndex;
		}
	}
	return INDEX_NONE;
}

bool UGridPartyInventoryComponent::GetPartyPortraitViews(TArray<FGridPartyPortraitView>& OutViews)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PERF_PROFILE01_RPG_PortraitViews);
	OutViews.Reset();
	SanitizePartyPortraitOrder();
	OutViews.Reserve(PartyPortraitOrder.Num());

	for (const FGuid& CharacterId : PartyPortraitOrder)
	{
		const int32 CharacterIndex = ResolveActiveCharacterIndexById(CharacterId);
		if (!PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			continue;
		}

		FGridInventoryCharacterSummary Summary;
		if (!GetCharacterSummary(CharacterIndex, Summary))
		{
			continue;
		}

		const FGridCharacterInventoryState& Character = PartyInventoryState.ActiveCharacters[CharacterIndex];
		FGridPartyPortraitView View;
		View.CharacterId = Character.CharacterId;
		View.CharacterIndex = CharacterIndex;
		View.Portrait = Summary.Portrait;
		View.bIsSelected = Summary.bIsSelected;
		View.DisplayName = Summary.DisplayName;
		View.ClassDisplayName = Summary.ClassDisplayName;
		View.Level = FMath::Max(1, Summary.Level);
		View.bHasUnspentProgressionPoints = Summary.bHasUnspentProgressionPoints;
		View.bOverloaded = Summary.bOverloaded;

		View.CurrentHealth = FMath::Max(0, Summary.Resources.CurrentHealth);
		View.MaximumHealth = FMath::Max(1, Summary.DerivedStats.MaxHealth);
		View.HealthRatio = ResolvePartyPortraitRatio(View.CurrentHealth, View.MaximumHealth);

		View.CurrentMana = FMath::Max(0, Summary.Resources.CurrentMana);
		View.MaximumMana = FMath::Max(0, Summary.DerivedStats.MaxMana);
		View.bHasMana = View.MaximumMana > 0;
		View.ManaRatio = View.bHasMana ? ResolvePartyPortraitRatio(View.CurrentMana, View.MaximumMana) : 0.0f;

		View.CurrentPhysicalArmor = FMath::Max(0, Summary.Resources.CurrentPhysicalArmor);
		View.CurrentMagicalArmor = FMath::Max(0, Summary.Resources.CurrentMagicalArmor);

		ResolvePartyPortraitArmorReferences(
			Character, Summary, View.ReferencePhysicalArmor, View.ReferenceMagicalArmor);

		View.PhysicalArmorRatio = ResolvePartyPortraitRatio(View.CurrentPhysicalArmor, View.ReferencePhysicalArmor);
		View.MagicalArmorRatio = ResolvePartyPortraitRatio(View.CurrentMagicalArmor, View.ReferenceMagicalArmor);
		OutViews.Add(MoveTemp(View));
	}

	return OutViews.Num() == PartyPortraitOrder.Num();
}

bool UGridPartyInventoryComponent::ReorderPartyPortrait(FGuid SourceCharacterId, FGuid TargetCharacterId, bool bInsertAfter)
{
	SanitizePartyPortraitOrder();
	if (!SourceCharacterId.IsValid() || !TargetCharacterId.IsValid() || SourceCharacterId == TargetCharacterId)
	{
		return false;
	}

	const int32 SourceIndex = PartyPortraitOrder.IndexOfByKey(SourceCharacterId);
	if (SourceIndex == INDEX_NONE || PartyPortraitOrder.IndexOfByKey(TargetCharacterId) == INDEX_NONE)
	{
		return false;
	}

	PartyPortraitOrder.RemoveAt(SourceIndex);
	const int32 TargetIndexAfterRemoval = PartyPortraitOrder.IndexOfByKey(TargetCharacterId);
	if (TargetIndexAfterRemoval == INDEX_NONE)
	{
		SanitizePartyPortraitOrder();
		return false;
	}

	const int32 InsertIndex = FMath::Clamp(TargetIndexAfterRemoval + (bInsertAfter ? 1 : 0), 0, PartyPortraitOrder.Num());
	PartyPortraitOrder.Insert(SourceCharacterId, InsertIndex);
	NotifyPartyInventoryChanged(INDEX_NONE);
	return true;
}

bool UGridPartyInventoryComponent::IsCharacterOverloaded(int32 CharacterIndex) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PERF_PROFILE01_RPG_CharacterOverloaded);
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}
	const float CurrentWeight = CalculateCharacterCurrentWeight(CharacterIndex);
	const float MaxWeight = FMath::Max(
		0.0f, CalculateCharacterBaseMaxWeight(CharacterIndex) +
		ComputeCharacterEquipmentStatBonus(CharacterIndex).CarryWeightBonus);
	return ResolveInventoryWeightState(CurrentWeight, MaxWeight) ==
		EGridInventoryWeightState::Overloaded;
}

bool UGridPartyInventoryComponent::IsAnyActiveCharacterOverloaded() const
{
	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		if (IsCharacterOverloaded(CharacterIndex))
		{
			return true;
		}
	}
	return false;
}

void UGridPartyInventoryComponent::GetOverloadedActiveCharacterIndices(TArray<int32>& OutIndices) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PERF_PROFILE01_RPG_OverloadScan);
	OutIndices.Reset();
	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		if (IsCharacterOverloaded(CharacterIndex))
		{
			OutIndices.Add(CharacterIndex);
		}
	}
}

bool UGridPartyInventoryComponent::IsValidCharacterIndex(int32 Index) const
{
	return PartyInventoryState.IsValidActiveCharacterIndex(Index);
}

bool UGridPartyInventoryComponent::CanAddItemToCharacterInventory(int32 CharacterIndex, const FGridItemInstance& Item) const
{
	FGridItemInstance ItemToAdd = Item;
	const int32 InitialQuantity = ItemToAdd.Quantity;
	const UGridItemDefinitionAsset* SourceDefinition = FindItemDefinition(ItemToAdd.ItemDefinitionId);
	if (!ApplyItemDefinitionToInstance(ItemToAdd) &&
		SourceDefinition && SourceDefinition->bUseWeaponCatalogue)
	{
		// Never accept an unresolved linked item as a silently downgraded Legacy item.
		return false;
	}
	ItemToAdd.Quantity = InitialQuantity;

	if (!IsValidCharacterIndex(CharacterIndex) || !ItemToAdd.IsValid())
	{
		return false;
	}

	const UGridItemDefinitionAsset* Definition = FindItemDefinition(ItemToAdd.ItemDefinitionId);
	const bool bStackable = Definition && Definition->bStackable;
	const int32 MaxStackSize = bStackable ? FMath::Max(1, Definition->MaxStackSize) : 1;

	int64 AvailableCapacity = 0;
	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (Slot.IsEmpty())
		{
			AvailableCapacity += MaxStackSize;
		}
		else if (bStackable && Slot.Item.ItemDefinitionId == ItemToAdd.ItemDefinitionId && HaveMatchingReadableContent(Slot.Item, ItemToAdd))
		{
			AvailableCapacity += FMath::Max(0, MaxStackSize - FMath::Max(1, Slot.Item.Quantity));
		}

		if (AvailableCapacity >= InitialQuantity)
		{
			return true;
		}
	}

	return AvailableCapacity >= InitialQuantity;
}

bool UGridPartyInventoryComponent::CanAddItemToSelectedCharacterInventory(const FGridItemInstance& Item) const
{
	return CanAddItemToCharacterInventory(PartyInventoryState.SelectedCharacterIndex, Item);
}

bool UGridPartyInventoryComponent::AddItemToCharacterInventory(int32 CharacterIndex, const FGridItemInstance& Item)
{
	FGridItemInstance ItemToAdd = Item;
	const int32 InitialQuantity = ItemToAdd.Quantity;
	const UGridItemDefinitionAsset* SourceDefinition = FindItemDefinition(ItemToAdd.ItemDefinitionId);
	if (!ApplyItemDefinitionToInstance(ItemToAdd) &&
		SourceDefinition && SourceDefinition->bUseWeaponCatalogue)
	{
		// Never accept an unresolved linked item as a silently downgraded Legacy item.
		return false;
	}
	ItemToAdd.Quantity = InitialQuantity;

	if (!IsValidCharacterIndex(CharacterIndex) || !ItemToAdd.IsValid() || !CanAddItemToCharacterInventory(CharacterIndex, ItemToAdd))
	{
		return false;
	}

	const UGridItemDefinitionAsset* Definition = FindItemDefinition(ItemToAdd.ItemDefinitionId);
	const bool bStackable = Definition && Definition->bStackable;
	const int32 MaxStackSize = bStackable ? FMath::Max(1, Definition->MaxStackSize) : 1;

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	ItemToAdd.OwnerType = EGridItemOwnerType::CharacterInventory;
	ItemToAdd.OwnerGuid = CharacterState.CharacterId;
	ItemToAdd.OwnerCharacterIndex = CharacterIndex;
	ItemToAdd.EquipmentSlot = EGridEquipmentSlot::None;

	TArray<FGridInventorySlot> UpdatedInventorySlots = CharacterState.InventorySlots;
	int32 RemainingQuantity = InitialQuantity;
	if (bStackable)
	{
		for (FGridInventorySlot& Slot : UpdatedInventorySlots)
		{
			if (RemainingQuantity <= 0)
			{
				break;
			}
			if (Slot.IsEmpty() || Slot.Item.ItemDefinitionId != ItemToAdd.ItemDefinitionId || !HaveMatchingReadableContent(Slot.Item, ItemToAdd))
			{
				continue;
			}

			const int32 AvailableInStack = FMath::Max(0, MaxStackSize - FMath::Max(1, Slot.Item.Quantity));
			const int32 QuantityToStack = FMath::Min(RemainingQuantity, AvailableInStack);
			Slot.Item.Quantity += QuantityToStack;
			RemainingQuantity -= QuantityToStack;
		}
	}

	bool bUsedIncomingRuntimeObjectId = false;
	for (FGridInventorySlot& Slot : UpdatedInventorySlots)
	{
		if (RemainingQuantity <= 0)
		{
			break;
		}
		if (!Slot.IsEmpty())
		{
			continue;
		}

		FGridItemInstance NewStack = ItemToAdd;
		NewStack.Quantity = FMath::Min(RemainingQuantity, MaxStackSize);
		if (bUsedIncomingRuntimeObjectId)
		{
			NewStack.RuntimeObjectId = FGuid::NewGuid();
		}

		Slot.bOccupied = true;
		Slot.Item = NewStack;
		RemainingQuantity -= NewStack.Quantity;
		bUsedIncomingRuntimeObjectId = true;
	}

	if (RemainingQuantity != 0)
	{
		return false;
	}

	CharacterState.InventorySlots = MoveTemp(UpdatedInventorySlots);
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::AddItemToSelectedCharacterInventory(const FGridItemInstance& Item)
{
	return AddItemToCharacterInventory(PartyInventoryState.SelectedCharacterIndex, Item);
}

bool UGridPartyInventoryComponent::TrySplitInventoryStackToFirstFreeSlot(int32 CharacterIndex, int32 SourceSlotIndex)
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory SplitStack Failed Character=%d SourceSlot=%d Reason=InvalidCharacter"), CharacterIndex, SourceSlotIndex);
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(SourceSlotIndex) || CharacterState.InventorySlots[SourceSlotIndex].IsEmpty())
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory SplitStack Failed Character=%d SourceSlot=%d Reason=InvalidSourceSlot"), CharacterIndex, SourceSlotIndex);
		return false;
	}

	FGridInventorySlot& SourceSlot = CharacterState.InventorySlots[SourceSlotIndex];
	const UGridItemDefinitionAsset* Definition = FindItemDefinition(SourceSlot.Item.ItemDefinitionId);
	if (!Definition || !Definition->bStackable || SourceSlot.Item.Quantity < 2)
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory SplitStack Failed Character=%d SourceSlot=%d Item=%s Quantity=%d Reason=NotSplittable"),
			CharacterIndex, SourceSlotIndex, *SourceSlot.Item.ItemDefinitionId.ToString(), SourceSlot.Item.Quantity);
		return false;
	}

	int32 TargetSlotIndex = INDEX_NONE;
	for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num(); ++SlotIndex)
	{
		if (CharacterState.InventorySlots[SlotIndex].IsEmpty())
		{
			TargetSlotIndex = SlotIndex;
			break;
		}
	}

	if (TargetSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogGridInventory, Warning, TEXT("GridInventory SplitStack Failed Character=%d SourceSlot=%d Item=%s Quantity=%d Reason=NoFreeSlot"),
			CharacterIndex, SourceSlotIndex, *SourceSlot.Item.ItemDefinitionId.ToString(), SourceSlot.Item.Quantity);
		return false;
	}

	const int32 SplitQuantity = SourceSlot.Item.Quantity / 2;
	FGridItemInstance SplitItem = SourceSlot.Item;
	SplitItem.RuntimeObjectId = FGuid::NewGuid();
	SplitItem.Quantity = SplitQuantity;
	SplitItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	SplitItem.OwnerGuid = CharacterState.CharacterId;
	SplitItem.OwnerCharacterIndex = CharacterIndex;
	SplitItem.EquipmentSlot = EGridEquipmentSlot::None;

	SourceSlot.Item.Quantity -= SplitQuantity;
	FGridInventorySlot& TargetSlot = CharacterState.InventorySlots[TargetSlotIndex];
	TargetSlot.bOccupied = true;
	TargetSlot.Item = MoveTemp(SplitItem);

	NotifyPartyInventoryChanged(CharacterIndex);
	UE_LOG(LogGridInventory, Verbose,
		TEXT("GridInventory SplitStack Character=%d SourceSlot=%d TargetSlot=%d Item=%s SourceQuantity=%d SplitQuantity=%d Result=true"),
		CharacterIndex, SourceSlotIndex, TargetSlotIndex, *SourceSlot.Item.ItemDefinitionId.ToString(), SourceSlot.Item.Quantity, SplitQuantity);
	return true;
}

bool UGridPartyInventoryComponent::RemoveItemFromCharacterInventoryByRuntimeId(int32 CharacterIndex, FGuid RuntimeObjectId, FGridItemInstance& OutRemovedItem)
{
	OutRemovedItem = FGridItemInstance();
	if (!IsValidCharacterIndex(CharacterIndex) || !RuntimeObjectId.IsValid())
	{
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (Slot.IsEmpty() || Slot.Item.RuntimeObjectId != RuntimeObjectId)
		{
			continue;
		}

		OutRemovedItem = Slot.Item;
		Slot = FGridInventorySlot();
		NotifyPartyInventoryChanged(CharacterIndex);
		return true;
	}

	return false;
}

bool UGridPartyInventoryComponent::RemoveFirstItemFromCharacterInventoryByDefinitionId(
	int32 CharacterIndex, FName ItemDefinitionId, FGridItemInstance& OutRemovedItem)
{
	OutRemovedItem = FGridItemInstance();
	if (!IsValidCharacterIndex(CharacterIndex) || ItemDefinitionId.IsNone())
	{
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (Slot.IsEmpty() || Slot.Item.ItemDefinitionId != ItemDefinitionId)
		{
			continue;
		}

		OutRemovedItem = Slot.Item;
		Slot = FGridInventorySlot();
		NotifyPartyInventoryChanged(CharacterIndex);
		return true;
	}

	return false;
}

bool UGridPartyInventoryComponent::RemoveFirstItemFromSelectedCharacterInventoryByDefinitionId(FName ItemDefinitionId, FGridItemInstance& OutRemovedItem)
{
	return RemoveFirstItemFromCharacterInventoryByDefinitionId(PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId, OutRemovedItem);
}

bool UGridPartyInventoryComponent::HasItemDefinitionInCharacterInventory(int32 CharacterIndex, FName ItemDefinitionId) const
{
	return CountItemDefinitionInCharacterInventory(CharacterIndex, ItemDefinitionId) > 0;
}

bool UGridPartyInventoryComponent::HasItemDefinitionInSelectedCharacterInventory(FName ItemDefinitionId) const
{
	return HasItemDefinitionInCharacterInventory(PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId);
}

int32 UGridPartyInventoryComponent::CountItemDefinitionInCharacterInventory(int32 CharacterIndex, FName ItemDefinitionId) const
{
	if (!IsValidCharacterIndex(CharacterIndex) || ItemDefinitionId.IsNone())
	{
		return 0;
	}

	int32 Count = 0;
	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemDefinitionId)
		{
			Count += FMath::Max(1, Slot.Item.Quantity);
		}
	}
	return Count;
}

int32 UGridPartyInventoryComponent::CountItemDefinitionInSelectedCharacterInventory(FName ItemDefinitionId) const
{
	return CountItemDefinitionInCharacterInventory(PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId);
}

bool UGridPartyInventoryComponent::RemoveItemDefinitionFromCharacterInventory(int32 CharacterIndex, FName ItemDefinitionId, int32 Quantity)
{
	if (!IsValidCharacterIndex(CharacterIndex) || ItemDefinitionId.IsNone() || Quantity <= 0)
	{
		return false;
	}

	if (CountItemDefinitionInCharacterInventory(CharacterIndex, ItemDefinitionId) < Quantity)
	{
		return false;
	}

	int32 RemainingToRemove = Quantity;
	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (RemainingToRemove <= 0)
		{
			break;
		}
		if (Slot.IsEmpty() || Slot.Item.ItemDefinitionId != ItemDefinitionId)
		{
			continue;
		}

		const int32 SlotQuantity = FMath::Max(1, Slot.Item.Quantity);
		if (SlotQuantity > RemainingToRemove)
		{
			Slot.Item.Quantity = SlotQuantity - RemainingToRemove;
			RemainingToRemove = 0;
		}
		else
		{
			RemainingToRemove -= SlotQuantity;
			Slot = FGridInventorySlot();
		}
	}

	if (RemainingToRemove != 0)
	{
		return false;
	}

	if (!CharacterHasInventoryItemDefinition(CharacterState, ItemDefinitionId))
	{
		ClearQuickItemHotbarBindings(CharacterState, ItemDefinitionId);
	}

	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::RemoveItemDefinitionFromSelectedCharacterInventory(FName ItemDefinitionId, int32 Quantity)
{
	return RemoveItemDefinitionFromCharacterInventory(PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId, Quantity);
}

bool UGridPartyInventoryComponent::RegisterItemDefinition(UGridItemDefinitionAsset* Definition)
{
	if (!Definition || Definition->ItemDefinitionId.IsNone())
	{
		return false;
	}

	if (const TObjectPtr<UGridItemDefinitionAsset>* ExistingDefinition = RuntimeItemDefinitionsById.Find(Definition->ItemDefinitionId))
	{
		if (ExistingDefinition->Get() == Definition)
		{
			return true;
		}

		UE_LOG(LogGridInventory, Warning,
			TEXT("GridInventory ItemDefinition Registration Failed ItemDefinitionId=%s Existing=%s Incoming=%s Reason=DuplicateId"),
			*Definition->ItemDefinitionId.ToString(), *GetPathNameSafe(ExistingDefinition->Get()), *GetPathNameSafe(Definition));
		return false;
	}

	RuntimeItemDefinitionsById.Add(Definition->ItemDefinitionId, Definition);
	NotifyPartyInventoryChanged(INDEX_NONE);

	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory Registered ItemDefinition=%s Asset=%s"), *Definition->ItemDefinitionId.ToString(), *Definition->GetPathName());

	return true;
}

bool UGridPartyInventoryComponent::RehydrateOwnedItemDefinitions(TFunctionRef<UGridItemDefinitionAsset*(FName)> Resolver, FName& OutMissingDefinitionId)
{
	OutMissingDefinitionId = NAME_None;

	TSet<FName> DefinitionIds;
	auto CollectCharacterItems = [&DefinitionIds](const FGridCharacterInventoryState& Character)
	{
		for (const FGridInventorySlot& Slot : Character.InventorySlots)
		{
			if (!Slot.IsEmpty())
			{
				DefinitionIds.Add(Slot.Item.ItemDefinitionId);
			}
		}
		for (const FGridCombatHotbarBinding& Binding : Character.CombatHotbarSlots)
		{
			const bool bUsesItemDefinition =
				Binding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment || Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
			if (!Binding.IsEmpty() && bUsesItemDefinition && !Binding.SourceDefinitionId.IsNone())
			{
				DefinitionIds.Add(Binding.SourceDefinitionId);
			}
		}
	};

	for (const FGridCharacterInventoryState& Character : PartyInventoryState.ActiveCharacters)
	{
		CollectCharacterItems(Character);
	}
	for (const FGridCharacterInventoryState& Character : PartyInventoryState.CharacterPool)
	{
		CollectCharacterItems(Character);
	}
	for (const FGridCharacterEquipmentState& Equipment : PartyInventoryState.ActiveEquipment)
	{
		GridEquipmentSlotUtils::ForEachEquipmentItem(Equipment,
			[&DefinitionIds](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
			{
				(void)Slot;
				if (Item.IsValid())
				{
					DefinitionIds.Add(Item.ItemDefinitionId);
				}
			});
	}
	if (PartyInventoryState.bHasCursorItem && PartyInventoryState.CursorItem.IsValid())
	{
		DefinitionIds.Add(PartyInventoryState.CursorItem.ItemDefinitionId);
	}

	TArray<UGridItemDefinitionAsset*> ResolvedDefinitions;
	ResolvedDefinitions.Reserve(DefinitionIds.Num());
	for (const FName DefinitionId : DefinitionIds)
	{
		UGridItemDefinitionAsset* Definition = Resolver(DefinitionId);
		if (!Definition || Definition->ItemDefinitionId.IsNone() || Definition->ItemDefinitionId != DefinitionId)
		{
			OutMissingDefinitionId = DefinitionId;
			return false;
		}
		ResolvedDefinitions.Add(Definition);
	}

	RuntimeItemDefinitionsById.Reset();
	for (UGridItemDefinitionAsset* Definition : ResolvedDefinitions)
	{
		RegisterItemDefinition(Definition);
	}
	return true;
}

UGridItemDefinitionAsset* UGridPartyInventoryComponent::FindItemDefinition(FName ItemDefinitionId) const
{
	if (ItemDefinitionId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UGridItemDefinitionAsset>* Definition = RuntimeItemDefinitionsById.Find(ItemDefinitionId))
	{
		return Definition->Get();
	}

	return nullptr;
}

bool UGridPartyInventoryComponent::ApplyItemDefinitionToInstance(FGridItemInstance& ItemInstance) const
{
	UGridItemDefinitionAsset* Definition = FindItemDefinition(ItemInstance.ItemDefinitionId);
	if (!Definition)
	{
		UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory ItemDefinition Missing ItemDefinitionId=%s"), *ItemInstance.ItemDefinitionId.ToString());
		return false;
	}

	// Assign the catalogue level only once for a newly minted instance. Never
	// overwrite a persisted level when moving/rehydrating an existing item.
	float ResolvedWeight = Definition->Weight;
	if (Definition->bUseWeaponCatalogue)
	{
		const int32 FixedLevel = ItemInstance.ItemLevel == 0
			? Definition->DefaultWeaponItemLevel : ItemInstance.ItemLevel;
		const FGridWeaponLevelProfile* LevelProfile = nullptr;
		if (!Definition->TryGetWeaponCatalogueProfile(FixedLevel, LevelProfile))
		{
			return false;
		}
		ItemInstance.ItemLevel = FixedLevel;
		ResolvedWeight = LevelProfile->WeightKg;
	}
	ItemInstance.Weight = ResolvedWeight;
	if (ItemInstance.DisplayName.IsEmpty())
	{
		ItemInstance.DisplayName = Definition->DisplayName;
	}
	// ITEM-LIGHT02: this function normalizes definition-owned metadata only.
	// The mutable light state belongs to the item instance and must survive transfers and rehydration.
	if (!Definition->bStackable)
	{
		ItemInstance.Quantity = 1;
	}
	else
	{
		ItemInstance.Quantity = FMath::Clamp(ItemInstance.Quantity, 1, FMath::Max(1, Definition->MaxStackSize));
	}

	UE_LOG(LogGridInventory, Verbose, TEXT("GridInventory ItemDefinition Applied Item=%s Weight=%.2f Type=%d"), *ItemInstance.ItemDefinitionId.ToString(),
		ItemInstance.Weight, static_cast<int32>(Definition->ItemType));
	return true;
}

float UGridPartyInventoryComponent::CalculateCharacterCurrentWeight(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return 0.0f;
	}

	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	float TotalWeight = 0.0f;

	for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
	{
		if (!Slot.IsEmpty())
		{
			TotalWeight += GetItemTotalWeight(Slot.Item);
		}
	}

	if (PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		TotalWeight += CalculateEquipmentWeight(PartyInventoryState.ActiveEquipment[CharacterIndex]);
	}

	return FMath::Max(0.0f, TotalWeight);
}

float UGridPartyInventoryComponent::CalculateCharacterBaseMaxWeight(int32 CharacterIndex) const
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return 0.0f;
	}

	return URPGCharacterRulesLibrary::CalculateMaxCarryWeight(PartyInventoryState.ActiveCharacters[CharacterIndex].Attributes);
}

bool UGridPartyInventoryComponent::ValidateInventoryOwnership(FString& OutError) const
{
	OutError.Empty();

	TMap<FGuid, FString> RuntimeOwners;
	auto RegisterRuntimeOwner = [&RuntimeOwners, &OutError](const FGridItemInstance& Item, const FString& Location) -> bool
	{
		if (!Item.IsValid())
		{
			return true;
		}

		if (const FString* ExistingOwner = RuntimeOwners.Find(Item.RuntimeObjectId))
		{
			OutError =
				FString::Printf(TEXT("Duplicate RuntimeObjectId=%s Existing=%s Duplicate=%s"), *Item.RuntimeObjectId.ToString(), **ExistingOwner, *Location);
			return false;
		}

		RuntimeOwners.Add(Item.RuntimeObjectId, Location);
		return true;
	};

	if (PartyInventoryState.bHasCursorItem)
	{
		if (!PartyInventoryState.CursorItem.IsValid())
		{
			OutError = TEXT("CursorItem flag is set but CursorItem is invalid");
			return false;
		}

		if (PartyInventoryState.CursorItem.OwnerType != EGridItemOwnerType::Cursor || PartyInventoryState.CursorItem.OwnerCharacterIndex != INDEX_NONE ||
			PartyInventoryState.CursorItem.EquipmentSlot != EGridEquipmentSlot::None)
		{
			OutError = FString::Printf(TEXT("CursorItem has invalid ownership Owner=%s Character=%d Slot=%s"),
				GetOwnerTypeName(PartyInventoryState.CursorItem.OwnerType), PartyInventoryState.CursorItem.OwnerCharacterIndex,
				GridEquipmentSlotUtils::GetLogName(PartyInventoryState.CursorItem.EquipmentSlot));
			return false;
		}

		if (!RegisterRuntimeOwner(PartyInventoryState.CursorItem, TEXT("CursorItem")))
		{
			return false;
		}
	}
	else if (PartyInventoryState.CursorItem.IsValid())
	{
		OutError = TEXT("CursorItem is valid while cursor flag is false");
		return false;
	}

	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
		for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num(); ++SlotIndex)
		{
			const FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[SlotIndex];
			if (InventorySlot.IsEmpty())
			{
				continue;
			}

			const FGridItemInstance& Item = InventorySlot.Item;
			if (Item.OwnerType != EGridItemOwnerType::CharacterInventory || Item.OwnerCharacterIndex != CharacterIndex ||
				Item.EquipmentSlot != EGridEquipmentSlot::None)
			{
				OutError =
					FString::Printf(TEXT("Inventory item has invalid ownership Character=%d Slot=%d Item=%s Owner=%s OwnerCharacter=%d EquipmentSlot=%s"),
						CharacterIndex, SlotIndex, *Item.ItemDefinitionId.ToString(), GetOwnerTypeName(Item.OwnerType), Item.OwnerCharacterIndex,
						GridEquipmentSlotUtils::GetLogName(Item.EquipmentSlot));
				return false;
			}

			const FString Location = FString::Printf(TEXT("Inventory Character=%d Slot=%d"), CharacterIndex, SlotIndex);
			if (!RegisterRuntimeOwner(Item, Location))
			{
				return false;
			}
		}

		if (!PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
		{
			continue;
		}

		GridEquipmentSlotUtils::ForEachEquipmentItem(PartyInventoryState.ActiveEquipment[CharacterIndex],
			[CharacterIndex, &RegisterRuntimeOwner, &OutError](EGridEquipmentSlot Slot, const FGridItemInstance& Item)
			{
				if (!Item.IsValid())
				{
					return;
				}

				if (Item.OwnerType != EGridItemOwnerType::EquipmentSlot || Item.OwnerCharacterIndex != CharacterIndex || Item.EquipmentSlot != Slot)
				{
					OutError =
						FString::Printf(TEXT("Equipment item has invalid ownership Character=%d Slot=%s Item=%s Owner=%s OwnerCharacter=%d EquipmentSlot=%s"),
							CharacterIndex, GridEquipmentSlotUtils::GetLogName(Slot), *Item.ItemDefinitionId.ToString(), GetOwnerTypeName(Item.OwnerType),
							Item.OwnerCharacterIndex, GridEquipmentSlotUtils::GetLogName(Item.EquipmentSlot));
					return;
				}

				const FString Location =
					FString::Printf(TEXT("Equipment Character=%d Slot=%s"), CharacterIndex, GridEquipmentSlotUtils::GetLogName(Slot));
				RegisterRuntimeOwner(Item, Location);
			});

		if (!OutError.IsEmpty())
		{
			return false;
		}
	}

	return true;
}

void UGridPartyInventoryComponent::EnsureEquipmentCountMatchesActiveCharacters()
{
	const int32 CharacterCount = PartyInventoryState.ActiveCharacters.Num();
	if (PartyInventoryState.ActiveEquipment.Num() < CharacterCount)
	{
		PartyInventoryState.ActiveEquipment.SetNum(CharacterCount);
	}
	else if (PartyInventoryState.ActiveEquipment.Num() > CharacterCount)
	{
		PartyInventoryState.ActiveEquipment.SetNum(CharacterCount);
	}
}

void UGridPartyInventoryComponent::InitializeCharacterDefaults(FGridCharacterInventoryState& CharacterState, int32 CharacterIndex) const
{
	if (!CharacterState.CharacterId.IsValid())
	{
		CharacterState.CharacterId = FGuid::NewGuid();
	}

	if (CharacterState.DisplayName.IsEmpty())
	{
		CharacterState.DisplayName = FText::FromString(CharacterIndex == 0 ? TEXT("Hero_01") : FString::Printf(TEXT("Hero_%02d"), CharacterIndex + 1));
	}

	if (CharacterState.ClassId.IsNone())
	{
		CharacterState.ClassId = TEXT("Warrior");
	}

	if (CharacterState.RaceId.IsNone())
	{
		CharacterState.RaceId = TEXT("Human");
	}

	if (CharacterState.ClassDisplayName.IsEmpty())
	{
		CharacterState.ClassDisplayName = FText::FromName(CharacterState.ClassId);
	}

	if (CharacterState.RaceDisplayName.IsEmpty())
	{
		CharacterState.RaceDisplayName = FText::FromName(CharacterState.RaceId);
	}

	CharacterState.Level = FMath::Max(1, CharacterState.Level);
	CharacterState.Experience = FMath::Max(0, CharacterState.Experience);

	CharacterState.Attributes.Strength = FMath::Max(0, CharacterState.Attributes.Strength);
	CharacterState.Attributes.Dexterity = FMath::Max(0, CharacterState.Attributes.Dexterity);
	CharacterState.Attributes.Constitution = FMath::Max(0, CharacterState.Attributes.Constitution);
	CharacterState.Attributes.Intelligence = FMath::Max(0, CharacterState.Attributes.Intelligence);
	CharacterState.Attributes.Wisdom = FMath::Max(0, CharacterState.Attributes.Wisdom);
	CharacterState.Attributes.Charisma = FMath::Max(0, CharacterState.Attributes.Charisma);

	const int32 ConfiguredInventorySlotCount = FMath::Max(1, DefaultInventorySlotCountPerCharacter);
	if (CharacterState.InventorySlots.Num() < ConfiguredInventorySlotCount)
	{
		CharacterState.InventorySlots.SetNum(ConfiguredInventorySlotCount);
	}

	InitializeCombatHotbarDefaults(CharacterState);
}

void UGridPartyInventoryComponent::InitializeCombatHotbarDefaults(FGridCharacterInventoryState& CharacterState) const
{
	TArray<FGridCombatHotbarBinding> PreviousBindings = MoveTemp(CharacterState.CombatHotbarSlots);
	const int32 TargetSlotCount = FMath::Max(FGridCombatHotbarBinding::MinimumSlotCount, PreviousBindings.Num());
	CharacterState.CombatHotbarSlots.SetNum(TargetSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < TargetSlotCount; ++SlotIndex)
	{
		FGridCombatHotbarBinding& Binding = CharacterState.CombatHotbarSlots[SlotIndex];
		Binding.Reset(SlotIndex);

		if (PreviousBindings.IsValidIndex(SlotIndex))
		{
			FGridCombatHotbarBinding PreviousBinding = PreviousBindings[SlotIndex];
			PreviousBinding.SlotIndex = SlotIndex;
			if (PreviousBinding.IsValid())
			{
				Binding = MoveTemp(PreviousBinding);
			}
		}
	}

	SanitizeCombatHotbarBindings(CharacterState);
}

bool UGridPartyInventoryComponent::ValidateCombatHotbar(const FGridCharacterInventoryState& CharacterState, FString& OutError) const
{
	OutError.Empty();
	if (CharacterState.CombatHotbarSlots.Num() < FGridCombatHotbarBinding::MinimumSlotCount)
	{
		OutError = FString::Printf(TEXT("SlotCount=%d Minimum=%d"), CharacterState.CombatHotbarSlots.Num(), FGridCombatHotbarBinding::MinimumSlotCount);
		return false;
	}

	TSet<FGuid> AssignedEquipmentRuntimeIds;
	TSet<FName> AssignedQuickItemDefinitionIds;
	for (int32 SlotIndex = 0; SlotIndex < CharacterState.CombatHotbarSlots.Num(); ++SlotIndex)
	{
		const FGridCombatHotbarBinding& Binding = CharacterState.CombatHotbarSlots[SlotIndex];
		if (Binding.SlotIndex != SlotIndex || !Binding.IsValid())
		{
			OutError = FString::Printf(TEXT("InvalidSlot=%d StoredIndex=%d Action=%s SourcePolicy=%s"), SlotIndex, Binding.SlotIndex,
				*Binding.ActionId.ToString(), *UEnum::GetValueAsString(Binding.SourcePolicy));
			return false;
		}
		if (SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex && !Binding.IsPrimaryAttackBinding())
		{
			OutError = TEXT("PrimaryAttackMissingFromSlot1");
			return false;
		}
		if (SlotIndex != FGridCombatHotbarBinding::PrimaryAttackSlotIndex && Binding.IsPrimaryAttackBinding())
		{
			OutError = FString::Printf(TEXT("PrimaryAttackDuplicated Slot=%d"), SlotIndex);
			return false;
		}

		if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
		{
			if (AssignedEquipmentRuntimeIds.Contains(Binding.PreferredSourceRuntimeId))
			{
				OutError = FString::Printf(TEXT("DuplicateEquipmentSource Slot=%d RuntimeId=%s"), SlotIndex, *Binding.PreferredSourceRuntimeId.ToString());
				return false;
			}
			AssignedEquipmentRuntimeIds.Add(Binding.PreferredSourceRuntimeId);
		}
		else if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
		{
			if (!CharacterHasInventoryItemDefinition(CharacterState, Binding.SourceDefinitionId))
			{
				OutError = FString::Printf(TEXT("MissingQuickItemSource Slot=%d Definition=%s"), SlotIndex, *Binding.SourceDefinitionId.ToString());
				return false;
			}
			if (AssignedQuickItemDefinitionIds.Contains(Binding.SourceDefinitionId))
			{
				OutError = FString::Printf(TEXT("DuplicateQuickItemSource Slot=%d Definition=%s"), SlotIndex, *Binding.SourceDefinitionId.ToString());
				return false;
			}
			AssignedQuickItemDefinitionIds.Add(Binding.SourceDefinitionId);
		}
	}

	return true;
}

float UGridPartyInventoryComponent::CalculateEquipmentWeight(const FGridCharacterEquipmentState& EquipmentState) const
{
	float TotalWeight = 0.0f;
	GridEquipmentSlotUtils::ForEachEquipmentItem(EquipmentState,
		[&TotalWeight](EGridEquipmentSlot, const FGridItemInstance& Item)
		{
			TotalWeight += GetItemTotalWeight(Item);
		});
	return TotalWeight;
}
