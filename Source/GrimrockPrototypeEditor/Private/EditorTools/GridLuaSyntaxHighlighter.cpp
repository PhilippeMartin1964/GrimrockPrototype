#include "EditorTools/GridLuaSyntaxHighlighter.h"

#include "Framework/Text/IRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/CoreStyle.h"

namespace
{
	FTextBlockStyle MakeLuaTextStyle(const FLinearColor& Color)
	{
		return FTextBlockStyle()
			.SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 10))
			.SetColorAndOpacity(FSlateColor(Color));
	}

	bool IsIdentifierChar(TCHAR Ch)
	{
		return FChar::IsAlnum(Ch) || Ch == TEXT('_');
	}

	bool IsStandaloneAt(const FString& SourceString, const FTextRange& Range)
	{
		const bool bHasIdentifierBefore = Range.BeginIndex > 0 && IsIdentifierChar(SourceString[Range.BeginIndex - 1]);
		const bool bHasIdentifierAfter = Range.EndIndex < SourceString.Len() && IsIdentifierChar(SourceString[Range.EndIndex]);
		return !bHasIdentifierBefore && !bHasIdentifierAfter;
	}

	bool IsNumberTokenAt(const FString& SourceString, const FTextRange& Range)
	{
		// Digits are tokenizer rules so a whole numeric literal may arrive as a
		// series of one-character syntax tokens. Do not colour digits embedded in
		// ordinary Lua identifiers such as Door2 or item42Value.
		const bool bLetterOrUnderscoreBefore = Range.BeginIndex > 0 &&
			(FChar::IsAlpha(SourceString[Range.BeginIndex - 1]) || SourceString[Range.BeginIndex - 1] == TEXT('_'));
		const bool bLetterOrUnderscoreAfter = Range.EndIndex < SourceString.Len() &&
			(FChar::IsAlpha(SourceString[Range.EndIndex]) || SourceString[Range.EndIndex] == TEXT('_'));
		return !bLetterOrUnderscoreBefore && !bLetterOrUnderscoreAfter;
	}
}

FGridLuaSyntaxHighlighter::FGridLuaSyntaxHighlighter(TSharedPtr<ISyntaxTokenizer> InTokenizer)
	: FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer))
	, NormalTextStyle(MakeLuaTextStyle(FLinearColor(0.86f, 0.86f, 0.86f, 1.0f)))
	, KeywordTextStyle(MakeLuaTextStyle(FLinearColor(0.72f, 0.55f, 1.00f, 1.0f)))
	, LiteralTextStyle(MakeLuaTextStyle(FLinearColor(0.45f, 0.78f, 1.00f, 1.0f)))
	, NumberTextStyle(MakeLuaTextStyle(FLinearColor(0.40f, 0.82f, 0.95f, 1.0f)))
	, StringTextStyle(MakeLuaTextStyle(FLinearColor(0.62f, 0.84f, 0.52f, 1.0f)))
	, CommentTextStyle(MakeLuaTextStyle(FLinearColor(0.48f, 0.62f, 0.50f, 1.0f)))
	, PersistentTextStyle(MakeLuaTextStyle(FLinearColor(1.00f, 0.72f, 0.28f, 1.0f)))
	, GridApiTextStyle(MakeLuaTextStyle(FLinearColor(0.34f, 0.80f, 1.00f, 1.0f)))
{
}

