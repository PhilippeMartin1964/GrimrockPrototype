#include "UI/GridSpellbookWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSpellbookEntryWidget.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Magic/GridProductionSpellLibrary.h"
#endif

namespace
{
#if !UE_BUILD_SHIPPING
	void SeedProductionSpellsForPIE(UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: no world."));
			return;
		}

		AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(World, 0));
		if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: party pawn or inventory component unavailable."));
			return;
		}

		UGridPartyInventoryComponent* InventoryComponent = PartyPawn->PartyInventoryComponent;
		const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
		if (!InventoryComponent->IsValidCharacterIndex(CharacterIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: invalid selected character index %d."), CharacterIndex);
			return;
		}

		const FGuid CharacterId = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
		if (!CharacterId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: selected character has no valid CharacterId."));
			return;
		}

		UGridPartySpellbookComponent* SpellbookComponent = PartyPawn->FindComponentByClass<UGridPartySpellbookComponent>();
		if (!SpellbookComponent)
		{
			SpellbookComponent =
				NewObject<UGridPartySpellbookComponent>(PartyPawn, UGridPartySpellbookComponent::StaticClass(), TEXT("PartySpellbookComponent"));
			if (!SpellbookComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: could not create PartySpellbookComponent."));
				return;
			}

			PartyPawn->AddInstanceComponent(SpellbookComponent);
			SpellbookComponent->RegisterComponent();
		}

		if (!SpellbookComponent->EnsureCharacterSpellbook(CharacterId))
		{
			UE_LOG(LogTemp, Warning, TEXT("Grimrock.Spellbook.SeedProduction failed: could not ensure character spellbook."));
			return;
		}

		TArray<FGridSpellDefinition> ProductionSpells;
		FGridProductionSpellLibrary::BuildAll(ProductionSpells);

		int32 AddedCount = 0;
		int32 AlreadyKnownCount = 0;
		for (const FGridSpellDefinition& Definition : ProductionSpells)
		{
			const EGridSpellbookMutationResult Result = SpellbookComponent->LearnSpell(CharacterId, Definition.SpellId);
			if (Result == EGridSpellbookMutationResult::Success)
			{
				++AddedCount;
			}
			else if (Result == EGridSpellbookMutationResult::AlreadyKnown)
			{
				++AlreadyKnownCount;
			}
		}

		UE_LOG(LogTemp, Display,
			TEXT(
				"Grimrock.Spellbook.SeedProduction: CharacterIndex=%d Added=%d AlreadyKnown=%d TotalProduction=%d. MON18.8 persists these learned spells on the next successful save."),
			CharacterIndex, AddedCount, AlreadyKnownCount, ProductionSpells.Num());
	}

	static FAutoConsoleCommandWithWorld GSeedProductionSpellsForPIECommand(TEXT("Grimrock.Spellbook.SeedProduction"),
		TEXT(
			"Development validation command. Teaches all MON18.5 production spells to the currently selected party character; MON18.8 persists them on the next successful save."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&SeedProductionSpellsForPIE), ECVF_Default);
#endif
}

void UGridSpellbookWidget::InitializeSpellbookWidget(AGrimrockPartyPawn* InPartyPawn)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandlePartyInventoryChanged);
	}
	if (SpellbookComponent)
	{
		SpellbookComponent->OnSpellbookChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandleSpellbookChanged);
	}

	OwningPartyPawn = InPartyPawn;
	InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;
	SpellbookComponent = ResolveOrCreateSpellbookComponent(InPartyPawn);

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandlePartyInventoryChanged);
		InventoryComponent->OnPartyInventoryChanged.AddDynamic(this, &UGridSpellbookWidget::HandlePartyInventoryChanged);
	}
	if (SpellbookComponent)
	{
		SpellbookComponent->OnSpellbookChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandleSpellbookChanged);
		SpellbookComponent->OnSpellbookChanged.AddDynamic(this, &UGridSpellbookWidget::HandleSpellbookChanged);
	}

	RefreshSpellbook();
}

void UGridSpellbookWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandlePartyInventoryChanged);
	}
	if (SpellbookComponent)
	{
		SpellbookComponent->OnSpellbookChanged.RemoveDynamic(this, &UGridSpellbookWidget::HandleSpellbookChanged);
	}

	Super::NativeDestruct();
}

void UGridSpellbookWidget::ClearViewState()
{
	SelectedCharacterIndex = INDEX_NONE;
	SelectedCharacterId.Invalidate();
	SpellEntries.Reset();
}

void UGridSpellbookWidget::NotifySpellbookRefreshed()
{
	RebuildSpellEntryWidgets();
	OnSpellbookRefreshed.Broadcast();
}

UGridPartySpellbookComponent* UGridSpellbookWidget::ResolveOrCreateSpellbookComponent(AGrimrockPartyPawn* PartyPawn) const
{
	if (!PartyPawn)
	{
		return nullptr;
	}

	if (UGridPartySpellbookComponent* Existing = PartyPawn->FindComponentByClass<UGridPartySpellbookComponent>())
	{
		Existing->InitializeSpellbookComponent(PartyPawn->PartyInventoryComponent);
		return Existing;
	}

	UGridPartySpellbookComponent* Created =
		NewObject<UGridPartySpellbookComponent>(PartyPawn, UGridPartySpellbookComponent::StaticClass(), TEXT("PartySpellbookComponent"));
	if (!Created)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSpellbookWidget failed to create PartySpellbookComponent."));
		return nullptr;
	}

	PartyPawn->AddInstanceComponent(Created);
	Created->InitializeSpellbookComponent(PartyPawn->PartyInventoryComponent);
	Created->RegisterComponent();

	UE_LOG(LogTemp, VeryVerbose, TEXT("GridSpellbookWidget created runtime PartySpellbookComponent for %s."), *GetNameSafe(PartyPawn));

	return Created;
}

