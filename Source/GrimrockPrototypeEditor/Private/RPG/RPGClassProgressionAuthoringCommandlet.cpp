#include "RPG/RPGClassProgressionAuthoringCommandlet.h"

#include "RPG/GridEditorAssetSave.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionAuthoring.h"

namespace RPGClassProgressionAuthoringCommandlet
{
	using GridEditorAssetSave::SaveClassAsset;
	const TCHAR* ClassPaths[] = {
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist")
	};

}

URPGClassProgressionAuthoringCommandlet::URPGClassProgressionAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGClassProgressionAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	for (const TCHAR* Path : RPGClassProgressionAuthoringCommandlet::ClassPaths)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Path);
		if (!IsValid(ClassAsset))
		{
			UE_LOG(LogTemp, Error, TEXT("[RPG03.10] Missing production class asset: %s"), Path);
			return 1;
		}
		ClassAsset->Modify();
		FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants(*ClassAsset);
		if (!FRPGClassProgressionAuthoring::HasCanonicalTalentGrants(*ClassAsset) || !ClassAsset->IsValidDefinition())
		{
			UE_LOG(LogTemp, Error, TEXT("[RPG03.10] Invalid class progression after authoring: %s"), Path);
			return 1;
		}
		FString Error;
		if (!RPGClassProgressionAuthoringCommandlet::SaveClassAsset(ClassAsset, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[RPG03.10] %s"), *Error);
			return 1;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("[RPG03.10] Canonical Talent Point progression authored on all six production classes."));
	return 0;
}