TSharedRef<FGridLuaSyntaxHighlighter> FGridLuaSyntaxHighlighter::Create()
{
	TArray<FSyntaxTokenizer::FRule> Rules;
	const auto AddRule = [&Rules](const TCHAR* Text)
	{
		Rules.Emplace(Text);
	};

	// Parse-state delimiters. Escaped quote rules must be greedier than quotes.
	AddRule(TEXT("--[["));
	AddRule(TEXT("--"));
	AddRule(TEXT("[["));
	AddRule(TEXT("]]"));
	AddRule(TEXT("\\'"));
	AddRule(TEXT("\\\""));
	AddRule(TEXT("'"));
	AddRule(TEXT("\""));

	// Grimrock API is tokenized as complete names so `grid.command` reads as one
	// visual concept rather than three unrelated tokens.
	AddRule(TEXT("grid.visual.set_material"));
	AddRule(TEXT("grid.vars.get_bool"));
	AddRule(TEXT("grid.vars.set_bool"));
	AddRule(TEXT("grid.vars.get_int"));
	AddRule(TEXT("grid.vars.set_int"));
	AddRule(TEXT("grid.command"));
	AddRule(TEXT("grid.log"));
	AddRule(TEXT("persistent"));

	for (const TCHAR* Keyword : {
		TEXT("and"), TEXT("break"), TEXT("do"), TEXT("else"), TEXT("elseif"), TEXT("end"), TEXT("for"), TEXT("function"),
		TEXT("goto"), TEXT("if"), TEXT("in"), TEXT("local"), TEXT("not"), TEXT("or"), TEXT("repeat"), TEXT("return"), TEXT("then"),
		TEXT("until"), TEXT("while"), TEXT("true"), TEXT("false"), TEXT("nil") })
	{
		AddRule(Keyword);
	}

	for (const TCHAR* Digit : { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9") })
	{
		AddRule(Digit);
	}

	Rules.Sort(
		[](const FSyntaxTokenizer::FRule& Left, const FSyntaxTokenizer::FRule& Right)
		{
			return Left.MatchText.Len() > Right.MatchText.Len();
		});

	return MakeShareable(new FGridLuaSyntaxHighlighter(FSyntaxTokenizer::Create(MoveTemp(Rules))));
}

EGridLuaSyntaxStyle FGridLuaSyntaxHighlighter::ClassifyStandaloneToken(const FString& Token)
{
	if (Token == TEXT("persistent"))
	{
		return EGridLuaSyntaxStyle::Persistent;
	}
	if (Token.StartsWith(TEXT("grid."), ESearchCase::CaseSensitive))
	{
		return EGridLuaSyntaxStyle::GridApi;
	}
	if (Token == TEXT("true") || Token == TEXT("false") || Token == TEXT("nil"))
	{
		return EGridLuaSyntaxStyle::Literal;
	}
	if (Token.Len() == 1 && FChar::IsDigit(Token[0]))
	{
		return EGridLuaSyntaxStyle::Number;
	}

	static const TSet<FString> Keywords = {
		TEXT("and"), TEXT("break"), TEXT("do"), TEXT("else"), TEXT("elseif"), TEXT("end"), TEXT("for"), TEXT("function"),
		TEXT("goto"), TEXT("if"), TEXT("in"), TEXT("local"), TEXT("not"), TEXT("or"), TEXT("repeat"), TEXT("return"), TEXT("then"),
		TEXT("until"), TEXT("while")
	};
	return Keywords.Contains(Token) ? EGridLuaSyntaxStyle::Keyword : EGridLuaSyntaxStyle::Normal;
}

void FGridLuaSyntaxHighlighter::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout,
	TArray<ISyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	enum class EParseState : uint8
	{
		None,
		SingleLineComment,
		MultiLineComment,
		SingleQuotedString,
		DoubleQuotedString,
		LongString
	};

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());
	EParseState ParseState = EParseState::None;

	for (const ISyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		if (ParseState == EParseState::SingleLineComment)
		{
			ParseState = EParseState::None;
		}

		TSharedRef<FString> ModelString = MakeShared<FString>();
		TArray<TSharedRef<IRun>> Runs;

		for (const ISyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{
			const FString TokenString = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());
			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenString.Len());
			ModelString->Append(TokenString);

			FRunInfo RunInfo(TEXT("SyntaxHighlight.GrimrockLua.Normal"));
			const FTextBlockStyle* CurrentStyle = &NormalTextStyle;
			const bool bWhitespace = TokenString.TrimStartAndEnd().IsEmpty();

			if (!bWhitespace)
			{
				if (ParseState == EParseState::SingleLineComment || ParseState == EParseState::MultiLineComment)
				{
					RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Comment");
					CurrentStyle = &CommentTextStyle;
					if (ParseState == EParseState::MultiLineComment && TokenString == TEXT("]]"))
					{
						ParseState = EParseState::None;
					}
				}
				else if (ParseState == EParseState::SingleQuotedString || ParseState == EParseState::DoubleQuotedString || ParseState == EParseState::LongString)
				{
					RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.String");
					CurrentStyle = &StringTextStyle;

					const bool bSingleClose = ParseState == EParseState::SingleQuotedString && TokenString == TEXT("'");
					const bool bDoubleClose = ParseState == EParseState::DoubleQuotedString && TokenString == TEXT("\"");
					const bool bLongClose = ParseState == EParseState::LongString && TokenString == TEXT("]]");
					if (bSingleClose || bDoubleClose || bLongClose)
					{
						ParseState = EParseState::None;
					}
				}
				else if (Token.Type == ISyntaxTokenizer::ETokenType::Syntax)
				{
					if (TokenString == TEXT("--[["))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Comment");
						CurrentStyle = &CommentTextStyle;
						ParseState = EParseState::MultiLineComment;
					}
					else if (TokenString == TEXT("--"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Comment");
						CurrentStyle = &CommentTextStyle;
						ParseState = EParseState::SingleLineComment;
					}
					else if (TokenString == TEXT("[["))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.String");
						CurrentStyle = &StringTextStyle;
						ParseState = EParseState::LongString;
					}
					else if (TokenString == TEXT("'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.String");
						CurrentStyle = &StringTextStyle;
						ParseState = EParseState::SingleQuotedString;
					}
					else if (TokenString == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.String");
						CurrentStyle = &StringTextStyle;
						ParseState = EParseState::DoubleQuotedString;
					}
					else
					{
						const EGridLuaSyntaxStyle Category = ClassifyStandaloneToken(TokenString);
						const bool bCanApply = Category == EGridLuaSyntaxStyle::Number
							? IsNumberTokenAt(SourceString, Token.Range)
							: IsStandaloneAt(SourceString, Token.Range);
						if (bCanApply)
						{
							switch (Category)
							{
								case EGridLuaSyntaxStyle::Keyword:
									RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Keyword");
									CurrentStyle = &KeywordTextStyle;
									break;
								case EGridLuaSyntaxStyle::Literal:
									RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Literal");
									CurrentStyle = &LiteralTextStyle;
									break;
								case EGridLuaSyntaxStyle::Number:
									RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Number");
									CurrentStyle = &NumberTextStyle;
									break;
								case EGridLuaSyntaxStyle::Persistent:
									RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.Persistent");
									CurrentStyle = &PersistentTextStyle;
									break;
								case EGridLuaSyntaxStyle::GridApi:
									RunInfo.Name = TEXT("SyntaxHighlight.GrimrockLua.GridApi");
									CurrentStyle = &GridApiTextStyle;
									break;
								case EGridLuaSyntaxStyle::Normal:
								default:
									break;
							}
						}
					}
				}
			}

			Runs.Add(FSlateTextRun::Create(RunInfo, ModelString, *CurrentStyle, ModelRange));
		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}
