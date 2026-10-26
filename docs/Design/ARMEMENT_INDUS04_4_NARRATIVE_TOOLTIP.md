# ARMEMENT-INDUS04.4 — Publication des descriptions narratives d'armes

**Statut : pipeline d'intégration + tests publiés sur master. Compilation UE5.5.4, tests et recette PIE en attente.**

## Diagnostic vérifié

Il **ne manque aucune liaison native de texte** dans le tooltip :
- `UGridInventorySlotWidget::GetTooltipView()` renseigne déjà `FGridItemTooltipView::Description = UGridItemDefinitionAsset::Description` ;
- `UGridItemTooltipWidget::PresentTooltipView()` transmet déjà cette valeur à `Text_Description` dans `WBP_ItemTooltip`, en rendant la section `Collapsed` lorsque le texte est vide ;
- `GetTooltipText()` (fallback textuel) inclut aussi ce champ.

**Le manque visible est un manque de contenu approuvé dans les DataAssets**, pas une ligne statistique ou un problème d'icône. Ne pas ajouter de nouveaux TextBlocks aux widgets natifs, ni dupliquer `Description` dans `WeaponProfile.Facts`.

## Source autoritaire et validation éditoriale

La source de vérité reste la feuille **`Narratif`** (13e onglet) du XLSX canonique `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`. Son export `Saved/ARMEMENT-INDUS/INDUS03_Narrative.csv` contient **82** lignes `NarrativeV1`, avec les champs :

| Champ | Consommateur | Publication |
|---|---|---|
| `DescriptionCourteFR` (max 300 caractères) | **Info-bulle `Text_Description`** via `UGridItemDefinitionAsset::Description` | **Seulement si `EtatValidation=APPROVED`** |
| `LoreFR` (max 4000) | Futur écran `PresentItemExamination` | **Non importé dans l'info-bulle** |
| `ParticulariteFR` et `EtatParticularite` | Suivi éditorial de mécanismes | **Non exécutable et non affiché** |
| `NotesConception` | Auteur uniquement | **Jamais exposé en jeu** |

**Le CSV de propositions `docs/Design/ARMEMENT_INDUS03_2_Narratif_DRAFT.csv` compte 82/82 textes marqués DRAFT, aucun APPROVED.** Ce fichier ne constitue pas une autorisation de publication. L'état du XLSX local peut différer ; la commande d'export doit fournir la vérité courante. Ne jamais passer automatiquement les 82 textes à APPROVED.

**Exemple : Grande Hache = `ARM031`, `DA_Weapon_GreatAxe_01`.** La proposition actuelle (DRAFT) de description courte est « Hache géante à deux mains, faisant reposer sa force sur une tête imposante. » ; le lore long et la propriété « Brise-armure » PLANNED ne doivent pas être présentés comme une compétence disponible. Relecture éditoriale nécessaire dans le XLSX avant toute application.

## Import contrôlé INDUS04.4

Nouvelle commande `Scripts/WeaponNarrativeApply.ps1` : **un seul archétype / un seul DataAsset** à la fois, jamais de publication en masse.

```powershell
cd D:\Development\GrimrockPrototype

# 1. Dans Excel, feuille Narratif : relire ARM031 et passer EtatValidation à APPROVED.
#    Enregistrer, fermer Excel.
# 2. Audit sans écriture de contenu :
.\Scripts\WeaponNarrativeApply.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM031

# 3. Après contrôle du rapport READY ; apply explicite :
.\Scripts\WeaponNarrativeApply.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM031 -Apply
```

