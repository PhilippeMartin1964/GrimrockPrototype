# MON14.2 — Check-list de validation UE 5.5.4

Ce fichier ne remplace pas `MON14_2_DIRECTIONAL_PERCEPTION_PATROL_DATA.md`. Il sert uniquement de check-list de validation locale après compilation.

## Build / Automation

Utiliser de préférence le harness local du projet :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON14.2"
```

Pour une régression plus large, exécuter au minimum les familles suivantes selon le périmètre modifié :

```text
Grimrock.Monsters.MON4
Grimrock.Monsters.MON5
Grimrock.Monsters.MON7
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14.1
Grimrock.Monsters.MON14.2
Grimrock.Monsters.MON
```

## PIE — perception directionnelle

1. Placer/choisir un Rat géant avec un `Facing` connu.
2. Se placer dans son axe avant : la vue peut engager automatiquement.
3. Revenir à la même distance dans son axe arrière : aucune vue.
4. Se placer latéralement : aucune vue.
5. Tourner le rat vers le groupe : l'engagement devient possible au prochain événement de perception sûr.
6. Vérifier qu'un mur et une porte fermée bloquent toujours la vue.
7. Vérifier qu'une position uniquement audible produit `Alert` sans combat automatique.

## PIE — Dormant

1. Utiliser un `MonsterSpawn` avec `Spawn at Start` coché et `InitialMonsterState=Dormant`.
2. Vérifier que l'Actor est visible/présent au démarrage mais ne démarre pas en état `Idle`.
3. Entrer dans son axe visuel : vérifier `Dormant -> Alert` puis engagement MON14.1.
4. Vérifier que `Spawn at Start=false` supprime uniquement la création initiale de l'Actor ; cela ne représente pas l'état `Dormant`.
5. Sauvegarder dans un slot de test distinct de `GrimrockParty`, changer l'état runtime, faire Continue et vérifier que l'état sauvegardé prévaut sur l'état initial du placement.

## PIE — données de patrouille

1. Définir `PatrolMode=Loop` ou `PingPong` avec au moins deux waypoints.
2. Vérifier que les données sont présentes sur l'Actor runtime.
3. Vérifier l'exécution MON14.3 : déplacement case par case, Facing d'arrivée et attentes.
4. Tester une route invalide dans un asset de test et vérifier le diagnostic de validation.

## Critère de clôture

MON14.2 reste validé lorsque le build passe, les suites concernées sont vertes et les scénarios PIE confirment Facing, Dormant, distinction `Spawn at Start` / `InitialMonsterState` et données de patrouille.
