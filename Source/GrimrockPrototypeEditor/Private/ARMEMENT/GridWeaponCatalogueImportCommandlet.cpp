#include "ARMEMENT/GridWeaponCatalogueImportCommandlet.h"

// ARM-CLEAN06.5: the pre-cutover XLSX import commandlet must never recreate
// or overwrite the retired source DA_WeaponCatalogue. The commandlet class
// remains registered temporarily so old automation receives a safe, explicit
// failure instead of silently writing a second gameplay authority.
UGridWeaponCatalogueImportCommandlet::UGridWeaponCatalogueImportCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridWeaponCatalogueImportCommandlet::Main(const FString& Params)
{
    UE_LOG(LogTemp, Error,
        TEXT("[ARM-CLEAN06.5] Legacy weapon catalogue import is retired. ")
        TEXT("DA_ItemSystem and CanonicalStage/DA_CanonicalWeaponCatalogue ")
        TEXT("are the only active production authorities. No package was modified."));
    return 1;
}
