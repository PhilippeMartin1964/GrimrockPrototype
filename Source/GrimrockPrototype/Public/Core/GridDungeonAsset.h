#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/GridLevelAsset.h"
#include "GridDungeonAsset.generated.h"

USTRUCT(BlueprintType)
struct FGridDungeonLevelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	FName LevelId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	TObjectPtr<UGridLevelAsset> LevelAsset = nullptr;

	// Logical dungeon position, not necessarily Unreal world position.
	// X/Y = cardinal wings, Z = vertical floor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	FIntVector LogicalPosition = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	bool bEnabled = true;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridDungeonAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (DisplayName = "Dungeon Name"))
	FText DungeonName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (DisplayName = "Author"))
	FText Author;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon", meta = (DisplayName = "Version"))
	FString Version = TEXT("0.1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Start", meta = (DisplayName = "Default Level Id"))
	FName DefaultLevelId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
	TArray<FGridDungeonLevelEntry> Levels;

	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	bool IsValidLevelId(FName LevelId) const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	UGridLevelAsset* GetLevelAssetById(FName LevelId) const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	UGridLevelAsset* GetDefaultLevelAsset() const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Diagnostics")
	FString GetDungeonDiagnostics() const;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Diagnostics")
	FString GetTransitionDiagnostics() const;

	const FGridDungeonLevelEntry* FindLevelEntry(FName LevelId) const;
};
