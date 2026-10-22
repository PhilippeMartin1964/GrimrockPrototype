#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponClassAffinityImportCommandlet.generated.h"

/** ARMEMENT01.9.5.2: projects two generated workbook CSVs into the existing weapon catalogue. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponClassAffinityImportCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridWeaponClassAffinityImportCommandlet();
    virtual int32 Main(const FString& Params) override;
};
