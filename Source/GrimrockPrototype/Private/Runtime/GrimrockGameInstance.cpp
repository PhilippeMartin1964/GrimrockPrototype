#include "Runtime/GrimrockGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Save/GrimrockPartySaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockGameInstance, Log, All);

namespace
{
	bool IsPartyInventoryStateLoadable(const FGridPartyInventoryState& PartyState)
	{
		if (!PartyState.bInitialCharacterCreationCompleted)
		{
			return false;
		}

		if (PartyState.ActiveCharacters.Num() < 1 || PartyState.MaxActiveCharacters < PartyState.ActiveCharacters.Num())
		{
			return false;
		}

		if (PartyState.ActiveEquipment.Num() != PartyState.ActiveCharacters.Num())
		{
			return false;
		}

		if (!PartyState.ActiveCharacters.IsValidIndex(PartyState.SelectedCharacterIndex))
		{
			return false;
		}

		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			if (!Character.CharacterId.IsValid())
			{
				return false;
			}
		}

		return true;
	}
}

UGrimrockGameInstance::UGrimrockGameInstance()
{
	ConfiguredPartySaveSlotNames.Add(TEXT("GrimrockParty"));
	ConfiguredPartySaveSlotNames.Add(TEXT("GrimrockParty_2"));
	ConfiguredPartySaveSlotNames.Add(TEXT("GrimrockParty_3"));
}

void UGrimrockGameInstance::SetPendingStartupMode(EGrimrockPartyStartupMode NewMode)
{
	PendingStartupMode = NewMode;

	if (PendingStartupMode == EGrimrockPartyStartupMode::NewGame)
	{
		ResetPendingLoadSlot();
	}

	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingStartupMode Set Mode=%d"), static_cast<int32>(PendingStartupMode));
}

EGrimrockPartyStartupMode UGrimrockGameInstance::GetPendingStartupMode() const
{
	return PendingStartupMode;
}

EGrimrockPartyStartupMode UGrimrockGameInstance::ConsumePendingStartupMode()
{
	const EGrimrockPartyStartupMode ConsumedMode = PendingStartupMode;
	PendingStartupMode = EGrimrockPartyStartupMode::Continue;

	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingStartupMode Consumed Mode=%d NextMode=%d"), static_cast<int32>(ConsumedMode),
		static_cast<int32>(PendingStartupMode));

	return ConsumedMode;
}

void UGrimrockGameInstance::ClearPendingStartupMode()
{
	PendingStartupMode = EGrimrockPartyStartupMode::Continue;
	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingStartupMode Cleared"));
}

void UGrimrockGameInstance::RequestReturnToMainMenu(const UObject* WorldContextObject)
{
	PendingStartupMode = EGrimrockPartyStartupMode::Continue;
	ResetPendingLoadSlot();

	if (MainMenuLevelName.IsNone())
	{
		UE_LOG(LogGrimrockGameInstance, Error, TEXT("GrimrockGameInstance ReturnToMainMenu Failed Reason=NoMainMenuLevelName"));
		return;
	}

	const UObject* EffectiveWorldContext = WorldContextObject ? WorldContextObject : this;
	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance ReturnToMainMenu Level=%s"), *MainMenuLevelName.ToString());
	UGameplayStatics::OpenLevel(EffectiveWorldContext, MainMenuLevelName, true);
}

FName UGrimrockGameInstance::GetMainMenuLevelName() const
{
	return MainMenuLevelName;
}

bool UGrimrockGameInstance::HasDefaultPartySaveGame() const
{
	return HasPartySaveGame(DefaultPartySaveSlotName, DefaultPartySaveUserIndex);
}

bool UGrimrockGameInstance::HasPartySaveGame(const FString& SlotName, int32 UserIndex) const
{
	if (!DoesPartySaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	UGrimrockPartySaveGame* SaveGame = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!SaveGame)
	{
		UE_LOG(LogGrimrockGameInstance, Verbose, TEXT("[MON18.9.3] SlotProbe Slot=%s UserIndex=%d Result=Rejected Reason=LoadFailedOrWrongClass"), *SlotName,
			UserIndex);
		return false;
	}

	if (!SaveGame->IsCompatible())
	{
		UE_LOG(LogGrimrockGameInstance, Verbose,
			TEXT("[MON18.9.3] SlotProbe Slot=%s UserIndex=%d Result=Rejected Reason=IncompatibleSave Version=%d Detail=%s"), *SlotName, UserIndex,
			SaveGame->SaveVersion, *SaveGame->GetLoadError());
		return false;
	}

	if (!IsPartyInventoryStateLoadable(SaveGame->PartyInventoryState))
	{
		UE_LOG(LogGrimrockGameInstance, Verbose,
			TEXT("[MON18.9.3] SlotProbe Slot=%s UserIndex=%d Result=Rejected Reason=PartyInventoryStateNotLoadable Version=%d ActiveCharacters=%d"), *SlotName,
			UserIndex, SaveGame->SaveVersion, SaveGame->PartyInventoryState.ActiveCharacters.Num());
		return false;
	}

	return true;
}

bool UGrimrockGameInstance::DoesDefaultPartySaveGameExist() const
{
	return DoesPartySaveGameExist(DefaultPartySaveSlotName, DefaultPartySaveUserIndex);
}

