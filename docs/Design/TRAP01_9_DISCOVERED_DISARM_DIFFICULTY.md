# TRAP01.9 — Difficulté visible après découverte

**Statut :** code publié sur `master`, validation UE5.5.4/PIE à réaliser.

## Objectif

Depuis TRAP01.8, une erreur de désamorçage peut activer les dégâts du piège
ou ouvrir sa fosse PIT03. Après **découverte effective**, le joueur doit
connaître la difficulté du jet `Skill_Traps` avant de décider s'il tente
de désamorcer le mécanisme.

`Difficulté de désamorçage` est le **DC**, pas le niveau du personnage :
un d20 + rang Pièges + DEX + Classe + Groupe doit atteindre ce seuil.

## Affichage

Le message `Dungeon.Trap.Detected` conserve l'observateur, la position,
le d20 de Perception et sa **difficulté de détection**. Il ajoute sur une
ligne distincte `Difficulté de désamorçage : N`.

Le survol du modèle révélé (`AGridGenericObjectActor`) présente :

```text
Piège détecté
Difficulté de désamorçage : 29
Clic gauche pour tenter de le désamorcer (Pièges + outils).
```

Pour un piège dont le DataAsset interdit le désamorçage :
`Piège détecté\nDésamorçage impossible`, sans incitation au clic et
sans niveau numérique.

Ces textes se fondent sur **l'autorité existante**
`AGridLevelRuntimeActor::GetTriggerTrapDisarmDifficulty`, qui utilise
`GridDungeonTrapInstance::ResolveDisarmDifficulty` : la surcharge propre
au placement prévaut sur le défaut du DataAsset. Le nouveau
`GetDungeonTrapDisarmHint` n'expose aucune valeur avant que l'état soit
`Detected` ou `Triggered`, ni après désamorçage/désactivation. Une fosse
ou un objet non reconnu comme piège ne divulgue rien.

Aucun tirage de dé, nouveau widget, changement aux règles de découverte,
modification d'asset partagé ou recalcul des chances de réussite.

## Vérification

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP_PIT01"
```

Recette PIE : piège caché = aucune information ; détection = vrai DC de
désamorçage en journal et au survol ; non désamorçable = message explicite ;
Save/Continue conserve le DC après découverte. Tester une surcharge
d'instance différente du DataAsset partagé.
