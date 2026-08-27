#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterIdentityPersistence.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07344Normalization
{
	struct FCacheGuard
	{
		FCacheGuard() { FRPGAuthoringIdentityResolver::ResetRuntimeCache(); }
		~FCacheGuard() { FRPGAuthoringIdentityResolver::ResetRuntimeCache(); }
	};

	TSoftObjectPtr<UTexture2D> Texture(const TCHAR* Path)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Path));
	}

	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* Definition = NewObject<URPGClassAsset>(GetTransientPackage());
		Definition->ClassId = TEXT("TD07344_Mage");
		Definition->DisplayName = FText::FromString(TEXT("Mage"));
		Definition->BaseAttributes = FRPGAttributes{ 8, 12, 10, 15, 12, 9 };
		Definition->HealthAtLevelOne = 8;
		Definition->HealthPerLevel = 4;
		Definition->ManaAtLevelOne = 18;
		Definition->ManaPerLevel = 8;
		return Definition;
	}

	URPGRaceAsset* MakeRace()
	{
		URPGRaceAsset* Definition = NewObject<URPGRaceAsset>(GetTransientPackage());
		Definition->RaceId = TEXT("TD07344_Elf");
		Definition->DisplayName = FText::FromString(TEXT("Elfe"));
		return Definition;
	}

	URPGCharacterPortraitSetAsset* MakePortraitSet()
	{
		URPGCharacterPortraitSetAsset* Set = NewObject<URPGCharacterPortraitSetAsset>(GetTransientPackage());
		Set->RaceId = TEXT("TD07344_Elf");
		FRPGCharacterPortraitVariant Variant;
		Variant.VariantId = TEXT("Female_01");
		Variant.Portrait = Texture(TEXT("/Game/TD07344/T_Portrait_Canonical.T_Portrait_Canonical"));
		Set->FemalePortraits.Add(Variant);
		return Set;
	}

	URPGClassVisualAsset* MakeClassVisual()
	{
		URPGClassVisualAsset* Visual = NewObject<URPGClassVisualAsset>(GetTransientPackage());
		Visual->ClassId = TEXT("TD07344_Mage");
		Visual->ClassIcon = Texture(TEXT("/Game/TD07344/T_ClassIcon_Canonical.T_ClassIcon_Canonical"));
		return Visual;
	}

	void RememberAll(URPGClassAsset* ClassDefinition, URPGRaceAsset* RaceDefinition,
		URPGCharacterPortraitSetAsset* PortraitSet, URPGClassVisualAsset* ClassVisual)
	{
		FRPGAuthoringIdentityResolver::RememberClassDefinition(ClassDefinition);
		FRPGAuthoringIdentityResolver::RememberRaceDefinition(RaceDefinition);
		FRPGAuthoringIdentityResolver::RememberPortraitSet(PortraitSet);
		FRPGAuthoringIdentityResolver::RememberClassVisual(ClassVisual);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07344SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_4_4.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07344SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	auto IsTransient = [CharacterStruct](const TCHAR* Name)
	{
		const FProperty* Property = FindFProperty<FProperty>(CharacterStruct, Name);
		return Property && Property->HasAnyPropertyFlags(CPF_Transient);
	};
	auto IsDurable = [CharacterStruct](const TCHAR* Name)
	{
		const FProperty* Property = FindFProperty<FProperty>(CharacterStruct, Name);
		return Property && !Property->HasAnyPropertyFlags(CPF_Transient);
	};

	TestTrue(TEXT("ClassId is durable"), IsDurable(TEXT("ClassId")));
	TestTrue(TEXT("RaceId is durable"), IsDurable(TEXT("RaceId")));
	TestTrue(TEXT("PortraitGender is durable"), IsDurable(TEXT("PortraitGender")));
	TestTrue(TEXT("PortraitVariantId is durable"), IsDurable(TEXT("PortraitVariantId")));
	TestTrue(TEXT("ClassDefinition is transient"), IsTransient(TEXT("ClassDefinition")));
	TestTrue(TEXT("ClassDisplayName is transient"), IsTransient(TEXT("ClassDisplayName")));
	TestTrue(TEXT("RaceDisplayName is transient"), IsTransient(TEXT("RaceDisplayName")));
	TestTrue(TEXT("Portrait is transient"), IsTransient(TEXT("Portrait")));
	TestTrue(TEXT("ClassIcon is transient"), IsTransient(TEXT("ClassIcon")));
	TestEqual(TEXT("TD07.3.4.4 opens SaveGame v22"), UGrimrockPartySaveGame::CurrentSaveVersion, 22);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07344CanonicalVisualResolverTest,
	"Grimrock.TechnicalDebt.TD07_3_4_4.Normalization.CanonicalVisualResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07344CanonicalVisualResolverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07344Normalization;
	FCacheGuard Guard;

	URPGClassAsset* ClassDefinition = MakeClass();
	URPGRaceAsset* RaceDefinition = MakeRace();
	URPGCharacterPortraitSetAsset* PortraitSet = MakePortraitSet();
	URPGClassVisualAsset* ClassVisual = MakeClassVisual();
	RememberAll(ClassDefinition, RaceDefinition, PortraitSet, ClassVisual);

	const TSoftObjectPtr<UTexture2D> Portrait =
		FRPGAuthoringIdentityResolver::ResolvePortraitVisual(TEXT("TD07344_Elf"), ERPGCharacterPortraitGender::Female, TEXT("Female_01"));
	const TSoftObjectPtr<UTexture2D> ClassIcon = FRPGAuthoringIdentityResolver::ResolveClassIcon(TEXT("TD07344_Mage"));

	TestTrue(TEXT("Portrait resolves from Race/Gender/VariantId"), Portrait.ToSoftObjectPath() ==
		PortraitSet->FemalePortraits[0].Portrait.ToSoftObjectPath());
	TestTrue(TEXT("Class icon resolves from ClassId"), ClassIcon.ToSoftObjectPath() == ClassVisual->ClassIcon.ToSoftObjectPath());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07344VisualRoundTripTest,
	"Grimrock.TechnicalDebt.TD07_3_4_4.Normalization.SaveRoundTripRehydratesVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07344VisualRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07344Normalization;
	FCacheGuard Guard;

	URPGClassAsset* ClassDefinition = MakeClass();
	URPGRaceAsset* RaceDefinition = MakeRace();
	URPGCharacterPortraitSetAsset* PortraitSet = MakePortraitSet();
	URPGClassVisualAsset* ClassVisual = MakeClassVisual();
	RememberAll(ClassDefinition, RaceDefinition, PortraitSet, ClassVisual);

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Aelwen"));
	Character.ClassId = ClassDefinition->ClassId;
	Character.ClassDefinition = ClassDefinition;
	Character.ClassDisplayName = ClassDefinition->DisplayName;
	Character.RaceId = RaceDefinition->RaceId;
	Character.RaceDisplayName = RaceDefinition->DisplayName;
	Character.PortraitGender = ERPGCharacterPortraitGender::Female;
	Character.PortraitVariantId = TEXT("Female_01");
	Character.Portrait = Texture(TEXT("/Game/TD07344/T_Portrait_STALE.T_Portrait_STALE"));
	Character.ClassIcon = Texture(TEXT("/Game/TD07344/T_ClassIcon_STALE.T_ClassIcon_STALE"));
	Character.Level = 1;
	Character.Experience = 0;
	Character.LastAcknowledgedLevel = 1;
	Character.Attributes = ClassDefinition->BaseAttributes;
	Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, 1);
	Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, ClassDefinition);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState.ActiveCharacters.Add(Character);
	Save->PartyInventoryState.ActiveEquipment.SetNum(1);

	TArray<uint8> Bytes;
	TestTrue(TEXT("v22 serializes with stale transient visual caches"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("v22 loads"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	const FGridCharacterInventoryState& Restored = Loaded->PartyInventoryState.ActiveCharacters[0];
	TestTrue(TEXT("Portrait is rebuilt canonically rather than serialized stale cache"),
		Restored.Portrait.ToSoftObjectPath() == PortraitSet->FemalePortraits[0].Portrait.ToSoftObjectPath());
	TestTrue(TEXT("ClassIcon is rebuilt canonically rather than serialized stale cache"),
		Restored.ClassIcon.ToSoftObjectPath() == ClassVisual->ClassIcon.ToSoftObjectPath());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07344AuthoringCleanupTest,
	"Grimrock.TechnicalDebt.TD07_3_4_4.Normalization.AuthoringCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07344AuthoringCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("Story Companion no longer authors a duplicate Portrait field"),
		FindFProperty<FProperty>(URPGStoryCompanionAsset::StaticClass(), TEXT("Portrait")));
	TestNull(TEXT("Story Companion no longer authors a duplicate ClassIcon field"),
		FindFProperty<FProperty>(URPGStoryCompanionAsset::StaticClass(), TEXT("ClassIcon")));

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 21;
	FText Error;
	TestFalse(TEXT("Previous v21 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v21 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation never rewrites v21"), Previous->SaveVersion, 21);
	TestTrue(TEXT("v21 rejection reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
