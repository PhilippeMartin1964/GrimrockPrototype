# ARM-CLEAN06.5H.69 — Résolveurs visuels historiques réservés à Automation

## Changement

Les deux services hérités `FGridWeaponVisualResolver` et `FGridWorldItemVisualResolver` lisaient encore `UGridItemDefinitionAsset`, les anciens catalogues visuels ou les meshes historiques. Après le cutover des pickups vers `InitializeFromCanonicalItem`, leur unique chemin acteur restant, `AGridItemActor::InitializeFromItemDefinition`, est déjà limité à `WITH_DEV_AUTOMATION_TESTS`. Le pont UI historique est lui aussi enfermé dans les fixtures de H.68.

Ce jalon enferme **les déclarations et toutes les implémentations** des deux résolveurs sous `WITH_DEV_AUTOMATION_TESTS`. Le code de repli de présentation ne se compile donc plus dans `GrimrockPrototype Win64 Shipping`. L'include de l'acteur est conditionné au même macro ; l'include devenu inutile de `GridEditorPreviewComponent.cpp` est retiré.

Les contrôles historiques d'authoring et de présentation dans les tests `ARMEMENT01.6` demeurent exécutables en Development Editor : ils continuent à vérifier les assets d'origine tant que la migration native n'est pas clôturée. Le remplacement des anciens tests par des preuves persistantes viendra avant la suppression physique des définitions.

## Invariants de non-régression

- Le runtime des objets enregistrés conserve `UGridItemSubsystem`, `FGridResolvedItem`, les meshes, transforms et icônes canoniques.
- Pas de nouvelle copie de stats, de catalogue, d'icône ou de niveau dans une instance physique.
- Aucun comportement combat/Chaos/SFX/VFX, donnée Unreal, texture, WBP, sauvegarde ou niveau modifié.
- Aucun texte UI ajouté ou remplacé : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé. Pas de matérialisation de String Table liée à ce lot.
- Aucune suppression de l'un des 53 packages historiques H.23, ni lecture ou écriture des sept sauvegardes privées.

H.70 reste bloqué jusqu'aux deux statuts Windows `UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS` sur le SHA H.69, avec compilation Editor/Shipping et tests `Grimrock.RPG.ARMEMENT01.6`. Le contrôle du rendu en PIE et Shipping GPU reste indépendant de ces validations.
