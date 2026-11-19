#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

/** Attach the real ItemSystem (or a private transient extension for synthetic
 * fixture IDs) to a world which previously had no GameInstance. No source
 * package or global configuration is modified. Detach before destroying World. */
struct FGridCanonicalLootTestScope
{
	UWorld* World = nullptr;
	TStrongObjectPtr<UGameInstance> GI;
	UGridItemSubsystem* Registry = nullptr;

	bool Attach(UWorld* InWorld, const TArray<FName>& SyntheticIds = {})
	{
		if (World || !IsValid(InWorld) || InWorld->GetGameInstance()) return false;
		World = InWorld;
		GI.Reset(NewObject<UGameInstance>(GetTransientPackage()));
		if (!GI.IsValid()) return false;
		GI->Init();
		World->SetGameInstance(GI.Get());
		Registry = GI->GetSubsystem<UGridItemSubsystem>();
		if (!Registry || !Registry->IsConfigured()) return false;
		if (SyntheticIds.IsEmpty()) return true;
		UGridItemSystemAsset* Production = Registry->LoadConfiguredItemSystemForPreview();
		if (!IsValid(Production)) return false;
		UGridItemSystemAsset* Root = DuplicateObject<UGridItemSystemAsset>(Production, GetTransientPackage());
		if (!IsValid(Root)) return false;
		for (const FName Id : SyntheticIds)
		{
			if (Id.IsNone() || Registry->HasCanonicalItem(Id)) return false;
			auto* Item = NewObject<UGridNonArmamentDefinitionAsset>(Root);
			Item->ItemId = Id;
			Item->DisplayName = FText::FromName(Id);
			Item->WeightKg = 1.0f;
			Root->ItemDefinitions.Add(Item);
		}
		Registry->Deinitialize();
		TArray<FString> Errors;
		return Registry->RegisterItemSystem(Root, Errors);
	}

	void Detach()
	{
		if (World) World->SetGameInstance(nullptr);
		if (GI.IsValid()) GI->Shutdown();
		Registry = nullptr;
		World = nullptr;
		GI.Reset();
	}

	~FGridCanonicalLootTestScope() { Detach(); }
};
#endif