void UGridSpellbookWidget::RefreshSpellbook()
{
	if (bRefreshInProgress)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshInProgress, true);

	ClearViewState();

	if (!InventoryComponent || !SpellbookComponent)
	{
		NotifySpellbookRefreshed();
		return;
	}

	SelectedCharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	if (!InventoryComponent->IsValidCharacterIndex(SelectedCharacterIndex))
	{
		ClearViewState();
		NotifySpellbookRefreshed();
		return;
	}

	SelectedCharacterId = InventoryComponent->PartyInventoryState.ActiveCharacters[SelectedCharacterIndex].CharacterId;
	if (!SelectedCharacterId.IsValid())
	{
		ClearViewState();
		NotifySpellbookRefreshed();
		return;
	}

	FGridCharacterSpellbookState CharacterSpellbook;
	if (!GetSelectedCharacterSpellbook(CharacterSpellbook))
	{
		ClearViewState();
		NotifySpellbookRefreshed();
		return;
	}

	TArray<FGridCombatHotbarBinding> HotbarBindings;
	const int32 HotbarSlotCount = InventoryComponent->GetCombatHotbarSlotCount();
	HotbarBindings.SetNum(HotbarSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < HotbarSlotCount; ++SlotIndex)
	{
		HotbarBindings[SlotIndex].Reset(SlotIndex);
		InventoryComponent->GetCharacterCombatHotbarBinding(SelectedCharacterIndex, SlotIndex, HotbarBindings[SlotIndex]);
	}

	UGridSpellbookUILibrary::BuildProductionSpellbookEntries(CharacterSpellbook, HotbarBindings, SpellEntries);

	NotifySpellbookRefreshed();
}

void UGridSpellbookWidget::RebuildSpellEntryWidgets()
{
	if (Text_EmptySpellbook)
	{
		Text_EmptySpellbook->SetVisibility(SpellEntries.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!Panel_SpellEntries)
	{
		return;
	}

	Panel_SpellEntries->ClearChildren();
	if (SpellEntries.IsEmpty() || !SpellEntryWidgetClass)
	{
		return;
	}

	for (const FGridSpellbookEntryView& Entry : SpellEntries)
	{
		UGridSpellbookEntryWidget* EntryWidget = CreateWidget<UGridSpellbookEntryWidget>(GetOwningPlayer(), SpellEntryWidgetClass);
		if (!EntryWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridSpellbookWidget failed to create spell row for %s."), *Entry.SpellId.ToString());
			continue;
		}

		EntryWidget->InitializeSpellEntry(Entry);
		Panel_SpellEntries->AddChild(EntryWidget);
	}
}

int32 UGridSpellbookWidget::GetSpellEntryCount() const
{
	return SpellEntries.Num();
}

bool UGridSpellbookWidget::GetSpellEntry(int32 EntryIndex, FGridSpellbookEntryView& OutEntry) const
{
	if (!SpellEntries.IsValidIndex(EntryIndex))
	{
		OutEntry = FGridSpellbookEntryView();
		return false;
	}

	OutEntry = SpellEntries[EntryIndex];
	return true;
}

EGridSpellHotbarAssignmentResult UGridSpellbookWidget::AssignSpellToHotbar(FName SpellId, int32 TargetSlotIndex)
{
	FGridCharacterSpellbookState CharacterSpellbook;
	if (!InventoryComponent || !GetSelectedCharacterSpellbook(CharacterSpellbook))
	{
		return EGridSpellHotbarAssignmentResult::InvalidCharacter;
	}

	const EGridSpellHotbarAssignmentResult Result =
		UGridSpellbookUILibrary::AssignKnownSpellToHotbar(InventoryComponent, SelectedCharacterIndex, CharacterSpellbook, SpellId, TargetSlotIndex);

	if (Result == EGridSpellHotbarAssignmentResult::Success)
	{
		RefreshSpellbook();
	}
	return Result;
}

EGridSpellHotbarAssignmentResult UGridSpellbookWidget::UnassignSpellFromHotbar(FName SpellId)
{
	FGridCharacterSpellbookState CharacterSpellbook;
	if (!InventoryComponent || !GetSelectedCharacterSpellbook(CharacterSpellbook))
	{
		return EGridSpellHotbarAssignmentResult::InvalidCharacter;
	}

	const EGridSpellHotbarAssignmentResult Result =
		UGridSpellbookUILibrary::UnassignSpellFromHotbar(InventoryComponent, SelectedCharacterIndex, CharacterSpellbook, SpellId);

	if (Result == EGridSpellHotbarAssignmentResult::Success)
	{
		RefreshSpellbook();
	}
	return Result;
}

void UGridSpellbookWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	RefreshSpellbook();
}

void UGridSpellbookWidget::HandleSpellbookChanged()
{
	RefreshSpellbook();
}

bool UGridSpellbookWidget::GetSelectedCharacterSpellbook(FGridCharacterSpellbookState& OutState) const
{
	OutState = FGridCharacterSpellbookState();
	return SpellbookComponent && SelectedCharacterId.IsValid() &&
		SpellbookComponent->GetCharacterSpellbookState(SelectedCharacterId, OutState);
}
