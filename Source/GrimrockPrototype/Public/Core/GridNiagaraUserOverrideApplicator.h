#pragma once

#include "CoreMinimal.h"
#include "Core/GridVfxProfileTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "NiagaraUserRedirectionParameterStore.h"

/** Generic, data-driven overrides applicable to any Niagara system,
 * deliberately independent of FireFX, the level grid and gameplay.
 */
struct FGridResolvedNiagaraOverride
{
	FName CanonicalName;
	FGridNiagaraUserOverride Definition;
};

struct FGridNiagaraUserOverrideApplicator
{
	static FString ShortName(FName Input)
	{
		FString Name = Input.ToString();
		Name.RemoveFromStart(TEXT("User."), ESearchCase::IgnoreCase);
		return Name;
	}

	static bool MatchesType(EGridNiagaraOverrideValueType ValueType,
		const FNiagaraTypeDefinition& NiagaraType)
	{
		switch (ValueType)
		{
		case EGridNiagaraOverrideValueType::Float: return NiagaraType == FNiagaraTypeDefinition::GetFloatDef();
		case EGridNiagaraOverrideValueType::Integer: return NiagaraType == FNiagaraTypeDefinition::GetIntDef();
		case EGridNiagaraOverrideValueType::Boolean: return NiagaraType == FNiagaraTypeDefinition::GetBoolDef();
		case EGridNiagaraOverrideValueType::Vector3: return NiagaraType == FNiagaraTypeDefinition::GetVec3Def();
		case EGridNiagaraOverrideValueType::Color: return NiagaraType == FNiagaraTypeDefinition::GetColorDef();
		default: return false;
		}
	}

	static void Resolve(const UNiagaraSystem* System,
		const TArray<FGridNiagaraUserOverride>& Authored,
		TArray<FGridResolvedNiagaraOverride>& Out)
	{
		Out.Reset();
		if (!IsValid(System) || Authored.IsEmpty()) return;
		TArray<FNiagaraVariable> Available;
		System->GetExposedParameters().GetUserParameters(Available);
		for (const FGridNiagaraUserOverride& Definition : Authored)
		{
			if (Definition.ParameterName.IsNone()) continue;
			for (const FNiagaraVariable& Var : Available)
			{
				if (ShortName(Var.GetName()).Equals(ShortName(Definition.ParameterName),
					ESearchCase::IgnoreCase) && MatchesType(Definition.ValueType, Var.GetType()))
				{
					FGridResolvedNiagaraOverride& Entry = Out.AddDefaulted_GetRef();
					Entry.CanonicalName = FName(*(FString(TEXT("User.")) + ShortName(Var.GetName())));
					Entry.Definition = Definition;
					break;
				}
			}
		}
	}

	static void Apply(UNiagaraComponent* Fx,
		const TArray<FGridResolvedNiagaraOverride>& Entries)
	{
		if (!IsValid(Fx)) return;
		for (const FGridResolvedNiagaraOverride& Entry : Entries)
		{
			const FGridNiagaraUserOverride& Value = Entry.Definition;
			switch (Value.ValueType)
			{
			case EGridNiagaraOverrideValueType::Float:
				if (FMath::IsFinite(Value.FloatValue))
					Fx->SetVariableFloat(Entry.CanonicalName, Value.FloatValue);
				break;
			case EGridNiagaraOverrideValueType::Integer:
				Fx->SetVariableInt(Entry.CanonicalName, Value.IntValue);
				break;
			case EGridNiagaraOverrideValueType::Boolean:
				Fx->SetVariableBool(Entry.CanonicalName, Value.bBoolValue);
				break;
			case EGridNiagaraOverrideValueType::Vector3:
				if (FMath::IsFinite(Value.VectorValue.X) &&
					FMath::IsFinite(Value.VectorValue.Y) && FMath::IsFinite(Value.VectorValue.Z))
					Fx->SetVariableVec3(Entry.CanonicalName, Value.VectorValue);
				break;
			case EGridNiagaraOverrideValueType::Color:
				Fx->SetVariableLinearColor(Entry.CanonicalName, Value.ColorValue);
				break;
			default: break;
			}
		}
	}
};
