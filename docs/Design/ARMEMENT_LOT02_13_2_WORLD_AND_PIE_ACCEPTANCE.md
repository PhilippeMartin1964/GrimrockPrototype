# ARMEMENT-LOT02.13.2 — Recette fonctionnelle en jeu

Statut 28.10.2026 : instrumentation publiée, tests locaux et PIE encore à réaliser.
Référence : `56c6371b` a corrigé la liaison ARM048 aux flèches ; prévalidation
locale du jalon LOT02.13.1 : 8/8 LOT02 et 7/7 Industrial, zéro échec.

## Matrice automatisée : niveau temporaire, vrais DataAssets

Le test `Grimrock.RPG.ARMEMENT.LOT02.RealWorldPickupAndReturn` crée un UWorld
temporaire, un niveau de grille 4×4 (200 cm) en mémoire, un runtime et un pawn.
Pour chaque arme ARM038, ARM048, ARM054, ARM057, ARM058, ARM061, ARM062,
ARM067, ARM069, ARM070 et ARM081 il réalise :

- création de l'objet au sol via `TryDropItemInstanceAtCell` et comparaison du mesh ;
- ramassage via `TryPickupItemActor`, équipement, extraction avec
  `TryExtractOneEquippedItemForWorldTransfer` ;
- redépôt physique et deuxième ramassage sans duplication, conservation GUID et N1 ;
- création d'un `AGridThrownItemActor` réel pour les trois armes de jet
  (sans simuler impacts ni dégâts).

Rapport sous `Saved/ARMEMENT-LOT02/LOT02_13_2_WorldAcceptance.csv`.
Le deuxième test `Grimrock.RPG.ARMEMENT.LOT02.RecurveBowRealArrows`
réutilise `FGridRangedAmmunitionPolicy` et les vrais items ARM048/ARM045
et `Item_Arrows_01` : 0 flèche bloque ; deux consommations font 2→1→0 ;
un troisième tir reste interdit. Ce test ne simule pas une attaque complète.

## Exécuter la validation UE5.5.4

Fermer Unreal Editor :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
Import-Csv .\Saved\ARMEMENT-LOT02\LOT02_13_2_WorldAcceptance.csv |
    Format-Table ArchetypeId,Mesh,Picked,Equipped,Extracted,Redropped,Repicked,Projectile,Status -AutoSize
git status --short
```

**Résultat attendu, non encore vérifié :** 11 lignes
`AUTOMATED_WORLD_PASS_PIE_PENDING`, trois lignes `SPAWNED_NO_IMPACT`,
aucun test LOT02 en échec. Une validation sur monde temporaire n'est PAS
une observation PIE par un joueur.

## Recette manuelle PIE — feuille de contrôle

Préparer dans l'éditeur UE une copie isolée de carte et du `UGridLevelAsset`
avec les objets de la palette `DA_ObjectPalette_Default`. Ne pas enregistrer
des changements sur `L_Dungeon`, les niveaux canoniques ou les sauvegardes
de production. Démarrer le jeu avec une sauvegarde de test indépendante.
Si le mode PIE ne permet pas d'utiliser cette copie, noter `BLOQUÉ`
et ne pas modifier le menu ni les kits historiques pour forcer le scénario.

| Scénario | Vérification observable |
| --- | --- |
| P1 | Mesh au sol, pivot/échelle, PBR, orientation et éclairage |
| P2 | Ramassage souris unique : disparition du monde et sac +1 |
| P3 | Icône 48×48 carrée, nom, poids et tooltip N1 |
| P4 | Équipement/déséquipement, bonne occupation MainHand/OffHand |
| P5 | Dépôt puis reprise sans duplication et sans changement de niveau |
| P6 | Attaque sur monstre réel : PA, dégâts, portée, LOS et refus |
| P7 | SaveGame puis rechargement : GUID, niveau, quantité, état main |

Cas spécifiques à effectuer en plus :

- ARM038 : deux mains, OffHand virtuellement bloquée, déplacement atomique
  des objets déjà équipés, refus si le sac est plein.
- ARM048 : 2 flèches → deux tirs (2→1→0), refus d'un troisième sans dépense
  de PA, refus hors portée sans consommer de flèche.
- ARM054/ARM057/ARM058 : jet depuis main et inventaire, un projectile
  consommé, collision, restitution et ramassage unique.
- ARM061/ARM062/ARM067/ARM069 : action magique directe N1, sans effet spécial
  supplémentaire non implémenté (brûlure, soin, surface).
- ARM070 : focaliseur OffHand, réserve magique/absorption et persistance,
  aucun sort ou attaque implicite.
- ARM081 : bouclier OffHand, armure physique/absorption sans double bonus,
  persistance après sauvegarde.

**Codes par case :** `NT` non testé (valeur initiale de P1 à P7 pour
chacune des 11 armes), `OK` réellement observé en PIE, `KO` avec
reproduction, `BLOQUÉ` impossible dans le parcours de test.
Ne jamais convertir `AUTOMATED_WORLD_PASS_PIE_PENDING` en `OK PIE`.

En cas de KO, relever ARMxxx, étapes, PA/quantités avant-après,
captures, lignes de log et éventuelle sauvegarde de test.

## Garde-fous

Aucun `Planned → Active`, aucune activation des 18 kits, aucune suppression
du ThrowingKnife historique. Les 11 DataAssets, les deux catalogues,
la carte `L_Dungeon`, les anciennes sauvegardes, le XLSX local et
le document Meshy non suivi ne sont pas modifiés par ce commit.
Après retours des tests UE locaux, corriger le premier vrai blocage,
puis faire une passe PIE observée avant activation.