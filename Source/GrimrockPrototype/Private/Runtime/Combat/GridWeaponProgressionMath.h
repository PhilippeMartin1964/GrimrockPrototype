#pragma once

#include "CoreMinimal.h"

namespace GridWeaponProgressionMath
{
	inline int32 CalculateGrowthValue(const int32 Base, const int32 Level,
		const int32 Every, const int32 Amount)
	{
		const int64 Delta = static_cast<int64>((Level - 1) / Every) * Amount;
		return static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Base) + Delta,
			static_cast<int64>(MIN_int32), static_cast<int64>(MAX_int32)));
	}
}
