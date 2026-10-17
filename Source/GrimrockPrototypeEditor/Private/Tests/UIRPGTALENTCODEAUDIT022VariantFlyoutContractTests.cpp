#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UnrealType.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridTalentVariantIconWidget.h"

namespace UIRPGTalentCodeAudit022
{
	void ValidateMandatoryWidgetBinding(
		FAutomationTestBase& Test,
		UClass* OwnerClass,
		FName PropertyName,
		UClass* ExpectedWidgetClass)
	{
		FProperty* Property = FindFProperty<FProperty>(OwnerClass, PropertyName);
		if (!Test.TestNotNull(
				*FString::Printf(TEXT("%s.%s binding exists"), *OwnerClass->GetName(), *PropertyName.ToString()),
				Property))
		{
			return;
		}

		Test.TestTrue(
			*FString::Printf(TEXT("%s.%s is a mandatory BindWidget"), *OwnerClass->GetName(), *PropertyName.ToString()),
			Property->HasMetaData(TEXT("BindWidget")));
		Test.TestFalse(
			*FString::Printf(TEXT("%s.%s is no longer optional"), *OwnerClass->GetName(), *PropertyName.ToString()),
			Property->HasMetaData(TEXT("BindWidgetOptional")));

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
		if (Test.TestNotNull(
				*FString::Printf(TEXT("%s.%s remains an object binding"), *OwnerClass->GetName(), *PropertyName.ToString()),
				ObjectProperty))
		{
			Test.TestTrue(
				*FString::Printf(TEXT("%s.%s keeps the expected widget type"), *OwnerClass->GetName(), *PropertyName.ToString()),
				ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf(ExpectedWidgetClass));
		}
	}

	void ValidateMaterializedWidgetBlueprint(
		FAutomationTestBase& Test,
		const TCHAR* BlueprintPath,
		UClass* ExpectedNativeClass)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintPath);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s loads"), BlueprintPath), Blueprint))
		{
			return;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Test.TestTrue(
			*FString::Printf(TEXT("%s recompiles with mandatory Talent variant bindings"), BlueprintPath),
			Blueprint->Status != BS_Error);

		if (Test.TestNotNull(
				*FString::Printf(TEXT("%s has a generated widget class"), BlueprintPath),
				Blueprint->GeneratedClass))
		{
			Test.TestTrue(
				*FString::Printf(TEXT("%s keeps the expected native parent"), BlueprintPath),
				Blueprint->GeneratedClass->IsChildOf(ExpectedNativeClass));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit022VariantFlyoutContractTest,
	"Grimrock.UI.RPG.CODEAUDIT02.VariantFlyoutContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit022VariantFlyoutContractTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit022;

	ValidateMandatoryWidgetBinding(
		*this,
		UGridTalentVariantFlyoutWidget::StaticClass(),
		TEXT("Grid_Variants"),
		UUniformGridPanel::StaticClass());

	ValidateMandatoryWidgetBinding(
		*this,
		UGridTalentVariantIconWidget::StaticClass(),
		TEXT("Button_Variant"),
		UButton::StaticClass());

	ValidateMandatoryWidgetBinding(
		*this,
		UGridTalentVariantIconWidget::StaticClass(),
		TEXT("Image_VariantIcon"),
		UImage::StaticClass());

	ValidateMaterializedWidgetBlueprint(
		*this,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantFlyout.WBP_RPGTalentVariantFlyout"),
		UGridTalentVariantFlyoutWidget::StaticClass());

	ValidateMaterializedWidgetBlueprint(
		*this,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Skills/RPG/WBP_RPGTalentVariantIcon.WBP_RPGTalentVariantIcon"),
		UGridTalentVariantIconWidget::StaticClass());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