Le wrapper :
1. vérifie `master`, le manifeste de production, l'existence du DataAsset et **refuse un fichier cible déjà modifié localement** ;
2. génère toujours `NarrativeV1` depuis le **XLSX**, jamais depuis `DRAFT.csv` ni un CSV externe fourni manuellement ;
3. exige le schéma exact à huit colonnes, 82 IDs canoniques uniques `ARM001..ARM082`, et `EtatValidation` valide ;
4. compile l'Editor via `Scripts/ValidateUE.ps1 -SkipAutomation`, puis appelle le commandlet `GridWeaponPipeline -Action=NarrativeItem` ;
5. en audit : renvoie `BLOCKED_REVIEW`, `READY`, `PRESENT_SAME` ou `CONFLICT` sans toucher aux assets ;
6. en `-Apply` : refuse **DRAFT**, **NEEDS_REVIEW**, texte vide, package déjà sale, ID/chemin discordants, asset inexistant ou contenu `Description` non vide **différent** (ne pas écraser une description personnalisée) ;
7. sur `READY` et **APPROVED** uniquement : écrit le `FText Description` de **cet unique `UGridItemDefinitionAsset`**, un seul package `.uasset`, sans toucher aux formules, niveaux N1–N20, visuels, actions ou LoreFR ;
8. sur `PRESENT_SAME` : succès idempotent, zéro écriture ;
9. contrôle `git status` avant/après pour détecter toute modification nouvelle hors du seul `.uasset` attendu. Il n'exécute aucun `git add/commit/push`.

**Une description déjà non vide et différente produit `CONFLICT` et n'est jamais remplacée.** La résolution se fait à la main après audit. Aucun objet `Planned` non créé ne peut être publié. Les quatre assets UI locaux restent intacts.

Rapport : `Saved/ARMEMENT-INDUS/INDUS04_4_Narrative_ARM031.csv` et log correspondant.

## Validation locale — avant toute approbation/apply

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.INDUS04.4"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.Item01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.WeaponTooltip04"
git status --short
```

Deux nouveaux tests vérifient le parsing strict `NarrativeV1` et la protection `APPROVED` / `PRESENT_SAME` / `CONFLICT` / `DRAFT`, avec accents et guillemets. Le test UI Item01 vérifie désormais explicitement `View.Description`.

La compilation et l'automatisation **n'ont pas été exécutées sur la machine Windows** dans ce commit : ne pas annoncer de résultats positifs avant réception des logs.

## Recette manuelle dans Unreal après import APPROVED

1. Fermer Unreal Editor pendant l'import, puis le rouvrir.
2. Ouvrir `DA_Weapon_GreatAxe_01` et vérifier `Description` identique à la ligne **ARM031 APPROVED** du XLSX (sans détails Lore ni pouvoir PLANNED).
3. Ouvrir `WBP_ItemTooltip` (racine), et vérifier la présence du **TextBlock existant `Text_Description` coché Is Variable**, avec `Auto Wrap Text` activé et une zone de largeur responsive en Designer. Ne pas éditer `WBP_ItemTooltipNativeLine` ni les comparaisons pour ce jalon.
4. En PIE : passer le pointeur sur la Grande Hache dans l'inventaire ; voir le texte descriptif entre l'en-tête et les statistiques, sans doublement, chevauchement ni troncature. Vérifier fermeture/réouverture et d'autres niveaux N1/N9 : **une seule description par archétype**, pas une description par niveau.
5. Vérifier un item Legacy décrit (comportement préservé) et un item sans description (zone repliée). Vérifier les autres champs, la Grande Hache à deux mains, les icônes PA et les deltas inchangés.

**Aucune édition binaire WBP autorisée depuis GitHub.** Si les textes sont trop longs en PIE, ajuster uniquement la présentation Designer en gardant la taille des icônes sous votre contrôle.

## Commit de contenu après approbation

La commande `-Apply` **modifie localement un .uasset**, mais ce dépôt ne dispose pas d'un mécanisme sûr pour publier à distance ce fichier binaire sans son exécution locale. Une fois les captures PIE vérifiées, committer sur `master` le **seul DataAsset de l'archétype traité** :

```powershell
git diff --name-only
git add -- Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.uasset
git commit -m "ARMEMENT-INDUS04.4-ARM031: publish approved Great Axe short description"
git push origin master
```

Ne jamais inclure les quatre assets locaux UI non concernés ni les CSV sous Saved/.

**Conclusion :** pipeline et contrat prêts ; la présence visible de chaque description en jeu dépend de l'approbation explicite dans le XLSX, d'un `-Apply` contrôlé, puis de la recette PIE. Les 82 DRAFT ne sont pas tacitement transformés en contenu publié.
