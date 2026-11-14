#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridEditorLooseItemPreviewReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ZEditorLooseItemPreview,
	"Grimrock.RPG.ARM.CLEAN06_3.EditorLooseItemCanonicalPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ZEditorLooseItemPreview::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Production ItemSystem exists"), Root.Get())) return false;
	UGridItemSystemAsset* Configured =
		GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
	if (!TestNotNull(TEXT("Editor fallback uses the configured ItemSystem"), Configured)) return false;
	TestTrue(TEXT("Editor fallback and runtime share exactly one configured root"),
		Configured == Root.Get());
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->bUseWeaponCatalogue = false; // Obsolete flag deliberately disagrees with real ARM.
	Old->DefaultWeaponItemLevel = 3;
	const FName ArmIds[] = {TEXT("Weapon_ThrowingKnife_01"),
		TEXT("Weapon_GreatAxe_01"), TEXT("Weapon_RoundShield_01")};
	for (const FName ArmId : ArmIds)
	{
		Old->ItemDefinitionId = ArmId;
		FGridLooseItemInstance Placement;
		Placement.InstanceId = FGuid::NewGuid();
		Placement.ItemDefinition = Old.Get();
		for (const int32 Level : {1, 20, 0})
		{
			Placement.ItemLevel = Level;
			const int32 ExpectedLevel = Level == 0 ? 3 : Level;
			FGridResolvedItem Expected;
			if (!TestTrue(TEXT("Effective ARM level resolves"),
				Registry->ResolveItem(ArmId, ExpectedLevel, Expected))) return false;
			const FGridEditorLooseItemPreviewReadView Preview =
				FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), Placement);
			TestTrue(TEXT("Known placement stays canonical"), Preview.bCanonicalIdentity);
			TestTrue(TEXT("Known placement resolves"), Preview.bResolved);
			TestEqual(TEXT("Preview level mirrors world materialization"), Preview.EffectiveItemLevel, ExpectedLevel);
			TestEqual(TEXT("Editor preview uses global authored world mesh"),
				Preview.WorldMesh.ToSoftObjectPath().ToString(),
				Expected.WorldMesh.ToSoftObjectPath().ToString());
			TestTrue(TEXT("Editor preview uses canonical world visual transform"),
				Preview.WorldTransform.Equals(Expected.WorldVisualTransform));
		}
		Placement.ItemLevel = 21;
		const FGridEditorLooseItemPreviewReadView Invalid =
			FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), Placement);
		TestTrue(TEXT("Registered invalid level stays classified"), Invalid.bCanonicalIdentity);
		TestFalse(TEXT("Registered invalid level cannot use old world visual"), Invalid.bResolved);
		TestTrue(TEXT("Invalid level has no mesh"), Invalid.WorldMesh.IsNull());
	}
	Old->bUseWeaponCatalogue = true; // Obsolete flag deliberately disagrees with ammo N0.
	Old->ItemDefinitionId = TEXT("Item_Arrows_01");
	FGridLooseItemInstance Arrows;
	Arrows.InstanceId = FGuid::NewGuid();
	Arrows.ItemDefinition = Old.Get();
	Arrows.ItemLevel = 0;
	FGridResolvedItem ExpectedAmmo;
	if (!TestTrue(TEXT("Production unlevelled arrows resolve at N0"),
		Registry->ResolveItem(Old->ItemDefinitionId, 0, ExpectedAmmo))) return false;
	const FGridEditorLooseItemPreviewReadView Ammo =
		FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), Arrows);
	TestTrue(TEXT("Arrow editor preview uses canonical registry"), Ammo.bCanonicalIdentity && Ammo.bResolved);
	TestEqual(TEXT("Arrow remains level zero"), Ammo.EffectiveItemLevel, 0);
	TestEqual(TEXT("Arrow preview uses sole ammo mesh"),
		Ammo.WorldMesh.ToSoftObjectPath().ToString(),
		ExpectedAmmo.WorldMesh.ToSoftObjectPath().ToString());
	// A known invalid inherited ARM default cannot be rescued by an old
	// flag, by N1, or by a legacy mesh.
	Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	Old->DefaultWeaponItemLevel = 0;
	Old->bUseWeaponCatalogue = false;
	FGridLooseItemInstance InvalidDefault;
	InvalidDefault.InstanceId = FGuid::NewGuid();
	InvalidDefault.ItemDefinition = Old.Get();
	InvalidDefault.ItemLevel = 0;
	const FGridEditorLooseItemPreviewReadView BadDefault =
		FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), InvalidDefault);
	TestTrue(TEXT("Known invalid default classified canonical"), BadDefault.bCanonicalIdentity);
	TestFalse(TEXT("Known ARM authored N0 without valid default fails"), BadDefault.bResolved);
	TestTrue(TEXT("Invalid default cannot expose old mesh"), BadDefault.WorldMesh.IsNull());
	Old->DefaultWeaponItemLevel = 3;
	Old->bUseWeaponCatalogue = true;
	Old->ItemDefinitionId = TEXT("Item_Arrows_01");
	Arrows.ItemLevel = 1;
	const FGridEditorLooseItemPreviewReadView InvalidAmmo =
		FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), Arrows);
	TestTrue(TEXT("Invalid ammo level remains classified canonical"), InvalidAmmo.bCanonicalIdentity);
	TestFalse(TEXT("Non-ARM cannot invent N1 despite transitional registry support"), InvalidAmmo.bResolved);
	Arrows.ItemLevel = 0;
	Old->ItemDefinitionId = TEXT("LegacyEditorPreview_063Z");
	const FGridEditorLooseItemPreviewReadView Legacy =
		FGridEditorLooseItemPreviewReadModel::Resolve(Registry.Get(), Arrows);
	TestFalse(TEXT("Unregistered identity retains old editor preview"), Legacy.bCanonicalIdentity);
	return true;
}

#endif
