#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/GridAutomationSaveIsolation.h"
#endif

class FGrimrockPrototypeModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
#if WITH_DEV_AUTOMATION_TESTS
		FString Root;
		if (GridAutomationSaveIsolation::ReadRoot(Root))
		{
			if (!GridAutomationSaveIsolation::IsConfigured())
			{
				UE_LOG(LogTemp, Fatal, TEXT("[GrimrockAutomation] Refusing unisolated Saved directory."));
			}
			UE_LOG(LogTemp, Display, TEXT("[GrimrockAutomation] Isolated SaveGames verified."));
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGrimrockPrototypeModule, GrimrockPrototype, "GrimrockPrototype");
