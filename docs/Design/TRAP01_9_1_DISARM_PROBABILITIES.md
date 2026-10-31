# TRAP01.9.1 — Prévision des chances et du risque de désamorçage

**Statut : code sur master ; compilation/tests UE5.5.4 et recette PIE à confirmer.**

## Calcul

Dès qu'un piège a été détecté, le survol conserve son DC effectif
(`ResolveDisarmDifficulty`) et estime les chances avec **le groupe actuel** :

- **Réussite** : le d20 + rang Pièges + DEX + classe + groupe atteint le DC.
- **Échec sûr** : jet insuffisant mais compris dans la marge d'échec sûr du
  meilleur désamorceur, tant qu'elle n'a pas été consommée pour ce GUID.
- **Déclenchement** : échec non protégé. Une tentative dangereuse passe par
  l'activation commune TRAP01.8.

Les trois résultats totalisent 100 %. Les vingt faces sont explicitement
comptées sans tirage, état changé, succès automatique sur un 20 ni échec
automatique sur un 1. Un score de 0 % est possible.

Exemple, DC=19, bonus fixe +5, marge 2 disponible :

```text
Piège détecté
Difficulté de désamorçage : 19
Meilleur désamorceur : Elias
Réussite : 35 % | Échec sûr : 10 % | Déclenchement : 55 %
Clic gauche pour tenter de le désamorcer (Pièges + outils).
```

À bonus +9 : 55 % / 10 % / 35 %.
Après consommation de la marge : 35 % / 0 % / 65 %.

## Une seule autorité et lecture seule

`FRPGSkillCheckService::TryPreviewBestPartySkillCheck` choisit le meilleur
membre via le **même projecteur de composants et la même sélection** que
`TryResolveBestPartySkillCheck`, mais sans `FRandomStream`.
`OutStaticResult.bResolved` reste false et `Roll` reste 0.
La résolution normale utilise toujours exactement un d20.

`AGridLevelRuntimeActor::TryGetDungeonTrapDisarmOdds` vérifie
`GetTriggerTrapDisarmDifficulty`, charge le skill canonique, utilise
**DEX** (identique à l'action), vérifie l'objet `Tool.Disarm` dans
l'inventaire et **masque les membres sans leur propre trousse** dans
une copie du groupe, exactement comme le désamorçage réel. Le meilleur
personnage est défini par les bonus fixes sans jet aléatoire.
Les bonus collectifs respectent leurs règles de cumul existantes.

Le calcul se fait à chaque survol, donc tenir compte d'un talent acquis,
de la montée de rang, de la DEX, des membres vivants, des outils et de
`SafeDisarmFailureUsedTrapIds` après Save/Continue ne nécessite pas de
persister des probabilités. Pas de widget nouveau, pas de modification
aux données de placement ni de mutation de la partie.

Si aucun personnage n'est éligible (pas d'outils, entraînement, compétence
indisponible) le survol garde le DC mais affiche
`Évaluation indisponible`, jamais une probabilité trompeuse de 0 %.
Le pourcentage est une **prévision conditionnelle** d'une tentative
possible ; la distance, les murs et le combat restent contrôlés lors
de l'action réelle, et la preview ne les contourne pas.

Le journal de découverte conserve le DC sans probabilité figée.

## Validation

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.9.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.6.Skills"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.9.3.Support"
```

En PIE : survol d'un piège détecté, sans aucun outil, puis avec
Saboteur entraîné et équipé ; progresser et revenir ; consommer une
protection sur un échec sûr et contrôler le risque actualisé ; Save/Continue ;
aucun indice pour les pièges cachés.
