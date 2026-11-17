#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Dom/JsonValue.h"
#include "../ARMEMENT/GridArmClean064X371BackupReceiptGuard.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X371BackupReceipt,
    "Grimrock.RPG.ARM.CLEAN06_4X371.NativeBackupReceiptClosedAllowlist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X371BackupReceipt::RunTest(const FString&)
{
    const FString Head(TEXT("5071d6eb4cf65b76e8f8bcd3f9bae1fc37d54687"));
    const FString Hash = FString::ChrN(64, TEXT('a'));
    const TSet<FString> Required = ARMCLEAN064X371::RequiredPaths();
    bool bOk = TestEqual(TEXT("Closed backup path inventory"), Required.Num(), 24);
    TArray<FString> Paths;
    for (const FString& Path : Required) { Paths.Add(Path); }
    Paths.Sort();

    TArray<TSharedPtr<FJsonValue>> OriginalRows;
    for (const FString& Path : Paths)
    {
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("path"), Path);
        Row->SetStringField(TEXT("sha256"), Hash);
        OriginalRows.Add(MakeShared<FJsonValueObject>(Row));
    }

    auto MakeReceipt = [&OriginalRows, &Head]()
    {
        TSharedPtr<FJsonObject> Receipt = MakeShared<FJsonObject>();
        Receipt->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X.37/native-backup/v1"));
        Receipt->SetStringField(TEXT("head"), Head);
        Receipt->SetBoolField(TEXT("verified"), true);
        Receipt->SetNumberField(TEXT("source_count"), 22);
        Receipt->SetNumberField(TEXT("output_count"), 2);
        Receipt->SetArrayField(TEXT("files"), OriginalRows);
        return Receipt;
    };

    TArray<FString> Validated;
    const TSharedPtr<FJsonObject> Good = MakeReceipt();
    bOk &= TestTrue(TEXT("Exact 24-file receipt is accepted"),
        ARMCLEAN064X371::ValidateReceipt(Good, Head, Validated));
    bOk &= TestEqual(TEXT("All 24 paths returned for byte comparison"), Validated.Num(), 24);
    bOk &= TestFalse(TEXT("Wrong SHA40 cannot authorize Apply"),
        ARMCLEAN064X371::ValidateReceipt(Good, FString::ChrN(40, TEXT('b')), Validated));
    bOk &= TestTrue(TEXT("Failure never exposes a partial allowlist"), Validated.IsEmpty());

    TSharedPtr<FJsonObject> Duplicate = MakeReceipt();
    TArray<TSharedPtr<FJsonValue>> DuplicateRows = OriginalRows;
    DuplicateRows[23] = DuplicateRows[0];
    Duplicate->SetArrayField(TEXT("files"), DuplicateRows);
    bOk &= TestFalse(TEXT("Duplicate source path fails closed"),
        ARMCLEAN064X371::ValidateReceipt(Duplicate, Head, Validated));

    TSharedPtr<FJsonObject> Missing = MakeReceipt();
    TArray<TSharedPtr<FJsonValue>> MissingRows = OriginalRows;
    MissingRows.RemoveAt(0);
    Missing->SetArrayField(TEXT("files"), MissingRows);
    bOk &= TestFalse(TEXT("Missing backup row fails closed"),
        ARMCLEAN064X371::ValidateReceipt(Missing, Head, Validated));

    TSharedPtr<FJsonObject> Traversal = MakeReceipt();
    TArray<TSharedPtr<FJsonValue>> TraversalRows = OriginalRows;
    TSharedRef<FJsonObject> BadPath = MakeShared<FJsonObject>();
    BadPath->SetStringField(TEXT("path"), TEXT("../OutsideProject/DA_CanonicalWeaponCatalogue.uasset"));
    BadPath->SetStringField(TEXT("sha256"), Hash);
    TraversalRows[0] = MakeShared<FJsonValueObject>(BadPath);
    Traversal->SetArrayField(TEXT("files"), TraversalRows);
    bOk &= TestFalse(TEXT("Path traversal fails closed"),
        ARMCLEAN064X371::ValidateReceipt(Traversal, Head, Validated));

    TSharedPtr<FJsonObject> BadHash = MakeReceipt();
    TArray<TSharedPtr<FJsonValue>> BadHashRows = OriginalRows;
    TSharedRef<FJsonObject> InvalidHash = MakeShared<FJsonObject>();
    InvalidHash->SetStringField(TEXT("path"), Paths[0]);
    InvalidHash->SetStringField(TEXT("sha256"), FString::ChrN(64, TEXT('x')));
    BadHashRows[0] = MakeShared<FJsonValueObject>(InvalidHash);
    BadHash->SetArrayField(TEXT("files"), BadHashRows);
    bOk &= TestFalse(TEXT("Invalid SHA256 token fails closed"),
        ARMCLEAN064X371::ValidateReceipt(BadHash, Head, Validated));

    TSharedPtr<FJsonObject> Unverified = MakeReceipt();
    Unverified->SetBoolField(TEXT("verified"), false);
    bOk &= TestFalse(TEXT("Unsigned/unverified receipt fails closed"),
        ARMCLEAN064X371::ValidateReceipt(Unverified, Head, Validated));

    AddInfo(TEXT("[ARM-CLEAN06.4X.37.1] Strict receipt tests only; no real backup files opened and no native asset written."));
    return bOk && !HasAnyErrors();
}
#endif
