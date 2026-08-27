#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemTransferService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD072EngineCompatibilityReflectionContractTest,
	"Grimrock.TechnicalDebt.TD07_2.EngineCompatibility.ReflectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD072EngineCompatibilityReflectionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UEnum* TransferResultEnum = StaticEnum<EGridItemTransferResult>();
	TestNotNull(TEXT("EGridItemTransferResult reflection enum exists"), TransferResultEnum);
	if (!TransferResultEnum)
	{
		return false;
	}

	const FString ScriptName = TransferResultEnum->GetMetaData(TEXT("ScriptName"));
	TestEqual(TEXT("EGridItemTransferResult owns a unique scripting name"), ScriptName, FString(TEXT("GridItemTransferResultCode")));

	UScriptStruct* TransferResultStruct = FGridItemTransferResult::StaticStruct();
	TestNotNull(TEXT("FGridItemTransferResult reflection struct exists"), TransferResultStruct);
	if (!TransferResultStruct)
	{
		return false;
	}

	TestEqual(TEXT("FGridItemTransferResult keeps its existing reflected name"), TransferResultStruct->GetName(), FString(TEXT("GridItemTransferResult")));

	return true;
}

#endif
