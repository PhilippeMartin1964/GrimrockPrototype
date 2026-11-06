#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridWorldObjectInstanceVisual.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01174CPlacedNiagara,
	"Grimrock.RPG.ARMEMENT01.17.4C.PlacedObjectNiagaraOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01174CPlacedNiagara::RunTest(const FString&)
{
	TestNotNull(TEXT("Reflected per-instance Niagara configuration"),
		FGridWorldObjectInstanceConfig::StaticStruct()->FindPropertyByName(TEXT("NiagaraOverrides")));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	UNiagaraSystem* DefinitionEffect = NewObject<UNiagaraSystem>();
	UNiagaraSystem* AlternateEffect = NewObject<UNiagaraSystem>();
	Definition->StaticPart.ActiveNiagaraSystem = DefinitionEffect;
	Definition->StaticPart.ActiveNiagaraLocalTransform =
		FTransform(FQuat::Identity, FVector(2.f, 3.f, 4.f), FVector(.5f));

	FGridWorldObjectInstance PlacementA;
	PlacementA.InstanceId = FGuid::NewGuid();
	PlacementA.Type = EGridLevelObjectType::Decoration;
	PlacementA.WorldObjectDefinitionId = TEXT("TestDecoration");

	FGridWorldObjectInstance PlacementB = PlacementA;
	PlacementB.InstanceId = FGuid::NewGuid();

	const FGridWorldObjectStaticPart Inherited = GridWorldObjectInstanceVisual::ResolveStaticPart(
		Definition->StaticPart, PlacementA.InstanceConfig.NiagaraOverrides);
	TestTrue(TEXT("No override keeps definition asset"), Inherited.ActiveNiagaraSystem == DefinitionEffect);
	TestTrue(TEXT("No override keeps original local transform"),
		Inherited.ActiveNiagaraLocalTransform.Equals(Definition->StaticPart.ActiveNiagaraLocalTransform));

	PlacementA.InstanceConfig.NiagaraOverrides.bOverrideScale = true;
	PlacementA.InstanceConfig.NiagaraOverrides.ScaleMultiplier = 1.4f;
	PlacementA.InstanceConfig.NiagaraOverrides.bOverrideOffset = true;
	PlacementA.InstanceConfig.NiagaraOverrides.LocalOffsetCm = FVector(10.f, 20.f, 30.f);
	const FGridWorldObjectStaticPart Local = GridWorldObjectInstanceVisual::ResolveStaticPart(
		Definition->StaticPart, PlacementA.InstanceConfig.NiagaraOverrides);
	TestTrue(TEXT("Scaled only this copy"),
		FMath::IsNearlyEqual(Local.ActiveNiagaraLocalTransform.GetScale3D().X, .7f));
	TestTrue(TEXT("Offset added to definition position"),
		Local.ActiveNiagaraLocalTransform.GetLocation().Equals(FVector(12.f, 23.f, 34.f)));
	TestTrue(TEXT("Definition position untouched"),
		Definition->StaticPart.ActiveNiagaraLocalTransform.GetLocation().Equals(FVector(2.f, 3.f, 4.f)));

	const FGridWorldObjectStaticPart Unrelated = GridWorldObjectInstanceVisual::ResolveStaticPart(
		Definition->StaticPart, PlacementB.InstanceConfig.NiagaraOverrides);
	TestTrue(TEXT("Second placed instance inherits independent default"),
		Unrelated.ActiveNiagaraLocalTransform.Equals(Definition->StaticPart.ActiveNiagaraLocalTransform));

	PlacementA.InstanceConfig.NiagaraOverrides.bOverrideSystem = true;
	PlacementA.InstanceConfig.NiagaraOverrides.NiagaraSystem = AlternateEffect;
	const FGridWorldObjectStaticPart Replaced = GridWorldObjectInstanceVisual::ResolveStaticPart(
		Definition->StaticPart, PlacementA.InstanceConfig.NiagaraOverrides);
	TestTrue(TEXT("Selected instance can select a local-only FireFX Niagara"),
		Replaced.ActiveNiagaraSystem == AlternateEffect);
	PlacementA.InstanceConfig.NiagaraOverrides.NiagaraSystem.Reset();
	const FGridWorldObjectStaticPart Disabled = GridWorldObjectInstanceVisual::ResolveStaticPart(
		Definition->StaticPart, PlacementA.InstanceConfig.NiagaraOverrides);
	TestNull(TEXT("Explicit blank system disables effect for this one instance"), Disabled.ActiveNiagaraSystem.Get());

	FGridRuntimeWorldObjectData Payload(PlacementA);
	TestTrue(TEXT("Sparse VFX overrides reach runtime object"),
		Payload.NiagaraOverrides.bOverrideSystem && Payload.NiagaraOverrides.bOverrideOffset);
	TestTrue(TEXT("Runtime keeps authored offset"),
		Payload.NiagaraOverrides.LocalOffsetCm.Equals(FVector(10.f, 20.f, 30.f)));

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	Level->WorldObjectInstances.Add(PlacementA);
	Level->WorldObjectInstances.Add(PlacementB);
	FGridWorldObjectInstanceConfig FirstCopy = PlacementA.InstanceConfig;
	FirstCopy.NiagaraOverrides.ScaleMultiplier = 2.f;
	TestTrue(TEXT("Selected-object API edits only one instance"),
		Level->SetWorldObjectInstanceConfig(PlacementA.InstanceId, FirstCopy));
	const FGridWorldObjectInstance* StoredA = Level->FindWorldObjectInstanceById(PlacementA.InstanceId);
	const FGridWorldObjectInstance* StoredB = Level->FindWorldObjectInstanceById(PlacementB.InstanceId);
	if (!TestNotNull(TEXT("First copy remains"), StoredA) ||
		!TestNotNull(TEXT("Second copy remains"), StoredB)) return false;
	TestTrue(TEXT("First instance customized scale"),
		FMath::IsNearlyEqual(StoredA->InstanceConfig.NiagaraOverrides.ScaleMultiplier, 2.f));
	TestFalse(TEXT("Second instance did not inherit the override"),
		StoredB->InstanceConfig.NiagaraOverrides.bOverrideScale);
	return true;
}
#endif
