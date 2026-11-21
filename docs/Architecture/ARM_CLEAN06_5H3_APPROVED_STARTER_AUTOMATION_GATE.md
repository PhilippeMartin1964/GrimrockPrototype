# ARM-CLEAN06.5H.3 — Garde des kits approuvés hors Automation

**Parent validé :** `d5b082a387e7bed8fbdaff173d181d1be32348fd` (UE5.5.4/SilentTask et UE5.5.4/LocalAuto SUCCESS).

## Défaut corrigé

`TryPopulateApprovedTransientClass`, utilisé par le wizard et le flux de création du personnage, refusait le registre canonique manquant dans les builds sans `WITH_DEV_AUTOMATION_TESTS`. Or Unreal Editor est typiquement compilé **avec** cette macro, même pendant une vraie session de développement hors tests. Il devenait donc possible de charger les anciens `DA_Weapon_*` et `DA_Item_*` à la place de `DA_ItemSystem` quand le registre n'était pas fourni.

Le refus exige maintenant réellement `GIsAutomationTesting`, via `FGridApprovedStarterLegacyFixtureGate`. Le comportement des essais 5C de parité historique en Automation, des 18 compositions approuvées et des appels avec registre configuré reste inchangé. La nouvelle suite `Grimrock.RPG.ARM.CLEAN06_5H.RegistrylessApprovedStarterFixture` contrôle le contrat booléen Automation actif/inactif ; `Grimrock.RPG.ARM.CLEAN06_5C.AllApprovedKitsCanonical` continue de couvrir les véritables appels, la parité et les rejets de registres vides.

**Hors périmètre :** ancien générateur de 12 kits `TryPopulateTransientClass` encore utilisé par un audit Editor historique, types C++ et outils 06.2, palette d'icônes, redirecteur, assets de tests et Readables v27. Aucun asset, SaveGame, niveau ou UI n'est modifié.

**Validation :** double statut Windows SUCCESS sur le SHA exact avant autre lot.
