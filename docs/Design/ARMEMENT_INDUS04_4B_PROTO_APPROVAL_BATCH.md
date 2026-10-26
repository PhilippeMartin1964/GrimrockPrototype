# ARMEMENT-INDUS04.4B — Approbation narrative globale pour le prototype

**Décision utilisateur, 26 octobre 2026 :** l'ensemble des 82 descriptions narratives proposées est considéré **APPROVED**, à titre **provisoire pour le prototype**, pour accélérer l'intégration. Ce statut ne certifie ni un lore final, ni l'implémentation des mécaniques marquées `PLANNED`. Les règles de combat et les niveaux N1–N20 restent inchangés.

**Implémentation :** `Scripts/WeaponNarrativePrototypeBatch.ps1`. Aucun changement de C++, UMG, StringTable, DataAsset ou XLSX n'est réalisé par ce commit : la mise à jour du classeur et des `.uasset` est effectuée **localement**, à la demande explicite de l'utilisateur.

## Couverture réelle et limite de la production

- **82/82** archétypes `ARM001..ARM082` présents dans le XLSX et dans `NarrativeV1` doivent être approuvés dans la feuille `Narratif`.
- Le manifeste actuel `Config/Armement/WeaponProductionManifest.csv` ne contient que **9 entrées** : **8 Active** (ARM001, ARM002, ARM014, ARM015, ARM025, ARM031, ARM045, ARM075), **1 Planned** (ARM003). Les 73 autres ne figurent pas encore dans le manifeste.
- Le batch publie les descriptions **uniquement pour les 8 entrées Active ayant déjà un `DA_Weapon_*` sur disque**. Il n'invente pas des DataAssets pour les archétypes prévus.
- **ARM031 (Grande Hache)** a déjà été publié localement lors de la recette utilisateur : `[INDUS04.4] SAVED NarrativeItem for ARM031; SavedPackages=1`. Son `.uasset` est actuellement **modifié localement**. Le batch le marque `SKIPPED_DIRTY` et **ne l'écrase pas**.

## Exécution locale : un seul workflow groupé

Fermer Unreal Editor et Excel. Les fichiers binaires locaux déjà modifiés ne doivent pas être restaurés.

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

# Approuve les 82 descriptions EN BLOC dans l'onglet Narratif.
.\Scripts\WeaponNarrativePrototypeBatch.ps1 -EngineRoot D:\UE_5.5 -ApproveAll

# Publie les descriptions des assets actifs non modifiés, sans toucher à ARM031.
.\Scripts\WeaponNarrativePrototypeBatch.ps1 -EngineRoot D:\UE_5.5 -Apply

git status --short
```

**Option directe :** `-ApproveAll -Apply` peut être exécuté en une commande, mais les deux appels ci-dessus laissent examiner le changement XLSX avant import.

### Règles de sécurité

1. `-ApproveAll` utilise l'installation locale d'Excel (COM). Elle exige une feuille `Narratif` avec `A1=ArchetypeId`, `G1=EtatValidation`, **82 IDs canoniques uniques**, chaque description courte non vide et un état DRAFT/NEEDS_REVIEW/APPROVED. **Avant la sauvegarde**, le classeur original est copié dans `Saved/ARMEMENT-INDUS/Backups/`, avec horodatage. Seules les cellules `G2:G83` sont demandées en écriture ; Excel enregistre le classeur. **Cette modification XLSX est intentionnelle** et reste dans le working tree jusqu'au commit manuel.
2. Si Excel COM n'est pas disponible, mettre manuellement `APPROVED` dans `G2:G83` (sélectionner la plage, saisir `APPROVED`, valider avec **Ctrl+Entrée**), enregistrer puis exécuter seulement `-Apply`.
3. Le batch régénère obligatoirement le CSV `NarrativeV1` **depuis le XLSX**, jamais depuis le CSV historique de propositions DRAFT. `-Apply` exige **82/82 APPROVED** avant de modifier un asset.
4. Seuls les DataAssets **Active**, déjà présents et **non modifiés localement** sont candidats à l'import. Un asset `SKIPPED_DIRTY` est préservé, même s'il contient déjà le texte désiré.
5. Le batch réutilise, pour chaque candidat, `WeaponNarrativeApply.ps1 -Apply` et le commandlet `NarrativeItem` **déjà validés**. Ce contrôle refuse l'écrasement d'une description existante différente (`CONFLICT`), la mauvaise identité d'objet, un statut non APPROVED, un chemin invalide ou un package initialement dirty.
6. L'exécution est séquentielle pour rester sur l'infrastructure actuelle, **un package maximum par opération**, et conserve les logs individuels. Une erreur d'un item produit `FAILED`, et le batch signale l'échec à la fin ; aucun rollback magique d'éventuels items déjà traités n'est annoncé.
7. Aucun `git add`, `commit`, `push` ou `reset` automatique. **Ne pas exécuter `git add -A`** ; conserver `T_Icon_PA.uasset`, les deux widgets d'info-bulle, `ST_UI.uasset`, l'XLSX et l'asset Grande Hache déjà modifiés.

### Rapport

`Saved/ARMEMENT-INDUS/INDUS04_4B_PrototypeBatch.csv` donne par archétype : `SAVED`, `PRESENT_SAME`, `SKIPPED_DIRTY`, `SKIPPED_NOT_CREATED`, `SKIPPED_REVIEW` ou `FAILED`. Le récapitulatif console compte les descriptions approuvées et les assets réellement traités.

Attendu de principe avec le working tree connu : `Approved=82/82`, `Active=8`, `PreservedDirty>=1` (ARM031), et les autres entrées actives : `SAVED` ou `PRESENT_SAME`, sauf éventuels `CONFLICT` à traiter séparément. Le **nombre exact** d'assets disponibles et publiables n'est établi qu'après exécution locale. Ne pas annoncer 82 DataAssets modifiés.

## Vérifications locales après import

- Vérifier `DA_Weapon_GreatAxe_01::Description` et un ou deux nouveaux DataAssets importés (Dague ARM001, Bouclier rond ARM075).
- En PIE, le tooltip affiche un paragraphe court dans `Text_Description` et ne déplace aucune ligne des statistiques/comparaisons.
- Vérifier qu'aucune particularité narrative `PLANNED` n'apparaît comme capacité exécutable.
- Régression : `Grimrock.UI.Item01` et `Grimrock.UI.WeaponTooltip04` (respectivement 5/5 et 9/9 confirmés par l'utilisateur le 26 octobre avant l'import groupé).
- Enregistrer et publier uniquement le XLSX et les DataAssets destinés à ce jalon, après contrôle PIE. Les quatre assets UI locaux restent isolés.

## Prochain pas

Les 82 descriptions éditorialement `APPROVED` sont prêtes pour les futurs archétypes ; elles seront injectées au fur et à mesure de la **création de leurs DataAssets**. Aucun changement de machine à états, de combat, de lore long, ni de graphique UMG n'est nécessaire pour les afficher.
