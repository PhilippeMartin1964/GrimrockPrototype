#include "UI/RPGTalentDetailGlyphSet.h"

#include "Engine/Texture2D.h"

UTexture2D* URPGTalentDetailGlyphSet::ResolveGlyph(EGridTalentDetailLineSemantic Semantic) const
{
	switch (Semantic)
	{
		case EGridTalentDetailLineSemantic::ActionPointCost:
		case EGridTalentDetailLineSemantic::ManaCost:
		case EGridTalentDetailLineSemantic::ItemCost:
		case EGridTalentDetailLineSemantic::TalentPointCost:
			return Cost;

		case EGridTalentDetailLineSemantic::Target:
			return Target;

		case EGridTalentDetailLineSemantic::Range:
			return Range;

		case EGridTalentDetailLineSemantic::Cooldown:
			return Cooldown;

		case EGridTalentDetailLineSemantic::LevelRequirement:
			return LevelRequirement;

		case EGridTalentDetailLineSemantic::Prerequisite:
			return Prerequisite;

		default:
			return nullptr;
	}
}
