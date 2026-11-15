# ARM-CLEAN06.4R — Actions non-ARM, profils complets sans perte

## Parent vérifié

- `ARM-CLEAN06.4Q` : commit `608a83c76205cfc126088fc19f36d5f0e2e173aa`, **trois statuts SUCCESS** ; rapport [#150](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/150).
- `ARM-CLEAN06.4O` : commit `20f641621cff0f2db0c12a94f9ff38176f0ca666` ; rapport [#148](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/148), **11 champs actifs ITEM_ACTION**, 1 action d'équipement, 3 actions QuickItem normalisées, 1 présentation et **54 propriétés imbriquées non défaut**.
- `ARM-CLEAN06.4G` : snapshot privé immuable de 84 binaires Unreal et leurs 84 copies ; SHA contrôlés sur Windows.
- **Source architecturale prioritaire :** `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`, version approuvée. Ce jalon constitue une étape **pré-cutover**, pas une nouvelle norme concurrente.

## Le problème : les actions ne sont pas réductibles à une simple entrée compacte

`FGridCombatActionDefinition` décrit déjà les mécanismes exécutés réellement par le runtime : ciblage, coûts PA/ressources, filtres, attaques multiples, dégâts, soins, statuts, protections, mouvements, surfaces, pièges, présentation, etc. Le petit `FGridCanonicalAction` initial ne possède pas encore tous ces mécanismes. Une projection vers celui-ci seule ferait perdre une partie des **54** données non défaut observées dans 06.4O, sans possibilité de récupération.

La solution de ce jalon est **un emplacement de données complet dans chacun des deux catalogues globaux EXISTANTS**, avec clés disjointes et sans cinquième catalogue :

| Propriétaire unique | Ajout | Sens |
|---|---|---|
| `DA_ActionCatalogue` | `DetailedActions : FGridCanonicalRuntimeAction[]` | `FGridCombatActionDefinition` intégral, le même type que le vrai moteur de combat |
| `DA_ActionPresentationCatalogue` | `DetailedPresentations : FGridCanonicalRuntimePresentation[]` | `FGridPlayerAttackPresentationProfile` intégral, conservant sons, offsets, VFX et temporisations |
| `UGridNonArmamentDefinitionAsset` | `ActionBindings : FGridItemActionBinding[]` | Couples `ActionId / PresentationId` **uniquement**, jamais de copie des mécanismes |

Les actions `Actions[]` existantes et `DetailedActions[]` ne peuvent **jamais** partager un même `ActionId`. La validation applique la même unicité aux deux familles de présentations. `ValidateBindings` vérifie l'existence des IDs dans les deux profils. Les consommables existants avec `UseActions` ne peuvent pas, simultanément, déclarer des liaisons non-ARM supplémentaires pour le même objet. **Il n'y a qu'un seul catalogue d'actions et un seul catalogue de présentations au runtime.**

Cette extension constitue un pont vers la conversion et l'exécution complète. Les nouvelles entrées sont **absentes des vrais assets** à ce stade : seuls le schéma, la validation et le commandlet de projection en mémoire sont livrés. Les sources historiques restent l'autorité d'exécution tant que la migration n'est pas prouvée.

### Ce que le pont ne doit pas faire

- Ne jamais reconstruire une QuickItem par son icône, son nom ou une catégorie. Utiliser **uniquement** `BuildQuickItemCombatActionDefinition` du DataAsset réel, avec ses tags, coûts et SourcePolicy normalisés.
- Ne pas déduire une présentation depuis une icône. L'unique présentation observée est copiée depuis `PlayerAttackPresentationProfile`. Son ID privé `NonArmPreview_<ItemId>` est une **clé temporaire de test non sauvegardée**, pas une clé approuvée ou un texte joueur.
- Ne pas tenter de placer des données ARM N1 dans les objets non-ARM : aucun nouvel archétype n'est créé.
- Ne pas supposer qu'un profil en catalogue possède déjà une route runtime. L'activation des bindings et les refus pré-consommation seront testés séparément.
- **Aucune duplication persistante** des données d'action entre asset concret et catalogue : les valeurs du legacy seront transférées, puis supprimées après preuve de cutover.

## Validation réelle UE5.5.4

Le sujet exact de commit qui déclenche le contrôle Windows est :

`feat(ARM-CLEAN06.4R): stage complete nonarm combat actions in canonical catalogues`

Le commandlet `GridArmClean064RNonArmFullActionBridge`, appelé par `Scripts/AuditARM_CLEAN06_4R_NonArmActionBridge.ps1` :

1. vérifie les reçus privés SHA-256 06.4K, 06.4O et 06.4Q, et rehash les 84 originaux **plus** les 84 sauvegardes binaires de 06.4G ;
2. ouvre les 11 véritables `UGridItemDefinitionAsset` non-ARM et compare les 50 propriétés actives avec la capture privée 06.4K ;
3. recharge le vrai `DA_ItemSystem` et crée, **sous le package transitoire seulement**, 11 définitions sans action et des copies non persistées des deux catalogues ;
4. vérifie **chacun des champs** des 4 définitions d'action avec le rapport privé 06.4O (ExportText), puis transfère intégralement les structures (CompareScriptStruct, Identical et comparaison de tous les exports réfléchis) dans les nouvelles sections du catalogue d'actions ;
5. vérifie l'unique présentation authorée, sa valeur privée 06.4O et sa correspondance `CompareScriptStruct` dans le catalogue de présentation ;
6. refuse les IDs d'actions présents en double ou déjà utilisés dans le catalogue existant, ainsi que les présentations invalides ;
7. valide **le vrai contrat `UGridItemSystemAsset::Validate`** sur un registre transitoire de **34 définitions** (23 + 11) et ses quatre catalogues ;
8. produit **uniquement sous Windows privé** la liste exacte des 11 identités et 4 futurs liens `ActionId`, encore **non approuvés**. Aucune action n'est activée et aucune définition réelle n'est resauvegardée ;
9. refuse toute transition d'état Dirty, toute écriture binaire ou SaveGame, et ne publie qu'un rapport agrégé GitHub.

Les validations à demander sur le **SHA exact** sont : `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4R`. Un succès signifie **copie exhaustive des données de combat et présentation en mémoire**, et **non** route exécutable en PIE.

## Clôture de 06.4 : plan resserré, pas de multiplication des audits

1. **06.4R** — présent jalon. Si les trois statuts sont verts, les 50 occurrences de propriétés des 11 objets non-ARM disposent d'un chemin de conservation **en mémoire**. Les 11 propriétés ITEM_ACTION seront couvertes sans perte apparente de champ, mais **pas encore par la résolution runtime**.
2. **Migration Editor** — écrire un unique outil Editor à transaction déclarée, manifest fermé, état pré-contrôlé des catalogues et des objets, dry-run non destructif, écriture exclusivement via `SavePackage` après garde, références monde/palettes/réceptacles préservées et rollback. Vérifier les familles réelles de chaque ItemId (une potion n'est pas un générique parce qu'elle s'empile) et l'unicité des bindings.
3. **Bascule de lecture** — activer les routes canoniques détaillées dans le moteur d'action/présentation, interdire les fallbacks pour les ItemId enregistrés, vérifier refus avant PA/consommation, ciblage, hotbar et cooldown. Contrôler la capture et la restauration des GUID/états mutables/quantités/niveaux sans conversion silencieuse des SaveGames.
4. **06.5** retire l'ancien code et les champs **après** vérification des assets ; **06.6** valide PIE, Chaos, Save/Continue, Cook/Shipping.

Le fait d'avoir une copie exhaustive des profils ne donne **pas** l'autorisation de supprimer les définitions anciennes. `migration_authorized=false`, listes d'écritures vides et aucun `.uasset`, `.umap`, `.sav` modifié par 06.4R.

## Reprise de l'historique des preuves 06.4Q

Les essais 06.4Q antérieurs au correctif `UGameInstance` ont écrit des JSON privés valides mais sans reçu de validation : Unreal avait émis un handled ensure. Une recherche `Private-Report` qui exige une **seule** capture 06.4Q échoue donc avant de lancer le commandlet R. Le script R filtre désormais les copies par **`stack-gate-private.json`**, validation SHA `608a83c7...`, empreinte SHA-256 exacte du rapport, preuves 84 originaux/copies et droits de migration fermés. Les éventuelles copies approuvées multiples doivent être **octet pour octet identiques** (SHA-256). Les essais échoués restent privés et sont ignorés ; aucune donnée publiée ni modifiée.
