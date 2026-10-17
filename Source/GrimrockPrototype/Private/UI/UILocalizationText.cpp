#include "UI/UILocalizationText.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableCoreFwd.h"
#include "Internationalization/Text.h"
#include "Internationalization/TextKey.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogUILocalization, Log, All);

namespace UILocalizationTextPrivate
{
	const TCHAR* ProductionStringTableObjectPath = TEXT("/Game/Localization/StringTables/ST_UI.ST_UI");
}

FText FUILocalizationText::Get(const TCHAR* Key)
{
	if (!Key || !*Key)
	{
		UE_LOG(LogUILocalization, Error, TEXT("UI localization lookup rejected an empty key."));
		return FText::GetEmpty();
	}

	UStringTable* StringTableAsset = LoadObject<UStringTable>(
		nullptr,
		UILocalizationTextPrivate::ProductionStringTableObjectPath);
	if (!IsValid(StringTableAsset))
	{
		UE_LOG(LogUILocalization, Error, TEXT("UI String Table asset is unavailable."));
		return FText::GetEmpty();
	}

	const FTextKey TextKey(Key);
	FString SourceString;
	if (!StringTableAsset->GetStringTable()->GetSourceString(TextKey, SourceString))
	{
		UE_LOG(LogUILocalization, Error, TEXT("UI String Table is missing key '%s'."), Key);
		return FText::GetEmpty();
	}

	return FText::FromStringTable(
		StringTableAsset->GetStringTableId(),
		TextKey,
		EStringTableLoadingPolicy::Find);
}


FText FUILocalizationText::Get(FName Key)
{
	if (Key.IsNone())
	{
		UE_LOG(LogUILocalization, Error, TEXT("UI localization lookup rejected NAME_None."));
		return FText::GetEmpty();
	}

	const FString KeyString = Key.ToString();
	return Get(*KeyString);
}

FText FUILocalizationText::GetOptional(FName Key)
{
	return Key.IsNone() ? FText::GetEmpty() : Get(Key);
}
