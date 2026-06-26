#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "GrimrockGameInstance.generated.h"

/**
 * Persistent runtime state shared across map transitions.
 *
 * The main menu uses this object to store the requested startup mode before
 * opening the runtime level. The runtime pawn then consumes that mode when the
 * gameplay map starts.
 */
UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGrimrockGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
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

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Startup", meta = (AllowPrivateAccess = "true"))
    EGrimrockPartyStartupMode PendingStartupMode = EGrimrockPartyStartupMode::Continue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true"))
    FString DefaultPartySaveSlotName = TEXT("GrimrockParty");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu|Save", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
    int32 DefaultPartySaveUserIndex = 0;
};
