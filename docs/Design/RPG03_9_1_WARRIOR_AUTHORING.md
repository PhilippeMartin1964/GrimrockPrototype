# RPG03.9.1 — Authoring Guerrier

Date : **5–6 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Statut : **VALIDÉ / MATÉRIALISÉ — 11/11 lors du jalon Guerrier ; couvert ensuite par la campagne globale RPG03 193/193**

## Périmètre

Le Guerrier matérialise **15 talents conceptuels** répartis en trois branches :

- Gardien ;
- Brise-ligne ;
- Maître d'armes.

La Spécialisation martiale possède trois variantes exclusives (Slashing/Piercing/Bludgeoning), ce qui produit **17 Choice records** pour 15 talents conceptuels.

L'authoring contient **10 actions actives** et **5 Status Effects de production**.

## Statuts de production

- `Status_Guarded`
- `Status_Stunned`
- `Status_Fortified`
- `Status_KnockedDown`
- `Status_Warlord`

## Architecture

`FRPGWarriorAuthoring::ConfigureClass()` configure le `DA_Class_Warrior` depuis les primitives génériques RPG03 :

- C1 pour les applications de statuts et ArmorGates ;
- C2 pour dégâts, Accuracy, critique et mitigation ;
- C3 pour restauration/dégâts directs d'armure ;
- C4 pour Riposte/Interception ;
- C8 pour les cibles de groupe et zones ;
- RPG03.9.0 pour les attaques basées sur l'arme équipée.

Aucune logique runtime ne branche sur un `Talent_Warrior_*`.

Depuis RPG03.10, `ConfigureClass()` appelle aussi `FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants()`, autorité commune des 10 Talent Points.

## Matérialisation

Le commandlet `RPGWarriorAuthoring`, appelé par `Scripts/AuthorRPGWarrior.ps1`, met à jour :

- `DA_Class_Warrior` ;
- les cinq `DA_Status_*` listés ci-dessus.

La matérialisation initiale a été enregistrée par le jalon RPG03.9.1A ; les grants canoniques ont ensuite été rematérialisés avec les cinq autres classes dans RPG03.10.

## Validation

Le jalon Guerrier a été validé **11/11**. La référence finale est la campagne :

```text
Grimrock.RPG.RPG03
193/193
0 warning
0 failed
0 not run
exit 0
```
