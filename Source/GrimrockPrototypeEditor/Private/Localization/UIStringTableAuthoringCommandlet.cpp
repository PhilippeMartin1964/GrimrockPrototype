#include "Localization/UIStringTableAuthoringCommandlet.h"

#include "Localization/UILocalizationCsvAuthoring.h"

UUIStringTableAuthoringCommandlet::UUIStringTableAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUIStringTableAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;

	bool bChanged = false;
	FString Error;
	if (!FUILocalizationCsvAuthoring::MaterializeProductionAsset(bChanged, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.4A] String Table materialization failed: %s"), *Error);
		return 1;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-LOCALIZATION01.4A] ST_UI materialization succeeded. Changed=%s Source=%s"),
		bChanged ? TEXT("true") : TEXT("false"),
		*FUILocalizationCsvAuthoring::SourceFilename());
	return 0;
}
