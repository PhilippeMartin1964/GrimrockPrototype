# ARMEMENT_PIPELINE_CONTRACT — Contrat vivant d'industrialisation (INDUS01)

**Version de conception : v0.1, 25 octobre 2026.** Ce document définit la cible technique, **non encore implémentée**. Aucune commande d'exemple ci-dessous n'existe aujourd'hui sous le nom WeaponPipeline.ps1.

## A. Principes non négociables

1. Une seule source éditoriale des caractéristiques N1-N20 : docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx. Aucun doublon de dés, PA, portée, poids ou précision dans les scripts, manifestes ou WBP.
2. Un manifeste technique versionné pour les identités stables et les vraies références UE, distinct du classeur de gameplay. Les chemins historiques ne sont pas réécrits.
3. Un **point d'entrée utilisateur** : Scripts/WeaponPipeline.ps1. Une interface stable, indépendamment du nombre éventuel de services internes.
4. Un **commandlet Editor générique** (ou dispatcher partagé avec services factorisés), sans une nouvelle classe C++ par arme.
5. DataAssets existants, FGridItemInstance, DA_WeaponCatalogue, DA_WeaponVisualCatalogue, palette, niveaux, systèmes combat/SaveGame demeurent compatibles ; pas de migration implicite.
6. C++ pour les validations, le calcul, les read models et le formatage ; Designer UMG pour la présentation. Ne pas reconstruire des TextBlocks ou de l'UMG en C++.
7. Toute modification d'asset nécessite -Apply explicite après audit. **Pas d'écriture en mode Audit/Plan/Validate.** Pas de git add/commit/push automatique.

## B. Sources et frontières

| Source | Autorité et clés | Diffusion |
|---|---|---|
| XLSX Catalogue + Niveaux 1-20 | ArchetypeId, ItemLevel, nom, famille, usage, NdM+K, accuracy, armure, PA, range, WeightKg, classes | DA_WeaponCatalogue (immutable runtime profiles) |
| Extension narrative XLSX (INDUS03) | ArchetypeId, NomCourtFR, DescriptionCourteFR, LoreFR, ParticulariteFR, EtatFonctionnel | FText de domaine/projection narrative vers DataAsset, jamais bonus non codé |
| Manifeste de production (INDUS02) | ArchetypeId, ItemDefinitionId, VisualId, asset names, exact icon/mesh/MI/PBR paths, Category, BuildKind, palette EntryId, SpecialBehaviorKey | Pipeline Editor et audit ; pas d'autorité des dégâts |
| UE DataAssets | Références runtime, compatibilité, équipement et identité sauvegardée | Inventaire, combat, monde et UI |
| Localization/Source/UI.csv | Labels statiques et patterns de UI partagés | ST_UI.uasset après authoring réel, puis WBP |

**Important :** le système actuel exige 12 onglets XLSX et 19 champs CSV ; ne pas ajouter un onglet/une colonne directement à l'import existant. INDUS03 doit déclarer SchemaVersion et tester v1 en compatibilité avant d'étendre à v2. Les formules du XLSX doivent être recalculées par l'outil de production avant export ; le pipeline rejette des caches absents plutôt que de deviner des valeurs.

## C. Enregistrement technique explicite proposé

Colonnes du manifeste INDUS02 (noms indicatifs, à geler dans ce jalon) :

- ArchetypeId et ItemDefinitionId : identités de gameplay stables.
- VisualId, ItemAssetPath, WorldMeshPath, IconPath, MaterialInstancePath : chemins UE complets, sans nom déduit.
- PbrBaseColorPath, PbrNormalPath, PbrOrmPath : ressources effectivement requises, ou description typée d'un matériau legacy.
- BuildKind : MeleeOneHand, MeleeTwoHand, Ranged, ShieldPassive, Throwing, LegacyCustom. Type explicite contrôlé par schéma, jamais choisi par suppositions sur le texte.
- EquipmentSlots, PaletteCategory, PaletteEntryId : règles provenant des systèmes existants et contrats approuvés ; ne jamais inventer une restriction par arme.
- SpecialBehaviorKey + Status : étiquette de suivi facultative. Aucune activation gameplay par une phrase narrative ou un indicateur d'audit.

