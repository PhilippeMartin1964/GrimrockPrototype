#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLightEmitterTypes.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridLightCONFIG02SinglePointLightAuthorityTest,
	"Grimrock.Items.LIGHT_CONFIG02.SinglePointLightAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLightCONFIG02SinglePointLightAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* ConfigStruct = FGridLightEmitterConfig::StaticStruct();
	if (!TestNotNull(TEXT("FGridLightEmitterConfig reflection exists"), ConfigStruct))
	{
		return false;
	}

	TestNotNull(TEXT("LightIntensity remains the single reflected base intensity"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("LightIntensity")));
	TestNotNull(TEXT("LightRadius remains the single reflected base radius"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("LightRadius")));
	TestNotNull(TEXT("LightColor remains the single reflected base color"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("LightColor")));
	TestNull(TEXT("BaseLightIntensity legacy authority is removed"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("BaseLightIntensity")));
	TestNull(TEXT("BaseAttenuationRadius legacy authority is removed"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("BaseAttenuationRadius")));
	TestNull(TEXT("BaseLightColor legacy authority is removed"),
		FindFProperty<FProperty>(ConfigStruct, TEXT("BaseLightColor")));

	FGridLightEmitterConfig Config;
	Config.bUsePointLight = true;
	Config.LightIntensity = 731.0f;
	Config.LightRadius = 487.0f;
	Config.LightColor = FLinearColor(0.95f, 0.52f, 0.18f, 1.0f);
	Config.FlickerIntensityAmount = 61.0f;
	Config.FlickerRadiusAmount = 14.0f;
	TestTrue(TEXT("Canonical point-light configuration remains valid"), Config.IsValid());

	const UGridItemDefinitionAsset* TorchDefinition = LoadObject<UGridItemDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch"));
	if (!TestNotNull(TEXT("Production DA_Item_Torch loads"), TorchDefinition))
	{
		return false;
	}

	TestTrue(TEXT("Production torch still authors a point light"), TorchDefinition->LightEmitter.bUsePointLight);
	TestTrue(TEXT("Production torch owns a positive canonical intensity"),
		FMath::IsFinite(TorchDefinition->LightEmitter.LightIntensity) && TorchDefinition->LightEmitter.LightIntensity > 0.0f);
	TestTrue(TEXT("Production torch owns a positive canonical radius"),
		FMath::IsFinite(TorchDefinition->LightEmitter.LightRadius) && TorchDefinition->LightEmitter.LightRadius > 0.0f);
	TestFalse(TEXT("Production torch canonical color is not the old black fallback sentinel"),
		TorchDefinition->LightEmitter.LightColor == FLinearColor::Black);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
