# ARM-CLEAN06 — Point de reprise fiable (17 novembre 2026)

> **Document de continuité, pas une autorisation de conversion.**
> Lors de toute reprise, **relire le HEAD réel de `origin/master` et ses statuts**
> avant d'utiliser ce point daté. Si le dépôt a avancé, ne jamais revenir au
> jalon présenté ici. Une conversation ChatGPT peut s'interrompre sans préavis.

## ARM-CLEAN06.5G.12 — Loot monstre sans pointeur historique

Parent ea4f755f9bfb2fb0cd8606f2e5e0a95710d43e16 : publication conditionnée à ses deux statuts SUCCESS. Suppression effective de FGridMonsterLootEntry.ItemDefinitionAsset et des chemins de résolution legacy de loot. Tests MON8/MON9/MON17.6/ARMEMENT01.8 et preuves X.48/X.60/X.61/TD07 alignés sur ItemId + N ; aucun asset, SaveGame ou WBP changé. Attendre les deux statuts Windows sur le nouveau SHA. Voir [5G.12](ARM_CLEAN06_5G12_CANONICAL_MONSTER_LOOT_SCHEMA.md).

## ARM-CLEAN06.5G.11 — Retrait des points d'entrée d'audit historiques

Parent 3f44454cc9d92fec9afad091a7132554bc3d636d : publication conditionnée aux deux statuts SUCCESS. Retrait de 13 commandlets d'audit/projection et 12 scripts (38 fichiers). Les outils Baseline, SaveCopy, RetirementAudit, preuves, helpers, fixtures et protections de rollback restent présents. Aucun changement runtime, asset ou SaveGame. Attendre les deux statuts du SHA exact. Voir [5G.11](ARM_CLEAN06_5G11_RETIRE_HISTORICAL_PROJECTION_ENTRYPOINTS.md).

## ARM-CLEAN06.5G.10 — Retrait des points d'entrée natifs de conversion

Parent 2683d5d602299e5f8d9124303aeb061d360e3f02 : publication conditionnée aux deux statuts SUCCESS. Suppression de 17 scripts ApplyARM_CLEAN06 et 17 commandlets associés (34 fichiers C++). Helpers de parité, tests, audits et sauvegardes/rollback conservés. Aucun asset ou SaveGame modifié. Attendre les deux statuts du SHA publié. Suite : dénouer les dépendances des audits historiques puis du schéma et des fixtures legacy avant 06.6. Voir [5G.10](ARM_CLEAN06_5G10_RETIRE_NATIVE_MIGRATION_ENTRYPOINTS.md).

## ARM-CLEAN06.5G.9 — Retrait des déclencheurs de migration terminés

Parent b35f8ce024bd54407d8eb6d6d417642c33ac0131 : publication de ce lot conditionnée aux deux statuts SUCCESS sur le parent. Les anciens hooks exact-SHA ARM sont retirés de l'agent et de ValidateUE ; les contrôles généraux, les trois auto-tests des sauvegardes/rollback et les récupérations non-ARM restent présents. Aucune suppression de packages, SaveGame, reçus ou fichiers de sauvegarde. Attendre les deux statuts Windows du présent SHA avant le retrait par dépendance des outils historiques. Voir ARM_CLEAN06_5G9_RETIRE_MIGRATION_HOOKS.md.

## ARM-CLEAN06.5G.8 — Contrat physique unique

Parent b7ba5f7ee906e58aa55f6094b300ee7b2e2da879 : SilentTask et LocalAuto SUCCESS. ResolveItem refuse ARM N0 et non-ARM N1–N20 avant toute projection. Tests historiques alignés, 542 rejets natifs supplémentaires, commentaires obsolètes corrigés. Aucun asset, SaveGame, texte joueur ou WBP modifié. Validation du présent SHA obligatoire ; les contrôles physiques périphériques sont conservés. Suite 5G.9 : draft 65f1a4a71ddbe5295a0888f634319ad4565c8980, rebaser uniquement ses fichiers après les deux statuts du HEAD. Le retrait des classes et assets historiques reste ouvert avant 06.6.

## ARM-CLEAN06.5G.7 — Authoring et aperçu des kits alignés

Parent 6762daf3f083e982e63e847f263face2002371b4 : SilentTask et LocalAuto SUCCESS. Le sélecteur d'objets et l'aperçu des kits utilisent la même identité canonique et le même niveau explicite que les constructeurs. Aucun changement d'asset ou de sauvegarde.

Draft suivant 5G.8 : `8d88ffbadf9e33f1168a754b25ba1c129ddc8174` (parent `5d36b39db2913a7465e2e3e48d84ee97a6cc1c73`, 10 fichiers). Draft 5G.9 : `65f1a4a71ddbe5295a0888f634319ad4565c8980` (parent 6762daf3f083e982e63e847f263face2002371b4, 3 fichiers), retrait des hooks ARM ponctuels de l'agent, sans retirer les contrôles ordinaires ni les outils de sauvegarde/rollback. Rebaser les diffs après validation du parent effectif.

## ARM-CLEAN06.5G.6 — Niveaux physiques stricts monde et loot

Parent 5d36b39db2913a7465e2e3e48d84ee97a6cc1c73 : deux statuts SUCCESS. Suppression de l'héritage du niveau historique pour IDs inscrits dans les placements, réceptacles, preview et loot. Niveaux explicites conservés ; aucun changement de RNG, asset ou SaveGame. Draft 5G.7 : b60ee136d8576beee7db2321d9ac9b29d87e7cf8 (parent 5d6ad973bbcd0df73edd1bf9c8ec4c8554584457, dix fichiers). Draft 5G.8 : 8d88ffbadf9e33f1168a754b25ba1c129ddc8174 (parent 5d36b39db2913a7465e2e3e48d84ee97a6cc1c73, dix fichiers) verrou central ResolveItem N0/N1–N20 et 542 cas de refus. Rebaser uniquement les fichiers de chaque draft après validation du HEAD. 5G.8 exige audit complémentaire des appels historiques du résolveur avant publication.

## ARM-CLEAN06.5G.5 — Niveaux physiques stricts inventaire

Parent 5d6ad973bbcd0df73edd1bf9c8ec4c8554584457 : deux statuts SUCCESS. Matérialisation d'instances et stack policy refusent N0 ARM/N1 non-ARM sans emprunter un niveau historique ; tests 3AG/3AS mis à jour. Aucun asset ou SaveGame modifié. Draft suivant 5G.6 : 02f3cdbcf28d5f01dbfdee732d78016ab1ecdcae (parent 5d6ad973bbcd0df73edd1bf9c8ec4c8554584457, neuf fichiers) strict monde/loot. Draft 5G.7 : b60ee136d8576beee7db2321d9ac9b29d87e7cf8 (même parent, dix fichiers) authoring/preview kits. Ajouter handoff et rebaser uniquement les différences sur chaque HEAD validé.

## ARM-CLEAN06.5G.4 — Registre des kits et inventaire

Parent fa5a76ee8f34aa420ba0ac88f3ff0e2e97c57fa3 : deux statuts SUCCESS. Les ID inscrits ne peuvent plus alimenter l'ancienne map de définitions et les kits exigent CanonicalItemId et niveau explicite ; les anciens pointeurs compatibles ne sont plus retournés. Les fixtures réellement hors registre restent provisoirement compatibles. Tests 3AM/3AO/4X43/3AT/5E4D mis à jour. Aucun asset ou SaveGame modifié. Prochain lot 5G.5 : draft 1e6fc752536a3b3c4843dcdc57c1448b31531329 (parent fa5a76ee8f34aa420ba0ac88f3ff0e2e97c57fa3, cinq fichiers) à rebaser sur HEAD validé.

## ARM-CLEAN06.5G.3 — Retrait des producteurs historiques

Parent 9c4d4ac734cee0ae680bec727f4d551be5d5c0a0 : deux statuts SUCCESS. Suppression des cinq commandlets et huit scripts capables de recréer ou de modifier l'ancien armement, conservation des helpers de parité et des manifests de référence. Test industriel adapté au contrat d'absence des producteurs historiques ; authoring canonique manuel documenté. Aucun changement binaire ou SaveGame. Prochain lot 5G.4 : draft d08e480769d659d37bb222851d48c4f939bcbc4c (parent 4d12154b71e651082193e77517ddc5a00965c9ca, huit différences seulement) à rebaser sur le HEAD validé.

## ARM-CLEAN06.5G.2 — Details des armes canoniques

Parent 4d12154b71e651082193e77517ddc5a00965c9ca : deux statuts SUCCESS, zéro arête externe. Remplacement de la personnalisation historique par celle de UGridArmamentDefinitionAsset : aperçu N1/N20 depuis le DA_ItemSystem configuré, refus d'un doublon hors index, navigation vers le registre et ses quatre catalogues via soft paths, aucune copie ni écriture. Test 22 définitions réelles. Contrôle visuel Details restant. Draft suivant 5G.3 : 9f3bad580a04c90ca777a80fc583e6502b06f543 (21 fichiers, ajouter handoff). Draft 5G.4 : d08e480769d659d37bb222851d48c4f939bcbc4c (8 fichiers, ajouter handoff). Attendre les statuts exacts de chaque lot.

## ARM-CLEAN06.5G.1 — retrait du schéma et du second classificateur d'icônes

5F.1 e1790a0b531011eebf9a72a1e99b8b24ab508d76 : trois statuts SUCCESS. Enfant binaire 2dfdd7fbc93203c5e6e6865b12f3a52880ae0f7c : deux statuts SUCCESS, seul WBP_GridCombatHudAction modifié. Publication désormais autorisée. Retrait de la propriété historique et de son helper de classification : les 82 archétypes et les cas négatifs passent par FGridCanonicalActionIconReadModel (LOT02.20 conservé). L'énumération stable est indépendante de l'ancienne classe de palette conservée uniquement pour les projections historiques. Gate permanent zéro arête externe et chargement du WBP natif, retrait des quatre outils one-shot et de leur hook. Aucun changement de texte, asset ou SaveGame. Voir [5G.1](ARM_CLEAN06_5G1_HUD_PALETTE_SCHEMA_RETIREMENT.md).

## ARM-CLEAN06.5F.1 — correctif de liaison

