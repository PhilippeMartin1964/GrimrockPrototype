# ARMEMENT-LOT02.5 — Véritables munitions du Rôdeur Tireur

**Code source et procédure publiés : production de packages UE5.5.4 encore à effectuer localement.**

## Contrat fonctionnel

- `DA_Item_Arrows_01` : définition propre `Item_Arrows_01`, `Misc`, empilable `64` unités maximum, poids nominal `0,04 kg` par flèche.
- `DA_Weapon_ShortBow_01` (`ARM045`) : `RequiredAmmunitionItemDefinitionId = Item_Arrows_01`.
- `Kit_Ranger_Marksman` : arc court N1 en MainHand, dague N1 dans le sac, **12 flèches** supplémentaires dans un seul slot du sac, via le service de création transactionnel.
- Les autres 17 recettes approuvées et les **12 anciens kits visibles dans le wizard** restent inchangés.
- `FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition` autorise le fallback de sauvegarde **uniquement pour cet ID exact** ; aucun ID inconnu ne se substitue.
- Le test `Grimrock.RPG.ARMEMENT.LOT02.MarksmanStarterAmmoCreationAndRestore` couvre création, objet N1, deux mains, quantité 12 → 11, propriété de pile, identité du kit, ownership et restauration en mémoire. Il ne constitue pas une validation de combat PIE avec monstre.

**Art en attente :** aucun mesh ni icône de flèche n'a été inventé ou réutilisé abusivement depuis une autre arme. La définition fonctionnelle est utilisable dans le sac, mais la présentation de son icône et de son objet au sol demandera un asset dédié avant clôture artistique.

## Production locale automatique — une commande

Fermer Unreal Editor, synchroniser `master`, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ProduceMarksmanAmmunition.ps1 -EngineRoot D:\UE_5.5 -Apply -Push
```

Le script assure préflight Git des deux cibles, build Development Editor, passage du commandlet en `-Apply`, vérification de ses diagnostics, quatre suites UE via `StarterKitProductionBatch -Action Validate`, puis **commit/push des deux seuls DataAssets**. `L_MainMenu.umap` n'est jamais indexé ni réécrit.

Mode inspection sans mutation : omettre `-Apply -Push`. En cas d'échec de compilation, de sauvegarde ou de test, le script **n'effectue aucun commit**. Une sauvegarde interrompue peut laisser un package partiel à examiner ; aucun nettoyage automatique dangereux n'est effectué.

## Validation attendue

- `Grimrock.RPG.ARMEMENT.LOT02` : quatre tests, zéro échec.
- `Grimrock.RPG.ARMEMENT01.9` : non-régressions historiques (certains avertissements préexistants).
- `Grimrock.RPG.ARMEMENT01.11.1` : profils mêlée.
- `Grimrock.RPG.ARMEMENT.Industrial` : manifest industriel.
- Rapport `Saved/ARMEMENT-LOT02/LOT02_3_All18KitMatrix.csv` : **6 candidats / 12 bloqués**, *seulement après validation des vrais assets*.

Reste hors de cette étape : art des flèches, animation/projectile de flèche spécifique, test PIE de tirs sur un monstre, et activation visible du kit dans le wizard lors de la migration cohérente des 18 kits.
