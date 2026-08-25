#include "UI/ReadableMessageWidget.h"

#include "Components/TextBlock.h"

void UReadableMessageWidget::SetReadableText(const FText& InText)
{
	if (ReadableTextBlock)
	{
		ReadableTextBlock->SetText(InText);
	}
}
