# ARMEMENT-INDUS01 — Audit de référence du système d'armement

**État : audit statique GitHub réalisé le 25 octobre 2026 sur master (HEAD initial 1f9cfe0997e47fd154ecb470d166cfd357307492).** Aucun asset Unreal, catalogue, niveau, WBP ni système de combat modifié par ce jalon. Ce document accompagne le contrat ARMEMENT_PIPELINE_CONTRACT.md et l'inventaire ARMEMENT_INDUS01_INVENTAIRE_ASSETS.csv.

## 1. Périmètre, méthodes, limites de preuve

- Référentiel audité : PhilippeMartin1964/GrimrockPrototype, branche master. Inspection de l'arbre Git et des sources C++/PowerShell/documentaires accessibles.
- Classeur canonique présent : docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx (blob 440200 octets dans l'arbre Git).
- **La lecture cellule par cellule du classeur binaire et la désérialisation des .uasset n'ont pas été effectuées** dans cet audit distant. Les contrôles ci-dessous sur le contrat XLSX sont dérivés de ses exporteurs/importeurs et du code existant, pas d'une analyse indépendante des cellules.
- **Aucun build UE5, test Automation ni PIE n'a été lancé par l'auteur de cet audit** ; les résultats antérieurs proviennent des logs communiqués par l'utilisateur. Ne pas confondre présence Git d'un fichier, tests de données et validation du gameplay.
- Le dernier git status --short communiqué par l'utilisateur était vide et le HEAD distant a été vérifié. Un état de travail propre ne garantit pas l'absence de divergence après le présent commit de documentation.

## 2. Inventaire quantifié

| Élément | Constat vérifiable | Commentaire |
|---|---:|---|
| Archétypes prévus | 82 | Contrat C++ de DA_WeaponCatalogue et exporteur |
| Profils fixes | 1 640 | 82 x N1–N20 |
| Profils passifs | 260 | Contrat actuel, à ne pas convertir en attaques fictives |
| DataAssets DA_Weapon_* versionnés | 10 | Fichiers présents, pas leur état runtime inspecté |
| StaticMeshes d'armes dans Meshes/Weapons | 10 | Chemins présents |
| Icônes d'armes dans UI/Icons/Weapons | 10 | Chemins présents |
| Instances matériau d'armes dans Art/Materials/Instances/Weapons | 10 | Chemins présents |
| Scripts Scripts/*ARMEMENT* ou *Weapon* | 12 | Plusieurs étapes historiques, pas 12 doublons équivalents |
| Fichiers C++ Editor au chemin/nom ARMEMENT | 43 | Public, Private, Tests compris |
| Documents docs/Design/ARMEMENT01* | 37 | XLSX inclus, beaucoup d'historique de jalons |

Le CSV annexe donne les **dix chemins d'items, meshes, icônes et MI réellement versionnés** : il s'agit de présences dans Git, non de preuves que les références soft/object internes pointent partout correctement.

Familles présentes : pilotes ARM001, ARM045, ARM075 ; Guerrier ARM014, ARM015, ARM025, ARM031 ; Poignard d'assassin ARM002 ; deux familles historiques Shuriken et ThrowingKnife dont l'ArchetypeId exact n'est **pas inféré** du nom de fichier. Le rapprochement des identifiants internes doit être fait en UE pendant INDUS02.

### Recettes localement attestées, uniquement d'après les journaux utilisateur

- ARM002 visuels : ARMEMENT01.11.2, rapport TD04-20261025-075614, 1/1, code 0.
- ARM002 DataAsset : ARMEMENT01.11.2.2, rapport TD04-20261025-080825, 2/2, code 0.
- ARM002 readiness : ARMEMENT01.11.1, rapport TD04-20261025-081202, 2/2, code 0. CSV : ARM002,true,true,true,1,DA_Weapon_AssassinDagger_01,Weapon_AssassinDagger_01,true,true,false,NeedsSpecialBehaviorReview.
- L'utilisateur a ensuite confirmé le fonctionnement en jeu (ramassage, inventaire, équipement, combat et persistance) pour ARM002.
- Les quatre armes Guerrier ont leur historique de recette et de tests dans ARMEMENT01_10_3_4_WARRIOR_PIE_ACCEPTANCE.md ; les cas limites non observés explicitement ne sont pas des PASS automatiques.
- Autres pilotes/objets legacy : statut détaillé à réévaluer via tests génériques, sans prétendre à un nouveau PASS.

## 3. Audit du contrat Excel -> Runtime

**Autorité actuelle :** classeur à 12 onglets, dont Catalogue et Niveaux 1-20. Scripts/ExportARMEMENT014Catalogue.ps1 suppose exactement les noms et l'ordre de ces onglets. Il exporte 19 champs normalisés dans Saved/ARMEMENT01/ARMEMENT01_4_Profiles.csv, puis GridWeaponCatalogueImportCommandlet.cpp impose le même en-tête.

Champs importés : ArchetypeId, ItemLevel, DisplayName, Family, HandUsage, AccuracyBonus, DamageType, PhysicalSubtype, ScalingAttribute, DiceCount, DiceSides, FlatBonus, PhysicalArmor, MagicalArmor, ActionPointCost, RangeCells, WeightKg, MinDamage, MaxDamage.

UGridWeaponCatalogueAsset::IsValidCatalogue rejette les profils manquants, doublons, invalides, mauvais niveaux, et le compte passif différent de 260. Son contrat est solide ; **il doit rester autorité unique des statistiques de combat et de progression**.

Faiblesses observées :
- Le schéma 12 onglets/19 colonnes est rigide et n'offre pas de projection de descriptions ni de lore.
- L'exporteur lit les caches de formules du XLSX ; la présence des formules ne garantit pas que le cache est recalculé. Rejeter tout cache manquant/périmé détectable ; journaliser hash et version.
- L'import actuel déclare des tableaux de profils mais ne matérialise pas les textes narratifs dans UGridItemDefinitionAsset::Description.
- Le manifest d'identités techniques et de chemins des ressources n'existe pas comme source générique ; les champs techniques sont recopiés dans des tableaux C++ spécifiques.

## 4. Redondances et dette d'outillage

Doubles workflows avérés :
- GridWarriorN1VisualRegistrationCommandlet.cpp (~326 lignes) et GridAssassinDaggerVisualRegistrationCommandlet.cpp (~326 lignes) : chargement visuels, contrôle texture sRGB, material slot, validation, insertion du binding, SavePackage.
- GridWarriorN1ItemAuthoringCommandlet.cpp (~295 lignes) et GridAssassinDaggerN1ItemAuthoringCommandlet.cpp (~292 lignes) : lecture profil N1, construction FGridCombatActionDefinition, garde doublon, création du DataAsset, sauvegarde.
- Des wrappers PowerShell propres aux groupes/armes : RegisterARMEMENT011021WarriorVisuals, RegisterARMEMENT011121AssassinDaggerVisual, AuthorARMEMENT01102WarriorN1, AuthorARMEMENT011122AssassinDaggerN1.

Les outils d'import initial des 1 640 profils, d'affinités, de palettes historiques et les tests spécialisés **ne sont pas automatiquement redondants**. Garder leur code jusqu'à preuve de non-régression ; n'envisager leur suppression qu'après un pipeline générique substitutif.

### Exceptions de nomenclature réellement détectées

- SM_Shuriken_01.uasset et SM_ThrowingKnife.uasset **ne suivent pas** SM_Weapon_<Stem>_01.
- T_Weapon_Shuriken.uasset et T_Weapon_ThrowingKnife.uasset **n'ont pas** le suffixe _01 des armes récentes.
- MI_Bow et MI_Shield ne s'obtiennent pas depuis le simple nom ShortBow/RoundShield.
- Les noms de fichiers ne prouvent jamais à eux seuls l'identité de l'archétype.
**Conclusion :** un manifeste avec références explicites est indispensable ; la dérivation naïve de chemins est interdite.

## 5. Audit des infobulles : lacune précise

- UGridInventorySlotWidget::GetTooltipView() fournit FGridItemTooltipView (titre, icône, ItemLevel, Description, poids, usage, bonus d'équipement et deltas).
- View.Description est lu depuis Definition->Description ; les authorings d'armes N1 examinés renseignent DisplayName et Weight mais **pas Description**.
- AppendDefinitionStats compare EquipmentStatBonus et EquipmentResistanceBonus ; **les dés de dégâts, le bonus de précision, le coût PA, la portée, le type de dégâts et l'attribut du FGridWeaponLevelProfile correspondant à l'instance ne sont pas projetés** dans le modèle existant.
- UGridItemDefinitionAsset::TryGetWeaponCatalogueProfile(ItemLevel, ...) et ResolveEquippedOffensiveProfile(...) permettent déjà une résolution au niveau réel. Les futurs champs tooltip doivent utiliser CachedItem.ItemLevel, pas le niveau de création.
- WBP_GridCombatTooltip est l'explication d'un **combat effectif** via sa trace ; ce n'est ni la source ni le bon endroit pour l'infobulle théorique d'inventaire.
- Les descriptions doivent rester FText du domaine de l'arme ; ST_UI ne doit contenir que les labels/patterns partagés, conformément à docs/UI_WBP_LOCALIZATION_WORKFLOW.md.

### Sévérités retenues

| Priorité | Constat | Suite |
|---|---|---|
| P0 | Duplication des commandlets et scripts de production | INDUS02 : moteur générique avec dry-run strict |
| P0 | Tooltip d'inventaire privé des profils offensifs N1-N20 | INDUS04 : un modèle de présentation partagé |
| P1 | Aucune description narrative projetée vers les DataAssets d'armes | INDUS03 puis INDUS04 |
| P1 | Contrat XLSX strict non versionné par schéma ; risques de cache de formules | INDUS03 : schéma explicite et validation |
| P1 | Exceptions réelles de noms de fichiers et types d'armes | INDUS02 : manifeste explicite + handlers typés |
| P1 | Modifications de palette/level sont des .uasset partagés | INDUS02 : préflight, simulation, sauvegarde sélective, validation UE |
| P2 | 37 documents de jalons mélangent règles actuelles et historiques | INDUS05 : index/archivage non destructifs |
| P2 | Critique de l'ombre ARM002 et autres aptitudes distinctes | Backlog gameplay dédié ; ne pas déclencher à partir d'un texte |

## 6. Risques de régression et garde-fous

- Les 260 profils passifs ne doivent pas recevoir un MeleeAttack artificiel.
- Ne jamais construire systématiquement une attaque MainHand pour les armes à distance, projectiles, boucliers et équipements à deux mains.
- Les GUID, ItemDefinitionId, ArchetypeId, niveaux N1-N20 et le sérialiseur de sauvegardes existants sont invariants.
- Aucun changement du comportement des 12 kits legacy, de la logique de combat, de la mobilité, de l'inventaire ou des WBP pendant INDUS01.
- Les contraintes de ressources PBR (sRGB, normal, ORM, dimensions, slot MI réel) restent obligatoires, même sous commande unique.
- Les références d'assets et transactions sur catalogues/palettes doivent être sûres et idempotentes. Ne jamais forcer une écriture sur un mapping existant différent.
- LFS activé pour .uasset/.umap/.png : vérifier avant commit binaire ; ne pas créer de faux fichiers binaires depuis GitHub ou PowerShell.
- Vérifier le comportement avec UE5.5.4 sur poste local. Ici aucun test local n'a été exécuté.

## 7. Fermeture du ticket INDUS01 et décision INDUS02

**Réalisé par INDUS01 :** audit distant des sources, inventaire des assets, identification des doublons et lacunes, règles de migration, critères de preuve. **Aucune migration et aucun nettoyage de code exécutés**.

**Prochain ticket : ARMEMENT-INDUS02** — commande publique générique + un service/commandlet Editor piloté par manifeste et tests de compatibilité (Rogue arme 1 main, Guerrier 2 mains, Bouclier passif, Arc, objets legacy). Commencer par comparaison/plan, sans modifier les dix assets existants. Migration destructive interdite avant preuve.

Tests UE locaux requis à l'ouverture d'INDUS02 : audit de cohérence catalogue/visuels/items/registre/palette + diff sur le dépôt avant/après un dry-run, rapport JSON/CSV dans Saved/ARMEMENT-INDUS, validation TD04.2 et recette PIE sur familles représentatives.
