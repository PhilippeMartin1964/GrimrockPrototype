#pragma once

#include "CoreMinimal.h"

/** FLASK-GLASS01: ephemeral cosmetic shards; no physics actors or gameplay authority.
 * All positions are local to the runtime actor, and gravity uses UE cm/s^2.
 */
struct FGridGlassShardState
{
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FRotator SpinDegreesPerSecond = FRotator::ZeroRotator;
	float GroundZ = 0.f;
	int32 Bounces = 0;
};

struct FGridGlassShardMotion
{
	static FGridGlassShardState MakeInitial(int32 Index, int32 Count,
		const FVector& ImpactCenter, float FloorZ, float LaunchSpeed)
	{
		FGridGlassShardState Result;
		const float Angle = 2.f * PI * (static_cast<float>(Index) +
			.13f * static_cast<float>(Index % 3)) / FMath::Max(1, Count);
		const float Speed = FMath::Max(1.f, LaunchSpeed) *
			(.7f + .12f * static_cast<float>(Index % 5));
		Result.GroundZ = FloorZ;
		Result.Position = ImpactCenter + FVector(0.f, 0.f, 1.f + float(Index % 3) * 1.5f);
		Result.Velocity = FVector(FMath::Cos(Angle) * Speed,
			FMath::Sin(Angle) * Speed,
			85.f + float(Index % 5) * 19.f);
		Result.Rotation = FRotator(float(Index * 31 % 180),
			float(Index * 47 % 360), float(Index * 67 % 180));
		Result.SpinDegreesPerSecond = FRotator(
			220.f + float(Index % 4) * 73.f,
			280.f + float(Index % 5) * 61.f,
			195.f + float(Index % 3) * 109.f);
		return Result;
	}

	/** Semi-implicit ballistic integration, with a small damped floor bounce.
	 * No airborne drag. Contact damping only, to keep shards in the impact cell.
	 */
	static void Advance(FGridGlassShardState& State, float DeltaSeconds,
		float GravityCmPerSecondSquared = 980.f)
	{
		const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.05f);
		if (Dt <= 0.f) return;
		State.Velocity.Z -= FMath::Max(0.f, GravityCmPerSecondSquared) * Dt;
		State.Position += State.Velocity * Dt;
		if (State.Position.Z < State.GroundZ)
		{
			State.Position.Z = State.GroundZ;
			if (State.Bounces < 2 && State.Velocity.Z < -30.f)
			{
				State.Velocity.Z = -State.Velocity.Z * .27f;
				State.Velocity.X *= .55f;
				State.Velocity.Y *= .55f;
				++State.Bounces;
			}
			else
			{
				State.Velocity.Z = 0.f;
				State.Velocity.X *= .85f;
				State.Velocity.Y *= .85f;
			}
		}
		State.Rotation += State.SpinDegreesPerSecond * Dt;
	}
};
