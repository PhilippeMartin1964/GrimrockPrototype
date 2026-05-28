#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridLevelAsset.generated.h"

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridLevelAsset : public UDataAsset
{
    GENERATED_BODY ()

public:

    // --- Grid size ---
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 Width = 32;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 Height = 32;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    float CellSize = 200.f;

    // --- Grid data ---
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Grid")
    TArray<FGridLevelCellData> Cells;

    // --- Gameplay ---
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
    int32 StartCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
    int32 StartCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Start")
    EGridEdge StartFacing = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
    TArray<FGridLevelObjectData> Objects;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
    TArray<FGridObjectLink> Links;

public:

    void EnsureCellCount ();

    bool IsValidCoord (int32 X, int32 Y) const;

    int32 GetIndex (int32 X, int32 Y) const;

    const FGridLevelCellData& GetCell (int32 X, int32 Y) const;

    FGridLevelCellData& GetCellMutable (int32 X, int32 Y);

    void ClearLevel ();

    FGuid AddObject (const FGridLevelObjectData& NewObject);

    bool RemoveObjectById (const FGuid& ObjectId);

    void RemoveLinksForObject (const FGuid& ObjectId);

    void EnsureObjectIds ();
};
