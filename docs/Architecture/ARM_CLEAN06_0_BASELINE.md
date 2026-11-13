# ARM-CLEAN06.0 — Baseline de migration UE, sans modification d'assets

**Sources normatives :** [architecture](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md),
[plan de migration](ARM_CLEAN06_MIGRATION_PLAN.md) et [AGENTS.md](../../AGENTS.md).

## Objet de cette étape

Capturer les valeurs **réellement sérialisées** dans Unreal avant tout changement de
UPROPERTY, de références, de statistiques de combat ou de sauvegardes.
L'outil n'applique **aucune migration** et ne réécrit aucun asset.
Les diagnostics ARM-CLEAN05 et les données du classeur ne remplacent pas
cette preuve binaire.

## Périmètre et fichiers de sortie

Le commandlet Editor-only GridArmClean06Baseline récupère les classes Unreal,
les UPROPERTY persistantes exportées avec leur représentation textuelle UE,
les dépendances et les référents de l'AssetRegistry :

- 24 définitions d'armes, dont les deux ThrowingKnife ;
- 24 autres définitions d'items, dont munitions, consommables et torche ;
- 2 catalogues ARM : statistiques et visuels ;
- 4 DA_GridLevel, 1 DA_Dungeon, 1 palette d'objets ;
- 1 640 profils ARM001–ARM082 aux niveaux 1–20, y compris les 260 profils passifs.

Les objets placés sont capturés dans les UPROPERTY des niveaux et de la palette.
Le graphe des référents couvre également les WBP qui référencent les assets
sélectionnés, sans modifier les WBP eux-mêmes.

Dans le répertoire privé de la tâche de validation Windows,
Runs/<SHA>-<date>/ARM-CLEAN06.0, l'agent enregistre :

1. asset-baseline.json : valeurs UE des propriétés, 1 640 profils développés,
   références directes et inverses par package ;
2. asset-hashes.json : SHA-256 et taille des 56 vrais fichiers .uasset, ainsi
   que nom/taille/SHA-256 des .sav éventuellement présents dans le checkout
   principal, **sans ouvrir les sauvegardes ni transmettre leur contenu** ;
3. github-issue.md : rapport public restreint aux nombres et identités des
   armes, sans texte métier intégral ni donnée de sauvegarde.

Les fichiers Saved et le répertoire d'état de l'agent ne sont pas commités.
Une éventuelle absence de fichiers de sauvegarde est mentionnée explicitement
et ne constitue **pas** une preuve de compatibilité SaveGame.

## Exécution autonome dans VALIDATION-AUTO01

Le commit exact avec le sujet
chore(ARM-CLEAN06.0): capture read-only UE asset migration baseline
active automatiquement l'extraction **une seule fois** après la compilation
UE 5.5.4 et la suite complète Automation Grimrock.

Le statut GitHub **UE5.5.4/ARM-CLEAN06.0** est distinct de
**UE5.5.4/SilentTask** et **UE5.5.4/LocalAuto**.
Une extraction réussie produit une issue publique
[ARM-CLEAN06.0] sur le SHA de départ, un manifeste privé vérifiable et
un statut success. Un statut absent ou failure signifie **baseline non validée**.

## Conditions de sortie avant ARM-CLEAN06.1

- 56 fichiers de données effectifs chargés depuis UE, et non de simples
  pointeurs Git LFS ;
- 1 640 profils valides et 260 profils passifs reconstitués exactement depuis
  le catalogue compressé, préservant dés, bonus, coûts, protection et portée ;
- graphe de dépendances et référents par package disponible ;
- empreinte SHA-256 de chaque fichier et du manifeste complet ;
- worktree inchangé et absence de sauvegarde d'asset ;
- issue de résultats et statut GitHub UE5.5.4/ARM-CLEAN06.0=success.

**Limites :** l'export reflète les propriétés du schéma Unreal et leurs
valeurs au chargement ; il ne décode ni les blobs de sauvegarde, ni les données
non-UPROPERTY. Les anomalies éventuelles d'ID ou de référence constituent des
observations à résoudre avant migration, jamais une autorisation de suppression.

**PIE :** aucun gameplay ni aspect visuel n'est modifié pendant ARM-CLEAN06.0.
Le contrôle PIE redevient requis lors des modifications runtime des étapes
suivantes. La capture n'est « terminée » qu'après le troisième statut success.