bool UGrimrockGameInstance::DoesPartySaveGameExist(const FString& SlotName, int32 UserIndex) const
{
	return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

FString UGrimrockGameInstance::GetDefaultPartySaveSlotName() const
{
	return DefaultPartySaveSlotName;
}

int32 UGrimrockGameInstance::GetDefaultPartySaveUserIndex() const
{
	return DefaultPartySaveUserIndex;
}

TArray<FGrimrockSaveSlotInfo> UGrimrockGameInstance::GetPartySaveSlotInfos() const
{
	TArray<FGrimrockSaveSlotInfo> SlotInfos;
	TSet<FString> AddedSlotNames;

	auto AddSlotInfo = [this, &SlotInfos, &AddedSlotNames](const FString& SlotName, int32 UserIndex, bool bIsDefaultSlot)
	{
		if (SlotName.IsEmpty() || AddedSlotNames.Contains(SlotName))
		{
			return;
		}

		AddedSlotNames.Add(SlotName);
		SlotInfos.Add(MakeSaveSlotInfo(SlotName, UserIndex, bIsDefaultSlot, SlotInfos.Num() + 1));
	};

	AddSlotInfo(DefaultPartySaveSlotName, DefaultPartySaveUserIndex, true);

	for (const FString& SlotName : ConfiguredPartySaveSlotNames)
	{
		AddSlotInfo(SlotName, DefaultPartySaveUserIndex, SlotName == DefaultPartySaveSlotName);
	}

	return SlotInfos;
}

TArray<FGrimrockSaveSlotInfo> UGrimrockGameInstance::GetExistingPartySaveSlotInfos() const
{
	TArray<FGrimrockSaveSlotInfo> ExistingSlots;

	for (const FGrimrockSaveSlotInfo& SlotInfo : GetPartySaveSlotInfos())
	{
		if (SlotInfo.bIsLoadable)
		{
			ExistingSlots.Add(SlotInfo);
		}
	}

	return ExistingSlots;
}

bool UGrimrockGameInstance::RequestContinueDefaultPartySaveSlot()
{
	return RequestLoadPartySaveSlot(DefaultPartySaveSlotName, DefaultPartySaveUserIndex);
}

bool UGrimrockGameInstance::RequestLoadPartySaveSlot(const FString& SlotName, int32 UserIndex)
{
	if (!HasPartySaveGame(SlotName, UserIndex))
	{
		UE_LOG(LogGrimrockGameInstance, Warning, TEXT("GrimrockGameInstance LoadSlot Request Failed Slot=%s UserIndex=%d Reason=SaveNotLoadable"), *SlotName,
			UserIndex);
		return false;
	}

	SetPendingLoadSlot(SlotName, UserIndex);
	SetPendingStartupMode(EGrimrockPartyStartupMode::Continue);

	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance LoadSlot Requested Slot=%s UserIndex=%d"), *SlotName, UserIndex);
	return true;
}

void UGrimrockGameInstance::SetPendingLoadSlot(const FString& SlotName, int32 UserIndex)
{
	PendingLoadSlotName = SlotName;
	PendingLoadSlotUserIndex = UserIndex;
	bHasPendingLoadSlot = !PendingLoadSlotName.IsEmpty();

	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingLoadSlot Set Slot=%s UserIndex=%d"), *PendingLoadSlotName, PendingLoadSlotUserIndex);
}

bool UGrimrockGameInstance::HasPendingLoadSlot() const
{
	return bHasPendingLoadSlot;
}

bool UGrimrockGameInstance::ConsumePendingLoadSlot(FString& OutSlotName, int32& OutUserIndex)
{
	if (!bHasPendingLoadSlot)
	{
		OutSlotName.Empty();
		OutUserIndex = DefaultPartySaveUserIndex;
		return false;
	}

	OutSlotName = PendingLoadSlotName;
	OutUserIndex = PendingLoadSlotUserIndex;
	ResetPendingLoadSlot();

	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingLoadSlot Consumed Slot=%s UserIndex=%d"), *OutSlotName, OutUserIndex);
	return true;
}

void UGrimrockGameInstance::ClearPendingLoadSlot()
{
	ResetPendingLoadSlot();
	UE_LOG(LogGrimrockGameInstance, Log, TEXT("GrimrockGameInstance PendingLoadSlot Cleared"));
}

FGrimrockSaveSlotInfo UGrimrockGameInstance::MakeSaveSlotInfo(const FString& SlotName, int32 UserIndex, bool bIsDefaultSlot, int32 DisplayIndex) const
{
	FGrimrockSaveSlotInfo SlotInfo;
	SlotInfo.SlotName = SlotName;
	SlotInfo.UserIndex = UserIndex;
	SlotInfo.bExists = DoesPartySaveGameExist(SlotName, UserIndex);
	SlotInfo.bIsLoadable = HasPartySaveGame(SlotName, UserIndex);
	SlotInfo.bIsDefaultSlot = bIsDefaultSlot;
	SlotInfo.DisplayName =
		bIsDefaultSlot ? FText::FromString(TEXT("Sauvegarde principale")) : FText::FromString(FString::Printf(TEXT("Sauvegarde %d"), DisplayIndex));
	return SlotInfo;
}

void UGrimrockGameInstance::ResetPendingLoadSlot()
{
	bHasPendingLoadSlot = false;
	PendingLoadSlotName.Empty();
	PendingLoadSlotUserIndex = DefaultPartySaveUserIndex;
}
