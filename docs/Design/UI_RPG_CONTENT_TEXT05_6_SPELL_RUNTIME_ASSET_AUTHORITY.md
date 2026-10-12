# UI-RPG-CONTENT-TEXT05.6 — Spell runtime serialized asset authority

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT05.6**

## Objet

Basculer la production Spell sur les quatre `UGridSpellDefinitionAsset`
matérialisés par TEXT05.5B sans modifier les consommateurs runtime.

## Autorité finale

```text
DA_Spell_ArcaneBolt
DA_Spell_LesserHeal
DA_Spell_Haste
DA_Spell_CurePoison
        ↓
FGridProductionSpellLibrary
        ↓
Spellbook / persistence / party spellbook / TurnManager / presentation
```

Les DataAssets possèdent désormais la définition complète et la présentation.
Le C++ ne recrée plus aucun nom, description, coût ou effet Spell.

## Façade conservée

L’API existante reste inchangée : `Make*`, `Make*Presentation()`,
`TryBuildPresentationProfile()` et `BuildAll()`.

Ces fonctions chargent désormais les assets sérialisés. Les consommateurs
runtime n’ont donc pas besoin d’être modifiés.

## Échec dégradé

Si un asset est absent ou invalide, une erreur est journalisée. `BuildAll()`
n’ajoute pas la définition invalide et les helpers `Make*` ne recréent pas
l’ancien contenu : ils retournent seulement un fallback technique portant le
`SpellId`.

## Packaging

Le dossier
`/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells`
est ajouté à `DirectoriesToAlwaysCook`.

## Nettoyage

L’infrastructure temporaire utilisée uniquement pour matérialiser les assets
est supprimée après usage :

```text
FGridSpellCatalogAuthoring
UGridSpellCatalogAuthoringCommandlet
UIRPGCONTENTTEXT055SpellAuthoringTests.cpp
Scripts/AuthorGridSpells.ps1
```

## Test

`Grimrock.UI.RPG.CONTENTTEXT05.SpellRuntimeAssetAuthority` vérifie :

- quatre assets chargés et valides ;
- quatre définitions retournées par `BuildAll()` ;
- texte/mécanique runtime identiques aux assets ;
- présentation runtime identique aux assets ;
- absence de prose et d’authoring mécanique dans la bibliothèque C++ ;
- cook explicite du dossier Spell.

## État

```text
P1 Skills          CLOSED
P2 Spells          CLOSED
P3 Attack_Unarmed  CLOSED — localized system vocabulary exception
Audit final        CLOSED
```

Après validation de TEXT05.6, la campagne n’a plus que deux petits tickets.
