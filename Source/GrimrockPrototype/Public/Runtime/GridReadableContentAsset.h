#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridReadableContentAsset.generated.h"

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridReadableContentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Readable")
	FName ReadableContentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Readable")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Readable", meta = (MultiLine = "true"))
	FText BodyText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Readable")
	FText ShortDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Readable")
	FName ContentType = NAME_None;
};
