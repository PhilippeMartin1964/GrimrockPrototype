#include "UI/RPGTalentDetailGlyphSet.h"

#include "Engine/Texture2D.h"

UTexture2D* URPGTalentDetailGlyphSet::ResolveGlyph(EGridTalentDetailLineSemantic Semantic) const
{
	switch (Semantic)
	{
		case EGridTalentDetailLineSemantic::Action:
			return Action;

		case EGridTalentDetailLineSemantic::ActionPointCost:
		case EGridTalentDetailLineSemantic::TalentPointCost:
			return Cost;

		case EGridTalentDetailLineSemantic::ManaCost:
			return ManaCost ? ManaCost.Get() : Cost.Get();

		case EGridTalentDetailLineSemantic::ItemCost:
			return ItemCost ? ItemCost.Get() : Cost.Get();

		case EGridTalentDetailLineSemantic::Target:
			return Target;

		case EGridTalentDetailLineSemantic::Range:
			return Range;

		case EGridTalentDetailLineSemantic::Area:
			return Area;

		case EGridTalentDetailLineSemantic::LineOfSight:
			return LineOfSight;

		case EGridTalentDetailLineSemantic::ResolutionCount:
			return ResolutionCount;

		case EGridTalentDetailLineSemantic::Cooldown:
			return Cooldown;

		case EGridTalentDetailLineSemantic::LevelRequirement:
			return LevelRequirement;

		case EGridTalentDetailLineSemantic::Prerequisite:
			return Prerequisite;

		case EGridTalentDetailLineSemantic::Exclusivity:
			return Exclusivity;

		case EGridTalentDetailLineSemantic::Recipe:
			return Recipe;

		default:
			return nullptr;
	}
}
