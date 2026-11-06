# EFFECTS-ORG01.2B — Renommage des bulles de potion : bilan final

## Résultat acquis

Le commit natif Unreal [325b72aa](https://github.com/PhilippeMartin1964/GrimrockPrototype/commit/325b72aac43d536ef479d46bd1ff772589aa70fc)
a été publié depuis le worktree Windows contrôlé. Le commit touche
**exactement trois assets du projet** :

- suppression de `Effects/Alchimie/M_Teleporter_Spark_02.uasset` ;
- création de `Effects/Alchimie/M_Potion_Spark.uasset` ;
- resauvegarde de `Effects/Alchimie/NS_LiquideSparkle.uasset`.

Le nouveau matériau reste dans **Alchimie** : il représente les bulles
de la potion de soin, et non le téléporteur. Aucun asset FireFX ni
effet de téléportation n'a été déplacé.

La validation UE5.5.4 complète du commit d'assets **325b72aa** porte
le statut GitHub `UE5.5.4/LocalAuto = success`. Le contrat de
dépendances vérifie la chaîne sérialisée :

`DA_Item_Potion_Healing_01 → NS_LiquideSparkle → M_Potion_Spark`.

## Nettoyage post-migration : EFFECTS-ORG01.2C

L'outil de migration est à usage unique. Après la validation du nouvel
asset, les sources suivantes n'ont plus de rôle :

- `Scripts/MigrateEffectsOrgPotion.ps1` ;
- `GridEffectsOrgPotionRenameCommandlet.cpp/.h` ;
- le déclencheur `UE5.5.4/EffectsOrgMigration` dans
  `UEValidationAgent.ps1`.

Elles sont supprimées par EFFECTS-ORG01.2C, et les tests de
`Grimrock.Editor.EffectsOrg01` exigent désormais **uniquement**
`M_Potion_Spark`. Aucun alias, fallback ni redirector ancien n'est
conservé. Les anciens statuts de migration en échec proviennent
notamment de tentatives répétées **après** la publication du
renommage ; le succès du commit binaire et son contenu Git sont les
preuves à retenir.

## Limite des tests

L'audit Asset Registry et la validation automatisée -NullRHI vérifient
les références, la classe et le chargement de ressources ; ils ne
démontrent pas la qualité de l'animation de bulles en rendu temps réel.
Un contrôle PIE restera nécessaire pour cette seule propriété visuelle,
sans bloquer le nettoyage architectural ni provoquer de nouvelles
modifications d'assets.

## Périmètre du jalon suivant

La migration de la torche `NS_Flame_8_Torch` vers
`Effects/Fire/Torch` sera réalisée dans un jalon distinct, avec
prise en compte de la définition d'item `DA_Item_Torch` et de ses
matériaux `MI_Distortion_1` et `MI_Flame_8`. Les
matériaux de l'huile ne seront pas déplacés tant que
le générateur/validateur d'assets n'est pas adapté.
