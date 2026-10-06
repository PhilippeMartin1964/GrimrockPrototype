# RPG03.9.5A — Support générique Prêtre

Date : **6 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Ajouter uniquement les primitives génériques réellement manquantes pour authorer les 15 talents du Prêtre, sans branche runtime sur un `TalentId` et sans créer de second système de soin, de statut, de déplacement ou de réaction.

## Extensions

- scaling direct de soin : base + modificateur d'attribut + rang de Skill, avec plancher optionnel de PV cible ;
- modificateur générique `OutgoingHealingPercentModifier`, séparé de `PositiveEffectPercentModifier` afin de ne pas augmenter les restaurations d'armure ;
- soin périodique de Status, exécuté sur la même frontière de durée que le DoT existant ;
- scaling direct de dégâts étendu à un modificateur d'attribut supplémentaire en plus du rang de Skill ;
- réactions capables d'exiger que le propriétaire soit la source de l'événement et qu'un dégât ait réellement été appliqué ;
- mouvement forcé `TargetCombatant` après attaque avec ArmorGate optionnel ;
- Area générique centrée sur la cellule du groupe, exécutable directement sans sélectionner une cellule artificielle.

Ces primitives couvrent notamment Soin renforcé, Régénération, Soin de groupe, Miracle, Sanctuaire, Lumière sacrée/Châtiment et Repousser les morts-vivants. La Dissipation sacrée réutilise le chemin C8 existant `RequestCharacterCombatActionOnMonsterTarget(...)` pour les Effects hostiles : aucune primitive supplémentaire n'est nécessaire.

## Autorités conservées

- `FGridCombatModifierResolver` reste l'autorité de composition des modificateurs ;
- `UGridStatusEffectLifecycleSubsystem` reste l'autorité des ticks et durées de Status ;
- C3 reste l'autorité de restauration d'armure ;
- C4 reste l'autorité des réactions ;
- C5 / `UGridMonsterMovementComponent` restent l'autorité du déplacement ;
- C8 reste l'autorité du ciblage/batch ;
- aucune donnée dérivée supplémentaire n'est sauvegardée.

## Automation

Filtre prévu :

```text
Grimrock.RPG.RPG03.9.5A
```

Couverture :

1. soin direct WIS + Skill + bonus de soin ;
2. plancher de PV type Miracle ;
3. dégâts avec second modificateur d'attribut + Skill ;
4. rupture de Status uniquement lorsque le propriétaire inflige réellement des dégâts ;
5. profil de régénération périodique ;
6. Area centrée groupe + push forcé sous ArmorGate.

La réussite UE n'est acquise qu'après sortie `Scripts\ValidateUE.ps1` fournie par l'utilisateur.
