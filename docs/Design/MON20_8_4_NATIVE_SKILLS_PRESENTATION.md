# MON20.8.4 — Native Skills/Talents Runtime Presentation

Statut : **CORRECTIF DE PRÉSENTATION IMPLÉMENTÉ — VALIDATION UE5.5.4 / PIE À FAIRE**  
Date : **24 août 2026**  
Jalon : **MON20.8.4 — Skills/Talents Page Read Model & Menu Integration**

## Constat

Après le reparent correct de `WBP_GridSkills` vers `UGridSkillsWidget`, la page restait visuellement identique en PIE : seul le placeholder `Competences` apparaissait.

La cause était structurelle : MON20.8.4 avait livré le read model (`View.Skills`, `View.Talents`, points de talent, personnage sélectionné) et le bridge natif, mais aucun widget UMG n'était construit pour afficher ces données.

Le reparent était donc valide, mais insuffisant pour produire un changement visuel.

## Correctif

`UGridSkillsWidget` construit maintenant au runtime une présentation native minimale dans le `Border` racine existant de `WBP_GridSkills`.

```text
WBP_GridSkills
  Border racine
      ↓ runtime
  ScrollBox
      ↓
  VerticalBox
      ├── Compétences & talents
      ├── personnage sélectionné
      ├── points de talent
      ├── Compétences
      │   └── nom / rang / attribut / entraînement / description
      └── Talents acquis
          └── nom / coût / description
```

Aucun Graph Blueprint, nouveau WBP ou nouvel asset n'est requis.

Le placeholder peut rester visible dans le Designer : il est remplacé seulement lorsque `RefreshSkills()` s'exécute au runtime.

## Autorités préservées

Le correctif ne change aucune règle gameplay :

```text
SelectedCharacterIndex -> UGridPartyInventoryComponent
SkillRanks              -> FGridCharacterInventoryState
Skill definitions       -> URPGSkillAsset
Talents                 -> MON15 / MON20.7 ProgressionChoices
Talent points           -> MON15 progression balance
```

La page reste strictement read-only.

## États vides

La présentation expose explicitement les situations suivantes :

```text
View invalide              -> Aucun personnage sélectionné ou données indisponibles.
aucune définition Skill    -> Aucune compétence définie.
aucun talent acquis        -> Aucun talent acquis.
```

Ainsi, une page sans Skill de production reste diagnostiquable et ne ressemble plus à un placeholder non connecté.

## Fichiers modifiés

```text
Source/GrimrockPrototype/Public/UI/GridSkillsWidget.h
Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp
docs/Design/MON20_8_4_NATIVE_SKILLS_PRESENTATION.md
```

Aucun `.uasset` / `.umap` n'est modifié par ce correctif GitHub.

## Validation attendue

1. compiler `GrimrockPrototypeEditor` sous UE5.5.4 ;
2. lancer PIE ;
3. ouvrir le menu puis l'onglet `Compétences` ;
4. vérifier que le placeholder est remplacé par la présentation native ;
5. changer le personnage sélectionné et vérifier le refresh ;
6. vérifier les états Skills/Talents présents ou vides ;
7. relancer `Grimrock.MON20.8` après validation visuelle.

MON20.8.4 ne sera clos qu'après cette validation PIE.
