#pragma once

#include "Runtime/GridItemContextActionLibrary.h"

inline const TCHAR* GetGridItemActionDiagnosticName(EGridItemActionType ActionType)
{
	switch (ActionType)
	{
		case EGridItemActionType::Equip:
			return TEXT("Equip");
		case EGridItemActionType::Unequip:
			return TEXT("Unequip");
		case EGridItemActionType::Consume:
			return TEXT("Consume");
		case EGridItemActionType::Read:
			return TEXT("Read");
		case EGridItemActionType::Examine:
			return TEXT("Examine");
		case EGridItemActionType::Use:
			return TEXT("Use");
		case EGridItemActionType::UseOnTarget:
			return TEXT("UseOnTarget");
		case EGridItemActionType::InsertIntoTarget:
			return TEXT("InsertIntoTarget");
		case EGridItemActionType::PlaceOnTarget:
			return TEXT("PlaceOnTarget");
		case EGridItemActionType::DropToGround:
			return TEXT("DropToGround");
		case EGridItemActionType::Throw:
			return TEXT("Throw");
		case EGridItemActionType::Combine:
			return TEXT("Combine");
		case EGridItemActionType::SplitStack:
			return TEXT("SplitStack");
		case EGridItemActionType::ToggleLight:
			return TEXT("ToggleLight");
		case EGridItemActionType::AddToHotbar:
			return TEXT("AddToHotbar");
		case EGridItemActionType::DisarmTrap:
			return TEXT("DisarmTrap");
		case EGridItemActionType::None:
		default:
			return TEXT("None");
	}
}
