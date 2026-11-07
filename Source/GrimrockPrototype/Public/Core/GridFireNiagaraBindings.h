#pragma once

#include "CoreMinimal.h"
#include "Core/GridVfxProfileTypes.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraTypes.h"
#include "NiagaraUserRedirectionParameterStore.h"

/** Authoring-level adapter: only binds real, exposed, type-correct Niagara
 * User parameters on each component; never mutates the shared system asset.
 * Supports vendors exposing shortened names without the "User." namespace.
 */
struct FGridFireNiagaraBindings
{
	bool bSpawnBox = false;
	FName SpawnBoxName;
	FVector NativeSpawnBox = FVector::ZeroVector;
	bool bFlameRate = false;
	FName FlameRateName;
	float NativeFlameRate = 0.f;
	bool bSmokeRate = false;
	FName SmokeRateName;
	float NativeSmokeRate = 0.f;
	bool bTintColor = false;
	bool bTintVec3 = false;
	bool bTintVec4 = false;
	FName TintName;

	static FString ShortName(FName Value)
	{
		FString Name = Value.ToString();
		Name.RemoveFromStart(TEXT("User."), ESearchCase::IgnoreCase);
		return Name;
	}

	static FName QualifiedName(FName Value)
	{
		return FName(*(FString(TEXT("User.")) + ShortName(Value)));
	}

	static float ScaledRate(float NativeRate, float Multiplier)
	{
		return FMath::Clamp(FMath::Max(0.f, NativeRate) *
			FMath::Max(0.f, Multiplier), 0.f, 1000000.f);
	}

	static FGridFireNiagaraBindings Resolve(const UNiagaraSystem* System,
		const FGridFireVfxSettings& Settings)
	{
		FGridFireNiagaraBindings Plan;
		if (!IsValid(System) || !Settings.bAutoBindKnownFireControls) return Plan;
		const FNiagaraUserRedirectionParameterStore& Store = System->GetExposedParameters();
		TArray<FNiagaraVariable> UserVariables;
		Store.GetUserParameters(UserVariables);
		const auto Find = [&UserVariables](const TCHAR* Name,
			const FNiagaraTypeDefinition& Type) -> const FNiagaraVariable*
		{
			for (const FNiagaraVariable& Var : UserVariables)
			{
				if (ShortName(Var.GetName()).Equals(Name, ESearchCase::IgnoreCase)
					&& Var.GetType() == Type) return &Var;
			}
			return nullptr;
		};
		// Explicit bindings always win. An authored 'SpawnBox' also uses this
		// path when supplied through SpawnAreaParameter.
		if (Settings.SpawnAreaParameter.IsNone() ||
			ShortName(Settings.SpawnAreaParameter).Equals(TEXT("SpawnBox"), ESearchCase::IgnoreCase))
		{
			if (const FNiagaraVariable* Var = Find(TEXT("SpawnBox"),
				FNiagaraTypeDefinition::GetVec3Def()))
			{
				if (const uint8* Bytes = Store.GetParameterData(*Var))
				{
					FVector3f Native;
					FMemory::Memcpy(&Native, Bytes, sizeof(Native));
					Plan.NativeSpawnBox = FVector(Native);
					Plan.SpawnBoxName = QualifiedName(Var->GetName());
					Plan.bSpawnBox = true;
				}
			}
		}
		if (Settings.FlameDensityParameter.IsNone() ||
			ShortName(Settings.FlameDensityParameter).Equals(TEXT("FlameRate"), ESearchCase::IgnoreCase))
		{
			if (const FNiagaraVariable* Var = Find(TEXT("FlameRate"),
				FNiagaraTypeDefinition::GetFloatDef()))
			{
				if (const uint8* Bytes = Store.GetParameterData(*Var))
				{
					FMemory::Memcpy(&Plan.NativeFlameRate, Bytes, sizeof(float));
					Plan.FlameRateName = QualifiedName(Var->GetName());
					Plan.bFlameRate = true;
				}
			}
		}
		if (Settings.SmokeDensityParameter.IsNone() ||
			ShortName(Settings.SmokeDensityParameter).Equals(TEXT("SmokeRate"), ESearchCase::IgnoreCase))
		{
			if (const FNiagaraVariable* Var = Find(TEXT("SmokeRate"),
				FNiagaraTypeDefinition::GetFloatDef()))
			{
				if (const uint8* Bytes = Store.GetParameterData(*Var))
				{
					FMemory::Memcpy(&Plan.NativeSmokeRate, Bytes, sizeof(float));
					Plan.SmokeRateName = QualifiedName(Var->GetName());
					Plan.bSmokeRate = true;
				}
			}
		}
		if (Settings.FireTintParameter.IsNone() ||
			ShortName(Settings.FireTintParameter).Equals(TEXT("Tint"), ESearchCase::IgnoreCase))
		{
			if (const FNiagaraVariable* ColorVar = Find(TEXT("Tint"),
				FNiagaraTypeDefinition::GetColorDef()))
			{
				Plan.bTintColor = true;
				Plan.TintName = QualifiedName(ColorVar->GetName());
			}
			else if (const FNiagaraVariable* Vector3Var = Find(TEXT("Tint"),
				FNiagaraTypeDefinition::GetVec3Def()))
			{
				Plan.bTintVec3 = true;
				Plan.TintName = QualifiedName(Vector3Var->GetName());
			}
			else if (const FNiagaraVariable* Vector4Var = Find(TEXT("Tint"),
				FNiagaraTypeDefinition::GetVec4Def()))
			{
				Plan.bTintVec4 = true;
				Plan.TintName = QualifiedName(Vector4Var->GetName());
			}
		}
		return Plan;
	}

	void Apply(UNiagaraComponent* Component, const FGridFireVfxSettings& Settings,
		float CellSizeCm, const FVector& ComponentScale) const
	{
		if (!IsValid(Component)) return;
		if (bSpawnBox)
		{
			const float Width = CellSizeCm * Settings.GroundFireCoverage /
				FMath::Max(.01f, ComponentScale.X);
			const float Height = CellSizeCm * Settings.GroundFireCoverage /
				FMath::Max(.01f, ComponentScale.Y);
			// Only X/Y are game-authored. Preserve the native Z spawn thickness
			// so particles originate near the floor rather than all along a flame.
			Component->SetVariableVec3(SpawnBoxName, FVector(Width, Height, NativeSpawnBox.Z));
		}
		if (bFlameRate && FMath::IsFinite(NativeFlameRate))
			Component->SetVariableFloat(FlameRateName,
				ScaledRate(NativeFlameRate, Settings.FlameDensity));
		if (bSmokeRate && FMath::IsFinite(NativeSmokeRate))
			Component->SetVariableFloat(SmokeRateName,
				ScaledRate(NativeSmokeRate,
					Settings.bEnableSmoke ? Settings.SmokeDensity : 0.f));
		if (bTintColor)
			Component->SetVariableLinearColor(TintName, Settings.FireTint);
		if (bTintVec3)
			Component->SetVariableVec3(TintName,
				FVector(Settings.FireTint.R, Settings.FireTint.G, Settings.FireTint.B));
		if (bTintVec4)
			Component->SetVariableVec4(TintName,
				FVector4(Settings.FireTint.R, Settings.FireTint.G,
					Settings.FireTint.B, Settings.FireTint.A));
	}
};
