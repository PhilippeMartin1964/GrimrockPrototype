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
		TMap<FName, float> SyntheticWeights;
		for (const FName Id : SyntheticIds)
		{
			if (SyntheticWeights.Contains(Id)) return false;
			SyntheticWeights.Add(Id, 1.0f);
		}
		return AttachWithWeights(InWorld, SyntheticWeights);
	}

	// Synthetic definitions belong to one transient registry. No global
	// metadata, instance copies or authored package edits are needed.
	bool AttachWithWeights(UWorld* InWorld, const TMap<FName, float>& SyntheticWeights,
		const TFunction<void(UGridItemSystemAsset&)>& Extend = {})
	{
		if (World || !IsValid(InWorld) || InWorld->GetGameInstance()) return false;
		World = InWorld;
		GI.Reset(NewObject<UGameInstance>(GetTransientPackage()));
		if (!GI.IsValid()) return false;
		GI->Init();
		World->SetGameInstance(GI.Get());
		Registry = GI->GetSubsystem<UGridItemSubsystem>();
		if (!Registry || !Registry->IsConfigured()) return false;
		if (SyntheticWeights.IsEmpty() && !Extend) return true;
		UGridItemSystemAsset* Production = Registry->LoadConfiguredItemSystemForPreview();
		if (!IsValid(Production)) return false;
		UGridItemSystemAsset* Root = DuplicateObject<UGridItemSystemAsset>(Production, GetTransientPackage());
		if (!IsValid(Root)) return false;
		for (const auto& Pair : SyntheticWeights)
		{
			const FName Id = Pair.Key;
			if (Id.IsNone() || Registry->ContainsItem(Id) ||
				!FMath::IsFinite(Pair.Value) || Pair.Value < 0.0f) return false;
			auto* Item = NewObject<UGridNonArmamentDefinitionAsset>(Root);
			Item->ItemId = Id;
			Item->DisplayName = FText::FromName(Id);
			Item->WeightKg = Pair.Value;
			Root->ItemDefinitions.Add(Item);
		}
		if (Extend) Extend(*Root);
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
