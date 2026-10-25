# ARMEMENT-INDUS02 — Guide opérationnel du pipeline d'armement

**Statut : INDUS02.2 validé localement le 25 octobre 2026 : 8/8 COMPLETE, 0 conflit, 4/4 tests Industrial, aucune écriture et git propre (TD04-20261025-102213). INDUS03 introduit un contrat narratif Excel V1 et son exporteur en lecture seule ; sa validation locale reste à réaliser.**
Références : docs/Architecture/ARMEMENT_PIPELINE_CONTRACT.md (contrat cible), docs/Design/ARMEMENT_INDUS01_AUDIT.md (état de référence).

## 1. Objectif

Ne plus ajouter de script ou de commandlet C++ pour chaque arme normale.
Le seul point d'entrée public devient **Scripts/WeaponPipeline.ps1** et un seul
commandlet Editor **GridWeaponPipeline** gère les sources réellement déclarées dans
Config/Armement/WeaponProductionManifest.csv.

Ne remplacer aucune définition déjà produite. Les outils ARMEMENT01 historiques
restent disponibles le temps de démontrer l'équivalence et ne sont pas supprimés.

## 2. Manifeste : données, pas valeurs de combat

Le manifeste contient 16 colonnes fixes : ArchetypeId, ProductionState, BuildKind,
ItemDefinitionId, ItemPath, AttackId, VisualId, VisualFamily, IconPath, MeshPath,
MaterialPath, BaseColorPath, NormalPath, OrmPath, ClassId, EquipmentSlots.

- **Active** : les huit archétypes produits avec le modèle standard et des références explicites :
  ARM001, ARM002, ARM014, ARM015, ARM025, ARM031, ARM045, ARM075.
- **Planned** : ARM003 Stylet, avec conventions de chemins *proposées*.
  Ses fichiers graphiques ne sont PAS présumés existants, ni générés par le pipeline.
- **Legacy non encore migré** : les DataAssets Shuriken et ThrowingKnife. Ils sont
  détectés dans l'inventaire runtime si bUseWeaponCatalogue est activé, mais leurs
  identifiants et conventions spécifiques ne sont pas inventés. **Pas de migration
  automatique et pas de génération par le handler standard.**
- BuildKind autorisés : MeleeOneHand, MeleeTwoHand, Ranged, ShieldPassive.
  Les armes de lancer personnalisées restent hors autorisation d'écriture.
- Les ressources sont des **chemins UE complets exacts**, y compris les exceptions
  de noms des pilotes. Ne pas appliquer une transformation automatique Stem -> mesh.
- Aucune colonne de dés, dommages, poids, portée ou PA : toujours lire
  DA_WeaponCatalogue (N1 pour la définition, N1–N20 pour les instances).

## 3. Modèle de sécurité

Modes **Audit, Plan, Validate, Build sans -Apply** : lecture seule pour tout
Content/, code et données versionnés. Rapports CSV + JSON + log créés exclusivement
sous Saved/ARMEMENT-INDUS. Les hashes SHA256 de DA_WeaponCatalogue,
DA_WeaponVisualCatalogue et DA_ObjectPalette_Default sont contrôlés avant/après.

Build -Apply exige :
- exactement **un** ArchetypeId et une étape **Visual, Item ou Palette** ;
- un profil existant et valide dans le catalogue, art réelle validée (icône,
  mesh, MI sur slot, parent MI, BC sRGB ON, N/ORM sRGB OFF) ;
- aucune collision d'identité, de chemin ou de mapping ; aucune substitution de
  visuel et aucun écrasement d'un DataAsset existant ;
- Stage Visual : sauvegarde au maximum du seul DA_WeaponVisualCatalogue ;
- Stage Item : sauvegarde au maximum d'un seul nouveau DataAsset ;
- Stage Palette : sauvegarde au maximum du seul DA_ObjectPalette_Default, après une définition validée et un contrôle de collision d'EntryId et d'alias ;
- deuxième passage identique : PRESENT_SAME, aucune réécriture.

La séparation des **trois** écritures est intentionnelle : tant qu'une vraie transaction
multi-package sûre n'est pas démontrée, on ne présente pas un ensemble de SavePackage
comme atomique. Le script bloque tout changement Git supplémentaire non prévu.

**Non couvert par INDUS02.2 :** placement dans DA_GridLevel_00, sauvegarde UE des
matériaux et meshes, art 3D, mécanique spéciale, descriptions narratives,
infobulle offensive N1–N20. Leur production reste sous leurs contrats existants,
ou sera abordée dans les jalons ultérieurs sans toucher au runtime par surprise.

## 4. Premier contrôle local obligatoire — sans écriture

Fermer Unreal Editor, se placer sur master et vérifier que le dépôt est sain.

    cd D:\Development\GrimrockPrototype
    git branch --show-current
    git status --short
    git pull --ff-only origin master

Puis :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Audit -All

**Attendre le verdict réel**, ne pas affirmer le succès UE avant lecture du log.
Rapports :
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.csv
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.json
- Saved/ARMEMENT-INDUS/INDUS02_Audit_All.log

L'audit signale par arme COMPLETE, ART_MISSING, VISUAL_MISSING,
ITEM_MISSING, PALETTE_MISSING ou CONFLICT, puis LEGACY_UNMANAGED dans le log le cas échéant. La nouvelle colonne Palette distingue MISSING, PRESENT_SAME et CONFLICT.
Un Planned ARM003 manquant est un état attendu ; il n'est pas une arme jouable.

