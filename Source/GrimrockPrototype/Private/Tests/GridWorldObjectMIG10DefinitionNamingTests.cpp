#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWorldObjectMIG10DefinitionNamingTest,
	"Grimrock.WorldObjects.MIG10.DefinitionNaming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG10DefinitionNamingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	TestEqual(TEXT("The native definition has its final reflected identity"), DefinitionClass->GetPathName(),
		FString(TEXT("/Script/GrimrockPrototype.GridWorldObjectDefinitionAsset")));
	TestNotNull(TEXT("Definition owns DefinitionId"), DefinitionClass->FindPropertyByName(TEXT("DefinitionId")));
	// These old serialized names are intentional negative assertions, not compatibility APIs.
	TestNull(TEXT("The old identity property is absent"), DefinitionClass->FindPropertyByName(TEXT("ArchetypeId")));
	TestEqual(TEXT("Serialized identity redirects after the class has already been renamed"),
		FProperty::FindRedirectedPropertyName(DefinitionClass, TEXT("ArchetypeId")), FName(TEXT("DefinitionId")));
	TestNull(TEXT("No old reflected class or wrapper exists"),
		FindObject<UClass>(nullptr, TEXT("/Script/GrimrockPrototype.GridObjectArchetypeAsset")));
	TestNotNull(TEXT("Placements retain their qualified definition reference"),
		FGridWorldObjectInstance::StaticStruct()->FindPropertyByName(TEXT("WorldObjectDefinitionId")));
	const FObjectPropertyBase* PaletteDefinition = FindFProperty<FObjectPropertyBase>(
		FGridObjectPaletteEntry::StaticStruct(), TEXT("DefaultWorldObjectDefinition"));
	TestTrue(TEXT("Palette references the unique world-object definition type"),
		PaletteDefinition && PaletteDefinition->PropertyClass == DefinitionClass);
	const FArrayProperty* RuntimeDefinitions = FindFProperty<FArrayProperty>(
		AGridLevelRuntimeActor::StaticClass(), TEXT("WorldObjectDefinitions"));
	const FObjectPropertyBase* RuntimeDefinition = RuntimeDefinitions ? CastField<FObjectPropertyBase>(RuntimeDefinitions->Inner) : nullptr;
	TestTrue(TEXT("Runtime stores references to the unique world-object definition type"),
		RuntimeDefinition && RuntimeDefinition->PropertyClass == DefinitionClass);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
