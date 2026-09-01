#include "EditorTools/GridLevelEditorActor.h"

#include "EditorTools/GridEditorLinkPolicy.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Components/TextRenderComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#endif

namespace
{
	struct FExpectedConcreteArchetypeSpec
	{
		const TCHAR* ArchetypeId;
		EGridLevelObjectType ExpectedType;
	};

	// Visual variants are concrete archetypes/palette entries, not EGridLevelObjectType values.
	static const FExpectedConcreteArchetypeSpec ExpectedConcreteArchetypes[] = { { TEXT("Button_Normal"), EGridLevelObjectType::Button },
		{ TEXT("Button_Secret"), EGridLevelObjectType::Button }, { TEXT("Button_Wall"), EGridLevelObjectType::Button },
		{ TEXT("Door_Stone"), EGridLevelObjectType::Door }, { TEXT("Door_Secret"), EGridLevelObjectType::Door },
		{ TEXT("Receptacle_Alcove"), EGridLevelObjectType::Receptacle }, { TEXT("Receptacle_Alcove_Stone"), EGridLevelObjectType::Receptacle },
		{ TEXT("Decoration_Wall_Stone_Cracked"), EGridLevelObjectType::Decoration },
		{ TEXT("Decoration_Wall_Stone_SewerDrain"), EGridLevelObjectType::Decoration }, { TEXT("Receptacle_TorchHolder"), EGridLevelObjectType::Receptacle },
		{ TEXT("Receptacle_Altar"), EGridLevelObjectType::Receptacle }, { TEXT("Receptacle_OfferingBowl"), EGridLevelObjectType::Receptacle } };

	EGridWallType GetWallTypeForEdge(const FGridLevelCellData& CellData, EGridEdge Edge)
	{
		switch (Edge)
		{
			case EGridEdge::North:
				return CellData.NorthWall;
			case EGridEdge::East:
				return CellData.EastWall;
			case EGridEdge::South:
				return CellData.SouthWall;
			case EGridEdge::West:
				return CellData.WestWall;
			default:
				return EGridWallType::None;
		}
	}

	float GetYawForOrientation(EGridEdge Orientation)
	{
		switch (Orientation)
		{
			case EGridEdge::North:
				return 0.f;
			case EGridEdge::East:
				return 90.f;
			case EGridEdge::South:
				return 180.f;
			case EGridEdge::West:
				return 270.f;
			default:
				return 0.f;
		}
	}

	EGridLevelValidationSeverity ConvertArchetypeValidationSeverity(EGridArchetypeValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridArchetypeValidationSeverity::Error:
				return EGridLevelValidationSeverity::Error;

			case EGridArchetypeValidationSeverity::Warning:
				return EGridLevelValidationSeverity::Warning;

			case EGridArchetypeValidationSeverity::Info:
			default:
				return EGridLevelValidationSeverity::Info;
		}
	}

	FName InferValidationCategory(const FString& Message)
	{
		if (Message.StartsWith(TEXT("Link ")) || Message.Contains(TEXT(" link")))
		{
			return TEXT("Links");
		}
		if (Message.Contains(TEXT("ObjectPalette")) || Message.Contains(TEXT("PaletteEntry")))
		{
			return TEXT("Palette");
		}
		if (Message.Contains(TEXT("Readable")) || Message.Contains(TEXT("readable")))
		{
			return TEXT("Readable");
		}
		if (Message.Contains(TEXT("Receptacle")) || Message.Contains(TEXT("receptacle")))
		{
			return TEXT("Receptacles");
		}
		if (Message.Contains(TEXT("Door")) || Message.Contains(TEXT("door")))
		{
			return TEXT("Doors");
		}
		if (Message.Contains(TEXT("Item")) || Message.Contains(TEXT("item")))
		{
			return TEXT("Items");
		}
		if (Message.Contains(TEXT("Monster")) || Message.Contains(TEXT("monster")))
		{
			return TEXT("Monsters");
		}
		if (Message.Contains(TEXT("Archetype")) || Message.Contains(TEXT("archetype")))
		{
			return TEXT("Archetypes");
		}
		if (Message.Contains(TEXT("wall")) || Message.Contains(TEXT("Wall")) || Message.Contains(TEXT("shared edge")))
		{
			return TEXT("Walls");
		}
		if (Message.Contains(TEXT("cell")) || Message.Contains(TEXT("Cell")) || Message.Contains(TEXT("Start")))
		{
			return TEXT("Grid");
		}
		if (Message.Contains(TEXT("runtime")) || Message.Contains(TEXT("Runtime")))
		{
			return TEXT("Runtime");
		}
		if (Message.Contains(TEXT("Object")) || Message.Contains(TEXT("object")) || Message.Contains(TEXT("Trigger")))
		{
			return TEXT("Objects");
		}
		return TEXT("Core");
	}

	FString ToGridObjectTypeText(EGridLevelObjectType ObjectType)
	{
		if (const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>())
		{
			return TypeEnum->GetNameStringByValue(static_cast<int64>(ObjectType));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(ObjectType));
	}

	FString ToGridObjectEventText(EGridObjectEvent Event)
	{
		if (const UEnum* EventEnum = StaticEnum<EGridObjectEvent>())
		{
			return EventEnum->GetNameStringByValue(static_cast<int64>(Event));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Event));
	}

	FString ToGridObjectCommandText(EGridObjectCommand Command)
	{
		if (const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>())
		{
			return CommandEnum->GetNameStringByValue(static_cast<int64>(Command));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Command));
	}

	FString ToGridObjectConditionText(EGridObjectCondition Condition)
	{
		if (const UEnum* ConditionEnum = StaticEnum<EGridObjectCondition>())
		{
			return ConditionEnum->GetNameStringByValue(static_cast<int64>(Condition));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Condition));
	}

	bool IsReceptacleCommand(EGridObjectCommand Command)
	{
		switch (Command)
		{
			case EGridObjectCommand::ReceptacleConsumeItem:
			case EGridObjectCommand::ReceptacleConsumeAllItems:
			case EGridObjectCommand::ReceptacleEnableRemoval:
			case EGridObjectCommand::ReceptacleDisableRemoval:
				return true;

			default:
				return false;
		}
	}

	bool IsEventEmittedByCurrentRuntime(EGridLevelObjectType SourceType, EGridObjectEvent Event)
	{
		switch (SourceType)
		{
			case EGridLevelObjectType::Button:
				return Event == EGridObjectEvent::Activated;

			case EGridLevelObjectType::Lever:
			case EGridLevelObjectType::PressurePlate:
			case EGridLevelObjectType::Trigger:
				return Event == EGridObjectEvent::Activated || Event == EGridObjectEvent::Deactivated;

			case EGridLevelObjectType::Receptacle:
				return Event == EGridObjectEvent::ItemInserted || Event == EGridObjectEvent::ItemRemoved || Event == EGridObjectEvent::ItemChanged;

			case EGridLevelObjectType::Pit:
				return Event == EGridObjectEvent::Opened || Event == EGridObjectEvent::Closed;

			case EGridLevelObjectType::MonsterSpawn:
			{
				FGridLevelObjectData MonsterSpawn;