Ensuite lancer la validation des contrats communs :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All

Tests ciblés : Grimrock.RPG.ARMEMENT.Industrial (**4 tests**, dont palette en lecture seule, idempotence et conflits simulés en mémoire).
**Ne pas publier d'assets générés tant que ces tests n'ont pas réussi.**
Les tests vérifient le manifeste, l'autorité des profils N1 et les objets
déjà produits, sans PIE réel.

En cas d'échec, transmettre uniquement le SUMMARY, les RESULT bloquants et
le bilan des tests TD04.2 ; les longs logs n'ont pas à être recollés si les
lignes pertinentes suffisent. Corriger ce ticket dans un commit atomique
avant toute utilisation d'Apply.

## 5. Scénario d'une nouvelle arme — APRÈS validation du pipeline

Ajouter d'abord sa ligne unique au manifeste, créer en UE5 ses vrais
art assets selon les chemins explicitement déclarés, puis utiliser :

    .\Scripts\WeaponPipeline.ps1 -Action Plan -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003

Uniquement si la sortie indique Art=READY sans CONFLICT :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Visual -Apply

Relancer le préflight et contrôler le nouveau visuel :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003

Après enregistrement réel du visuel et confirmation de l'état ITEM_MISSING :

    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Item -Apply

Chaque Apply déclenche la suite de tests génériques. Aucun -Apply n'est
autorisé pour plusieurs armes. Un état CONFLICT ne peut jamais être
« réparé » en remplaçant arbitrairement l'asset existant.

Après Item, si le rapport indique PALETTE_MISSING, contrôler le plan puis
enregistrer **une seule nouvelle entrée**, sans modifier les entrées déjà
présentes :

    .\Scripts\WeaponPipeline.ps1 -Action Plan -Archetype ARM003
    .\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Palette -Apply

Les pilotes historiques ARM001/ARM045/ARM075 peuvent conserver leur
catégorie préexistante Items, mais toute nouvelle entrée est catégorisée
Weapon. Les doublons d'identifiant ou les entrées pointant vers un autre item
sont refusés. L'étape n'effectue **aucun placement dans le donjon**.

Compléter ensuite le placement dans l'éditeur Unreal avec le procédé
existant, et la recette PIE réelle : mesh/matériau, pickup unique,
infobulle, MainHand/OffHand, dépôt/récupération, combat PA/dégâts/portée,
sauvegarde/rechargement.

## 6. Critères de décision

### Validation locale

- Build Development Editor OK ; **4 tests Industrial** sans avertissement ni échec.
- Audit des huit lignes Active ; chaque CONFLICT/ART_MISSING documenté,
  jamais remplacé en silence.
- ARM003 Planned n'est pas interprété comme objet produit.
- Un audit sans Apply ne modifie aucun .uasset, .umap, WBP ni catalogue.
- Les anciens objets de lancer ne sont jamais réécrits par un handler
  généralisé ne sachant pas les gérer.

### Publication

Un ticket = un commit atomique sur master. Jamais de force push,
jamais de branche/PR, jamais de git add . pour masquer un conflit.
Ne pas publier de données Unreal produites à la place d'un rapport de
validation local. Les tests automatiques ne remplacent pas le contrôle PIE.


### Garde-fous spécifiques à la palette (INDUS02.2)

