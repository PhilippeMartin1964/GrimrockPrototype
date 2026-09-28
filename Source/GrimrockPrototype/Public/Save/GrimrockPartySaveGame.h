#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartySaveGame.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockPartySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** MON21.6.5: map exploration cells + discovered secret identities are durable. */
	static constexpr int32 CurrentSaveVersion = 23;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FGridPartyInventoryState PartyInventoryState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FGridDungeonRuntimeState DungeonRuntimeState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FName CurrentDungeonLevelId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 PartyCellX = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 PartyCellY = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	EGridEdge PartyFacing = EGridEdge::North;

	virtual void Serialize(FArchive& Ar) override;

	bool ValidateCurrentState(FText& OutError) const;

	bool IsCompatible() const
	{
		return bLoadValid && SaveVersion == CurrentSaveVersion;
	}

	bool IsLoadValid() const
	{
		return bLoadValid;
	}

	const FString& GetLoadError() const
	{
		return LoadError;
	}

private:
	bool bLoadValid = true;
	FString LoadError;
};
