#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

namespace GridEditorWorkspaceTabs
{
	inline const FName& DungeonLevels()
	{
		static const FName Name(TEXT("GrimrockGridDungeonLevels"));
		return Name;
	}

	inline const FName& PlaytestValidation()
	{
		static const FName Name(TEXT("GrimrockGridPlaytestValidation"));
		return Name;
	}

	inline const FName& ToolsPalette()
	{
		static const FName Name(TEXT("GrimrockGridToolsPalette"));
		return Name;
	}

	inline const FName& SelectedObject()
	{
		static const FName Name(TEXT("GrimrockGridSelectedObject"));
		return Name;
	}

	inline const FName& LuaScripts()
	{
		static const FName Name(TEXT("GrimrockLuaEditor"));
		return Name;
	}
}

#endif
