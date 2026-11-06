# EFFECTS-ORG01.2B — Renommage Unreal natif des bulles de potion

## Objectif

Renommer exclusivement le matériau `M_Teleporter_Spark_02` en
`M_Potion_Spark` **dans le même dossier Effects/Alchimie**, selon
la fonction réelle confirmée par l'auteur et le graphe Asset Registry
[issue #50](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/50).

Chaîne canonique à préserver :
`DA_Item_Potion_Healing_01 → NS_LiquideSparkle → M_Potion_Spark`.

Aucun effet du téléporteur n'est déplacé ou renommé. Aucune
compatibilité arrière n'est ajoutée.

## Automatisation en deux commits

1. Le commit d'outillage EFFECTS-ORG01.2B ajoute le commandlet
   `GridEffectsOrgPotionRename`, son script de sécurité
   `Scripts/MigrateEffectsOrgPotion.ps1` et son hook limité dans
   `UEValidationAgent.ps1`. En l'absence de `-Apply`, le commandlet
   est strictement un audit en lecture seule.
2. L'agent Windows attend la **réussite complète** du build et des tests
   pour ce commit, puis démarre le commandlet sous UE5.5.4 dans son
   **worktree isolé**. Le script vérifie que `master` n'a pas bougé,
   que Git LFS a matérialisé les assets, qu'il n'existe aucun fichier
   modifié, et conserve une sauvegarde SHA-256 des deux sources.
3. Le commandlet vérifie qu'un seul référent Unreal utilise le matériau
   (NS_LiquideSparkle), et que la potion référence toujours ce système.
   Il utilise `IAssetTools::RenameAssets` pour renommer, corrige
   uniquement les redirectors du paquet renommé, et sauvegarde
   matériel/consommateur avec l'API Unreal.
4. Le script exige exactement **trois changements** :
   - disparition de `M_Teleporter_Spark_02.uasset` ;
   - apparition de `M_Potion_Spark.uasset` binaire natif ;
   - resauvegarde de `NS_LiquideSparkle.uasset`.
   Si d'autres fichiers sont touchés, **aucun push** n'a lieu.
5. Sur réussite, le script crée un second commit depuis le worktree
   isolé et pousse en fast-forward sur `master`, sans `force`.
   Le cycle suivant de l'agent recompilera et **revalidera toute la
   suite Grimrock** sur ce nouveau commit contenant les assets.
6. Si la migration échoue sans commit, le script récupère seulement
   les trois chemins connus, après sauvegarde privée. Il refuse tout
   nettoyage global d'un worktree contenant des changements inattendus.
   Les journaux restent localement dans le répertoire de l'agent.

L'état est publié séparément sur le commit de démarrage :

- `UE5.5.4/LocalAuto` : compilation + suite UE de l'outillage.
- `UE5.5.4/EffectsOrgMigration` : résultat de la migration de l'asset.
- Le commit d'asset produit ensuite son **propre** `LocalAuto`.

**Aucune migration n'est déclarée terminée tant que ce dernier
`LocalAuto` n'est pas `success`.**

Le code et le script sont limités à la potion de soin et sont **à
usage ponctuel**. Après migration validée, le test de contrat doit être
resserré sur le seul nom `M_Potion_Spark` et l'outillage de migration
devenu inutile doit être **supprimé**, sans alias de compatibilité.

## Protection des dossiers privés

Les dossiers `Content/FireFX/` et
`Content/GrimrockPrototype/Effects/Fire/Local/` sont exclus de Git.
Cette migration ne les consulte ni ne les écrit.

## Contrôle visuel

La suite -NullRHI vérifie la sérialisation et les liens Niagara, pas
les bulles animées. Le comportement visuel de `NS_LiquideSparkle`
dans le jeu doit être vérifié en PIE après la migration binaire.
