# RPG03.9.4C — Mage Évocateur : Affinité élémentaire + Surcharge

Statut : **VALIDÉ — intégré à l'authoring Mage final et couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Périmètre

Ce jalon regroupe l'authoring complet des deux premiers talents Évocateur :

- Affinité élémentaire ;
- Surcharge élémentaire.

Les treize autres talents Mage restent hors de ce jalon. Aucun comportement incomplet n'est authoré sous forme de placeholder.

## Affinité élémentaire

Le talent logique `Talent_Mage_Evoker_ElementalAffinity` est matérialisé par quatre ChoiceIds exclusifs :

- `Talent_Mage_Evoker_ElementalAffinity_Fire` ;
- `Talent_Mage_Evoker_ElementalAffinity_Frost` ;
- `Talent_Mage_Evoker_ElementalAffinity_Air` ;
- `Talent_Mage_Evoker_ElementalAffinity_Earth`.

Ils utilisent tous `TalentGroup_Mage_Evoker_ElementalAffinity` et accordent l'alias logique
`Talent_Mage_Evoker_ElementalAffinity`.

Chaque variante projette +15 % de dégâts pour les actions `SourcePolicy=Spell` portant le tag
`Spell.School.<School>`.

## Surcharge élémentaire

`Action_Mage_ElementalOverload` :

- 1 PA ;
- 4 mana ;
- Self ;
- cooldown 3 ;
- applique `Status_ElementalOverload` pour 1 Turn.

Le statut est unique. Il contient quatre profils de modifier et quatre réactions, conditionnés à la fois par :

- le ChoiceId d'affinité du propriétaire ;
- le tag `Spell.School.*` de l'action.

Pour l'affinité Fire, par exemple :

```text
Selected Affinity_Fire
+ Status_ElementalOverload
+ Spell.School.Fire
= +15 % permanent +35 % temporaire
= +50 % total
= consommation du Status après ActionResolved
```

Un sort Frost du même personnage ne reçoit ni le +15 % Fire ni le +35 % temporaire et ne consomme pas la surcharge.

## Assets

Le commandlet `RPGMageAuthoring` modifie/crée :

- `DA_Class_Mage` ;
- `DA_Status_ElementalOverload`.

Aucun autre asset binaire n'est attendu.

## Authoring local

Ce document décrit le jalon historique C. Depuis RPG03.9.4D, `Scripts/AuthorRPGMage.ps1` matérialise la branche Évocateur complète et lance la campagne D.

La campagne historique `Grimrock.RPG.RPG03.9.4C` reste disponible comme régression des deux premiers talents.

Validation finale de référence : `Grimrock.RPG.RPG03` — 193/193, 0 warning, 0 échec, exit 0 (6 octobre 2026).
