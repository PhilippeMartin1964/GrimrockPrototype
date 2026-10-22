# ARMEMENT01.9.6.1 — Nettoyage documentaire et consolidation des 18 kits

**Date : 22 octobre 2026 — STATUT : consolidation documentaire.**

> **Mise à jour ARMEMENT01.9.7 (22.10.2026)** : cette consolidation précède l'approbation des 18 recettes pour la production des assets. Le statut `DesignProposal` cité ci-dessous était exact **au moment du jalon 01.9.6.1** ; il est remplacé par `ApprovedDesign` dans le manifeste courant. Voir [ARMEMENT01.9.7 — décisions finales et ambiguïtés](ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md). Les 12 kits historiques demeurent actifs en runtime et la migration reste différée.
**Périmètre : aucune modification C++, tests, .uasset, CSV de données, GamePlay, SaveGame ou UMG.**

## Décision de référence pour le futur

**Une distinction de version est obligatoire :**

- **Production actuelle = 12 kits historiques** (deux par classe), distribués par
  `FRPGStartingEquipmentPresetCatalog` et présentés par le wizard. Leurs
  `StartingEquipmentKitId` restent enregistrés et exploitables dans les
  sauvegardes. Ce fonctionnement ne change pas dans ce ticket.
- **Design futur = 18 kits proposés** (trois par classe, un par branche réelle),
  décrits dans
  [ARMEMENT01.9.6 — 18 kits](ARMEMENT01_9_6_EIGHTEEN_STARTING_KITS.md)
  et dans `ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`.
  Tests locaux validés : filtre `Grimrock.RPG.ARMEMENT01.9.KitDesign`,
  **3 réussis, 0 avertissement, 0 échec, exit code 0**,
  `TD04-20261022-122937`. Cette validation atteste **la conformité des
  propositions aux données de référence**, pas l'existence de 18 kits
  jouables ; toutes les recettes restent au statut `DesignProposal`.

Le joueur choisira un **kit de départ**, pas une spécialisation acquise.
La sélection ne confère aucun talent, n'en bloque aucun et ne limite pas
les armes autorisées. Premier choix de talent : **niveau 2**.

## Hiérarchie des autorités à respecter

| Question | Référence | Statut |
|---|---|---|
| Préférences armes/classes, branches typiques, qualificatifs | Classeur `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`, onglets `Classes` et `Catalogue` | **Autorité éditoriale** |
| Valeurs exactes d'armes N1–N20 | Le même XLSX, onglet `Niveaux 1-20` | **Autorité statistique** |
| Lecture C++ des 82 affinités et six préférences | `DA_WeaponCatalogue` (projection ARMEMENT01.9.5.2) | **Projection du XLSX**, pas saisie concurrente |
| Choix des 18 futures compositions N1 | `ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` | **Propositions de design, pas données runtime** |
| Talents disponibles et progression | Six `DA_Class_*` et mécaniques RPG | **Autorité gameplay** |
| Affinité des 25 compétences par classe | `DA_RPGSkillPresentation` | Axe distinct de l'affinité d'arme |
| Effet réel, slots, poids, actions, sauvegardes | `UGridItemDefinitionAsset`, `FRPGStartingEquipmentService`, runtime d'inventaire | **Autorité d'exécution** |
| Libellés visibles par le joueur | `Localization/Source/UI.csv` → `ST_UI` / `FUILocalizationText` | Localisation unique ; pas de chaînes françaises en dur |

Le **CSV ARMEMENT01.9.5** (12 décisions `Retain/Revise`), les anciennes
notes de priorité et les 12 recettes codées en C++ ne deviennent **pas**
une deuxième source pour les futurs kits. Ils sont préservés pour
la traçabilité et la compatibilité.

## État consolidé des jalons

