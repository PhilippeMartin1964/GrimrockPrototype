# RPG03.9.5B2 — Prêtre : Exorcisme et clôture logique

Date : **6 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendances : `RPG03.9.5A` validé 6/6, `RPG03.9.5B1` validé 6/6  
Statut : **CODE AUTHORING — VALIDATION UE UTILISATEUR REQUISE**

## Résultat attendu

La branche Exorcisme complète le Prêtre à :

- **15/15 talents conceptuels** ;
- **15 Choice records** ;
- **14 actions actives** ;
- 3 branches consommables avec exactement 5 Talent Points chacune.

Aucun `.uasset` n'est matérialisé dans B2.

## Exorcisme

1. **Lumière sacrée** — 2 PA / 4 mana, Hostile R5, 5 + WIS mod + Religion Rank Holy, +50 % contre Undead/Demon.
2. **Repousser les morts-vivants** — 3 PA / 7 mana, Area2 centrée groupe, Undead uniquement, 4 Holy ; sous MagicalArmor=0 après dégâts : push 1 cellule + `Status_TurnedUndead` (-4 Initiative, 1 round).
3. **Dissipation sacrée** — 2 PA / 6 mana, AllyOrHostile R3, CD2 ; allié : jusqu'à 2 Debuffs tagués `Necrotic|Curse` ; hostile : Undead uniquement, 1 Buff `Dispel.Magical`.
4. **Châtiment** — 3 PA / 8 mana, Hostile R4, CD2, 10 + 2×WIS mod + Religion Rank Holy, +50 % contre Undead/Demon, sans critique.
5. **Exorcisme majeur** — 4 PA / 14 mana, Area2 à R4, CD5, uniquement Undead/Demon/Summoned, 14 + 2×WIS mod + Religion Rank Holy ; `Status_Banished` 1 Turn sous MagicalArmor=0.

## Autorités

- catégories de monstres : `TargetFilter` / contexte C8 ;
- bonus Undead/Demon : modificateurs C2 filtrés par ActionId + catégorie ;
- push : C5 avec ArmorGate post-dégâts ;
- Initiative temporaire et Banished : C1 ;
- purge déterministe : C8 existant ;
- aucune comparaison runtime sur `TalentId`, `StatusId` ou catégorie spécifique.

## Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.5B2"
```

Après réussite utilisateur, un jalon séparé matérialisera `DA_Class_Priest` et les Status Prêtre via Unreal Editor.
