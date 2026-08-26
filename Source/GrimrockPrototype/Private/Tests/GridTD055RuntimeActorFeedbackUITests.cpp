#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridTD055RuntimeFeedbackWorld
	{
		UWorld* World = nullptr;

		FGridTD055RuntimeFeedbackWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("TD055RuntimeFeedback_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD055RuntimeFeedbackWorld()
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

	UObject* GridTD055GetObjectPropertyValue(const UObject* Object, const TCHAR* PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}

		const FObjectProperty* Property = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName);
		return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD055RuntimeActorFeedbackUIContractTest,
	"Grimrock.TechnicalDebt.TD05_5.RuntimeActorFeedbackUI.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD055RuntimeActorFeedbackUIContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD055RuntimeFeedbackWorld TestWorld;
	TestNotNull(TEXT("The transient feedback world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* RuntimeActor = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("The runtime actor is created"), RuntimeActor);
	if (!RuntimeActor)
	{
		return false;
	}

	const UFunction* ShowReadableFunction = RuntimeActor->FindFunction(TEXT("ShowReadableMessage"));
	const UFunction* HasReadableFunction = RuntimeActor->FindFunction(TEXT("HasActiveReadableMessage"));
	const UFunction* DismissReadableFunction = RuntimeActor->FindFunction(TEXT("DismissReadableMessage"));
	const UFunction* HideReadableFunction = RuntimeActor->FindFunction(TEXT("HideReadableMessage"));
	const UFunction* ShowInteractionFunction = RuntimeActor->FindFunction(TEXT("ShowInteractionFeedback"));
	const UFunction* HideInteractionFunction = RuntimeActor->FindFunction(TEXT("HideInteractionFeedback"));
	const UFunction* ShowCombatFunction = RuntimeActor->FindFunction(TEXT("ShowCombatFeedback"));
	const UFunction* HideCombatFunction = RuntimeActor->FindFunction(TEXT("HideCombatFeedback"));

	TestNotNull(TEXT("ShowReadableMessage remains reflected"), ShowReadableFunction);
	TestNotNull(TEXT("HasActiveReadableMessage remains reflected"), HasReadableFunction);
	TestNotNull(TEXT("DismissReadableMessage remains reflected"), DismissReadableFunction);
	TestNotNull(TEXT("HideReadableMessage remains reflected"), HideReadableFunction);
	TestNotNull(TEXT("ShowInteractionFeedback remains reflected"), ShowInteractionFunction);
	TestNotNull(TEXT("HideInteractionFeedback remains reflected"), HideInteractionFunction);
	TestNotNull(TEXT("ShowCombatFeedback remains reflected"), ShowCombatFunction);
	TestNotNull(TEXT("HideCombatFeedback remains reflected"), HideCombatFunction);

	TestTrue(TEXT("ShowReadableMessage remains BlueprintCallable"),
		ShowReadableFunction && ShowReadableFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("HasActiveReadableMessage remains BlueprintPure"),
		HasReadableFunction && HasReadableFunction->HasAnyFunctionFlags(FUNC_BlueprintPure));
	TestTrue(TEXT("DismissReadableMessage remains BlueprintCallable"),
		DismissReadableFunction && DismissReadableFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("ShowInteractionFeedback remains BlueprintCallable"),
		ShowInteractionFunction && ShowInteractionFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	TestTrue(TEXT("ShowCombatFeedback remains BlueprintCallable"),
		ShowCombatFunction && ShowCombatFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable));

	TestFalse(TEXT("Readable message auto-hide remains disabled by default"), RuntimeActor->bReadableMessageAutoHide);
	TestEqual(TEXT("Readable message duration remains four seconds by default"), RuntimeActor->ReadableMessageDuration, 4.0f);
	TestNull(TEXT("Readable widget class remains unconfigured by default"), RuntimeActor->ReadableMessageWidgetClass.Get());
	TestNull(TEXT("Interaction feedback widget class remains unconfigured by default"), RuntimeActor->InteractionFeedbackWidgetClass.Get());
	TestNull(TEXT("Combat feedback widget class remains unconfigured by default"), RuntimeActor->CombatFeedbackWidgetClass.Get());

	TestFalse(TEXT("No readable message is active initially"), RuntimeActor->HasActiveReadableMessage());
	TestFalse(TEXT("Dismissing an inactive readable message remains a safe false result"), RuntimeActor->DismissReadableMessage());

	RuntimeActor->ShowReadableMessage(FText::GetEmpty());
	RuntimeActor->HideReadableMessage();
	TestFalse(TEXT("Empty readable text remains a no-op"), RuntimeActor->HasActiveReadableMessage());

	RuntimeActor->ShowInteractionFeedback(FText::FromString(TEXT("TD05 interaction feedback")), 0.01f);
	RuntimeActor->HideInteractionFeedback();

	FGridPlayerAttackFeedbackRequest CombatFeedback;
	CombatFeedback.PrimaryText = FText::FromString(TEXT("TD05 combat feedback"));
	CombatFeedback.DetailText = FText::FromString(TEXT("detail"));
	CombatFeedback.DurationSeconds = 0.01f;
	RuntimeActor->ShowCombatFeedback(CombatFeedback);
	RuntimeActor->HideCombatFeedback();

	TestNull(TEXT("Readable feedback keeps no active widget without a configured class"),
		GridTD055GetObjectPropertyValue(RuntimeActor, TEXT("ActiveReadableMessageWidget")));
	TestNull(TEXT("Interaction feedback keeps no active widget without a configured class"),
		GridTD055GetObjectPropertyValue(RuntimeActor, TEXT("ActiveInteractionFeedbackWidget")));
	TestNull(TEXT("Combat feedback keeps no active widget without a configured class"),
		GridTD055GetObjectPropertyValue(RuntimeActor, TEXT("ActiveCombatFeedbackWidget")));

	const FObjectProperty* ActiveReadableProperty =
		FindFProperty<FObjectProperty>(RuntimeActor->GetClass(), TEXT("ActiveReadableMessageWidget"));
	const FObjectProperty* ActiveInteractionProperty =
		FindFProperty<FObjectProperty>(RuntimeActor->GetClass(), TEXT("ActiveInteractionFeedbackWidget"));
	const FObjectProperty* ActiveCombatProperty =
		FindFProperty<FObjectProperty>(RuntimeActor->GetClass(), TEXT("ActiveCombatFeedbackWidget"));
	TestTrue(TEXT("Readable active widget state remains transient"),
		ActiveReadableProperty && ActiveReadableProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Interaction active widget state remains transient"),
		ActiveInteractionProperty && ActiveInteractionProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Combat active widget state remains transient"),
		ActiveCombatProperty && ActiveCombatProperty->HasAnyPropertyFlags(CPF_Transient));

	return true;
}

#endif
