#pragma once

#include "CoreMinimal.h"

/** FLASK01: deterministic ballistic presentation (centimeters, seconds). No gameplay authority. */
struct FGridFlaskBallistics
{
    FVector Start = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    float GravityCmPerSec2 = 980.f;
    float DurationSeconds = 0.f;

    FVector PositionAt(float Seconds) const
    {
        const float T = FMath::Clamp(Seconds, 0.f, DurationSeconds);
        return Start + Velocity * T - FVector(0.f, 0.f, .5f * GravityCmPerSec2 * T * T);
    }

    static bool Solve(const FVector& From, const FVector& To, float SpeedCmPerSec,
        float GravityCmPerSec2, FGridFlaskBallistics& Out)
    {
        Out = FGridFlaskBallistics();
        if (From.ContainsNaN() || To.ContainsNaN() ||
            !FMath::IsFinite(SpeedCmPerSec) || !FMath::IsFinite(GravityCmPerSec2) ||
            SpeedCmPerSec <= 0.f || GravityCmPerSec2 <= 0.f)
        {
            return false;
        }
        const FVector Delta = To - From;
        const FVector Horizontal(Delta.X, Delta.Y, 0.f);
        const float Range = Horizontal.Size();
        if (Range <= KINDA_SMALL_NUMBER) return false;

        const double V2 = static_cast<double>(SpeedCmPerSec) * SpeedCmPerSec;
        const double G = GravityCmPerSec2;
        const double R = Range;
        const double Z = Delta.Z;
        const double Discriminant = V2 * V2 - G * (G * R * R + 2.0 * Z * V2);
        if (Discriminant < 0.0) return false; // Not physically reachable at this speed.

        // Lower-angle solution: suitable for indoor ceilings.
        const double Tangent = (V2 - FMath::Sqrt(Discriminant)) / (G * R);
        const double Cos = 1.0 / FMath::Sqrt(1.0 + Tangent * Tangent);
        const double Time = R / (static_cast<double>(SpeedCmPerSec) * Cos);
        if (!FMath::IsFinite(Time) || Time <= 0.0 || Time > 10.0) return false;
        Out.Start = From;
        Out.Velocity = Horizontal.GetSafeNormal() * static_cast<float>(SpeedCmPerSec * Cos) +
            FVector(0.f, 0.f, static_cast<float>(SpeedCmPerSec * Cos * Tangent));
        Out.GravityCmPerSec2 = GravityCmPerSec2;
        Out.DurationSeconds = static_cast<float>(Time);
        return true;
    }
};
