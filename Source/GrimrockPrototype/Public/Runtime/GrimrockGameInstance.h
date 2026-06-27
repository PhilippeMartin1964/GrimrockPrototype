#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "GrimrockGameInstance.generated.h"

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGrimrockSaveSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save")
    FString SlotName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (ClampMin = "0"))
    int32 UserIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save")
    bool bExists = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save")
    bool bIsDefaultSlot = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save")
    FText DisplayName;
};

/**
 * Persistent runtime state shared across map transitions.
 *
 * The main menu uses this object to store the requested startup mode and save
 * slot before opening the runtime level. The runtime pawn then consumes that
 * request when the gameplay map starts.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGrimrockGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UGrimrockGameInstance();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Startup")
    void SetPendingStartupMode(EGrimrockPartyStartupMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Main Menu|Startup")
    EGrimrockPartyStartupMode GetPendingStartupMode() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Startup")
    EGrimrockPartyStartupMode ConsumePendingStartupMode();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Startup")
    void ClearPendingStartupMode();

    UFUNCTION(BlueprintPure, Category = "Main Menu|Save")
    bool HasDefaultPartySaveGame() const;

    UFUNCTION(BlueprintPure, Category = "Main Menu|Save")
    bool HasPartySaveGame(const FString& SlotName, int32 UserIndex) const;

    UFUNCTION(BlueprintPure, Category = "Main Menu|Save")
    FString GetDefaultPartySaveSlotName() const;

    UFUNCTION(BlueprintPure, Category = "Main Menu|Save")
    int32 GetDefaultPartySaveUserIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    TArray<FGrimrockSaveSlotInfo> GetPartySaveSlotInfos() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    TArray<FGrimrockSaveSlotInfo> GetExistingPartySaveSlotInfos() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    bool RequestContinueDefaultPartySaveSlot();

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    bool RequestLoadPartySaveSlot(const FString& SlotName, int32 UserIndex);

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    void SetPendingLoadSlot(const FString& SlotName, int32 UserIndex);

    UFUNCTION(BlueprintPure, Category = "Main Menu|Save")
    bool HasPendingLoadSlot() const;

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    bool ConsumePendingLoadSlot(FString& OutSlotName, int32& OutUserIndex);

    UFUNCTION(BlueprintCallable, Category = "Main Menu|Save")
    void ClearPendingLoadSlot();

private:
    FGrimrockSaveSlotInfo MakeSaveSlotInfo(const FString& SlotName, int32 UserIndex, bool bIsDefaultSlot, int32 DisplayIndex) const;
    void ResetPendingLoadSlot();

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Startup", meta = (AllowPrivateAccess = "true"))
    EGrimrockPartyStartupMode PendingStartupMode = EGrimrockPartyStartupMode::Continue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    FString DefaultPartySaveSlotName = TEXT("GrimrockParty");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 DefaultPartySaveUserIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    TArray<FString> ConfiguredPartySaveSlotNames;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    bool bHasPendingLoadSlot = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    FString PendingLoadSlotName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    int32 PendingLoadSlotUserIndex = 0;
};