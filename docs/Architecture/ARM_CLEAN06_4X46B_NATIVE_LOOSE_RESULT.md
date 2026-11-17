# ARM-CLEAN06.4X.46B — Résultat de la première conversion native publiée

**État au 17 novembre 2026 : conversion binaire terminée pour les 44 objets libres de DA_GridLevel_00 ; recette PIE post-migration VALIDÉE par le propriétaire lors de la reprise X.48.**

## Preuves Windows et Git

| Contrôle | Constat vérifié |
|---|---|
| Parent de la transaction | `875814f2d189a244cd84ab5f7bfa2fb521174f87` (test X.47 corrigé) |
| Commit de la transaction | `cd177cd5473c8ab3777bfecae56cd0e33abd23eb` |
| Fichiers changés entre parent et commit | **un seul** : `Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset` |
| Action commandlet UE | `Saved ONE native level: 44 pointers removed` |
| Rechargement UE dans un nouveau processus | `-Verify` terminé avec code 0, 44 entrées pointerless |
| Automation complète sur le niveau converti | **1961 succès sans avertissement, 26 succès avec avertissements, 0 échec, 0 ignoré** (1987 cas au total) |
| Sortie `ValidateUE.ps1` | Code 0, `[OK] Automation filter validated.` |
| `UE5.5.4/SilentTask` pour le SHA binaire | **SUCCESS** |
| `UE5.5.4/LocalAuto` pour le SHA binaire | **SUCCESS** |
| Publication | `875814f2..cd177cd5 HEAD -> master` (un seul commit binaire) |

Les vérifications GitHub de HEAD, du parent et du diff ont été faites après réception
du journal local. Le fichier de sauvegarde a été créé hors dépôt dans le répertoire
`D:\Development\GrimrockPrototype_Audits\ARM_CLEAN06_4X46`, par le script protégé.
Ne pas supprimer les sauvegardes privées tant que la recette PIE et les
sauvegardes de partie ne sont pas validées.

## Signification de la conversion

Le package réel `DA_GridLevel_00` a été resauvegardé dans UE5.5.4
après avoir effacé le champ sérialisé `FGridLooseItemInstance::ItemDefinition`
des **44 objets libres** qui disposent déjà de `CanonicalItemId` et
`ItemLevel`. Le registre `DA_ItemSystem` est désormais l'autorité
de ces instances, sans réintroduire de source `DA_Weapon_*`.

L'outil a validé les identités et niveaux physiques réels avant
l'écriture, puis a contrôlé la réouverture et la suite des tests.
Les **17 contenus initiaux de réceptacles**, les **3 overrides inactifs**
du même package, les **33 entrées de palette**, les serrures, les loots
de monstres, les autres niveaux et les anciens SaveGames v27
**n'ont pas été migrés par cette opération**.

Le dernier audit complet préalable signalait 107 pointeurs historiques
dans neuf assets. Cette transaction en retire 44 ; le solde théorique
est donc 63 références, **à confirmer par un nouveau scan UE natif**,
pas à présenter comme un inventaire vérifié après conversion.

## Recette PIE validée par le propriétaire

L'Automation `-NullRHI` n'est pas une preuve de qualité visuelle,
de rendu Niagara ni de comportement Chaos en PIE. Le propriétaire a confirmé
« Fonctionnement en PIE validé » après le commit binaire X.46B.
Les vérifications suivantes constituaient la grille de recette de cette validation :

1. Apparition / orientation / maillage d'armes représentatives N1 et N20,
   des non-ARM N0, éclairage des objets et effets Niagara.
2. Ramassage, inventaire, rééquipement et dépôt au sol, avec identité et
   niveau inchangés.
3. Fosse PIT02 ouverte pendant le passage d'objets et armes :
   réception dans **la bonne cellule du niveau inférieur**, absence
   d'apparition sur une cellule voisine.
4. Sauvegarde et `Continue` d'une partie existante, contrôle des
   objets ramassés/déposés avant et après relancement du processus.
5. Aucune régression de réceptacle, serrure, palette ou autre niveau
   non migré.

**Important :** le signalement antérieur de transition d'armes
dans une fosse vers la mauvaise cellule reste un sujet à inspecter
séparément. Le succès de la migration des pointeurs ne prouve pas sa
correction.

## Prochain lot de migration

Préparer un **nouvel audit natif de dépendances après X.46B**.
Les prochaines conversions potentielles (réceptacles, palette, serrures,
loot de monstres, SaveGames) doivent rester des transactions
indépendantes avec preuves d'identité, sauvegarde, diff fermé,
réouverture et validations. Ne jamais réexécuter l'ancien
`-Apply -Publish` sur un SHA historique comme si le niveau était
encore dans l'état initial : une nouvelle exécution du commandlet
en état pointerless est destinée à être un no-op, pas une nouvelle
migration.

**Statut global : ARM-CLEAN06.4 PARTIELLE ; ARM-CLEAN06.5 BLOQUÉE**.
Seul le sous-lot des 44 objets libres est converti et validé
techniquement ; sa recette PIE reste à accomplir.