La palette est partagée par toutes les catégories d'objets. Le contrôleur
prend une **copie en mémoire** des entrées, y ajoute au maximum une entrée
\`EntryId=ItemDefinitionId, PaletteCategory=Weapon, DefaultItemDefinition=Item\`
et rejette les candidats invalides via \`ValidatePalette\` avant \`SavePackage\`.
Il **ne modifie jamais** les entrées préexistantes, même pour réparer une
catégorie : ce serait un autre ticket. Une catégorie \`Items\` préexistante
n'est admise que pour ARM001, ARM045 et ARM075, conformément à leurs
tests pilotes historiques. Les entrées déjà identiques produisent
\`PRESENT_SAME\` sans sauvegarde.

**Ne pas lancer -Apply avant réussite des quatre tests et du contrôle des
champs Palette dans les huit lignes actives.** Exemple de premier test local :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All
    git status --short

Résultat attendu : huit COMPLETE ; un Planned ARM003 ART_MISSING ;
zéro CONFLICT ; zéro SavedPackages ; **4/4** tests Industrial réussis ;
aucune modification Git. En cas d'écart, communiquer uniquement RESULT
non COMPLETE, SUMMARY et Automation summary. L'action Validate
**ne tente aucune écriture sur la palette**.

Pour les futures modifications binaires, inspecter la palette sous UE5
avant le commit ; un commit de code ne constitue pas une validation de
contenu.

## INDUS03.2 — Propositions éditoriales des 82 archétypes (DRAFT)

Les 82 textes de `DescriptionCourteFR` et `LoreFR` ont été rédigés en un seul lot de relecture, en prenant pour référence les 82 lignes de l'onglet historique `Catalogue` (noms, familles et colonne Particularité). Source versionnée : `docs/Design/ARMEMENT_INDUS03_2_Narratif_DRAFT.csv`.

**Toutes les lignes restent DRAFT**. Les récits proposés sont des ajouts créatifs au lore du jeu, pas des faits déjà adoptés dans la fiction du projet. Les mécanismes spéciaux ne sont jamais marqués IMPLEMENTED par supposition. Les valeurs de combat, le XLSX canonique, les DataAssets, le runtime et la palette ne sont pas modifiés dans ce commit.

Pour intégrer ces propositions après relecture : ouvrir le XLSX canonique, puis coller les sept colonnes A:G du CSV proposé dans l'onglet `Narratif` (A1:G83), sans créer d'onglet supplémentaire. Fermer Excel et lancer `WeaponPipeline.ps1 -Action Narrative`. Attendre `Archetypes=82 WithDescription=82 Approved=0` jusqu'à approbation éditoriale. Ne pas passer un élément à APPROVED sans relecture.

---

## INDUS03 — Enrichissement narratif sans duplication des statistiques

La source éditoriale reste
\`docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx\`.
Elle doit contenir soit les **12 onglets historiques**, soit ces douze onglets
**inchangés et dans le même ordre**, suivis de \`Narratif\` en treizième.
Les 19 colonnes de statistiques et les 1 640 lignes N1–N20 ne changent pas.

Un modèle d'onglet Narratif prêt à copier est livré à l'utilisateur et un
modèle CSV conservé sous
\`docs/Design/ARMEMENT_INDUS03_Narratif_Modele.csv\` comme **exemple de
schéma non autoritatif**. Il ne doit pas être chargé directement par le jeu.
Pour rester fidèle à la source unique, copier l'onglet Narratif du modèle
dans le **classeur canonique**, en dernière position, puis enregistrer.

### Colonnes exactes de la feuille Narratif

| Colonne | Nom | Règle |
|---|---|---|
| A | ArchetypeId | 82 lignes ARM001..ARM082, sans doublons ; ordre libre |
| B | DescriptionCourteFR | Pour l'infobulle de l'inventaire ; facultatif en DRAFT, maximum 300 caractères |
| C | LoreFR | Pour l'écran d'examen futur, facultatif, max. 4 000 caractères |
| D | ParticulariteFR | Narration uniquement, max. 300 caractères ; ne déclenche jamais une mécanique |
| E | EtatParticularite | UNDOCUMENTED, PLANNED, IMPLEMENTED, NOT_APPLICABLE |
| F | NotesConception | Notes auteur non exposées, max. 2 000 caractères |
| G | EtatValidation | DRAFT, NEEDS_REVIEW ou APPROVED |

La première ligne contient les en-têtes, les lignes 2..83 les données.
**APPROVED** nécessite une description courte non vide. **IMPLEMENTED**
nécessite un texte de particularité et, **séparément**, une validation
effective du comportement en jeu ; l'exporteur ne peut pas prouver celle-ci.

Ce sont des **textes éditoriaux indépendants des statistiques**. Le nom
affiché n'est pas dupliqué : il reste dans la colonne B de Catalogue.
Aucune description n'a été inventée ni automatiquement approuvée.

### Exécution du nouvel export en lecture seule

Sur un dépôt propre, après avoir enregistré le 13e onglet dans le XLSX
canonique et fermé Excel :

    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Narrative
    git status --short

Le script existant \`ExportARMEMENT014Catalogue.ps1\` est réutilisé.
Il produit le CSV historique de 19 colonnes
\`Saved/ARMEMENT01/ARMEMENT01_4_Profiles.csv\`, toujours 1640 profils,
**et** le nouveau
\`Saved/ARMEMENT-INDUS/INDUS03_Narrative.csv\`, 82 lignes au schéma
\`NarrativeV1\`. Le mode \`-Action Narrative\` exige le 13e onglet :
il ne transforme pas en silence l'ancienne version à 12 onglets.

Le CSV exporté contient \`SchemaVersion\`, \`ArchetypeId\`,
\`DescriptionCourteFR\`, \`LoreFR\`, \`ParticulariteFR\`,
\`EtatParticularite\`, \`NotesConception\`, \`EtatValidation\`.
Seules les données de la feuille Excel font foi. Pour l'instant,
**aucun runtime asset n'est modifié** : la projection vers
\`UGridItemDefinitionAsset::Description\`, puis vers le modèle commun
\`FGridItemTooltipView\`, appartient aux jalons suivants.

**Preuves attendues** : export de l'ancien classeur 12 onglets inchangé ;
export NarrativeV1 (82/82) avec 13 onglets ; 1 640 profils et
260 profils passifs conservés ; \`git status\` reflétant seulement la
modification volontaire du XLSX, pas une altération des .uasset.
Aucune commande \`-Apply\` pour ce jalon narratif.

---


## INDUS04.1 — Projection des profils d'armes dans les infobulles (C++)

**Statut : code source publié, validation UE5.5.4 locale requise.**
L'export INDUS03.2 du classeur a été confirmé localement : 82/82
descriptions, 1640 profils, 260 passifs ; tous les textes restent
DRAFT, et le XLSX canonique est encore modifié localement jusqu'à
son commit séparé.

### Frontière de responsabilité

- \`UGridItemDefinitionAsset::TryGetWeaponCatalogueProfile(ItemLevel, Profile)\`
  demeure l'autorité des données RPG. On utilise **le niveau réel de
  \`FGridItemInstance\`**, pas le niveau par défaut ni l'action N1 authorée.
- \`FGridWeaponTooltipProjection::Build\` est un calcul de présentation
  **en lecture seule** ; ne crée aucune attaque, ne calcule pas de dégâts
  futurs et n'applique aucun bonus de personnage.
- \`FGridItemTooltipView::WeaponProfile\` est un sous-modèle indépendant :
  \`ArchetypeId\`, \`ItemLevel\`, \`bPassive\`, \`DiceText\`,
  \`DamageTypeText\`, \`ScalingAttributeText\`, \`FamilyText\` et
  \`Facts\` (chaque \`FGridWeaponTooltipFact\` contient un \`StatId\` stable,
  \`Label\` et \`ValueText\`).
- Les faits offensifs affichent le vrai NdM+K, les dégâts min/max dérivés
  des dés, la précision, le coût PA, la portée, le type de dégâts et
  l'attribut de scaling. Les passifs n'affichent **ni dés fictifs,
  ni portée, ni PA**. L'armure N1–N20 des boucliers utilise le niveau
  de l'instance.
- Les bonus \`EquipmentStatBonus\` historiques restent inchangés et
  indépendants des faits du profil. **Ne pas sommer** leur armure avec
  l'armure du profil dans l'UI. Les comparaisons offensives N1–N20
  restent un travail ultérieur : attention au sens du delta PA (moins
  est mieux).
- \`GetTooltipText()\` inclut les faits comme fallback texte. Le WBP
  structuré \`WBP_ItemTooltip\` / \`WBP_ItemTooltipStatLine\` n'est
  **pas édité** par ce jalon. Son branchement aux champs \`WeaponProfile\`
  devra être effectué manuellement dans UE5, avec captures pour
  contrôle visuel.
- L'export narratif du classeur reste indépendant. Aucun texte
  \`DRAFT\` n'est copié dans \`UGridItemDefinitionAsset::Description\`
  ni affiché comme capacité réellement disponible.

### Recette locale (Unreal Editor fermé)

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.UI.WeaponTooltip04'
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.UI.Item01'
    git status --short

Attendu : build Development Editor OK, **3/3** tests WeaponTooltip04,
régression Item01 inchangée, aucune modification de .uasset, .umap
ou WBP. Le classeur XLSX encore modifié localement est un changement
indépendant attendu. Ne pas annoncer le WBP terminé avant son
raccordement UMG et sa validation PIE.


## INDUS04.2 — Comparaisons d'armement à niveau fixe et contrat UMG

**Statut : C++ source publié ; validation locale et câblage manuel UMG en attente.**
INDUS04.1 : trois tests WeaponTooltip04 et cinq tests Item01 réussis
le 25 octobre 2026, sans warnings (rapports TD04-20261025-133531
et TD04-20261025-133607).

### Nouveaux champs du modèle partagé

\`FGridItemTooltipEquipmentComparison::WeaponFacts\` est une liste séparée
de \`StatLines\`, précisément pour éviter de mélanger les anciens bonus
\`EquipmentStatBonus\` avec les profils d'armes N1–N20.
Chaque \`FGridWeaponTooltipComparisonFact\` expose :
\`StatId\`, \`Label\`, \`CandidateText\`, \`EquippedText\`, \`Delta\`,
\`bHasDelta\`, \`DeltaState\`, \`DeltaText\`.

\`FGridWeaponTooltipProjection::BuildComparison\` utilise deux
\`FGridItemInstance\` réelles avec leurs propres \`ItemLevel\`.
C'est une **projection en lecture seule**, sans ajout de dégâts.
Elle compare les vrais \`NdM+K\` comme formules textuelles,
les bornes de dégâts min/max, la précision, les PA, la portée,
les types de dégâts et les attributs.
Les dés, types et attributs sont textuels : **aucun delta numérique**.
Les bornes de dégâts min/max reçoivent un delta colorable lorsque
le type de dégâts et, dans le cas physique, le sous-type sont identiques.
**L'attribut de progression peut différer**, car min/max comparent les
dés bruts \`NdM+K\`, avant application de Force/Dextérité du personnage.
Ainsi \`1d4+3\` (Force, perforant) contre \`1d4\` (Dextérité,
perforant) produit \`+3\` favorable aux bornes min/max. Ce delta
n'est PAS une estimation des dégâts réels finaux.
Les dégâts de nature différente restent lisibles mais sans delta
chiffré trompeur. Pour \`WeaponAP\`, **un delta négatif est favorable** :
par exemple 2 PA contre 3 PA => \`Delta=-1\` et \`DeltaState=Positive\`.
Le vert/rouge doit être fondé sur \`DeltaState\`, jamais seulement sur
le signe du chiffre. Un delta nul reste neutre.

Les boucliers passifs ne comparent que l'armure physique et magique du
niveau réel ; jamais de dés ou PA fictifs. Les paires de rôle offensif
contre rôle passif sont incomparables. Une main vide n'invente pas une
attaque sans arme : la comparaison offensive reste absente, mais
la comparaison générique des bonus et le libellé de slot vide restent.
Aucun champ du gameplay ou de la sauvegarde n'est modifié.

### Exécuter les tests avant toute modification WBP

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.UI.WeaponTooltip04'
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.UI.Item01'
    git status --short

**Attendu : 5/5 tests WeaponTooltip04, 5/5 tests Item01, zéro
avertissement, zéro échec, aucun .uasset changé.**
Le XLSX canonique est volontairement modifié par INDUS03.2 ; le
committer dans son **propre ticket/commit**, sans l'inclure dans le commit
de l'infobulle ni le remplacer ou le réinitialiser.

### Raccordement UMG Designer (manuel, après les tests)

Ne **pas** modifier directement le binaire WBP depuis des scripts ou C++.
Avant édition, prendre une capture du Designer et une du Graph de
\`WBP_ItemTooltip\`, en montrant la construction des sections et des lignes
de comparaison. L'UI préexistante utilise
\`WBP_ItemTooltipStatLine\` (bonus génériques) et
\`WBP_ItemTooltipComparisonRow\` (slots) ; leurs noms sont connus du
dépôt mais l'implémentation de leurs graphes Blueprint n'est pas
observable en texte Git.

Conserver les zones actuelles titre/icône/description/poids/bonus.
Ajouter une section affichée si \`TooltipView.WeaponProfile.bHasProfile\`
et une liste répétée sur \`TooltipView.WeaponProfile.Facts\`, avec
\`StatId\` comme clé stable pour les icônes, \`Label\` et \`ValueText\`
fournis par C++. Dans la section de comparaison par slot existante,
itérer \`EquipmentComparisons[].WeaponFacts\`. Présenter
\`CandidateText\` et \`EquippedText\` ; afficher \`DeltaText\`
seulement lorsque \`bHasDelta\` est vrai, appliquer la teinte favorable/
défavorable d'après \`DeltaState\`. Ne **pas** remettre de delta
sur les dés ni considérer \`WeaponAP=-1\` comme un malus.

Pour les boucliers, veiller à ne pas imprimer deux fois l'armure
du profil et le bonus statique ARM075 : le \`WeaponProfile\` au niveau
réel est l'autorité de l'armure N1–N20, et les \`StatLines\`
historiques doivent être conservées pour les équipements sans catalogue.
Ce point requiert une décision de présentation à partir du Graph
réel de \`WBP_ItemTooltip\`, pas une suppression aveugle d'un champ C++.

**Validation PIE visuelle requise** : ARM002 N1 puis N9, remplacement
N1 par N9, ARM075 N1/N12, slot vide, comparaison avec une arme de
type de dégâts différent, puis objet legacy sans catalogue.
Captures avant/après et jeu en cours sont nécessaires avant de
déclarer INDUS04.2 complet côté WBP.

---

## ARMEMENT-INDUS04.2A — Présentateur natif + localisation centrale

**Statut : source C++/CSV publiée ; compilation, matérialisation ST_UI et Automation LOCALES EN ATTENTE.**
INDUS04.2A prépare le remplacement de l'ancien Event Graph, sans toucher
aux .uasset existants, au classeur XLSX narratif, aux DataAssets ou au gameplay.

### Autorités, sans doublons

- **Vocabulaire générique et formats partagés** : \`Localization/Source/UI.csv\`
  reste la source de vérité. \`FUILocalizationText::Get\` utilise le vrai
  \`/Game/Localization/StringTables/ST_UI\` matérialisé par Unreal.
  Les clés \`Inventory.Tooltip.Weapon.*\` et \`Inventory.Tooltip.Fallback.*\`
  contiennent les libellés de dégâts/précision/PA/armure, les catégories
  élémentaires explicites, le titre des sections et les formats de secours.
  Les attributs existants réutilisent \`CharacterCreation.Attribute.*\`.
- **Nom/description de l'objet** : \`FText\` du DataAsset. Les 82 descriptions
  narratives \`DRAFT\` ne sont pas encore publiées dans les assets.
- **N1–N20 et comparaison** : \`GetTooltipView\` /
  \`FGridWeaponTooltipProjection\`. Aucune lecture du niveau 1 comme
  substitut au niveau réel de l'instance ; aucun calcul du gameplay dans UMG.
- **Présentation native** : \`UGridItemTooltipWidget\` dérive de \`UUserWidget\`.
  \`PresentFromInventorySlot\` (nouveau nom distinct de la fonction BP
  \`InitFromInventorySlot\`) lit une seule fois le modèle C++ ; \`PresentTooltipView\`
  alimente les TextBlocks dynamiques et gère les sections/lignes.
  \`UGridItemTooltipWeaponLineWidget\` fournit \`PresentFact\` et
  \`PresentComparison\`. Les champs textuels et les comparaisons utilisent
  \`FText\` et les modèles existants, pas des nouveaux calculs.
- **Designer** : les futurs WBP resteront propriétaires du layout, des
  icônes, polices, marges et couleurs. Il y a trois TextBlocks distincts
  \`Text_DeltaPositive\`, \`Text_DeltaNegative\`, \`Text_DeltaNeutral\`,
  chacun stylé indépendamment. Le C++ n'applique pas de teinte ;
  il choisit la visibilité selon \`DeltaState\`.
- **Graph** : l'ancien \`InitFromInventorySlot\` et ses appelants ne sont
  **pas modifiés ici**. Le remplacement du Graph après reparentage,
  et la migration de ses anciennes lignes de stats/comparaison qui
  conservent aujourd'hui une gestion d'icônes, appartiennent à 04.2B.
  Ne pas supprimer les boucles BP ni les assets de ligne à cette étape.

### Nouveau contrat Designer (pour 04.2B, PAS à faire maintenant)

Parent futur du WBP actuel : \`UGridItemTooltipWidget\`.
Bindings natifs optionnels correspondant aux composants actuels :
\`Text_ItemName\`, \`Text_ItemType\`, \`Text_Description\`,
\`Text_Quantity\`, \`Text_Weight\`, \`Text_Equipment\`, \`Text_Light\`,
\`Text_ItemLevel\`, \`Image_ItemIcon\`.

Les deux sections supplémentaires seront créées dans le Designer :
\`VerticalBox_WeaponSection\` avec \`VerticalBox_WeaponFacts\` ;
\`VerticalBox_WeaponComparisonSection\` avec \`VerticalBox_WeaponComparisons\`.
Un titre \`Text_WeaponTitle\` pourra référencer DIRECTEMENT
\`ST_UI: Inventory.Tooltip.Weapon.Title\` : il est statique,
**ne jamais le passer aussi dans un SetText() C++**.

Un WBP de ligne, parent \`UGridItemTooltipWeaponLineWidget\`, contiendra
\`Text_Label\`, \`Text_Value\`, \`Text_Equipped\`, \`Text_Slot\`,
\`Text_DeltaPositive\`, \`Text_DeltaNegative\`, \`Text_DeltaNeutral\`.
Une classe unique est fournie à \`WeaponLineWidgetClass\` dans les
Class Defaults du futur WBP parent. Le code natif crée **la ligne complète
pré-définie dans le Designer**, sans fabriquer de TextBlock.
Pour les lignes de bonus historiques, conserver l'affichage et les
icônes actuels jusqu'à migration vérifiée ; ne pas faire disparaître
les icônes 48×48, les deltas et les comparaisons actuelles.
Éviter une double ligne d'armure pour les boucliers passifs.

### Ordre STRICT de validation locale 04.2A

1. Fermer Unreal Editor. Le fichier XLSX canonique modifié localement
   pour INDUS03.2 reste hors de ce ticket ; ne pas \`checkout\`, \`restore\`
   ni \`reset\` ce fichier.
2. Récupérer le commit source par \`git pull --ff-only origin master\`.
3. **Matérialiser et tester la localisation** :

       .\Scripts\Localization\RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5

   Ce workflow réutilise le commandlet \`UIStringTableAuthoring\`,
   le target \`Game\`, ses PO/locres et le smoke fr/en existants.
   Ne JAMAIS référencer dans un WBP une nouvelle clé seulement présente
   dans le CSV. Ouvrir \`ST_UI.uasset\` dans Unreal et confirmer la présence
   des clés avant tout changement de Designer.
   Le smoke anglais ne traduit pas automatiquement les nouvelles clés :
   vérifier les traductions éditoriales séparément.
4. **Tester le C++ après matérialisation** :

       .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.WeaponTooltip04"
       .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.Item01"
       git status --short

   Attendu : **6/6 tests WeaponTooltip04**, **5/5 Item01**, 0 warning/échec.
   Le nouveau test \`NativePresenterAndLocalization\` vérifie des clés
   du véritable ST_UI, la présentation de niveau et la polarité de delta.
   La commande de localisation peut produire un changement volontaire
   de \`ST_UI.uasset\` : **ne pas le restaurer** avant les tests et ne pas
   mélanger le commit du binaire avec le XLSX non publié.
5. Les C++ et les tests validés ne démontrent PAS un rendu WBP. Celui-ci
   ne sera revendiqué qu'après reparentage et captures PIE dans 04.2B.

### Textes non encore convertis

\`GridInventorySlotWidget::GetTooltipText\` compose désormais les
lignes \`Quantité\`, \`Poids\`, \`Équipement\`, le profil et les
comparaisons par des clés de format \`Inventory.Tooltip.Fallback.*\`.
Les anciennes valeurs \`WeightText\` produites par les règles d'inventaire
et certaines chaînes de compatibilité d'emplacements restent inchangées :
pas de changement d'arrondi ou de locale numérique silencieux dans
le même ticket. \`FamilyText\` n'affiche plus directement le libellé
interne (non localisé) du tableur : l'autorité d'une future traduction
des familles devra être décidée sans inventer de valeur affichée.

---

## ARMEMENT-INDUS04.2B.1 — Lignes legacy et comparaisons intégrées en C++

**État : code source publié sur master ; build + tests UE5.5.4 à confirmer sur le poste local.**
ARMEMENT-INDUS04.2A (script de localisation et 6/6 WeaponTooltip04 + 5/5
Item01) a été validé localement le 25 octobre 2026. Les sept changements
binaires/Excel conservés localement ont été poussés séparément ensuite ;
HEAD de départ de B.1 : \`5d5da76c16ea89e472855c90c57dba1aeb592bfb\`.

### Pourquoi compléter le natif AVANT de supprimer les nœuds BP

L'ancien \`WBP_ItemTooltip\` affiche 18 bonus/résistances au moyen de
\`FGridItemTooltipStatLine\`, puis des comparaisons par \`EquipmentSlot\`.
04.2A ne projetait que \`WeaponProfile.Facts\` et les comparaisons
\`WeaponFacts\` séparées, donc reparent/suppression du Graph à ce stade
aurait fait **disparaître les anciennes informations**.

B.1 ajoute les fonctionnalités en C++ sans changer de WBP :
- \`UGridItemTooltipWeaponLineWidget::PresentLegacyStat\` : affiche label,
  valeur, delta déjà formaté et couleur **sémantique par choix d'un des
  trois Text Blocks du Designer**. Pas de formatage BP.
- \`UGridItemTooltipWeaponLineWidget::PresentFact/PresentComparison\` :
  conserve les informations N1–N20 de 04.2A et les deltas PA inversés.
  Chaque ligne peut afficher un \`Image_StatIcon\` piloté par
  \`StatId\` et une \`StatIconTextures\` Map dans les Class Defaults
  du *WBP enfant*, sans forcer de style, couleur ou dimension dans C++.
  L'icône absente est \`Hidden\` pour préserver la taille authored, pas
  un placeholder blanc. Conserver 48×48 dans \`SizeBox_StatIcon\`.
- \`UGridItemTooltipEquipmentComparisonWidget\` : construit les lignes
  classiques ET \`WeaponFacts\` **dans le même groupe par emplacement**.
  Les objets sans profil, les mains vides et les slots équipés conservent
  leurs états et deltas existants.
- \`UGridItemTooltipWidget\` : exploite les conteneurs existants
  \`VerticalBox_StatLines\` et \`VerticalBox_Comparisons\`, et le
  \`VerticalBox_WeaponFacts\` ajouté en Designer. Plus de \`ForEach\`,
  \`CreateWidget\`, \`SetText\` ou \`Branch\` de présentation dans le Graph.
  Les éventuels anciens conteneurs de comparaison d'armes séparés restent
  masqués pour éviter de répéter les mêmes lignes.
- Les boucliers passifs dont le profil réel contient
  \`WeaponPhysicalArmor\` n'affichent pas une seconde ligne
  \`Armor\` issue des anciens bonus statiques. Les autres lignes et
  les équipements sans catalogue sont conservés.
- Les méthodes renvoient \`false\` lorsque les conteneurs/variables
  indispensables du Designer manquent ; **ne pas** supprimer le vieux
  Graph tant que le nouveau chemin n'est pas effectivement fonctionnel.
  Aucun asset ni gameplay n'est écrit par les présentateurs.

### Recette C++ préalable au reparentage

UE fermé :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.WeaponTooltip04"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.Item01"
    git status --short

Attendu **8/8** pour \`WeaponTooltip04\` (6 existants + 2 tests
\`NativeLegacyStatsPreserved\`, \`NativeShieldArmorSingleAuthority\`) et
**5/5** pour \`Item01\`, zéro échec et avertissement nouveau.
Les tests C++ ne remplacent **pas** le test PIE des WBP.

### ARMEMENT-INDUS04.2B.2 — Passage UMG MANUEL, pas de binaire via GitHub

**GATE : après les tests, ouvrir UE et vérifier les variables. Une capture
du Designer et du Graph existants est nécessaire avant la suppression
des boucles. Sauvegarder le WBP dans un commit distinct après PIE.**

1. Créer le WBP \`WBP_ItemTooltipNativeLine\` (parent C++ :
   \`GridItemTooltipWeaponLineWidget\`). Préparer dans le Designer les
   \`Text_Label\`, \`Text_Value\`, \`Text_Equipped\`, \`Text_Slot\`,
   \`Text_DeltaPositive\`, \`Text_DeltaNegative\`,
   \`Text_DeltaNeutral\` et \`Image_StatIcon\` (facultatif).
   Tous les noms sont EXACTS, \`Is Variable\` coché.
   \`Image_StatIcon\` dans un SizeBox 48×48, \`Hidden\` si icône inconnue.
   Régler les trois couleurs distinctes et les fonts uniquement ici.
   Dans **Class Defaults > StatIconTextures**, recopier les 18 paires
   \`StatId : Texture2D\` définies auparavant dans
   \`WBP_ItemTooltipStatLine::StatIcons\` (sans déplacer ni supprimer
   l'ancien WBP). Laisser les contenus Text dynamiques vides pour
   éviter \`TextBlockDefaultValue\` au Gather.
2. Créer \`WBP_ItemTooltipNativeComparison\` (parent C++ :
   \`GridItemTooltipEquipmentComparisonWidget\`), avec
   \`Text_EquipmentSlot\`, \`Text_EquippedItem\`,
   \`VerticalBox_ComparisonStats\` et
   \`VerticalBox_ComparisonWeaponFacts\`. Optionnel :
   \`VerticalBox_ComparisonWeaponSection\` enveloppant le
   titre localisé \`ST_UI: Inventory.Tooltip.Weapon.ComparisonTitle\`
   et les weapon facts. Réglages dans Designer seulement.
3. **Ouvrir l'asset existant**
   \`/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltip\`.
   Dans *Class Settings > Parent Class*, sélectionner
   \`GridItemTooltipWidget\` ; ne pas créer un deuxième tooltip racine.
   Contrôler les noms et types des champs dynamiques :
   \`Text_ItemName\`, \`Text_ItemType\`, \`Text_Description\`,
   \`Text_Quantity\`, \`Text_Weight\`, \`Text_Equipment\`,
   \`Text_Light\`, \`Text_ItemLevel\`, \`Image_ItemIcon\`.
   Certains sont nécessaires même si leur visibilité est \`Collapsed\`.
   Éviter tout binding de texte BP concurrent au \`SetText\` natif.
4. Conserver le Designer et les conteneurs préexistants
   \`VerticalBox_StatLines\` et \`VerticalBox_Comparisons\`.
   Ajouter si absents \`VerticalBox_WeaponSection\` avec
   \`VerticalBox_WeaponFacts\`. Le titre statique
   \`Text_WeaponTitle\` pointe directement sur la clé
   \`ST_UI: Inventory.Tooltip.Weapon.Title\`. Les wrappers
   \`VerticalBox_StatsSection\`, \`VerticalBox_ComparisonSection\`
   sont optionnels mais recommandés pour masquer les sections vides.
   Ne pas recréer le panneau de comparaison N1–N20 à la racine :
   les lignes se trouvent déjà dans chaque groupe par équipement.
5. Dans \`WBP_ItemTooltip > Class Defaults\` :
   \`WeaponLineWidgetClass = WBP_ItemTooltipNativeLine\` ;
   \`ComparisonRowWidgetClass = WBP_ItemTooltipNativeComparison\`.
6. **Conserver la fonction Blueprint publique EXISTANTE**
   \`InitFromInventorySlot\` et sa signature pour ses appelants.
   Dans son corps, remplacer les anciennes constructions par
   **un seul appel** à \`PresentFromInventorySlot(InInventorySlot)\`
   (le paramètre d'entrée est celui de la fonction BP).
   Ce n'est pas une nouvelle boucle ni un Tick : tout le Graph de
   projection devient une simple délégation native. Si la compilation
   expose une variable BP incompatible, arrêter et inspecter le
   graph au lieu de supprimer/renommer des éléments au hasard.
7. Compiler et enregistrer les trois WBP. PIE : survol d'un item
   legacy (bonus et 18 icônes), puis ARM002 N1 et N9, comparaison
   équipée puis main vide, ARM075 N1 et N12 (armure une seule fois),
   objet sans catalogue et différences de type de dégâts. Vérifier
   les couleurs PA (delta -1 => vert), les dés textuels, l'affichage
   français/anglais, les bordures, la taille 48×48 et la réouverture
   répétée sans accumulation de lignes. Après contrôle visuel,
   enregistrer seulement les WBP volontairement modifiés et
   recueillir un journal de tests. Ne pas sauvegarder des assets
   potentiellement cassés en bloc.

**État 04.2B** : C++ B.1 versionnable et vérifiable localement ;
la migration des \`.uasset\` B.2 et la recette PIE doivent être faites dans
Unreal par l'utilisateur. Les tests automatiques seuls ne la clôturent pas.

---

### ARMEMENT-INDUS04.2C.1 — Deltas positifs et comparaisons par main

La capture PIE du 25 octobre 2026 confirme le rendu des lignes.
Le delta positif était absent pour pic de guerre ARM025
(Force, perforant, 1d4+3) contre poignard d'assassin ARM002
(Dextérité, perforant, 1d4), car le filtre des dégâts bruts exigeait
abusivement le même attribut de progression. Le correctif permet
un delta vert +3 pour les bornes min/max, mais conserve les vérifications
de nature des dégâts. Un test Automation positif +3 et le test
d'incompatibilité de type restent prévus. La teinte appartient au
\`Text_DeltaPositive\` du Designer.

La sélection des **comparaisons d'emplacements** parcourt
\`UGridItemDefinitionAsset::CompatibleEquipmentSlots\`, pas la seule
classification « une main ». Le manifeste canonique
\`Config/Armement/WeaponProductionManifest.csv\` indique :
- \`ARM001\` dague : \`MainHand;OffHand\` : peut remplacer le bouclier
  en main secondaire ;
- \`ARM025\` pic de guerre : \`MainHand\` seulement : le bouclier
  d'\`OffHand\` reste équipé ;
- \`ARM075\` bouclier rond : \`OffHand\`.

N'ajouter pas arbitrairement \`OffHand\` au pic pour rendre visible
une comparaison ; cela constitue un changement des règles d'équipement,
des DataAssets et du combat, hors de cette correction de tooltip.

**Statut : source modifiée, compilation et tests UE5.5.4 à confirmer.**
Aucun DataAsset ou WBP n'est édité dans ce correctif.

---

## 7. Calendrier de consolidation

1. INDUS02.1 : commande générique Visual/Item et validation des huit armes **validées localement**.
2. INDUS02.2 : création idempotente d'entrées dans la palette et quatre tests génériques, **validation locale en attente**. Aucun placement automatique dans un niveau.
3. INDUS03 : enrichissement narratif du classeur (schéma compatible et versionné).
4. INDUS04 : infobulle d'inventaire fondée sur le ItemLevel réel.
5. INDUS05 : archiver les scripts/commandlets spécifiques après preuves
   d'équivalence ; ne pas supprimer l'historique ni les outils historiques
   avant d'avoir couvert les exceptions.

Ce guide remplace les recettes ad hoc pour une nouvelle arme de types supportés.
Les documents ARMEMENT01 anciens restent des archives techniques.
