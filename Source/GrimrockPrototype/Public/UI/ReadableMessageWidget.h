#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReadableMessageWidget.generated.h"

class UTextBlock;

UCLASS()
class GRIMROCKPROTOTYPE_API UReadableMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Readable")
	void SetReadableText(const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Readable")
	TObjectPtr<UTextBlock> ReadableTextBlock;
};
