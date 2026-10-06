# RPG03.9.4E2 — branche Mage Arcaniste complète

Statut : **VALIDÉ — intégré à l'authoring Mage final et couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Objectif

E2 authorise les cinq talents Arcaniste sur le socle générique validé par E1 :

- Bouclier arcanique ;
- Dissipation ;
- Manipulation runique ;
- Téléportation courte ;
- Maîtrise de l'Arcane.

Aucun `if Mage` n'est ajouté au runtime dans E2.

## Bouclier arcanique

`Action_Mage_ArcaneShield` :

- SourcePolicy = Spell ;
- SourceTag = `Spell.School.Arcane` ;
- 2 PA / 5 mana ;
- Ally R3 ;
- CD2 ;
- restaure MagicalArmor = `6 + INT mod + Skill_Arcana Rank` ;
- clamp au pool MagicalArmor de référence.

Le scaling réutilise le resolver C3 existant.

## Dissipation

`Action_Mage_Dispel` :

- SourcePolicy = Spell ;
- SourceTag = `Spell.School.Arcane` ;
- 2 PA / 6 mana ;
- AllyOrHostile R4 ;
- ligne de vue requise ;
- CD2.

Profils de retrait :

- côté Party : 1 Debuff tagué `Dispel.Magical` ;
- côté Hostile : 1 Buff tagué `Dispel.Magical` ;
- priorité résolue par E1 : Potency décroissante puis EffectId lexical.

Le tag `Dispel.Magical` signifie **amovible par la primitive Dissipation**, pas nécessairement « créé par une source magique ». Il s'agit d'une propriété sémantique du statut, pas d'un historique de provenance.

E2 ajoute ce tag aux statuts Mage authorés :

- `Status_ElementalOverload` ;
- `Status_Burning` ;
- `Status_Slow`.

Les statuts partagés `Status_Stunned` et `Status_Immobilized` conservent leur autorité Warrior/Rogue et ne sont pas réécrits par le Mage.

## Manipulation runique

`Talent_Mage_Arcanist_RunicManipulation` :

- `Skill_Runes` CheckModifier +2 ;
- Spell + `Spell.School.Arcane` ;
- cible portant `Rune` ou `Construct` ;
- dégâts sortants +20 %.

E1 expose à la fois :

- les `SemanticTags` génériques d'une définition de monstre ;
- son `CategoryId` comme tag sémantique de fallback.

Aucune auto-réussite n'est introduite dans les Skill Checks.

## Téléportation courte

`Action_Mage_ShortTeleport` :

- SourcePolicy = Spell ;
- SourceTag = `Spell.School.Arcane` ;
- 3 PA / 8 mana ;
- Cell R2 ;
- ligne de vue requise ;
- CD4 ;
- `bRelocatePartyToTargetCell=true`.

La primitive E1 valide cellule libre/marchable, portée, murs/portes/frontières et n'utilise aucun PAM. L'exécution ne déclenche pas les événements ordinaires de changement de cellule, donc aucun téléporteur ou changement de niveau ne chaîne automatiquement.

## Maîtrise de l'Arcane

`Talent_Mage_Arcanist_ArcaneMastery` projette sur les actions :

- SourcePolicy = Spell ;
- RequiredSourceTag = `Spell.School.Arcane`.

Effets :

- ManaCost -1 ;
- MinimumManaCost = 1 pour toute action ayant un coût positif ;
- RangeCells +1, plafonné par le contrat runtime à 32 ;
- dégâts sortants +15 %.

Les actions gratuites restent gratuites.

## Résultat Mage après E2

Authoring attendu :

- 6 actions actives au total : 3 Évocateur + 3 Arcaniste ;
- 13 Choice records : 8 Évocateur + 5 Arcaniste.

## Assets modifiés

Le commandlet Mage met à jour uniquement :

- `DA_Class_Mage` ;
- `DA_Status_ElementalOverload` ;
- `DA_Status_Burning` ;
- `DA_Status_Slow`.

Aucun nouvel asset n'est créé par E2.

## Materialisation et validation locale

Ce document décrit le jalon historique E2. Depuis RPG03.9.4F2, `Scripts/AuthorRPGMage.ps1` matérialise les trois branches Mage complètes et lance la campagne F2.

La campagne historique `Grimrock.RPG.RPG03.9.4E2` reste disponible comme régression de la branche Arcaniste.

Validation finale de référence : `Grimrock.RPG.RPG03` — 193/193, 0 warning, 0 échec, exit 0 (6 octobre 2026).
