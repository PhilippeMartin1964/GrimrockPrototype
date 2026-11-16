# ARM-CLEAN06.4U — QuickItems non-ARM : vrais assets et vrais profils d'action

## Point de départ vérifié

- Parent : ARM-CLEAN06.4T, fcf13df12f31e5da4018fc10d2b621d84212b33e, trois SUCCESS Windows, issue #153.
- Vrai DA_ItemSystem.uasset : 29 définitions (23 ARM/munitions et 6 non-ARM).
- Rapport privé 06.4K : 11 identités, 40 références historiques ; rapport 06.4O : trois QuickItems et une action équipée.
- L'ordre du propriétaire est de migrer des fichiers Unreal réels. Aucun nouveau fallback legacy n'est permis dans le runtime.

## Runtime : une seule source des actions

UGridItemSubsystem::ResolveDetailedNonArmAction résout ItemId, niveau et ActionId par le seul DA_ItemSystem et son seul DA_ActionCatalogue. Il exige une liaison explicite ActionId/PresentationId, refuse les collisions avec les actions compactes et renvoie le FGridCombatActionDefinition complet, sans le fabriquer depuis le legacy. FGridCanonicalQuickItemUseReadModel utilise ce profil intégral pour l'inventaire, le HUD, les raccourcis et le TurnManager ; coûts, effets, ciblage, cooldowns et actions personnalisées demeurent authorés dans le catalogue unique. Les actions compactes déjà canoniques restent prises en charge, sans lecture d'un ancien DataAsset pour une identité enregistrée.

## Migration Editor réelle

Le commandlet GridArmClean064UNonArmQuickMaterialize ouvre les 11 anciens DataAssets de la preuve privée 06.4K, confirme les six déjà convertis, et projette uniquement les trois propriétaires QuickItem non équipés réellement compatibles. Deux autres identités restent réservées au futur raccordement offensif/équipement.

- BuildQuickItemCombatActionDefinition normalise les trois profils depuis les sources réelles ; les structures complètes sont copiées telles quelles dans DA_ActionCatalogue.DetailedActions.
- Chaque objet reçoit exactement une liaison d'action, UseActions si consommable et ActionBindings sinon, sans mécanismes dupliqués sur l'objet.
- L'identité, les visuels, piles, lumières, lecture, Chaos et lancer proviennent des mêmes opérations de copie vérifiées que 06.4T.
- Une présentation explicite non résolue bloque la conversion ; si aucune présentation n'est authorée, seul le profil neutre existant PRES_AttackNoFX est lié. Aucun son ou Niagara nouveau n'est fabriqué.
- -Apply crée trois vrais DataAssets sous CanonicalStage/Definitions et sauve DA_ItemSystem.uasset et DA_ActionCatalogue.uasset via UPackage::SavePackage.
- -Verify rouvre les vrais binaires dans un second processus Unreal, compare FGridCombatActionDefinition avec CompareScriptStruct, toutes ses propriétés, les bindings et les valeurs UPROPERTY de chaque objet.

## Validation et Git atomique

Sujet unique : feat(ARM-CLEAN06.4U): migrate authored nonarm QuickItems into real action catalogue

L'agent Windows compile UE5.5.4 et lance Grimrock. Le script ApplyARM_CLEAN06_4U_NonArmQuickMigration.ps1 exerce la conversion et refuse tout fichier modifié hors de cinq assets exactement (trois nouvelles définitions, le registre et le catalogue d'actions). Les cinq .uasset doivent être gérés par Git LFS. Il relance Unreal -Verify et la suite Grimrock sur les binaires sauvegardés, puis amende exclusivement son propre commit 06.4U avec ces binaires avant le push --force-with-lease ; parent exact : 06.4T. Ce n'est pas un second commit de migration.

Le test Grimrock.RPG.ARM.CLEAN06_4.RealDetailedNonArmQuickItems vérifie la résolution des trois actions originales, N0, le rejet N21, l'unicité des propriétaires et le refus des identités non liées. Avant matérialisation, la présence de zéro profil détaillé est admise uniquement pour la compilation ; le commit final doit en posséder trois.

Un jalon est déclaré clos UNIQUEMENT si les trois checks SilentTask, LocalAuto et ARM-CLEAN06.4U sont SUCCESS sur le SHA final contenant les cinq .uasset.

## Suite non masquée

Restent deux identités non-ARM avec action/équipement, et 40 références historiques de cellules/palettes/réceptacles, puis la vérification des SaveGames, PIE/Chaos, Cook/Shipping et le retrait des anciens adaptateurs en 06.5. Aucune conversion des niveaux ou des SaveGames n'est revendiquée ici. Pas de changement de texte WBP ; UI.csv -> ST_UI -> WBP -> Game/fr/en inchangé.