| Jalon | Ce qui reste vrai | Ce qui n'est plus une décision active |
|---|---|---|
| `ARMEMENT01.9.2` | **12 kits runtime** encore distribués, IDs historiques et sauvegardes | Leur équipement provisoire ne doit pas dicter les 18 futurs kits |
| `ARMEMENT01.9.3` | Sélection du kit dans le wizard et sauvegarde fonctionnelles | L'hypothèse « seulement 2 options par classe » n'est pas une cible finale |
| `ARMEMENT01.9.4` | Audit **historique** réussi 3/3 des 12 kits, résultats exploitables pour la régression | Ses rapprochements de talents, antérieurs aux affinités du XLSX, ne sont pas prescriptifs |
| `ARMEMENT01.9.5` | Registre documentant pourquoi les 12 kits étaient imparfaits ; tests historiques de conception | Les décisions `Retain`, `Revise`, etc. **sont supplantées** pour le futur |
| `ARMEMENT01.9.5.1` | Décision confirmant l'autorité du classeur Excel | Les premières suggestions de substitution faites sans les affinités sont à écarter |
| `ARMEMENT01.9.5.2` | Import des 82 affinités et six préférences validé ; 1 640 profils conservés, deuxième import `Changed=false` ; `DA_WeaponCatalogue.uasset` poussé d'après confirmation utilisateur | Les consignes de matérialisation initiales décrivent une procédure reproductible, pas une tâche encore bloquée |
| `ARMEMENT01.9.6` | **18 recettes N1 conçues, 3/3 tests UE validés**, 22 archétypes distincts, 5 compléments non armement encore à créer | Aucun nouveau kit n'a été activé ; les 18 recettes sont encore `DesignProposal` |
| `ARMEMENT01.9.6.1` | Balisage des documents et plan de transition **sans suppression** | Aucun comportement de jeu modifié |

Ne pas renommer/réécrire les anciennes données comme si elles avaient
toujours contenu 18 kits : elles documentent l'historique réel.

## Inventaire précis des dépendances à conserver puis faire évoluer

| Emplacement actuel | Hypothèse legacy / rôle | Action future, **pas dans 01.9.6.1** |
|---|---|---|
| `Source/GrimrockPrototype/Private/RPG/RPGStartingEquipmentPresetCatalog.cpp` | `FKitSpec` contient `First` et `Second`, et `TryPopulateTransientClass` exige `Candidate.Num() == 2` | Générateur prenant en charge trois kits par classe et un nombre **variable** d'objets, sans perdre les anciens ID |
| `FRPGStartingEquipmentKit::Items` (`RPGClassAsset.h`) | Modèle déjà basé sur un tableau ; ne fixe pas deux objets | **Conserver** ce modèle ; ne pas inventer un deuxième format de kit |
| `GridARMEMENT0192SixClassPresetsTests.cpp` | Deux kits et deux objets dans chaque kit, défauts historiques | Garder les assertions legacy comme non-régression ; créer des attentes séparées pour les futurs 18 kits |
| `GridARMEMENT0193StartingKitWizardTests.cpp` | Certaines vérifications de liste à **2 options** | Adapter pour **3 options** à l'activation ; maintenir les tests des anciennes requêtes/sélections |
| `GridStartingKitCoherenceAudit.cpp` + `GridEditorARMEMENT0194CoherenceAuditTests.cpp` | Constantes `ExpectedKits=12`, **2 kits** par classe, 12 propositions | Réserver ce rapport au périmètre historique ou créer un audit N1 à 18 recettes alimenté par l'Excel ; **ne pas simplement remplacer 12 par 18** |
| `GridEditorARMEMENT0195KitDecisionTests.cpp` + `ARMEMENT01_9_5_KIT_DECISIONS.csv` | Décisions historiques de **12 kits**, `PendingApproval` | Conserver ensemble tant que les tests historiques sont utiles ; ne pas utiliser pour produire les nouveaux assets |
| `GridEditorARMEMENT0196StarterKitRecipeTests.cpp` | Exige **18 recettes proposées** et **aucun nouvel ID injecté** dans les classes | À scinder/faire évoluer quand les kits deviennent runtime ; la dernière assertion cessera alors légitimement d'être vraie |
| `Localization/Source/UI.csv` et `Content/Localization/StringTables/ST_UI.uasset` | 12 noms legacy et les formats de texte du wizard | **Ajouter** les noms/conseils nécessaires aux 18 nouveaux kits, sans effacer les clés encore référencées ; régénérer ST_UI localement |
| `RPGCharacterCreationWizardWidget.cpp` | Liste les kits d'une classe ; sélection et affichage basés sur leurs noms localisés | Vérifier trois options, unicité des libellés et correspondance stable `KitId`; conserver présentation dans les cinq étapes |
| `RPGCharacterTypes.h`, `GridInventoryTypes.h`, `GridPartyInventoryComponent.cpp` | `StartingEquipmentKitId` demandé et persisté ; restauration d'items par leur ID réel | Prévoir **lecture des anciennes sauvegardes**, refus explicite des IDs inconnus et absence de substitution silencieuse |
| `GridARMEMENT0192SaveRehydrationTests.cpp` | Vérifie le chargement des items historiques et le rejet des IDs non définis | **Conserver et compléter**, notamment sauvegardes avec ancien `StartingEquipmentKitId` |

