// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GrimrockPrototypeTarget : TargetRules
{
	public GrimrockPrototypeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("GrimrockPrototype");
	}
}
