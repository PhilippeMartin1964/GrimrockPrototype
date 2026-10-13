# UI-LOCALIZATION01 — P0 localization closure summary

Date de clôture : **13 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Statut : **CLOSED**  
Baseline finale : **6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e**

## 1. Objet de la campagne

UI-LOCALIZATION01 a supprimé les principales chaînes joueur P0 construites
directement en C++ et a établi une autorité de localisation stable pour le
vocabulaire UI générique.

La campagne ne cherchait pas à déplacer tout `FText` vers une String Table.
Elle a conservé la séparation suivante :

~~~text
contenu RPG authoré
    -> Data Assets

vocabulaire UI générique partagé
    -> Localization/Source/UI.csv
    -> ST_UI générée

LOCTEXT / NSLOCTEXT déjà localisables
    -> C++ en attendant la campagne de couverture

mécanique / valeurs
    -> C++

diagnostics techniques
    -> C++ / logs / stable reasons
~~~

## 2. Pipeline établi

~~~text
Localization/Source/UI.csv
    -> UIStringTableAuthoring
    -> /Game/Localization/StringTables/ST_UI
    -> Localization Target Game
    -> Game.manifest / archives / PO
    -> Game.locres fr + en
~~~

Culture native : `fr`.  
Culture de smoke : `en`.

Le script `ApplyEnglishSmokeTranslations.ps1` est un fixture de validation,
pas un catalogue anglais de production complet.

## 3. Jalons fermés

| Jalon | Objet | Source / contrat | Matérialisation |
| --- | --- | --- | --- |
| 01.1 | Talent TextSet integrity | `d53e4310` + asset `e8d57f9c` | n/a |
| 01.2 | Player-facing C++ audit | `6ac148d3` | n/a |
| 01.3 | CSV / String Table contract | `788733b6` | n/a |
| 01.4 | CSV importer + ST_UI | `24dcee4d` | `0191ed7d` |
| 01.5 | Character Creation | `975188e7` + `2b2817f9` | `f9620718` |
| 01.6 | Unreal target + fr/en smoke | `6449fdb3` ... `fb4d873c` | `06a0cfd1` |
| 01.7 | Status Effects | `6b8299cf` ... `042acb2f` | `08f6f54a` |
| 01.8 | World interactions | `ce871d5c` | `429b0ace` |
| 01.9 | Combat presentation | `f9d55611` | `ff045086` |
| 01.10 | Inventory / SaveLoad / Spellbook / Skills | `f0321337` | `55a74f90` |
| 01.11 | Recruitment error boundary | `d98bd006` | `e8e98765` |
| 01.12 | Final P0 closure | `62203a68` | `6e5cb5f1` |

## 4. Garde-fous finaux

Filtre parent :

~~~text
Game.UI.Localization.LOCALIZATION01
~~~

À la clôture, la validation locale utilisateur a confirmé :

~~~text
Succeeded              : 13
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

Le smoke runtime fr/en a également été validé après la fermeture 01.12A.

## 5. Frontières explicitement hors P0

Ne sont pas des dettes P0 de UI-LOCALIZATION01 :

- diagnostics Save / persistence non affichés directement ;
- outils PIE et console développeur ;
- IDs et fallbacks techniques de contenu invalide ;
- valeurs et notations mécaniques ;
- validations internes d'assets et de runtime ;
- `LOCTEXT/NSLOCTEXT` déjà localisables.

## 6. Limite connue du target Game

À la clôture de 01.12, `Config/Localization/Game_Gather.ini` rassemble
volontairement uniquement :

~~~text
Content/Localization/StringTables/*
~~~

Le Gather ne parcourt donc pas encore les sources C++ contenant des
`LOCTEXT/NSLOCTEXT`, ni nécessairement toutes les autres autorités `FText`
authorées dans les assets.

Ce point n'est pas une réouverture P0. Il devient l'objet de la campagne
suivante.

## 7. Prochaine campagne

~~~text
UI-LOCALIZATION02.1 — Localization coverage audit
~~~

Objectif : mesurer ce que le target `Game` couvre réellement et classifier
les autorités textuelles encore hors Gather avant toute modification du
pipeline.

Baseline de départ obligatoire :

~~~text
6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e
~~~

UI-LOCALIZATION02 ne doit pas déplacer des textes par réflexe. Il doit d'abord
établir la couverture réelle de :

- ST_UI ;
- `LOCTEXT/NSLOCTEXT` C++ ;
- Data Assets RPG et gameplay ;
- UMG / assets contenant du `FText` ;
- contenu authored de niveau ;
- diagnostics et IDs à exclure.

## 8. Documents de référence

- `UI_LOCALIZATION01_1_TALENT_TEXTSET_INTEGRITY.md`
- `UI_LOCALIZATION01_2_PLAYER_FACING_CPP_TEXT_AUDIT.md`
- `UI_LOCALIZATION01_3_UI_STRING_TABLE_CSV_AUTHORING_CONTRACT.md`
- `UI_LOCALIZATION01_4A_STRING_TABLE_CSV_IMPORTER.md`
- `UI_LOCALIZATION01_5A_CHARACTER_CREATION_TEXT_MIGRATION.md`
- `UI_LOCALIZATION01_6A_UNREAL_LOCALIZATION_TARGET.md`
- `UI_LOCALIZATION01_7A_STATUS_EFFECT_PRESENTATION.md`
- `UI_LOCALIZATION01_8A_INTERACTION_PROMPTS.md`
- `UI_LOCALIZATION01_9A_COMBAT_PRESENTATION.md`
- `UI_LOCALIZATION01_10A_RESIDUAL_P0_PRESENTATION.md`
- `UI_LOCALIZATION01_11A_RECRUITMENT_ERROR_BOUNDARY.md`
- `UI_LOCALIZATION01_12_FINAL_P0_CLOSURE_AUDIT.md`

## 9. Conclusion

`UI-LOCALIZATION01` est fermé.

Aucune nouvelle migration P0 ne doit être ajoutée à cette campagne sauf
régression explicitement démontrée. Les travaux futurs de couverture,
traduction exhaustive et sélection de langue appartiennent aux campagnes
UI-LOCALIZATION02 et suivantes.