fe491a4e902b34195c483ef8ae4ba69f7748f00a en échec BUILD_FAILED (issue #201, LNK2019/LNK2001/LNK1120). Aucun symbole précis publié ; les méthodes directes UMG MinimalAPI et K2 sont remplacées par la réflexion CoreUObject, la classe générique UBlueprint et l'inspection des sous-objets de Designer et graphes, UMG explicite. Runner one-shot relancé sur la filiation exacte avec le même diff cumulé. Aucun changement runtime, asset ou save. Attendre les deux statuts et statut natif, puis enfant binaire. 5G.1 rebasé après preuve. Draft 5G.2 (Details canoniques) dd4425289b7a3002b3e4964c14d7cb25cd40a74b : reprendre exclusivement ses quatre fichiers après deux statuts du nouveau HEAD. Draft 5G.3 (retrait producteurs historiques) 9f3bad580a04c90ca777a80fc583e6502b06f543 : reprendre exclusivement ses 21 fichiers après 5G.2 validé.

## ARM-CLEAN06.5F — palette d'icônes, transaction native du WBP

Parent c8dfc6b161047bdf77b31d3389f551d015e7ea70 validé. Unification projection widget/vue, aucun recours à la palette historique ni fallback dual enregistré. Commandlet natif Audit/Apply/Verify limité au CDO de WBP_GridCombatHudAction, parité des 12 catégories et manifeste du Designer/graphes/propriétés. Agent one-shot sur sujet/parent/diff exacts, copie SHA256/LFS, relecture et Automation complète avant push du seul WBP avec rollback. Statut tiers et enfant binaire à constater. Voir [5F](ARM_CLEAN06_5F_ICON_PALETTE_RETIREMENT.md).

## ARM-CLEAN06.5E.4D — transferts physiques sans ancien DA inscrit

Parent `749301ba89f408b2f21e76517066269c6905a364` : deux statuts SUCCESS. Le résolveur runtime refuse les ID inscrits avant parcours des placements ou recettes ; le lookup de transfert pawn/controller filtre aussi la map. Les jets utilisent exclusivement les politiques matérialisées 5D.3, et les projectiles inscrits ignorent un ancien pointeur explicitement fourni. Tests 3AV/3BF et 5E4D.PhysicalTransferLegacyLookup. Aucun asset, SaveGame, WBP ou clé localisée modifié. Voir [5E.4D](ARM_CLEAN06_5E4D_PHYSICAL_TRANSFER_RETIREMENT.md).

## ARM-CLEAN06.5E.4C — réhydratation strictement hors registre

Parent `29d8b144c54fce3518bdf5da0c078797bd17b9eb` : SilentTask et LocalAuto SUCCESS. New Game / Continue valident les instances canoniques mais n'appellent plus le resolver historique pour un seul ID inscrit (sac actif/réserve, équipement, curseur, raccourci seul). Remplacement atomique de map uniquement hors registre, échec strict des ID inconnus, pas de mutation des instances/caches/SaveGame. Nom des sources d'absorption physique lu depuis FGridResolvedItem.DisplayName, y compris sans ancien pointeur. Test 3AT vérifie zéro appel canonique, éviction des anciens assets préinscrits, rollback et accès strict hors registre, trace bouclier réelle et raccourci seul. Le test historique 0192 sans GI conserve son contrat de compatibilité. Aucun changement UI.csv/ST_UI/WBP/Game fr/en. Voir [5E.4C](ARM_CLEAN06_5E4C_CANONICAL_REHYDRATION.md). Les chargements explicites de DA historiques dans les transferts restent un objectif distinct à auditer/retirer avant 5G ; ne pas confondre cette étape avec suppression de tous les accès historiques.

## ARM-CLEAN06.5E.4B — présentation ARM canonique

Parent `7e3b7720ff34b800fe3e09f773c6d877bf697fb0` : deux statuts SUCCESS. Snapshot transitoire GUID/niveau/action réelle avant dépenses/consommation dans attaques simples, doubles (deux sources), ciblées et contre-attaques ; compétences empruntant une arme conservent son unique liaison d'attaque de base. Projection inverse sans perte des trois canaux média et timings, aucune relecture inventaire ou ancien asset ARM à la présentation. Test natif 3A égalité complète des 19 anciens profils (dont neutres), N1/N20 ; 3AP source N3/N19 retirée avant résolution des FX, binding emprunté/refus non lié. Pas de migration SaveGame, WBP, binaire ni nouveaux canaux. Validations exact-SHA obligatoires. [Détail](ARM_CLEAN06_5E4B_CANONICAL_ATTACK_PRESENTATION.md).

## ARM-CLEAN06.5E.4A — préconditions au retrait de la réhydratation

Parent `47a3fad5454d1a48da8b90257568743a9b6e3cf2` : SilentTask/LocalAuto SUCCESS. Deux dépendances encore réelles découvertes : le clic droit Equiper imposait une ancienne définition malgré le menu/CanEquipItemToSlot canoniques ; ResolveProfile du composant PlayerAttackPresentation lit toujours l'ancien profil ARM. 5E.4A retire la double validation UI et protège la réhydratation contre N1 physique non-ARM (alias famille autorisé seulement en lecture non physique). Tests exécution réelle menu indexé N20/N1 et ancien Book contradictoire, refus N0, contrôle Save N1 avant resolver et rollback. **Ne pas désactiver encore le resolver historique : migrer les profils d'attaque Audio/VFX ARM (snapshot de la source réellement utilisée, y compris dernier projectile et dual wield) puis revoir les transferts.** Aucune écriture de SaveGame ou binaire, UI.csv/ST_UI/WBP/Game fr/en inchangés. [Détail](ARM_CLEAN06_5E4A_REHYDRATION_PRECONDITIONS.md).

## ARM-CLEAN06.5E.3.1 — correction de compilation du test

SHA précédent `3ce60215d5bac92b9c6de97358cb5abd06d1bbd2` : SilentTask SUCCESS, LocalAuto FAILURE (BUILD_FAILED, C2672, [issue #200](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/200)). Correction de la déduction de type `TestNotNull` par `.Get()` sur le composant de projectile ; assertion conservée. Aucun changement runtime ni asset. Attendre les deux statuts exacts du commit correctif avant poursuivre.

## ARM-CLEAN06.5E.3 — lancement canonique des munitions

Parent `bbe1705d8492a8f20065571f17eb5ee27c4c3d78` : SilentTask/LocalAuto SUCCESS. Projection de vitesse des munitions au même préflight que le lien AmmoKindId, priorité au projectile spécialisé actif sinon GenericThrow.InitialSpeed avec clamp historique 400–4800. Pointeur ancien nul pour toute munition inscrite (y compris lanceur non inscrit). Refus vitesse invalide/niveau physique N1/stack invalide avant débit ; aucun changement des trajectoires valides, de l'ordre de transaction ou de SaveGame. Test 3BB renforcé (parité source native, priorité, refus, vrais projectile/stock). Attendre les deux statuts exact-SHA. La réhydratation globale et les transferts restants restent à examiner. [Détail](ARM_CLEAN06_5E3_CANONICAL_AMMO_LAUNCH.md).

## ARM-CLEAN06.5E.2 — sources des attaques et effets

Parent `83f3643af6fb85e78ae70e919367e4698e5f6516` : SilentTask/LocalAuto SUCCESS. L'entrée d'attaque directe et les attaques empruntées utilisent maintenant la même projection standard que HUD/dual wield. Tags ARM lus sur le vrai objet, même si liste vide ; QuickItem conserve les tags de l'action projetée, aucun retour aux anciens tags. Effets directs pris sur l'action canonique ; effets explicites des actions de classe inchangés. Garde de production 22 sources/19 attaques/3 passifs contre perte de tags ou effets non projetés. Aucun changement du coût de la commande de diagnostic directe, de la préparation des projectiles, des assets ni des sauvegardes ; prochains audits munitions/flux et réhydratation. Deux statuts exact-SHA requis. [Détail](ARM_CLEAN06_5E2_ATTACK_CONTRIBUTIONS.md).

## ARM-CLEAN06.5E.1 — lecteurs d'infobulles et lecture

Parent `59202029bb9fb81403b349ebb5623d446f73fac9` : SilentTask/LocalAuto SUCCESS. Précondition au retrait de Rehydrate : la projection tooltip exigeait encore un ancien pointeur même pour les ARM inscrits. Désormais profils, comparaisons source-backed, bonus, lumière, actions QuickItem et texte de surface passent par les modèles canoniques ; la lecture partage le texte par défaut canonique avec le menu et son exécution. Test monde réel N20/N1, torche, huile et refus du niveau non-ARM invalide. UI.csv/ST_UI/WBP/Game fr/en inchangés, aucune nouvelle clé ; PIE dû. Ne pas retirer encore la réhydratation globale : autres consommateurs (combat, transferts, etc.) à vérifier. Aucun binaire ni sauvegarde modifié. Attendre deux statuts du SHA exact. [Détail](ARM_CLEAN06_5E1_POINTERLESS_TOOLTIP.md).

## ARM-CLEAN06.5D.5 — retrait des contributions historiques ARM

Parent exact `6a475dcb3c35bf533f12e940a7fb3e26bf778b91` : SilentTask et LocalAuto SUCCESS, y compris la preuve native read-only des 22 sources ARM sans bonus hors armure ni résistance. Le modèle commun ignore désormais toute ancienne contribution d'une identité ARM inscrite, même avec pointeur contradictoire ; armure N1–N20 canonique inchangée, contributions non-ARM et cas non inscrits conservés. Les tests 3AD/3AE sont actualisés au nouveau contrat (N0/N21 invalides, pointeur absent/contradictoire, prévisualisation, résistance). La preuve native 5D.4 reste intacte et obligatoire. Aucun changement de package, de sauvegarde, de schéma ou de WBP. Attendre les deux statuts exact-SHA avant poursuite ; aucun retrait global des pointeurs anciens ni validation PIE implicite. [Détail](ARM_CLEAN06_5D5_ARM_CONTRIBUTION_RETIREMENT.md).

## ARM-CLEAN06.5D.4 — contributions d'équipement non-ARM

5D.3B matérialisé par `fcd13d5d8c117ed70acdfd97001a487f73f77184`, enfant direct de `1c74fd7ced53266d40a626d6c26b7e6f33eba4ea` (statut spécifique SUCCESS : 19 ARM, deux catalogues, réouverture et Automation OK). Exactement les deux packages canoniques attendus. 5D.4 : projection des bonus et résistances non-ARM déjà authorés, armure N0 et réserves par instance via ItemSystem, gameplay/prévisualisation/diagnostics communs, aucune écriture de sauvegarde ni binaire. Garde native read-only sur les 22 sources ARM : contributions hors armure strictement égales au défaut ou migration explicite nécessaire. Attendre les deux statuts de ce lot avant tout retrait historique. [Détail](ARM_CLEAN06_5D4_NONARM_EQUIPMENT.md).

## ARM-CLEAN06.5D.3.7 — parité réelle des suites pré/post conversion

`267eab44fe9bc737721fef94608ee7a1054bf49b` a publié le diagnostic [issue #199](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/199) : 1981/32/2/0 (succès/avertissements/échecs/non exécutés). Deux tests anciens présupposaient l'absence des politiques 5D.3 : CompleteProductionPreview23 compare maintenant le candidat transitoire précisément reprojeté depuis les 19 sources, sans exclure de propriété ; GenericFallback vérifie le runtime canonique après matérialisation (null/ancien contradictoire, permission, niveau, menu) et conserve le test historique pré-matérialisation. Relance native sur nouveau SHA après ses deux statuts, aucun runtime modifié. Attendre le statut spécial et commit binaire puis ses deux statuts. Draft suivant `5b96b41db3090d53bbd6b16cdff9deecbfb15f4d`.

## ARM-CLEAN06.5D.3.6 — diagnostic post-conversion

`bb58ae3be7fa9d8ee913a2ca516096e251434389` : SilentTask et LocalAuto SUCCESS ; statut spécial FAILURE/AUTOMATION après écriture et réouverture vérifiées, aucun enfant binaire publié. Collecte rétrospective autonome du rapport privé exact par l'agent, statut UE5.5.4/ARM-CLEAN06.5D3-diagnostic et issue limitée aux tests/états/compteurs. Ne pas relancer Apply avant lecture de ce diagnostic. Ce SHA diagnostic ne déclenche volontairement pas la conversion. Draft 5D.4 amélioré `5b96b41db3090d53bbd6b16cdff9deecbfb15f4d` ; reprendre uniquement ses fichiers après le commit binaire et ses deux statuts.

## ARM-CLEAN06.5D.3.5 — correction syntaxique confirmée

`a74d58da10baf45af58da8c35bec6d3d9905208a` : deux statuts généraux SUCCESS ; statut natif FAILURE `PARSE_MissingEndParenthesisInSubexpression_L238_C5`. Cause exacte : parenthèse fermante manquante du tableau des arguments Apply. Tableau séparé et appel nommé. Aucun changement de C++, binaire ni sauvegarde. Relance de l'agent sur le SHA exact ; vérifier le statut natif et l'enfant binaire avant 5D.4. Draft `f5401a1fc7547a73f4d9963d1c6e096f63ef4ba2`.

## ARM-CLEAN06.5D.3.4 — exécution réelle du self-test Windows

`989ea666ef6404ed8e119c7b4b6ba58c4cfe7e88` : les deux statuts généraux sont SUCCESS ; statut natif FAILURE HOOK_PARSE. Le hook lançait un appel de parsing supplémentaire avant le processus réel. Désormais `-File -SelfTest` est le test de parsing et d'exécution obligatoire ; diagnostic sûr identifiant/ligne/colonne si démarrage impossible, reçu SELFTEST_SUCCESS exigé avant écriture. Aucun changement de C++ ni binaire. Deux statuts exacts du nouveau SHA + statut natif et enfant binaire à vérifier. Draft 5D.4 `f5401a1fc7547a73f4d9963d1c6e096f63ef4ba2` inchangé.

## ARM-CLEAN06.5D.3.3 — isolation et diagnostic du self-test

`c7b56b425a531164452d533951b51e23b0c7d0f7` : SilentTask SUCCESS, LocalAuto FAILURE/PREFLIGHT, [issue #198](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/198), aucune compilation ni écriture native confirmée. Le self-test de migration reste obligatoire mais est isolé dans le hook de conversion après la validation générale, avec un reçu de réussite même RunDir et des codes d'étape fixes. Le correctif C++ validé d19b3535 n'est pas modifié. 5D.4 préparé, arbre de draft corrigé `f5401a1fc7547a73f4d9963d1c6e096f63ef4ba2` (reprendre seulement les neuf fichiers du lot ; ne pas reprendre les anciennes versions des autres fichiers). Rebase après commit binaire validé.

## ARM-CLEAN06.5D.3.2 — préflight de conversion native

`d19b3535b313ba405210da2d3090eaa612554c46` : les deux validations Windows du C++ et de tous les tests passent. Le statut spécial de conversion native est FAILURE/PREFLIGHT, aucun enfant binaire. Correction du runner et hook : normalisation worktree Windows, diagnostics de sous-phase et SelfTest dans ValidateUE. Le détail de la cause initiale reste inconnu. Relance automatique uniquement après validation exacte du nouveau SHA. Aucune écriture de sauvegarde. 5D.4 demeure préparé, rebase après binaires validés.

## ARM-CLEAN06.5D.3.1 — permission de lancer indépendante des slots

5D.3 `e7e4f11d7c9dd6ea622e03c71dafd038f5e4f11a` : SilentTask SUCCESS, LocalAuto FAILURE (issue #197, RealHandThrowProjection). Cause probable étayée par `GridWeaponPipelineCommandlet::Usage` et le pilote : les passifs NotHandHeld restent équipables OffHand, contrairement à l'hypothèse du convertisseur. Correction explicite de la permission physique, assertions de parité conservées, diagnostics publics à liste blanche. Le code déclenche le même runner natif uniquement après ses deux validations exactes. Aucune écriture de sauvegarde ni package à ce stade. Le lot 5D.4 préparé (arbre Git `ebd25012b35e51955745e0194b69d14a5bd7aee9`) devra être rebasé après le commit binaire et ses deux statuts.

## ARM-CLEAN06.5D.3 — lancer physique des ARM non spécialisées

Parent 5D.2.1 `0be0a8ddd471d74a2a70e05fff47df0a5782b7f0`. Projection des 19 politiques ordinaires, 3 spécialisées inchangées, 2 catalogues et 22 identités exactes. Pas d'attaque inventée ; préflight transitoire, même politique physique/visuelle N1/N20, tests runtime et garde avant écriture. [5D.3](ARM_CLEAN06_5D3_ARM_HAND_THROW.md). Aucune modification de save. Le code n'est pas la preuve de matérialisation ; attendre le SHA binaire et ses deux statuts.

## ARM-CLEAN06.5D.2.1 — correction des réceptacles autonomes

5D.2 `bed4605243471c0c858b60ecebb1615693a626fe` : SilentTask SUCCESS, LocalAuto FAILURE (deux tests de réceptacles autonomes, [issue #196](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/196)). Cause : la création canonique exigeait un AGridLevelRuntimeActor alors qu'un réceptacle autonome possède un UWorld et le registre réel sans cet acteur. Création canonique directe dans ce cas seulement, exactement même initialisation complète, jamais fallback ID-only ; tests autonomes renforcés (GUID/niveau/pointeur/présentation). Retest complet avant poursuite. Pas de fichiers natifs/saves touchés.

## ARM-CLEAN06.5D.2 — réceptacles et serrures

Parent 5D.1 `2879e8464dafda87d792f177399b3f5c428fc656` : SilentTask + LocalAuto SUCCESS. Initial migré, cursor, restore, clé physique : présentation complète canonique sans ancien pointeur, GUID/niveau/readables conservés. Fixtures initiales non converties et méthode ID seul encore historiques. [5D.2](ARM_CLEAN06_5D2_RECEPTACLE_CUTOVER.md). Aucun binaire ni save modifié. PIE dû. Reste physique ARM hors projectile spécialisé, autres lecteurs, WBP/anciens packages/outils.

## ARM-CLEAN06.5D.1 — lancers génériques non-ARM

Parent 5D `46d7dcd8113685aca7629543acffde62046b57c6` : SilentTask + LocalAuto SUCCESS. Projection transitoire de GenericThrow et HandPolicy depuis les définitions natives, N0 réel obligatoire, force/masse et spawn sans ancien pointeur. Paramètres de vol des non-ARM et munitions lus depuis GenericThrow, y compris axe, pose, rotation, durée et offset. Test natif des 26 non-ARM et spawn des objets à une main éligibles. Pas de mutation de package ni save ; 5B.1 reste installée. **Les lancers génériques ARM sans projectile spécialisé conservent encore les anciennes données physiques** : audit/cutover dédié nécessaire avant suppression. PIE reste dû.

## ARM-CLEAN06.5D — lecteurs d'exploration pointerless

Parent 5C `00a0b3a5798a6f7cf3ff324694a67db4fb229e78`. Trois parcours pièges, HUD et exécution huile/catalyseur et mesh de présentation résolus via DA_ItemSystem lorsque connu, pas de fallback enregistré. Tests sur identités réelles N0 et refus d'absence/action/niveau ; 5C est un prérequis exact-SHA. [5D](ARM_CLEAN06_5D_EXPLORATION_ITEM_READERS.md). Sauvegardes 5B.1 installées, ne pas relancer. Reste retrait final des rehydrate et autres lecteurs, WBP, propriétés, packages et outils de migration ; PIE dû.

## ARM-CLEAN06.5C — kits de départ canoniques

5B.1 `5bf6ae63bd903c0fd58cd640a988ba086ab80997` : deux statuts SUCCESS et installation locale réussie, cold audit 7/7 sans erreur/avertissement, deux readables restaurés et une seule save remplacée, six inchangées, originaux archivés. Ne pas relancer installation/migration. 5C : 18 kits, 6 classes, ItemId+N1/N0 explicites dans le chemin registre, pas de définition historique ni fallback si registre fourni. Tests historiques worldless encore conservés et rehydrate non encore supprimé ; test réel 48 définitions / 18 constructions et parité. [5C](ARM_CLEAN06_5C_CANONICAL_STARTERS.md). Attendre les deux validations exact SHA ; poursuivre lecteurs runtime et WBP/retrait des assets. Pas d'intervention opérateur pour ce lot C++.

## ARM-CLEAN06.5B.1 — installation contrôlée

SHA conversion `475f7f3f04c60ea50bd9ed44ef2bee69c7116ba0` : deux statuts SUCCESS, conversion réelle 7/7, six copies octet-identiques, une réécrite avec deux références restaurées, zéro erreur. Warning attendu sur ancien chemin à l'entrée seulement. Originales intactes, reçu privé non installé. Installer après validation du SHA code 5B.1 via InstallARM_CLEAN06_5B_ReadableSaveCopy.ps1, reçu 5B exact ; relecture froide des copies et contrôle des deux cibles réelles avant remplacement atomique de la seule save concernée. Reçu d'intention et preuve d'installation privés, garde par hash des sept fichiers, retour arrière sur octets connus uniquement. Aucun asset supprimé. Puis 5C, 5D, 5E–G, 06.6.

## ARM-CLEAN06.5B — traitement des readables sur copies uniquement

Parent `177664b26aa07debf775b1f16716844037345112` : SilentTask/LocalAuto SUCCESS ; audit opérateur 5A.1 terminé, 48↔48 IDs, 466 résolutions, 54 candidats, une seule arête externe WBP_GridCombatHudAction → DA_EquippedWeaponActionIcons. 7/7 v27 décodables et live inchangé ; 172 entrées objets, 33 caches nom/poids, zéro ID inconnu, zéro pointeur projectile/pending, deux anciens chemins readables et deux IDs sans pointeur dans une seule save. Les quatre fichiers MON188 restent protégés. [5B](ARM_CLEAN06_5B_READABLE_SAVE_REPAIR.md). Réparation ponctuelle Editor-only par identité réelle des deux readables déplacés ; correspondance exacte des occurrences du chemin ancien et des champs nuls, preuve round-trip et refus d'ambiguïté, no-op octet exact. Aucun fallback runtime, aucune installation live ni suppression ; les readables ne sont pas des anciennes définitions d'objets. Mode -RepairReadables du runner X.64, reçu distinct. Attendre deux statuts du SHA code puis copies privées vérifiées. Kits et lecteurs/caches à traiter ensuite, WBP et anciens packages seulement après. PIE dû.

## ARM-CLEAN06.5A.1 — correction du wrapper Git de l'audit

Parent `a202b99882de3e1499a02d48885e78fad1e6f810` : SilentTask + LocalAuto SUCCESS ; lancement opérateur du script 5A en échec CallDepthOverflow avant toute copie/commande Unreal. La fonction Git masquait git (PowerShell insensible à la casse). Wrapper renommé Invoke-ArmRetirementGit, chemin git.exe résolu comme Application. ValidateUE appelle désormais le script réel avec -SelfTest (HEAD/status et copie/hash de fichier synthétique, sans lecture de saves réelles ni réseau/UE). Nouveau SHA à valider, puis audit opérateur lecture seule sur copies ; aucune suppression. Inutile de restaurer la transaction précédente pour cet échec de préflight.

## ARM-CLEAN06.5A — inventaire de retrait (ne pas supprimer)

X.66B binaire `52fdac2a79cd8f33660996b527b9c69b1c69a6ac` validé SilentTask + LocalAuto, exactement 12 packages, 48 définitions/14 actions, 107/107 références de X.48, 2006 Automation réussis dont 31 avertis, aucune save modifiée (isolation OK, zéro résidu). PIE alchimie reste dû. [Audit 5A](ARM_CLEAN06_5A_RETIREMENT_EVIDENCE.md) : scan natif final des anciens packages et des copies v27, y compris pointeurs de monde toujours sérialisés, caches DisplayName/Weight, readables déplacés et chemins bruts. Aucune suppression jusqu'à examen des preuves. Après validation du SHA 5A, exécuter l'audit local en lecture seule sur copies et transmettre retirement-evidence.json ; aucune relance des anciens scripts Apply. Les quatre anciens MON188 restent protégés. 06.4 non clôturée globalement, 06.5 préparatoire, 06.6 non faite, v27 préservée.

## ARM-CLEAN06.4X.66F.1 — remplacement atomique PowerShell 5.1

Le premier SHA X.66F `f3bf06b62aa94f5b97ffd0643fce3e629ab2bcc1` a obtenu SilentTask success, LocalAuto failure au PREFLIGHT ([issue #195](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/195)), avant Unreal. Cause probable identifiée par revue : le paramètre de backup .NET File.Replace reçoit un $null PowerShell susceptible de devenir une chaîne vide. La restauration des packages et l'installation/restauration des saves utilisent désormais le même helper compare-and-replace avec NullString.Value, vérification après écriture et tests synthétiques aller/retour du vrai helper. Phases fixes des tests publiables par l'agent sans chemin, contenu ou diagnostic privé. Pas de réécriture de master ; deux statuts du nouveau SHA nécessaires. Ne pas lancer X.66B sur le SHA F en échec.

## ARM-CLEAN06.4X.66F — rollback indépendant et preuves natives persistantes

Parent : `46f42513f42eccae7a1b2c778146e62e0a083de7` (SilentTask et LocalAuto success, isolation Automation X.66E validée). Le catch X.66 n'interrompt plus la restauration des packages connus sur erreur SaveGames : quatre phases indépendantes agrègent les erreurs, et conservent le refus de toucher aux octets inconnus, aux sauvegardes non installées, au travail utilisateur et à un commit existant. Reçus natifs avant/après, archive des candidats exacts et rapport privé de rollback. Tests PowerShell synthétiques exécutés par ValidateUE avant compilation. [Rapport X.66F](ARM_CLEAN06_4X66F_NATIVE_ROLLBACK_SAFETY.md).

Deux statuts exact-SHA nécessaires. Ensuite seulement, checkout principal propre, UE et jeu fermés, `git pull --ff-only` AVANT invocation du script X.66 révisé (ne pas exécuter l'ancien script chargé en mémoire). Nouvelle transaction native complète, pas de réutilisation implicite des anciennes copies ; vérifier ensuite 48/14, 12 packages et deux statuts du SHA binaire. Les quatre résidus MON188 historiques sans preuve individuelle restent protégés comme toute autre sauvegarde du reçu. Aucun fichier personnel publié ; v27 inchangée.

## ARM-CLEAN06.4X.66E — récupération X.66D et isolation Automation

Parent : `e59b192eeb8d210ff851aeb28f73cd3adc9a1ac0` (les deux statuts Windows success). X.66B avait réussi Apply, Verify (48/14) et 2005 tests (31 avertis), puis CheckSaveSet a rejeté un checkpoint MON135 supplémentaire ; le catch n'a pas restauré. L'audit opérateur et la récupération X.66D confirment 7/7 saves live originales et deux jeux de copies intacts, 12/12 états natifs restaurés, 12 candidats et le checkpoint exact archivés en privé, master propre. 38 persistées, 107/107. Quatre anciens MON188 restent à qualifier. [Rapport X.66E](ARM_CLEAN06_4X66E_AUTOMATION_SAVE_ISOLATION.md).

Ce lot isole les processus ValidateUE via UserDir vérifié au démarrage, contrôle l'inventaire live et les résidus isolés, protège les tests disque connus et corrige le nettoyage MON135/MON188. Deux statuts exact-SHA requis. X.66F : rollback indépendant, preuve persistante des candidats et tests de panne avant nouvel Apply ; aucun relancement natif ni suppression historique avant validation. SaveGame v27 inchangée ; aucun fichier de sauvegarde ou snapshot publié.

## ARM-CLEAN06.4X.66C — résolution RPG à cache froid

Parent : `5d13c82a3e3a6f93d1f8fcaf79b9f70d9f9cad99` validé. Exécution opérateur X.66B stoppée avant Apply : conflit RPGRace/RPGRaceAsset (Dwarf), RPGPortraitSet/RPGCharacterPortraitSetAsset, puis rejet budget ActiveCharacter[0] v27. [Rapport X.66C](ARM_CLEAN06_4X66C_COLD_RPG_SAVE_RESOLUTION.md). Corriger la résolution sémantique par classe AssetRegistry lorsque AssetManager ne trouve pas le vrai ID ; test natif cache froid et diagnostic de budget détaillé, aucun assouplissement du validateur ni écriture joueur. Après validation du SHA code, reprendre la même transaction native avec le nouveau SHA. 38 persistant, migration non appliquée.

## X.66 — Native alchemy cutover prepared (not applied)

Code prepares the exact 12-package cutover (38 -> 48 definitions, 4 -> 14 detailed actions), guarded save copy installation and verified historical regression views. Parent X.65 `d944814a072f94b7483b23a63ea4fc204d4257c5` passed both Windows statuses. See [X.66 native procedure](ARM_CLEAN06_4X66_NATIVE_ALCHEMY_CUTOVER.md). Native apply requires local Unreal/SaveDirectory and exact code SHA validation. No old assets removed; no local saves accessed by code publication. Remain at 38 persisted definitions until X.66B.

## ARM-CLEAN06.4X.65 — kits alchimie et garde d'authoring

**Parent :** `433c08fde6fe47fcdf5550c211bc521b7bc4281a`. [Rapport X.65](ARM_CLEAN06_4X65_ALCHEMY_STARTERS_AND_AUTHORING_GUARD.md). Les deux entrées production CreateInitialCharacter/preview wizard passent le registre existant : FireBomb et Oil enregistrés sont des entrées explicites N0 quantité 1 en sac sans ancien pointer, aucun fallback pour ID connu invalide ; FireBomb non inscrit reste source transitoire avant cutover. Lecteur UI gère les entrées explicites. Garde legacy FRPGAlchemistAuthoring : ne touche déjà plus Oil, puis aucun des onze une fois inscrits ; refus partiel. Tests vrais kits et authoring, aucun nouvel asset/texte/sauf code. Prochain travail : transaction 12 packages, gates 38/48, conversion de copies et installation synchronisée, gameplay. Registre natif 38, 107/107, aucune sauvegarde live touchée. 06.4 partielle.

## ARM-CLEAN06.4X.64 — conversion binaire v27 sur copies, sans installation

**Parent :** `89f3beb4891633cd042ad153fcd96997c79cd38b` (X.63, SilentTask + LocalAuto SUCCESS). Première validation X.64 : C2440 fixture six attributs (issue #193), correction du fixture ; deuxième SHA compilé, fatal pendant preuve binaire (issue #194), invariant de variables corrigé + EncodeChecked avant sérialisation, nouvelle validation exacte-SHA. [Rapport X.64](ARM_CLEAN06_4X64_SAVE_COPY_CONVERSION.md).

Helper Editor-only transforme les vrais octets SaveGame v27, compare toutes les propriétés SaveGame après rechargement indépendant, refuse anciens IDs ambigus de surfaces/pièges, no-op octet exact. Commandlet SaveCopy + runner PrepareARM_CLEAN06_4X64_SaveCopies.ps1 produisent copies privées vérifiées et reçu SHA256, jamais d'installation ni écriture live. Test binaire actif/réserve et état monde. Aucun fichier joueur ouvert dans ce commit ; aucune commande manuelle demandée séparément. Préparer ensuite la transaction alchimie 12 assets et synchroniser l'installation des copies, gates 38/48, kit Grenadier, authoring et gameplay. Les pointeurs de sauvegarde monde historiques restent hors lot. 38 persistant ; 107/107 ; 06.4 partielle.

## ARM-CLEAN06.4X.63 — projection conjointe alchimie et remapping en copie

**Parent :** `2497018bbc89616ce8f551e9faf13641677265dc` (X.62, deux statuts SUCCESS, rapport natif transmis le 18.11.2026 : 10/10 projections, Oil exact, 0 erreurs / 2 warnings Status_Silence absent, dépôt propre). [Rapport X.63](ARM_CLEAN06_4X63_ALCHEMY_JOINT_PROJECTION.md). Preuve privée `AlchemyEvidence-20261118-201511678.json`.

Plan transitoire 38→48 définitions, 4→14 actions détaillées, quatre IDs DefensiveElixir_Fire/Ice/Lightning/Poison uniques, seul ActionId change ; six tags de catégorie Potion, autres SourceTags/effets/PA/cooldowns intacts. Helper Editor-only convertit en copie les raccourcis des quatre élixirs par SourceDefinitionId dans actifs et réserve, atomique et idempotent, refus ambiguïté. Tests projection, hotbar réelle, copie de struct v27 et catégorie. **Aucune migration native ni SaveGame ici, ni compat legacy par alias.** Valider le SHA code, puis transaction protégée 12 packages et stratégie des fichiers v27 sur copies avant activation ; adapter authors/gates, vérifier exécuteur complet. Status_Silence uniquement filtre de retrait historique (Panacea/Priest), pas d'invention ni suppression de mécanique. 38 persistant, 107/107, 06.4 partielle, 06.5/06.6 ouvertes.

## ARM-CLEAN06.4X.62 — audit natif d'alchimie et projections sans écriture

**Parent :** `4cdde5e4dbe670de0bb95efa30f36167c5a56c62` (X.61B, deux statuts Windows SUCCESS, 1997 tests, propriétaire éditeur/PIE 8/8). **107/107 références du périmètre X.48 migrées ; 38 définitions.** [Rapport X.62](ARM_CLEAN06_4X62_ALCHEMY_SOURCE_AUDIT.md).

Dix vrais objets d'alchimie hors registre + Oil canonique témoin. Le code d'authoring des quatre élixirs partage Action_Alchemist_DefensiveElixir malgré quatre statuts différents ; les données natives doivent confirmer. FireBomb utilisé par kit Grenadier C++, anciens outils et authoring d'Oil à traiter. Audit/commandlet lecture seule projettent individuellement chaque objet dans un registre transitoire, comparent toutes les propriétés d'action, exportent métadonnées, groupes de collisions, différences Oil, références package et statuts. Test de collecte et refus d'ID dupliqué. Aucun changement runtime, ni native, ni SaveGame, ni localisation. **Attendre deux statuts sur SHA code puis recevoir le JSON du commandlet -ReadOnly** ; aucune recette PIE supplémentaire. Ne jamais exécuter un ancien migrateur. Pas de nouveau registre de 48 ni de modification d'identité d'action avant vérification des données et traitement explicite des bindings v27. 06.4 partielle, 06.5/06.6 ouvertes.

## ARM-CLEAN06.4X.61 — quatre loots + registre et fin du périmètre de 107 références

**Parent :** `84d7324af420687e5a17a9696642a80cb88d3a28` (X.60B deux statuts Windows SUCCESS, 1995 tests réussis avec 31 avertis, PIE 8/8). [Rapport X.61](ARM_CLEAN06_4X61_FOUR_LOOT_CUTOVER.md).

X.60B a prouvé quatre placeholders non équipables, sans action ni mesh/icon : GoblinKnife 0.5 kg pile 1, EmptyVial 0.1 kg pile 10, RatMeat 0 kg pile 99, RatTooth 0 kg pile 99. Projection exacte N0 avec catégories UI explicites via tags réservés, sans ARM ni effet inventé. Préflight 34 → 38, vérifie toutes les propriétés, mêmes tables de chances/quantités et 128 seeds par monstre. Commandlet/runner protègent sept packages : trois sources existantes + quatre nouvelles définitions ; rollback exact et fresh Verify avant publication. **Aucun Apply fait par le code : 4 anciens pointeurs restent ; zéro des 107 après X.61B.** Deux statuts exacts + PIE nécessaires ; dix définitions d'alchimie, anciens catalogues, WBP et SaveGame v27 encore à traiter. Pas de nouveau texte/asset visuel ; UI.csv/ST_UI/WBP/Game-fr/en inchangés.

## ARM-CLEAN06.4X.60 — transaction Stone et evidence des quatre autres loots

**Parent :** `1cf5ace7e979eac8843bd9e556a382cb4bc0c8f2` (X.59B, SilentTask + LocalAuto SUCCESS, 1993 tests, PIE 8/8). [Rapport X.60](ARM_CLEAN06_4X60_STONE_LOOT_PROTECTED_TRANSACTION.md).

Le seul loot déjà enregistré Stone N0 a une projection pointerless exacte, manifeste complet de deux monstres, 128 seeds de parité, test de mort/placement réel, adaptation X.48 et TD07. Le composant de mort cesse le lookup legacy pour les IDs enregistrés : lumière canonique, ancienne source directe uniquement pour niveau ARM hérité pendant migration. Commandlet/runner protègent le seul DA_MON_GoblinThrower. Preflight journalise et exporte les cinq définitions historiques ; quatre hors registre restent inchangées (GoblinKnife sans ARM ; RatMeat Food avec ancienne autorisation Consume ; EmptyVial et RatTooth à projeter). **Pas d'Apply par ce commit : 5 références restent, 4 après X.60B réel.** 06.4 partielle, 06.5 bloquée, v27 préservé. Ne pas relancer anciens outils.

## ARM-CLEAN06.4X.59 — nettoyage protégé des trois overrides dormants

**Parent :** `d59d3cb25dd43d02c6b14c8cad340bdf23d81896` (X.58B : 18 contenus natifs migrés, deux statuts Windows SUCCESS, 1992 tests réussis, 8/8 éditeur/PIE). [Rapport X.59](ARM_CLEAN06_4X59_DORMANT_OVERRIDES_PROTECTED_TRANSACTION.md).

Plan strict des trois GUID/cellules de l'audit X.19, listes inactives uniquement, parité du comportement effectif et des helpers de réactivation canonique ; X.48 verrouille les 99 références déjà supprimées et accepte uniquement 8/5 restantes. Commandlet et runner protègent le seul DA_GridLevel_00, manifeste complet des quatre niveaux, snapshot SHA256, verify frais, tests et commit LFS mono-asset. **Aucun Apply fait par le code : 8 pointeurs restent.** Après X.59B réel : 5 loots. 06.4 partielle ; 06.5 bloquée ; SaveGame v27 préservé.

## ARM-CLEAN06.4X.58 — préparation complète de la transaction des 18 contenus initiaux

**Parent binaire :** `8ae270b3e3f2f90ae46e7b88a80676427e7073d6` (X.57B : 33 références palette supprimées, deux statuts Windows SUCCESS, 8/8 éditeur/PIE). Rapport : [ARM_CLEAN06_4X58_INITIAL_CONTENT_PROTECTED_TRANSACTION.md](ARM_CLEAN06_4X58_INITIAL_CONTENT_PROTECTED_TRANSACTION.md).

Selected Object édite désormais le CanonicalItemId et le niveau physique sans source historique ; Details garde celle-ci en lecture seule. Audits 4W/X.2/X.4/X.48/X.91 et acteurs X.92/X.21 adaptés et renforcés ; tests X.163 et nouveau plan natif. Préflight exige 17+1 contenus dans les niveaux 00/01, 0 dans 03/Pillars et soit 18 soit 0 anciens pointeurs. Runner protégé vérifie les deux validations Windows du SHA code, sauvegarde les deux cibles, vérifie la parité de toutes les propriétés de niveau réfléchies avant/après, les SHA256 des quatre niveaux, la suite Grimrock complète et le pointeur LFS staged. **Aucun Apply effectué par ce commit de code : 26 références restantes.** 8 seulement après X.58B réel, puis PIE. Trois overrides dormants et cinq loots strictement préservés. 06.4 partielle ; 06.5 bloquée ; SaveGame v27 non modifié.

## ARM-CLEAN06.4X.57 — runner protégé pour la palette native unique

**Parent :** `bd02f2b0dbb5978e216c5edbe485fc31c4ad0104`. Rapport : [ARM_CLEAN06_4X57_NATIVE_PALETTE_PROTECTED_TRANSACTION.md](ARM_CLEAN06_4X57_NATIVE_PALETTE_PROTECTED_TRANSACTION.md).

Commandlet X.57 utilise le plan canonique X.55, requiert sauvegarde privée byte-equal et SHA opérateur, ne sauvegarde que la palette et compare le manifeste avant/après rechargement. Runner vérifie LFS, SHA256, suite complète, git index, rollback borné et publication binaire mono-asset. Aucun Apply réalisé par le seul commit de code : 59 pointeurs restent, 26 seulement après transaction locale validée. Deux statuts Windows du SHA code requis avant exécution opérateur et deux statuts du SHA binaire avant recette PIE. 06.4 partielle ; 06.5 bloquée.

## ARM-CLEAN06.4X.56 — audits et protection d'authoring de la palette native

**Parent :** `859fa3cc79f04d906c6c6b235a64d28bf55e49ac` (X.55 SilentTask + LocalAuto succès). Rapport : [ARM_CLEAN06_4X56_PALETTE_DUAL_STATE_AUDITS.md](ARM_CLEAN06_4X56_PALETTE_DUAL_STATE_AUDITS.md).

Audits du contenu et tests de présentation/placement autorisent exactement 33/0 anciens pointeurs de palette, contrôlent toujours les IDs/niveaux et la vue canonique ; X.48 borne les vrais totaux à 59/26, quatre règles X.54B définitivement pointerless. Les inspecteurs historiques reconnaissent les identités canoniques sans réattacher de pointeur ; source historique en lecture seule dans Details, Icon réservé aux objets monde. Aucun binaire ni SaveGame changé. Migration native à préparer après les deux statuts Windows du SHA X.56. 06.4 partielle ; 06.5 bloquée.

## ARM-CLEAN06.4X.55 — préflight de la palette complète

**Parent :** `51d3a1e4d15d3560bf77c17ab158d4df934c87c2` (X.54B, quatre assets natifs, SilentTask + LocalAuto succès, PIE 8/8 confirmé). Rapport : [ARM_CLEAN06_4X55_NATIVE_PALETTE_PREFLIGHT.md](ARM_CLEAN06_4X55_NATIVE_PALETTE_PREFLIGHT.md).

Plan Editor-only transitoire, strictement 33 entrées directes et état 33/0 anciens pointeurs ; parité des projections Slate, validation structurelle, contrôle de chaque champ réfléchi et idempotence par test réel. Aucun binaire changé : 59 références restent après X.54B. Adapter les audits et inspecteurs historiques encore liés au pointeur puis préparer la transaction native protégée, jamais les anciens pipelines. 06.4 partielle ; 06.5 bloquée ; deux statuts Windows requis sur le nouveau SHA.

## ARM-CLEAN06.4X.54 — outil protégé pour retirer les quatre pointeurs natifs partagés

**Parent :** `724e987c1b325eab1484bd51fc7c8d26710d8690` (X.53 validé SilentTask + LocalAuto). Rapport : [ARM_CLEAN06_4X54_FOUR_NATIVE_RULE_PROTECTED_TRANSACTION.md](ARM_CLEAN06_4X54_FOUR_NATIVE_RULE_PROTECTED_TRANSACTION.md).

Nouveau commandlet `GridArmClean064X54NativeSharedRules` et script `ApplyARM_CLEAN06_4X54_NativeSharedRules.ps1` : préflight et verify lecture seule, transaction `-Apply` explicitement protégée par quatre sauvegardes privées byte-equal/SHA256, `SavePackage` borné aux quatre définitions Torch/Guardian/Copper/Iron, seconde réouverture Unreal, suite Grimrock, contrôle Git LFS, rollback et publication binaire limitée aux quatre fichiers sur `master`. **Ce commit est uniquement du code : aucun -Apply lancé, aucun .uasset ni SaveGame changé ; 63 pointeurs toujours présents.** Exiger les deux statuts Windows du SHA code X.54 avant toute utilisation locale ; puis opération native opérateur et recette PIE distinctes. 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.53 — plus aucun ajout d'ancien pointeur dans l'inspecteur des serrures

**Parent :** `e4537dbc8de1148232fbc89cc9116c0ef6eeac4a` (X.52 validé SilentTask + LocalAuto). Rapport : [ARM_CLEAN06_4X53_LOCK_INSPECTOR_POINTER_GUARD.md](ARM_CLEAN06_4X53_LOCK_INSPECTOR_POINTER_GUARD.md).

Après effacement du dernier `AcceptedKeyId`, l'inspecteur ne fait plus réapparaître `Add Accepted Key Asset` ou l'ancien picker. Les `AcceptedKeyItems` historiques restent visibles en lecture seule, les IDs restent éditables. X.50 teste maintenant la liste volontairement vide et la conservation des vieux placements ; lecture compatible avec les vrais locks avant/après conversion des quatre définitions. **Aucun binaire migré, 63 références restantes.** Deux statuts Windows requis sur le SHA X.53 ; 06.4 partielle et 06.5 bloquée.

## ARM-CLEAN06.4X.52 — audits de règles partagées sûrs avant/après migration native

**Parent :** `9ab587af5d7c80aac6041eeb9a72ed70309b3f75`. Rapport : [ARM_CLEAN06_4X52_DUAL_STATE_SHARED_RULE_TESTS.md](ARM_CLEAN06_4X52_DUAL_STATE_SHARED_RULE_TESTS.md).

Les tests X.48, X.1661, TD0737/whitelist 841 et X.71/X.81 AuditOnly acceptent désormais les **deux états natifs complets** : quatre pointeurs historiques encore cohérents (63 résiduels) ou aucun de ces quatre pointeurs (59 résiduels), en préservant strictement `Item_Torch`, `Gem_Blue`, `Key_Copper` et `Key_Iron`. Les tests X.71/X.81 ne relancent plus les anciens commandlets dont les préconditions appartenaient à l'ancien schéma. Toute conversion partielle et toute clé inattendue sont refusées. **Aucun .uasset / SaveGame modifié** ; préparer ensuite le nouveau commandlet natif sécurisé, pas les anciens scripts X.72/X.83. 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.51 — préflight des quatre règles partagées réelles

**Parent :** `2690fc7dc335a552ffb0afd647604b5182d9176c`. Rapport : [ARM_CLEAN06_4X51_SHARED_RULE_NATIVE_PREFLIGHT.md](ARM_CLEAN06_4X51_SHARED_RULE_NATIVE_PREFLIGHT.md).

Les quatre règles natives déjà projetées par ID `Item_Torch`, `Gem_Blue`, `Key_Copper` et `Key_Iron` sont testées en **lecture seule**. Le test X.51 duplique chaque DataAsset dans `GetTransientPackage()`, retire le seul pointeur obsolète sur cette copie et contrôle l'identité, les politiques de réceptacle, les messages et la consommation des serrures ; refuse une conversion partielle, sans modifier les packages originaux ni SaveGame. Les anciens tests de manifeste devront devenir compatibles avant toute migration de ces quatre binaires. **63 références natives restent actuellement ; 59 après conversion n'est qu'un résultat attendu.** 06.4 partielle / 06.5 bloquée. Deux contrôles Windows requis sur le SHA X.51.

## ARM-CLEAN06.4X.50 — copie canonique des clés de serrure dans les nouveaux overrides

**Parent :** `336ad30a946284bca1af58a5178eb57836ec681c`. Rapport : [ARM_CLEAN06_4X50_POINTERLESS_LOCK_OVERRIDE_AUTHORING.md](ARM_CLEAN06_4X50_POINTERLESS_LOCK_OVERRIDE_AUTHORING.md).

L'inspecteur verrouille désormais la recopie de `AcceptedKeyItems` lorsque l'utilisateur active l'override de serrure : un helper Editor-only projette atomiquement les sources historiques enregistrées en `AcceptedKeyIds` via le preview X.165, sans ajouter de pointeurs dans le placement. Les IDs scriptés existants restent préservés, une source absente ou non enregistrée refuse la transaction, et le picker legacy n'est plus accessible lorsqu'une liste canonique explicite est autoritaire. Nouveau test X.50, y compris les deux vrais DataAssets Copper/Iron en lecture seule. **Aucune conversion native ni SaveGame écrits.** X.48 a confirmé 63 pointeurs restants ; 06.4 partielle / 06.5 bloquée. Vérifier les deux statuts Windows du SHA X.50.

## ARM-CLEAN06.4X.49 — nouvelles règles de réceptacle pointerless

**Parent :** `e2f4db137dc21b3e27797bd8abe5470ac19eab28`. Rapport : [ARM_CLEAN06_4X49_POINTERLESS_RECEPTACLE_RULE_AUTHORING.md](ARM_CLEAN06_4X49_POINTERLESS_RECEPTACLE_RULE_AUTHORING.md).

L'éditeur ne recopie plus `ItemDefinition` lorsqu'une règle d'acceptation pointe vers un `CanonicalItemId` connu ; l'attribution et le bouton « Copy Defaults » produisent des overrides à ID seul, tout en conservant les cas hors-registre et le refus d'une divergence entre ancien pointeur et identité canonique. Le panneau Slate affiche explicitement l'ItemId des règles pointerless. Test X.162 étendu. **Aucun .uasset ni SaveGame écrit** ; les 63 références restantes vérifiées dans le test X.48 n'ont pas encore été converties. Deux validations Windows requises sur X.49 ; 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.48 — lecture UE des dépendances historiques restantes, après PIE X.46B

**Parent :** `848d0caa2047911869e8eba3a06c79a59f7445ff`. Rapport : [ARM_CLEAN06_4X48_POST_NATIVE_RESIDUAL_AUDIT.md](ARM_CLEAN06_4X48_POST_NATIVE_RESIDUAL_AUDIT.md).

Le propriétaire a **validé le fonctionnement PIE** du premier package migré X.46B (`DA_GridLevel_00`, 44 objets libres pointerless, SHA binaire `cd177cd5473c8ab3777bfecae56cd0e33abd23eb` ; validations Windows déjà SUCCESS). X.48 ajoute le contrôle Editor-only post-conversion des **63** anciennes références restantes **attendues** dans les niveaux, overrides dormants, palette, règles de réceptacles/serrures et deux monstres. Aucun package ou SaveGame n'est écrit ; ce lot échoue si un des 44 objets libres retrouve un pointeur ou si les 63 références changent sans lot de migration. Tant que ses deux statuts Windows ne sont pas `success`, 63 reste un décompte **attendu**. À suivre : lecteurs de règles et conversion binaire bornée, SaveGame v27 conservé. 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.46B — 44 objets libres migrés dans le vrai .uasset, contrôles Windows verts

**Transaction binaire :** `cd177cd5473c8ab3777bfecae56cd0e33abd23eb` (parent `875814f2d189a244cd84ab5f7bfa2fb521174f87`). Rapport de réception : [ARM_CLEAN06_4X46B_NATIVE_LOOSE_RESULT.md](ARM_CLEAN06_4X46B_NATIVE_LOOSE_RESULT.md).

`DA_GridLevel_00.uasset` est désormais réellement resauvegardé par UE5.5.4 avec les **44 `FGridLooseItemInstance::ItemDefinition` retirés**. GitHub compare : **un seul package natif modifié** ; deux validations Windows `SilentTask=SUCCESS` et `LocalAuto=SUCCESS` sur le SHA exact. Sur la version convertie : **1961 tests réussis, 26 avec avertissements, zéro échec ou non exécuté**, réouverture `-Verify` avec code 0. Aucun autre asset ni SaveGame dans le commit. **Prochaine porte : recette PIE post-conversion (mesh, pickup, PIT02/transfert dans la bonne cellule, Save/Continue)**. Avant le reste de 06.4, refaire un scan UE natif des pointeurs résiduels (solde de 63 estimé d'après l'audit antérieur, non revérifié). 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.47 — correctif de test du vrai niveau après conversion et restauration visible

**Parent :** `939ef5dc1846aec896cb2f04261a6e20b3444a30`. Rapport : [ARM_CLEAN06_4X47_POST_NATIVE_LOOSE_TEST_AND_ROLLBACK.md](ARM_CLEAN06_4X47_POST_NATIVE_LOOSE_TEST_AND_ROLLBACK.md).

Premier essai local `X.46 -Apply -Publish` : conversion des 44 références et réouverture UE réussies, puis un seul échec sur 1987 tests (1961 succès, 25 avertissements, 1 échec) : X.8 exigeait encore un pointeur historique dans le vrai niveau. Le script a déclenché le rollback avant tout commit binaire. Correction X.47 : le test X.8 accepte le vrai ARM avec source cohérente **ou** pointerless, tout en imposant la présentation canonique, et injecte explicitement une source d'ID discordant pour que son test négatif continue après migration. Le script affiche `ROLLBACK VERIFIED` seulement après contrôle SHA-256 + worktree propre. **Ce commit ne migre pas l'asset.** Attendre les deux validations du SHA X.47, puis relancer le script X.46 avec le nouveau `-ExpectedHead` sur Windows ; recette PIE après le commit binaire. 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.46 — outil UE natif pour 44 objets libres (code seulement)

**Parent :** `5e79797e8234ccf392642350a84d7d30aff10818`. Rapport : [ARM_CLEAN06_4X46_NATIVE_LOOSE_PROTECTED_APPLY.md](ARM_CLEAN06_4X46_NATIVE_LOOSE_PROTECTED_APPLY.md).

Nouveau commandlet Editor-only `GridArmClean064X46NativeLoosePointers` et script PowerShell `ApplyARM_CLEAN06_4X46_NativeLoosePointers.ps1` : préflight/verify lecture seule, Apply no-op si zéro ancien pointeur, sinon 44 exactement, backup privé SHA256, refus des niveaux/identités invalides, SavePackage limité **exclusivement** à `DA_GridLevel_00.uasset`. La publication binaire n'est déclenchée que par `-Apply -Publish` explicite, après réouverture UE, contrôle mono-fichier et suite Grimrock complète ; aucune tâche automatique ne la lance. Les packages réceptacles/palette, trois overrides, autres niveaux et SaveGame v27 restent protégés. **Le commit de code ne réalise pas la migration native.** Deux statuts Windows requis avant le préflight local. 06.4 partielle / 06.5 bloquée.

## ARM-CLEAN06.4X.45 — préflight natif, 44 références d'objets libres (sans écriture)

**Parent :** `9f72e6e8094b66760ad0b9d1166f0a82bf083197`. Rapport : [ARM_CLEAN06_4X45_NATIVE_LOOSE_POINTER_PREFLIGHT.md](ARM_CLEAN06_4X45_NATIVE_LOOSE_POINTER_PREFLIGHT.md).

Quatre anciennes suites d'audit (X.2, X.4, W, test de véritable donjon ARMEMENT01.10.3) reconnaissent désormais une pose libre canonical-only **sans** autoriser la perte d'ID, de niveau, de GUID ou le retrait des 18 sources de réceptacles / 33 entrées de palette. Nouveau test X.45 sur les 44 vraies entrées de DA_GridLevel_00 : copie transitoire, suppression des pointeurs dans la copie, comparaison de champs stables et refus d'un ARM N0. Aucun SavePackage, package UE, SaveGame ni contenu personnel modifié. Transaction UE native à préparer séparément après deux statuts Windows sur SHA final. **06.4 partielle / 06.5 bloquée.**

## ARM-CLEAN06.4X.44 — Authoring Editor : nouveaux objets canoniques sans anciennes définitions

**Parent :** `53dc881d2be1559fe3154bae7a86b0e0f26867d7`. Rapport : [ARM_CLEAN06_4X44_EDITOR_POINTERLESS_AUTHORING.md](ARM_CLEAN06_4X44_EDITOR_POINTERLESS_AUTHORING.md).

`ApplyPaletteEntry`, `PlaceSelectedObject`, `ApplyEditedSelectedObject` et le picker ItemDefinition ne réécrivent plus `DA_Weapon_*` pour une entrée déjà identifiée par `CanonicalItemId`. Nouvelle pose : pointeur null, identité/niveau/GUID conservés ; édition explicite : purge ciblée uniquement de l'objet modifié ; anciennes entrées non touchées sans action. Les IDs non enregistrés gardent leur chemin historique. Test X.28 étendu à une palette réellement legacy-backed et à l'édition N12. Après un premier échec Automation #191, cinq fixtures historiques de **nouveau placement transitoire** sont corrigées pour exiger ItemId/niveau et prévisualisation canonique plutôt qu'un pointeur ancien ; même lot X.44, aucun rollback de gameplay. Aucun binaire UE ou SaveGame modifié. **06.4 partielle / 06.5 bloquée ; attendre deux statuts Windows.**

## ARM-CLEAN06.4X.43 — kits explicitement canoniques : stack/main/lumière hors legacy

**Parent :** `d3adc0e99adb604c479569fdbf6dba233b863d93`. Rapport : [ARM_CLEAN06_4X43_STARTING_KIT_CANONICAL_AUTHORITY.md](ARM_CLEAN06_4X43_STARTING_KIT_CANONICAL_AUTHORITY.md).

Dans `FRPGStartingEquipmentService::TryBuildKit`, un `CanonicalItemId` explicite s'appuie désormais sur `FGridResolvedItem` pour MaxStack, lumière initiale et politique de main, **même avec un ancien pointeur cohérent encore sérialisé**. Test réel : épée N20, 3 flèches N0 et torche OffHand N0 avec faux anciens champs contradictoires. La liste transitoire des définitions reste inchangée ; aucune classe .uasset ou SaveGame modifiée. **06.4 partielle/06.5 bloquée ; vérifier les deux statuts Windows.**

## ARM-CLEAN06.4X.42 — visuel tenu ARM : niveau physique explicite, sans DA_Weapon

**Parent :** `02ffcb99ad48d3a308a5d31f76cfb59d88199294`. Rapport : [ARM_CLEAN06_4X42_CANONICAL_HELD_PHYSICAL_INSTANCE.md](ARM_CLEAN06_4X42_CANONICAL_HELD_PHYSICAL_INSTANCE.md).

Le visuel tenu des items enregistrés peut désormais partir du `FGridItemInstance` réellement équipé via `EquipHeldItemFromInstance` et obtenir le `EquippedMesh`, la lumière et les effets du catalogue au N1–N20 réel. `SyncHeldVisualFromSelectedCharacterEquipment` utilise cette voie, y compris après changement de niveau. L'API historique `EquipHeldItem(FName)` continue pour torche N0 et IDs hors-registre mais refuse les ARM enregistrées, faute de niveau explicite ; aucun fallback DA_Weapon inventé. Test ARM N1/N20 pointerless, ARM N0 / torche N1 refusés, torche N0/hors registre conservés. Aucun binaire UE ni SaveGame écrit. **06.4 partielle, 06.5 bloquée ; vérifier deux statuts Windows.**

## ARM-CLEAN06.4X.41 — lumière équipée : fin de l'autorité DA_Weapon pour les ARM enregistrées

**Parent :** `396a269290f9bad65cebd4dc827faa2dd93e589b`. Rapport : [ARM_CLEAN06_4X41_EQUIPPED_CANONICAL_LIGHT.md](ARM_CLEAN06_4X41_EQUIPPED_CANONICAL_LIGHT.md).

`DoesEquippedItemEmitLight` et `GridPartyIlluminationComponent::RefreshFromEquipment` utilisent désormais le seul `LightEmitter` canonique pour les ARM N1–N20 et non-ARM N0. Un faux ancien `DA_Weapon` de même ID ne peut plus modifier la lumière équipée. Tests réels : 2 niveaux ARM, refus ARM N0 / torche N1, et maintien hors-registre. Aucun asset UE ou SaveGame écrit. `EquipHeldItem(FName)` reste en dette distincte car son API n'a pas d'ItemLevel. **06.4 partielle / 06.5 bloquée ; vérifier les deux validations Windows.**

## ARM-CLEAN06.4X.40 — entrées monde transitoires sans ancienne source pour les ItemId canoniques

**Parent :** `06effc11ee81d6de76230007a8c0b01f3417ae7b` (règle de lots cohérents). Rapport : [ARM_CLEAN06_4X40_CANONICAL_RUNTIME_ENTRY_SOURCE.md](ARM_CLEAN06_4X40_CANONICAL_RUNTIME_ENTRY_SOURCE.md).

Dans `AddPlacedItemActor`, un objet libre à identité canonique validée ne conserve plus `DA_Weapon_*` dans son `FGridSpawnedItemRuntimeEntry`, en plus de l'acteur physique pointerless X.39. Le poids des plaques de pression ignore désormais tout pointeur d'entrée resté dans une instance historique pour un ID enregistré. Test X.8 étendu aux vrais ARM et non-ARM, avec source historique de même ID contradictoire, vérification directe (lecture seule) des entrées et conservation de l'ItemId/niveau. Aucun .uasset/SaveGame/UPROPERTY touché ; autres pointeurs historiques non migrés. **06.4 partielle / 06.5 bloquée ; SHA final et deux statuts Windows à relever.**

## ARM-CLEAN06.4X.39 — objets libres ARM existants : présentation physique canonique

**Parent :** `519ae9433e8007f66f33793338b78516f590e740`. Rapport : [ARM_CLEAN06_4X39_REAL_ARM_LOOSE_RUNTIME_CUTOVER.md](ARM_CLEAN06_4X39_REAL_ARM_LOOSE_RUNTIME_CUTOVER.md).

Après X.38 (22 profils natifs déjà égaux), `AGridLevelRuntimeActor::AddPlacedItemActor` ne dépend plus de `DA_Weapon_*` pour la création physique d'un objet enregistré, même si le pointeur historique est encore sérialisé dans le niveau : l'acteur n'attache plus l'ancien UObject et lit lumière, Niagara, mesh, transform et physique depuis `FGridResolvedItem`. Test étendu aux 44 vraies instances libres, avec vraie source ARM et faux ancien pointeur de même ID mais lumière contradictoire, ARM N1/N20 pointerless, non-ARM N0 et rejets des identités invalides. La fixture PIT02 `FlyingProjectilePassage` charge sa source historique de lancer indépendamment du nouvel acteur monde sans pointeur (correctif issu de l'échec Windows #190), sans modifier la logique du lancer. Aucun asset natif, WBP ni SaveGame modifié ; les entrées de runtime et 107 pointeurs de packages restent à auditer. **06.4 partielle, 06.5 bloquée ; attendre deux statuts Windows du SHA final.**

## ARM-CLEAN06.4X.38 — 22 profils ARM réels déjà en parité : pas de SavePackage inutile

**Parent :** `5d83c683e7ea6c867187b6dacd9ace9abe96d236`. Rapport : [ARM_CLEAN06_4X38_NATIVE_ALREADY_EQUAL_NOOP.md](ARM_CLEAN06_4X38_NATIVE_ALREADY_EQUAL_NOOP.md).

Preuve Windows utilisateur : compilation native de 452 actions UE5.5.4 réussie, `-Preflight` réel sur les deux catalogues `.uasset` avec `sources=22 mismatching_profiles=0`. L'ancien `-Apply` a malgré tout exigé un manifeste de sauvegarde et renvoyé code 5 **avant SavePackage** ; restauration des deux catalogues annoncée. Correction : `-Apply` lance maintenant `-Verify` dans un SECOND processus UE avant tout snapshot ; si les 22 profils sont déjà identiques, sortie réussie `VERIFIED NO-OP` sans backup ni écriture. Si différences vérifiées (code 4), conserver intact le chemin protégé des 24 backups. Le commandlet direct possède le même no-op sûr. Test Windows sur vrais assets : zéro différence des 220 champs, parité rechargée. **Aucun binaire LFS artificiellement modifié ni commit de migration à créer ; 06.4 partielle et 06.5 bloquée. Deux statuts Windows à vérifier sur SHA final.**

## ARM-CLEAN06.4X.37.2 — build local automatique avant preflight natif

**Parent :** `eefec795d2252dca8371892535063374582135a0`. Rapport : [ARM_CLEAN06_4X372_LOCAL_EDITOR_BUILD_GATE.md](ARM_CLEAN06_4X372_LOCAL_EDITOR_BUILD_GATE.md).

Incident Windows réel : `Failed to find commandlet class GridArmClean064X37NativeArmWorldProfilesCommandlet` lors du `-Preflight` dans le checkout utilisateur. L'agent Windows compile un worktree isolé et ne met pas à jour les DLL du projet principal. Le script X.37 exécute maintenant `Build.bat GrimrockPrototypeEditor Win64 Development -Project=... -WaitMutex -NoHotReloadFromIDE` **avant** le preflight, après contrôle du HEAD/branch/propreté et avec recontrôle après build. Un échec UBT bloque le lancement sans backups/SavePackage. Aucun binaire UE, SaveGame ou migration effectués par ce commit. **Rerun natif X.38 requis avec le nouveau ExpectedHead. 06.4 partielle ; 06.5 bloquée.**

## ARM-CLEAN06.4X.37.1 — sauvegardes natives X.38 : garde-fou renforcé

**Parent :** `5071d6eb4cf65b76e8f8bcd3f9bae1fc37d54687`. Rapport : [ARM_CLEAN06_4X371_BACKUP_RECEIPT_GUARD.md](ARM_CLEAN06_4X371_BACKUP_RECEIPT_GUARD.md).

Le mode `-Apply` du commandlet natif X.37 exige désormais les 24 chemins uniques exacts, leurs identifiants SHA-256 bien formés et une comparaison octet par octet entre les 24 backups privés et les fichiers actifs avant le premier SavePackage. Tests Automation de refus de reçu incomplet, duplicata, sortie de répertoire, hash malformé et HEAD erroné. Le script PowerShell conserve son contrôle SHA-256 propre. **Pas d'exécution Apply ni de modification de .uasset/SaveGame, 06.4 partielle et 06.5 bloquée. Vérifier deux statuts Windows sur SHA final.**

## ARM-CLEAN06.4X.37 — migrateur natif des 22 profils ARM, code seulement

**Parent :** 1ba63f264892b20e4fc5eb72ce39998a86fb7d7b. Rapport : [ARM_CLEAN06_4X37_NATIVE_ARM_WORLD_PROFILES.md](ARM_CLEAN06_4X37_NATIVE_ARM_WORLD_PROFILES.md).

Nouveau commandlet Editor-only GridArmClean064X37NativeArmWorldProfiles : jointure fermée de 22 anciennes armes à leurs 22 profils visuels et 82 archétypes canoniques, copie exacte en mémoire des 8 propriétés visuelles persistantes et 2 propriétés physiques. Preflight et Verify sont en lecture seule ; Apply exige une intervention native explicite avec 24 sauvegardes SHA-256 privées, pour deux catalogues .uasset seulement. Le script ne se lance pas par LocalAuto et ne publie pas de commit. Régression sur sources UE réelles et copies transitoires. **Aucun profil réel migré dans le commit de code X.37 ; X.38 pour les binaires. 06.4 partielle ; 06.5 bloquée ; vérifier les deux validations Windows.**

## ARM-CLEAN06.4X.36 — projectile pointerless complet (mesh, lumière, Niagara)

**Parent :** `871f26100e50fbebed254062dc3c54dd1d7075df`. Rapport : [ARM_CLEAN06_4X36_POINTERLESS_PROJECTILE_PRESENTATION.md](ARM_CLEAN06_4X36_POINTERLESS_PROJECTILE_PRESENTATION.md).

`AGridThrownItemActor` initialise désormais toute arme ou munition enregistrée sans ancienne définition via `InitializeFromCanonicalItem` : présentation complète, puis application du mesh/axe de vol exact. En cas d'échec, le projectile est détruit et la porte de spawn refuse de le livrer. Les projectiles utilisant encore leurs anciennes définitions et les IDs hors registre restent inchangés. Tests sur vraie arme N20 et flèche N0 avec Niagara/lumière. Aucun binaire UE, SaveGame ni nouvel objet authoré. **06.4 partielle ; 06.5 bloquée ; attendre les deux SUCCESS Windows du SHA final.**

## ARM-CLEAN06.4X.35 — refus du spawn physique non-ARM N1

**Parent :** `e41a6b89e87f8c3e2799d9674f9b32d9cb9fac11`. Rapport : [ARM_CLEAN06_4X35_PHYSICAL_THROW_SPAWN_LEVEL.md](ARM_CLEAN06_4X35_PHYSICAL_THROW_SPAWN_LEVEL.md).

La porte de spawn des objets lancés vérifie maintenant le niveau physique strict pour tout ItemId enregistré, même si un ancien `ItemDefinition` correspondant prétend autoriser le lancer. Un non-ARM N1 ne peut plus voler avec un ancien mesh puis échouer à l'atterrissage ; les ARM N1–N20, non-ARM N0 et véritables IDs historiques hors registre sont préservés. Test runtime de spawn avec registre réel et définition contradictoire transitoire. Aucun binaire UE ni SaveGame écrit. **06.4 partielle, 06.5 bloquée ; attendre validations Windows sur SHA final.**

## ARM-CLEAN06.4X.34 — vol physique : les munitions sont N0

**Parent :** `df36e7836d491b9db92f8c56f68f18af3ecbaa4f`. Rapport : [ARM_CLEAN06_4X34_CANONICAL_PROJECTILE_PHYSICAL_LEVEL.md](ARM_CLEAN06_4X34_CANONICAL_PROJECTILE_PHYSICAL_LEVEL.md).

Le lecteur de vol `FGridCanonicalProjectileVisualReadModel` rejette les non-ARM N1–N20 avant tout mesh, classe `Ammo` ou paramètre de vol, sans effacer `bCanonicalIdentity` ni modifier les lectures de famille historiques. Les ARM N1–N20 et les flèches N0 conservent leurs rendus. Test renforcé sur vrais `DA_ItemSystem` et munitions. Aucun `.uasset`, `.umap` ni SaveGame touché. **06.4 partielle ; 06.5 bloquée ; attendre deux SUCCESS Windows sur SHA final.**

## ARM-CLEAN06.4X.33 — acteurs physiques non-ARM uniquement N0

**Parent :** `d4d329415b46ff33039adace55a6fdeac1909a26`. Rapport : [ARM_CLEAN06_4X33_CANONICAL_WORLD_ACTOR_PHYSICAL_LEVEL.md](ARM_CLEAN06_4X33_CANONICAL_WORLD_ACTOR_PHYSICAL_LEVEL.md).

`AGridItemActor::InitializeFromCanonicalItem` ne peut plus matérialiser une famille non-ARM à N1–N20 malgré la compatibilité historique de `ResolveItem` pour les lectures de famille. Refus avant toute mutation d'identité, GUID, niveau ou présentation. Régression étendue au test des 34 définitions réelles : 22 ARM N1, 12 non-ARM N0 avec refus N1. Aucune écriture `.uasset`, `.umap` ou SaveGame. **06.4 partielle ; 06.5 bloquée ; attendre deux statuts Windows sur SHA final.**

## ARM-CLEAN06.4X.32 — précontrôle physique d'insertion cohérent

**Parent :** `4f1f58913b21301a8dc10edbee25727605a06ee1`. Rapport : [ARM_CLEAN06_4X32_RECEPTACLE_PHYSICAL_ACCEPTANCE_PREFLIGHT.md](ARM_CLEAN06_4X32_RECEPTACLE_PHYSICAL_ACCEPTANCE_PREFLIGHT.md).

`EvaluateItemAcceptance` refuse désormais un ARM enregistré N0 ou un non-ARM enregistré N1 dès le précontrôle de curseur, sans faux signal « accepté » avant le rejet par `AddContainedItem`. Le refus reste `InvalidItem` ; les objets réellement hors registre et les priorités `InsertionDisabled`/`Full`/`NoMatchingAcceptanceRule` conservent leur comportement (règle historique X.16.1). Aucun changement de `ResolveItem` (projections non-ARM N1 historiquement permises), aucun binaire UE ni SaveGame. **06.4 partielle ; 06.5 bloquée ; attendre les deux contrôles Windows du SHA final.**

## ARM-CLEAN06.4X.31 — condition de type dans les réceptacles ARM canoniques

**Parent :** `54f547b0802715492a8e0cb73639cbb85def501a`. Rapport : [ARM_CLEAN06_4X31_CANONICAL_RECEPTACLE_ITEM_TYPE.md](ARM_CLEAN06_4X31_CANONICAL_RECEPTACLE_ITEM_TYPE.md).

`ContainsItemType` reconnaît désormais les ARM enregistrés N1–N20 sans `ItemDefinition` : `Shield` pour les boucliers, `Weapon` pour les autres ; un niveau invalide ne lit jamais l'ancien type. Les catégories non-ARM restent historiques seulement avec un ancien ID concordant (le registre ne porte pas encore leur taxonomie gameplay). Les objets hors registre restent compatibles. Régression sur vrai registre et réceptacle transitoire ; aucun binaire ni SaveGame modifié. **06.4 partielle ; 06.5 bloquée ; vérifier les deux statuts Windows sur le SHA final.**

## ARM-CLEAN06.4X.30 — visuel de clé insérée sans ancien ItemDefinition

**Parent :** `145a97b3f217e84a796a756db61f31c6902b8bde`. Rapport : [ARM_CLEAN06_4X30_POINTERLESS_LOCK_INSERTED_KEY.md](ARM_CLEAN06_4X30_POINTERLESS_LOCK_INSERTED_KEY.md).

Une serrure peut désormais créer, insérer et afficher une clé enregistrée (`Key_Copper` N0) sans définition historique, avec mesh, Niagara, lumière, GUID et absence de collision depuis `ItemSystem`. Une clé de niveau invalide est refusée sans ouvrir la serrure. Le chemin historique est préservé tant que l'ancienne définition existe. Test de vrais acteurs sur monde transitoire ; aucun binaire UE ni SaveGame écrit. **06.4 partielle ; 06.5 bloquée ; contrôler les statuts Windows avant la suite.**

## ARM-CLEAN06.4X.29 — serrure : priorité aux IDs de clés autorisés

**Parent :** `474e622472fcb15a2f6a85f0ca0730d626a55a61`. Rapport : [ARM_CLEAN06_4X29_CANONICAL_WALL_LOCK_KEYS.md](ARM_CLEAN06_4X29_CANONICAL_WALL_LOCK_KEYS.md).

`AGridWallLockActor` accepte maintenant exactement les `AcceptedKeyIds` lorsqu'ils sont explicitement renseignés, sans union permissive avec les anciens pointeurs `AcceptedKeyItems`. Les serrures historiques sans ID explicite conservent leur branche d'origine. Test d'acteur transitoire cuivre/fer, legacy-only et IDs malformés. Aucun asset de serrure ou SaveGame écrit. **06.4 partielle ; 06.5 bloquée ; vérifier les statuts Windows exacts.**

## ARM-CLEAN06.4X.28 — palette C++ directement canonique sans ancien DataAsset

**Parent :** `0ca06dfbd351b364830e233762bf843ee87786ba`. Rapport : [ARM_CLEAN06_4X28_POINTERLESS_CANONICAL_PALETTE.md](ARM_CLEAN06_4X28_POINTERLESS_CANONICAL_PALETTE.md).

Les entrées de palette possédant `CanonicalItemId + CanonicalDefaultItemLevel` peuvent être structurées, affichées, sélectionnées, placées et rééditées **sans ancien DefaultItemDefinition**. ItemSystem valide les niveaux physiques au moment de la sélection et avant la modification de la grille ; un ARM N0, une ID inconnue ou un ancien pointeur contradictoire est refusé. Test sur palette et monde transitoires avec ARM N20 et munitions N0 ; les palettes et niveaux réels restent inchangés. **06.4 partielle ; 06.5 bloquée ; contrôler les statuts Windows du SHA.**

## ARM-CLEAN06.4X.27 — Selected Object par ItemId canonique

**Parent :** `b34eccf33e56fed6ec8feb8d759eb4399714c048`. Rapport : [ARM_CLEAN06_4X27_EDITOR_CANONICAL_SELECTED_OBJECT.md](ARM_CLEAN06_4X27_EDITOR_CANONICAL_SELECTED_OBJECT.md).

Le panneau Selected Object affiche et permet de changer un `CanonicalItemId` sans pointer vers un ancien `ItemDefinition`, avec niveau physique explicite N1–N20 pour ARM, N0 pour non-ARM. Un changement volontaire de famille conserve un niveau légal sinon applique le défaut d'authoring N1/N0 ; le recomit du même ID historique ne migre **jamais** son ancien rendu. Les changements incompatibles de quantités et les anciens pointeurs contradictoires sont refusés sans mutation. Test transitoire sur loose items et ItemSpawns. Aucun binaire Unreal ni SaveGame écrit. **06.4 partielle ; 06.5 bloquée ; attendre les deux contrôles Windows sur le SHA.**

## ARM-CLEAN06.4X.26 — validation Editor canonique et modification du niveau physique

**Parent :** fffa04d9b0a0118d4edb72b1cb77c96b61f2f4e1. Rapport : [ARM_CLEAN06_4X26_EDITOR_POINTERLESS_VALIDATION.md](ARM_CLEAN06_4X26_EDITOR_POINTERLESS_VALIDATION.md).

Le validateur de niveaux reconnaît les objets possédant CanonicalItemId sans ancien ItemDefinition, contrôle strictement leur ItemLevel et refuse les contradictions. L'édition d'ItemLevel fonctionne désormais sur loose items et ItemSpawns pointerless. Les réceptacles convertis ne déclenchent plus de fausses alertes de pointeur manquant. Test isolé du module Editor ; aucun asset UE ni SaveGame modifié. **06.4 non terminée, 06.5 bloquée.**

## ARM-CLEAN06.4X.25 — aperçu d'objets pointerless dans l'éditeur

**Parent :** `cafd5850a5e471dfc323f3002ccc7e1e98a251ff`. [Rapport X.25](ARM_CLEAN06_4X25_POINTERLESS_EDITOR_WORLD_PREVIEW.md).

Le modèle d'aperçu privilégie le `CanonicalItemId` et le niveau physique des loose items ; un ID enregistré peut être visualisé dans l'éditeur sans `ItemDefinition`. Le composant ne refuse plus ces objets avant la résolution canonique. Test acteur d'aperçu transitoire N20 et refus ARM N0/nonARM N1/ancien pointeur contradictoire/registre manquant. Les niveaux réels ne sont pas édités, la validation/Selected Object reste legacy et sera traitée séparément. **06.4 partielle, 06.5 bloquée ; contrôler les deux statuts Windows du commit.**

## ARM-CLEAN06.4X.24 — kits de départ explicitement canoniques sans ancien asset

**Parent :** `3d353420e9fb1d61403e6a6b4f7981b50e880eeb`. Rapport : [ARM_CLEAN06_4X24_POINTERLESS_CANONICAL_STARTING_KITS.md](ARM_CLEAN06_4X24_POINTERLESS_CANONICAL_STARTING_KITS.md).

`FRPGStartingEquipmentEntry.CanonicalItemId` est une nouvelle option d'authoring non activée par les presets actuels. Les entrées explicitement canoniques peuvent être accordées sans `ItemDefinition` en respectant ItemId, niveau physique, quantité, main, lumière et transaction complète, avec régression N20/N0/invalides et conflit deux mains. Les anciens kits C++ ne sont pas modifiés ; certaines recettes d'alchimie sont hors registre. Aucun asset UE/SaveGame modifié. **06.4 partielle, 06.5 bloquée ; vérifier SilentTask + LocalAuto sur le commit.**

## ARM-CLEAN06.4X.23 — torche tenue et lumière de groupe sans ancienne définition

**Parent :** `7f5f5ae4045f93e273fc904e351502dd580084eb`. [Rapport X.23](ARM_CLEAN06_4X23_POINTERLESS_HELD_TORCH_LIGHT.md).

Les non-ARM enregistrés, dont `Item_Torch` N0, peuvent maintenant produire un objet tenu et l'éclairage du groupe depuis `DA_ItemSystem` sans `UGridItemDefinitionAsset`. Régression automatique dans les deux mains et après extinction, sans modification de WBP, .uasset ni SaveGame. Les ARM encore dotés d'un ancien asset conservent leurs propriétés lumineuses historiques jusqu'à parité native. `EquipHeldItem(FName)` ne devine pas de niveau ARM. **06.4 encore partielle, 06.5 bloquée ; attendre les deux statuts Windows sur le commit exact.**

## ARM-CLEAN06.4X.22 — restauration de contenu ARM canonique sans ancien asset

**Parent :** `168594fe504af3ca8cb5489fe6534ea9fd8fca9e`. Rapport : [ARM_CLEAN06_4X22_RECEPTACLE_POINTERLESS_RESTORE.md](ARM_CLEAN06_4X22_RECEPTACLE_POINTERLESS_RESTORE.md).

La restauration d'un ARM contenu, déjà enregistré dans ItemSystem, utilise l'initialisation canonique complète **si aucun ancien ItemDefinition n'est encore résoluble**. Lorsqu'un acteur est déjà initialisé canoniquement, AddContainedItem n'y réattache plus un vieux DA_Weapon. L'ancien rendu est préservé pour les ARM avec définition toujours disponible. Test en mémoire à N20 avec GUID, niveau, lumière et propriétaire sans pointeur. Aucun asset ni SaveGame modifié. **06.4 partielle, 06.5 bloquée ; vérifier les deux validations Windows sur le SHA publié.**

## ARM-CLEAN06.4X.21 — initialisation canonique d'ARM sans pointeur dans les réceptacles

**Parent :** `476fe10700d3ace56c1416ab9b5a762ac5564cee`. Rapport : [ARM_CLEAN06_4X21_RECEPTACLE_POINTERLESS_INITIAL_ARM.md](ARM_CLEAN06_4X21_RECEPTACLE_POINTERLESS_INITIAL_ARM.md).

Un contenu initial de réceptacle explicitement canonique (ID enregistré + niveau N1–N20) peut désormais construire son acteur ARM **sans rechercher ni attacher un ancien DA_Weapon** si son `ItemDefinition` est absent. Les réceptacles actuellement authorés avec leurs pointeurs historiques **ne basculent pas**. `ContainsItemTag` privilégie les tags du registre canonique ; le fallback legacy est réservé aux identités non inscrites au registre. Test transitoire sur N1/N20/N0 et capture runtime ; aucun asset Unreal/SaveGame écrit. **06.4 en cours, 06.5 bloquée.** Attendre deux statuts Windows sur le SHA exact.

## ARM-CLEAN06.4X.20 — présentation physique ARM prête côté C++ (17 novembre 2026)

**Parent :** 0e8da96346ed897b5844a340af8e65f3ff6ef061. **Pas de modification des assets binaires ni des sauvegardes.**
Rapport : [ARM_CLEAN06_4X20_ARM_WORLD_PRESENTATION_PRECUTOVER.md](ARM_CLEAN06_4X20_ARM_WORLD_PRESENTATION_PRECUTOVER.md).

Le VisualCatalogue ARM possède désormais les champs persistants de lumière, Niagara et sparkle ; le WeaponCatalogue porte la politique physique de pickup par archétype. UGridItemSubsystem projette les deux sans copie de masse. AddPlacedItemActor accepte un ARM portant CanonicalItemId à N1/N20 sans pointeur historique, testé sur copies transitoires. **Les armes réellement authorées avec leur ancien ItemDefinition gardent leur ancienne présentation jusqu'à la migration native des 22 profils visuels et des politiques physiques des archétypes actifs ; leur comportement n'est pas basculé par ce jalon.**

**06.4 encore partielle, 06.5 bloquée.** Vérifier UE5.5.4/SilentTask et UE5.5.4/LocalAuto sur le SHA de X.20. Le prochain verrou est la preuve et migration native par Unreal des paramètres persistants, pas un nouveau gros audit Codex. L'audit X.19 et son issue #182 restent valables.

## ARM-CLEAN06.4X.19 — audit natif UE terminé sur le périmètre, sortie 06.4 toujours bloquée

Audit natif du contenu de `ae6742b1694355bc71c3a49a4b2d6f7a408fa69d` (code fonctionnel
`9e8331c1` inchangé). **06.4 NON TERMINÉ ; 06.5 BLOQUÉE.**
[Rapport complet](ARM_CLEAN06_4X19_FINAL_LEGACY_DEPENDENCY_AUDIT.md) et [issue #182](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/182).

UE 5.5.4 : 1486 packages /Game indexés, 238 DataAssets + un CDO WBP inspectés,
244 empreintes de packages inchangées avec les redirecteurs. Registre : **34 définitions
(22 ARM + 12 non-ARM)**, quatre catalogues ; **452 résolutions natives réussies**.
Les 24 DA_Weapon_* sont 22 ARM, Shuriken non-ARM et un redirecteur ThrowingKnife,
pas deux armes orphelines. Les 95 IDs/niveaux (44 loose + 18 contenus + 33 palette)
sont matérialisés mais leurs anciens pointeurs subsistent.

**Restes établis : 107 pointeurs vers les anciennes définitions dans neuf DataAssets,
44 pointeurs de catalogue dans les 22 ARM, une ancienne palette référencée par
WBP_GridCombatHudAction ; 14 définitions hors registre, dont quatre loots réels.**
Les quatre règles TorchHolder/Gardien/Cuivre/Fer sont déjà converties, avec anciens
pointeurs conservés. Trois overrides de niveau ont encore des valeurs anciennes
mais sont désactivés. Six assets de classe ont zéro kit sérialisé : presets C++.

Les placements/restaurations ARM, réceptacles, held/lumière, présentation d'attaque
et authoring exigent encore un cutover C++ avant nettoyage binaire. SaveGame v27 :
schéma confirmé, **aucun fichier personnel ouvert et aucune migration sur copies
prouvée**. Aucun ancien migrateur lancé. Preuves JSON/CSV hors dépôt :
`D:/Development/GrimrockPrototype_Audits/ARM_CLEAN06_4X19/`.

PIT02 / piège fosse reste validé par le propriétaire sur `9e8331c1` ; issue #181
clôturée selon le retour du 17.11.2026. Ce jalon ne le modifie pas et ne prétend pas
remplacer ses preuves PIE par les vérifications natives de catalogue.

L'historique X.16.13 et les sections antérieures ci-dessous sont conservés comme **historique d'incidents**, non comme état actuel.


## ARM-CLEAN06.4X.16.13 — ordre physique PIT02 et atterrissage voisin

L'utilisateur a confirmé un **ÉCHEC PIE** de X.16.12 malgré
SilentTask/LocalAuto SUCCESS sur `3b55adf4824d195925de96d06c9ad34cb869ab75` :
les armes posées sur une fosse piégée disparaissent à l'ouverture,
mais la cellule voisine d'arrivée reste vide.
**La migration ARM-CLEAN06 N'EST PAS TERMINÉE** :
06.4 encore partielle, 06.5/06.6 en attente.

X.16.13 transfère les objets **avant** la suppression du sol, reconnaît
la cellule de placement ou la position physique du mesh, utilise
son XY pour PIT02, journalise entrée/sortie et matérialisation. Le test
ARM N1 / N20 existant exige maintenant une **destination voisine (2,1)**,
le poids des deux armes à cette cellule et le bon XY avant et après
SaveGame en mémoire et changement de niveau. Aucun binaire ni .sav
personnel modifié. Voir `ARM_CLEAN06_4X1613_PIT_FLOOR_ORDER_AND_LANDING.md`.

**Validation Windows sur le SHA réel** requise, puis recette PIE ciblée
des deux armes du joueur. Ne pas déclarer X.16.13 résolu avant PIE.
Si échec, consulter les logs `GridPit TrapOpenTransfer`,
`GridPit ItemTransfer queued`, `GridPit PendingItem materialized`.
Respecter 1 commit atomique max par tâche, master seul.

## ARM-CLEAN06.4X.16.12 — correction armes ARM N1/N20 dans trappe ouverte

Déclencheur utilisateur : des armes préalablement posées sur une fosse piégée
ne réapparaissent pas au bon endroit après ouverture à l'échec d'un
désamorçage. La torche PIT02 n'est pas actuellement manipulable par le joueur.
Le scénario PIE pertinent est **armes déposées, trappe s'ouvre, armes dans
niveau inférieur, aller-retour niveau/save**, pas une torche.

Parent `738c37d380dbe13bd79e6f3eadc2abbdd13b519d` : 16.11 double validation SUCCESS.

Unification de l'initialisation des pickups canoniques au sol (ARM N1–N20
compris) via `InitializeFromCanonicalItem`, suppression des anciens
pointeurs dans la file PIT02 pour *tous* les items canoniques, restauration
complète après Save/Continue et rafraîchissement de la cellule réelle
de physique juste avant ouverture de trappe. Un avertissement explicite
est émis si le transfert échoue. Régression nouvelle :
`Grimrock.RPG.ARM.CLEAN06_4X1612.TrapOpensUnderTwoWeaponsRealLevels`
(sword N1 + throwing knife N20, identités GUID, meshes, descente,
SaveGame mémoire et aller-retour). Aucun binaire UE ni .sav modifié.

**Ne déclarer le correctif vérifié que sur deux statuts SUCCESS au SHA
exact.** La validation visuelle PIE des armes posées sur la trappe par
l'utilisateur reste indispensable ; ne pas proposer un test de torche
impossible à exécuter actuellement.

Lire `ARM_CLEAN06_4X1612_REAL_ARM_PIT_RESTORE.md`.

## ARM-CLEAN06.4X.16.11 — PIT02 canonique non-ARM et refus du fallback ARM N0

X.16.10 `afecaacf7dd0f0f6d81da358d0cd624aea3bc4a2` est désormais **validé** :
`UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS`.

X.16.11 (en cours de validation sur son SHA publié) ferme le fallback
legacy lors du routage d'un ItemId enregistré mais invalide à son niveau,
n'enregistre plus `FGridPendingWorldItemState.ItemDefinitionAsset` pour
les non-ARM canoniques et ignore les anciens pointeurs à l'atterrissage.
Test PIT02 sur deux niveaux temporaires + round-trip SaveGame en mémoire,
contrôle d'une torche N0 allumée, Niagara et refus d'ARM N0 invalide.
Les objets non enregistrés et les ARM sans cutover visuel gardent leur
voie existante. Les champs SaveGame restent présents, aucune sauvegarde
personnelle ou binaire Unreal modifié.

Lire `ARM_CLEAN06_4X1611_CANONICAL_PIT_TRANSFER.md`.
Ne déclarer X.16.11 validé qu'avec les **deux statuts SUCCESS sur son SHA**.
À la prochaine reprise : autres lecteurs legacy, conversion des ARM visuels,
protection des copies de SaveGame, 06.5 et 06.6. PIE PIT02 à effectuer
sur contenu réel pour valider lumière et Niagara de la torche après chute.

## ARM-CLEAN06.4X.16.10 — rechargement non-ARM canonique

Parent `7886a9b757b639f87d6ed8202e9d34e0d19c1a64` : SilentTask et LocalAuto SUCCESS.
Les serrures cuivre/fer ont été validées en PIE par le propriétaire.
16.10 restaure les vrais profils non-ARM (lumière et Niagara compris)
depuis le registre canonique, au sol et dans les réceptacles, sans
réacquérir les pointeurs historiques. Deux tests de SaveGame en mémoire
sont ajoutés ; aucun .sav, .uasset, .umap ni WBP écrit.
**Ne déclarer validé que lorsque SilentTask et LocalAuto sont SUCCESS sur
le SHA de 16.10.** Lire ARM_CLEAN06_4X1610_CANONICAL_WORLD_RESTORE.md.
Ensuite : derniers lecteurs, copies SaveGame, cutover ARM, 06.5, 06.6.

## Reprise après validation PIE du 16 novembre 2026 — ARM-CLEAN06.4X.16.9

Le propriétaire confirme avoir testé et validé en PIE les serrures cuivre/fer
après la migration binaire 16.8.5. Ne pas refaire cette recette ni revenir
aux anciennes étapes X.16.8.1–.4.2.

16.9 ajoute une régression **strictement en mémoire** des SaveGames courants
(v27 exact-match), pour les identités d'objets, niveaux N0/N20,
quantités, états de réceptacles/serrures/portes et transferts PIT.
Lancer les statuts automatiques Windows sur le SHA **réel** de 16.9
avant de considérer cette régression validée.

**Aucun** fichier .sav personnel, .uasset ou carte n'est modifié par 16.9.
Les deux champs legacy de SaveGame restent présents volontairement.
Ensuite : revue des derniers lecteurs historiques et copies isolées de
SaveGames v27, puis seulement 06.5 et 06.6. Lire
ARM_CLEAN06_4X169_SAVEGAME_MEMORY_ROUNDTRIP.md.

## Reprise actuelle — ARM-CLEAN06.4X.16.8.5 VALIDÉ (serrures cuivre et fer)

**Dernier commit fonctionnel validé :**
`da9c6c046f1fdc0322274484c995555a8b90a5ca`.
`UE5.5.4/SilentTask=SUCCESS` et
`UE5.5.4/LocalAuto=SUCCESS` sur le **SHA binaire final**.

**Parent de la transaction native :**
`871c97d5046720c9ac3d664ba73b2e92a17429e2`.
Trois statuts SUCCESS :
`UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
`UE5.5.4/ARM-CLEAN06.4X83`. L'agent Windows a
effectué le préflight, les sauvegardes SHA-256
avec provenance Git LFS, l'écriture Unreal,
la réouverture dans un second processus, la
suite complète Grimrock et le push d'un commit
binaire enfant contenant **seulement deux fichiers** :

- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset` :
  `DefaultBehavior.Lock.AcceptedKeyIds=[Key_Copper]`.
- `Content/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset` :
  `DefaultBehavior.Lock.AcceptedKeyIds=[Key_Iron]`.

Les pointeurs `AcceptedKeyItems` restent intacts.
Le runtime `AGridWallLockActor` calcule une union
`AddUnique` des deux listes ; **les seules clés
admises restent les mêmes**, sans modification de
`bConsumeKeyOnUnlock`, `bStartsUnlocked`,
messages ou liens logiques. Aucune carte, aucun
réceptacle et aucune sauvegarde personnelle
n'a été modifié dans cette transaction.

**Jalons intermédiaires :**
- X.16.8.1 `b5a8584960275aefe90e4fb4b47587fbbf64de71` :
  audit read-only des vraies serrures, deux statuts verts.
- X.16.8.2 `85a06d65d083cbeff76d39f2be1953bbc61827aa` :
  l'audit historique des réceptacles reconnaît
  précisément l'état canonique des deux serrures ;
  deux statuts verts.
- X.16.8.3 `148d1213aa969af0b4c28871806d45497f170b85` :
  helper de migration natif désarmé, deux statuts verts.
- X.16.8.4 `eda5a72d807e820b5393aed4b1c8b69a4eda59b9` :
  premier déclenchement arrêté en phase
  `POST_AUTOMATION_FAILED`, sans commit binaire.
  Les vieux audits TD073 refusaient tous les
  `AcceptedKeyIds` non vides.
- X.16.8.4.1 `2455d2ba7dcd16a71f11e5c55ad9a2a9a1619688` :
  correction très ciblée du schéma TD073,
  uniquement pour les deux clés canoniques
  concordantes ; deux statuts verts.
- X.16.8.4.2 `871c97d5046720c9ac3d664ba73b2e92a17429e2` :
  transaction protégée complète, trois statuts verts.
- X.16.8.5 `da9c6c046f1fdc0322274484c995555a8b90a5ca` :
  deux `.uasset` natifs et double validation
  UE5.5.4 sur leur SHA réel.

**Les quatre références historiques recensées
dans l'issue GitHub #176 sont maintenant persistées
canoniquement dans leurs DataAssets** :
TorchHolder/Item_Torch, Guardian/Gem_Blue,
CopperWall/Key_Copper et IronWall/Key_Iron.
Leurs anciens pointeurs sont conservés.
La recette PIE des deux réceptacles est
**validée par l'utilisateur** ; les serrures
restent à tester en PIE. Lire
`ARM_CLEAN06_4X85_LOCK_PIE_HANDOFF.md`.

**Prochaine intervention utilisateur :** mettre à jour
le `master` local propre et Git LFS après fermeture
d'Unreal, recompiler, tester l'ouverture des serrures
cuivre/fer avec leur bonne clé et le refus de la mauvaise,
les messages/porte et la consommation selon leur
configuration existante. Tester Save/Continue
uniquement sur copies privées.

**Suite technique après validation PIE :**
auditer les lecteurs historiques restants,
les SaveGames sur copies, les ARM physiques
et les tests Cook ; ne supprimer les pointeurs
que lorsqu'aucun lecteur ni sauvegarde ne
les nécessite. Ne pas rejouer 06.4W/X.5
ou les déclencheurs historiques X.16.7.2.

La section suivante décrit **l'ancien point X.16.7.3**,
pas le HEAD courant.

## Reprise actuelle — ARM-CLEAN06.4X.16.7.3 VALIDÉ (deux réceptacles natifs)

**Dernier commit fonctionnel validé :**
`d3ecdf6bb19b249c891db9e90e87413dbf3bc118`.
Sur ce SHA binaire final, `UE5.5.4/SilentTask=SUCCESS` et
`UE5.5.4/LocalAuto=SUCCESS`.

**Parent de la transaction :**
`ca32a56d825fa1f9e172556d258475a069ca1395`,
avec `SilentTask`, `LocalAuto` et
`UE5.5.4/ARM-CLEAN06.4X72` tous SUCCESS.
L'agent Windows a appliqué la transaction dans son
worktree isolé après sauvegardes SHA-256,
contrôle Git LFS, relecture par un deuxième
processus UE et tests Grimrock complets.

**Deux seuls fichiers natifs modifiés et publiés :**
- `Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset` :
  `DefaultBehavior.Receptacle.AcceptedItems[0].CanonicalItemId=Item_Torch`.
- `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset` :
  `DefaultBehavior.Receptacle.AcceptedItems[0].CanonicalItemId=Gem_Blue`.

Le diff final vérifié par GitHub contient exactement ces
deux `.uasset` (commit enfant par avance rapide).
Les anciens `ItemDefinition` restent disponibles
pour les lecteurs en transition. Ni niveau, ni map,
ni serrure, ni SaveGame personnel, ni WBP modifié.

**Historique d'incidents d'automatisation :**
les premiers déclencheurs X.16.7.2B/C/D ont échoué
à cause de substitutions incorrectes dans
`Scripts/ValidateUE.ps1` et du nom de fonction
PowerShell `Git` masquant `git.exe`.
Le SHA final X.16.7.2F a corrigé ces erreurs, et
la transaction plus le SHA binaire ont été validés.
**Ne pas rejouer** les anciens déclencheurs ni les
migrations W/X.5. Ne plus réécrire le volumineux
`Scripts/UEValidationAgent.ps1` par des
substitutions textuelles non contrôlées.

**Intervention PIE nécessaire maintenant** :
fermer Unreal, synchroniser `origin/master`
dans le checkout de développement propre,
`git lfs pull origin master`, recompiler.
Vérifier sur une session PIE du niveau :
1. support de torche : insertion, extraction,
   lumière allumée au sol et reprise en main ;
2. Gardien : insertion de `Gem_Blue` acceptée,
   mauvais objet refusé, énigme et liens inchangés ;
3. aucun changement dans les interactions
   inventaire, clés ou combat.
Ne pas écraser de sauvegardes personnelles.

**Ensuite seulement** : X.16.8, migration native
séparée de `DA_Lock_CopperWall` (Key_Copper)
et `DA_Lock_IronWall` (Key_Iron), avec contrôles
et backups indépendants. Aucune migration de clé
n'a été effectuée à ce jour.

Lire `ARM_CLEAN06_4X73_RECEPTACLE_PIE_HANDOFF.md`.
La section suivante est l'historique X.16.6.3,
non le HEAD actuel.

## Reprise actuelle — X.16.6.3 : quatre packages exacts identifiés

Le test X.16.6.1 a été validé sur db4d5114b068.
La publication filtrée X.16.6.2 est validée sur
6c3c72e24083 : SilentTask, LocalAuto et le
contrôle de publication sont tous SUCCESS.

Preuve nominative : issue GitHub #176 :
https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/176

Quatre sources, dans quatre DataAssets distincts :
- DA_Receptacle_TorchHolder : AcceptedItems[0] = Item_Torch
- DA_Guardian : AcceptedItems[0] = Gem_Blue
- DA_Lock_CopperWall : AcceptedKeyItems[0] = Key_Copper
- DA_Lock_IronWall : AcceptedKeyItems[0] = Key_Iron

ARM_CLEAN06_4X1663_EXACT_ACCEPTANCE_PACKAGE_GATE.md
établit le manifeste des quatre fichiers Git LFS
et deux transactions binaires distinctes, avec
copies SHA-256, réouverture UE et tests de régression.

Aucune modification des vrais DataAssets, niveaux,
SaveGames ou Blueprints n'a encore été faite.
Ne pas relancer les 95 conversions 06.4W/X.5.

## Reprise actuelle — X.16.6.1 : manifeste exact des 4 sources

Le rapport réel Windows fourni par l'utilisateur confirme :
- 44 définitions et 4 niveaux ; aucun override de réceptacle ou serrure ;
- 6 listes de réceptacles, toutes sûres ; exactement 2 règles à migrer ;
- 2 listes de serrures, toutes sûres ; exactement 2 pointeurs à migrer ;
- aucun ID déjà canonique, aucune référence absente, inconnue
  ou contradictoire dans ces quatre sources.

X.16.6.1 ajoute un test read-only enregistrant nom de
package/DefinitionId/ItemId de chaque référence et
bloquant toute dérive des comptes. Les vrais assets restent
intacts. Lire ARM_CLEAN06_4X1661_REAL_ACCEPTANCE_ASSET_MANIFEST.md.

Une conversion binaire exige d'abord ces noms, un manifeste
de packages exact, des copies SHA-256 et une vérification
indépendante. Ne pas utiliser de noms de fichiers supposés.

## Reprise actuelle — ARM-CLEAN06.4X.16.5 VALIDÉ (règles d'énigmes)

Dernier SHA fonctionnel validé :
f12487b61dd099cb3dcd9240c6e696ee07e3c651.
UE5.5.4/SilentTask=SUCCESS et UE5.5.4/LocalAuto=SUCCESS.

Retour PIE : la torche reste allumée lors du dépôt au sol,
du ramassage et du rééquipement (X.18 validé manuellement).
Les objets, l'inventaire, le combat, les réceptacles et les
énigmes existants ont également été contrôlés dans les jalons précédents.

- X.16.4 : 50e9f628d844f96f187f72cca38c3a2d03a9f6bd.
  Préflight atomique en lecture seule des AcceptedItems des
  définitions de réceptacles et de leurs surcharges des quatre niveaux.
  Une référence absente, inconnue ou contradictoire interdit
  de convertir la liste entière.
- X.16.5 : f12487b61dd099cb3dcd9240c6e696ee07e3c651.
  Préflight indépendant des AcceptedKeyItems et AcceptedKeyIds
  de serrures, en préservant tous les IDs explicitement authorés,
  même ceux des clés narratives extérieures au registre.
  L'ensemble des clés acceptées est inchangé avant/après.
- Les deux jalons ont passé les deux contrôles UE Windows.
  Les vrais DataAssets, .umap, WBP, SaveGames et liens
  d'énigme ne sont pas réécrits par ces préflights.

ATTENTION : une suite d'automatisation verte n'atteste pas
que toutes les références sont convertibles. Les décomptes
des listes sûres et bloquées ne figurent que dans
les journaux UE de l'agent Windows, non dans les
statuts GitHub.

La porte de migration effective est documentée dans
ARM_CLEAN06_4X166_ACCEPTANCE_MIGRATION_GATE.md.
La prochaine intervention nécessaire est la récupération
des deux lignes de compteurs anonymisées de ce rapport.
Ne pas lancer une conversion des vrais assets avant de
connaître ces nombres et de disposer d'un manifeste
de packages, de copies SHA-256 et d'une vérification
par réouverture UE indépendante.

Travail futur : migrer les règles de réceptacles et de
serrures séparément, valider refus/acceptation et énigmes
en PIE, puis considérer les derniers lecteurs historiques,
SaveGames sur copies et suppression conditionnelle du legacy.
Ne jamais relancer les migrations 06.4W et 06.4X.5.

Le chapitre suivant est historique (X.18).

## Reprise actuelle — ARM-CLEAN06.4X.18 VALIDÉ (torche au sol lumineuse)

**Dernier jalon fonctionnel validé :**
`76edb1aec65aa0e524868ea26d14ceea3ac0fd43`,
`UE5.5.4/SilentTask=SUCCESS` et
`UE5.5.4/LocalAuto=SUCCESS` sur ce SHA exact.

**PIE utilisateur après X.17.1 :** la torche s'équipe et
se déplace désormais entre support, sac et mains,
mais **elle s'éteignait en étant déposée au sol et
restait éteinte après ramassage**.

**Cause vérifiée dans le code :**
`TryDropItemInstanceAtCell` utilisait
`InitializeFromItemDefinitionId` pour le
non-ARM canonique, qui efface
`FGridLightEmitterConfig` et
`VisualEffects`; `ApplyCanonicalWorldVisual`
n'appliquait ensuite que le mesh. Une tentative
`SetItemLightsEnabled(true)` sans configuration
échouait silencieusement. La capture au ramassage
transformait alors l'état allumé en éteint.

**X.18 :** un objet non-ARM du registre au sol
est maintenant initialisé avec
`InitializeFromCanonicalItem` et reçoit
tout son profil lumineux/Niagara/physique
avant que son `bLightsEnabled` soit restauré.
Échec atomique d'initialisation, sans
fallback vers le vieux DataAsset.
Les ARM et identités non enregistrées conservent
leur chemin historique dans ce jalon.

**Test automatique X.18** : vraie torche
`DA_Item_Torch`, état au sol allumé, capture
de l'état du niveau, ramassage avec GUID
inchangé, sac, `MainHand` et
`UGridPartyIlluminationComponent` allumé.
Une seconde torche explicitement éteinte
reste éteinte au sol et après ramassage.
Les **deux contrôles UE5.5.4 sont verts**.

**Contrat de conception :** une manipulation
ne doit pas changer l'état lumineux. Support
et sol éclairent localement si allumés ;
sac/curseur conservent l'état sans émettre
de lumière ; une main équipée alimente
l'éclairage du groupe si la torche est allumée.
Pas d'extinction automatique au sol.
Aucune usure ni action explicite d'allumage
nouvellement créée ; ce seront des tickets
distincts. Lire
`ARM_CLEAN06_4X18_TORCH_LIGHT_STATE_CONTRACT.md`.

**Recette PIE non encore validée par l'utilisateur :**
sur la dernière version compilée, recommencer
depuis une nouvelle torche allumée (une ancienne
instance déjà éteinte conserve volontairement
cet état), puis tester support -> sac -> main
-> sol -> sac -> main -> support, flamme et
éclairage visibles à chaque emplacement.
Sur une copie uniquement pour tester
Save/Continue ; ne pas modifier les vrais
niveaux ni les `.sav` personnels.

**Suite après PIE** : X.16.4 migration
transactionnelle des vraies règles
`AcceptedItems` des réceptacles,
puis `AcceptedKeys` de serrures,
en deux chantiers séparés.
Ne pas relancer les 95 migrations physiques.

La section suivante est l'historique X.17.1,
pas le point de reprise courant.

## Reprise actuelle — ARM-CLEAN06.4X.17.1 VALIDÉ (torche rééquipable)

**Dernier SHA fonctionnel validé :**
`a33247db90cf05029f542718b04530958869dbaa`
(`UE5.5.4/SilentTask=SUCCESS` et
`UE5.5.4/LocalAuto=SUCCESS`).
Parent fonctionnel X.17 :
`fe5d05a44f5dd8cd3c0d94cf7cf3544f3ede4ff1`
(les deux contrôles également SUCCESS).

**Régression rapportée en PIE :** une torche se détache et se
réinsère dans le réceptacle, mais après ramassage dans le sac
elle ne peut plus être mise en main principale/secondaire ;
le groupe se retrouve dans l'obscurité. Armes, potions,
flasques et combats préalablement validés.

**Cause C++ confirmée :** le cutover 06.3G refusait
indistinctement **tout non-ARM canonique** dans
`CanEquipItemToSlot` et
`FGridCanonicalEquipmentSlotsReadModel`.
Pourtant les `UGridOtherEquipmentDefinitionAsset`
portent déjà `EquipmentSlots` et `HandPolicy`
canoniques. Torch est un non-ARM équipable, non une
arme de progression N1–N20.

**Correction X.17 :** le `UGridItemSubsystem` projette
`NonArmEquipmentSlots`/`bNonArmTwoHanded`
depuis le seul registre. Equipement direct,
curseur, menus contextuels et tooltips lisent ces
permissions ; les munitions et consommables
restent non équipables et les non-ARM restent N0.
L'identité, le niveau, le GUID et `bLightsEnabled`
sont préservés pendant l'équipement.

**Test complémentaire X.17.1 :** véritable
`DA_Item_Torch`, main principale et secondaire,
conservation de GUID et de l'état lumineux, plus
`UGridPartyIlluminationComponent` :
`ActiveSourceId` correspond à la torche et le
PointLight existe avec intensité/rayon canoniques.
Les deux commits sont **validés UE 5.5.4**.

**Recette PIE à confirmer par le propriétaire :**
sur une copie de niveau, détacher la torche du support,
la ramasser, l'équiper dans chacune des deux mains,
vérifier le retour de l'éclairage et de la flamme,
la retirer puis la remettre sur le support. Contrôler
également le glisser-déposer sac/curseur. Ne pas
modifier les vrais niveaux ni les `.sav` personnels.
La validation -NullRHI n'est pas une preuve du rendu PIE.

**À poursuivre après recette :** migration transactionnelle
des règles historiques `AcceptedItems` et
`AcceptedKeyItems` réellement enregistrées,
en séparant réceptacles et serrures ; puis
ARM physiques, sauvegardes sur copies et
suppression conditionnelle des anciens pointeurs.

Le point ci-dessous décrit l'état **historique X.16.3**,
et ne constitue plus le HEAD fonctionnel courant.

## Reprise actuelle — ARM-CLEAN06.4X.16.3 VALIDÉ

**Dernier jalon fonctionnel validé : ARM-CLEAN06.4X.16.3**
`2952e852e9e4ee633fc3f42f66fbcfd5fe6bf1cb`.
Vérifié sur GitHub le **16 novembre 2026** :
`UE5.5.4/SilentTask=SUCCESS`,
`UE5.5.4/LocalAuto=SUCCESS`.

L'agent de validation Windows a également confirmé :
- **X.16.1** `d5a2dbdd4dd1b4c921aaf53473fb78e8602778a4` :
  champ `CanonicalItemId` sur les règles de réceptacle,
  lecture stricte des identités persistées, compatibilité
  des règles existantes sans champ canonique ;
- **X.16.2** `adaf7f6cf6fed2eb4d5cc86feb6d922846739151` :
  choix dans Selected Object et copie d'une règle
  Definition vers une surcharge d'instance, avec
  écriture canonique des identités déjà reconnues ;
- **X.16.3** `2952e852e9e4ee633fc3f42f66fbcfd5fe6bf1cb` :
  correction de l'inspecteur de contenu initial,
  `CanonicalItemId` et niveau physique changés ensemble,
  pas de N0 ARM ni de N1 non-ARM.

Les trois jalons sont **validés par les deux contrôles UE**.
Ils n'ont modifié **aucun véritable `.uasset`, `.umap`,
`.sav`, Blueprint ou WBP**. Les vrais tableaux historiques
`AcceptedItems` de réceptacles et
`AcceptedKeyItems` de serrures n'ont **pas encore été
convertis** : leur migration est une opération future distincte.

**Retour utilisateur PIE confirmé** : palette,
recherche, sélection/édition d'objets, ramassage des
armes, potions et flasques, combats, réceptacles et
énigmes existantes. Cela n'est **pas** une recette
manuelle de l'édition des *nouvelles* règles
canoniques ni des *nouveaux* contenus initiaux.
La recette ciblée est dans
`ARM_CLEAN06_4X163_MANUAL_EDITOR_ACCEPTANCE_RECIPE.md`.

### Jalons antérieurs acquis (ne pas rejouer)

| Jalon | SHA validé | Résultat |
|---|---|---|
| X.9.2 | `3a6832d918fb` | Présentation initiale non-ARM canonique dans les réceptacles |
| X.10 | `62dbfa08dea9` | ItemSpawns typés, niveaux physiques stricts ; 0 occurrence réelle |
| X.11 | `56f09ecdaab2` | Audit des 33 entrées réelles de palette |
| X.12 | `4f6ed4c4d6eb` | Placement/édition enregistrant ItemId et niveau physique |
| X.13 | `d330d613d175` | Parité des noms, descriptions, icônes |
| X.14 | `d9595b630837` | Affichage et recherche Slate canoniques |
| X.15 | `8b9d14d8e649` | Inventaire read-only des règles de réceptacles et serrures |
| X.16.1 | `d5a2dbdd4dd` | Lecture canonique fail-closed des AcceptedItems |
| X.16.2 | `adaf7f6cf6fe` | Écriture canonique par l'inspecteur des AcceptedItems |
| X.16.3 | `2952e852e9e4` | Correction identité/niveau physique des contenus initiaux |

**Registre : 34 définitions (22 ARM + 12 non-ARM).**
Occurrences réelles déjà migrées : **95**
(44 loose items, 18 contenus initiaux, 33 palette,
0 ItemSpawn). Ne **pas** rejouer 06.4W et X.5.

### Prochain chantier à reprendre — X.16.4

1. Lire le test `RealPuzzleAcceptanceSourcesReadOnly`
   de X.15 et **obtenir les comptes concrets** des
   règles de définition et des surcharges des quatre
   niveaux ; les statuts GitHub n'exposent pas les
   valeurs des journaux UE Windows privés.
2. Préparer une migration des seules règles
   réellement utilisées : `AcceptedItems`
   et `AcceptedKeys` à traiter **séparément**.
   Les sources absentes/inconnues et les règles
   historiques explicites nécessitent une
   qualification, sans hypothèse ni fallback.
3. Réserver l'écriture des DataAssets à une
   transaction dédiée, avec sauvegarde de copies,
   contrôle des hashes/relecture des vrais assets,
   preuve de non-régression des liens d'énigmes,
   puis validation UE sur le SHA final.
4. Ensuite seulement, traiter les ARM physiques,
   les autres lecteurs historiques, SaveGames sur
   copies privées, PIE/Chaos/Niagara et Cook.
   Aucune suppression des vieux pointeurs
   avant preuve de non-utilisation.

**Règles :** `master` uniquement,
un jalon = un commit + push, contrôles UE
SUCCESS sur le SHA final, aucun changement
de niveaux ou sauvegardes personnels non
autorisé. Cette section est le point courant ;
les sections suivantes sont **historiques**.
Toujours relire le HEAD réel et les statuts
en reprenant après interruption.

## Historique vérifié — point antérieur ARM-CLEAN06.4X.9.1

**Vérifié sur GitHub, 16 novembre 2026.**
Projet UE **5.5.4** : `PhilippeMartin1964/GrimrockPrototype`,
branche `master` exclusivement. Le HEAD fonctionnel actuel,
avant ce rafraîchissement documentaire, est :

`d345268e8f2bc23c7ee1aaf9992a2071c317eed8` —
**ARM-CLEAN06.4X.9.1 : identité canonique des contenus initiaux
de réceptacles**. Les contrôles `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` sont tous deux **SUCCESS** sur ce SHA.

### Jalons acquis : ne jamais rejouer les migrations binaires

| Jalon | Commit final validé | Résultat |
|---|---|---|
| **06.4W** | `a56f4b5c22a7` | 95 `CanonicalItemId` enregistrés dans quatre niveaux et une palette ; trois contrôles UE SUCCESS |
| **06.4X.4** | `0a79e8a11368` | préflight des 95 niveaux réels |
| **06.4X.5** | `c395ebb53891` | cinq vrais `.uasset` convertis aux niveaux physiques, trois contrôles UE SUCCESS |
| **06.4X.6** | `68ef913cad26` | projection lumière, Niagara, lecture, scintillement, physique ; deux SUCCESS |
| **06.4X.7** | `28c0426982d3` | `AGridItemActor::InitializeFromCanonicalItem`, testé sur les 34 familles ; deux SUCCESS |
| **06.4X.8** | `688f54c45d98` | objets libres non-ARM basculés vers l'acteur canonique ; ARM visuels conservés ; deux SUCCESS |
| **06.4X.9.1** | `d345268e8f2b` | 18 contenus initiaux de réceptacles résolus par identité et niveau persistés ; deux SUCCESS |

**Registre natif : 34 définitions = 22 ARM + 12 non-ARM**,
dont les flèches préexistantes (en plus des 11 identités
non-ARM migrées). Les quatre niveaux et la palette contiennent
**95 occurrences** : 44 objets libres, 0 spawns réels,
18 contenus initiaux de réceptacles et 33 entrées directes de
palette. Ces chiffres comptent des placements, pas des familles.
La palette stocke maintenant `CanonicalDefaultItemLevel`.
La migration X.5 a matérialisé **45 références ARM et 50 non-ARM** ;
23 références ARM du monde ont reçu leur vrai niveau
physique N1–N20 depuis leur source authorée (jamais N1
inventé), et les non-ARM conservent N0.

**Prochaine étape ciblée : ARM-CLEAN06.4X.9.2** — faire
utiliser `InitializeFromCanonicalItem` à la présentation
physique des **items non-ARM initialement contenus dans
les réceptacles**, sans modifier les règles d'acceptation,
les emplacements, la logique des énigmes ni les données de
persistance. Lire :
`docs/Architecture/ARM_CLEAN06_4X91_RECEPTACLE_INITIAL_IDENTITIES.md`,
`docs/Architecture/ARM_CLEAN06_4X8_REAL_LOOSE_ITEM_CUTOVER.md`,
`docs/Architecture/ARM_CLEAN06_4X7_CANONICAL_ACTOR_INITIALIZATION.md`.

Ensuite : cutover des spawns (zéro instance réelle),
des **33 entrées de palette**, des règles d'acceptation
des réceptacles, des équipements ARM et de l'éditeur.
Les SaveGames seront migrés et vérifiés **sur copies**
avant la suppression des anciens pointeurs. Ne créer
aucun nouveau fallback runtime vers les anciens assets
pour une identité canonique connue. Garder les GUID,
quantités, lumière, Niagara, Chaos, textes lisibles,
links logiques et règles d'énigmes strictement identiques.

**Limite de preuve :** compilation et suite `Grimrock`
en `-NullRHI` ne prouvent pas PIE/UMG/Chaos dynamique,
Save/Continue ni Cook/Shipping. Ne pas annoncer ces
recettes comme validées. Pour tout jalon :
**un commit fonctionnel = une étape** sur `master`,
validation UE réelle sur le SHA final, aucun changement
des `.sav`, `.umap`, WBP sauf autorisation dédiée.
Toujours relire le HEAD GitHub et les statuts à la reprise.

### Jalons récents déjà réalisés — ne pas les refaire

| Jalon | Portée | Commit |
|---|---|---|
| 06.3AV | Vols ARM spécialisés canoniques | `bf444f0` |
| 06.3AW | Interactions, lecture, raccourcis | `96e02fa` |
| 06.3AX | `Use / Self / Heal` | `4f78087` |
| 06.3AY | `Use / Self / RemoveStatus` | `b12ad68` |
| 06.3AZ | `Use / Self / ApplyStatus` | `d486a2e` |
| 06.3BA | `Use / Ally`, clic portrait, transaction ciblée | `3c537bc` |

Documents de détail :
- `docs/Architecture/ARM_CLEAN06_3_REMAINING_LEGACY_READERS_AUDIT.md`
- `docs/Architecture/ARM_CLEAN06_3BA_CANONICAL_ALLY_USE_TARGETING.md`
- `docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`
- `docs/Architecture/ARM_CLEAN06_MIGRATION_PLAN.md`

## Avancement 06.3BB — munitions canoniques

Le jalon 06.3BB ajoute la résolution unique des exigences de munitions,
les préflights partagés HUD/exécution et la porte de spawn canonique
des flèches N0. Lire `ARM_CLEAN06_3BB_CANONICAL_RANGED_AMMUNITION.md`
et vérifier les deux statuts de validation **sur le SHA fonctionnel BB**
avant tout verdict. Les paramètres visuels des flèches conservés en
transition et la recette PIE restent hors du cutover 06.4.

## Validation fonctionnelle encore nécessaire pour BA

L'Automation vérifie les projections d'actions `Heal`,
`RemoveStatus`, `ApplyStatus`, leur sélection `Ally` et leurs
préflights. **PIE du WBP réel non vérifié.** Recette visuelle ciblée :

1. Dans un combat, lanceur à PV maximum, allié blessé ; sélectionner
   le raccourci `Use / Ally / Heal` sans consommer l'objet.
2. Cliquer sur le **portrait de l'allié**. Vérifier priorité du clic UMG
   sur le clic monde, cible correcte, soin et **une seule** unité consommée.
3. Répéter pour `RemoveStatus` et `ApplyStatus`, contrôler
   résultat, refus d'effet inutile, cooldown et son.
4. Vérifier Échap, fin de tour, cible morte et inventaire ouvert :
   pas d'exécution parasite, pas de dépense lors d'un refus.

Ne pas présenter une simulation `-NullRHI` comme une preuve du
hit testing du `UGridCombatActionPanelWidget` dans son WBP binaire.

## ARM-CLEAN06.3BB — réalisé et validé

- SHA : `69f122fcd895147eef55fe64f93dbefe29147619`
- `UE5.5.4/SilentTask=success`, `UE5.5.4/LocalAuto=success`.
- Disponibilité HUD et combat : munitions canoniques via `RequiredAmmoKindId`
  de l'arme, `AmmoKindId` de `Item_Arrows_01`, débit unitaire,
  vrai projectile récupérable. Pas de conversion binaire.

## ARM-CLEAN06.3BC — continuité des projectiles au Save/Continue

- Voir `ARM_CLEAN06_3BC_CANONICAL_ARROW_PERSISTENCE.md`.
- Capture d'une flèche canonique en vol ou fichée : aucun ancien
  `ProjectileDefinitionAsset` sauvegardé.
- Restauration : flèche N0 enregistrée résolue depuis `DA_ItemSystem` ;
  le monstre vivant conserve la flèche fichée, sinon un pickup réel
  conserve le GUID et la pose. Aucun fallback vers un ancien DA_Item
  pour un ItemId enregistré mais invalide.
- Le résultat du `LocalAuto` doit être contrôlé sur le SHA BC final,
  et le rendu PIE/Chaos reste une validation distincte.

## ARM-CLEAN06.3BD — identité canonique monde/pickup

- Le résolveur de world item est désormais commun au clic,
  ramassage par cellule et capture SaveGame.
- ItemId/ItemLevel connus enregistrés : aucune définition ancienne
  ne peut renommer un objet, ni corriger silencieusement un niveau
  invalide ; les identités inconnues restent compatibles.
- Tests de capture et ramassage effectifs sur le registre réel.
- Lire `docs/Architecture/ARM_CLEAN06_3BD_CANONICAL_WORLD_PICKUP_IDENTITY.md`.
- Vérifier les statuts Windows **sur le SHA BD** avant de déclarer
  la phase validée ; PIE/Chaos non couvert par l'Automation.

## ARM-CLEAN06.3BE — lancers physiques en combat

- Le moteur de combat utilise `FGridCombatPhysicalThrowSourceReadModel` :
  seul le niveau réel du projectile canonique et **le coût unitaire authoré**
  permettent le jet de l'arme équipée ou en raccourci.
- Le vieux `bCombatThrowWeapon` n'est plus l'autorité pour les objets
  enregistrés dans le registre ; il reste transitoire hors registre.
- Un niveau N0/N21 ARM connu ou un coût incompatible refuse le combat
  **avant les PA**. Ni vitesse ni action inventées.
- Consulter `ARM_CLEAN06_3BE_COMBAT_PHYSICAL_THROW.md` et contrôler
  les deux checks Windows du SHA fonctionnel BE avant verdict.

## ARM-CLEAN06.3BF — menu et Pawn cohérents sur les jets physiques

- Un seul `FGridPhysicalThrowPreflightReadModel` compose les lectures
  Force/poids et vitesse/arc pour le menu « Lancer » et le Pawn.
- Les balistiques historiques absentes ou non finies n'autorisent
  plus l'action ; les vrais jets N1–N20 restent purement canoniques.
- Aucune vitesse arbitraire pour les jets génériques restant à
  convertir en 06.4. Sans conversion binaire ni texte WBP nouveau.
- Lire `ARM_CLEAN06_3BF_SHARED_UTILITY_THROW_PREFLIGHT.md` et
  vérifier les deux statuts Windows sur le SHA BF publié.

## ARM-CLEAN06.3BG — réceptacles et inventaire de conversion

- Les réceptacles n'utilisent plus `DA_Item_*` pour renommer un
  `ItemId` déjà enregistré dans `DA_ItemSystem`.
- Capture : contrôle de niveau, GUID et accord de l'acteur physique ;
  les anciens contenus initiaux sans `ItemId` doivent être migrés en 06.4.
- Les trois tests `ReceptacleIdentity` couvrent insertion N0,
  niveau ARM N20, capture et conflit actor/ItemId.
- Inventaire de passage vers 06.4 :
  `docs/Architecture/ARM_CLEAN06_3BG_RECEPTACLE_AND_064_READINESS.md`.
- Ne déclarer BG validé qu'après **les deux statuts** du SHA publié.
  La conversion binaire et le PIE restent ouverts.

## ARM-CLEAN06.4A — recapture read-only des vrais contenus UE

- Après UE5.5.4 Build + Automation `Grimrock` réussis, l'agent Windows
  reconnaît **uniquement** le sujet du commit 06.4A et relance le
  commandlet existant `GridArmClean06Baseline`.
- Nouveau contexte `UE5.5.4/ARM-CLEAN06.4A` indépendant de
  `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.
  Exiger **les trois SUCCESS** sur le SHA de 06.4A.
- Manifeste UE privé (UPROPERTY et AssetRegistry), SHA-256 des 56
  binaires, preuves des 1640 profils/260 passifs, et SHA-256 du
  vrai `DA_ItemSystem` ; sauvegardes uniquement inventoriées
  par empreinte locale et jamais diffusées.
- Un ticket GitHub `[ARM-CLEAN06.4A]` publie uniquement les nombres
  et l'empreinte du manifeste, sans chemins personnels ni données.
- **Aucun** asset Unreal, SaveGame ou contenu de niveau n'est modifié :
  ce point est un **préflight avant** la migration binaire.
- Lire `docs/Architecture/ARM_CLEAN06_4A_READ_ONLY_ASSET_RECAPTURE.md`.
  Contrôler le ticket et les checks avant de décider 06.4B.

## ARM-CLEAN06.4B — comparaison privée des captures UE

- Le commit 06.4B déclenche, après Build et suite Grimrock, la
  comparaison des deux captures UE réelles (06.0 SHA c66bf68 et
  06.4A SHA 27563be), leurs reçus et leurs SHA-256.
- Les 56 packages, leurs UPROPERTY, les dépendances/référents
  AssetRegistry, les empreintes binaires et les 1640 profils
  sont comparés champ par champ. Les différences restent privées.
- Un rapport public [ARM-CLEAN06.4B] expose uniquement les compteurs.
- Nouveau contexte UE5.5.4/ARM-CLEAN06.4B : SUCCESS signifie
  **comparaison produite**, jamais migration autorisée.
- Aucune écriture binaire. Lire le document
  ARM_CLEAN06_4B_PRIVATE_PARITY_PREFLIGHT.md avant 06.4C.

## ARM-CLEAN06.4C — qualification des divergences UE sans écriture

- 06.4B validé : huit UPROPERTY, quatre graphes de dépendances,
  quatre empreintes binaires sur quatre packages ; zéro profil ARM modifié.
- Le rapport privé 06.4B (`comparison-private.json`) est lu **sur son
  SHA exact**, avec les deux SHA du manifeste vérifiés contre les tickets
  GitHub 84 et 134. Aucun contenu privé n'est déplacé sur GitHub.
- `QualifyARM_CLEAN06_4C_Deltas.ps1` classe les champs en domaines
  **indicatifs**, conserve chaque nom/valeur/edge dans un rapport privé
  et émet un plan `conversion-gate-private.json` qui interdit toute
  écriture, suppression, migration `.sav` ou modification d'asset.
- L'agent Windows publie `UE5.5.4/ARM-CLEAN06.4C` en plus des deux
  checks habituels et une issue `[ARM-CLEAN06.4C]` à compteurs seuls.
- `success` = qualification produite, **PAS** divergence résolue et
  **PAS** conversion autorisée. La classification détaillée et le
  mapping des valeurs doivent être prouvés avant le convertisseur.
- Voir `docs/Architecture/ARM_CLEAN06_4C_PRIVATE_DELTA_QUALIFICATION.md`.

## ARM-CLEAN06.4D — dry-run réel Unreal Editor : destinations candidates

- Charge la qualification privée 06.4C (`5da0a70`) et le manifeste
  UE 06.4A (`27563be`) avec leurs preuves et son verrou d'écriture.
- Après Build et suite `Grimrock` réussis, le commit au sujet exact
  `chore(ARM-CLEAN06.4D): verify live UE mapping candidates without binary writes` déclenche
  `PlanARM_CLEAN06_4D_EditorDryRun.ps1`.
- Lance le commandlet **réel** `GridArmClean06Baseline` en lecture seule ;
  compare 56 packages (UPROPERTY, liens, classes), 56 SHA-256 binaires,
  les 1640 profils, puis les 8 propriétés et les 4 liens divergents.
- Calcule des **destinations candidates**, sans inventer les valeurs
  ni attribuer automatiquement un ancien champ aux catalogues canoniques.
  Sortie privée et issue publique à compteurs seuls.
- `UE5.5.4/ARM-CLEAN06.4D=success` certifie la **parité de la
  recapture live** avec 06.4A, pas la migration ni son mapping final.
  Liste d'écriture des `.uasset` et sauvegardes **toujours vide**.
- Voir `ARM_CLEAN06_4D_LIVE_EDITOR_DRYRUN.md` ; vérifier les
  trois statuts Windows et l'issue avant 06.4E.

## ARM-CLEAN06.4E — contrôle du catalogue de présentations réel

- **Préflight** : rapport privé `ARM-CLEAN06.4D/dry-run-private.json`
  du SHA `046d785`, huit propriétés sur quatre packages, conversion interdite.
- Commandlet **Editor-only**
  `GridArmClean064EPresentationParity` : contrôle la valeur actuelle de
  chaque champ UE divergent, charge le vrai `DA_ItemSystem` et son
  `DA_ActionPresentationCatalogue`, suit les `ActionId` et liaisons
  `PresentationId` réellement authorés sur les archétypes.
- Le projecteur read-only `GridArmClean06PresentationProjection`
  existant fournit la valeur attendue sans inventer d'action/son ;
  comparaison stricte de **tous** les champs USTRUCT, y compris
  sons ordonnés, timings, Niagara et canaux supplémentaires.
- Rapports détaillés et états `EQUAL_ALL_FIELDS`,
  `DIFFERENT_FIELDS`, `UNBOUND` et manquants : **privés**.
  Issue `[ARM-CLEAN06.4E]` : compteurs agrégés uniquement.
- `UE5.5.4/ARM-CLEAN06.4E=success` prouve uniquement que
  l'audit a produit son rapport, pas une égalité présumée.
  **Écritures autorisées = 0** ; voir
  `docs/Architecture/ARM_CLEAN06_4E_AUTHORED_PRESENTATION_PARITY.md`.

## ARM-CLEAN06.4F — provenance des changements audio historiques

- 06.4E prouve la stricte égalité des **4 profils d'attaque actuels**
  avec les **4 présentations réellement authorées** dans le catalogue
  canonique ; issue 138, trois statuts Windows SUCCESS.
- 06.4F recoupe les **trois captures privées Unreal**
  06.0 (avant ajout des sons Sword), 06.2A (audit des ajouts utilisateur)
  et 06.4A (avant conversion), avec leurs SHA-256 et
  l'attestation de parité 06.4E.
- Compare les UPROPERTY des 56 assets et les 1640 profils ;
  dénombre les changements apparus au moment de 06.2A et
  toute dérive supplémentaire jusqu'à 06.4A.
- Rapport détaillé privé, issue `[ARM-CLEAN06.4F]` à
  **compteurs seuls**, contexte `UE5.5.4/ARM-CLEAN06.4F`.
  SUCCESS signifie « comparaison produite », pas « tout égal ».
- Un écart tardif reste bloquant. Aucune écriture, aucune suppression,
  aucun SaveGame converti. Lire
  `ARM_CLEAN06_4F_THREE_WAY_USER_AUDIO_PROVENANCE.md`.

## ARM-CLEAN06.4G — photographie et restauration binaire en bac à sable

- Source 06.4F validée : quatre définitions Sword présentent les huit
  champs audio modifiés dès 06.2A, **sans dérive** ultérieure ;
  06.4E certifie leur parité canonique strictement actuelle.
- L'agent Windows construit une **liste fermée** :
  les 56 fichiers des reçus de 06.4A + les 28 fichiers réellement
  versionnés sous `CanonicalStage` (registre, quatre catalogues,
  23 définitions). Toute apparition, disparition, LFS pointer,
  altération de hash ou fichier hors préfixe refuse le lot.
- Relance `GridArmClean06Baseline` dans UE5.5.4 : vérifie les
  UPROPERTY, graphes AssetRegistry et 1640 profils de 06.4A.
- Sauvegarde les 84 vrais binaires dans un dossier **privé** de l'agent,
  vérifie SHA-256 avant et après chaque copie, puis simule
  la restauration **uniquement dans un autre dossier privé**.
- `UE5.5.4/ARM-CLEAN06.4G` publie le statut du SHA exact ;
  ticket GitHub `[ARM-CLEAN06.4G]` à compteurs seuls.
- La restauration du vrai projet n'est **pas** exécutée, et aucune
  écriture .uasset/.umap/.sav n'est autorisée. Les SaveGames
  personnels ne sont ni recopiés ni ouverts.
- Livrable `docs/Architecture/ARM_CLEAN06_4G_BINARY_TRANSACTION_GUARD.md`.
  Suite : audit exhaustif d'identités, niveaux, jets et références
  puis transaction Editor-only explicite avec sauvegardes prouvées.

## ARM-CLEAN06.4H — inventaire des identités monde et schéma SaveGame

- Source 06.4G validée : sauvegarde privée et restauration simulée
  des 84 binaires, SHA-256 de chaque fichier, sans aucune écriture
  projet ni SaveGame (issue 140).
- Commandlet **Editor-only** `GridArmClean064HReferenceAudit` :
  charge les quatre `UGridLevelAsset`, la vraie palette,
  les définitions de monde `UGridWorldObjectDefinitionAsset`
  répertoriées et les 23 définitions du vrai `DA_ItemSystem`.
- Inventorie les objets physiques `LooseItemInstances`,
  générateurs `ItemSpawns`, `ReceptacleInitialContent`,
  les `DefaultItemDefinition` de palette et les contenus
  initiaux par défaut des définitions d'objets. Enregistre
  les `ItemId`, niveaux, quantités, GUID et pointeurs exacts
  **dans le rapport privé uniquement**.
- Contrôle par domaine les candidats canoniques, identifiants
  sans correspondance, IDs vides, niveaux ou quantités invalides
  et archétypes incompatibles ; un candidat d'identité n'est
  PAS une preuve de parité comportementale ou de conversion.
- Examine **uniquement le schéma C++** SaveGame : les deux
  pointeurs legacy `ProjectileDefinitionAsset` et
  `ItemDefinitionAsset` subsistent ; **aucun .sav ouvert**.
- Le script rehash les 84 binaires locaux et leurs copies privées
  06.4G avant l'audit ; contrôle la propreté Git après.
  Statut `UE5.5.4/ARM-CLEAN06.4H` sur le SHA et ticket
  `[ARM-CLEAN06.4H]` à compteurs agrégés uniquement.
- Migration, suppression et mise à jour de sauvegardes **interdites**.
  Lire `ARM_CLEAN06_4H_WORLD_REFERENCES_READ_ONLY.md`.

## ARM-CLEAN06.4I — distinguer 40 occurrences des ItemId manquants

- Source 06.4H validée : 95 références physiques réelles (55 IDs
  candidats et **40 occurrences non mappées**), zéro GUID dupliqué,
  quatre niveaux, 78 entrées de palette, 44 définitions monde ;
  issue GitHub 141.
- Commandlet Editor-only `GridArmClean064IIdentityGap` : relit les
  **95 lignes privées exactes**, charge les `UGridItemDefinitionAsset`
  correspondants, recoupe l'identité/l'état N0–N20 et les
  23 définitions sérialisées dans `DA_ItemSystem`.
- Regroupe les **40 références** par **ItemId distinct**, inventorie
  les définitions historiques associées et compare **toutes leurs
  UPROPERTY** pour détecter les duplications contradictoires.
  Les valeurs complètes (SFX/VFX, meshes, slots, jets, poids, tags...)
  restent dans les rapports privés.
- Sur les 55 candidats canoniques, compare uniquement quelques
  métadonnées *sélectionnées* (DisplayName, Description, Weight,
  Tags, classe/archétype, Icon et WorldMesh non-ARM).
  Ceci **n'est pas une preuve d'équivalence générale**.
- Le script revalide le verrou 06.4H, le reçu 06.4G et
  les SHA-256 des **84 sources plus sauvegardes privées**.
  Issue publique `[ARM-CLEAN06.4I]` à chiffres agrégés.
- `UE5.5.4/ARM-CLEAN06.4I=success` = inventaire exact produit,
  pas migration autorisée. **Zéro écriture d'asset ou SaveGame**.
  Voir `ARM_CLEAN06_4I_DISTINCT_IDENTITY_GAPS.md`.

## ARM-CLEAN06.4J — projections non-ARM transitoires en mémoire

- Parent 06.4I validé sur `d3fc4e1`, issue 142 : **11 ItemId
  distincts** manquants dans `DA_ItemSystem`, **40 occurrences**
  physiques, **zéro** conflit de définition historique, **zéro**
  arme manquante ; 95 placements en N0.
- Le commandlet UE Editor-only
  `GridArmClean064JNonArmProjection` relit les **11 exports complets
  d'UPROPERTY** privés et les confronte aux véritables `DA_Item_*`.
- Pour chacun, déduit **une famille candidate** depuis `ItemType`
  et les flags de gameplay, construit un `UObject` **transitoire**
  de la classe canonique adéquate et vérifie nom, ID, description,
  poids, tags, icône et mesh. Le cas munitions **n'est jamais
  déduit automatiquement** d'un nom/tag.
- Comptabilise tous les anciens champs **non par défaut et sans
  propriétaire canonique démontré**, plus les verrous spécifiques
  (consommables/actions, équipement, lumière, interaction,
  balistique/Chaos, réceptacles et SaveGame).
- `Scripts/AuditARM_CLEAN06_4J_NonArmTransient.ps1` exige les preuves
  privées de 06.4I, le verrou d'écriture fermé et les 84 binaires
  d'origine + copies 06.4G vérifiées SHA-256.
- Rapport privé `nonarm-projection-private.json`, ticket
  `[ARM-CLEAN06.4J]` à compteurs agrégés et statut Windows
  `UE5.5.4/ARM-CLEAN06.4J` sur le SHA exact.
  **SUCCESS = simulation terminée, pas authoring autorisé.**
- Aucun asset, niveau, palette ni SaveGame sauvegardé ;
  `migration_authorized=false` et listes d'écriture vides.
  Voir `ARM_CLEAN06_4J_NONARM_TRANSIENT_PROJECTION.md`.

## ARM-CLEAN06.4K — propriétaire des 50 propriétés legacy non-armes

- Source 06.4J validée : **11** identités absentes, **40** occurrences,
  **11/11** projections temporaires exactes pour les champs communs,
  **50** propriétés historiques actives sans destination prouvée
  (1 consommable candidat, 9 génériques, 1 non classifiée), issue 143.
- Le commandlet Editor-only `GridArmClean064KNonArmFieldOwnership`
  recharge les sources réelles, compare **toutes les UPROPERTY**
  avec les preuves privées 06.4I, puis vérifie les 50 champs
  non par défaut reportés par 06.4J.
- Chaque champ entre dans **un domaine explicite de revue** :
  lecture, lumière, FX persistant, physique Chaos, lancer générique
  (non-ARM), actions, équipement, empilement, legacy armement,
  ou non classifié. Aucune catégorie ne constitue une autorisation
  de conversion ni une preuve de parité.
- Les règles de balistique **non-armement** ne doivent pas être
  transférées vers le `WeaponCatalogue.ProjectilePolicies`, qui
  concerne les armes. Les textes lisibles et Niagara persistant
  exigent également une autorité spécialisée.
- Le PowerShell `AuditARM_CLEAN06_4K_NonArmFieldOwners.ps1`
  contrôle les preuves privées 06.4I/4J, rehash les 84 binaires et
  leurs sauvegardes privées 06.4G, puis lance le commandlet UE.
- Publication du statut `UE5.5.4/ARM-CLEAN06.4K` et du rapport
  GitHub `[ARM-CLEAN06.4K]` **agrégé uniquement**.
  Exactes valeurs/champs/identités conservés sur Windows.
- `migration_authorized=false`, **0 écriture de .uasset/.umap/.sav**,
  aucune modification du jeu. Voir
  `ARM_CLEAN06_4K_NONARM_FIELD_OWNERSHIP.md`.

## Reprise recommandée — ARM-CLEAN06.4L (après 06.4K validé)

- Preuve publique 06.4K : issue
  https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/144 ;
  11 ItemId non-armement manquants, 40 occurrences monde,
  **50 occurrences de propriétés actives**, **26 noms distincts**,
  aucune autorité totalement approuvée, zéro écriture autorisée.
- Répartition vérifiée : lecture **1**, lumière **1**, présentation
  persistante **11**, physique/Chaos **2**, lancer générique **3**,
  actions d'objet **11**, équipement hors ARM **15**, empilement
  hors consommable **6**, ancien catalogue ARM **0**, non classé **0**.
- **Prochain travail 06.4L :** lire les rapports privés existants dans
  l'agent Windows avec preuve SHA ; établir les destinations
  sémantiques *champ par champ*, en priorité pour les domaines dont
  l'autorité canonique n'existe pas encore, puis réaliser des
  comparaisons sur objets Unreal transitoires et tests comportementaux.
  Un domaine de classement ne suffit **jamais** à valider une conversion.
- Conserver distinctes l'autorité des lancers **non-armes** et celle
  des projectiles des armes : ne pas réutiliser abusivement
  `DA_WeaponCatalogue.ProjectilePolicies`. Pas d'inférence depuis le
  nom d'un objet, une icône ou un tag ; ne pas perdre le N0.
- 06.4L reste **bloqué pour les écritures binaires** tant que
  lecture, lumière, FX persistants, physique, interactions, équipements,
  empilement, références monde et schéma SaveGame n'ont pas leurs
  preuves. Garder le snapshot 06.4G (84 sources + 84 copies privées)
  vérifiable ; aucun `SavePackage`, aucun `.sav` modifié.
- Les validations automatiques `-NullRHI` ne prouvent ni les clics UMG,
  ni Chaos/PIE, ni un Cook. Garder ces recettes pour la phase 06.6.
  Documentation détaillée :
  `ARM_CLEAN06_4K_NONARM_FIELD_OWNERSHIP.md`.

## ARM-CLEAN06.4L — contrat présenté au contrôle UE (statuts à vérifier)

- Sujet exact attendu : `feat(ARM-CLEAN06.4L): preserve nonarm light and persistent visual data in transient canonical projection`.
- 06.4K et ses trois statuts SUCCESS sont le parent de preuve ; il a
  distingué **50** occurrences actives : lumière **1**, présentation
  persistante **11**, les autres domaines **38**.
- La classe canonique `UGridNonArmamentDefinitionAsset` reçoit les
  huit champs lumière/VFX/scintillement présents sur les sources legacy.
  Aucune nouvelle autorité n'entre dans les armes ou leur catalogue.
- `GridArmClean064LNonArmPresentationParity` doit copier **88** valeurs
  de propriétés dans **11** objets transitoires UE et constater
  l'identité exacte, y compris les défauts, ainsi que les prédicats purs.
- L'agent Windows doit publier pour ce commit :
  `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`,
  `UE5.5.4/ARM-CLEAN06.4L`.
  **Ne déclarer 06.4L validé qu'après trois SUCCESS sur son SHA exact**.
- Des tests PIE/Chaos/SaveGame restent nécessaires : le commandlet n'a
  produit qu'une parité de *données* ; aucune conversion `.uasset`,
  `.umap` ni `.sav`. Les 38 autres propriétés sont encore bloquantes.
- Document détaillé :
  `docs/Architecture/ARM_CLEAN06_4L_NONARM_PRESENTATION_DATA_PARITY.md`.
- Suite envisagée : 06.4M, traitement des politiques non-ARM encore
  sans propriétaire et contrôles comportementaux, sans migration hâtive.

## ARM-CLEAN06.4M — données de lecture et de physique non-ARM

- 06.4L contrôlé : `a279761d52fba8d8a5280104e009bd336cb6c2a1`,
  trois SUCCESS, issue 145 ; aucun binaire migré.
- Scope 06.4M : `ReadText` (**1** champ actif) et opt-in
  masse/tilt Chaos (**2** champs actifs), en conservant le noyau
  canonique à cinq champs et les politiques non-ARM spécialisées.
- Une capacité explicite `bReadable` conserve livres/parchemins
  même sans texte ; les overrides de texte restent des états d'instance.
- La projection UE temporaire prévoit **33 copies réfléchies** des
  trois propriétés sur 11 sources et **77 contrôles purs**
  (44 lectures et 33 règles de masse/tilt), sans `SavePackage`.
- Le vrai runtime continue d'utiliser le legacy tant que les
  11 `ItemId` et les 40 placements n'ont pas été convertis en UE.
- L'agent doit publier `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto` et `UE5.5.4/ARM-CLEAN06.4M` sur le
  **SHA du commit 06.4M**. Aucune validation anticipée.
- Les autres **35** occurrences actives restent à auditer :
  lancer non-ARM 3, actions 11, équipement non-ARM 15, empilement 6.
- Voir `docs/Architecture/ARM_CLEAN06_4M_NONARM_READING_WORLD_PHYSICS_PARITY.md`.
  Prochaine phase recommandée : 06.4N, lancer physique non-armement.

## ARM-CLEAN06.4N — lancer générique non-ARM, projection en mémoire

- Parent 06.4M validé sur `8d925f186e02f419c3b5087f7df8f8f848548552`,
  trois contrôles SUCCESS, issue 146.
- 06.4N projette les **3 propriétés actives** du domaine
  `GENERIC_THROW` sans inférer de nouvelle action.
- `FGridNonArmamentThrowPolicy`, propriété de
  `UGridNonArmamentDefinitionAsset`, conserve 12 paramètres
  (balistique, collision/retombée, apparence du mesh, rotation).
  Aucune modification de `DA_WeaponCatalogue.ProjectilePolicies`.
- Commandlet `GridArmClean064NNonArmGenericThrow` : 11 objets
  transitoires sur les sources réelles, **132 propriétés** correspondantes,
  **121 copies exactes** et **11 modes enum** mappés explicitement,
  **77 sondes pures** (33 balistique, 44 visuel).
- Les identités et valeurs privées ne sont jamais publiées. 84 sources
  et leurs copies immuables vérifiées SHA ; sauvegardes non lues.
- **À vérifier sur SHA 06.4N :** `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4N` tous SUCCESS ;
  ce jalon ne vaut qu'une parité de données, pas un PIE/Chaos/cutover.
- Autres **32** occurrences actives restant à projeter : actions 11,
  équipement hors ARM 15, empilement 6 ; ne pas convertir les assets
  avant les contrôles d'interaction, SaveGame et runtime.
- Document détaillé :
  `docs/Architecture/ARM_CLEAN06_4N_NONARM_GENERIC_THROW_PARITY.md`.
  Suite prévue : 06.4O, actions non-ARM, puis équipements et empilement.

## ARM-CLEAN06.4O — inventaire des actions non-ARM (lecture seule)

- Parent 06.4N validé sur `99bd3afc3207830c76542078eaba417dcbbc9b63`,
  issue 147 et les trois statuts SUCCESS.
- Objet du jalon : **11** occurrences d'UPROPERTY `ITEM_ACTION`
  actives sur 11 ItemId/40 placements. Conserver leurs détails réels
  avant de proposer des liens au `DA_ActionCatalogue`.
- Commandlet Editor-only `GridArmClean064ONonArmActionSemantics` :
  inspecte les vrais `CombatActions`, QuickItems,
  présentations, validités, champs non par défaut et collisions d'IDs ;
  produit une preuve Windows privée, pas de migration.
- Contrôles : provenance 06.4K et 06.4N exactes, 84 binaires et
  84 copies vérifiés, **zéro mutation** d'assets/SaveGames.
- Requiert `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
  `UE5.5.4/ARM-CLEAN06.4O` SUCCESS sur son SHA.
- Même avec SUCCESS, ne déclarer **aucune** parité sémantique
  des actions. Les **32** champs actifs restants comprennent
  actions 11, équipement hors ARM 15, empilement 6.
- Voir `docs/Architecture/ARM_CLEAN06_4O_NONARM_ACTION_SEMANTIC_INVENTORY.md`.

## ARM-CLEAN06.4P — équipements non-ARM, projection avec invariants de main

- Source validée : 06.4O `20f641621cff0f2db0c12a94f9ff38176f0ca666`,
  trois SUCCESS et issue 148. Les 11 propriétés d'actions restent en revue.
- 06.4P traite les **15 occurrences actives** du domaine
  `NONARM_EQUIPMENT` parmi les 11 ItemId/40 placements réels.
- `UGridOtherEquipmentDefinitionAsset` reçoit
  `HandPolicy` explicite, `Resistances` et `EquippedMesh`,
  sans dupliquer `EquipmentSlots` ou `Bonuses` existants.
- Le commandlet `GridArmClean064PNonArmEquipmentParity`
  vise **44 copies strictes**, **11 politiques de main résolues**,
  **209 vérifications des emplacements**, **11 listes UI**, **33
  sondes de bonus N0/N1/N20** et **11 replis de mesh**.
- 84 sources et 84 copies privées rehashées, 50 valeurs actives
  recontrôlées. Aucune écriture binaire et aucun SaveGame ouvert.
- Attendre `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et
  `UE5.5.4/ARM-CLEAN06.4P` tous SUCCESS **sur le SHA de 06.4P**.
  Un succès signifie parité en mémoire, pas test PIE ni cutover.
- **17 champs** resteraient ouverts après validation des données :
  actions 11, empilement 6. Suite : 06.4Q pour empilement, puis
  mapping complet d'actions et régression Save/Continue.
- Documentation :
  `docs/Architecture/ARM_CLEAN06_4P_NONARM_EQUIPMENT_PARITY.md`.

## ARM-CLEAN06.4Q — pile générique et véritable registre UE

- Parent 06.4P validé : `29955df612e1478b94f30a706bac009df3c8d531`,
  3 statuts Windows SUCCESS, issue 149.
- Propriété active du domaine `NONARM_STACK` : **6 occurrences**
  de `bStackable` et/ou `MaxStackSize`. Ne pas confondre avec
  six ItemId ; sources : 11 ItemId non-ARM et 40 placements.
- `UGridGenericStackableDefinitionAsset.MaxStack` est une autorité
  distincte pour les piles *génériques* ; le `MaxStack` existant des
  munitions et consommables reste leur seule autorité. Le booléen
  de pile legacy se résout en capacité, pas en second champ.
- Commandlet `GridArmClean064QNonArmStackParity` : vrais assets UE,
  22 valeurs source, 6 actives, **34 définitions transitoires**,
  `UGridItemSubsystem` réellement enregistré, **44 sondes
  N0/N1/N20/N21** et **33 tests de quantités**.
- **Ne valider 06.4Q qu'avec :** `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto` et `UE5.5.4/ARM-CLEAN06.4Q`
  SUCCESS sur son SHA exact. Un succès n'approuve **aucune**
  conversion de niveau, SaveGame ou choix de famille réelle.
- Suite **resserrée** : 06.4R pour décider tous les mappings
  sémantiques des 4 actions et de la présentation observées en
  06.4O, puis migration Editor contrôlée, vérification et cutover.
  **Éviter de prolonger les micro-audits** pour des champs déjà
  couverts par preuve transitoire. La migration ne commence qu'avec
  le schéma SaveGame, les références monde et le rollback validés.
- Détails : `docs/Architecture/ARM_CLEAN06_4Q_NONARM_STACK_REGISTRY_PARITY.md`.

## ARM-CLEAN06.4R — action complexe non-ARM sans perte de champ

- Parent : 06.4Q validé sur `608a83c76205cfc126088fc19f36d5f0e2e173aa`,
  issue 150 et trois statuts SUCCESS.
- Source : 06.4O a inventorié **11 occurrences ITEM_ACTION**,
  **1** action équipement, **3** QuickItems normalisées,
  **1** présentation et **54** champs imbriqués non défaut.
- Nouveau pont **dans les catalogues uniques existants** :
  `DA_ActionCatalogue.DetailedActions` contient le vrai
  `FGridCombatActionDefinition` intégral, et
  `DA_ActionPresentationCatalogue.DetailedPresentations` la
  vraie `FGridPlayerAttackPresentationProfile`. Les IDs
  sont disjoints de leurs entrées compactes. AUCUN cinquième catalogue.
- Des `ActionBindings` optionnels sont ajoutés à la définition
  non-ARM, mais ne sont **pas activés** par R. Les anciennes
  `UseActions` des consommables ne sont pas dupliquées.
- Le commandlet `GridArmClean064RNonArmFullActionBridge`
  projette les 11 vrais ItemId dans un root de 34 entrées,
  vérifie les 50 champs actifs et chaque FProperty des
  **4 actions** contre la capture 06.4O ; conserve la présentation.
- Interdits : aucun `SavePackage`, aucune bascule runtime,
  aucun nouveau contenu joueur. Les quatre liens détaillés
  restent **non approuvés** tant que leurs routes n'ont
  pas été testées sur l'exécuteur et l'UMG.
- Vérifier sur SHA R : `UE5.5.4/SilentTask`,
  `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4R`
  tous SUCCESS. Ne jamais inférer la parité PIE des comptes.
- **Suite resserrée : outil de migration Editor transactionnel,
  activation des bindings réels, tests Save/Continue, puis 06.5/06.6.**
  Ne pas rouvrir un audit de chacune des familles déjà couvertes.
- Document :
  `docs/Architecture/ARM_CLEAN06_4R_NONARM_ACTION_LOSSLESS_BRIDGE.md`.

## ARM-CLEAN06.4S — transaction binaire répétée en privé (PAS de conversion)

- Source : 06.4R `37cf07dc21359a9762c6dd244eebe74bba02bc41`
  trois SUCCESS, issue 151 ; backup immuable 06.4G.
- Le `Scripts/AuditARM_CLEAN06_4S_TransactionRehearsal.ps1`
  rehash les 84 sources + copies, génère un manifeste SHA-256
  fermé et crée uniquement **84 copies dans le RunDir privé**.
- Simulation : ajout volontaire d'un octet dans **une copie de
  test privée**, détection par SHA-256, restauration depuis
  le snapshot 06.4G, vérification des **84** copies restaurées
  et suppression du seul répertoire temporaire.
- `migration_authorized=false`,
  `allowed_binary_writes=[]`,
  `allowed_package_deletions=[]`,
  `allowed_savegame_writes=[]`.
  **AUCUN `SavePackage` ni fichier utilisateur touché.**
- Attendre `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`
  et `UE5.5.4/ARM-CLEAN06.4S` tous SUCCESS **sur SHA S**.
- Verrous à lever pour **06.4T** : choix des 11 familles
  exactes, 40 références, 4 actions encore sans route runtime,
  backups/compatibilité des vrais SaveGames, assets WBP/presets
  périphériques et démonstration post-conversion.
- Document :
  `docs/Architecture/ARM_CLEAN06_4S_TRANSACTION_REHEARSAL.md`.

## ARM-CLEAN06.4T — véritable lot de DataAssets (pas un audit supplémentaire)

- Source validée : 06.4S `07c757485a929707bcaed9832ed5933ab409950d`,
  issue 152, trois SUCCESS, 84 binaires protégés.
- `GridArmClean064TNonArmMaterialize` crée de vrais DataAssets
  via `CreatePackage` + `SavePackage` et met à jour
  `DA_ItemSystem.uasset`. Les objets avec actions non
  routées ou famille ambiguë sont **différés**, non supprimés.
- Le commandlet `-Verify` rouvre les UAssets dans un **second**
  processus UE et compare les vrais champs C++ aux sources
  06.4K. Ne jamais simuler cette étape par un simple JSON.
- `Scripts/ApplyARM_CLEAN06_4T_NonArmEditorMigration.ps1`
  est déclenché seulement sur le sujet de commit exact ; il
  contrôle le set Git/LFS, relance Grimrock, amende **seulement
  notre propre commit de code 06.4T** et push avec lease ;
  ainsi, un seul commit final sur `master`.
- Sur le SHA final (révision contenant réellement `.uasset`) :
  exiger `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`,
  `UE5.5.4/ARM-CLEAN06.4T` SUCCESS.
- Ne pas dire « terminé » si seul le commit de code est
  présent. Les 40 références de niveau et SaveGames sont
  la suite. Aucun nouveau fallback legacy en runtime.
- Détails :
  `docs/Architecture/ARM_CLEAN06_4T_REAL_NONARM_ASSET_MIGRATION.md`.

## ARM-CLEAN06.4U — actions QuickItem natives

- Parent 06.4T fcf13df12f31e5da4018fc10d2b621d84212b33e, 3 SUCCESS.
- Nouveau lecteur UGridItemSubsystem::ResolveDetailedNonArmAction,
  données intégrales FGridCombatActionDefinition dans le
  DA_ActionCatalogue unique ; aucun fallback legacy ajouté.
- Commandlet GridArmClean064UNonArmQuickMaterialize :
  3 vrais DataAssets + DA_ActionCatalogue + DA_ItemSystem.
- Second processus -Verify et Automation après SavePackage,
  puis amend/push atomique du seul commit 06.4U.
- Lire docs/Architecture/ARM_CLEAN06_4U_NONARM_QUICKITEM_REAL_MIGRATION.md.
- Après U : les 2 objets action/équipement, les 40
  références historiques, SaveGame, puis 06.5/06.6.
## ARM-CLEAN06.4V — reprise de la conversion effective

- Parent validé : 06.4U `1fd53212b60b26363a874db2b561dcf2c92347d2`.
- `GridArmClean064VNonArmEquipmentMaterialize` crée deux vrais
  DataAssets puis écrit DA_ItemSystem, DA_ActionCatalogue et
  DA_ActionPresentationCatalogue. La vérification `-Verify`
  recharge les fichiers dans un second Unreal Editor.
- `ResolveDetailedNonArmEquippedAction` et le TurnManager
  transmettent la source de main physique et les profils complets.
  Les équipements empilables conservent leur MaxStack unique.
- `Scripts/ApplyARM_CLEAN06_4V_NonArmEquipmentMigration.ps1`
  limite les écritures à cinq binaires LFS et n'amende que
  son propre commit V avec lease après les tests.
- Exiger SilentTask, LocalAuto et ARM-CLEAN06.4V SUCCESS sur
  le commit final comportant ces binaires ; sinon V reste ouvert.
- Puis : 40 références monde, SaveGames sur copies, nettoyage 06.5,
  PIE/Chaos/Cook/Shipping 06.6.
- Détail : `docs/Architecture/ARM_CLEAN06_4V_LAST_NONARM_EQUIPMENT_MIGRATION.md`.

## ARM-CLEAN06.4W — archives du plan avant sa validation (réalisé)

**ARCHIVE : cette section est l'ancien plan de 06.4W. Le résultat vérifié et le prochain jalon figurent en tête. Ne pas l'utiliser comme instruction d'exécution.**

**Point de départ obligatoire :** commit final 06.4V
`b9750269732f1322ecca11ec2bcd30ba3a4796c2`, trois contrôles Windows SUCCESS.
Ce passage doit être une **migration effective d'assets UE**, et
non une répétition des audits 06.4H/4I ni une conversion SaveGame déguisée.

- Périmètre : les 4 niveaux `DA_GridLevel_*`, la palette physique,
  les contenus initiaux des réceptacles et les définitions de monde
  réellement référencées. Revalider l'inventaire de **95 références
  06.4H** ; ne pas limiter artificiellement la conversion aux
  seules **40** occurrences précédemment non canoniques.
- Sérialiser l'identité canonique `ItemId` (avec le niveau **N0
  conservé tel quel**), le GUID, la quantité, les cellules/poses,
  les paramètres de réceptacle et les champs propres à l'énigme.
  La source de vérité runtime doit passer par
  `UGridItemSubsystem::ResolveItem`. Les accès d'éditeur et
  les lecteurs historiques doivent être mis à jour sans réintroduire
  une seconde autorité par pointeur.
- Exécuter un préflight **Editor-only** à périmètre de packages fermé,
  vérification des sources et du snapshot privé 06.4G, liste blanche
  des sorties, suivi Git LFS, puis `SavePackage` uniquement sur
  les fichiers explicitement validés. Aucun `.sav` ni fichier hors
  liste ne peut être écrit.
- Après écriture, ouvrir les vrais assets dans **un second processus
  Unreal Editor** et prouver la parité de chacune des références,
  quantités, GUID, N0/N1–N20, poses et règles de réceptacle, puis
  refaire la suite `Grimrock` sur les binaires enregistrés.
  Prévoir restauration des copies privées si un contrôle échoue.
- Publier au plus **un commit final** sur `master`, puis vérifier
  sur son SHA `SilentTask`, `LocalAuto` et le contrôle spécifique
  06.4W. Aucune validation PIE/Chaos/Save/Continue n'est inférée
  des tests `-NullRHI`.
- **Après 4W seulement :** migration des SaveGames sur copies
  privées, avec contrôle de version et de GUID, puis 06.5
  suppression du legacy et 06.6 PIE/Cook/Shipping.

**Point de vigilance confirmé dans le C++ courant :**
`FGridLooseItemInstance.ItemDefinition`,
`FGridReceptacleInitialItemConfig.ItemDefinition` et
`FGridObjectPaletteEntry.DefaultItemDefinition` sont encore des
pointeurs vers `UGridItemDefinitionAsset` ; le schéma SaveGame
possède encore `ProjectileDefinitionAsset` et
`FGridPendingWorldItemState.ItemDefinitionAsset`.
Ne pas vider ces champs tant que leurs consommateurs et
la conversion des assets n'ont pas été validés.

## Étapes ultérieures inchangées

1. Terminer l'audit des autres lecteurs 06.3 (jets génériques,
   monde/réceptacles, outils d'authoring, catégories restantes).
2. **06.4 :** conversion contrôlée des contenus binaires via UE Editor,
   y compris les cas SaveGame ; pas d'écriture artificielle des `.uasset`.
3. **06.5 :** retrait des anciens champs, chemins et outils après
   preuve de migration et contrôle des références AssetRegistry.
4. **06.6 :** régression complète UE, PIE transversal, Save/Continue,
   puis vérification Cook/Shipping.

## Règles permanentes de reprise

- Lire `AGENTS.md` avant toute modification.
- Une étape fonctionnelle = **un seul commit final maximum**, sur
  `master`, push automatique après vérification, jamais un commit
  par correction ou fichier.
- Après push C++ : consulter `UE5.5.4/SilentTask` **et**
  `UE5.5.4/LocalAuto` du **nouveau** SHA exact ; suivre les échecs
  Windows automatiquement, sans imposer `ValidateUE.ps1` à l'utilisateur.
- C++ = comportement ; WBP Designer = présentation. Si le jalon
  touche au texte visible, appliquer
  `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` et contrôler
  `UI.csv → ST_UI.uasset → WBP → Game/fr/en`.
- Ne pas effacer de modifications distantes imprévues. Ne pas
  conclure `zéro legacy` avant 06.5 et la recette 06.6.
- Lors d'une reprise avec deux conversations de titre similaire,
  identifier **la plus récente** puis recouper impérativement le
  dernier compte rendu avec `master` et ce document.