Ne pas recopier WeightKg, DamageDice, PA, Range ou Accuracy dans ce manifeste ; ils sont dans DA_WeaponCatalogue. Le manifeste doit référencer un profil N1 existant avant de créer une définition.

## D. Interface d'orchestration envisagée

Exemples non implémentés, pour cadrer INDUS02 :

    .\Scripts\WeaponPipeline.ps1 -Action Audit -All
    .\Scripts\WeaponPipeline.ps1 -Action Plan -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Apply
    .\Scripts\WeaponPipeline.ps1 -Action Validate -Archetypes ARM002,ARM003,ARM013,ARM020

- Audit : inventaire complet et cohérence des référencements, sans mutation.
- Plan : différences proposées, explicites et vérifiées avant écriture ; aucune mutation.
- Build sans -Apply : préflight complet et simulation des changements.
- Build -Apply : création/raccordement **au maximum d'un archétype par transaction au premier déploiement** ; rejeter les doublons, ressources manquantes, conflits ou asset existant non conforme.
- Validate : tests de contrat ciblés et un rapport consolidé. Les tests PIE manuels gardent leur propre statut.
- -All et -Archetypes multiples : autorisés pour Audit/Plan/Validate ; au départ, interdits pour Build -Apply tant qu'une transaction multi-asset vraiment atomique n'a pas été démontrée.
- Toutes les commandes écrivent uniquement journaux et rapports **sous Saved/** ; mode dry-run doit conserver les hashes et le git status des fichiers métier.

## E. Machine d'états et codes de sortie

États métier distincts : UNDECLARED, DECLARED, ART_MISSING, ART_READY, VISUAL_REGISTERED, ITEM_AUTHORED, PALETTE_REGISTERED, LEVEL_PLACED, AUTOMATION_PASSED, PIE_PASSED, PUBLISHED. Un état produit ne doit pas masquer les étapes antérieures ni la mention de spécialité non implémentée.

Messages structurés pour chaque ArchetypeId : BEFORE, PLANNED, AFTER, changed package paths, errors/warnings, tests, docs, source workbook hash/schema revision. Rapport JSON + CSV + log. Au minimum : READY, BLOCKED, PRESENT_SAME, CONFLICT, CREATED, UPDATED, SKIPPED.

Codes de sortie : 0 = opération conforme au mode demandé, avec résultats exacts ; non-zéro = commande, validation ou écriture échouée. Une présence BLOCKED dans un **audit** peut être non fatale si le rapport la qualifie clairement ; un Build -Apply ne doit jamais réussir sur BLOCKED. Un résultat indéterminé ne peut produire [OK].

## F. Tests génériques et contrats de compatibilité

Suite envisagée Grimrock.RPG.ARMEMENT.Industrial :
1. WorkbookSchema : 82 archétypes, 1640 profils, 260 passifs, 20 niveaux chaque, uniqueness, cache et schema version.
2. ManifestIntegrity : IDs uniques, chemins exacts, aucune collision de ItemDefinitionId/VisualId/EntryId ; exceptions legacy autorisées explicitement.
3. ArtValidation : présence/chargement icône, mesh, MI, PBR, mip/dimensions, sRGB BC/N/ORM, slots MI, matériau parent.
4. VisualBinding : cohérence lecture seule du DA_WeaponVisualCatalogue, pas de doublons ou substitutions.
5. ItemDefinition : poids/niveau/type/usage dérivés de profil, capacités des mains, action réelle et restrictions selon BuildKind.
6. WorldPaletteLevel : contrôle référentiel de la palette et du niveau sans placer/effacer de cellule en test automatique.
7. NoMutationDryRun : status et hashes identiques avant/après Audit/Plan/Build sans -Apply ; pas de fichier binaire inconnu.
8. IdempotenceAndConflicts : second passage = PRESENT_SAME, pas de save ; conflit = erreur, aucune perte.
9. InventoryTooltipProjection (INDUS04) : profil au **CachedItem.ItemLevel réel**, traduction, NdM+K, accuracy, AP, range, scaling, comparaison sans double-comptage.
10. LegacyRegression : ARM001, ARM045, ARM075, ARM014/015/025/031, ARM002 et familles ThrowingKnife/Shuriken, avec rapports réels.

Les tests de fichiers peuvent passer sans que l'arme soit jouable. Les tests PIE conservent sept cas : visuel, pickup, tooltip d'inventaire, équipement, drop/reprise, combat, sauvegarde/rechargement. Cas particuliers pour deux mains, distance, projectiles et capacités spéciales. **Ne jamais valider une spécialité du fait de sa simple mention dans le classeur.**

## G. Tooltip et narratif : responsabilités

- Créer une **projection en lecture seule** des propriétés de FGridWeaponLevelProfile pour chaque item au niveau réel de l'instance : nom, famille, dés NdM+K, accuracy, PA, range, poids, type/sous-type de dégâts, scaling et statistiques défensives pertinentes.
- Injecter cette projection dans FGridItemTooltipView ou son sous-modèle partagé. Ne pas faire de calcul de dégâts simulés dans la vue ni dans WBP_InventorySlot ; les valeurs affichées sont des valeurs descriptives de catalogue.
- Garder EquipmentStatBonus/ResistanceBonus et leur comparaison existants ; les enrichir sans dupliquer les chiffres offensifs.
- DescriptionCourteFR et LoreFR sont des **FText de domaine** localisables, pas des clés ST_UI de présentation. Noms de sections/labels UI dans Localization/Source/UI.csv selon docs/UI_WBP_LOCALIZATION_WORKFLOW.md.
- Reprendre WBP existant dans UMG Designer, pas de refonte obligatoire du style. Ne pas confondre l'infobulle d'inventaire avec WBP_GridCombatTooltip (trace de combat réelle).
- Toute donnée spéciale a un état : IMPLEMENTED/PLANNED/UNSUPPORTED. Ne pas afficher une compétence comme utilisable si elle est seulement narrative.

## H. Stratégie de migration et documentation allégée

- INDUS01 : audit + présent contrat (lecture seule des assets), livré par un seul commit documentation.
- INDUS02 : adapter **d'abord un profil existant en dry-run**, sans le réécrire. Prouver compatibilité de ARM002, ARM031, ARM075, ARM045 et legacy. Ensuite créer une nouvelle arme par la même commande.
- INDUS03 : enrichissement XLSX versionné et texte narratif ; ne pas changer les statistiques sans ticket explicite.
- INDUS04 : infobulles d'inventaire basées sur profil réel, tests de localisation et comparaisons.
- INDUS05 : retirer les anciennes commandes du chemin de production seulement après équivalence prouvée ; conserver historique par index d'archives, éviter des dizaines de nouveaux documents.

Documentation **vivante** : ce contrat, un guide utilisateur unique du pipeline (à créer INDUS02), un registre de production/rapport machine (à générer par pipeline). Les 37 docs historiques restent traçables mais ne dictent plus une procédure d'arme à arme.

## I. Git, revue et bavardage minimal

Branche master uniquement, un ticket = un commit atomique, sans force-push, pas de branches/PR, pas de git add . ni de remplacement des binaires locaux ; vérifier git status, git lfs status, delta des fichiers effectivement touchés avant tout commit. Ne jamais automatiser le push des .uasset.

Un lot de fabrication donne **un rapport consolidé** : états par arme, chemins affectés, tests TD04, validation PIE, raisons des bloqueurs. Un seul échange pour lancement, un seul pour obstacles réels, un seul pour recette/validation si le pipeline est sain. Aucun nouveau script, classe C++ ou document par arme normale.

**Critère de réussite industriel :** fabriquer une arme nouvelle autorisée avec 0 classe C++ spécifique, 0 script spécifique, 0 document spécifique et la même suite de tests paramétrée ; seules les données éditoriales, les ressources et les éventuelles mécaniques inédites font l'objet d'un travail additionnel identifié.
