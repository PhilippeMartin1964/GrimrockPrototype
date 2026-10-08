#include "RPG/RPGProgressionPIESimulator.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/GridPartyInventoryComponent.h"

#if !UE_BUILD_SHIPPING
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GrimrockPartyPawn.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogRPGProgressionPIESimulator, Log, All);

namespace
{
	FString ResolveCharacterLabel(const FGridCharacterInventoryState& Character, int32 CharacterIndex)
	{
		return Character.DisplayName.IsEmpty()
			? FString::Printf(TEXT("Personnage %d"), CharacterIndex + 1)
			: Character.DisplayName.ToString();
	}
}

bool FRPGProgressionPIESimulator::TrySetCharacterLevel(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	int32 TargetLevel,
	FText& OutFeedback)
{
	OutFeedback = FText::GetEmpty();

	if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutFeedback = FText::FromString(TEXT("Personnage sélectionné invalide."));
		return false;
	}

	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 MaximumLevel = URPGCharacterRulesLibrary::GetMaximumLevel();
	if (TargetLevel < MinimumLevel || TargetLevel > MaximumLevel)
	{
		OutFeedback = FText::FromString(
			FString::Printf(TEXT("Niveau cible invalide : %d. Valeurs autorisées : %d à %d."), TargetLevel, MinimumLevel, MaximumLevel));
		return false;
	}

	FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const FString CharacterLabel = ResolveCharacterLabel(Character, CharacterIndex);

	if (!URPGCharacterRulesLibrary::IsLevelExperienceConsistent(Character.Level, Character.Experience))
	{
		OutFeedback = FText::FromString(FString::Printf(
			TEXT("%s possède un état Niveau/XP incohérent : niveau %d, XP %d."),
			*CharacterLabel,
			Character.Level,
			Character.Experience));
		return false;
	}

	if (TargetLevel <= Character.Level)
	{
		OutFeedback = FText::FromString(FString::Printf(
			TEXT("%s est déjà niveau %d. RPG-DEV01 autorise uniquement une progression ascendante."),
			*CharacterLabel,
			Character.Level));
		return false;
	}

	const int32 PreviousLevel = Character.Level;
	const int32 PreviousExperience = Character.Experience;
	const int32 TargetExperience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(TargetLevel);

	Character.Experience = TargetExperience;
	if (!FRPGLevelUpService::ApplyPendingLevelUp(PartyInventoryComponent, CharacterIndex, true))
	{
		Character.Experience = PreviousExperience;
		OutFeedback = FText::FromString(FString::Printf(
			TEXT("La simulation de progression de %s vers le niveau %d a été refusée par le pipeline Level Up."),
			*CharacterLabel,
			TargetLevel));
		return false;
	}

	OutFeedback = FText::FromString(FString::Printf(
		TEXT("%s : niveau %d -> %d, XP %d -> %d."),
		*CharacterLabel,
		PreviousLevel,
		Character.Level,
		PreviousExperience,
		Character.Experience));
	return true;
}

bool FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 TargetLevel,
	FText& OutFeedback)
{
	if (!IsValid(PartyInventoryComponent))
	{
		OutFeedback = FText::FromString(TEXT("Composant d'inventaire du groupe indisponible."));
		return false;
	}

	return TrySetCharacterLevel(
		PartyInventoryComponent,
		PartyInventoryComponent->GetSelectedCharacterIndex(),
		TargetLevel,
		OutFeedback);
}

#if !UE_BUILD_SHIPPING

namespace
{
	void ReportPIEProgressionCommand(bool bSuccess, const FString& Message)
	{
		if (bSuccess)
		{
			UE_LOG(LogRPGProgressionPIESimulator, Display, TEXT("[RPG-DEV01] %s"), *Message);
		}
		else
		{
			UE_LOG(LogRPGProgressionPIESimulator, Warning, TEXT("[RPG-DEV01] %s"), *Message);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				INDEX_NONE,
				6.0f,
				bSuccess ? FColor::Green : FColor::Red,
				FString::Printf(TEXT("RPG-DEV01: %s"), *Message));
		}
	}

	void SetSelectedCharacterLevelForPIE(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->WorldType != EWorldType::PIE)
		{
			ReportPIEProgressionCommand(false, TEXT("commande disponible uniquement en PIE."));
			return;
		}

		if (Args.Num() != 1 || !Args[0].IsNumeric())
		{
			ReportPIEProgressionCommand(false, TEXT("usage : Grimrock.RPG.SetSelectedLevel <niveau 1..20>."));
			return;
		}

		AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(World, 0));
		if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
		{
			ReportPIEProgressionCommand(false, TEXT("pawn du groupe ou inventaire indisponible."));
			return;
		}

		const int32 TargetLevel = FCString::Atoi(*Args[0]);

		// Protect real saves before any authoritative progression mutation.
		// These values belong to the duplicated PIE pawn only.
		const FString PreviousSaveSlotName = PartyPawn->PartySaveSlotName;
		const bool bPreviousAutoSaveOnInventoryClose = PartyPawn->bAutoSaveOnInventoryClose;
		PartyPawn->PartySaveSlotName.Empty();
		PartyPawn->bAutoSaveOnInventoryClose = false;

		FText Feedback;
		if (!FRPGProgressionPIESimulator::TrySetSelectedCharacterLevel(PartyPawn->PartyInventoryComponent, TargetLevel, Feedback))
		{
			PartyPawn->PartySaveSlotName = PreviousSaveSlotName;
			PartyPawn->bAutoSaveOnInventoryClose = bPreviousAutoSaveOnInventoryClose;
			ReportPIEProgressionCommand(false, Feedback.ToString());
			return;
		}

		ReportPIEProgressionCommand(
			true,
			FString::Printf(
				TEXT("%s Sauvegarde désarmée pour cette session PIE."),
				*Feedback.ToString()));
	}

	static FAutoConsoleCommandWithWorldAndArgs GSetSelectedCharacterLevelForPIECommand(
		TEXT("Grimrock.RPG.SetSelectedLevel"),
		TEXT(
			"RPG-DEV01 PIE only. Moves the selected character upward through cumulative XP + FRPGLevelUpService. "
			"Usage: Grimrock.RPG.SetSelectedLevel <1..20>. A successful simulation disarms saving for the PIE pawn."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetSelectedCharacterLevelForPIE),
		ECVF_Default);
}

#endif
