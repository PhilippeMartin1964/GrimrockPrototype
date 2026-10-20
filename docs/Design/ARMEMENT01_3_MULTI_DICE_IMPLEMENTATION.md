# ARMEMENT01.3 — Moteur multi-dés C++

Date : 20 octobre 2026
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UE5 EN ATTENTE**
Parent : docs/Design/ARMEMENT01_2_DICE_ENGINE_TECHNICAL_CONTRACT.md

## Périmètre

- FGridDamageDiceProfile dans FGridAttackDefinition, dés activés explicitement (d4, d6, d8, d10, d12, d20).
- Validation de DiceCount [1..32], FlatBonus >= 0, prévention des débordements et correspondance stricte de MinDamage/MaxDamage avec les bornes de NdM+K.
- FGridCombatResolver reste l'autorité unique : d20 de toucher, N vrais jets indépendants, FlatBonus une seule fois, chaîne existante WD, critique, résistances et armures.
- API déterministe ResolveAttackFromDiceRolls contrôlant chaque valeur injectée. ResolveAttackFromRolls (total scalaire) préservé pour l'ancien mode ; il rejette une attaque multi-dés.
- FGridCombatRollTrace.DiceFlatBonus et conservation des dés dans IndividualRolls sur une attaque résolue ; formatage du flux événementiel sans relance de dés.
- Consommation des dés après le d20 même en cas de raté, sans dégâts appliqués ni dévoilement des faces dans la trace d'une attaque manquée.
- Aucun asset binaire, aucune classe, aucun niveau d'objet, aucune rareté, aucun WBP modifié.

## Contrat d'authoring

Lorsqu'une attaque d'arme utilise de vrais dés, elle doit déclarer simultanément :

| Propriété | Exemple |
|---|---|
| DamageDice.bEnabled | true |
| DamageDice.DiceCount | 2 |
| DamageDice.DiceSides | 8 |
| DamageDice.FlatBonus | 4 |
| MinDamage (projection de compatibilité) | 6 |
| MaxDamage (projection de compatibilité) | 20 |

Le nouveau jet est 2d8+4 : min 6, max 20, moyenne 13. Les deux champs Min/Max doivent correspondre aux bornes dérivées pour satisfaire IsValid(), mais ne contrôlent pas un deuxième tirage RNG en mode multi-dés.

Les profils existants gardent DamageDice.bEnabled=false et continuent de produire un RandRange(MinDamage,MaxDamage). Le flux d'événements conserve sa notation virtuelle historique exclusivement pour ces anciens profils Range.

## Fichiers C++

- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTypes.h
- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTraceTypes.h
- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatResolver.h
- Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatResolver.cpp
- Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatEventFeedFormatter.cpp
- Source/GrimrockPrototype/Private/Tests/GridARMEMENT013MultiDiceTests.cpp

## Automatisation proposée

Sept tests sous le filtre Grimrock.RPG.ARMEMENT01.3 vérifient : acceptation des dés, incohérences, bornes/moyennes, jets individuels, extrêmes, rejet d'injections illégales, échecs, critiques, WD, ordre de consommation RNG, rétrocompatibilité et équipements passifs.

Dans PowerShell, depuis D:\Development\GrimrockPrototype :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.3"

Puis vérification de non-régression avec :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.RPG03"

IMPORTANT : compilation et tests non exécutés sur une installation UE locale par l'assistant. Les résultats ne sont validés qu'après remise des logs TD04.2 de l'utilisateur.

## Hors périmètre

- La conversion du catalogue Excel en DataAssets.
- Le niveau fixe, les pénalités de niveau, la rareté, les runes, la forge, l'alchimie.
- La mise à jour de tous les anciens profils de monstres, sorts et pièges : Legacy reste inchangé.

Prochain jalon : ARMEMENT01.4 — authoring contrôlé des nouveaux profils d'armes.
