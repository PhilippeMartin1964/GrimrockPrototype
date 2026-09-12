#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/SyntaxTokenizer.h"
#include "Styling/SlateTypes.h"

/** LUA-UX04 syntax categories that can be identified without parse state. */
enum class EGridLuaSyntaxStyle : uint8
{
	Normal,
	Keyword,
	Literal,
	Number,
	Persistent,
	GridApi
};

/**
 * LUA-UX04 — presentation-only syntax highlighting for the Grimrock Lua editor.
 *
 * This marshaller never changes the Lua source and has no role in compilation.
 * FGridLuaAuthoringCompiler remains the sole authoring validation authority.
 */
class GRIMROCKPROTOTYPEEDITOR_API FGridLuaSyntaxHighlighter final : public FSyntaxHighlighterTextLayoutMarshaller
{
public:
	static TSharedRef<FGridLuaSyntaxHighlighter> Create();

	/** Deterministic token classification shared by rendering and automation tests. */
	static EGridLuaSyntaxStyle ClassifyStandaloneToken(const FString& Token);

protected:
	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout,
		TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

private:
	explicit FGridLuaSyntaxHighlighter(TSharedPtr<ISyntaxTokenizer> InTokenizer);

	FTextBlockStyle NormalTextStyle;
	FTextBlockStyle KeywordTextStyle;
	FTextBlockStyle LiteralTextStyle;
	FTextBlockStyle NumberTextStyle;
	FTextBlockStyle StringTextStyle;
	FTextBlockStyle CommentTextStyle;
	FTextBlockStyle PersistentTextStyle;
	FTextBlockStyle GridApiTextStyle;
};
