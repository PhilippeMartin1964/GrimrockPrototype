#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace RPGCustomRecruitMON205EventCommandTests
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("MON2055_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FTestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2055EventCommandContractTest, "Grimrock.MON20.5.CustomRecruit.EventCommandContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2055EventCommandContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Story companion command keeps serialized value 23"), static_cast<uint8>(EGridObjectCommand::OfferRecruitment), static_cast<uint8>(23));
	TestEqual(TEXT("OpenCustomRecruit appends serialized command value 24"), static_cast<uint8>(EGridObjectCommand::OpenCustomRecruit), static_cast<uint8>(24));

	const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
	TestNotNull(TEXT("Grid command enum exists"), CommandEnum);
	if (CommandEnum)
	{
		TestEqual(TEXT("Lua-style enum lookup resolves OpenCustomRecruit"), CommandEnum->GetValueByNameString(TEXT("OpenCustomRecruit")),
			static_cast<int64>(EGridObjectCommand::OpenCustomRecruit));
	}

	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	TestNotNull(TEXT("Grid object type enum exists"), TypeEnum);
	if (TypeEnum)
	{
		TestEqual(TEXT("CustomRecruiter is exposed by the object type enum"), TypeEnum->GetValueByNameString(TEXT("CustomRecruiter")),
			static_cast<int64>(EGridLevelObjectType::CustomRecruiter));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2055CustomRecruiterArchetypeContractTest, "Grimrock.MON20.5.CustomRecruit.CustomRecruiterArchetypeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2055CustomRecruiterArchetypeContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>();
	Archetype->ArchetypeId = TEXT("CustomRecruiter_Service");
	Archetype->DisplayName = FText::FromString(TEXT("Custom Recruiter"));
	Archetype->SupportedType = EGridLevelObjectType::CustomRecruiter;
	Archetype->PlacementKind = EGridObjectPlacementKind::Center;
	Archetype->Category = TEXT("Recruitment");
	Archetype->ObjectCategory = EGridObjectCategory::Decoration;
	Archetype->bDefaultInitiallyEnabled = true;
	Archetype->bDefaultInitiallyActive = false;
	Archetype->RuntimeActorClass = nullptr;
	Archetype->bIsInteractable = false;
	Archetype->bIsReadable = false;

	TArray<FGridArchetypeValidationMessage> Messages;
	TestTrue(TEXT("Data-only CustomRecruiter archetype validates without RuntimeActorClass"), Archetype->ValidateArchetype(Messages));
	TestFalse(TEXT("CustomRecruiter does not require a runtime actor class"), Archetype->RequiresRuntimeActorClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2055EventCommandMissingPlayerPawnTest, "Grimrock.MON20.5.CustomRecruit.EventCommandMissingPlayerPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2055EventCommandMissingPlayerPawnTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	RPGCustomRecruitMON205EventCommandTests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON20.5.5 test world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor spawns"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;

	const FGuid SourceId(20, 5, 5, 1);
	FGridLevelObjectData Source;
	Source.ObjectId = SourceId;
	Source.Type = EGridLevelObjectType::Trigger;
	Level->Objects.Add(Source);

	const FGuid RecruiterId(20, 5, 5, 2);
	FGridLevelObjectData Recruiter;
	Recruiter.ObjectId = RecruiterId;
	Recruiter.Type = EGridLevelObjectType::CustomRecruiter;
	Level->Objects.Add(Recruiter);

	FGridObjectLink Link;
	Link.SourceObjectId = SourceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.TargetObjectId = RecruiterId;
	Link.Command = EGridObjectCommand::OpenCustomRecruit;
	Level->Links.Add(Link);

	Runtime->LevelAsset = Level;
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	TestNotNull(TEXT("Activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}

	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	TestFalse(
		TEXT("OpenCustomRecruit rejects when no player party pawn exists"), Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));
	TestFalse(TEXT("Rejected CustomRecruiter command does not create stateful activation"), Activation->GetActiveObjectIds().Contains(RecruiterId));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