**Attention :** `Grimrock.RPG.ARMEMENT01.9.KitDesign.NoLegacyKitOrSaveMutation`
montre aujourd'hui que les **nouveaux IDs** ne sont pas activés ;
ce n'est **pas à lui seul** une preuve exhaustive de compatibilité
d'anciennes sauvegardes après migration.

## Risque précis : les affinités conditionnelles du XLSX

Dans l'onglet `Catalogue`, certaines colonnes `Classes secondaires`
contiennent des **qualificatifs** tels que `Prêtre (marteaux)`.
Le pipeline ARMEMENT01.9.5.2 :
1. conserve le texte original dans `SecondarySourceLabel` ;
2. convertit le nom de classe en `Priest` dans `SecondaryClassIds` ;
3. expose `GetClassAffinity(..., Priest) == Secondary`
   **sans interpréter la condition entre parenthèses**.

Cette perte de nuance est tolérable tant que le résultat reste
**un avis consultatif** ; elle ne l'est **pas** pour un générateur
automatique de recommandations ou de kits N1. De plus,
des lignes de grandes épées/haches portent le qualificatif
`Prêtre (marteaux)` : ne pas en déduire que toutes ces armes
sont recommandées au Prêtre. Il faut vérifier le détail de chaque
ligne du XLSX et faire valider la sémantique des exceptions.

**Décision provisoire :** si un rapprochement utilise un libellé
qualifié, classer le choix `NeedsDesignReview` ; ne pas autoriser
un simple `Secondary` à valider automatiquement ce cas, et
surtout ne jamais modifier `CanEquip` en fonction de ces données.
La correction de l'interprétation ou de la source est un **ticket
technique distinct**, testable sur plusieurs exemples qualifiés,
sans nouveau tableau parallèle.

## Ordre recommandé pour reprendre la production

1. **Ratifier les 18 recettes** (`DesignProposal` → approuvées
   dans une future revue), clarifier les qualificatifs Excel et
   les cas d'objets passifs/armes de jet. Aucune assignation de
   talents au niveau 1.
2. **ARMEMENT01.10** : matérialiser les 22 archétypes prioritaires N1
   (dont trois pilotes existants) en lots atomiques et vérifier
   le `WeaponArchetypeId`, le vrai `ItemLevel=1`, l'emplacement,
   les actions, les meshes et les icônes.
3. Résoudre les **cinq compléments fonctionnels non armement**
   signalés dans la spécification 01.9.6 ; tester les munitions,
   consommables, jet/récupération et focaliseurs.
4. **Activer 18 kits** seulement une fois leurs dépendances disponibles :
   nouveaux IDs stables, compatibilité des anciens IDs, nouvelle
   localisation `ST_UI`, attribution transactionnelle et tests
   PIE/SaveGame ; mise à jour ciblée des tests historiques.

## Règles du présent nettoyage

- **Ne rien supprimer :** ni `FRPGStartingEquipmentPresetCatalog`,
  ni les tests 12-kits, ni les 12 anciennes clés de localisation,
  ni les CSV d'historique, ni le classeur Excel.
- **Ne rien migrer aujourd'hui :** aucune modification
  `StartingEquipmentKitId`, `UGridItemDefinitionAsset`,
  `DA_WeaponCatalogue`, `ST_UI.uasset`, widget UMG ou sauvegarde.
- **Ne jamais déclarer les 18 kits « jouables »** sur la seule
  base de l'audit de recettes 3/3.
- Le présent commit contient **uniquement des fichiers Markdown**.
  Une compilation UE n'est pas nécessaire pour ce nettoyage ;
  les validations citées proviennent des journaux utilisateur déjà reçus.

**Critère de clôture ARMEMENT01.9.6.1 :** historique balisé,
référence de conception unifiée, dépendances de migration explicites,
qualificatifs d'affinité signalés ; aucune action irréversible.
