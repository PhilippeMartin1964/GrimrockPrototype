# Consolidation — architecture, nomenclature et état de référence

**Référence :** `master`, `a8de4f58b6c7ecc42e62aef0dbbe8fa9b7ede272`, relevé du 22 novembre 2026. Lot documentaire limité à [DataModel.svg](../Architecture/DataModel.svg), [DataModel.md](../Architecture/DataModel.md) et ce fichier. Aucun code, asset, script, configuration ou SaveGame modifié ; aucun renommage ou retrait engagé. Cette référence et ses constats datés sont conservés ci-dessous ; l’état courant et la clôture ARM-CLEAN06 figurent dans la mise à jour suivante.

## CLEANUP04 — simplification C++

### CLEANUP04-A — résidus natifs inutilisés

Référence : `36205b2d02e511c61119e5e3acf7ca2f85e49332`. Retrait limité aux résidus D01–D06 ; D08–D13 et CLEANUP04-B ne sont pas engagés. Les constats/quantités CLEANUP03 ci-dessous restent ceux du SHA audité.

| Candidat | Décision et preuve |
| --- | --- |
| D01 · GridNonArmamentReadPolicies.h | Supprimé : helper non réfléchi, aucun include, appel ou chargement indirect identifié |
| D02 · GridNonArmamentThrowReadModel.h | Supprimé : même absence de consommateurs ; lecteurs actuels inchangés |
| D03 · FormatFloatWithBonus | Supprimé de GridInventoryWidget.cpp : fonction anonyme native, une seule occurrence, aucun appel |
| D04 · GridAssassinDaggerN1ItemAuthoringSpecs.h | Supprimé : table/namespace d’authoring sans consommateur, aucun mécanisme de génération trouvé ; les identités et assets AssassinDagger restent intacts |
| D05 · FGridCharacterSpellbookState::Sanitize | Supprimé : méthode inline sans appel, non UFUNCTION, absente des bindings UHT et des hooks de struct ; la validation active des sorts est conservée |
| D06 · FGridCharacterEquipmentState::ClearSlot | Supprimé : mêmes garanties natives ; slots, propriétés, règles de transfert et sérialisation inchangés |
| D07 · FRPGCharacterPortraitOption | Conservé : USTRUCT(BlueprintType) publique toujours enregistrée, avec quatre UPROPERTY ; zéro usage C++/nom sérialisé repéré ne démontre pas l’extinction du contrat Blueprint/public. Retrait non justifié dans ce lot sans évolution réfléchie ni preuve complète des consommateurs |

Bilan : **trois headers supprimés, trois fichiers C++ modifiés, 191 lignes C++ retirées, aucune ligne C++ ajoutée** ; ce document est le seul autre fichier modifié (sept chemins au total). Recherche dans Source, plugins/configurations/scripts versionnés et packages uasset/umap ; aucun test supprimé, wrapper ajouté, type réfléchi, champ, identité persistante ou redirect modifié. Les six gen.cpp des trois headers concernés restent **identiques octet par octet** en Editor et Shipping ; les 1 439 autres fichiers Source et D07 sont inchangés.

Validations locales : Development Editor et Shipping réussis ; **61 tests Magic et huit UI Inventory**, sans avertissement/échec ; **2 044 tests Grimrock réussis (1 997 sans avertissement, 47 avec avertissements identiques à la baseline locale), zéro échec et zéro non-exécuté**. **Build/Cook/Stage/Package/Archive Shipping réussis sur le contenu du lot ; listing natif et INI de l’archive contrôlés : 53 assets courants, deux Readables, 53 redirects d’objet et 50 alias Primary IDs présents**. Références/includes et diff --check vérifiés. Empreintes des sept sauvegardes personnelles et des deux Readables conservées. Aucune nouvelle conversion ni nouvelle lecture de sauvegarde personnelle ; schéma v27 et réflexion inchangés, round-trips couverts par la suite existante.

Localisation : chaîne `UI.csv → ST_UI → WBP → Game/fr/en` inchangée ; aucun texte joueur, binding ou clé modifié par le retrait du formatter inutilisé. Aucun authoring UMG ni régénération de localisation requis.

Publication : un commit atomique ; clôture conditionnée à **SilentTask et LocalAuto SUCCESS sur le SHA publié**, résultats dans le compte rendu de clôture. Preuves privées sous `D:/GrimrockNativeSnapshots/CLEANUP04A_20261122-223557/`. Aucune régression détectée dans les contrôles exécutés ; D07 reste la réserve explicite. Arrêt après CLEANUP04-A pour validation utilisateur.

### CLEANUP04-B — invariants Combat, RPG et Save

Référence : `260ba8863364015eef135f34c1d15391402cfa21`. U04–U08 uniquement ; C03-02 caractérisé, sans correction comportementale. CLEANUP04-C et D08–D13 ne sont pas engagés.

| Famille | Consolidation et contrat conservé |
| --- | --- |
| U04 | `GridCombatTurnPlan::AppendTurnsToward` dans un header privé ; trois variantes remplacées. Source None → North, destination None refusée, priorité gauche puis droite, demi-tour gauche/gauche, ordre et nombre des actions inchangés. Les factories restent locales : Keeper conserve `bIsRepositioningAction`, les autres planificateurs leurs politiques |
| U05 | Réutilisation de `FGridCombatResolver::GetResistancePercent` par `UGridMonsterCombatComponent` ; suppression du switch privé concurrent. Huit correspondances et défaut zéro identiques ; lecture brute, clamp −100/+100, arrondis, effets et partage armure/PV restent à leurs emplacements antérieurs |
| U06 | `RPGResourceRules::PreserveHealthDeficit` / `PreserveManaDeficit` partagées par AttributePoint et LevelUp. PV morts à zéro, déficit absolu, bornes respectives 1/0, maxima croissants/décroissants conservés. Les deux règles restent distinctes ; un ancien maximum de mana nul conserve exactement le comportement existant |
| U07 | `GridCombatOwnerRequirements::Collect` / `AreSatisfied` remplacent les deux paires identiques. Classe absente tolérée seulement sans choix, choix vides/dupliqués/inconnus refusés ; propriétaires, statut source, diagnostics et projections effets/réactions restent locaux |
| U08 | `RPGSelectionRules::BuildSelectionSet` partagé par Save/ProgressionTransaction : sortie remise à zéro, refus de None/doublon sans résultat partiel. `GridPartySaveProjections::CountCharactersWithKnownSpells` partagé par PawnSave/SaveGame : actifs puis pool, nombre de personnages et non de sorts. Sélection, ordre des portraits et validation avant mutation inchangés |

**Métriques :** 17 implémentations initiales → huit primitives, neuf répétitions retirées ; cinq petits headers privés ajoutés, aucun gestionnaire ni cache nouveau. Vingt fichiers Source concernés (douze existants, huit nouveaux dont trois d’infrastructure/tests) : **+649/−354 lignes**, dont production **+198/−354** et tests **+451** ; ce document est le seul autre fichier modifié. Les autres projections/comptages et résolutions de définitions restent locaux : contrats distincts ou résolution non pure, sans abstraction générique supplémentaire.

**Caractérisation :** sept tests exécutés avant mutualisation, dix après sous `Grimrock.Invariants`. Matrice des quatre orientations et source None, quarts/demi-tours, destination absente/inconnue ; huit résistances distinctes et limites int32 ; tables de ressources saines/blessées/mortes, mana consommé, clamp et variation des maxima ; prérequis invalides, absence de classe, sélection/doublons et absence de mutation partielle. Les tests historiques AttributePoint, LevelUp, combat et sauvegarde restent présents. Seize comparaisons des corps d’origine établissent l’équivalence hors noms/commentaires/espaces.

**C03-02 — chronologie et limites démontrées :**

| Parcours | Mutations et callbacks synchrones conservés |
| --- | --- |
| Inventaire/équipement → réceptacle | Snapshot + prévalidation → retrait source → `NotifyPartyInventoryChanged` (sanitisation portraits puis Broadcast) → insertion → en échec, contrôle de disponibilité, restauration + seconde notification ; sinon `RollbackFailed` |
| Inventaire → serrure | Même retrait/notification avant déverrouillage ; restauration inconditionnelle du snapshot + notification en échec : risque d’écraser une mutation imbriquée, constat statique à tester avant correction |
| Personnage → personnage | Source déjà débitée avant `AddItemToCharacterInventory` ; destination validée et écrite, notification destination, puis notification source. AddItem ne relit pas CharacterState après son Broadcast ; ses appelants peuvent conserver des références |
| Réceptacle → inventaire | Retrait du contenant puis AddItem ; restauration du contenant si insertion refusée, liens de retrait après succès. La destruction d’un acteur contenu peut également déclencher des callbacks : conservation des snapshots/contrôles existants indispensable |
| Choix de classe | Toutes les validations/candidat avant mutation → sélection durable + projection runtime + résultat → notification inventaire → événement progression → log utilisant encore `Character*` |

Tests sûrs sur objets transitoires : changement de sélection toléré, transfert imbriqué sur un autre slot réussi, refus d’insertion avec rollback normal, slot réoccupé retournant `RollbackFailed` sans écrasement. **Défaut reproduit** : une permutation du roster dans le premier callback, suivie d’un refus d’insertion, restaure le snapshot dans l’autre personnage tout en gardant l’ancien OwnerGuid ; le personnage source perd son objet. Une permutation pendant le commit de progression rend l’index de l’événement suivant incohérent avec le personnage ayant reçu le choix. Enfin, un callback peut réallouer ActiveCharacters ; l’invalidation d’adresse est constatée sur une notification isolée, sans déréférencer ensuite le pointeur libéré. Les références CharacterState, SourceItem/équipement et Character* conservées au-delà des callbacks restent donc dangereuses selon le parcours. Ces défauts conditionnels préexistent au lot ; aucune reproduction sur sauvegarde personnelle ni affirmation de déclenchement par l’UI actuelle.

**Correction proposée, non exécutée :** décider séparément du contrat de mutation du roster et de notification pendant une transaction, puis préparer/commettre localement par identité avec gestion explicite des conflits de rollback. Réacquérir une référence ne règle pas à lui seul disparition, remplacement ou réoccupation. Tester retrait/réallocation du roster, changement de sélection, notifications imbriquées, destination modifiée, identité/ownership et tous les retours d’échec ; aucun verrou global, report de notification ni modification de nombre/ordre intégré ici. Les tests de caractérisation du défaut devront évoluer lors de sa correction approuvée.

**Validation :** Development Editor et Shipping réussis ; dix tests ciblés réussis ; suite complète **2 054 réussites (2 006 sans avertissement, 48 avec), zéro échec/non-exécuté**. Les 2 044 scénarios historiques et leurs nombres d’avertissements correspondent exactement au reçu local A ; un nouveau scénario de refus contrôlés produit trois avertissements attendus dans un seul test. La différence locale/LocalAuto liée aux avertissements de présentation optionnelle reste distinguée dans le compte rendu publié. Build/Cook/Stage/Package/Archive Shipping réussis sur le contenu final ; pas de nouvelle recette graphique, aucune présentation modifiée. Tous les littéraux TEXT des douze fichiers de production sont identiques à la référence ; les 395 gen.cpp comparés avant/après sont inchangés.

**SaveGame :** sept copies hors dépôt, audit natif `ReadOnly`, zéro conversion/écriture des entrées. Avant/après : 7/7 v27 chargées, instances reconstituées, projections persistantes et identités égales, round-trips mémoire/propriétés égaux, monde valide. Empreintes des originaux et copies inchangées ; format, Core Redirects, alias Primary IDs, assets et deux Readables inchangés. `git diff --check` contrôlé. Preuves privées : `D:/GrimrockNativeSnapshots/CLEANUP04B_20261122-231335/`. Un commit atomique ; clôture conditionnée aux deux statuts SilentTask/LocalAuto SUCCESS sur son SHA exact, résultats dans le compte rendu final.

### CLEANUP04-B.1 — transactions réentrantes sécurisées

Référence : `02e2248583b49c0b6a2147d30d1e81836ab19044`. Correction de C03-02 sur les transferts et commits de classe ; CLEANUP04-C non engagé. Les chronologies et défauts décrits dans le reçu B précédent restent les constats de son SHA.

**Cause reproduite avant correction :** des références à ActiveCharacters/ActiveEquipment et des indices traversaient les callbacks synchrones ; une permutation redirigeait le rollback et l’événement de progression, une réallocation invalidait le pointeur utilisé ensuite par le log. Les deux anciens tests de caractérisation ont été exécutés sur la référence avant modification.

| Contrat | Correction retenue |
| --- | --- |
| Réentrance | `FGridPartyTransactionScope`, RAII privé : réservation temporaire des CharacterId, RuntimeObjectId et du contenant concerné. Refus avant mutation par `TransactionConflict` si recouvrement ; opérations sur personnages et contenants disjoints autorisées, y compris dans le même inventaire. Réservation du contenant portée par son acteur, donc respectée entre inventaires différents. Aucun verrou global, manager ni cache persistant |
| Mutateurs | Ajout/retrait, déplacement, curseur, équipement, extraction monde et consommation personnelle respectent les réservations ; un GUID en transit ne peut être recopié au curseur ou dans un autre personnage. Reset/Restore/création et recrutements remplaçant le roster sont refusés pendant la transaction. Une insertion interne privée permet au transfert de terminer sans transmettre une autorisation ambiante à ses callbacks. Lecture, sélection et présentation restent disponibles |
| Identité et durée de vie | Résolution unique du CharacterId après chaque frontière de callback ; aucune référence de conteneur n’est réutilisée après notification. Snapshots indépendants, slot physique d’équipement mémorisé, objets nécessaires retenus temporairement par références fortes ; restauration d’un contenant réacquise par RuntimeObjectId après ses callbacks d’acteur |
| Rollback | Comparaison du slot courant avec l’état attendu après débit avant restauration ; propriétaire stable, quantité et GUID du snapshot conservés. OwnerCharacterIndex, dérivé du roster, est normalisé pour tolérer une permutation cohérente, y compris sur une pile partiellement débitée. Source absente/modifiée : `RollbackFailed` explicite sans écrasement de l’état concurrent |
| Progression | Identité, niveau et résultat du commit copiés avant notification ; index de l’événement résolu juste avant émission. Aucun pointeur Character utilisé après callback. Le résultat et ses choix restent stables même si un callback réutilise le même OutResult pour un commit imbriqué refusé |
| Persistance en cours de transaction | Save/Continue/New Game refusés avant lecture, capture, écriture, suppression du slot ou réinitialisation du monde. Un callback ne peut donc pas sauvegarder un objet en transit ni poursuivre un New Game après un reset refusé |
| Notifications | Points d’émission conservés : retrait source avant insertion/déverrouillage, seconde notification seulement après restauration ; personnage→personnage : destination puis source ; progression : inventaire puis événement métier. Les liens de donjon restent dans les méthodes existantes des réceptacles/serrures. Seuls les indices devenus obsolètes sont réacquis |

Les API publiques protègent les mutations physiques réservées. Du code natif qui écrit directement dans un slot réservé, supprime son propriétaire ou duplique son CharacterId contourne ce contrat : une telle corruption n’est pas rendue récupérable artificiellement ; elle est signalée sans restaurer sur un autre personnage (`RollbackFailed` ou erreur d’identité). Les permutations cohérentes et réallocations sont explicitement éprouvées ; aucune politique de secours monde/curseur ni stockage d’objets orphelins n’est ajouté.

**Tests :** huit tests dédiés réussis, sans avertissement (deux adaptés, six nouveaux), couvrant 22 scénarios paramétrés : cinq routes physiques, pile entière/séparée, sélection, permutation et réallocation des personnages/équipements, destruction du visuel source, refus d’insertion, capacités modifiées après prévalidation, rollback normal/partiel, mutations imbriquées refusées ou indépendantes, alias OutResult RPG, refus Save/Continue/New Game et conservation exacte d’un slot synthétique créé puis supprimé uniquement sous le Saved isolé du harness. Assertions sur états finaux, quantités, unicité des GUID, propriétaires, libération des réservations et nombre/ordre/identité des événements. Les refus attendus sont déclarés dans les tests, sans masquer d’erreur inattendue.

**Validations :** Development Editor et Shipping réussis ; suite Grimrock complète : **2 060 réussites (2 013 sans avertissement, 47 avec), zéro échec et zéro non-exécuté**. Les 2 054 scénarios du reçu B restent présents ; seuls les deux tests de défaut caractérisé changent de contrat attendu. Six tests ajoutés ; l’avertissement du scénario historique de refus est maintenant explicitement attendu. Build/Cook/Stage/Package/Archive Shipping réussis sur le contenu final ; contrôle natif de l’archive : mêmes 2 112 entrées, 53 assets courants, deux Readables, 53 redirections d’objets et 50 alias Primary IDs. Pas de recette graphique supplémentaire : aucun WBP, texte joueur nouveau ni comportement visuel modifié ; `UI.csv → ST_UI → WBP → Game/fr/en` inchangé, message de refus générique existant réutilisé.

**Persistance/intégrité :** audit natif ReadOnly avant/après sur sept copies hors dépôt : 7/7 v27 chargées, instances reconstituées, projections durables/identités et round-trips mémoire/propriétés égaux ; originaux et copies inchangés par SHA-256. Deux Readables protégés et fichiers étrangers au lot inchangés. Aucun champ sérialisé, format, ItemId/ActionId/GUID, asset, configuration ou Core Redirect modifié ; les nouveaux codes de refus sont ajoutés en fin d’enum sans déplacer les anciennes valeurs.

**Périmètre :** dix-sept fichiers Source (quinze modifiés, deux ajoutés) et ce seul document ; Source **+944/−66 lignes**, dont production **+378/−40**, tests **+566/−26**. Preuves privées sous `D:/GrimrockNativeSnapshots/CLEANUP04B1_20261123-064251/`. `git diff --check` vérifié ; un commit atomique, clôture après SilentTask et LocalAuto SUCCESS sur son SHA exact, statuts consignés dans le compte rendu final.

### CLEANUP04-C — outillage Editor mutualisé

Référence : `f02bcb6f16671d5e504666c92d045e0fd21e9954`. U01, U11 et U12 uniquement ; aucune modification Runtime, GrimrockLua, Blueprint, DataAsset, configuration, script ou sauvegarde. CLEANUP04-D non engagé.

| Famille | Mutualisation et différences conservées |
| --- | --- |
| U01 | `GridEditorAssetSave`, privé au module : huit implémentations de sauvegarde remplacées par un corps commun et deux entrées `SaveAuthoredAsset` / `SaveClassAsset`. Diagnostics propres à chaque entrée conservés, ainsi que IsValid/package, MarkPackageDirty, création du répertoire, flags RF_Public/Standalone et SAVE_NoError, veto natif et résultat SavePackage. Le succès ne réinitialise pas OutError. Validation métier, Modify, transactions et autorisation d'appliquer restent chez les appelants ; aucun authoring exécuté avec Apply. SaveSkillAsset reste local : contrat par référence non nullable et diagnostics français propres |
| U11 | `GridLuaPersistentCompatibility` : quatre définitions remplacées par deux prédicats purs, `AreDefinitionsCompatible` / `MatchesLevelVariable`. Identité, type et valeur active Bool/Int32 identiques ; champs inactifs ignorés comme auparavant. Validation VM, ordre de collecte, refus ou accumulation des diagnostics E001/E103 et mutations restent dans leurs services. Le comparateur Runtime de GrimrockLua reste local : frontière de module différente |
| U12 | `GridEditorActorLookup` : cinq recherches identiques, dont la préparation PIE, utilisent la même primitive. Recherche dans le contexte Editor, premier acteur de l'itérateur natif, aucun repli PIE/autre monde ; sélection et niveau courant ne deviennent pas prioritaires. Les méthodes des clients restent des adaptateurs pour leurs appels/delegates. UndoBridge conserve son parcours de tous les acteurs et ses filtres propres. Aucun cache, singleton ni nouvelle politique de choix |

**Caractérisation :** quatre tests `Grimrock.Editor.Consolidation.*`, réussis avant raccordement des consommateurs puis après mutualisation. Avant extraction, les huit fonctions originales de sauvegarde sont exercées ; les services Lua publics et la copie inchangée de la primitive d'itération sont également éprouvés. Comparaison des corps : six sauvegardes authored identiques, deux class identiques, quatre prédicats équivalents et cinq recherches identiques hors espaces/noms locaux et égalités symétriques.

**Couverture ajoutée :** sauvegarde réelle dans un montage unique sous le Saved isolé du harness, succès/dirty/transaction/diagnostics, objet nul, veto natif d'écriture et package partiellement chargé refusés sans modification du fichier existant ; retrait des seuls fichiers de fixture et vérification du nettoyage. Aucun asset utilisateur/versionné réécrit par ces tests. Lua : identités/types/defaults, limites Int32, déclarations compatibles ou conflictuelles, doublons et règle Lua existante de dernière valeur, ScriptId dupliqué, types interdits et erreur de compilation ; définitions persistantes inchangées après refus. Acteur : absence, plusieurs acteurs/niveaux, sélection/niveau courant, destruction, mondes Editor/PIE/autre distincts, absence de repli ; contexte et sélection restaurés après test. Le premier acteur suit l'ordre UE existant, sans garantie nouvelle entre sessions.

**Métriques :** vingt fichiers de production Editor concernés (quinze modifiés, cinq ajoutés), **+162/−408 lignes C++**, soit −246 net ; un fichier de tests ajouté, **+364 lignes**. Total Source : **21 chemins, +526/−408** ; ce document est le seul autre fichier modifié. Trois helpers privés, aucune API réfléchie ni dépendance de module ajoutée.

**Validation locale :** Development Editor et Shipping réussis ; quatre tests dédiés sans avertissement ; suite complète **2 064 réussites (2 017 sans avertissement, 47 avec), zéro échec et zéro non-exécuté**. Comparaison individuelle avec le dernier reçu local B.1 : les 2 060 tests précédents conservent leur résultat, nombre et messages d'avertissement ; quatre nouveaux tests sans avertissement. Les huit tests de réentrance B.1 restent réussis sans avertissement. Les tests Editor, RPG, Lua, Dungeon, Save et les refus de sauvegarde sont inclus dans cette suite. La différence historique locale/LocalAuto d'avertissement de présentation optionnelle reste distinguée lors du suivi publié.

**Livraison Shipping :** Build/Cook/Stage/Package/Archive réussis sur le contenu final. Listing natif : mêmes 2 112 chemins distincts (3 444 lignes de conteneur), 53 assets courants et deux Readables ; INI cuisinés vérifiés, 53 redirections d'objet et 50 alias Primary IDs conservés. Aucun changement de présentation nécessitant une nouvelle recette GPU ; séparation Editor/PIE couverte automatiquement.

**Intégrité :** empreintes SHA-256 des sept sauvegardes originales et des deux Readables inchangées ; tous les fichiers étrangers au lot restent identiques. Format v27, chemins de lecture/reconstruction, identités persistantes et protections B.1 inchangés ; tests Save/round-trip existants réussis. Aucun nouvel audit de copies personnelles requis pour ces seules primitives privées Editor. Chaîne `UI.csv → ST_UI → WBP → Game/fr/en` inchangée, aucun texte joueur ni authoring UMG modifié.

**Publication :** références/dépendances, diff et `git diff --check` contrôlés ; un commit atomique. Clôture conditionnée à SilentTask et LocalAuto SUCCESS sur son SHA exact, résultats dans le compte rendu final. Preuves privées : `D:/GrimrockNativeSnapshots/CLEANUP04C_20261123-073609/`. Aucune régression détectée dans les contrôles exécutés ; arrêt après CLEANUP04-C.

## CLEANUP03 — audit C++ et préparation de CLEANUP04

**Référence :** `176bb664116181ba91078f1067feddba94358f60`, `master`, 22 novembre 2026. CLEANUP01/02 et ARM-CLEAN06 sont clos. Audit uniquement ; CLEANUP04 n’est pas engagé. Les inventaires historiques plus bas restent datés et ne remplacent pas les chiffres suivants.

### Périmètre et méthode

Analyse récursive de tous les fichiers C++ des deux modules demandés : définitions, tokens, includes résolus, corps de fonctions, réflexion, chargements et callbacks, tests et branches conditionnelles compris. Types recoupés par reconnaissance syntaxique et lexicale. Les forward declarations et classes générées par macros de tests ne sont pas comptées comme définitions explicites ; quatre structures anonymes le sont.

Le parseur exploratoire ne remplace ni UHT ni le préprocesseur Unreal : ses rapprochements servent au triage, pas à prouver un graphe d’appels complet. Les candidats retenus sont relus. Les lignes ci-dessous désignent le SHA analysé. **Une absence d’appel textuel ne prouve pas qu’un type réfléchi est mort.**

Recoupements indirects : Asset Registry natif en lecture seule, classes d’assets, parents Blueprint, imports cuisinés du tooltip, noms sérialisés, INI, delegates/timers, Automation et v27. Le registre local contient 1 501 packages `/Game` : 1 316 packages versionnés et 185 locaux ; ce n’est pas un inventaire Git. Les trois anciennes classes ont zéro instance dans les deux ensembles.

Données détaillées et outils jetables : `%TEMP%/GrimrockCleanup03/`, rien de permanent ajouté. Les preuves CLEANUP02 au même SHA restent applicables : 2 044 tests, sept copies v27, 70 Blueprints, Cook/Shipping, alias et empreintes. Aucun nouveau build, packaging, conversion ni chargement de sauvegarde personnelle nécessaire ; seule l’archive existante a été inspectée en lecture seule.

### Inventaire exact

| Module | `.h` | `.cpp` | `.inl` | Lignes physiques | Classes | Structures | Enums |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| GrimrockPrototype | 322 | 730 | 0 | 242 488 | 200 | 708 | 157 |
| GrimrockPrototypeEditor | 86 | 270 | 23 | 70 187 | 58 | 150 | 16 |
| **Total** | **408** | **1 000** | **23** | **312 675** | **258** | **858** | **173** |

**1 408 fichiers h/cpp, 1 431 avec les inl** ; lignes incluant commentaires/blancs, pas du code exécuté. Les neuf fichiers C++ de GrimrockLua expliquent l’écart avec les 1 440 de CLEANUP02 ; ce module est une dépendance examinée, pas une extension du corpus. Les deux Build.cs sont lus séparément.

**694 fichiers de tests** (481 Runtime, 213 Editor), 147 737 lignes ; hors Tests : 737 fichiers, 164 938 lignes. Hors Tests ne signifie pas Shipping : des corps Automation y subsistent. Repérage de 2 049 macros SIMPLE et une COMPLEX_AUTOMATION_TEST : ce comptage source n’est pas le nombre de cas exécutés, dépendant du préprocesseur, des flags et cas complexes.

Réflexion déclarée : **165 UCLASS, 278 USTRUCT, 141 UENUM, 4 109 UPROPERTY, 1 190 UFUNCTION**. Types réfléchis Runtime : 143/277/138 ; Editor : 22/1/3. Une déclaration réfléchie signale un usage indirect possible, pas à elle seule un consommateur métier.

Répartition physique exhaustive : `R` = `Source/GrimrockPrototype`, `E` = `Source/GrimrockPrototypeEditor`, Public/Private réunis. Dungeon/inventaire/équipement/interactions sont surtout dans Runtime racine ; Skills/Talents dans RPG et UI. Les tests restent séparés.

| Module / domaine | h / cpp / inl | Lignes | Classes / structures / enums |
| --- | --- | ---: | --- |
| R · racine | 0 / 1 / 0 | 27 | 1 / 0 / 0 |
| R · Core | 29 / 4 / 0 | 5 166 | 6 / 53 / 22 |
| R · Magic | 13 / 11 / 0 | 2 483 | 4 / 26 / 10 |
| R · Quests | 3 / 2 / 0 | 773 | 2 / 4 / 3 |
| R · RPG/Skills/Talents/StatusEffects | 40 / 36 / 0 | 10 231 | 17 / 67 / 18 |
| R · Runtime racine | 86 / 73 / 0 | 43 253 | 38 / 98 / 24 |
| R · Runtime/Combat | 33 / 35 / 0 | 24 291 | 30 / 99 / 40 |
| R · Runtime/Items | 11 / 4 / 0 | 3 193 | 14 / 23 / 11 |
| R · Runtime/Map | 3 / 2 / 0 | 1 252 | 2 / 10 / 3 |
| R · Runtime/Monsters | 25 / 23 / 0 | 9 959 | 28 / 22 / 8 |
| R · Save | 3 / 3 / 0 | 736 | 3 / 0 / 0 |
| R · UI | 69 / 62 / 0 | 31 791 | 55 / 58 / 17 |
| R · Tests | 7 / 474 / 0 | 109 333 | 0 / 248 / 1 |
| E · racine | 0 / 1 / 0 | 269 | 1 / 0 / 0 |
| E · ARMEMENT | 27 / 10 / 0 | 3 940 | 10 / 21 / 2 |
| E · Details | 1 / 2 / 0 | 329 | 3 / 0 / 0 |
| E · EditorTools | 27 / 23 / 23 | 19 539 | 18 / 20 / 11 |
| E · Effects | 1 / 1 / 0 | 170 | 1 / 0 / 0 |
| E · Localization | 3 / 3 / 0 | 677 | 2 / 2 / 0 |
| E · RPG | 17 / 17 / 0 | 5 978 | 9 / 14 / 0 |
| E · Tests | 5 / 208 / 0 | 38 404 | 11 / 90 / 3 |
| E · UI | 5 / 5 / 0 | 881 | 3 / 3 / 0 |

Concentrations : ActionCatalog joueur 3 815 lignes, GridCombatTypes.h 3 237, InventoryWidget 2 721, LevelRuntimeActor 2 339, PartyInventoryComponent 2 165, LevelRuntimeActorSurfaces 2 144, CombatHudWidget 2 062. Le plus gros fichier est un test : GridMonsterMON12CombatHudTests.cpp, 3 880 lignes. **La taille seule n’est pas un défaut.**

Agrégats des cpp de même préfixe : GridTurnManager 10 892 lignes/11 fichiers, GridLevelRuntimeActor 9 840/12, GridPartyInventoryComponent 4 579/8. Le découpage physique existe : analyser responsabilités et états partagés avant un nouveau fractionnement.

### Architecture et autorités

9 398 includes, dont 5 455 résolus dans le corpus ; aucun cycle résolu, include Runtime → Editor ou Editor → header privé Runtime détecté. Cela n’exclut ni les cycles logiques ni les includes transitifs inutiles. Hubs hors Tests : GridItemSubsystem.h (88), GridPartyInventoryComponent.h (83), GridLevelRuntimeActor.h (69), GrimrockPartyPawn.h (67), ancien GridItemDefinitionAsset.h (66), GridInventoryTypes.h (46), GridCombatTypes.h (36). ActionCatalog inclut 51 headers.

Runtime n’a pas de dépendance UBT vers Editor. Editor expose largement AssetTools/UnrealEd/Slate/Json en dépendances publiques : examiner ce qui peut devenir privé selon les signatures. Les deux modules sont volontairement non-unity pour isoler helpers/logs ; ne pas réactiver unity comme optimisation supposée. Les UPROPERTY embarquées peuvent nécessiter des includes complets ; ne pas imposer des forward declarations impossibles pour UHT.

Le [modèle relationnel](../Architecture/DataModel.svg) concorde : une racine/quatre catalogues, 48 définitions/22 ARM ; ItemId statique, GUID/niveau physiques, réserves dépensées et possession dans les instances, présentation transitoire. Aucun second catalogue d’équipements proposé.

- [GridItemSubsystem.cpp:33](../../Source/GrimrockPrototype/Private/Runtime/Items/GridItemSubsystem.cpp#L33),65 : validation initiale, RootRegistry/ItemIndex possédés par UPROPERTY, refus d’IDs/niveaux ; cet index est utile.
- [GridInventoryTypes.h:398](../../Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h#L398),409,424 : ClassDefinition, Level, DerivedStats sont Transient ; [GrimrockPartySaveGame.cpp:293](../../Source/GrimrockPrototype/Private/Save/GrimrockPartySaveGame.cpp#L293),340 les reconstruit. Pas d’autorité persistée concurrente à ClassId/Experience/Attributes.
- [GridItemCatalogues.cpp:87](../../Source/GrimrockPrototype/Private/Runtime/Items/GridItemCatalogues.cpp#L87) contrôle l’unicité entre formes : 19 actions compactes + 14 détaillées ; cinq présentations compactes + une détaillée. Les deux formes sont consommées ; ne pas inventer de doublon inter-formes ni fusionner sans caractériser les payloads.
- Quests valide définitions et ordre des objectifs ; Map/occupation/perception ont des contrats distincts. Deux BFS voisines ne sont pas forcément interchangeables.
- Skills : caches UPROPERTY et invalidation explicite ([GridSkillsWidget.cpp:178](../../Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp#L178), [GridSkillsWidget.h:374](../../Source/GrimrockPrototype/Public/UI/GridSkillsWidget.h#L374)). Tableau ordonné et index ID ont deux usages ; leur fusion n’a pas de gain démontré.
- Ownership examiné : timers arrêtés en EndPlay, lambda VFX weak ([GridLevelRuntimeActorSurfaces.cpp:254](../../Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorSurfaces.cpp#L254)), quatre AddRaw Editor retirés ([GrimrockPrototypeEditor.cpp:75](../../Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp#L75),87). Pas de défaut de durée de vie démontré sur ces sites. Les 24 const_cast ne sont pas 24 bugs : plusieurs servent un simple accès const.

### Candidats au retrait : 13 unités, pas 13 suppressions autorisées

Certitude : **A** absence de consommateur étayée pour un helper non réfléchi ; **B** API sans appel repéré, vérification indirecte finale nécessaire ; **C** historique encore utilisé, retrait conditionné au remplacement de ses garanties. **4 A, 3 B, 6 C**. Aucune classe réfléchie n’est prouvée supprimable sans préparation.

| ID | Unité et preuve au SHA analysé | Certitude / condition |
| --- | --- | --- |
| D01 | [R/Public/Runtime/Items/GridNonArmamentReadPolicies.h:13](../../Source/GrimrockPrototype/Public/Runtime/Items/GridNonArmamentReadPolicies.h#L13) | A ; aucun include/usage du type, trois méthodes sans appel |
| D02 | Même dossier, [GridNonArmamentThrowReadModel.h:8](../../Source/GrimrockPrototype/Public/Runtime/Items/GridNonArmamentThrowReadModel.h#L8) | A ; aucun include/usage, projections remplacées par les lecteurs actuels |
| D03 | [R/Private/UI/GridInventoryWidget.cpp:117](../../Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp#L117), FormatFloatWithBonus | A ; helper anonyme, une occurrence lexicale, aucune inscription réfléchie |
| D04 | [E/Public/ARMEMENT/GridAssassinDaggerN1ItemAuthoringSpecs.h:8](../../Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridAssassinDaggerN1ItemAuthoringSpecs.h#L8) | A ; table/namespace sans include ; ne pas retirer les identités d’assets correspondantes |
| D05 | [R/Public/Magic/GridSpellbookTypes.h:75](../../Source/GrimrockPrototype/Public/Magic/GridSpellbookTypes.h#L75), Sanitize | B ; méthode native inline sans appel ; préserver les validations actives de persistance |
| D06 | [R/Public/Runtime/GridInventoryTypes.h:680](../../Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h#L680), ClearSlot | B ; méthode native inline sans appel ; pas la donnée d’équipement elle-même |
| D07 | [R/Public/RPG/RPGCharacterTypes.h:185](../../Source/GrimrockPrototype/Public/RPG/RPGCharacterTypes.h#L185), FRPGCharacterPortraitOption | B ; USTRUCT sans usage C++ ni nom repéré dans les assets locaux ; contrat réfléchi/public à confirmer |
| D08 | UGridItemDefinitionAsset | C ; 188 fichiers citants, propriétés/fixtures et résidu Blueprint ; voir collisions |
| D09 | UGridWeaponCatalogueAsset | C ; sept fichiers citants ; extraire les règles de croissance toujours actives |
| D10 | UGridWeaponVisualCatalogueAsset | C ; huit fichiers citants ; fixtures et signatures réfléchies |
| D11 | [R/Private/Runtime/Combat/GridWeaponVisualResolver.cpp:3](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridWeaponVisualResolver.cpp#L3) | C ; corps WITH_DEV_AUTOMATION_TESTS, caractérisations à remplacer |
| D12 | Même dossier, [GridWorldItemVisualResolver.cpp:3](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridWorldItemVisualResolver.cpp#L3) | C ; même garde ; conserver la matérialisation actuelle via GridItemMeshResolver |
| D13 | [R/Private/Runtime/GridLegacyItemLookup.h:12](../../Source/GrimrockPrototype/Private/Runtime/GridLegacyItemLookup.h#L12) | C ; Resolve retourne nullptr en Shipping ; corps historique utile aux tests, IsRegistered encore utile |

Paramètres LegacyDefinition, corps historiques et ExpandedProfileCache sont rattachés à D08–D13, pas recomptés en dizaines de candidats. Faux positifs écartés : SetGridPose est une UFUNCTION, ShutdownModule un override Engine ; ResourceCosts/DirectDamageScaling/HealingScaling/TargetedActionResult sont embarqués dans d’autres structures, plusieurs dans sept assets RPG.

Le Main qui retourne 1 dans [GridWeaponCatalogueImportCommandlet.cpp:16](../../Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridWeaponCatalogueImportCommandlet.cpp#L16) est un **refus de recréer une ancienne autorité**, pas un import à réactiver ou du code mort certain. Plans/reçus/commandlets de protection restent consommés par les validations ; examen coordonné des scripts en CLEANUP06, aucun retrait ici.

### Duplications : 12 familles retenues

Criblage hors Tests des corps d’au moins 50 tokens : **37 groupes identiques après retrait des commentaires/espaces**, **93 groupes après normalisation des identifiants**, exacts inclus. Candidats syntaxiques, pas 93 défauts ; lecture et décision retenues pour les **12 familles** suivantes.

| ID | Fonctions / preuves | Proposition et garde |
| --- | --- | --- |
| U01 | E/Private/RPG : SaveAuthoredAsset dans six authorings ([RPGMageAuthoring.cpp:275](../../Source/GrimrockPrototypeEditor/Private/RPG/RPGMageAuthoring.cpp#L275), [RPGAlchemistAuthoring.cpp:218](../../Source/GrimrockPrototypeEditor/Private/RPG/RPGAlchemistAuthoring.cpp#L218)…) ; SaveClassAsset dans deux commandlets ([RPGClassProgressionAuthoringCommandlet.cpp:21](../../Source/GrimrockPrototypeEditor/Private/RPG/RPGClassProgressionAuthoringCommandlet.cpp#L21) / [UIRPGTalentTreeAuthoringCommandlet.cpp:23](../../Source/GrimrockPrototypeEditor/Private/RPG/UIRPGTalentTreeAuthoringCommandlet.cpp#L23)) | Petit helper Editor de sauvegarde/erreur, transactions et autorisations d’écriture inchangées |
| U02 | R/Private/Runtime : GetWorldMonsters et variantes, [GridLevelRuntimeActor.cpp:81](../../Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp#L81), [GridLevelRuntimeActorDiagnostics.cpp:135](../../Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorDiagnostics.cpp#L135), [GridLevelRuntimeActorMonsters.cpp:46](../../Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorMonsters.cpp#L46), [GridLevelRuntimeActorPersistence.cpp:80](../../Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorPersistence.cpp#L80) | Primitive de lecture ; mêmes ordre/filtrage, pas de cache global d’acteurs |
| U03 | [GridItemContextActionLibrary.cpp:33](../../Source/GrimrockPrototype/Private/Runtime/GridItemContextActionLibrary.cpp#L33) / [GridInventoryWidget.cpp:157](../../Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp#L157) | Mapping localisé des libellés d’action partagé ; préserver clés ST_UI et formats |
| U04 | [GridTurnManagerTypes.cpp:54](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerTypes.cpp#L54) / [GridMonsterRangedAttackPlanner.cpp:31](../../Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterRangedAttackPlanner.cpp#L31), AppendTurnsToward ; variante Keeper | Calcul pur commun ; ordre déterministe des rotations, politiques des planificateurs distinctes |
| U05 | [GridCombatResolver.cpp:254](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatResolver.cpp#L254) / [GridMonsterCombatComponent.cpp:585](../../Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterCombatComponent.cpp#L585), GetResistancePercent | Lecture commune ; huit types, bornes et ordre des effets testés |
| U06 | [RPGAttributePointService.cpp:72](../../Source/GrimrockPrototype/Private/RPG/RPGAttributePointService.cpp#L72),89 / [RPGLevelUpService.cpp:13](../../Source/GrimrockPrototype/Private/RPG/RPGLevelUpService.cpp#L13),29 | PreserveHealthDeficit/PreserveManaDeficit ; ne pas ressusciter ni rembourser le mana |
| U07 | [GridCombatModifierResolver.cpp:46](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatModifierResolver.cpp#L46) / [GridCombatReactionResolver.cpp:11](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatReactionResolver.cpp#L11) | Collecte commune des prérequis du propriétaire ; effets/réactions restent distincts |
| U08 | [GrimrockPartyPawnSave.cpp:25](../../Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnSave.cpp#L25) / [GrimrockPartySaveGame.cpp:240](../../Source/GrimrockPrototype/Private/Save/GrimrockPartySaveGame.cpp#L240) ; BuildSelectionSet dans [GrimrockPartySaveGame.cpp:63](../../Source/GrimrockPrototype/Private/Save/GrimrockPartySaveGame.cpp#L63) et [RPGClassProgressionTransactionService.cpp:58](../../Source/GrimrockPrototype/Private/RPG/RPGClassProgressionTransactionService.cpp#L58) | Projections/prédicats purs partagés ; maintenir rejets et validations avant mutation |
| U09 | [GridMonsterActor.cpp:21](../../Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterActor.cpp#L21), [GridMonsterOccupancySubsystem.cpp:12](../../Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterOccupancySubsystem.cpp#L12), [GridMonsterSpawnConfiguration.cpp:10](../../Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterSpawnConfiguration.cpp#L10) | Lookup de RuntimeActor commun ; préserver contexte et durée de vie du monde |
| U10 | GetGridEdgeName et cinq variantes ; GridObjectEventToString dans trois unités Activation | Noms techniques de diagnostic ; ne pas confondre avec les libellés joueur |
| U11 | [E/Private/EditorTools/GridEditorLuaService.cpp:207](../../Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLuaService.cpp#L207) / [GridLuaAuthoringCompiler.cpp:625](../../Source/GrimrockPrototypeEditor/Private/EditorTools/GridLuaAuthoringCompiler.cpp#L625) | Un prédicat de compatibilité des définitions persistantes Lua ; ordre/noms/types inchangés |
| U12 | E/Private/EditorTools : FindEditorActor dans [GridLevelEdMode.cpp:276](../../Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdMode.cpp#L276), [GridLevelEdModeToolkit.cpp:58](../../Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp#L58), [SGridEditorLuaScriptsPanel.cpp:51](../../Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLuaScriptsPanel.cpp#L51), [SGridEditorWorkspaceTab.cpp:72](../../Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp#L72) | Lookup partagé ; pas de singleton de monde ni confusion PIE/EditorWorld |

Ne pas mutualiser les wrappers historiques destinés au retrait avant D08–D13. Constructeurs Button/Lever, Bind/Unbind, accès const/mutable et BFS différentes peuvent exprimer des contrats distincts. Deux validations à des frontières différentes (asset/instance, préflight/commit) ne sont pas automatiquement redondantes.

### Top 10 prioritaire

P0 = critique/intégrité démontrée ; P1 = complexité importante ou risque élevé à caractériser ; P2 = maintenance ; P3 = secondaire. **Aucun P0 démontré.** Difficulté indicative F/M/E = faible/moyenne/élevée, pas un engagement de délai.

| ID / priorité / domaine | Preuve et problème | Impact / correction |
| --- | --- | --- |
| C03-01 · P1 · Items/compatibilité | D08–D10, types historiques encore dans les signatures malgré zéro asset | Collision et dette de fixtures ; libérer les trois noms selon la procédure ci-dessous, sans réinterpréter l’ancien schéma |
| C03-02 · P1 · possession/transactions | [GridItemTransferService.cpp:88](../../Source/GrimrockPrototype/Private/Runtime/GridItemTransferService.cpp#L88)–101,143 : Notify entre mutation et insertion/rollback ; Notify diffuse synchroniquement ([GridPartyInventoryComponent.cpp:321](../../Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp#L321)) | Réentrance plausible, référence CharacterState conservée à travers callback ; caractériser et réacquérir par identité sans changer implicitement les notifications |
| C03-03 · P1 · combat/RPG | U05–U08 : mêmes prédicats métier dans plusieurs calculs | Divergence possible lors d’une correction ; factoriser de petites fonctions pures dans leurs domaines, pas un gestionnaire global |
| C03-04 · P1 · lecteurs/UI | [GridInventoryWidget.cpp:1056](../../Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp#L1056),1064 : type puis tri ; [GridInventoryItemMetadataReadModel.h:34](../../Source/GrimrockPrototype/Private/UI/GridInventoryItemMetadataReadModel.h#L34) revalide la vue résolue | Répétition des résolutions/contrats ; une résolution par opération, mêmes erreurs/niveaux/piles/refus de fallback |
| C03-05 · P1 · tests/migration | 110 fichiers de tests citent l’ancienne définition ; [GridItemCacheParityAudit.cpp:13](../../Source/GrimrockPrototype/Private/Runtime/Items/GridItemCacheParityAudit.cpp#L13) compare des projections d’une même racine | Matrice assertions/invariants/oracles avant condensation ; une façade comparée à la même source n’est pas un oracle indépendant |
| C03-06 · P2 · dépendances | Ancien header inclus 66 fois hors Tests ; [GridItemCatalogues.h:5](../../Source/GrimrockPrototype/Public/Runtime/Items/GridItemCatalogues.h#L5) l’importe pour la croissance | Extraire les types partagés sans changer leur chemin réfléchi ; nettoyer après analyse, pas d’après le seul nombre d’includes |
| C03-07 · P2 · orchestration | ActionCatalog 3 815 lignes/51 includes ; LevelRuntime mêle monde, VFX, état et sauvegarde | Clarifier contrats et déléguer aux politiques existantes ; garder les propriétaires d’état actuels |
| C03-08 · P2 · Editor | U01/U11/U12, sauvegardes et lookups répétés | Mutualiser dans Editor, garder erreurs/transactions ; aucun déplacement de cet outillage vers Runtime |
| C03-09 · P2 · assets/performance | [RPGSkillRequirementProjectionService.cpp:89](../../Source/GrimrockPrototype/Private/RPG/RPGSkillRequirementProjectionService.cpp#L89), [GridStatusEffectPersistence.cpp:348](../../Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPersistence.cpp#L348), [RPGAuthoringIdentityResolver.cpp:32](../../Source/GrimrockPrototype/Private/RPG/RPGAuthoringIdentityResolver.cpp#L32) | Scans/chargements synchrones à froid ; mesurer avant découverte commune/préchargement, préserver l’identité sémantique |
| C03-10 · P2 · résidus | D01–D07, quatre candidats forts non réfléchis | Petit retrait vérifiable d’abord ; séparer les trois candidats publics/réfléchis à confirmer |

Critères propres à chaque constat, en plus de la validation commune des lots :

- **C03-01 : E, risque élevé**, dépend de C03-05/06. Tests UHT/anciens chemins, croissance N1–N20, BP/53 assets, v27 sur copies, empreintes, IDs et Cook/Shipping.
- **C03-02 : M, risque élevé**, indépendant du retrait historique. Callback changeant sélection/roster, réallouant un conteneur ou tentant un transfert imbriqué ; succès, échec et rollback. [RPGClassProgressionTransactionService.cpp:397](../../Source/GrimrockPrototype/Private/RPG/RPGClassProgressionTransactionService.cpp#L397)–402 réutilise aussi Character après Broadcast. Aucune perte/UAF reproduite ; changer chronologie ou nombre de notifications serait une décision comportementale distincte.
- **C03-03 : M, risque moyen**, U04–U08. Résistances, prérequis invalides, doublons de choix, mort, déficits, rotations et Save/load ; préserver refus/diagnostics.
- **C03-04 : M, risque moyen/élevé**, après C03-01 pour retirer les paramètres historiques. Type/tri/icône/tooltip, ARM N0 et non-ARM N1 rejetés, piles/réserves ; ne pas confondre masse d’un pickup et poids total.
- **C03-05 : E, risque élevé**, préalable à C03-01. Carte assertions → invariants → oracles ; garder identités inconnues, GUID unique, deux mains sans duplication, monde/réceptacles/pending, v27 et alias.
- **C03-06 : M, risque moyen**, précède les libérations de noms. UHT, public/private, consommateurs hors des deux modules, builds Editor/Shipping ; préserver les enums de catégorie actifs.
- **C03-07 : E, risque moyen/élevé**, après prédicats/lecteurs. Autorisation/source/coût des actions, déplacements, rebuild/load ; pas de God object ni de changement du ciblage visuel reporté.
- **C03-08 : M, risque moyen**, Editor seulement. Save réussie/refusée sur packages jetables, erreurs/transactions, Lua/PIE ; ne pas tester par réécriture des assets utilisateur.
- **C03-09 : M/E, risque moyen, mesure préalable**, après stabilisation des autorités. Froid/chaud, IDs absents/doublons, Insights/AssetRegistry ; aucun gain démontré, optimisation reportée à CLEANUP05.
- **C03-10 : F pour D01–D04, M pour D05–D07**, indépendant. Absence de consommateurs indirects, builds/tests ciblés ; D07 exige l’examen Blueprint avant retrait.

Priorités/dépendances des familles : U01/U11/U12 → C03-08 ; U03 → C03-04 ; U04–U08 → C03-03. U02/U09 : P2, F/M, contrats monde/occupation/PIE à préserver ; U10 : P3, F, seulement si la maintenance y gagne, tests de diagnostic. D01–D07 → C03-10 ; D08–D13 → C03-01/05. Aucune priorité de retrait ne supprime ses conditions de compatibilité.

### Trois collisions : résolution définitive proposée

Chiffres = fichiers citant le nom hors commentaires/chaînes, **pas appels** ; déclarations, forward declarations et gardes inclus. Ancien → actuel :

| Types | Ancien : Runtime / Editor / Tests | Actuel : Runtime / Editor / Tests | Responsabilité à préserver |
| --- | --- | --- | --- |
| UGridItemDefinitionAsset / UGridCanonicalItemDefinitionAsset | 74 / 4 / 110 | 6 / 1 / 19 | Base actuelle à cinq champs ; EGridItemType et autres types réutilisés ne disparaissent pas avec l’ancienne classe |
| UGridWeaponCatalogueAsset / UGridCanonicalWeaponCatalogueAsset | 5 / 1 / 1 | 2 / 0 / 4 | FGridWeaponIntegerGrowthRule, FGridWeaponGrowthException et Resolve/IsValid actifs ; pas les anciens 1 640 profils/cache sans asset de production |
| UGridWeaponVisualCatalogueAsset / UGridCanonicalWeaponVisualCatalogueAsset | 5 / 1 / 2 | 2 / 0 / 2 | Catalogue actuel ; fixtures de profils/résolveurs historiques à remplacer avant retrait |

Le nom GridItemDefinitionAsset existe encore dans le package Editor WBP_ItemTooltip. Parent natif actuel : GridItemTooltipWidget ; les **58 imports de sa version cuisinée au SHA 176bb664 ne nomment pas l’ancienne classe**. Résidu Editor à qualifier (nom/import/nœud orphelin), pas dépendance Shipping vivante démontrée. L’API Python exposée ne donne pas ces graphes : examiner/recompiler/réenregistrer nativement une copie avant retrait. Aucun nom des deux anciens catalogues repéré dans les uasset locaux ; leurs signatures réfléchies C++ existent néanmoins.

Procédure proposée, **un lot de compatibilité préparé intégralement avant commit**, sans nouveaux noms temporaires :

1. Déplacer les deux types de croissance/méthodes vers un header/source de progression métier, **sans renommer USTRUCT, propriétés ni module**. Pas de Core Redirect pour ce déplacement physique. [GridWeaponCatalogueCompact.cpp:86](../../Source/GrimrockPrototype/Private/Runtime/Combat/GridWeaponCatalogueCompact.cpp#L86),102 reste appelé par le modèle actuel ; ne pas retirer tout le fichier avec l’ancienne classe.
2. Adapter fixtures/consommateurs/signatures et le résidu Blueprint par mécanismes natifs sur copies. Garder des valeurs attendues indépendantes et les refus de fallback Shipping. Ne pas supprimer l’ancien header tant que ses enums de catégorie sont nécessaires.
3. Retirer les trois anciennes UCLASS puis renommer les actuelles vers les trois noms naturels. **Ne pas rediriger aveuglément l’ancien schéma vers le nouveau.** L’ancien chemin de classe sera réoccupé : les vieux packages retirés doivent rester interdits par les gardes, jamais interprétés implicitement comme de nouvelles définitions.
4. Ajouter trois ClassRedirects `/Script/GrimrockPrototype.GridCanonical…` → `Grid…` ; conserver tous les anciens alias directement vers leur cible finale. Aucun ItemId/ActionId/ArchetypeId/GUID changé ; contrôler les références UFUNCTION/UPROPERTY, BP et AssetRegistry.
5. Les deux catalogues changeraient de PrimaryAssetType natif : deux PrimaryAssetTypeRedirects, plus **aplatissement des deux valeurs concernées des 50 PrimaryAssetIdRedirects existants**. AssetManager consulte d’abord l’alias d’ID : ne pas supposer un second redirect de type ensuite. Les classes concrètes des 48 définitions ne changent pas par le seul renommage de leur base abstraite.
6. Vérifier UHT/Editor/Shipping, BP/53 assets, anciens/current Primary IDs, 53 alias Editor et soft paths Shipping, sept copies v27 et hashes originaux, Cook/Package. Aucun retrait de redirecteur ou Readable inclus dans cette opération.

### Les 144 identifiants Canonical restants

Classement exclusif, pas un compte d’occurrences :

| Ensemble | Nombre | Décision |
| --- | ---: | --- |
| Tests/preuves ARMEMENT uniquement, plus le friend de test 4X41 | 82 | Filtres, reçus et traçabilité conservés jusqu’au remplacement des garanties |
| UCLASS en collision | 3 | Procédure ci-dessus ; pas de nouvelle génération de noms temporaires |
| Noms réfléchis à traiter explicitement | 4 | EGridCombatSurfaceInteraction::AnyCanonical, CanonicalBranches, les deux CachedCanonicalSkillDefinitions ; Enum ValueChanges ou PropertyRedirect selon propriétaire |
| Normalisation sans distinction de migration | 2 | MakeCanonical (frontières), CanonicalName (bindings Niagara) : sens fonctionnel à conserver |
| Autres noms de production simplifiables | 53 | Locaux/paramètres/helpers : CanonicalRegistry, TryResolveCanonical, LogGridCanonicalItems, helpers RPG… ; choisir au retrait des anciennes interfaces, jamais changer les identités |

« Production » ne certifie pas Shipping : HasCanonicalProjection reste une preuve Editor. CanonicalBranches est un nom de propriété, AnyCanonical une valeur d’enum réfléchie. Les API RPG de référence et les signatures anciennes ne doivent pas être renommées mécaniquement ; aucune recherche artificielle de zéro occurrence.

### Performances : distinguer les niveaux de preuve

| Niveau | Observation vérifiable | Mesure / décision |
| --- | --- | --- |
| 1 · opération démontrée, pas ralentissement mesuré | [GridInventoryWidget.cpp:1056](../../Source/GrimrockPrototype/Private/UI/GridInventoryWidget.cpp#L1056),1064 résout type puis métadonnées ; [GridItemSubsystem.cpp:94](../../Source/GrimrockPrototype/Private/Runtime/Items/GridItemSubsystem.cpp#L94),135 copie tags/actions/profils et cherche dans les catalogues | Compter résolutions/octets par refresh, p50/p95 ; réutilisation locale possible avant tout cache persistant |
| 1 · opération démontrée | [GridMapWidget.cpp:1563](../../Source/GrimrockPrototype/Private/UI/GridMapWidget.cpp#L1563) reconstruit 12 brushes par Paint ; U02/U09 répètent des parcours TActorIterator | Mesurer Slate/Paint, acteurs/appels ; pas de LoadSynchronous dans ce Paint, pas de gain par cache promis |
| 2 · risque plausible | 75 sites LoadSynchronous dans 32 fichiers hors Tests, incluant Editor et corps historiques ; boot, événements mesh/VFX, refresh d’icônes | Isoler les sites Shipping atteints et chargements froids ; ce ne sont pas 75 appels par frame |
| 2 · risque plausible | Quatre sites ScanPathsForPrimaryAssets, fallback RPG/Skills/StatusEffects ; conversions FString/FName et tableaux reconstruits | Froid/chaud, ID inconnu, taille des catalogues ; observer stalls avant préchargement ou index nouveaux |
| 3 · hypothèse | Fusion de caches Skills, cache global de vues d’objet, autres conteneurs ou centralisation des pathfinders | Aucune mesure favorable ; invalidation, ownership et maintenance peuvent dépasser le gain |

NativeTick du HUD ([GridPersistentHudWidget.cpp:91](../../Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp#L91)) synchronise des layouts avec ses caches, pas toute l’UI. Planificateurs déjà bornés/déterministes ; traces PERF_PROFILE01 disponibles, mais aucun profil actuel chiffré. Aucun coût CPU/GPU/IO ni gain en millisecondes certifié. Mesures et optimisations non prouvées sont réservées à CLEANUP05.

### Tests, compatibilité et lacunes

Baseline publiée : 1 996 succès + 48 succès avec avertissements, zéro échec. Les 694 fichiers ne sont ni tous redondants ni tous indépendants. Les fixtures monde/GI réalistes ([GridARMCLEAN063ATSavedItemRehydrationTests.cpp:25](../../Source/GrimrockPrototype/Private/Tests/GridARMCLEAN063ATSavedItemRehydrationTests.cpp#L25)) et les oracles mémoire v27 ([GridEditorARMCLEAN062ADSaveInstanceParityTests.cpp:20](../../Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN062ADSaveInstanceParityTests.cpp#L20)) couvrent des frontières différentes.

- Conserver : refus d’IDs/niveaux, piles/masse, attaque/source/main, GUID et possession uniques, réserves, monde/réceptacles/pending, Save/Continue v27 exact et deux Readables. La version stricte n’est pas une migration automatique des anciennes sauvegardes.
- Condenser seulement après carte des assertions : helpers historiques, micro-étapes relisant les mêmes données, parités de façades à la même racine. Un test de raccordement reste utile, sans remplacer un oracle indépendant.
- Lacunes prioritaires : réentrance/invalidation des références ; ancien nom de classe réutilisé ; résidu Editor du tooltip ; échec/rollback d’authoring mutualisé ; invalidation après changement d’asset ; cold-load/cooked lookup sans contenu local optionnel.
- Maintenir **34 redirects réfléchis, 53 Core Asset Redirects, 53 redirecteurs natifs, 50 alias Primary IDs** ; tester la cible finale après renommage. Ne pas enlever les gardes parce que les chemins courants sont propres.
- NullRHI ne prouve pas le rendu ; ciblage visuel reporté hors périmètre. Aucun texte joueur, Designer, ST_UI ou localisation modifié ici.

### Plan CLEANUP04 : sept lots cohérents

Les lettres désignent des objectifs, pas des micro-jalons par fichier. Toute dépendance indivisible code/asset doit être préparée ensemble avant publication. Une décision comportementale issue de C03-02 doit être isolée et approuvée avant correction, jamais glissée dans une mutualisation.

| Lot | Objectif / fichiers | Dépendances et validation spécifique |
| --- | --- | --- |
| A · résidus natifs | D01–D04 ; D05–D07 seulement après confirmation | Indépendant ; consommateurs indirects, Editor/Shipping et tests ciblés, aucune protection retirée |
| B · invariants métier | U04–U08, Combat/RPG/Save cités ; caractériser C03-02 dans TransferService/ProgressionTransaction avant réorganisation | Indépendant de A ; limites/prérequis/résistances/ressources/rotations, réentrance/rollback ; diagnostics et chronologie conservés |
| C · outillage Editor | U01/U11/U12, authorings RPG, Lua, EdMode/Toolkit/panels | Indépendant de A/B ; packages jetables, échec SavePackage, Lua, PIE/EditorWorld ; aucun script modifié |
| D · collisions et compatibilité | D08–D13, croissance, classes actuelles/historiques, fixtures, tooltip si nécessaire, INI/alias et écritures natives indispensables | Matrice C03-05 et extraction C03-06 préparées ; atomique si types interdépendants ; UHT/BP/53 assets/v27/anciens chemins/Cook/Shipping |
| E · projections et UI | C03-04/U03, Metadata/Sort/Type/Mesh/Equipment views, InventoryWidget/Slot/Tooltip | Après D pour retirer les paramètres historiques ; une résolution par opération, aucun cache durable nouveau ; tri/icônes/poids/refus/comparaisons inchangés |
| F · orchestration monde/combat | C03-07/U02/U09, unités LevelRuntime/TurnManager/Monster existantes ; déléguer aux politiques actuelles | Après B/E ; sources/coûts/actions, surfaces, rebuild/load, occupation et EndPlay ; pas de nouveau manager central ni d’évolution du ciblage |
| G · interfaces finales | Includes/hubs, visibilités UBT, 53 noms simplifiables et commentaires de transition | Après D/E/F ; public/private/UHT, Editor/Shipping et suite complète ; tous les alias sérialisés conservés |

Validation commune de chaque lot technique : périmètre Git et diff --check, Editor/Shipping, tests ciblés puis Grimrock, SilentTask/LocalAuto SUCCESS au SHA exact. Ajouter hashes originaux et chargement sur copies pour une frontière persistante ; BP/AssetRegistry/CoreRedirects et Cook/Package pour D ou toute évolution réfléchie/asset. Conserver snapshots et reçus privés ; aucun retrait groupé sur la seule taille ou le nombre de candidats.

Hors CLEANUP04 : profiling/optimisations non prouvées → CLEANUP05 ; scripts → CLEANUP06 ; restructuration documentaire → CLEANUP07. Aucun équipement futur implémenté. **DataModel.md:81 conserve une phrase historique sur deux sauvegardes récentes non démontrées, contredite par CLEANUP02** : incohérence signalée, sans modifier ce fichier ni le SVG dans CLEANUP03.

### Contrôle et clôture documentaire

L’exhaustivité porte sur le corpus indexé, pas sur l’absence de toute erreur dynamique. Faux positifs écartés, 13 candidats gradués, 12 familles de duplication et risques de performance ont des critères distincts. Aucun P0 confirmé ; incertitudes Blueprint/callbacks/profiling = conditions de CLEANUP04/05, pas blocages à la publication de l’audit.

Contrôles : recomptage fichiers/types/domaines, chemins/lignes vérifiés, empreintes du corpus inchangées, registre/imports du tooltip en lecture seule, cohérence SVG, observations/hypothèses séparées, lots réalisables et diff --check. **Seul Consolidation.md est modifié** : aucun C++, asset, Blueprint, script, configuration ou SaveGame changé. Un seul commit documentaire ; validation utilisateur requise avant CLEANUP04.

## État courant — ARM-CLEAN06 clos, CLEANUP02 — nomenclature consolidée

ARM-CLEAN06 est **clos**, sur confirmation utilisateur des recettes PIE et Shipping : Continue, inventaire, équipement, présentation, combat et Save/Continue validés. Référence fonctionnelle `420dce2ee4f55c3630e8a99fa7ae9d77299a461e` : SilentTask/LocalAuto SUCCESS, 2 044 tests réussis (48 avec avertissements analysés), sept copies v27 compatibles dont les deux récentes, originaux inchangés, Build/Cook/Stage/Package/Archive Shipping réussis. Preuves privées conservées sous `D:/GrimrockNativeSnapshots/ARM_CLEAN06_FinalAcceptance_420dce2ee4f5_20261122-155244/`. Le problème visuel de ciblage déjà reporté reste hors périmètre.

Le lot des noms internes est publié sous `809a677f` (Editor + Shipping + 2 044 tests locaux, puis statuts Windows exacts contrôlés avant la suite). Le lot réfléchi `9fe0f3a2` a également reçu SilentTask et LocalAuto SUCCESS sur son SHA exact. Il renomme 24 types et dix propriétés, plus 20 helpers/membres C++ associés. **34 Core Redirects** sont ajoutés ; les enums conservent aussi leurs noms de valeurs sérialisées par `ValueChanges`. Les anciennes classes et leurs trois noms occupés restent intactes. Validation locale : Editor et Shipping compilés, 2 044 tests Grimrock réussis, 53 assets avec toutes leurs propriétés métier identiques après résolution des anciens noms, 70 Blueprints compilés sans réenregistrement, sept copies v27 rechargées et round-trippées sans perte. Empreintes originales et Readables inchangées. Les anciens chemins sont testés par le chargement des vrais assets et par les copies v27, sans conversion de sauvegarde.

Le dernier lot organise **53 assets** sous `Items/` (racine, quatre catalogues, 48 définitions), avec **50 noms simplifiés**, par `AssetTools.rename_assets` et enregistrement natif Unreal uniquement. Les 53 fichiers source sont devenus des `ObjectRedirector`, sans suppression des anciens chemins. Snapshot privé vérifié avant écriture ; toutes les propriétés métier sont égales après normalisation des seuls chemins. **50 `PrimaryAssetIdRedirects`** préservent aussi les identités dérivées des noms : ces assets n’étaient pas inscrits à un scan AssetManager, mais leur contrat natif reste couvert. Le Cook conserve `Items/` et `CanonicalStage/`. Comme IoStore n’embarque pas les `UObjectRedirector` en packages, **53 Core Asset Redirects exacts** garantissent la résolution des anciens chemins d’objet par `FSoftObjectPath` en Shipping ; le test vérifie la correspondance `FCoreRedirects`, le chargement du chemin final et l’ancien chemin via son alias natif. En Editor, un redirecteur déjà chargé empêche volontairement le fixup en place ; le contrôle distingue donc ces deux contrats. Les chemins de package natifs restent portés par les redirecteurs Editor ; aucune redirection INI de package seul ne court-circuite le renommage du nom d’objet. Aucun identifiant métier durable n’a changé. Les deux Readables restent à leur emplacement exact.

Le contrôle de retrait dérive ses 53 packages actifs de la racine validée ; il ne dispense pas globalement le dossier `Items/` d’audit. Les redirecteurs de nomenclature ne sont exclus des candidats historiques que s’ils résolvent vers ces packages. Les anciennes définitions, catalogues, palette et redirecteur ThrowingKnife doivent toujours être absents ; seuls les deux Readables restent candidats protégés. Le test existant vérifie en plus les 53 anciens chemins, leurs redirections Core de chemin complet et leurs Primary Asset IDs. Dix-neuf noms internes de leurs consommateurs sont harmonisés (`ItemPreviewRegistry`, `ResolvedProjectileForwardAxis`, `TryResolveLaunchSpeed`, indicateurs d’instances enregistrées), sans changer les décisions du code.

### Validation du dernier lot et intégrité

Development Editor et Shipping compilés ; suite Grimrock complète : **2 044 réussites** (1 997 sans avertissement, 47 avec avertissements attendus), zéro échec et zéro test non exécuté. L’écart 47/48 avec le poste de validation correspond à la disponibilité locale de FireFX déjà identifiée, pas à un test retiré. Les 53 assets ont été rechargés depuis le disque et comparés champ par champ au snapshot ; 70 Blueprints compilés en mémoire, sans réenregistrement, zéro erreur de compilateur. Sept copies v27 : décodage, reconstitution des instances, validation monde, projections et round-trips sémantiques égaux ; empreintes des sept originaux, des copies d’audit et des deux Readables inchangées.

Build/Cook/Stage/Package/Archive Shipping réussis sur le contenu du lot avant publication. Contrôle supplémentaire par listing natif UnrealPak/IoStore : les 53 packages courants sont présents. L’absence des deux Readables dans le premier Cook a conduit à leur inclusion explicite par `DirectoriesToAlwaysCook`, sans modifier leurs assets ; le second packaging les contient bien tous les deux. Le message Windows de rattachement du ShaderCompileWorker au job object n’empêche pas le Cook ; le commandlet de Cook finit avec zéro erreur et zéro avertissement. La validation après push sur le SHA exact et le packaging archivé à ce SHA restent suivis jusqu’à clôture, sans assimiler un résultat parent au nouveau commit.

SVG : XML valide, 96 IDs uniques, 47 entités (37 vertes, cinq ambre, cinq futures), aucun chevauchement ni débordement détecté ; rendu contrôlé. `git diff --check` sans erreur. Preuves et snapshots privés conservés dans `D:/GrimrockNativeSnapshots/CLEANUP02_20261122-162148/` ; aucune donnée de partie publiée.

Inventaire courant après ce lot : **4 355 fichiers/entrées**, dont 1 445 sous `Source/` (1 440 C++ `.h/.cpp/.inl`), 1 378 sous `Content/`, 62 scripts PowerShell et 1 442 fichiers sous `docs/`. L’augmentation de 53 fichiers correspond aux redirecteurs natifs conservés en plus des 53 assets déplacés ; les renommages C++ ne changent pas les effectifs. Aucun fichier utilisateur ou historique n’a été supprimé.

### Exclusions justifiées et protections conservées

- Les trois UCLASS en collision et les fichiers portant directement leur nom attendent l’analyse de leurs consommateurs dans CLEANUP03/CLEANUP04. Les anciennes classes ne sont ni écrasées, ni renommées artificiellement, ni supprimées.
- Les noms/filtres des tests, commandlets et scripts de jalons ainsi que les clés de rapports d’audit restent stables pour la traçabilité et les gardes Windows. Les scripts de snapshot/reprise restent liés à leurs SHA historiques ; leurs anciens chemins ne sont pas des consommateurs de production à réécrire. Réexamen des scripts dans CLEANUP06 uniquement.
- `Legacy` reste explicite quand il désigne réellement un ancien schéma ou une fixture de compatibilité. `Canonical` demeure pour la normalisation mathématique (`GridBoundary::MakeCanonical`), les identités RPG normatives, les comparaisons au modèle enregistré et les preuves historiques. Il ne désigne plus les helpers/familles d’assets renommés.
- `DA_GridLevel_New_Level_03` est un niveau existant : aucun remplacement métier n’est démontré et l’identité du niveau peut être persistée ; report à un examen ciblé, sans modifier LevelId. Les autres `New`/`Old` détectés sont notamment `NewObject`, `NewGuid`, `NewGame`, des états précédent/suivant ou des fragments de mots ; `Stage` du pipeline de packaging est une responsabilité réelle. Aucun fichier Content avec suffixe de transition V2/V3 ou Temporary n’est identifié.
- Les 34 Core Redirects du lot réfléchi, les 53 Core Asset Redirects, les redirections antérieures, les 53 redirecteurs natifs et 50 alias Primary Asset IDs restent nécessaires. Aucun retrait de protection ni fixup destructif de redirecteurs dans CLEANUP02.
- L’équipement futur reste exclusivement conceptuel dans le SVG. Aucun changement fonctionnel, future gamme d’équipement ou recette du ciblage reporté n’est engagé. CLEANUP03 attend une validation finale distincte de CLEANUP02.

<details>
<summary>Organisation native des assets — correspondance exhaustive des 53 packages</summary>

Chemins relatifs à `/Game/GrimrockPrototype/Core/DataAssets/` ; chaque ancien package reste un redirecteur valide.

| Ancien package | Package courant |
| --- | --- |
| `CanonicalStage/DA_ActionCatalogue` | `Items/Catalogues/DA_ActionCatalogue` |
| `CanonicalStage/DA_ActionPresentationCatalogue` | `Items/Catalogues/DA_ActionPresentationCatalogue` |
| `CanonicalStage/DA_CanonicalWeaponCatalogue` | `Items/Catalogues/DA_WeaponCatalogue` |
| `CanonicalStage/DA_CanonicalWeaponVisualCatalogue` | `Items/Catalogues/DA_WeaponVisualCatalogue` |
| `CanonicalStage/DA_ItemSystem` | `Items/DA_ItemSystem` |
| `CanonicalStage/Definitions/DA_Canonical_EmptyVial` | `Items/Definitions/DA_Item_EmptyVial` |
| `CanonicalStage/Definitions/DA_Canonical_Gem_Blue` | `Items/Definitions/DA_Item_Gem_Blue` |
| `CanonicalStage/Definitions/DA_Canonical_GoblinKnife` | `Items/Definitions/DA_Item_GoblinKnife` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Antidote` | `Items/Definitions/DA_Item_Antidote` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Arrows_01` | `Items/Definitions/DA_Item_Arrows_01` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Bomb_Fire` | `Items/Definitions/DA_Item_Bomb_Fire` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Bomb_Toxic` | `Items/Definitions/DA_Item_Bomb_Toxic` |
| `CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Fire` | `Items/Definitions/DA_Item_DefensiveElixir_Fire` |
| `CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Ice` | `Items/Definitions/DA_Item_DefensiveElixir_Ice` |
| `CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Lightning` | `Items/Definitions/DA_Item_DefensiveElixir_Lightning` |
| `CanonicalStage/Definitions/DA_Canonical_Item_DefensiveElixir_Poison` | `Items/Definitions/DA_Item_DefensiveElixir_Poison` |
| `CanonicalStage/Definitions/DA_Canonical_Item_DisarmTools_01` | `Items/Definitions/DA_Item_DisarmTools_01` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Flask_Acid` | `Items/Definitions/DA_Item_Flask_Acid` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Flask_CorrosiveCloud` | `Items/Definitions/DA_Item_Flask_CorrosiveCloud` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Flask_Oil` | `Items/Definitions/DA_Item_Flask_Oil` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Panacea` | `Items/Definitions/DA_Item_Panacea` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Potion_Healing_01` | `Items/Definitions/DA_Item_Potion_Healing_01` |
| `CanonicalStage/Definitions/DA_Canonical_Item_RatMeat` | `Items/Definitions/DA_Item_RatMeat` |
| `CanonicalStage/Definitions/DA_Canonical_Item_RatTooth` | `Items/Definitions/DA_Item_RatTooth` |
| `CanonicalStage/Definitions/DA_Canonical_Item_SurfaceCatalyst_01` | `Items/Definitions/DA_Item_SurfaceCatalyst_01` |
| `CanonicalStage/Definitions/DA_Canonical_Item_TestNote` | `Items/Definitions/DA_Item_TestNote` |
| `CanonicalStage/Definitions/DA_Canonical_Item_Torch` | `Items/Definitions/DA_Item_Torch` |
| `CanonicalStage/Definitions/DA_Canonical_Key_Copper` | `Items/Definitions/DA_Item_Key_Copper` |
| `CanonicalStage/Definitions/DA_Canonical_Key_Iron` | `Items/Definitions/DA_Item_Key_Iron` |
| `CanonicalStage/Definitions/DA_Canonical_Shuriken` | `Items/Definitions/DA_Item_Shuriken` |
| `CanonicalStage/Definitions/DA_Canonical_Stone` | `Items/Definitions/DA_Item_Stone` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ArcaneOrb_01` | `Items/Definitions/DA_Weapon_ArcaneOrb_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ArcaneWand_01` | `Items/Definitions/DA_Weapon_ArcaneWand_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_AssassinDagger_01` | `Items/Definitions/DA_Weapon_AssassinDagger_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_Dagger_01` | `Items/Definitions/DA_Weapon_Dagger_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_Dart_01` | `Items/Definitions/DA_Weapon_Dart_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ElementalStaff_01` | `Items/Definitions/DA_Weapon_ElementalStaff_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_FireWand_01` | `Items/Definitions/DA_Weapon_FireWand_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_GreatAxe_01` | `Items/Definitions/DA_Weapon_GreatAxe_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_HolyAegis_01` | `Items/Definitions/DA_Weapon_HolyAegis_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_HolyScepter_01` | `Items/Definitions/DA_Weapon_HolyScepter_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_Javelin_01` | `Items/Definitions/DA_Weapon_Javelin_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_LongSword_01` | `Items/Definitions/DA_Weapon_LongSword_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_Mace_01` | `Items/Definitions/DA_Weapon_Mace_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_MainGauche_01` | `Items/Definitions/DA_Weapon_MainGauche_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_RecurveBow_01` | `Items/Definitions/DA_Weapon_RecurveBow_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_RoundShield_01` | `Items/Definitions/DA_Weapon_RoundShield_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ShortBow_01` | `Items/Definitions/DA_Weapon_ShortBow_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ShortSpear_01` | `Items/Definitions/DA_Weapon_ShortSpear_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_Stiletto_01` | `Items/Definitions/DA_Weapon_Stiletto_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_StraightSword_01` | `Items/Definitions/DA_Weapon_StraightSword_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_ThrowingKnife_01` | `Items/Definitions/DA_Weapon_ThrowingKnife_01` |
| `CanonicalStage/Definitions/DA_Canonical_Weapon_WarPick_01` | `Items/Definitions/DA_Weapon_WarPick_01` |

</details>

### Plan de nomenclature et résolution des collisions

| Ensemble cohérent | Décision | Validation avant publication |
| --- | --- | --- |
| Helpers C++ non réfléchis | 46 identifiants et 30 fichiers renommés ; `GridEquipmentSlots.h`, `GridItemMeshResolver.h`, `GridProjectileVisuals.h`, `GridItemCatalogues.h` ; consommateurs et garde PowerShell mis à jour sans changement de logique | Compilation Editor, tests Grimrock, Shipping et deux statuts Windows |
| Types et propriétés réfléchis | Noms métier sans Canonical lorsqu’ils sont libres ; Core Redirects conservant les anciens chemins ; aucune valeur ItemId/ActionId/ArchetypeId/GUID modifiée | Assets existants, anciens noms, copies v27, UHT et suite native |
| Organisation des assets | 53 déplacements effectués par AssetTools natif, dont 50 renommages : `Items/DA_ItemSystem`, `Items/Catalogues/`, `Items/Definitions/` ; 53 redirecteurs natifs et 50 alias Primary Asset IDs conservés | Snapshot privé, registre/références/chargement, copies v27 et Cook/Package |
| Trois classes en collision | `UGridCanonicalItemDefinitionAsset`, `UGridCanonicalWeaponCatalogueAsset`, `UGridCanonicalWeaponVisualCatalogueAsset` restent nommées ainsi ; leurs noms cibles sont réellement occupés | Report explicite à CLEANUP03/CLEANUP04 : supprimer leurs consommateurs/déplacer des responsabilités dépasserait une opération de nomenclature |
| Tests, gardes et preuves historiques | Garder les noms/filtres Automation et les reçus figés ; modifier seulement les consommateurs indispensables aux fichiers renommés | Pas de suppression de tests, de scripts ou de Core Redirects historiques |

<details>
<summary>Noms réfléchis et propriétés — correspondances conservées par Core Redirects</summary>

| Avant | Après |
| --- | --- |
| `EGridCanonicalArmamentKind` | `EGridArmamentKind` |
| `EGridCanonicalHandPolicy` | `EGridHandPolicy` |
| `EGridCanonicalActionKind` | `EGridActionKind` |
| `EGridCanonicalEffectKind` | `EGridEffectKind` |
| `EGridCanonicalReactionTrigger` | `EGridReactionTrigger` |
| `FGridCanonicalActionEffect` | `FGridActionEffect` |
| `FGridCanonicalAction` | `FGridAction` |
| `FGridCanonicalRuntimeAction` | `FGridRuntimeAction` |
| `FGridCanonicalAudioVariants` | `FGridAudioVariants` |
| `FGridCanonicalVFXVariants` | `FGridVFXVariants` |
| `FGridCanonicalActionPresentation` | `FGridActionPresentation` |
| `FGridCanonicalRuntimePresentation` | `FGridRuntimePresentation` |
| `FGridCanonicalWeaponVisualBinding` | `FGridWeaponVisualBinding` |
| `EGridCanonicalProjectileVisualMode` | `EGridProjectileVisualMode` |
| `FGridCanonicalProjectileVisual` | `FGridProjectileVisual` |
| `FGridCanonicalWeaponVisual` | `FGridWeaponVisual` |
| `FGridCanonicalProjectilePolicy` | `FGridProjectilePolicy` |
| `FGridCanonicalReactionPolicy` | `FGridReactionPolicy` |
| `FGridCanonicalResolvedWeapon` | `FGridResolvedWeapon` |
| `FGridCanonicalArmorReserveView` | `FGridArmorReserveView` |
| `FGridCanonicalWeaponArchetype` | `FGridWeaponArchetype` |
| `FGridCanonicalClassPreference` | `FGridClassPreference` |
| `EGridCanonicalItemResolveError` | `EGridItemResolveError` |
| `EGridCanonicalBoundActionError` | `EGridBoundActionError` |
| `FGridLooseItemInstance.CanonicalItemId` | `FGridLooseItemInstance.ItemId` |
| `FGridItemSpawnInstance.CanonicalItemId` | `FGridItemSpawnInstance.ItemId` |
| `FGridReceptacleInitialItemConfig.CanonicalItemId` | `FGridReceptacleInitialItemConfig.ItemId` |
| `FGridReceptacleAcceptedItemConfig.CanonicalItemId` | `FGridReceptacleAcceptedItemConfig.ItemId` |
| `FGridObjectPaletteEntry.CanonicalItemId` | `FGridObjectPaletteEntry.ItemId` |
| `FGridObjectPaletteEntry.CanonicalDefaultItemLevel` | `FGridObjectPaletteEntry.DefaultItemLevel` |
| `FRPGStartingEquipmentEntry.CanonicalItemId` | `FRPGStartingEquipmentEntry.ItemId` |
| `AGridItemActor.CanonicalWorldItemLevel` | `AGridItemActor.WorldItemLevel` |
| `AGridItemActor.CanonicalWorldPresentation` | `AGridItemActor.ResolvedWorldPresentation` |
| `FGridCombatSurfaceConversionProfile.bTriggerCanonicalReactionBeforeConversion` | `FGridCombatSurfaceConversionProfile.bTriggerReactionBeforeConversion` |

</details>

Mesure reproductible : identifiants C++ distincts contenant `Canonical`, hors commentaires et chaînes, fichiers `.h`, `.cpp`, `.inl`, tous modules et tests inclus : **258 avant CLEANUP02**, **212 après les noms internes**, **163 après le lot réfléchi**, **144 après harmonisation des consommateurs d’assets**, soit **114 noms normalisés**. La comparaison des 1 440 fichiers C++ au parent de chaque lot confirme uniquement les substitutions prévues ; les deux fichiers du contrôle de retrait et son test sont relus séparément pour le dernier lot. Ce nombre ne compte ni les lignes ni les occurrences textuelles. Les inventaires suivants restent la photographie historique de CLEANUP01.

<details>
<summary>Renommages de fichiers non réfléchis du premier lot — 30 chemins</summary>

| Chemin Source initial | Chemin Source actuel |
| --- | --- |
| `GrimrockPrototype/Private/Runtime/Combat/GridCanonicalActionIconReadModel.h` | `GrimrockPrototype/Private/Runtime/Combat/GridActionIconReadModel.h` |
| `GrimrockPrototype/Private/Runtime/Combat/GridCanonicalEquippedAttackReadModel.h` | `GrimrockPrototype/Private/Runtime/Combat/GridEquippedAttackReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalEquipmentSlotsReadModel.h` | `GrimrockPrototype/Private/Runtime/GridEquipmentSlots.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalExplorationItemReadModel.h` | `GrimrockPrototype/Private/Runtime/GridExplorationItemReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalInventoryDefinitionDiagnosticReadModel.h` | `GrimrockPrototype/Private/Runtime/GridInventoryDefinitionDiagnosticReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalInventoryStackReadModel.h` | `GrimrockPrototype/Private/Runtime/GridInventoryStackReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalItemInteractionReadModel.h` | `GrimrockPrototype/Private/Runtime/GridItemInteractionReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalItemMeshReadModel.h` | `GrimrockPrototype/Private/Runtime/GridItemMeshResolver.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalItemSpawnReadModel.h` | `GrimrockPrototype/Private/Runtime/GridItemSpawnReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalProjectileVisualReadModel.h` | `GrimrockPrototype/Private/Runtime/GridProjectileVisuals.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalQuickItemUseReadModel.h` | `GrimrockPrototype/Private/Runtime/GridQuickItemUseReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridCanonicalThrowBallisticsReadModel.h` | `GrimrockPrototype/Private/Runtime/GridThrowBallisticsReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridReceptacleAcceptedCanonicalReadModel.h` | `GrimrockPrototype/Private/Runtime/GridReceptacleAcceptedReadModel.h` |
| `GrimrockPrototype/Private/Runtime/GridReceptacleInitialCanonicalReadModel.h` | `GrimrockPrototype/Private/Runtime/GridReceptacleInitialReadModel.h` |
| `GrimrockPrototype/Private/Runtime/Items/GridCanonicalItemCatalogues.cpp` | `GrimrockPrototype/Private/Runtime/Items/GridItemCatalogues.cpp` |
| `GrimrockPrototype/Private/UI/GridCanonicalInventoryItemTypeReadModel.h` | `GrimrockPrototype/Private/UI/GridInventoryItemTypeReadModel.h` |
| `GrimrockPrototype/Private/UI/GridCombatHudCanonicalActionReadModel.h` | `GrimrockPrototype/Private/UI/GridCombatHudActionReadModel.h` |
| `GrimrockPrototype/Public/Runtime/Combat/GridCanonicalAttackPresentationReadModel.h` | `GrimrockPrototype/Public/Runtime/Combat/GridAttackPresentationReadModel.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalInventoryCategoryTags.h` | `GrimrockPrototype/Public/Runtime/Items/GridInventoryCategoryTags.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalItemCatalogues.h` | `GrimrockPrototype/Public/Runtime/Items/GridItemCatalogues.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalNonArmamentReadPolicies.h` | `GrimrockPrototype/Public/Runtime/Items/GridNonArmamentReadPolicies.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalNonArmamentThrowReadModel.h` | `GrimrockPrototype/Public/Runtime/Items/GridNonArmamentThrowReadModel.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalPuzzleCategoryTags.h` | `GrimrockPrototype/Public/Runtime/Items/GridPuzzleCategoryTags.h` |
| `GrimrockPrototype/Public/Runtime/Items/GridCanonicalWorldSnapshotReadModel.h` | `GrimrockPrototype/Public/Runtime/Items/GridWorldSnapshotReadModel.h` |
| `GrimrockPrototypeEditor/Private/Details/GridEditorCanonicalArmamentPreview.h` | `GrimrockPrototypeEditor/Private/Details/GridEditorArmamentPreview.h` |
| `GrimrockPrototypeEditor/Private/EditorTools/Widgets/GridEditorCanonicalPaletteReadModel.h` | `GrimrockPrototypeEditor/Private/EditorTools/Widgets/GridEditorPaletteReadModel.h` |
| `GrimrockPrototypeEditor/Public/EditorTools/GridEditorCanonicalItemLevelPolicy.h` | `GrimrockPrototypeEditor/Public/EditorTools/GridEditorItemLevelPolicy.h` |
| `GrimrockPrototypeEditor/Public/EditorTools/GridEditorCanonicalLockRuleAuthoring.h` | `GrimrockPrototypeEditor/Public/EditorTools/GridEditorLockRuleAuthoring.h` |
| `GrimrockPrototypeEditor/Public/EditorTools/GridEditorCanonicalReceptacleInitialAuthoring.h` | `GrimrockPrototypeEditor/Public/EditorTools/GridEditorReceptacleInitialAuthoring.h` |
| `GrimrockPrototypeEditor/Public/EditorTools/GridEditorCanonicalReceptacleRuleAuthoring.h` | `GrimrockPrototypeEditor/Public/EditorTools/GridEditorReceptacleRuleAuthoring.h` |

</details>

## 1. État et preuves historiques de CLEANUP01


| Contrôle préalable | Résultat vérifié |
| --- | --- |
| HEAD local / distant | `a8de4f58b6c7ecc42e62aef0dbbe8fa9b7ede272` pour les deux après `git fetch origin master` ; aucune divergence |
| Working tree initial | Seulement `?? docs/Architecture/DataModel.svg`, dépôt manuel attendu et autorisé |
| Branche | `master` ; aucune branche créée |
| Worktree principal | `D:/Development/GrimrockPrototype`, `master`, HEAD ci-dessus |
| Worktree de validation | `D:/Development/GrimrockPrototype_UEValidation`, détaché au même HEAD |
| Worktree EffectsOrgTorch | Détaché à `c951ccafb3d3c0071d5aa3a32bd64a631a1f56b5` ; conservé |
| Ancien worktree RECOVERY | Référence `backup-before-lua02-reset`, `29deaf5c3bd2ffddf92182c8c954a0f7592acb27`, signalé `prunable` car chemin absent ; ni prune ni réparation effectué |
| Dépendance externe | Sous-module `ThirdParty/Lua54` à `6e22fedb74cf0c9b6656e9fce8b7331db847c605`, sans marqueur de divergence dans `git submodule status` |
| `UE5.5.4/SilentTask` | `success` sur le SHA de référence, vérifié par GitHub ; reçu local `SilentTaskVerified.sha` concordant |
| `UE5.5.4/LocalAuto` | `success` sur le même SHA ; état agent du 22.11.2026 à 13:07:36 +01:00, code 0 |
| Dernière Automation | Rapport `a8de4f58b6c7-20261122-130536/Automation/TD04-20261122-130610` : 1 996 réussites, 48 réussites avec avertissements, 0 échec, 0 non exécuté ; total 2 044 |
| Portée de LocalAuto | Compilation Development Editor, Shipping compile-only et suite `Grimrock` ; l'agent active effectivement `-BuildShipping` |

### ARM-CLEAN06 : retrait effectif, clôture globale non démontrée

Le code et les binaires actuels confirment une migration très avancée, pas simplement une intention de conception :

- `Config/DefaultGame.ini` configure réellement `CanonicalStage/DA_ItemSystem` ; la racine et ses quatre catalogues existent en packages Unreal matérialisés.
- 48 définitions actuelles : 22 ARM et 26 non ARM. Le catalogue mécanique possède 82 archétypes et six préférences de classe ; les 82 lignes ne sont pas 82 armes complètement disponibles.
- `c2bd170c` retire les anciens chargeurs de kits ; `af7cfc3b` retire exactement 48 définitions historiques et le redirecteur ThrowingKnife ; `a8de4f58` retire les deux catalogues historiques d'authoring. L'ancienne palette d'icônes avait déjà été retirée. Ces suppressions sont antérieures à CLEANUP01.
- Le test natif `Grimrock.Editor.ARM.CLEAN06_5A.NativeRetirementEvidence` exige l'absence des anciennes définitions/catalogues/redirecteur et conserve les deux Readables protégés. La dernière suite est réussie sur le HEAD final de ces retraits.
- Des classes, signatures, fixtures, commandes neutralisées et corps historiques sous `WITH_DEV_AUTOMATION_TESTS` subsistent. Ils ne prouvent pas une seconde autorité Shipping, mais contredisent une lecture littérale de « zéro legacy dans tout le dépôt ».

**ARM-CLEAN06 n'est pas déclaré entièrement clos par ce relevé.** Les reçus [retrait des définitions](../Architecture/ARM_CLEAN06_5H25_NATIVE_RETIREMENT_RECEIPT.md) et [retrait des catalogues](../Architecture/ARM_CLEAN06_5H26_SOURCE_CATALOGUE_RETIREMENT.md) réservent encore la recette PIE, Cook/Package et Save/Continue. Un packaging Shipping réussi le 21 novembre, rapporté dans `ARM_CLEAN06_CURRENT_HANDOFF.md`, précède les derniers retraits du 22 novembre : il ne valide pas leur état final. Il faut une preuve au SHA post-retrait avant clôture de recette.

Le contrat normatif et `AGENTS.md` comportent toujours des phrases « non encore implémenté ». Le point de reprise courant contient des états intermédiaires contradictoires et n'est pas un certificat de clôture. Ce lot les signale sans les modifier, conformément au périmètre de trois fichiers.

### Sauvegardes : preuve conservée et limite actuelle

Le rapport privé `ARM_CLEAN06_5A_c2bd170cc7bc_…/retirement-evidence.json` est lisible localement, daté du parent précédant le retrait des packages : 48 identités, 22 ARM, 466 résolutions objet/niveau, 14 actions détaillées, 53 candidats classés, zéro arête externe AssetRegistry ; sept sauvegardes v27 décodées, zéro identité inconnue, sept validations physiques et round-trips mémoire positifs. Les preuves spécialisées monde/cache sémantique/projection de schéma sont activées ; `weight_cache_evidence` est faux dans ce rapport, ne pas le présenter comme un audit indépendant complet du poids.

Empreinte SHA256 du rapport : `076b9f59df062d8a2c709e439c6f4e9942ffa2f2471b67ddb93fc0781b26aa17`. Les sept copies existent encore sous `AuditCopies`. Le reçu de copie identifie `c2bd170cc7bcbb50d568718c353f4b1d22a452f6` et les empreintes des fichiers. Aucune donnée privée de partie, aucun nom de slot ni contenu de sauvegarde n'est reproduit ici.

Sur les sept fichiers live actuels, **cinq** correspondent aux empreintes de ce reçu, **deux** diffèrent. Cette différence ne démontre ni corruption ni incompatibilité : des parties ont pu être sauvegardées depuis. Elle interdit de certifier que le rapport antérieur décrit les sept fichiers live actuels. CLEANUP01 lit seulement les fichiers et leurs empreintes ; il ne les décode pas avec une nouvelle session UE, ne les convertit pas et ne les réécrit pas.

Le schéma courant exige exactement v27 (`UGrimrockPartySaveGame::IsCompatible` et `Serialize`), sans migration automatique des versions précédentes. Les deux `DA_BookPage_Test` et `DA_Scroll_Test` restent nécessaires aux références Readable de sauvegarde. Les conserver jusqu'à une migration explicitement séparée et validée. Les limites de recette n'empêchent pas la publication de cette documentation ; elles empêchent de déclarer ARM-CLEAN06 totalement clos.

## 2. Inventaire quantifié et méthode

Les comptes ci-dessous décrivent le **parent de CLEANUP01**, avant l'ajout des trois livrables. `git ls-files -z` assure la prise en compte exacte des espaces et accents et inclut une entrée gitlink pour Lua. Les produits ignorés, fichiers privés et le contenu interne du sous-module ne sont pas des fichiers versionnés du dépôt principal. Après ce lot : +3 fichiers documentaires, sans variation du code et des assets.

L'inventaire des chemins est exhaustif dans les annexes repliables de ce fichier. Le corpus C++ a été parcouru intégralement pour les noms, volumes, directives et dépendances textuelles ; les parcours critiques objets/sauvegardes et les principaux candidats ci-dessous ont été relus. Il s'agit d'un audit préparatoire statique, pas d'une preuve d'absence de code mort, ni d'une analyse sémantique exhaustive de 314 597 lignes. Les candidats requièrent l'analyse dédiée CLEANUP03 avant décision.

| Ensemble versionné | Entrées |
| --- | ---: |
| Total dépôt principal | 4 299 |
| `Source/` | 1 445 |
| `Content/` | 1 325 |
| `docs/` | 1 439 |
| `Scripts/` (tous formats) | 66 |
| `Config/` | 9 |
| Autres entrées, dont gitlink Lua | 15 |
| C++ `.cpp` / `.h` / `.inl` | 1 007 / 410 / 23 = 1 440 |
| Règles/targets C# | 5 |
| Fichiers source dans un répertoire `Tests/` | 698 |
| Lignes C++ physiques, commentaires et blancs inclus | 314 597 |
| Assets `.uasset` / niveaux `.umap` | 1 313 / 2 |
| PowerShell versionné `.ps1` | 62 ; aucun `.psm1` ou `.psd1` |
| PowerShell local récursif, hors `.git` seulement | 66 `.ps1`, dont 4 ignorés/non versionnés |
| Documentation `docs/` | 1 325 MD, 54 PNG, 29 SVG, 12 JPG, 10 CSV, 7 PDF, 2 XLSX |

| Module / règles | Fichiers | `.cpp` | `.h` | `.inl` | `.cs` | Lignes C++ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `GrimrockLua` Runtime | 10 | 7 | 2 | 0 | 1 | 1 986 |
| `GrimrockPrototype` Runtime | 1 053 | 730 | 322 | 0 | 1 | 242 495 |
| `GrimrockPrototypeEditor` Editor | 380 | 270 | 86 | 23 | 1 | 70 116 |
| Deux targets à la racine `Source/` | 2 | 0 | 0 | 0 | 2 | 0 |
| **Total** | **1 445** | **1 007** | **410** | **23** | **5** | **314 597** |

Le module Lua dépend de Core/CoreUObject et des sources officielles du sous-module. Le module Runtime dépend notamment d'Engine, EnhancedInput, UMG, Niagara, Lua ; Slate/SlateCore/AssetRegistry sont privés. Le module Editor ajoute UnrealEd, AssetTools, PropertyEditor, BlueprintGraph, MaterialEditor et les services d'édition. Les trois modules désactivent Unity ; ce choix est commenté pour éviter les collisions de helpers locaux et macros Lua. Ne pas le renverser pour gagner du temps de build sans preuve de sûreté.

## 3. Nomenclature et collisions à traiter dans CLEANUP02

Le tableau distingue le repérage large d'un véritable renommage. Les nombres de fichiers « contenu C++ » incluent commentaires et chaînes ; ils ne sont pas des nombres de classes. Les listes exactes des chemins et des identifiants lexicaux sont dans les annexes.

| Motif, recherche insensible à la casse | Chemins versionnés contenant le motif | Fichiers C++ le contenant |
| --- | ---: | ---: |
| Canonical | 266 | 629 |
| Legacy | 38 | 316 |
| Stage | 56 | 119 |
| Temporary | 0 | 34 |
| Old | 19 | 365 |
| New | 18 | 756 |
| ARM_CLEAN06 | 498 | 21 |
| ARMEMENT | 223 | 163 |
| INDUS | 16 | 16 |
| WORLDOBJ | 59 | 256 |

`Gold`, `Bold`, `Holder`, `Cooldown`, `ArcaneWand`, `NewObject`, `NewGame` et `NewGuid` produisent des faux positifs de la recherche large ou des noms de domaine légitimes. `Stage` peut désigner une phase de packaging ; `Temporary` une durée de vie locale correcte. Aucune substitution globale n'est recommandée.

| Candidat | Direction proposée, non appliquée | Collision / précaution |
| --- | --- | --- |
| `UGridCanonicalItemDefinitionAsset` | `UGridItemDefinitionAsset` une fois l'ancien type réellement retiré | **Nom déjà occupé** par le type historique ; références UHT, chemins `/Script`, BP et sérialisation |
| `UGridCanonicalWeaponCatalogueAsset` | `UGridWeaponCatalogueAsset` après retrait du type historique | **Nom déjà occupé** ; structures de croissance de l'ancien header encore réutilisées |
| `UGridCanonicalWeaponVisualCatalogueAsset` | `UGridWeaponVisualCatalogueAsset` après analyse de l'ancien type | **Nom déjà occupé**, fixtures et types visuels historiques encore référencés |
| `CanonicalStage/` et `DA_Canonical_*` | Dossier stable `Items/` et préfixes UE de rôle `DA_Item_*` / `DA_Weapon_*`, à décider | Certains anciens chemins supprimés restent inscrits dans les tests de non-réintroduction ; collisions futures et redirecteurs à inventorier avant édition native |
| `FGridCanonical*`, `bCanonicalIdentity`, `HasCanonicalItem`, `GetCanonicalItemIds` | Nom du domaine et du résultat, sans qualificatif de migration | Fonctions exposées et propriétés Blueprint ; ne pas perdre les garanties d'identité dans le renommage |
| `CanonicalItemId` / `ItemDefinitionId` / `ItemId` | Clarifier définition, placement et instance avant harmonisation | Tous désignent la jointure de définition, mais les noms sérialisés et leurs propriétaires diffèrent |
| `GridLegacy*Gate`, `FGridLegacyItemLookup` | Retirer seulement après extinction des fixtures, sinon nom explicite du garde | Le garde est une protection, pas un repli runtime à réactiver |
| Classes/tests `ARMCLEAN…`, `ARMEMENT…`, `TD…`, `MON…`, `WORLDOBJ…` | Scénarios et responsabilités de domaine | Les filtres Automation et scripts historiques consomment ces noms ; conserver la couverture et adapter les consommateurs ensemble |
| Commentaires « temporary », « not yet authored », « legacy-backed … do not switch » | Décrire le comportement actuel | Exemples dans `GridItemSubsystem` et les définitions ; des commentaires pré-cutover survivent à la conversion |

Conventions proposées : types Unreal `U/A/F/E`, booléens `b…`, assets `DA_`, `BP_`, `WBP_`, `SM_`, `T_`, vocabulaire métier court, pas de numéro de jalon dans une API durable. Décider d'un seul usage de `Catalogue` / `Catalog` selon les concepts existants ; ne pas renommer les types Engine. Aucun nom cible ci-dessus n'est une autorisation de conversion.

## 4. C++ : candidats et limites

| Candidat vérifié | Constat / risque | Action ultérieure et preuve attendue |
| --- | --- | --- |
| `GridWeaponCatalogueImportCommandlet.cpp` | `Main` ne fait que signaler le retrait et retourner 1 ; classe enregistrée conservée pour les consommateurs historiques | Candidat fort au retrait après inventaire des tests/lanceurs ; maintenir un refus explicite pour les anciennes procédures |
| `UGridItemDefinitionAsset`, anciens catalogues, `GridWeaponVisualResolver`, `GridWorldItemVisualResolver` | Types et helpers historiques encore compilés partiellement ; plusieurs entrées visuelles sous `WITH_DEV_AUTOMATION_TESTS` | Distinguer interfaces actives, fixtures et véritables corps inaccessibles ; pas de suppression par simple comptage de références textuelles |
| `GridLegacyItemLookup` / registration / actor-init gates | Replis de caractérisation limités à Automation ; Shipping retourne sans lecture historique | Préserver les tests de refus pendant le retrait, puis supprimer ensemble signatures, fixtures et chemins de garde devenus inutiles |
| Actions / présentations compactes et détaillées | Deux formes dans les mêmes catalogues, plusieurs consommateurs ; `UseActions` / `ActionBindings` côté non ARM | Définir les responsabilités et la parité avant fusion ; aucune forme n'est déclarée morte aujourd'hui |
| `GridCanonicalItemMeshReadModel`, `GridEquipmentPreviewItemReadModel`, `GridInventoryItemMetadataReadModel`, `GridItemInstanceMaterializationReadModel` | Plusieurs projections du même registre, paramètres historiques encore propagés | Cartographier qui valide identité/niveau et qui formate/présente ; réunir seulement les validations identiques, garder les vues de rôle utiles |
| `GridItemWeightReadModel`, `GridWorldPhysicsMassReadModel`, `GridInventoryDiagnosticWeightReadModel`, `GridInventorySortReadModel` | Résolutions répétées plausibles ; poids unitaire monde et poids total de pile ont des sémantiques distinctes | Mesurer appels/coûts avant cache ; ne pas fusionner masse physique unitaire et charge d'inventaire |
| `GridCanonicalItemCatalogues.cpp::Find*` | `FindByPredicate` linéaires pour actions, présentations, archétypes et visuels ; double recherche binding/profil | Avec 48 objets/82 lignes, coût non quantifié ; profiler d'abord, envisager des index immuables seulement si justifiés |
| `UGridItemSubsystem::ResolveItem` et vues résolues | Reconstitue noms, tags, actions et profils à chaque lookup ; `ItemIndex` est déjà un index, pas un cache inutile démontré | Mesurer fréquence et copies ; réutiliser une vue à l'échelle d'une opération si nécessaire, sans nouveau cache durable |
| Chargements synchrones | Racine au démarrage ; API de preview `LoadConfiguredItemSystemForPreview` exposée ; meshes/icônes chargés aux transitions par leurs consommateurs | Auditer les sites réellement appelés au Tick/UI ; la simple présence de `LoadSynchronous` n'est pas une régression démontrée |
| `GridTurnManagerPlayerActionCatalog.cpp` | 3 815 lignes ; collecte d'actions, sources et nombreuses politiques | Cartographier les responsabilités avant extraction ; tests d'autorisation et d'identité source indispensables |
| `GridCombatTypes.h` | 3 237 lignes, nombreuses structures dans un header partagé | Mesurer graphe d'includes / recompilation ; extraire par frontière cohérente, pas un header par petit type |
| `GridInventoryWidget.cpp` / `GridLevelRuntimeActor.cpp` / `GridPartyInventoryComponent.cpp` | Respectivement 2 721 / 2 339 / 2 165 lignes ; UI, monde et possession sont des points de concentration | Conserver les invariants de transfert, réserves et rechargement ; réduire les responsabilités si le graphe le justifie |
| `GridItemCacheParityAudit` et preuves Editor de migration | Diagnostic relisant plusieurs modèles, `WITH_EDITOR` ; utile aux gardes historiques, pas un coût Shipping établi | Condenser les tests de migration en invariants permanents après identification de leurs preuves uniques |
| Suite de tests historique | 698 fichiers, noms de jalons, caractérisations parfois migrées pour pointer la même source | Une comparaison à la même racine n'est pas une preuve indépendante de parité ; préserver assertions métier et refus sans conserver toutes les étapes |

Il n'y a pas de suppression de code mort certaine déduite de l'absence de `rg` : réflexion Unreal, UFUNCTION, Blueprint, commandlets et sérialisation sont des consommateurs possibles. Les noms candidats sont indexés intégralement dans les annexes. Aucun test de miroir ni refactorisation n'est ajouté dans ce lot documentaire.

## 5. PowerShell : usage, dépendances et candidats

Les 62 scripts versionnés et quatre scripts locaux sont classés individuellement dans l'annexe dédiée. Catégories : compilation, tests, packaging, infrastructure Windows, migration, réparation, génération ponctuelle, audit historique, autre. La classification est un rôle principal ; plusieurs scripts orchestrent plusieurs activités.

Chaîne courante à conserver : tâche Windows silencieuse `wscript.exe //B //Nologo` → launcher VBS généré hors dépôt → `UEValidationBootstrap.ps1` → `UEValidationAgent.ps1` → `UEValidationSelectSDK.ps1` → `ValidateUE.ps1` → builds + Automation → publication des deux statuts sur le SHA exact. Le launcher VBS n'est pas un fichier versionné à supprimer. Git/Git LFS, UE 5.5.4, VS 2026 avec MSVC 14.44.35207 et SDK sélectionné sont des dépendances d'exécution, pas des fichiers à embarquer avec le jeu.

`ValidateUE.ps1` appelle encore **dix scripts** de protection de migration/retrait : rollback 4X66, audit 5A, plan H12, snapshot H14, continuité H15, canary H16, gardes Shipping H18, inspection H23, installation Readable 5B et preflight sauvegardes 5G85. Leurs autotests et dépendances doivent être remplacés ou maintenus ensemble ; la table ci-dessous identifie les appels directs.

Candidats au retrait ou à la condensation : captures/baselines ARM-CLEAN06, comparateurs de transactions privées, plans ponctuels et anciens exports d'authoring. **Ils restent reliés au préflight ou à des reçus privés pour certains** ; retirer un script seul casserait la validation. Conserver une protection permanente minimale contre la réintroduction des anciennes autorités.

Deux cas concrets : `MigrateWorldObjectAssets.ps1` demande `GridWorldObjectMIG08`, absent des sources actuelles ; `AuthorARMEMENT014.ps1` demande `GridWeaponCatalogueImport`, désormais volontairement inerte (code 1). Ce sont des procédures historiques non exécutables pour recréer les données actuelles. Les wrappers RPG/UI, de localisation et les réparations de talents peuvent encore être des outils d'authoring actifs : ne pas les classer obsolètes sur leur seul âge.

`Build/Build.ps1`, ignoré, double un packaging Development avec chemins absolus. Les deux scripts sous `Saved/` sont des traces opérateur. `Sounds/Weapon/Renommer-SFX-Sword.ps1` est un utilitaire local de renommage audio avec aperçu par défaut. Aucun de ces quatre fichiers ne devient versionné ou n'est supprimé dans ce lot.

## 6. Documentation : état et structure recommandée

La documentation représente 1 439 fichiers versionnés, dont 568 sous `docs/Architecture/` et 744 sous `docs/Design/`. Les annexes recensent tous les chemins ; leur classement est un tri préparatoire reproductible, pas une autorisation de suppression de chaque texte sans relecture.

| Statut | Références / décision |
| --- | --- |
| Référence courante | `docs/README.md`, contrats UI/localisation, règles de projet, licences, architecture des domaines, environnement ; conserver un point d'entrée court |
| Spécification encore applicable | Invariants du contrat `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`, contrat d'authoring, contrats de possession/sauvegarde/combat ; confronter les prescriptions et noms au code |
| Historique utile | Rapports de jalons, matrices de parité, reçus de retrait, décisions artistiques et diagnostics qui expliquent les invariants actuels |
| Redondance | `ARM_CLEAN06_CURRENT_HANDOFF.md`, centaines de rapports successifs, index et cartes qui répètent des états intermédiaires ; condenser après extraction des décisions et preuves finales |
| Obsolescence constatée | « modèle non encore exécuté » dans le contrat/AGENTS ; référence à `Scripts/WeaponPipeline.ps1` absent ; anciens noms d'assets sources supprimés présentés comme procédures courantes |
| Référence scripts datée | `docs/SCRIPTS_REFERENCE.md` décrit surtout septembre/octobre et n'inventorie pas les 62 scripts actuels ; ses constats MIG08 restent pertinents |
| Art et données d'appui | ArtBook, images, tables et PDF : préserver licences, sources et provenance ; différencier documentation technique et productions artistiques |

Structure cible proposée, sans déplacement dans CLEANUP01 :

```text
docs/
  README.md                 # entrée courte et liens vers les autorités
  Architecture/             # modèle de données et contrats durables
  Technical/                # environnement, validation, performances, scripts
  Design/                   # seules spécifications gameplay encore applicables
  Tests/                    # recette humaine et critères de sortie
  History/                  # synthèses condensées des décisions/migrations
  ArtBook/ + Images/        # ressources utiles, provenance et licences
```

En CLEANUP07, extraire une synthèse par grande migration dans `docs/History/`, conserver SHA des preuves et décisions encore utiles, puis supprimer/fusionner les micro-rapports seulement après contrôle des liens et de l'information unique. Git conserve le détail historique ; ne pas déplacer mécaniquement 1 000 rapports dans History. Les obligations de `AGENTS.md` et les contrats normatifs ne changent pas implicitement par cette proposition.

## 7. Risques et critères de validation

| Risque | Priorité | Garde avant changement ultérieur |
| --- | --- | --- |
| Clôture ARM-CLEAN06 confondue avec tests verts | Haute | PIE, Cook/Package et Save/Continue au SHA post-retrait ; annoncer les limites tant que les preuves manquent |
| Sauvegardes privées et Readables | Haute | Nouvelle preuve en lecture seule sur copies des fichiers récents ; préserver originaux et références ; aucune conversion implicite |
| Renommage de classes sérialisées déjà occupées | Haute | Graphe UHT/BP/AssetRegistry, plan d'identités, conversion Editor et sauvegardes avant retrait des anciens types |
| Nettoyage d'un script encore appelé | Haute | Graphe exact lanceurs/autotests/tâche Windows ; remplacer les gardes avant retrait |
| Perte de comportement en fusionnant les vues | Haute | Refus d'IDs inconnus, niveaux stricts, main physique, propriété unique, réserves, lecture et piles inchangés |
| Nouvelle autorité d'équipement | Haute | Réutiliser les définitions unifiées ; futurs champs gris jusqu'à conception/implémentation validées |
| Optimisation non mesurée | Moyenne | Baseline Insights reproductible, coûts p50/p95 et nombre de résolutions ; pas de cache persisté |
| Suppression d'historique utile ou de fixtures uniques | Moyenne | Extraire invariants et preuves indépendantes, puis vérifier leur couverture avant condensation |
| Nettoyage de worktrees | Faible pour ce lot | Aucun prune/reset ; examiner les travaux éventuels séparément |

Validation de CLEANUP01 : XML valide ; 29 entités d'origine traçables dans 47 blocs ; attributs et relations confrontés aux headers et résolveurs ; légende explicite ; rendu final contrôlé panneau par panneau ; états implémenté/transition/futur concordants dans les trois fichiers ; inventaires recalculés depuis le manifest NUL ; périmètre Git strictement limité aux trois chemins ; `git diff --check` sans erreur. Une compilation UE n'est pas nécessaire pour ce changement documentaire. Les statuts du parent sont une preuve de baseline, pas les statuts du futur commit CLEANUP01 : ceux du SHA publié doivent être contrôlés séparément dans le compte rendu.

## 8. Roadmap de consolidation

| Lot | Objectif limité | Critère de sortie |
| --- | --- | --- |
| **CLEANUP01 — Architecture et modèle de données** | Trois livrables actuels, inventaire, autorités et limites | XML/rendu/diff/inventaires vérifiés ; publication atomique ; validation utilisateur avant CLEANUP02 |
| **CLEANUP02 — Nomenclature** | Noms métier courts, collisions et noms sérialisés | Table avant/après approuvée, aucun conflit de type/package, conversions natives si nécessaires, filtres et sauvegardes préservés |
| **CLEANUP03 — Audit C++** | Graphe de dépendances et appels, code mort prouvé, duplications | Liste de décisions avec preuves et couverture ; aucune suppression sur simple absence de référence textuelle |
| **CLEANUP04 — Refactorisation C++** | Appliquer les décisions par responsabilités cohérentes | Même comportement, Editor et Shipping compilés, Automation verte, recette ciblée et modèle actualisé |
| **CLEANUP05 — Performances** | Profiler résolution, UI, monde, chargements et compilation | Mesures avant/après reproductibles, amélioration démontrée sans cache durable redondant |
| **CLEANUP06 — Scripts PowerShell** | Réduire lanceurs et protections historiques | Chaîne silencieuse intacte, dépendances résolues, autotests négatifs et statuts exacts vérifiés |
| **CLEANUP07 — Documentation** | Références courtes et History condensé | Liens valides, autorités uniques, décisions/provenance conservées, procédures exécutables |
| **CLEANUP08 — Recette générale** | Recette intégrée du prototype consolidé | Editor/Shipping + Automation, PIE visuelle, package installé, nouvelle partie et Save/Continue sur copies, pas de divergence Git |

La réalisation des futurs équipements est hors de cette roadmap de nettoyage tant qu'un lot fonctionnel distinct n'est pas explicitement décidé. Aucun lot suivant n'est engagé par CLEANUP01.

## Annexes — inventaires exhaustifs du parent
Ces annexes sont des index de recherche repliables. Le corps du document porte les décisions ; aucun fichier listé n’est supprimé ou renommé par sa présence ici. Les noms relatifs sont regroupés par répertoire pour éviter de répéter les mêmes chemins.

### A. Scripts PowerShell — 66 fichiers locaux dont 62 versionnés
Dépendances indiquées : autres scripts réellement nommés, commandlets littéraux et outils/données principaux. Les chemins privés des reçus ne sont pas publiés. La colonne « suite » identifie les appels directs du préflight central ; un script historique peut donc être nécessaire aujourd’hui. Aucun script n’a été exécuté pour cet inventaire.
| Chemin | Rôle principal | Dépendances / usage | Suite / disposition |
| --- | --- | --- | --- |
| `Build/Build.ps1` | packaging | UE/UBT/UAT et assets projet | local ignoré ; préserver |
| `Saved/ARMEMENT01.7.1/ARMEMENT01.7.1-LOCAL.ps1` | migration | `ValidateUE.ps1` | local ignoré ; préserver |
| `Saved/Codex/MIG09/RunAutomation.ps1` | tests | UE/UBT/UAT et assets projet | local ignoré ; préserver |
| `Scripts/ARM_CLEAN06_4X66_Rollback.ps1` | migration | JSON/empreintes privés ; Git et état de transaction | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/AnalyzeARMWeaponProgression.ps1` | audit historique | `ExportARMEMENT014Catalogue.ps1` | candidat après extinction des consommateurs |
| `Scripts/AssertARM_CLEAN06_5H18_ShippingGuards.ps1` | tests | PowerShell ; fichiers projet selon paramètres | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/AuditARM_CLEAN06_4S_TransactionRehearsal.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | candidat après extinction des consommateurs |
| `Scripts/AuditARM_CLEAN06_5_Retirement.ps1` | audit historique | `ARM_CLEAN06_4X66_Rollback.ps1` ; UE `-run=GridArmClean065RetirementAudit` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/AuditStarterWeaponArt.ps1` | audit historique | PowerShell ; fichiers projet selon paramètres | candidat après extinction des consommateurs |
| `Scripts/AuthorARMEMENT014.ps1` | génération ponctuelle | `ExportARMEMENT014Catalogue.ps1` ; `ValidateUE.ps1` ; UE `-run=GridWeaponCatalogueImport` | historique ; import désormais refusé |
| `Scripts/AuthorOilSurfaceMaterial.ps1` | génération ponctuelle | UE `-run=GridOilSurfaceMaterialAuthoring` | appel agent ; conserver/remplacer ensemble |
| `Scripts/AuthorRPGAlchemist.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGAlchemistAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGClassProgression.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGClassProgressionAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGMage.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGMageAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGPriest.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGPriestAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGRanger.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGRangerAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGRogue.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGRogueAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorRPGWarrior.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=RPGWarriorAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorUIRPGPresentation.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=UIRPGPresentationAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/AuthorUIRPGTalentTree.ps1` | génération ponctuelle | `ValidateUE.ps1` ; UE `-run=UIRPGTalentTreeAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/CaptureARM_CLEAN06_Baseline.ps1` | audit historique | UE `-run=GridArmClean06Baseline` | candidat après extinction des consommateurs |
| `Scripts/CapturePerfProfile01.ps1` | autre | UE/UBT/UAT et assets projet | outil courant ou authoring à confirmer |
| `Scripts/CheckCppFormat.ps1` | tests | `FormatCpp.ps1` | outil courant ou authoring à confirmer |
| `Scripts/CheckEffectsOrgMigrationScripts.ps1` | tests | `MigrateEffectsOrgTorch.ps1` ; `RunEffectsOrgTorchStandalone.ps1` | outil courant ou authoring à confirmer |
| `Scripts/CheckProjectDependencies.ps1` | autre | PowerShell ; fichiers projet selon paramètres | outil courant ou authoring à confirmer |
| `Scripts/CleanProject.ps1` | autre | UE/UBT/UAT et assets projet | outil courant ou authoring à confirmer |
| `Scripts/CompareARM_CLEAN06_4B_Baselines.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | candidat après extinction des consommateurs |
| `Scripts/CompareARM_CLEAN06_4F_ThreeWayProvenance.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | candidat après extinction des consommateurs |
| `Scripts/ExportARMEMENT014Catalogue.ps1` | génération ponctuelle | PowerShell ; fichiers projet selon paramètres | outil courant ou authoring à confirmer |
| `Scripts/ExportARMEMENT01952Affinities.ps1` | génération ponctuelle | `ExportARMEMENT014Catalogue.ps1` | outil courant ou authoring à confirmer |
| `Scripts/FixRPGRangerFavoredEnemyTitles.ps1` | réparation | `ValidateUE.ps1` ; UE `-run=RPGRangerAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/FormatCpp.ps1` | autre | clang-format 19.1.5 ; `.clang-format` | outil courant ou authoring à confirmer |
| `Scripts/GenerateStarterWeaponArt.ps1` | génération ponctuelle | Python ; sources art/tables | outil courant ou authoring à confirmer |
| `Scripts/GetARM_CLEAN06_5G89_CacheRefusal.ps1` | audit historique | `ARM_CLEAN06_4X66_Rollback.ps1` | appel agent ; conserver/remplacer ensemble |
| `Scripts/InspectARM_CLEAN06_5H23_HistoricalSources.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/InstallARM_CLEAN06_5B_ReadableSaveCopy.ps1` | réparation | `ARM_CLEAN06_4X66_Rollback.ps1` ; UE `-run=GridArmClean065RetirementAudit` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/InstallUEValidationAgent.ps1` | infrastructure Windows | `UEValidationAgent.ps1` ; `UEValidationBootstrap.ps1` ; `UEValidationSilentTask.ps1` | outil courant ou authoring à confirmer |
| `Scripts/Localization/ApplyEnglishSmokeTranslations.ps1` | génération ponctuelle | PowerShell ; fichiers projet selon paramètres | outil courant ou authoring à confirmer |
| `Scripts/Localization/RunLocalizationSmoke.ps1` | tests | `ApplyEnglishSmokeTranslations.ps1` ; `TestLocalizationScripts.ps1` ; UE `-run=GatherText` ; UE `-run=UILocalizationEnglishSmoke` ; UE `-run=UIStringTableAuthoring` | outil courant ou authoring à confirmer |
| `Scripts/Localization/TestLocalizationScripts.ps1` | tests | `ApplyEnglishSmokeTranslations.ps1` ; `RunLocalizationSmoke.ps1` | outil courant ou authoring à confirmer |
| `Scripts/MigrateEffectsOrgTorch.ps1` | migration | UE `-run=GridEffectsOrgTorchMove` | candidat après extinction des consommateurs |
| `Scripts/MigrateWorldObjectAssets.ps1` | migration | `ValidateUE.ps1` | candidat après extinction des consommateurs |
| `Scripts/PlanARM_CLEAN06_4D_EditorDryRun.ps1` | audit historique | UE `-run=GridArmClean06Baseline` | candidat après extinction des consommateurs |
| `Scripts/PlanARM_CLEAN06_5H12_Retirement.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/PreflightARM_CLEAN06_2.ps1` | audit historique | UE `-run=GridArmClean06Baseline` | candidat après extinction des consommateurs |
| `Scripts/PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1` | audit historique | `ARM_CLEAN06_4X66_Rollback.ps1` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/PrepareARM_CLEAN06_4G_BinarySnapshot.ps1` | audit historique | UE `-run=GridArmClean06Baseline` | candidat après extinction des consommateurs |
| `Scripts/PrepareARM_CLEAN06_4X64_SaveCopies.ps1` | migration | UE `-run=GridArmClean064X64SaveCopy` | candidat après extinction des consommateurs |
| `Scripts/PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1` | audit historique | `ARM_CLEAN06_4X66_Rollback.ps1` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/QualifyARM_CLEAN06_4C_Deltas.ps1` | audit historique | JSON/empreintes privés ; Git et état de transaction | candidat après extinction des consommateurs |
| `Scripts/RepairCanonicalSkillTraps.ps1` | réparation | UE `-run=RPGSkillCatalogAuthoring` | appel agent ; conserver/remplacer ensemble |
| `Scripts/RunEffectsOrgTorchStandalone.ps1` | migration | `MigrateEffectsOrgTorch.ps1` ; `ValidateUE.ps1` | candidat après extinction des consommateurs |
| `Scripts/StageARMEMENT0163Assets.ps1` | migration | PowerShell ; fichiers projet selon paramètres | candidat après extinction des consommateurs |
| `Scripts/TestARM_CLEAN06_4X66_Rollback.ps1` | tests | `ARM_CLEAN06_4X66_Rollback.ps1` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/TestARM_CLEAN06_5G85_WorldSavePreflight.ps1` | tests | `ARM_CLEAN06_4X66_Rollback.ps1` ; UE `-run=GridArmClean065RetirementAudit` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Scripts/UEValidationAgent.ps1` | infrastructure Windows | `AuthorOilSurfaceMaterial.ps1` ; `GetARM_CLEAN06_5G89_CacheRefusal.ps1` ; `RepairCanonicalSkillTraps.ps1` ; `TestARM_CLEAN06_5G85_WorldSavePreflight.ps1` ; `UEValidationSelectSDK.ps1` ; `UEValidationSilentTask.ps1` ; `ValidateUE.ps1` | outil courant ou authoring à confirmer |
| `Scripts/UEValidationBootstrap.ps1` | infrastructure Windows | `UEValidationAgent.ps1` | outil courant ou authoring à confirmer |
| `Scripts/UEValidationSelectSDK.ps1` | infrastructure Windows | PowerShell ; fichiers projet selon paramètres | appel agent ; conserver/remplacer ensemble |
| `Scripts/UEValidationSilentTask.ps1` | infrastructure Windows | `UEValidationBootstrap.ps1` | appel agent ; conserver/remplacer ensemble |
| `Scripts/UninstallUEValidationAgent.ps1` | infrastructure Windows | `UEValidationAgent.ps1` ; `UEValidationBootstrap.ps1` | outil courant ou authoring à confirmer |
| `Scripts/ValidatePackage.ps1` | packaging | UE/UBT/UAT et assets projet | outil courant ou authoring à confirmer |
| `Scripts/ValidateRPG03.ps1` | tests | `ValidateUE.ps1` | outil courant ou authoring à confirmer |
| `Scripts/ValidateUE.ps1` | compilation | `ARM_CLEAN06_4X66_Rollback.ps1` ; `AssertARM_CLEAN06_5H18_ShippingGuards.ps1` ; `AuditARM_CLEAN06_5_Retirement.ps1` ; `InspectARM_CLEAN06_5H23_HistoricalSources.ps1` ; `InstallARM_CLEAN06_5B_ReadableSaveCopy.ps1` ; `PlanARM_CLEAN06_5H12_Retirement.ps1` ; `PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1` ; `PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1` ; `TestARM_CLEAN06_4X66_Rollback.ps1` ; `TestARM_CLEAN06_5G85_WorldSavePreflight.ps1` ; `VerifyARM_CLEAN06_5H16_Canary.ps1` | appel agent ; conserver/remplacer ensemble |
| `Scripts/ValidateUIRPGVisual.ps1` | tests | `ValidateUE.ps1` | outil courant ou authoring à confirmer |
| `Scripts/VerifyARM_CLEAN06_5H16_Canary.ps1` | audit historique | `ARM_CLEAN06_4X66_Rollback.ps1` ; UE `-run=GridArmClean065RetirementAudit` | appel direct ValidateUE ; conserver/remplacer ensemble |
| `Sounds/Weapon/Renommer-SFX-Sword.ps1` | autre | PowerShell ; fichiers projet selon paramètres | local ignoré ; préserver |

Répartition des rôles principaux : **audit historique : 17**, **autre : 5**, **compilation : 1**, **génération ponctuelle : 15**, **infrastructure Windows : 6**, **migration : 7**, **packaging : 2**, **réparation : 3**, **tests : 10**.

### B. Identifiants de nomenclature — index lexical complet
Commentaires et chaînes sont exclus de cet index ; branches de préprocesseur et fixtures sont incluses. Les noms de types, fonctions/appels et propriétés/variables restent volontairement indexés ensemble : un résultat lexical n’est pas une preuve de déclaration ni de code mort. `Old/New` est segmenté en CamelCase pour éviter `Gold`/`ArcaneWand` ; `NewObject` reste listé comme usage Unreal légitime. Le manifeste suivant couvre aussi assets et répertoires ; les compteurs larges du corps incluent, eux, commentaires/chaînes.

<details>
<summary>Canonical — 258 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
AddCanonicalEntry [1] ; AddMON176CanonicalLoot [1] ; AnyCanonical [5] ; ApplyCanonicalDiscouragedAffinities [3]
ApplyCanonicalWorldVisual [5] ; CachedCanonicalSkillDefinitions [2] ; CachedCanonicalSkillDefinitionsById [2] ; CanAuthorCanonicalLevel [14]
Canonical [73] ; CanonicalActor [1] ; CanonicalArm [1] ; CanonicalArms [1]
CanonicalArrow [3] ; CanonicalBranches [4] ; CanonicalCatalystTest [1] ; CanonicalCategoryTitle [1]
CanonicalClass [1] ; CanonicalClassIds [3] ; CanonicalClasses [1] ; CanonicalCount [1]
CanonicalDefaultItemLevel [18] ; CanonicalDefinition [1] ; CanonicalDiscouragedMatrix [1] ; CanonicalDisplayName [1]
CanonicalDrop [1] ; CanonicalEffectId [6] ; CanonicalEntry [2] ; CanonicalErrors [1]
CanonicalFlightSource [1] ; CanonicalGI [1] ; CanonicalHeldMesh [1] ; CanonicalId [8]
CanonicalItem [1] ; CanonicalItemId [110] ; CanonicalKeys [1] ; CanonicalKnown [1]
CanonicalLoot [1] ; CanonicalMaxRank [1] ; CanonicalName [2] ; CanonicalObjectPath [4]
CanonicalPath [1] ; CanonicalPreviewGI [2] ; CanonicalPreviewRegistry [2] ; CanonicalProjectileForwardAxis [2]
CanonicalRegistry [10] ; CanonicalRoot [1] ; CanonicalRules [1] ; CanonicalSkill [1]
CanonicalSkillCount [1] ; CanonicalSkillIds [1] ; CanonicalSkills [3] ; CanonicalSpawn [1]
CanonicalStone [3] ; CanonicalTag [1] ; CanonicalTool [2] ; CanonicalTorch [1]
CanonicalUse [2] ; CanonicalView [1] ; CanonicalVisual [1] ; CanonicalWorldItemLevel [21]
CanonicalWorldPresentation [16] ; ConfigureCanonicalItems [1] ; ConfigureCanonicalTalentGrants [9] ; DirectCanonicalAction [1]
EGridCanonicalActionKind [14] ; EGridCanonicalArmamentKind [19] ; EGridCanonicalBoundActionError [3] ; EGridCanonicalEffectKind [7]
EGridCanonicalHandPolicy [24] ; EGridCanonicalItemResolveError [6] ; EGridCanonicalProjectileVisualMode [4] ; EGridCanonicalReactionTrigger [1]
EnsureCanonicalSkillDefinitionCache [2] ; FCanonicalFixture [3] ; FCanonicalRegistry [2] ; FCanonicalWorld [2]
FGridBECombatThrowCanonicalActions [1] ; FGridBFCanonical [1] ; FGridCanonicalAction [12] ; FGridCanonicalActionEffect [6]
FGridCanonicalActionIconReadModel [5] ; FGridCanonicalActionPresentation [7] ; FGridCanonicalArmorReserveResolver [6] ; FGridCanonicalArmorReserveView [11]
FGridCanonicalAttackPresentationReadModel [7] ; FGridCanonicalAudioVariants [3] ; FGridCanonicalClassPreference [5] ; FGridCanonicalEquipmentSlotsReadModel [7]
FGridCanonicalEquipmentSlotsReadView [4] ; FGridCanonicalEquippedAttackReadModel [5] ; FGridCanonicalExplorationItemReadModel [6] ; FGridCanonicalInventoryDefinitionDiagnosticReadModel [3]
FGridCanonicalInventoryDefinitionDiagnosticRow [3] ; FGridCanonicalInventoryItemTypeReadModel [4] ; FGridCanonicalInventoryItemTypeReadView [2] ; FGridCanonicalInventoryStackReadModel [6]
FGridCanonicalInventoryStackReadView [4] ; FGridCanonicalItemInteractionReadModel [11] ; FGridCanonicalItemInteractionReadView [6] ; FGridCanonicalItemMeshReadModel [7]
FGridCanonicalItemMeshReadView [7] ; FGridCanonicalItemSpawnReadModel [3] ; FGridCanonicalLootTestScope [27] ; FGridCanonicalNonArmamentReadPolicies [1]
FGridCanonicalNonArmamentThrowReadModel [1] ; FGridCanonicalProjectilePolicy [2] ; FGridCanonicalProjectileVisual [4] ; FGridCanonicalProjectileVisualReadModel [4]
FGridCanonicalProjectileVisualReadView [4] ; FGridCanonicalQuickItemUseReadModel [14] ; FGridCanonicalQuickItemUseReadView [9] ; FGridCanonicalReactionPolicy [2]
FGridCanonicalRegistryFixture [59] ; FGridCanonicalResolvedWeapon [10] ; FGridCanonicalRuntimeAction [5] ; FGridCanonicalRuntimePresentation [2]
FGridCanonicalThrowBallisticsReadModel [4] ; FGridCanonicalThrowBallisticsReadView [5] ; FGridCanonicalVFXVariants [3] ; FGridCanonicalWeaponArchetype [15]
FGridCanonicalWeaponVisual [13] ; FGridCanonicalWeaponVisualBinding [4] ; FGridCanonicalWeightTestWorld [31] ; FGridCanonicalWorldSnapshotReadModel [7]
FGridClean061CanonicalResolutionTest [1] ; FGridClean061CanonicalSchemaTest [1] ; FGridClean063MCanonicalWorldMesh [1] ; FGridClean064X1611CanonicalPitTransfer [1]
FGridClean064X29CanonicalWallLockKeys [1] ; FGridClean064X41CanonicalEquippedLight [2] ; FGridClean064X43CanonicalStarterAuthority [1] ; FGridClean064X6CanonicalPresentation [1]
FGridClean065D2CanonicalReceptacleInsertRestore [1] ; FGridCombatHudCanonicalActionReadModel [3] ; FGridEditorARMCLEAN064X63CanonicalAlchemyHotbar [1] ; FGridEditorARMCLEAN065CanonicalStarterKits [1]
FGridEditorCanonicalItemLevelPolicy [14] ; FGridEditorCanonicalLockRuleAuthoring [5] ; FGridEditorCanonicalPaletteReadModel [6] ; FGridEditorCanonicalPaletteReadView [6]
FGridEditorCanonicalReceptacleInitialAuthoring [3] ; FGridEditorCanonicalReceptacleRuleAuthoring [7] ; FGridEditorCanonicalStarterTestWorld [14] ; FGridEditorClean064X26CanonicalAuthoring [1]
FGridEditorClean064X27CanonicalSelectedObject [1] ; FGridItemLIGHT02CanonicalItemLightStateTest [1] ; FGridLUACOMP01CanonicalScriptTest [1] ; FGridPit03CanonicalARMPersistedFall [1]
FGridReceptacleAcceptedCanonicalReadModel [3] ; FGridReceptacleInitialCanonicalReadModel [3] ; FGridTD031CanonicalEditorActionsContractTest [1] ; FGridTD07337CanonicalDefinitionAndHotbarIndependenceTest [1]
FGridTD07344CanonicalVisualResolverTest [1] ; FGridWallAxis01CanonicalBoundaryFrameTest [1] ; FRPG036CanonicalReactionTableTest [1] ; FRPG0396AnyCanonicalReactionTest [1]
FRPGATTR01CanonicalRefreshPathTest [1] ; FRPGCanonicalSkillDefinition [5] ; FRPGMON168SingleCanonicalModelTest [1] ; FTRAP014CanonicalDexterity [1]
FUIRPG022CanonicalOrderAndLabelsTest [1] ; FUIRPG061CanonicalSkillCatalogTest [1] ; FUIRPGCONTENTTEXT025CanonicalTextRetirementParityTest [1] ; FUIRPGDESC016CanonicalRecipeDisplayNamesTest [1]
FUIRPGDESC016SixClassCanonicalSurfaceTest [1] ; FUIRPGDESC01CanonicalSimpleDetailTest [1] ; FUIRPGDESC01CanonicalVariantDetailTest [1] ; FUIRPGTalentCodeAudit025CanonicalResolverTest [1]
FUISKILLS021CanonicalSkillCacheContractTest [1] ; FUISKILLSAFFINITY013CanonicalMatrixTest [1] ; FindCanonicalItemDefinition [1] ; GetCanonicalDefinitions [4]
GetCanonicalDiscouragedClassIds [3] ; GetCanonicalItemIds [9] ; GetCanonicalItemRegistry [1] ; GetCanonicalSkillDefinitions [3]
GridCanonicalInventoryCategoryTags [6] ; GridCanonicalLootRegistryTestUtils [22] ; GridCanonicalPuzzleCategoryTags [7] ; GridCanonicalThrowTests [1]
GridCanonicalThrowVisuals [1] ; GridEditorCanonicalArmamentPreview [3] ; GridEditorCanonicalIconFixture [1] ; GridPIT02Canonical [1]
GridPIT03CanonicalARM [1] ; GridPartyInventoryCanonicalRegistry [1] ; GridTD073ApprovedCanonicalLockAcceptance [6] ; GridTD073ApprovedCanonicalStoneLoot [5]
HasCanonicalDiscouragedAffinities [2] ; HasCanonicalItem [104] ; HasCanonicalProjection [4] ; HasCanonicalTalentGrants [3]
InboundCanonical [1] ; InitializeFromCanonicalItem [14] ; IsCanonicalArchetype [1] ; IsCanonicalArchetypeId [5]
IsCanonicalDirect [2] ; IsCanonicalSkill [5] ; IsCanonicalSpellId [2] ; IsCanonicalTierLevel [1]
IsCanonicalWorldTransferItem [1] ; LogGridCanonicalItems [1] ; MakeCanonical [4] ; MakeCanonicalItem [1]
MakeCanonicalVariantNode [1] ; PreviousCanonicalId [1] ; RefreshCanonicalSections [2] ; RehydrateCanonicalOwnedItemDefinitions [6]
ResolveCanonical [10] ; ResolveCanonicalAsset [1] ; ResolveCanonicalSkillDefinition [3] ; ResolveCanonicalSkillDefinitions [5]
ResolveInventoryCanonicalRegistry [1] ; RouteCanonical [1] ; STARTUPFLOW01Canonical [1] ; TryGetCanonicalDefinition [5]
TryResolveCanonical [2] ; TryResolveCanonicalLaunchSpeed [3] ; TryResolveCanonicalOffensiveProfile [1] ; TrySelectCanonicalId [5]
UGridCanonicalItemDefinitionAsset [26] ; UGridCanonicalWeaponCatalogueAsset [6] ; UGridCanonicalWeaponVisualCatalogueAsset [4] ; ValidateCanonicalCatalog [3]
bCanConsumeCanonical [8] ; bCanonical [3] ; bCanonicalAmmo [1] ; bCanonicalAmmoFlight [1]
bCanonicalAmmunition [8] ; bCanonicalArm [1] ; bCanonicalArmamentProjectile [1] ; bCanonicalAttackSource [1]
bCanonicalEquipment [1] ; bCanonicalGenericArm [2] ; bCanonicalGenericNonArm [1] ; bCanonicalIdentity [70]
bCanonicalItem [2] ; bCanonicalNonArmFlight [1] ; bCanonicalPreviewReady [2] ; bCanonicalRegistered [2]
bCanonicalResolved [1] ; bCanonicalRoute [1] ; bCanonicalSkillDefinitionCacheInitialized [2] ; bCanonicalSource [12]
bCanonicalSpecialized [1] ; bCanonicalWorld [1] ; bExplicitCanonical [2] ; bHasCanonicalReaction [1]
bKnownCanonical [1] ; bPointerlessCanonical [1] ; bRequireCanonical [2] ; bRequireCanonicalCapture [1]
bRequireCanonicalRestore [1] ; bResolvedCanonical [2] ; bTriggerCanonicalReactionBeforeConversion [6] ; bUseCanonicalProjectileForwardAxis [2]
bValidCanonicalInbound [1] ; bValidCanonicalPhysicalLevel [1]
```

</details>

<details>
<summary>Jalons — 2168 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
ARMCLEAN063AO [1] ; ARMCLEAN063AP [1] ; ARMCLEAN063AQ [1] ; ARMCLEAN063AR [1]
ARMCLEAN063AS [1] ; ARMCLEAN063AT [1] ; ARMCLEAN063AU [1] ; ARMCLEAN063AV [1]
ARMCLEAN063AW [1] ; ARMCLEAN063AX [1] ; ARMCLEAN063AY [1] ; ARMCLEAN063AZ [1]
ARMCLEAN063BA [1] ; ARMCLEAN063BB [1] ; ARMCLEAN063BC [1] ; ARMCLEAN063BD [1]
ARMCLEAN063BE [1] ; ARMCLEAN063BF [1] ; ARMCLEAN063BG [1] ; ARMCLEAN064X10 [1]
ARMCLEAN064X12 [1] ; ARMCLEAN064X13 [1] ; ARMCLEAN064X15 [1] ; ARMCLEAN064X161 [1]
ARMCLEAN064X1610 [1] ; ARMCLEAN064X164 [1] ; ARMCLEAN064X165 [1] ; ARMCLEAN064X169 [1]
ARMCLEAN064X17 [1] ; ARMCLEAN064X18 [1] ; ARMCLEAN064X21 [1] ; ARMCLEAN064X24 [1]
ARMCLEAN064X25 [1] ; ARMCLEAN064X371 [2] ; ARMCLEAN064X45 [1] ; ARMCLEAN064X92 [1]
ARMEMENT01101 [1] ; ARMEMENT01102 [3] ; ARMEMENT011021 [2] ; ARMEMENT011021Tests [1]
ARMEMENT011023 [2] ; ARMEMENT011023Tests [1] ; ARMEMENT01102Tests [1] ; ARMEMENT01103 [1]
ARMEMENT011031 [1] ; ARMEMENT011033 [1] ; ARMEMENT01111 [1] ; ARMEMENT011121 [2]
ARMEMENT011121Tests [1] ; ARMEMENT011122 [1] ; ARMEMENT011122Tests [1] ; ARMEMENT01132 [1]
ARMEMENT01154 [1] ; ARMEMENT0115Test [1] ; ARMEMENT01161Test [1] ; ARMEMENT01162A [1]
ARMEMENT01163 [1] ; ARMEMENT01171 [1] ; ARMEMENT011713 [1] ; ARMEMENT011714 [1]
ARMEMENT01172B [1] ; ARMEMENT013 [1] ; ARMEMENT014 [1] ; ARMEMENT015 [1]
ARMEMENT0161 [1] ; ARMEMENT0162 [1] ; ARMEMENT0163 [1] ; ARMEMENT0164 [1]
ARMEMENT0165 [1] ; ARMEMENT0166Tests [1] ; ARMEMENT0171Tests [1] ; ARMEMENT0172 [1]
ARMEMENT0173 [1] ; ARMEMENT0181 [1] ; ARMEMENT0182 [1] ; ARMEMENT0183 [1]
ARMEMENT0184 [1] ; ARMEMENT0191 [1] ; ARMEMENT0192 [1] ; ARMEMENT0192SaveFix [1]
ARMEMENT0193 [1] ; ARMEMENT0193Localization [1] ; ARMEMENT0194 [1] ; ARMEMENT0195 [1]
ARMEMENT01952 [1] ; ARMEMENT0196 [1] ; ARMEMENT0197 [1] ; ARMEMENTLOT02 [1]
ARMEMENTLOT0216 [1] ; ARMEMENTLOT022 [1] ; ARMLOT02132 [1] ; AddGuaranteedMON9Loot [1]
AddMON126Contribution [1] ; AddMON176CanonicalLoot [1] ; AddMON176GuaranteedLoot [1] ; AddMON2063Rank [1]
AddMON2092Rank [1] ; AddMON2093Rank [1] ; AmmoN0 [1] ; AreMON154ChoiceDependenciesResolvable [1]
AreMON154RequirementIdsValid [1] ; ArmClean062AD [1] ; BuildMON135SerializablePartyState [1] ; BuildMON154AvailableAction [1]
BuildMON155ChoiceActionAvailability [2] ; BuildMON1891FloorLevel [1] ; BuildMON2083SingleAction [1] ; BuildMON2094Action [1]
BuildMON8LootSeed [1] ; BuildRPG033PartyArmorSnapshot [1] ; BuildTD075Runtime [1] ; Clean061 [1]
CollectMON2083MissingRequirements [1] ; CommitRPG033PartyArmorRestoration [1] ; ConfigureMON10AudioFloor [1] ; ConfigureMON10IdleFloor [1]
ConfigureMON10VFXFloor [1] ; ConfigureMON1891PersistentParty [1] ; ConfigureMON8Floor [1] ; CorruptTD073310DerivedStats [1]
CountMON10AttackEntries [1] ; CountMON10VFXAttackEntries [1] ; CountMON132WorldMonsters [1] ; CountMON135EncounterActors [1]
CreateMON128Inventory [1] ; CreateMON128SerializableInventory [1] ; CreateMON187Inventory [1] ; DoesMON12ItemDeclareAttack [1]
EGridTD0731FindingKind [1] ; EffectsOrg011 [1] ; EvaluateMON126Availability [1] ; FARMCLEAN063BBInventory [1]
FARMCLEAN063BBProjectile [1] ; FARMCLEAN063BBProjection [1] ; FARMCLEAN063BCDrop [1] ; FARMCLEAN063BCEmbeddedSave [1]
FARMCLEAN063BCInFlightSave [1] ; FARMCLEAN063BDCapture [1] ; FARMCLEAN063BDLegacy [1] ; FARMCLEAN063BDPickup [1]
FARMCLEAN063BDPresentation [1] ; FARMCLEAN063BDRegisteredGuards [1] ; FARMEMENT01101ReadinessAudit [1] ; FARMEMENT011021VisualCatalogueIntegrity [1]
FARMEMENT011023BombPlacementTest [1] ; FARMEMENT011023IdempotenceTest [1] ; FARMEMENT01102WarriorGuard [1] ; FARMEMENT01102WarriorSource [1]
FARMEMENT011031Persistence [1] ; FARMEMENT011031RealShieldDamage [1] ; FARMEMENT011031Restore [1] ; FARMEMENT01103CombatProfiles [1]
FARMEMENT01103GreatAxeTwoHands [1] ; FARMEMENT01103ProductionBindings [1] ; FARMEMENT01103RealInventory [1] ; FARMEMENT01103TransientPlacement [1]
FARMEMENT01111ApprovedSources [1] ; FARMEMENT01111Readiness [1] ; FARMEMENT011121VisualCatalogueIntegrity [1] ; FARMEMENT011122Definition [1]
FARMEMENT011122SourceContract [1] ; FARMEMENT01132Creation [1] ; FARMEMENT01132Legacy [1] ; FARMEMENT01132Projection [1]
FARMEMENT01141RangerStartingAmmunition [1] ; FARMEMENT01151ContextMenu [1] ; FARMEMENT01151SelfRestoration [1] ; FARMEMENT01152ExplorationHotbar [1]
FARMEMENT01154InventoryItemAudit [1] ; FARMEMENT0115AuthoringContract [1] ; FARMEMENT0115ProductionKits [1] ; FARMEMENT01161DisarmToolAuthoring [1]
FARMEMENT01161SaboteurApprovedGrant [1] ; FARMEMENT01162AOptIn [1] ; FARMEMENT01162ARuntime [1] ; FARMEMENT01162BDisarmContext [1]
FARMEMENT01163NewGameDisarmSave [1] ; FARMEMENT01163SaboteurKitOnly [1] ; FARMEMENT011713OilCombatAvailability [1] ; FARMEMENT011713OilTooltip [1]
FARMEMENT011714OilQuickItemRealPipeline [1] ; FARMEMENT011717OilExplorationRealPipeline [1] ; FARMEMENT01171OilContract [1] ; FARMEMENT01171StarterAndSave [1]
FARMEMENT01172BCombatCatalyst [2] ; FARMEMENT01172BStarterSave [1] ; FARMEMENT01172CProductionVisuals [1] ; FARMEMENT01172CatalystContract [1]
FARMEMENT01172CatalystProductionAsset [1] ; FARMEMENT01173AMixedSurfaceSnapshot [1] ; FARMEMENT01173ASquareArea [1] ; FARMEMENT01173BExplorationCatalyst [1]
FARMEMENT01173DNativeActionRuntime [1] ; FARMEMENT01173DSerializedCatalyst [1] ; FARMEMENT01174CPlacedNiagara [1] ; FARMEMENT015CatalogueBindingTest [1]
FARMEMENT015CombatTest [1] ; FARMEMENT015InstanceLevelTest [1] ; FARMEMENT015InventoryEquipmentTest [1] ; FARMEMENT015LegacyTest [1]
FARMEMENT015PassiveAndFailureTest [1] ; FARMEMENT0161DuplicateTest [1] ; FARMEMENT0161ItemOptInTest [1] ; FARMEMENT0161NoGameplayAuthorityTest [1]
FARMEMENT0161SchemaTest [1] ; FARMEMENT0161VisualProfileTest [1] ; FARMEMENT0162LegacyFallbackTest [1] ; FARMEMENT0162LookupFailuresTest [1]
FARMEMENT0162MalformedTest [1] ; FARMEMENT0162NoMutationTest [1] ; FARMEMENT0162PartialResourceTest [1] ; FARMEMENT0162SharedVisualTest [1]
FARMEMENT0163MaterialDepsTest [1] ; FARMEMENT0163MaterializedAssetTest [1] ; FARMEMENT0163PassiveTest [1] ; FARMEMENT0163ResolverTest [1]
FARMEMENT0164HotbarIconTest [1] ; FARMEMENT0164InventoryIconTest [1] ; FARMEMENT0165EditorPreviewTest [1] ; FARMEMENT0165RuntimeMeshTest [1]
FARMEMENT0166AuthoredDefinitionsTest [1] ; FARMEMENT0166EquipmentAttackTest [1] ; FARMEMENT0171PlacementPreviewTest [1] ; FARMEMENT0171ProductionPaletteTest [1]
FARMEMENT0172CapacityTest [1] ; FARMEMENT0172CursorTest [1] ; FARMEMENT0172InventoryTest [1] ; FARMEMENT0172OneHandReplacementTest [1]
FARMEMENT0172ProductionPilotTest [1] ; FARMEMENT0172WorldTransferTest [1] ; FARMEMENT0173OneHandContextActionTest [1] ; FARMEMENT0173TwoHandedContextActionTest [1]
FARMEMENT0181DropTest [1] ; FARMEMENT0181EditorTest [1] ; FARMEMENT0181PickupTest [1] ; FARMEMENT0182AuthoringTest [1]
FARMEMENT0182MaterializationTest [1] ; FARMEMENT0182PersistenceTest [1] ; FARMEMENT0183FullSaveTest [1] ; FARMEMENT0183InitialTest [1]
FARMEMENT0183TransferTest [1] ; FARMEMENT0184DeathTest [1] ; FARMEMENT0184LevelContractTest [1] ; FARMEMENT0184SaveTest [1]
FARMEMENT0191LegacyCreationTest [1] ; FARMEMENT0191OneHandKitTest [1] ; FARMEMENT0191RollbackTest [1] ; FARMEMENT0191TwoHandsKitTest [1]
FARMEMENT0192CatalogTest [1] ; FARMEMENT0192LegacyTest [1] ; FARMEMENT0192MissingDungeonTest [1] ; FARMEMENT0192RangerTest [1]
FARMEMENT0192SaveRestoreTest [1] ; FARMEMENT0192SixClassCreationTest [1] ; FARMEMENT0193CompatibilityTest [1] ; FARMEMENT0193InvalidSelectionTest [1]
FARMEMENT0193KitCsvCoverageTest [1] ; FARMEMENT0193KitPresentationTest [1] ; FARMEMENT0193KitProjectionTest [1] ; FARMEMENT0193RequestTest [1]
FARMEMENT0193WizardClassSwitchTest [1] ; FARMEMENT0193WizardHunterDescriptionTest [1] ; FARMEMENT0193WizardReadModelTest [1] ; FARMEMENT0193WizardWorldBindingTest [1]
FARMEMENT0194AuthoritativeCoverage [1] ; FARMEMENT0194NoMutation [1] ; FARMEMENT0194SemanticGaps [1] ; FARMEMENT0195ActualTalentChecks [1]
FARMEMENT0195AdvisoryOnly [1] ; FARMEMENT0195AllKitsCovered [1] ; FARMEMENT0196Catalogue [1] ; FARMEMENT0196Compatibility [1]
FARMEMENT0196Coverage [1] ; FARMEMENT0197IndependentFragments [1] ; FARMEMENT0197NoGameplayMutation [1] ; FARMEMENT0197QualifiedPriest [1]
FARMEMENTCLEAN02LevelArmorAuthorityTest [1] ; FARMEMENTLOT021322Arrows [1] ; FARMEMENTLOT0213ProductionMatrix [1] ; FARMEMENTLOT02161MeleeN1Execution [1]
FARMEMENTLOT02162BowN1Execution [1] ; FARMEMENTLOT02163RealThrowingN1 [1] ; FARMEMENTLOT02164MagicN1Execution [1] ; FARMEMENTLOT02165PassiveN1Execution [1]
FARMEMENTLOT0216DiskPersistence [1] ; FARMEMENTLOT02191HandProjection [1] ; FARMEMENTLOT02191ShadowCombat [1] ; FARMEMENTLOT021921DualAttackIconViewTest [1]
FARMEMENTLOT02192DualWield [1] ; FARMEMENTLOT0220EquippedIconCategories [1] ; FARMEMENTLOT022NewGame [1] ; FARMEMENTLOT025MarksmanStarter [1]
FARMEMENTLOT026Throwing [1] ; FARMEMENTLOT027MagicAndSpear [1] ; FARMEMENTLOT028Focus [1] ; FARMEMENTLOT02Readiness [1]
FARMLOT02136AEmbeddedHit [1] ; FARMLOT02136BMissT [1] ; FARMLot02132Ammo [1] ; FARMLot02132RealWorld [1]
FARMLot02134RangedArrow [1] ; FCheckMON135ContinuePIE [1] ; FCheckMON135FreshPIE [1] ; FCleanupMON135RealPIE [1]
FEditorUITooltipClean01ReferenceAudit [1] ; FGridARMCLEAN03LosslessCatalogueTest [1] ; FGridARMCLEAN063AWInteractionIdentityTest [1] ; FGridARMCLEAN065ExplorationItemReaders [1]
FGridARMEMENT01103310CriticalPipeline [1] ; FGridARMEMENT01103310DamageMultiDiePipeline [1] ; FGridARMEMENT01103310DirectAndMiss [1] ; FGridARMEMENT0110337ExceptionalOutcomes [1]
FGridARMEMENT0110337LabeledComponents [1] ; FGridARMEMENT0110338DesignerSeparatedLines [1] ; FGridARMEMENT0110339NoLegacyBindings [1] ; FGridARMEMENT011033AttackBreakdown [1]
FGridARMEMENT011033DepletionSnapshot [1] ; FGridARMEMENT011033ImpactSnapshot [1] ; FGridARMEMENT011033MultipleSources [1] ; FGridARMEMENT013BoundsTest [1]
FGridARMEMENT013DeterministicTest [1] ; FGridARMEMENT013DiceValidationTest [1] ; FGridARMEMENT013ExtremesTest [1] ; FGridARMEMENT013PassiveTest [1]
FGridARMEMENT013PipelineTest [1] ; FGridARMEMENT013RngTest [1] ; FGridARMEMENT014CatalogueTest [1] ; FGridARMEMENT014CombatCompatibilityTest [1]
FGridARMEMENT014InvalidCatalogueTest [1] ; FGridARMEMENT014PassiveTest [1] ; FGridARMEMENT014ProfileTest [1] ; FGridARMEMENT01952Examples [1]
FGridARMEMENT01952Preferences [1] ; FGridARMEMENT01952ProjectionCoverage [1] ; FGridArmClean064X2AuthoredSourceEvidence [1] ; FGridArmClean064X3AuthoredLevelMaterializer [3]
FGridArmClean064X3AuthoredLevelRules [1] ; FGridClean061AmmoAfterRegisterTest [1] ; FGridClean061AmmoIdentityTest [1] ; FGridClean061AmmoNonemptyTest [1]
FGridClean061AmmoOutTypeTest [1] ; FGridClean061AmmoResolutionTest [1] ; FGridClean061AmmoSourceTest [1] ; FGridClean061AmmoTextTest [1]
FGridClean061AmmoTypeTest [1] ; FGridClean061CanonicalResolutionTest [1] ; FGridClean061CanonicalSchemaTest [1] ; FGridClean061ConsumableResolutionTest [1]
FGridClean061PassiveResolutionTest [1] ; FGridClean061ReferentialIntegrityTest [1] ; FGridClean061RejectionTest [1] ; FGridClean061SwordResolutionTest [1]
FGridClean062ABBoundHandAttackTest [1] ; FGridClean062ACProjectileReadModelTest [1] ; FGridClean062ADArmorDebtRoundTrip [1] ; FGridClean062ADOptionalDimensionRoundTrip [1]
FGridClean062ADOwnershipRoundTrip [1] ; FGridClean062ADRehydrationRoundTrip [1] ; FGridClean062ADRejectLossySave [1] ; FGridClean062ADSaveInstanceParity [1]
FGridClean062AENeverRefillShield [1] ; FGridClean062AEProductionArmorReserves [1] ; FGridClean062AFInstanceArmorReserve [1] ; FGridClean062AGEquippedInstanceAttack [1]
FGridClean062ClassParityTest [1] ; FGridClean062E2WeaponPackageRegistryTest [1] ; FGridClean062GProductionActionsPresentationsTest [1] ; FGridClean062GProjectionSafetyTest [1]
FGridClean062HGeneratedActionProjectionTest [1] ; FGridClean062HRejectModifiedActionTest [1] ; FGridClean062HandActionSeparationTest [1] ; FGridClean062IBoundActionResolutionTest [1]
FGridClean062JVisualProjectionTest [1] ; FGridClean062KIconSafety [1] ; FGridClean062KRealIconPaletteProjection [1] ; FGridClean062LIdentitySafety [1]
FGridClean062LProductionItems [1] ; FGridClean062MClassAffinityProjection [1] ; FGridClean062NLiveVisualReferencesTest [1] ; FGridClean062PProductionCandidate [1]
FGridClean062PresentationInvalidTest [1] ; FGridClean062PresentationVariantsTest [1] ; FGridClean062ProductionARMHandsTest [1] ; FGridClean062QProductionActions [1]
FGridClean062RArmSemanticGateTest [1] ; FGridClean062SProductionNumbers [1] ; FGridClean062TAttackReadinessGate [1] ; FGridClean062UFullProductionCandidate [1]
FGridClean062VRealAmmo [1] ; FGridClean062WRealCombatThrows [1] ; FGridClean062WRejectInvalidThrow [1] ; FGridClean062XProjectileVisualSafety [1]
FGridClean062XRealProjectileVisuals [1] ; FGridClean062YCompleteProductionPreview [1] ; FGridClean063AAWeaponTooltip [1] ; FGridClean063ABGuardIdentity [1]
FGridClean063ACInventoryDiagnosticWeight [1] ; FGridClean063ADEquipmentContributions [1] ; FGridClean063AEEquipmentPreviewReadModel [1] ; FGridClean063AFStartingKitDisplay [1]
FGridClean063AGMaterialization [1] ; FGridClean063AHLootLevelResolution [1] ; FGridClean063AIReceptacleMaterialization [1] ; FGridClean063AJWorldItemLevels [1]
FGridClean063ALActionIcons [1] ; FGridClean063AMStartingKitLevels [1] ; FGridClean063ANAuthoring [1] ; FGridClean063AOStartingKitHands [1]
FGridClean063APDualWield [1] ; FGridClean063AQEquipmentSlots [1] ; FGridClean063ARHudActionIcons [1] ; FGridClean063ARealRegistry [1]
FGridClean063ASNoLegacyInventory [1] ; FGridClean063ATSavedRehydration [1] ; FGridClean063ATStrictPartyRehydration [1] ; FGridClean063AUInventoryItemTypes [1]
FGridClean063AVCatalogues [1] ; FGridClean063AVInvalidAndLegacy [1] ; FGridClean063AVLanding [1] ; FGridClean063AVProjectile [1]
FGridClean063AXContext [1] ; FGridClean063AXExploration [1] ; FGridClean063AXHotbar [1] ; FGridClean063AXInventory [1]
FGridClean063AXProjection [1] ; FGridClean063AXRegistration [1] ; FGridClean063AXRejections [1] ; FGridClean063AYStatusCombat [1]
FGridClean063AYStatusProjection [1] ; FGridClean063AZStatusCombat [1] ; FGridClean063AZStatusProjection [1] ; FGridClean063BAAllyApply [1]
FGridClean063BAAllyHeal [1] ; FGridClean063BAAllyRemove [1] ; FGridClean063HArmorRuntime [1] ; FGridClean063IWeightRuntime [1]
FGridClean063JInventoryIconTest [1] ; FGridClean063KInventoryMetadata [1] ; FGridClean063LHeldVisual [1] ; FGridClean063MCanonicalWorldMesh [1]
FGridClean063NWorldWeight [1] ; FGridClean063OSortProjection [1] ; FGridClean063PFreshReceptacleMesh [1] ; FGridClean063QWorldMass [1]
FGridClean063RProjectileVisual [1] ; FGridClean063TThrowBallistics [1] ; FGridClean063UPhysicalThrowStrength [1] ; FGridClean063VUnlevelledAmmo [1]
FGridClean063WAmmoFlightMesh [1] ; FGridClean063XLootMetadata [1] ; FGridClean063YStartingKitStaticMetadata [1] ; FGridClean063ZEditorLooseItemPreview [1]
FGridClean064UQuickActionMigration [1] ; FGridClean064VEquippedNonArm [1] ; FGridClean064WRealReferences [1] ; FGridClean064X10AInventory [1]
FGridClean064X10BIdentity [1] ; FGridClean064X10C1AmmoN1 [1] ; FGridClean064X10C2WeaponN0 [1] ; FGridClean064X10C3WeaponN20 [1]
FGridClean064X10C4UnknownId [1] ; FGridClean064X10C5ZeroQuantity [1] ; FGridClean064X10C6ContradictorySource [1] ; FGridClean064X10DWorld [1]
FGridClean064X10ELegacy [1] ; FGridClean064X1610ReceptacleRestore [1] ; FGridClean064X1610WorldRestore [1] ; FGridClean064X1611CanonicalPitTransfer [1]
FGridClean064X161Rules [1] ; FGridClean064X162Authoring [1] ; FGridClean064X163InitialAuthoring [1] ; FGridClean064X164AtomicPreview [1]
FGridClean064X164RealSourcesPreview [1] ; FGridClean064X165KeyAtomicPreview [1] ; FGridClean064X165RealKeyPreview [1] ; FGridClean064X1661RealAcceptanceManifest [1]
FGridClean064X169SaveSchemaGate [1] ; FGridClean064X169SaveWorldRoundTrip [1] ; FGridClean064X17TorchHandRegression [1] ; FGridClean064X18TorchFloorLightState [1]
FGridClean064X20ArmWorldPresentation [1] ; FGridClean064X21PointerlessInitialArm [1] ; FGridClean064X22PointerlessArmRestore [1] ; FGridClean064X23PointerlessHeldTorch [1]
FGridClean064X24PointerlessKits [1] ; FGridClean064X25PointerlessEditorPreview [1] ; FGridClean064X29CanonicalWallLockKeys [1] ; FGridClean064X30PointerlessLockVisual [1]
FGridClean064X31ReceptacleTypeConditions [1] ; FGridClean064X32ReceptacleAcceptance [1] ; FGridClean064X41CanonicalEquippedLight [2] ; FGridClean064X42HeldPhysicalArm [1]
FGridClean064X43CanonicalStarterAuthority [1] ; FGridClean064X4RealAuthoredLevels [1] ; FGridClean064X60PointerlessLootDeath [1] ; FGridClean064X61LootPresentationCategory [1]
FGridClean064X63PotionPresentationOnly [1] ; FGridClean064X6CanonicalPresentation [1] ; FGridClean064X6ProbeExtra [1] ; FGridClean064X6ProbeFamilies [1]
FGridClean064X6ProbeLight [1] ; FGridClean064X6ProbeNiagara [1] ; FGridClean064X6ProbeResolve [1] ; FGridClean064X71RealReceptacleAudit [1]
FGridClean064X7ActorProjection [1] ; FGridClean064X7World [1] ; FGridClean064X81RealLockAudit [1] ; FGridClean064X841TD073LockSchemaWhitelist [1]
FGridClean064X8LooseCutover [2] ; FGridClean064X8World [1] ; FGridClean064X91RealReceptacleSources [1] ; FGridClean064X92RealInitialPresentation [1]
FGridClean065D2CanonicalReceptacleInsertRestore [1] ; FGridClean065D3GenericArmFlight [1] ; FGridClean065D4NonArmContributions [1] ; FGridClean065DGenericThrow [1]
FGridClean065E1PointerlessTooltip [1] ; FGridClean065E4ContextEquip [1] ; FGridClean065E4DLegacyLookup [1] ; FGridClean065G48ReceptacleService [1]
FGridClean065G52ReceptacleCapture [1] ; FGridClean065G59InFlightTravel [1] ; FGridClean065G60ImpactRetry [2] ; FGridClean065H10RegistrationGate [1]
FGridClean065H11LegacyActorGate [1] ; FGridClean065H3ApprovedStarterRegistrylessGate [1] ; FGridClean065H6EditorStarterParity [1] ; FGridClean5G34WorldPreflight [1]
FGridEditorARMCLEAN064X371BackupReceipt [1] ; FGridEditorARMCLEAN064X37WorldPresentationProjection [1] ; FGridEditorARMCLEAN064X45NativeLoosePreflight [1] ; FGridEditorARMCLEAN064X48PostNativeRefs [1]
FGridEditorARMCLEAN064X50LockRuleAuthoring [1] ; FGridEditorARMCLEAN064X51NativeSharedRules [1] ; FGridEditorARMCLEAN064X55NativePalette [1] ; FGridEditorARMCLEAN064X58NativeInitialPlan [1]
FGridEditorARMCLEAN064X59NativeDormantPlan [1] ; FGridEditorARMCLEAN064X60StonePlan [1] ; FGridEditorARMCLEAN064X61LootProjection [1] ; FGridEditorARMCLEAN064X62AlchemyEvidence [1]
FGridEditorARMCLEAN064X63AlchemyJointProjection [1] ; FGridEditorARMCLEAN064X63CanonicalAlchemyHotbar [1] ; FGridEditorARMCLEAN064X63SavedElixirIdentity [1] ; FGridEditorARMCLEAN064X64SaveCopy [1]
FGridEditorARMCLEAN064X65AlchemyStarterAuthoring [1] ; FGridEditorARMCLEAN064X66ColdNativeSave [1] ; FGridEditorARMCLEAN065CanonicalStarterKits [1] ; FGridEditorARMCLEAN065D3NativeProjection [1]
FGridEditorARMCLEAN065D4ArmExtraStats [1] ; FGridEditorARMCLEAN065G85WorldPreflight [1] ; FGridEditorARMCLEAN065G88CacheEvidence [1] ; FGridEditorARMCLEAN065NativeAudit [1]
FGridEditorARMCLEAN065ReadableSaveRepair [1] ; FGridEditorARMCLEAN065SaveAudit [1] ; FGridEditorClean064X11RealPalette [1] ; FGridEditorClean064X12PalettePlacement [1]
FGridEditorClean064X13Descriptions [1] ; FGridEditorClean064X13Icons [1] ; FGridEditorClean064X13Labels [1] ; FGridEditorClean064X14SlateReadModel [1]
FGridEditorClean064X15RealAcceptanceInventory [1] ; FGridEditorClean064X26CanonicalAuthoring [1] ; FGridEditorClean064X27CanonicalSelectedObject [1] ; FGridEditorClean064X28PointerlessPalette [1]
FGridEditorCppClean03BlueprintLegacyReferenceAuditTest [1] ; FGridEditorCppClean04AssetConfigurationTest [1] ; FGridEditorEffectsOrg011InventoryTest [1] ; FGridEditorEffectsOrg012ADependencyContractTest [1]
FGridEditorEffectsOrg012DPowerShellSyntaxTest [1] ; FGridEditorEffectsOrg012DTorchPreflightTest [1] ; FGridEditorMON133FreshPIEPreparationTest [1] ; FGridEditorMON133MonsterSpawnLinkPolicyTest [1]
FGridEditorMON134EncounterLinkPolicyTest [1] ; FGridEditorMON135RealPIEIntegrationTest [1] ; FGridEditorMON1923LogicPolicyTest [1] ; FGridEditorMON1923LogicRuntimeSupportTest [1]
FGridEditorMON192ConditionConfigurationTest [1] ; FGridEditorMON192ConditionalLinkIdentityTest [1] ; FGridEditorMON192ExactLinkMutationsTest [1] ; FGridEditorMON192LinkPolicyMatrixTest [1]
FGridEditorMON2044RecruitmentLinkPolicyTest [1] ; FGridEditorMON2045StoryCompanionPalettePlacementTest [1] ; FGridEditorMON2055CustomRecruitLinkPolicyTest [1] ; FGridEditorUIAssetClean01ReferenceAuditTest [1]
FGridEditorUITooltipCppClean023BlueprintCompatibility [1] ; FGridEditorWorldObjectMIG05DirectItemPlacementTest [1] ; FGridEditorWorldObjectMIG06SparsePlacementTest [1] ; FGridEditorWorldObjectMIG07TypedWriteThroughTest [1]
FGridEditorWorldObjectMIG09ItemLegacyBlueprintReferenceAuditTest [1] ; FGridEditorWorldObjectMIG09MechanismLegacyBlueprintReferenceAuditTest [1] ; FGridItemThrowCLEAN01Test [1] ; FGridItemVFX011ConfigurationTest [1]
FGridItemVFX011RuntimeTest [1] ; FGridItemVFX012ProductionPreflightTest [1] ; FGridItemVFX012ProductionVfxContractTest [1] ; FGridItemVFX012StackLifecycleTest [1]
FGridItemVFX012VisualOnlyAuthoringTest [1] ; FGridItemVFX013MigratedProductionTest [1] ; FGridItemVFX013ProductionPreflightTest [1] ; FGridItemVFX013TorchAuthoringTest [1]
FGridItemVFX013TorchRuntimeTest [1] ; FGridItemVFX014BRuntimeTest [1] ; FGridItemVFX014BSchemaTest [1] ; FGridItemVFX014FlaskVisualPolicyTest [1]
FGridItemVFX014ProductionPolicyAuditTest [1] ; FGridMIG09EditorTypedMutationsTest [1] ; FGridMIG09EditorTypedValidationTest [1] ; FGridMON10AudioTestWorld [1]
FGridMON10IdleTestWorld [1] ; FGridMON10OptimizationTestWorld [1] ; FGridMON10TestWorld [1] ; FGridMON10VFXTestWorld [1]
FGridMON1141TestWorld [1] ; FGridMON1141ThrownWeaponLifecycleTest [1] ; FGridMON1142PlacedItemRebuildUniquenessTest [1] ; FGridMON114AudioVFXMappingTest [1]
FGridMON114EventOrderTest [1] ; FGridMON114FeedbackAndRejectionsTest [1] ; FGridMON114HeldItemMotionTest [1] ; FGridMON114ProfileValidationTest [1]
FGridMON114SequencedStrikePresentationTest [1] ; FGridMON114TargetReactionExclusivityTest [1] ; FGridMON11EquipmentFixture [1] ; FGridMON11EquipmentWorld [1]
FGridMON11Fixture [1] ; FGridMON11ResolutionFixture [1] ; FGridMON11ResolutionWorld [1] ; FGridMON11TestWorld [1]
FGridMON1288ClearShortcutKeepsItemSourceTest [1] ; FGridMON1289BindingSanitizationTest [1] ; FGridMON1289QuickItemBindingLifetimeTest [1] ; FGridMON1289UniqueEquipmentBindingTest [1]
FGridMON128DefaultHotbarTest [1] ; FGridMON128HotbarSaveRoundTripTest [1] ; FGridMON128IdentityCacheGuard [1] ; FGridMON128MoveOrSwapHotbarTest [1]
FGridMON128PerCharacterHotbarTest [1] ; FGridMON128QuickItemDropTest [1] ; FGridMON128RejectInvalidHotbarTest [1] ; FGridMON128RejectLegacySaveWithoutHotbarTest [1]
FGridMON128StableEquipmentResolutionTest [1] ; FGridMON12Fixture [1] ; FGridMON12TestWorld [1] ; FGridMON132TestWorld [1]
FGridMON141Fixture [1] ; FGridMON141TestWorld [1] ; FGridMON142TestWorld [1] ; FGridMON1431EditorFixture [1]
FGridMON1431EditorTestWorld [1] ; FGridMON1431PatrolRouteEditingModelTest [1] ; FGridMON1431PatrolRouteGuardTest [1] ; FGridMON143Fixture [1]
FGridMON143TestWorld [1] ; FGridMON144Fixture [1] ; FGridMON144TestWorld [1] ; FGridMON161TestWorld [1]
FGridMON162TestWorld [1] ; FGridMON163TestWorld [1] ; FGridMON164Fixture [1] ; FGridMON164TestWorld [1]
FGridMON165PartyFixture [1] ; FGridMON165TestWorld [1] ; FGridMON166Fixture [1] ; FGridMON166TestWorld [1]
FGridMON1751Fixture [1] ; FGridMON1751TestWorld [1] ; FGridMON176TestWorld [1] ; FGridMON182CharacterIsolationTest [1]
FGridMON182CharacterRegistrationTest [1] ; FGridMON182DurableContractTest [1] ; FGridMON182LearnForgetTest [1] ; FGridMON182StableIdentityTest [1]
FGridMON183IdentityMismatchNoMutationTest [1] ; FGridMON183InsufficientActionPointsNoMutationTest [1] ; FGridMON183InsufficientManaNoMutationTest [1] ; FGridMON183SuccessfulCommitTest [1]
FGridMON183TargetingDeferredTest [1] ; FGridMON183UnknownSpellNoMutationTest [1] ; FGridMON184AllyRelationTest [1] ; FGridMON184AxialTargetSuccessTest [1]
FGridMON184CellAreaResolutionTest [1] ; FGridMON184LineOfSightNoMutationTest [1] ; FGridMON184NonAxialNoMutationTest [1] ; FGridMON184OutOfRangeNoMutationTest [1]
FGridMON184SelfResolutionTest [1] ; FGridMON184TransactionFailureAfterTargetTest [1] ; FGridMON185ApplyStatusBridgeTest [1] ; FGridMON185AtomicFailureTest [1]
FGridMON185DamageResolutionTest [1] ; FGridMON185HealingClampTest [1] ; FGridMON185ProductionDefinitionsTest [1] ; FGridMON185RemoveStatusTest [1]
FGridMON186InstantPlanSequenceTest [1] ; FGridMON186PresentationPurityTest [1] ; FGridMON186ProductionProfilesTest [1] ; FGridMON186ProjectilePlanSequenceTest [1]
FGridMON186ProjectileTimingReuseTest [1] ; FGridMON186ProjectileTrajectoryReuseTest [1] ; FGridMON186VisualOptionalityTest [1] ; FGridMON187AssignKnownSpellTest [1]
FGridMON187BindingContractTest [1] ; FGridMON187DragDropPayloadTest [1] ; FGridMON187DuplicateMoveSwapTest [1] ; FGridMON187MissingDefinitionTest [1]
FGridMON187ProductionEntriesTest [1] ; FGridMON187RejectUnknownNoMutationTest [1] ; FGridMON1891CombatSavePreservesMainSlotTest [1] ; FGridMON1891DefeatPreservesCheckpointTest [1]
FGridMON1891PreCombatCheckpointCreatedTest [1] ; FGridMON1891SaveDuringCombatRejectedTest [1] ; FGridMON1891SaveOutsideCombatAcceptedTest [1] ; FGridMON1891TestSaveSlots [1]
FGridMON1891TestWorld [1] ; FGridMON1891TransientCheckpointSkippedTest [1] ; FGridMON1892BalanceContractTest [1] ; FGridMON1892CleanCureNoCostTest [1]
FGridMON1892CurePoisonCommitTest [1] ; FGridMON1892FullHealthHealNoCostTest [1] ; FGridMON1892HasteCommitTest [1] ; FGridMON1893CheckpointIsolationTest [1]
FGridMON1893SaveSlotDiagnosticsTest [1] ; FGridMON1923ComparisonPrimitivesTest [1] ; FGridMON1923EventCommandChainTest [1] ; FGridMON1923LatchRelayTest [1]
FGridMON1923MutationPrimitivesTest [1] ; FGridMON1923ValidationOverflowTest [1] ; FGridMON192LevelVariableDefaultsTest [1] ; FGridMON192LevelVariableReconcileTest [1]
FGridMON192LevelVariableSaveRoundTripTest [1] ; FGridMON192LevelVariableSnapshotValidationTest [1] ; FGridMON1931LuaAtomicReloadTest [1] ; FGridMON1931LuaInstructionBudgetTest [1]
FGridMON1931LuaMemoryQuotaTest [1] ; FGridMON1931LuaMultipleScriptsTest [1] ; FGridMON1931LuaSandboxSurfaceTest [1] ; FGridMON1931LuaVersionLifecycleTest [1]
FGridMON194EventContextAndVarsTest [1] ; FGridMON194HostFailureTest [1] ; FGridMON194LuaCommandToLogicTest [1] ; FGridMON194LuaLinkIdentityTest [1]
FGridMON194SharedActionBudgetTest [1] ; FGridMON195CurrentSourceWinsTest [1] ; FGridMON195InvalidCurrentSourceDropsVmTest [1] ; FGridMON195SaveRestoreFreshVmTest [1]
FGridMON195SaveVersionContractTest [1] ; FGridMON195TestWorld [1] ; FGridMON196LuaBindingMutationTest [1] ; FGridMON196LuaReferenceCountRegressionTest [1]
FGridMON196LuaScriptAnalysisTest [1] ; FGridMON196ValidationAlignmentTest [1] ; FGridMON196World [1] ; FGridMON1971GridVarsCompatibilityTest [1]
FGridMON1971LogicIdCommandTest [1] ; FGridMON1971LogicIdEditorTest [1] ; FGridMON1971PersistentEditorSyncTest [1] ; FGridMON1971PersistentTableTest [1]
FGridMON1971PersistentValidationTest [1] ; FGridMON197LuaEmbeddedSourceTest [1] ; FGridMON197LuaForbiddenSurfaceTest [1] ; FGridMON197LuaSourceHardLimitsTest [1]
FGridMON197LuaTextOnlyTest [1] ; FGridMON198PuzzleADirectLeverDoorTest [1] ; FGridMON198PuzzleBLogicCounterThresholdTest [1] ; FGridMON198PuzzleCLuaConditionalTest [1]
FGridMON198PuzzleDEncounterLuaDoorTest [1] ; FGridMON202DuplicateIdentityRejectTest [1] ; FGridMON202FullPartyAtomicRejectTest [1] ; FGridMON202InventoryOwnershipNormalizationTest [1]
FGridMON202MissingCandidateAtomicRejectTest [1] ; FGridMON202OwnershipCollisionRollbackTest [1] ; FGridMON202ValidPoolRecruitmentTest [1] ; FGridMON203AlreadyActiveRecognitionTest [1]
FGridMON203DefinitionValidationTest [1] ; FGridMON203IdempotentRegistrationTest [1] ; FGridMON203IdentityCollisionRejectTest [1] ; FGridMON203InvalidDefinitionAtomicRejectTest [1]
FGridMON203RegisterCandidateTest [1] ; FGridMON2044RecruitmentEventCommandContractTest [1] ; FGridMON2044RecruitmentMissingDefinitionTest [1] ; FGridMON2045StoryCompanionPaletteContractTest [1]
FGridMON2045StoryCompanionPaletteMissingDefinitionTest [1] ; FGridMON2046RecruitmentAlreadyActiveSuppressionTest [2] ; FGridMON2046RecruitmentOfferDeclineSourceScopeTest [2] ; FGridMON204AlreadyActiveNoDoubleRecruitmentTest [1]
FGridMON204AlreadyInPoolTest [1] ; FGridMON204DeclineNoMutationTest [1] ; FGridMON204IdentityCollisionAtomicTest [1] ; FGridMON204InvalidDefinitionTest [1]
FGridMON204NominalRecruitmentTest [1] ; FGridMON204PartyFullAtomicTest [1] ; FGridMON204RecruitmentRuntimeContractTest [1] ; FGridMON204RecruitmentRuntimeDefaultStateTest [1]
FGridMON204RecruitmentViewProjectionTest [1] ; FGridMON8TestWorld [1] ; FGridMON9TestSaveSlot [1] ; FGridMON9TestWorld [1]
FGridMagicMON181CastRequestValidationTest [1] ; FGridMagicMON181DefinitionValidationTest [1] ; FGridMagicMON181NoRuntimePaymentTest [1] ; FGridMagicMON181StatusEffectBridgeTest [1]
FGridMagicMON188DeterministicMutationTest [1] ; FGridMagicMON188DiskRoundTripTest [1] ; FGridMagicMON188DuplicateCharacterRejectedTest [1] ; FGridMagicMON188DuplicateSpellRejectedTest [1]
FGridMagicMON188HotbarSpellRoundTripTest [1] ; FGridMagicMON188HotbarWithoutKnowledgePreservedTest [1] ; FGridMagicMON188InvalidSpellIdRejectedTest [1] ; FGridMagicMON188MultipleCharactersRoundTripTest [1]
FGridMagicMON188SingleCharacterRoundTripTest [1] ; FGridMagicMON188SpellBindingIsNotItemDefinitionTest [1] ; FGridMagicMON188UnknownDefinitionRejectedTest [1] ; FGridMapMON21610ContractTest [1]
FGridMapMON21610PanTest [1] ; FGridMapMON21610RecenterTest [1] ; FGridMapMON21610ZoomTest [1] ; FGridMapMON21611DefaultsTest [1]
FGridMapMON21611NoiseTest [1] ; FGridMapMON21611SecretRegressionTest [1] ; FGridMapMON21611TransientTest [1] ; FGridMapMON21612ArrivalRevealTest [1]
FGridMapMON21612FilteringTest [1] ; FGridMapMON21612FloorProjectionTest [1] ; FGridMapMON21612PitTest [1] ; FGridMapMON21612PresentationTest [1]
FGridMapMON21613BreathingRoomTest [1] ; FGridMapMON21613CanvasFitTest [1] ; FGridMapMON21613ClosureContractTest [1] ; FGridMapMON21613PolishDefaultsTest [1]
FGridMapMON2162DefaultsTest [1] ; FGridMapMON2162MutationTest [1] ; FGridMapMON2162PerLevelAuthorityTest [1] ; FGridMapMON2162SaveBoundaryTest [1]
FGridMapMON2163DoorTest [1] ; FGridMapMON2163RadiusTest [1] ; FGridMapMON2163RuntimeHookTest [1] ; FGridMapMON2163WallsTest [1]
FGridMapMON2164InitialOpenTest [1] ; FGridMapMON2164NormalDoorAndSaveBoundaryTest [1] ; FGridMapMON2164OpeningTest [1] ; FGridMapMON2164StateTest [1]
FGridMapMON2165MalformedTest [1] ; FGridMapMON2165RoundTripTest [1] ; FGridMapMON2165SchemaFlagsTest [1] ; FGridMapMON2165VersionTest [1]
FGridMapMON2166DoorStateTest [1] ; FGridMapMON2166FilteringTest [1] ; FGridMapMON2166SecretTest [1] ; FGridMapMON2166TransientTest [1]
FGridMapMON2167ContractTest [1] ; FGridMapMON2167CoordinatesTest [1] ; FGridMapMON2167FloorsTest [1] ; FGridMapMON2167SeamTest [1]
FGridMapMON2168FailClosedTest [1] ; FGridMapMON2168RefreshTest [1] ; FGridMapMON2168StandaloneHookTest [1] ; FGridMapMON2168WidgetContractTest [1]
FGridMapMON2169LevelLabelTest [1] ; FGridMapMON2169ProjectionTest [1] ; FGridMapMON2169RefreshContractTest [1] ; FGridMapMON2169TraversalTest [1]
FGridMapMON2169UIContractTest [1] ; FGridMonsterDefinitionMON1InvalidDataTest [1] ; FGridMonsterDefinitionMON1ValidationTest [1] ; FGridMonsterMON10AudioAttackAndImpactExactlyOnceTest [2]
FGridMonsterMON10AudioCombatStartAlertExactlyOnceTest [1] ; FGridMonsterMON10AudioDefinitionValidationTest [1] ; FGridMonsterMON10AudioDeterministicVariantSelectionTest [1] ; FGridMonsterMON10AudioHurtDeathExclusivityTest [1]
FGridMonsterMON10AudioIdleTimerLifecycleTest [1] ; FGridMonsterMON10AudioRestoreDeadSilentTest [1] ; FGridMonsterMON10CombatLogAttackExactlyOnceTest [2] ; FGridMonsterMON10CombatLogCharacterDefeatedTest [2]
FGridMonsterMON10CombatLogFailedStartDoesNotClearHistoryTest [2] ; FGridMonsterMON10CombatLogFormatterTest [1] ; FGridMonsterMON10CombatLogMonsterDefeatedAndVictoryTest [2] ; FGridMonsterMON10CombatLogNoPersistenceRegressionTest [1]
FGridMonsterMON10CombatLogRingBufferTest [2] ; FGridMonsterMON10IdleVariationDefinitionValidationTest [1] ; FGridMonsterMON10IdleVariationDeterministicDelayTest [1] ; FGridMonsterMON10IdleVariationDeterministicSelectionTest [1]
FGridMonsterMON10IdleVariationNoImmediateRepeatTest [1] ; FGridMonsterMON10IdleVariationNoTickNoPersistenceAndIsolationTest [2] ; FGridMonsterMON10IdleVariationRequestExactlyOnceTest [2] ; FGridMonsterMON10IdleVariationRestoreSilentTest [2]
FGridMonsterMON10IdleVariationSchedulingLifecycleTest [2] ; FGridMonsterMON10OptimizationBalanceSnapshotTest [1] ; FGridMonsterMON10OptimizationEncounterSeedDeterministicTest [1] ; FGridMonsterMON10OptimizationEncounterSeedDistinguishesTest [1]
FGridMonsterMON10OptimizationEncounterSeedLifecycleTest [2] ; FGridMonsterMON10OptimizationInvariantsTest [1] ; FGridMonsterMON10OptimizationMetricsLifecycleTest [2] ; FGridMonsterMON10OptimizationMetricsNoGameplayInfluenceTest [2]
FGridMonsterMON10VFXAttackAndImpactExactlyOnceTest [2] ; FGridMonsterMON10VFXCombatStartAlertExactlyOnceTest [1] ; FGridMonsterMON10VFXDefinitionValidationTest [1] ; FGridMonsterMON10VFXDeterministicVariationSelectionTest [1]
FGridMonsterMON10VFXHurtDeathExclusivityTest [1] ; FGridMonsterMON10VFXNoPersistenceNoTickAndCleanupTest [1] ; FGridMonsterMON10VFXRequestDataAndNativeSpawnDisabledTest [1] ; FGridMonsterMON10VFXRestoreDeadSilentTest [1]
FGridMonsterMON11ElementalOffensiveEquipmentTest [2] ; FGridMonsterMON11EquippedGuardRuntimeTest [1] ; FGridMonsterMON11EquippedWeaponMappingTest [2] ; FGridMonsterMON11HandPriorityAndFallbackTest [2]
FGridMonsterMON11OffensiveProfileValidationTest [2] ; FGridMonsterMON11PerCharacterActionGateTest [2] ; FGridMonsterMON11PlayerResolutionArmorAndCriticalTest [2] ; FGridMonsterMON11PlayerResolutionDeathVictoryTest [2]
FGridMonsterMON11PlayerResolutionDeterminismTest [2] ; FGridMonsterMON11PlayerResolutionMappingTest [2] ; FGridMonsterMON11RangedWeaponTargetingTest [2] ; FGridMonsterMON11RequestAcceptedAndResolvedTest [2]
FGridMonsterMON11RequestValidationTest [1] ; FGridMonsterMON11TargetingTest [2] ; FGridMonsterMON1210HotbarTargetingTest [1] ; FGridMonsterMON1211HotbarValidationTest [2]
FGridMonsterMON1271DynamicInitiativeTest [2] ; FGridMonsterMON1271InitiativePreviewTest [1] ; FGridMonsterMON1283HotbarClickExecutionTest [1] ; FGridMonsterMON1283HotbarKeyboardGuardTest [1]
FGridMonsterMON1283UnarmedHotbarExecutionTest [1] ; FGridMonsterMON1284QuickItemEffectTest [1] ; FGridMonsterMON1284QuickItemScrollAttackTest [1] ; FGridMonsterMON1285ActionPaletteBindingTest [1]
FGridMonsterMON1285DirectSpellManaTransactionTest [1] ; FGridMonsterMON1285SelfAbilityEffectTest [1] ; FGridMonsterMON1286AreaTargetingTransactionTest [1] ; FGridMonsterMON1286CellTargetingLifecycleTest [1]
FGridMonsterMON1286TargetRequiredNoSpendTest [1] ; FGridMonsterMON1287InventoryThrowableTest [1] ; FGridMonsterMON1287PersistentHotbarTest [1] ; FGridMonsterMON1289WeaponDragDropUniquenessTest [1]
FGridMonsterMON129ActionRolloverTest [1] ; FGridMonsterMON12ActionCatalogContributionsTest [1] ; FGridMonsterMON12ActionTransactionTest [2] ; FGridMonsterMON12CharacterActionPointLifecycleTest [2]
FGridMonsterMON12CombatActionPanelLiveDataTest [1] ; FGridMonsterMON12CombatActionPanelTurnAuthorityTest [1] ; FGridMonsterMON12CombatHudLifecycleTest [2] ; FGridMonsterMON12CombatHudViewModelTest [1]
FGridMonsterMON12FreshPlacedCombatInitializationTest [2] ; FGridMonsterMON12GenericActionAttackLifecycleTest [1] ; FGridMonsterMON12GlobalInitiativeLifecycleTest [2] ; FGridMonsterMON12InitializedPersistenceCaptureTest [2]
FGridMonsterMON12InitiativeOrderRulesTest [1] ; FGridMonsterMON12InvalidDefinitionAdmissionTest [2] ; FGridMonsterMON12PartyMobilityLifecycleTest [3] ; FGridMonsterMON12RatGiantAssetCompatibilityTest [1]
FGridMonsterMON12RestoredDeadCombatInitializationTest [2] ; FGridMonsterMON12RestoredInjuredCombatInitializationTest [2] ; FGridMonsterMON12UninitializedInitiativeStateTest [2] ; FGridMonsterMON131PaletteContractTest [1]
FGridMonsterMON131PersistentModelTest [1] ; FGridMonsterMON131TypedAuthorityTest [1] ; FGridMonsterMON131ValidationTest [1] ; FGridMonsterMON132AtomicFailureTest [1]
FGridMonsterMON132EditorPreviewTest [1] ; FGridMonsterMON132RuntimePipelineTest [1] ; FGridMonsterMON133AtomicCommandsTest [1] ; FGridMonsterMON133DeferredSpawnLinksTest [1]
FGridMonsterMON133LifecyclePersistenceTest [1] ; FGridMonsterMON134AtomicWaveFailureTest [1] ; FGridMonsterMON134EncounterWavesTest [1] ; FGridMonsterMON134ValidationTest [1]
FGridMonsterMON135ProductionAssetContractTest [1] ; FGridMonsterMON136CoalescedGroupsTest [1] ; FGridMonsterMON136DeferredRetryTest [1] ; FGridMonsterMON136DeterministicEncounterStartTest [1]
FGridMonsterMON141CoalescingAggroTest [1] ; FGridMonsterMON141DirectSightTest [1] ; FGridMonsterMON141ExclusionTest [1] ; FGridMonsterMON141HearingDormantManualTest [1]
FGridMonsterMON141RangeWallDoorTest [1] ; FGridMonsterMON141RelocationRebuildSafetyTest [1] ; FGridMonsterMON142BehaviorFacingIntegrationTest [1] ; FGridMonsterMON142DirectionalSightTest [1]
FGridMonsterMON142FreshSpawnConfigurationTest [1] ; FGridMonsterMON142SpawnModelValidationTest [1] ; FGridMonsterMON143AuthoredRouteResyncTest [1] ; FGridMonsterMON143AutomaticStartupPatrolPipelineTest [1]
FGridMonsterMON143BehaviorOrphanedAwarenessNormalizationTest [1] ; FGridMonsterMON143BlockedHearingWaitTest [1] ; FGridMonsterMON143CombatExitStateReconciliationTest [1] ; FGridMonsterMON143CombatSuspensionTest [1]
FGridMonsterMON143CursorRulesTest [1] ; FGridMonsterMON143DormantPatrolTest [1] ; FGridMonsterMON143HearingInvestigationTest [1] ; FGridMonsterMON143PatrolMovementTest [1]
FGridMonsterMON143RuntimeEntryActorReplacementTest [1] ; FGridMonsterMON144AlarmFilteringTest [1] ; FGridMonsterMON144HearingAlarmPropagationTest [1] ; FGridMonsterMON144SharingDisabledTest [1]
FGridMonsterMON171GoblinDefinitionTest [1] ; FGridMonsterMON171RangedAttackContractTest [1] ; FGridMonsterMON171SpawnPersistenceContractTest [1] ; FGridMonsterMON172PresentationBridgeContractTest [1]
FGridMonsterMON172VisualRotationOffsetContractTest [1] ; FGridMonsterMON1731LineOfSightTest [1] ; FGridMonsterMON1731MeleeRegressionPlannerTest [1] ; FGridMonsterMON1731StationaryRangedPlannerTest [1]
FGridMonsterMON1732ProjectileTimingTest [1] ; FGridMonsterMON1732ProjectileTrajectoryTest [1] ; FGridMonsterMON1732ProjectileVisualOptionalityTest [1] ; FGridMonsterMON1733ProjectileSourceContractTest [1]
FGridMonsterMON1734CooldownIsolationTest [1] ; FGridMonsterMON1734CooldownLifecycleTest [1] ; FGridMonsterMON1734GoblinZeroCooldownContractTest [1] ; FGridMonsterMON1741MultiTurnApproachTest [1]
FGridMonsterMON1741PreferredFiringCandidatesTest [1] ; FGridMonsterMON1741RepositionThenAttackTest [1] ; FGridMonsterMON1751DirectionalPerceptionTest [1] ; FGridMonsterMON1751HearingAlarmTest [1]
FGridMonsterMON1751PatrolRangedKeeperTest [1] ; FGridMonsterMON1751VisionEngagementHandoffTest [1] ; FGridMonsterMON1761DeathRewardsExactlyOnceTest [1] ; FGridMonsterMON1761EncounterWaveParticipationTest [1]
FGridMonsterMON1761PersistenceNoReplayTest [1] ; FGridMonsterMON1761ProductionRewardContractTest [1] ; FGridMonsterMON177FinalBalanceContractTest [1] ; FGridMonsterMON177ProductionBalanceBaselineTest [1]
FGridMonsterMON177ProductionLootBaselineTest [1] ; FGridMonsterMON177RewardPacingBaselineTest [1] ; FGridMonsterMON178AnimationStateBridgeContractTest [1] ; FGridMonsterMON178BestiaryPresentationBridgeTest [1]
FGridMonsterMON178DeadRestorePresentationContractTest [1] ; FGridMonsterMON178DeathDefinitionContractTest [1] ; FGridMonsterMON178DeathDissolveApiContractTest [1] ; FGridMonsterMON178DeathDissolveDefinitionContractTest [1]
FGridMonsterMON178DeathPresentationApiContractTest [1] ; FGridMonsterMON178LivingRestoreClearsDeathPresentationTest [1] ; FGridMonsterMON179FatalDamageBypassesHurtTest [1] ; FGridMonsterMON179HurtDefinitionContractTest [1]
FGridMonsterMON179HurtPresentationApiContractTest [1] ; FGridMonsterMON179HurtRestoreNormalizationTest [1] ; FGridMonsterMON179MissingHurtMontageIsSafeTest [1] ; FGridMonsterMON179NonFatalDamageRequestsHurtTest [1]
FGridMonsterMON20102DeadRestoreOverPartyCellTest [1] ; FGridMonsterMON20102LivingRestoreStillRejectsPartyCellTest [1] ; FGridMonsterMON3InvalidTransitionsTest [1] ; FGridMonsterMON3OccupancyRegistryTest [1]
FGridMonsterMON4PathfinderTest [1] ; FGridMonsterMON4PerceptionTest [1] ; FGridMonsterMON5ActionPointsAndPlanningTest [1] ; FGridMonsterMON5PhaseStateMachineTest [1]
FGridMonsterMON6CombatResolverTest [1] ; FGridMonsterMON6DirectMeleePlannerTest [1] ; FGridMonsterMON6PartyTargetSelectorTest [1] ; FGridMonsterMON7ActionPlanningTest [1]
FGridMonsterMON7GroupAggroTest [1] ; FGridMonsterMON7RetreatChanceTest [1] ; FGridMonsterMON7RetreatScoringTest [1] ; FGridMonsterMON8DeathExactlyOnceTest [1]
FGridMonsterMON8LootResolverTest [1] ; FGridMonsterMON8MonsterDiedEventTest [1] ; FGridMonsterMON8MultipleIndependentDropsTest [1] ; FGridMonsterMON8OccupancyReleaseTest [1]
FGridMonsterMON8VictoryAfterEnemyPhaseStartedTest [1] ; FGridMonsterMON8VictoryOnLastDeathTest [1] ; FGridMonsterMON9BeginPlayOrderIndependenceTest [1] ; FGridMonsterMON9DeadRoundTripTest [1]
FGridMonsterMON9DirectPlacedInitialCellInferenceTest [1] ; FGridMonsterMON9DirectPlacedNewGameStartsAliveTest [1] ; FGridMonsterMON9DiskSaveRoundTripTest [1] ; FGridMonsterMON9LevelTransitionTest [1]
FGridMonsterMON9MovementAutoInitSkipsDeadTest [1] ; FGridMonsterMON9OccupancyRestoreTest [1] ; FGridMonsterMON9OrphanedPursuitRestoreTest [1] ; FGridMonsterMON9RestoredCellNotReinferredTest [1]
FGridMonsterMON9RestoredDeadRemainsDeadTest [1] ; FGridMonsterMON9SaveVersionExactMatchTest [1] ; FGridMonsterMON9StableIdentityTest [1] ; FGridMonsterMON9StateRoundTripTest [1]
FGridQuestMON212CampaignRuntimeStateTest [1] ; FGridQuestMON212DefinitionValidationTest [1] ; FGridQuestMON213EventCommandIntegrationTest [1] ; FGridQuestMON213World [1]
FGridQuestMON2142SaveRoundTripTest [1] ; FGridQuestMON2142SaveSchemaTest [1] ; FGridQuestMON2142SubsystemSnapshotApiTest [1] ; FGridQuestMON214PipelineGapCharacterizationTest [1]
FGridQuestMON214RuntimeAuthorityCharacterizationTest [1] ; FGridQuestMON214SaveEnvelopeCharacterizationTest [1] ; FGridQuestMON214ValidationCharacterizationTest [1] ; FGridRPG0394B1SpellSchoolSourceTagProjectionTest [1]
FGridRPG0394B4SpellOutgoingDamageModifierTest [1] ; FGridRPGPerf01OverloadReadTest [1] ; FGridRPGPerf01ProgressionReadTest [1] ; FGridTD011DisabledRoundTripTest [1]
FGridTD011EnabledRoundTripTest [1] ; FGridTD012NoLegacyTorchFieldsTest [1] ; FGridTD012SelectedCharacterFilterTest [1] ; FGridTD012SelectionChangeTest [1]
FGridTD0132StateOnlyRuntimeRejectionTest [1] ; FGridTD013EventCommandPolicyTest [1] ; FGridTD013EventCommandValidationTest [1] ; FGridTD021World [1]
FGridTD021WorldItemsContractTest [2] ; FGridTD022EquipmentWorldTransferContractTest [1] ; FGridTD023PartyInventoryDiagnosticsContractTest [1] ; FGridTD027PartyItemTransferCursorFacadeContractTest [1]
FGridTD027PhysicalThrowAimingContractTest [1] ; FGridTD027PhysicalThrowRulesTest [1] ; FGridTD027TestWorld [1] ; FGridTD028PartyUIFacadeModalBlockingContractTest [1]
FGridTD028TestWorld [1] ; FGridTD029PartyMovementFacadeGridStartContractTest [1] ; FGridTD029TestWorld [1] ; FGridTD031CanonicalEditorActionsContractTest [1]
FGridTD031EditorTestWorld [1] ; FGridTD032EditorTestWorld [1] ; FGridTD032ObjectInspectorMoveContractTest [1] ; FGridTD033DefinitionSyncContractTest [1]
FGridTD033EditorTestWorld [1] ; FGridTD034DungeonLevelContractTest [1] ; FGridTD034EditorTestWorld [1] ; FGridTD052RuntimeActorDiagnosticsContractTest [1]
FGridTD052RuntimeDiagnosticsWorld [1] ; FGridTD055RuntimeActorFeedbackUIContractTest [1] ; FGridTD055RuntimeFeedbackWorld [1] ; FGridTD062PartyInventoryHotbarContractTest [1]
FGridTD064InventoryStackMergeTest [1] ; FGridTD064InventoryStackMergeWorld [1] ; FGridTD064PartyInventoryCursorTransferContractTest [1] ; FGridTD066PartyInventoryEquipmentCoreContractTest [1]
FGridTD068PartyInventoryItemDefinitionRegistryContractTest [1] ; FGridTD072EngineCompatibilityReflectionContractTest [1] ; FGridTD0731CurrentSchemaAssetAuditTest [1] ; FGridTD0731Finding [1]
FGridTD0732ExactVersionContractTest [1] ; FGridTD0732PersistentChoiceRoundTripTest [1] ; FGridTD0732RejectCurrentMismatchTest [1] ; FGridTD0732RejectInvalidChoiceSnapshotTest [1]
FGridTD0732StateGuard [1] ; FGridTD073310ActiveRoundTripTest [1] ; FGridTD073310AuthorityFlagsTest [1] ; FGridTD073310DerivedStatsPersistenceGapTest [1]
FGridTD073310DerivedStatsValidationGapTest [1] ; FGridTD073310PoolRoundTripTest [1] ; FGridTD073310SaveEnvelopeTest [1] ; FGridTD073310SaveSchemaVersionTest [1]
FGridTD073310SchemaAuthorityTest [1] ; FGridTD07332CurrentAttributeAuthorityTest [1] ; FGridTD07332RecruitmentUsesCurrentAttributesTest [1] ; FGridTD07332SaveSchemaVersionTest [1]
FGridTD07333CombatConsumerBoundaryTest [1] ; FGridTD07333EquipmentProjectionContractTest [1] ; FGridTD07333MagicResourceBoundaryTest [1] ; FGridTD07333MixedFactoryContractTest [1]
FGridTD07333ResourceInitializationTest [1] ; FGridTD07333ResourceRemovalProjectionTest [1] ; FGridTD07333SaveSchemaVersionTest [1] ; FGridTD07333SchemaSeparationTest [1]
FGridTD07334CachedCapacityContractTest [1] ; FGridTD07334CursorWeightBoundaryTest [1] ; FGridTD07334EquipmentCapacityProjectionContractTest [1] ; FGridTD07334EquipmentPolicyTest [1]
FGridTD07334LiveProjectionTest [1] ; FGridTD07334RestoreRecalculatesCachesTest [1] ; FGridTD07334SaveSchemaVersionTest [1] ; FGridTD07334SchemaSeparationTest [1]
FGridTD07335B2CharacterChoiceRoundTripTest [1] ; FGridTD07335B2ProjectionRebuildTest [1] ; FGridTD07335B2SaveSchemaVersionTest [1] ; FGridTD07335B2SchemaPurgeTest [1]
FGridTD07335CharacterChoiceAuthorityTest [1] ; FGridTD07335ExperienceLevelContractTest [1] ; FGridTD07335LevelUpProjectionRefreshTest [1] ; FGridTD07335ProgressionMirrorBoundaryTest [1]
FGridTD07335SaveSchemaVersionTest [1] ; FGridTD07335SchemaAuthorityTest [1] ; FGridTD07335StoredLevelSynchronizationTest [1] ; FGridTD07335TransientLevelRoundTripTest [1]
FGridTD07336DeterministicMutationTest [1] ; FGridTD07336DirectValidationTest [1] ; FGridTD07336RestoreReplacementBoundaryTest [1] ; FGridTD07336RuntimeAuthorityBoundaryTest [1]
FGridTD07336SaveSchemaVersionTest [1] ; FGridTD07336SchemaAuthorityTest [1] ; FGridTD07336SeparatePersistenceMirrorTest [1] ; FGridTD07336SparseSnapshotContractTest [1]
FGridTD07337ActivePoolDurabilityTest [1] ; FGridTD07337CanonicalDefinitionAndHotbarIndependenceTest [1] ; FGridTD07337DirectMutationTest [1] ; FGridTD07337RestoreReplacementBoundaryTest [1]
FGridTD07337RuntimeAuthorityBoundaryTest [1] ; FGridTD07337SaveSchemaVersionTest [1] ; FGridTD07337SchemaAuthorityTest [1] ; FGridTD07337SparsePersistenceMirrorTest [1]
FGridTD07338AtomicRehydrationTest [1] ; FGridTD07338DirectDurableValidationTest [1] ; FGridTD07338MonsterSnapshotIsolationTest [1] ; FGridTD07338PartyRestoreReplacementBoundaryTest [1]
FGridTD07338PartySparseSaveMirrorTest [1] ; FGridTD07338RuntimeAuthorityBoundaryTest [1] ; FGridTD07338SaveSchemaVersionTest [1] ; FGridTD07338SchemaAuthorityTest [1]
FGridTD07342ClassIdentityContractTest [1] ; FGridTD07342RaceIdentityContractTest [1] ; FGridTD07342StrictResolverContractTest [1] ; FGridTD07342VisualIdentityContractTest [1]
FGridTD07343ActivePoolRehydrationTest [1] ; FGridTD07343SaveRoundTripTest [1] ; FGridTD07343SaveSchemaVersionTest [1] ; FGridTD07343SchemaAuthorityTest [1]
FGridTD07344AuthoringCleanupTest [1] ; FGridTD07344CanonicalVisualResolverTest [1] ; FGridTD07344SchemaAuthorityTest [1] ; FGridTD07344VisualRoundTripTest [1]
FGridTD0734CharacterDurableIdentityDuplicationTest [1] ; FGridTD0734CreationCopiesPresentationStateTest [1] ; FGridTD0734PrimaryAssetIdentityGapTest [1] ; FGridTD0734VisualFallbackDuplicationTest [1]
FGridTD07352CombatActionsOnlyBehaviorTest [1] ; FGridTD07352RuntimeFallbackRemovalTest [1] ; FGridTD07352SchemaAuthorityTest [1] ; FGridTD07352ShurikenAssetAuthorityTest [1]
FGridTD07353CurrentPresentationDefinitionsTest [1] ; FGridTD07353RepairedAssetsTest [1] ; FGridTD07353RuntimeFallbackRemovalTest [1] ; FGridTD07353SchemaAuthorityTest [1]
FGridTD07354GoblinThrowerRangeAuthorityTest [1] ; FGridTD07354RatGiantRangeAuthorityTest [1] ; FGridTD07354RuntimeConsumersNormalizedTest [1] ; FGridTD07354SchemaAuthorityTest [1]
FGridTD0735ItemSchemaAuthorityTest [1] ; FGridTD0735LegacyItemAdapterRemovedTest [1] ; FGridTD0735MonsterPresentationAuthorityTest [1] ; FGridTD0735MonsterPresentationRuntimeNormalizedTest [1]
FGridTD0736CurrentMonsterSpawnAssetsTest [1] ; FGridTD0736DeprecatedCombatAndKeyboardUseTest [1] ; FGridTD0736LegacyInfrastructureRemovedTest [1] ; FGridTD0736LegacyPlacementMirrorsTest [1]
FGridTD0736LegacyRebuildModeTest [1] ; FGridTD0736LegacySymbolsAbsentTest [1] ; FGridTD0736MonsterSpawnFacingAuthorityTest [1] ; FGridTD0736MonsterSpawnYawFallbackTest [1]
FGridTD0737CurrentAssetRepairCharacterizationTest [1] ; FGridTD0737CurrentAssetsNormalizedTest [1] ; FGridTD0737LockAuthoringNormalizedTest [1] ; FGridTD0737MonsterSpawnAuthoringAuthorityTest [1]
FGridTD0737RatLootDefinitionsNormalizedTest [1] ; FGridTD0738CharacterAuthorityTest [1] ; FGridTD0738CurrentAssetsStrictTest [1] ; FGridTD0738LegacySymbolsAbsentTest [1]
FGridTD0738MonsterAssetsStrictTest [1] ; FGridTD0738SaveExactMatchTest [1] ; FGridTD074ActivationAuthorityCharacterizationTest [1] ; FGridTD074ActivationCoverageCharacterizationTest [1]
FGridTD074ActivationResponsibilitiesCharacterizationTest [1] ; FGridTD074ActivationSurfaceCharacterizationTest [1] ; FGridTD075ReceptacleCommandRecoveryTest [1] ; FGridTRAP0191ExactD20Odds [1]
FGridUIClean01NoInventoryShellFallbackTest [1] ; FGridUIClean02NoGeneratedPaperDollLegacyTest [1] ; FGridUIClean03NoLegacyArmorAliasTest [1] ; FGridUIClean04NoPrimitiveDisplayHelpersTest [1]
FGridUIItem01Clean01NoLegacyTooltipProjectionTest [1] ; FGridUITooltipClean021RemovedDetachedContainers [1] ; FGridWorldObjectMIG00PlacementSchemaCharacterizationTest [1] ; FGridWorldObjectMIG00PlacementTransformCharacterizationTest [1]
FGridWorldObjectMIG01PlacementSchemaTest [1] ; FGridWorldObjectMIG01PlacementTransformParityTest [1] ; FGridWorldObjectMIG021DefinitionValidationTest [1] ; FGridWorldObjectMIG02BoundaryKeyTest [1]
FGridWorldObjectMIG02SpatialBehaviorSchemaTest [1] ; FGridWorldObjectMIG03DefinitionVisualContractTest [1] ; FGridWorldObjectMIG03RuntimeConsumerContractTest [1] ; FGridWorldObjectMIG03RuntimeSpawnFromVisualCompositionTest [1]
FGridWorldObjectMIG03TargetMotionStateMachinesTest [1] ; FGridWorldObjectMIG03TestWorld [1] ; FGridWorldObjectMIG03VisualCompositionTypesTest [1] ; FGridWorldObjectMIG04BehaviorSchemaAuthorityTest [1]
FGridWorldObjectMIG04RuntimeGenericMotionContractTest [1] ; FGridWorldObjectMIG05DirectCollectibleDefinitionTest [1] ; FGridWorldObjectMIG05PaletteAmbiguityTest [1] ; FGridWorldObjectMIG05PaletteIconAuthorityTest [1]
FGridWorldObjectMIG06SparseBehaviorResolutionTest [1] ; FGridWorldObjectMIG06SparseStorageContractTest [1] ; FGridWorldObjectMIG07SchemaReflectionTest [1] ; FGridWorldObjectMIG07TypedAuthorityBridgeTest [1]
FGridWorldObjectMIG07TypedAuthoritySchemaTest [1] ; FGridWorldObjectMIG07TypedLifecycleSchemaTest [1] ; FGridWorldObjectMIG07TypedLifecycleTest [1] ; FGridWorldObjectMIG07TypedPlacementProjectionTest [1]
FGridWorldObjectMIG09DefinitionAuthorityCutoverTest [1] ; FGridWorldObjectMIG09ReceptacleItemIdentityTest [1] ; FGridWorldObjectMIG09RuntimeItemSaveIdentityTest [1] ; FGridWorldObjectMIG09RuntimePayloadTest [1]
FGridWorldObjectMIG09SparseMarkerPurgeTest [1] ; FGridWorldObjectMIG10DefinitionNamingTest [1] ; FINDUS044NarrativeApprovalTest [1] ; FINDUS044NarrativeParserTest [1]
FIsolateMON135PIE [1] ; FLOT024AmmunitionInventory [1] ; FMIG05EditorTestWorld [1] ; FMIG05ItemTestWorld [1]
FMIG06EditorTestWorld [1] ; FMIG07CEditorTestWorld [1] ; FMIG09AuthorityWorld [1] ; FMON135RealPIEState [1]
FMON155RuntimeStateGuard [16] ; FMON167ProgressionGuard [1] ; FMON188DiskSlot [1] ; FMON1923TestWorld [1]
FMON194RuntimeFixture [1] ; FMON194TestWorld [1] ; FMON1971EditorWorld [1] ; FMON1971LogicIdWorld [1]
FMON198TestWorld [1] ; FMON2094DurableSkillState [1] ; FPrepareMON135ContinuePIE [1] ; FProtectMON135SaveSlot [1]
FRPG0310ActionEconomyTest [1] ; FRPG0310AppliedStatusClosureTest [1] ; FRPG0310ProductionClassSetTest [1] ; FRPG0310RealPIESmokeTest [1]
FRPG0310RequirementClosureTest [1] ; FRPG0310TierEconomyTest [1] ; FRPG031ActionProjectionTest [1] ; FRPG031AttackModifierTest [1]
FRPG031ChoiceProjectionTest [1] ; FRPG031CriticalResolutionTest [1] ; FRPG031FilterIsolationTest [1] ; FRPG031IncomingModifierTest [1]
FRPG031ProfileValidationTest [1] ; FRPG032LifecycleBridgeTest [1] ; FRPG032MagicalTriggerAndDeathTest [1] ; FRPG032MonsterAttackValidationTest [1]
FRPG032PhysicalArmorGateTest [1] ; FRPG032ProfileValidationTest [1] ; FRPG032StatusModifierStackProjectionTest [1] ; FRPG032StatusOnlyCatalogTest [1]
FRPG033ArmorGateAfterC3Test [1] ; FRPG033ArmorOnlyCatalogTest [1] ; FRPG033FlatRestoreClampTest [1] ; FRPG033ProfileValidationTest [1]
FRPG033RawDamageNoOverflowTest [1] ; FRPG033ReferenceAndRestorationModifiersTest [1] ; FRPG033ScaledFlatMagnitudeTest [1] ; FRPG033SelfActionExecutionTest [1]
FRPG034FilterMatchingTest [1] ; FRPG034OncePerActionLedgerTest [1] ; FRPG034OncePerRoundLedgerTest [1] ; FRPG034ProfileValidationTest [1]
FRPG034ProjectionTest [1] ; FRPG034RecursiveIsolationTest [1] ; FRPG034ResolveMatchesTest [1] ; FRPG034RuntimeStatusConsumptionTest [1]
FRPG035BlockedGeometryTest [1] ; FRPG035CatalogMobilityGateTest [1] ; FRPG035DestinationResolutionTest [1] ; FRPG035DirectionResolutionTest [1]
FRPG035ForcedControlBypassTest [1] ; FRPG035ProfileValidationTest [1] ; FRPG035RejectedRetreatNoSpendTest [1] ; FRPG035TacticalRetreatExecutionTest [1]
FRPG036CanonicalReactionTableTest [1] ; FRPG036ChainReactionModifiersTest [1] ; FRPG036DurationAdvanceTest [1] ; FRPG036PeriodicArmorGateTest [1]
FRPG036ProfileValidationTest [1] ; FRPG036RuntimePersistenceTest [1] ; FRPG036RuntimeReactionMutationTest [1] ; FRPG036StateModifierProjectionTest [1]
FRPG037AlchemyDamageScalingTest [1] ; FRPG037CatalogSkillScaledEffectTest [1] ; FRPG037DiffusionProjectionTest [1] ; FRPG037GrenadierProjectionTest [1]
FRPG037ModifierValidationTest [1] ; FRPG037PositivePotionScalingTest [1] ; FRPG037QuickItemNormalizationTest [1] ; FRPG037SourceTagFilteringTest [1]
FRPG038ActionValidationTest [1] ; FRPG038DirectArmorDamageTest [1] ; FRPG038MonsterPartyTargetabilityTest [1] ; FRPG038PartyBatchSelectionTest [1]
FRPG038PrimaryTargetScopeTest [1] ; FRPG038StatusRemovalFilterTest [1] ; FRPG038TargetFilterTest [1] ; FRPG038TargetabilityTest [1]
FRPG0390ActionValidationTest [1] ; FRPG0390CatalogAvailabilityTest [1] ; FRPG0390CriticalOrderingTest [1] ; FRPG0390DescriptorProjectionTest [1]
FRPG0390ItemTagMatchingTest [1] ; FRPG0390ProfileValidationTest [1] ; FRPG0390RawDamageFloorTest [1] ; FRPG0390RawDamageMinimumTest [1]
FRPG0390ResolvedModifierContextTest [1] ; FRPG0391BreakerAuthoringTest [1] ; FRPG0391ConditionalModifierTest [1] ; FRPG0391GuardianAuthoringTest [1]
FRPG0391MaximumResourcePercentTest [1] ; FRPG0391ProductionAssetsTest [1] ; FRPG0391ReactionResponseTest [1] ; FRPG0391SecondWindFullHealthGuardTest [1]
FRPG0391StatusAuthoringTest [1] ; FRPG0391TargetVitalsTest [1] ; FRPG0391VariantProgressionTest [1] ; FRPG0391WarriorClassAuthoringTest [1]
FRPG0391WeaponMasterAuthoringTest [1] ; FRPG0392AssassinAuthoringTest [1] ; FRPG0392ProductionAssetsTest [1] ; FRPG0392ReactionSemanticTest [1]
FRPG0392RogueClassAuthoringTest [1] ; FRPG0392SaboteurAuthoringTest [1] ; FRPG0392ShadowAuthoringTest [1] ; FRPG0392SkillModifierTest [1]
FRPG0392StatusAuthoringTest [1] ; FRPG0392TargetConditionModifierTest [1] ; FRPG0392TrapAndSkillGateTest [1] ; FRPG0392WeaponSubtypeTest [1]
FRPG0393DistinctSourceStatusTest [1] ; FRPG0393DynamicWeaponRangeModifierCompositionTest [1] ; FRPG0393FavoredEnemyCategoryTitleAuthorityTest [1] ; FRPG0393GroupSkillCheckTest [1]
FRPG0393HunterAuthoringTest [1] ; FRPG0393MarkedStatusAuthoringTest [1] ; FRPG0393MarksmanAuthoringTest [1] ; FRPG0393OwnedTargetReactionTest [1]
FRPG0393PartyProgressionTest [1] ; FRPG0393ProductionAssetsTest [1] ; FRPG0393RangerClassAuthoringTest [1] ; FRPG0393ScoutAuthoringTest [1]
FRPG0393WeaponRangeProjectionTest [1] ; FRPG0394B3ActionResolvedSourceTagsTest [1] ; FRPG0394B6StatusModifierOwnerRequirementsTest [1] ; FRPG0394B6StatusReactionOwnerRequirementsTest [1]
FRPG0394CAffinityAuthoringTest [1] ; FRPG0394CEvokerRuntimeCompositionTest [1] ; FRPG0394CMageClassAuthoringTest [1] ; FRPG0394COverloadStatusAuthoringTest [1]
FRPG0394CProductionAssetsTest [1] ; FRPG0394DCataclysmAffinityTest [1] ; FRPG0394DChainTopologyTest [1] ; FRPG0394DControlledExplosionTest [1]
FRPG0394DEvokerBranchAuthoringTest [1] ; FRPG0394DProductionAssetsTest [1] ; FRPG0394DVariantAndDamageFormulaTest [1] ; FRPG0394E1MinimumManaCostTest [1]
FRPG0394E1SelectedCellRelocationTest [1] ; FRPG0394E1SemanticTargetTagsTest [1] ; FRPG0394E1StatusRemovalContractTest [1] ; FRPG0394E2ArcaneMasteryTest [1]
FRPG0394E2ArcaneShieldFormulaTest [1] ; FRPG0394E2ArcanistBranchAuthoringTest [1] ; FRPG0394E2DispelAuthoringTest [1] ; FRPG0394E2ProductionAssetsTest [1]
FRPG0394E2RunicManipulationTest [1] ; FRPG0394F1AffinitySurfaceProjectionTest [1] ; FRPG0394F1ReactionSecondaryDamageTest [1] ; FRPG0394F1SurfaceConversionTest [1]
FRPG0394F1TargetEnvironmentContextTest [1] ; FRPG0394F2ConductionAndPersistentSurfaceTest [1] ; FRPG0394F2ElementalConversionTest [1] ; FRPG0394F2ImbuementStatusTest [1]
FRPG0394F2ProductionAssetsTest [1] ; FRPG0394F2SurfaceWeaverBranchAuthoringTest [1] ; FRPG0394F2TerrainArchitectTest [1] ; FRPG0395B1HealingFormulaTest [1]
FRPG0395B1PriestStatusesTest [1] ; FRPG0395B1ProtectionActionsTest [1] ; FRPG0395B1RestorationActionsTest [1] ; FRPG0395B1SanctuaryBastionTest [1]
FRPG0395B1StructureTest [1] ; FRPG0395B2FavoredHolyTargetsTest [1] ; FRPG0395B2HolyDamageTest [1] ; FRPG0395B2HolyDispelTest [1]
FRPG0395B2ProgressionTest [1] ; FRPG0395B2StatusTest [1] ; FRPG0395B2StructureTest [1] ; FRPG0395B2TurnUndeadTest [1]
FRPG0395CProductionClassTest [1] ; FRPG0395CProductionStatusesTest [1] ; FRPG0395DirectDamageAttributeSkillScalingTest [1] ; FRPG0395DirectHealingScalingTest [1]
FRPG0395HealingFloorTest [1] ; FRPG0395PartyCenteredForcedMovementTest [1] ; FRPG0395PeriodicHealingProfileTest [1] ; FRPG0395ReactionDamageSourceFilterTest [1]
FRPG0396AnyCanonicalReactionTest [1] ; FRPG0396B1ApothecaryItemsTest [1] ; FRPG0396B1ApothecaryPassivesTest [1] ; FRPG0396B1BombsTest [1]
FRPG0396B1GrenadierPassivesTest [1] ; FRPG0396B1ItemIdentityTest [1] ; FRPG0396B1StatusesTest [1] ; FRPG0396B1StructureTest [1]
FRPG0396B2AcidTest [1] ; FRPG0396B2CatalystTest [1] ; FRPG0396B2CloudTest [1] ; FRPG0396B2CorrodedTest [1]
FRPG0396B2MajorTransmutationTest [1] ; FRPG0396B2OilTest [1] ; FRPG0396B2StructureTest [1] ; FRPG0396CProductionClassTest [1]
FRPG0396CProductionItemsTest [1] ; FRPG0396CProductionStatusesTest [1] ; FRPG0396CatalystActionValidationTest [1] ; FRPG0396CatalystCatalogueTest [1]
FRPG0396ChainReactionLedgerTest [1] ; FRPG0396ConversionReactionBridgeTest [1] ; FRPG0396ConversionTraversalTest [1] ; FRPG0396FixedConversionDurationTest [1]
FRPG0396ReactionRecursionGuardTest [1] ; FRPG0396RuntimeTraversalQueryTest [1] ; FRPG0396SurfaceEffectReactionBridgeTest [1] ; FRPG0396SurfaceTraversalStateTest [1]
FRPGMON151ExistingCharacterStateTest [1] ; FRPGMON151LevelFromExperienceTest [1] ; FRPGMON151ProgressionBoundariesTest [1] ; FRPGMON151ProgressionCurveTest [1]
FRPGMON152ActivePartyDistributionTest [1] ; FRPGMON152LootIndependenceTest [1] ; FRPGMON152MonsterDeathExactlyOnceTest [1] ; FRPGMON152PersistenceStateTest [1]
FRPGMON152ProgressionBoundariesTest [1] ; FRPGMON152TestWorld [1] ; FRPGMON153AtomicFailureTest [1] ; FRPGMON153DeadCharacterTest [1]
FRPGMON153ExperienceIntegrationTest [1] ; FRPGMON153MultiLevelTransactionTest [1] ; FRPGMON153PersistenceTest [1] ; FRPGMON153SingleLevelResourcePolicyTest [1]
FRPGMON154ChoicePointAccountingTest [1] ; FRPGMON154CombatActionUnlockProjectionTest [1] ; FRPGMON154DefinitionValidationTest [1] ; FRPGMON154LevelUpIntegrationTest [1]
FRPGMON154NoProgressionChoicesCompatibilityTest [1] ; FRPGMON154RequirementProjectionTest [1] ; FRPGMON154SelectionRulesTest [1] ; FRPGMON155AtomicBatchCommitTest [1]
FRPGMON155AtomicFailureTest [1] ; FRPGMON155CatalogUnlockTest [1] ; FRPGMON155CharacterIsolationTest [1] ; FRPGMON155LevelUpNotificationSourceTest [1]
FRPGMON155TransientBoundaryTest [1] ; FRPGMON157FrozenCurveTest [1] ; FRPGMON157PartyRatPacingTest [1] ; FRPGMON157ProductionRatAssetTest [1]
FRPGMON157SoloRatPacingTest [1] ; FRPGMON161AtomicInvalidAddTest [1] ; FRPGMON161DefinitionValidationTest [1] ; FRPGMON161DeterministicCollectionTest [1]
FRPGMON161NoUIDependencyTest [1] ; FRPGMON161RuntimeStateCreationTest [1] ; FRPGMON161StableIdentityTest [1] ; FRPGMON161TargetIsolationTest [1]
FRPGMON162AddStacksTest [1] ; FRPGMON162AtomicFailureTest [1] ; FRPGMON162DeterministicExpirationTest [1] ; FRPGMON162NoStackAndRefreshTest [1]
FRPGMON162NoUIDependencyTest [1] ; FRPGMON162PermanentDurationLifecycleTest [1] ; FRPGMON162ReplaceIfStrongerTest [1] ; FRPGMON162RoundDurationLifecycleTest [1]
FRPGMON162TurnDurationLifecycleTest [1] ; FRPGMON162TurnManagerEventIntegrationTest [1] ; FRPGMON163DamageTypeRoutingTest [1] ; FRPGMON163DefinitionValidationTest [1]
FRPGMON163DirectDamagePipelineTest [1] ; FRPGMON163LethalMonsterDotTest [1] ; FRPGMON163MonsterMultiplierTest [1] ; FRPGMON163NoParallelSystemTest [1]
FRPGMON163NonPeriodicIsolationTest [1] ; FRPGMON163RoundLifecycleTest [1] ; FRPGMON163StackScalingTest [1] ; FRPGMON163TurnLifecycleMonsterTest [1]
FRPGMON163TurnLifecyclePartyTest [1] ; FRPGMON164ActiveCombatantStabilityTest [1] ; FRPGMON164AggregateModifierTest [1] ; FRPGMON164FutureHasteReorderTest [1]
FRPGMON164FutureSlowReorderTest [1] ; FRPGMON164MonsterParityTest [1] ; FRPGMON164NoParallelSystemTest [1] ; FRPGMON164ReapplicationUpdatesModifierTest [1]
FRPGMON164RoundExpirationTest [1] ; FRPGMON164StackScalingAndSaturationTest [1] ; FRPGMON164TurnExpirationTest [1] ; FRPGMON164TurnOrderBroadcastProjectionTest [1]
FRPGMON165ControlAggregationTest [1] ; FRPGMON165NoParallelSystemTest [1] ; FRPGMON165PartyImmobilizeRotationTest [1] ; FRPGMON165PartyImmobilizeTranslationTest [1]
FRPGMON165PermanentSkipActivationRejectedTest [1] ; FRPGMON165RoundSkipLifecycleTest [1] ; FRPGMON165SilenceCatalogIsolationTest [1] ; FRPGMON165SilenceRequestAtomicTest [1]
FRPGMON165StackBooleanSemanticsTest [1] ; FRPGMON165TargetParityTest [1] ; FRPGMON165TurnSkipLifecycleTest [1] ; FRPGMON166DeterministicProjectionTest [1]
FRPGMON166DurationFormattingTest [1] ; FRPGMON166FallbackProjectionTest [1] ; FRPGMON166MonsterFeedbackParityTest [1] ; FRPGMON166NoParallelSystemTest [1]
FRPGMON166PartyApplyFeedbackTest [1] ; FRPGMON166PartyPanelProjectionTest [1] ; FRPGMON166PresentationProjectionTest [1] ; FRPGMON166RefreshFeedbackTest [1]
FRPGMON166TickAndExpirationFeedbackTest [1] ; FRPGMON167AtomicRestoreFailureTest [1] ; FRPGMON167CollectionCaptureTest [1] ; FRPGMON167CollectionRestoreTest [1]
FRPGMON167DefinitionContractMismatchTest [1] ; FRPGMON167DuplicateEffectRejectedTest [1] ; FRPGMON167MonsterSnapshotContractTest [1] ; FRPGMON167PartyActivePoolRoundTripTest [1]
FRPGMON167PartyAtomicFailureTest [1] ; FRPGMON167SaveVersionContractTest [1] ; FRPGMON167TransientRuntimeBoundaryTest [1] ; FRPGMON168CrossFeatureCompositionTest [1]
FRPGMON168DeterministicPersistenceOrderTest [1] ; FRPGMON168LifecycleArchitectureBoundaryTest [1] ; FRPGMON168NoHardCodedStatusIdentityTest [1] ; FRPGMON168PersistenceRoundTripSemanticsTest [1]
FRPGMON168PrimaryAssetIdentityContractTest [1] ; FRPGMON168RegressionNamespaceCoverageTest [1] ; FRPGMON168RuntimeSaveBoundaryTest [1] ; FRPGMON168SaveVersionContractTest [1]
FRPGMON168SingleCanonicalModelTest [1] ; FRPGMON2055CustomRecruiterDefinitionContractTest [1] ; FRPGMON2055EventCommandContractTest [1] ; FRPGMON2055EventCommandMissingPlayerPawnTest [1]
FRPGMON2056CustomRecruitRuntimeCombatGateTest [1] ; FRPGMON205AllocatedAttributesPreservedTest [1] ; FRPGMON205ContextContractTest [1] ; FRPGMON205ContextGateCompletedPartyTest [1]
FRPGMON205ContextGateIncompletePartyTest [1] ; FRPGMON205ContextGatePartyFullTest [1] ; FRPGMON205CustomRecruitRuntimeContractTest [1] ; FRPGMON205CustomRecruitRuntimeDefaultStateTest [1]
FRPGMON205InitialHeroStatePreservedTest [1] ; FRPGMON205InvalidRequestAtomicRejectTest [1] ; FRPGMON205PartyFullAtomicRejectTest [1] ; FRPGMON205RecruitmentRollbackLeavesNoPoolCandidateTest [1]
FRPGMON205UniqueCharacterIdentityTest [1] ; FRPGMON205ValidCreateAndRecruitTest [1] ; FRPGMON205VisualSelectionPreservedTest [1] ; FRPGMON205WizardContextDefaultTest [1]
FRPGMON205WizardCustomCancelNoMutationTest [1] ; FRPGMON205WizardCustomRecruitSubmitTest [1] ; FRPGMON205WizardNewGameSubmitRegressionTest [1] ; FRPGMON2062DefaultRuntimeStateTest [1]
FRPGMON2062DefinitionValidationTest [1] ; FRPGMON2062DuplicateSkillStateRejectedTest [1] ; FRPGMON2062IncreaseRankTest [1] ; FRPGMON2062MaxRankAtomicRejectTest [1]
FRPGMON2062NegativeRankAtomicRejectTest [1] ; FRPGMON2062SetRankTest [1] ; FRPGMON2062ZeroRankRemovesSparseEntryTest [1] ; FRPGMON2063DeterministicSameSeedTest [1]
FRPGMON2063DifficultyThresholdTest [1] ; FRPGMON2063FormulaTest [1] ; FRPGMON2063InvalidDifficultyNoRandomTest [1] ; FRPGMON2063InvalidStateNoRandomTest [1]
FRPGMON2063NoneAttributeTest [1] ; FRPGMON2063UntrainedAllowedTest [1] ; FRPGMON2063UntrainedRejectedNoRandomTest [1] ; FRPGMON2074CaptureUsesMON15SnapshotTest [1]
FRPGMON2074InvalidRestoreAtomicTest [1] ; FRPGMON2074RequirementAfterTalentTest [1] ; FRPGMON2074RequirementBeforeTalentTest [1] ; FRPGMON2074RequirementCharacterIsolationTest [1]
FRPGMON2074RestoreByCharacterIdTest [1] ; FRPGMON2074RestoreLiveRequirementsTest [1] ; FRPGMON2074RestoreTalentReadModelTest [1] ; FRPGMON2082InvalidRankAtomicTest [1]
FRPGMON2082MissingDefinitionAtomicTest [1] ; FRPGMON2082MultipleSkillsProjectionTest [1] ; FRPGMON2082PrimaryAssetIdentityTest [1] ; FRPGMON2082RequirementGrantValidationTest [1]
FRPGMON2082ThresholdRequirementsTest [1] ; FRPGMON2082TrainedSkillIdTest [1] ; FRPGMON2082UntrainedNoRequirementsTest [1] ; FRPGMON2083MissingRequirementDiagnosticTest [1]
FRPGMON2083MissingRequirementsDeterministicOrderTest [1] ; FRPGMON2083NonRequirementFailureKeepsDiagnosticTest [1] ; FRPGMON2083RequirementsSatisfiedTest [1] ; FRPGMON2083SkillAndExistingRequirementsComposeTest [1]
FRPGMON2083SkillIdUnlocksActionTest [1] ; FRPGMON2083SkillProjectionFailureAtomicTest [1] ; FRPGMON2083SkillThresholdUnlocksActionTest [1] ; FRPGMON2084DuplicateDefinitionAtomicTest [1]
FRPGMON2084MissingRankDefinitionAtomicTest [1] ; FRPGMON2084SelectedAuthorityTest [1] ; FRPGMON2084SelectedCharacterIdentityTest [1] ; FRPGMON2084SkillOrderTest [1]
FRPGMON2084SkillRanksTest [1] ; FRPGMON2084TalentBalanceTest [1] ; FRPGMON2084TalentProjectionTest [1] ; FRPGMON2092CaptureActivePoolSparseTest [1]
FRPGMON2092CaptureDeterministicOrderTest [1] ; FRPGMON2092CaptureInvalidCharacterAtomicTest [1] ; FRPGMON2092CaptureInvalidSkillAtomicTest [1] ; FRPGMON2092RestoreByCharacterIdTest [1]
FRPGMON2092RestoreInvalidSnapshotAtomicTest [1] ; FRPGMON2092RestoreRoundTripTest [1] ; FRPGMON2093ActiveSnapshotRestoresAfterReserveMoveTest [1] ; FRPGMON2093MixedActivePoolIsolationTest [1]
FRPGMON2093PoolReorderRestoresByIdentityTest [1] ; FRPGMON2093PoolSnapshotRestoresAfterRecruitmentTest [1] ; FRPGMON2093RecruitmentPreservesPoolSkillTest [1] ; FRPGMON2093RecruitmentRejectPreservesPoolSkillTest [1]
FRPGMON2093SelectedCharacterIndependentTest [1] ; FRPGMON2093SnapshotValidAfterRecruitmentMoveTest [1] ; FRPGMON2094BelowThresholdStaysLockedTest [1] ; FRPGMON2094EmptySnapshotClearsConsumersTest [1]
FRPGMON2094InvalidRestorePreservesConsumersTest [1] ; FRPGMON2094RestoreProjectsSkillIdTest [1] ; FRPGMON2094RestoreProjectsThresholdTest [1] ; FRPGMON2094RestoreSkillsPageRankTest [1]
FRPGMON2094RestoreUnlocksActionTest [1] ; FRPGMON2094SelectedCharacterPageTest [1] ; FRPGUIRPG013LegacyMetadataBridgeTest [1] ; FRPGUIRPG013PartialMetadataAtomicTest [1]
FRPGUIRPG013PointsLockTest [1] ; FRPGUIRPG013TreeShapeTest [1] ; FRPGUIRPG013VariantSelectionTest [1] ; FSetupMON135RealPIE [1]
FStartMON135PIEIfReady [1] ; FTD0132TestWorld [1] ; FTD075TestWorld [1] ; FUIRPG014ExplicitTalentPresentationTypesTest [1]
FUIRPG014FavoredEnemyCategoryPresentationTest [1] ; FUIRPG014GlobalShapeTest [1] ; FUIRPG014ReadModelProjectionTest [1] ; FUIRPG014SixClassesMetadataTest [1]
FUIRPG014TierChainsTest [1] ; FUIRPG014VariantGroupingTest [1] ; FUIRPG021DuplicateBranchTest [1] ; FUIRPG021DuplicateClassTest [1]
FUIRPG021ThreeBranchContractTest [1] ; FUIRPG021ValidCatalogTest [1] ; FUIRPG021VisualOrderTest [1] ; FUIRPG022CanonicalOrderAndLabelsTest [1]
FUIRPG022CatalogLoadsTest [1] ; FUIRPG022MatchesGameplayBranchesTest [1] ; FUIRPG032OverrideValidationTest [1] ; FUIRPG032ProductionCoverageTest [1]
FUIRPG032SimpleNodeResolutionTest [1] ; FUIRPG032VariantOverrideTest [1] ; FUIRPG041KnownNodeSelectionTest [1] ; FUIRPG041RoutingContractTest [1]
FUIRPG041UnknownNodeSelectionTest [1] ; FUIRPG042SharedResolverTest [1] ; FUIRPG042SimpleDetailTest [1] ; FUIRPG042VariantDetailTest [1]
FUIRPG043AConfirmationStateTest [1] ; FUIRPG043ASimpleCommitTest [1] ; FUIRPG043AVariantDeferredTest [1] ; FUIRPG044AuthoredVariantLabelsTest [1]
FUIRPG044FlyoutConsultationSelectionTest [1] ; FUIRPG044GenericNVariantSelectionTest [1] ; FUIRPG044VariantTransactionTest [1] ; FUIRPG05NotificationWidgetStateTest [1]
FUIRPG05RejectMessagesTest [1] ; FUIRPG05SkillAndAttributeFeedbackTest [1] ; FUIRPG05TalentFailureNotificationTest [1] ; FUIRPG05TalentSuccessNotificationTest [1]
FUIRPG061CanonicalSkillCatalogTest [1] ; FUIRPG061SkillAuthoringPreservesExtensionsTest [1] ; FUIRPG061TrainingPolicyProjectionTest [1] ; FUIRPG062AProductionSkillCatalogTest [1]
FUIRPG062BSkillEntryPresentationTest [1] ; FUIRPG062BSkillEntryRejectsInvalidProjectionTest [1] ; FUIRPG062BSkillsRendererContractTest [1] ; FUITooltipClean01NominalShieldTest [1]
FUITooltipClean01RecoveryAndPreviewTest [1] ; FWaitForMON135PIE [1] ; FindMON152PartyInventory [1] ; FindMON1971Variable [1]
FindMON2092Character [1] ; FindMON2093Character [1] ; FindMON2094SkillView [1] ; FindMON8LootResult [1]
GetMON126AvailabilityText [1] ; GetMON135PIEWorld [1] ; GetMON155RuntimeRequirements [3] ; GetMON8LootLocalOffset [1]
GridEditorMON204StoryCompanionPlacementTests [1] ; GridEditorUIAssetClean01Private [1] ; GridItemVFX011Tests [1] ; GridItemVFX012AuthoringTests [1]
GridItemVFX012StackTests [1] ; GridItemVFX013AuthoringTests [1] ; GridItemVFX013RuntimeTests [1] ; GridItemVFX014BTests [1]
GridItemVFX014Policy [1] ; GridMON182Tests [1] ; GridMON188Tests [1] ; GridMON192LogicPrimitiveTests [1]
GridMON198Tests [1] ; GridMON202RecruitmentTests [1] ; GridMON203StoryCompanionTests [1] ; GridMON204RecruitmentUITests [1]
GridMapMON21610Tests [1] ; GridMapMON21611Tests [1] ; GridMapMON21612ArrivalRevealTests [1] ; GridMapMON21612Tests [1]
GridMapMON2163Tests [1] ; GridMapMON2164Tests [1] ; GridMapMON2166Tests [1] ; GridMapMON2167Tests [1]
GridMapMON2168Tests [1] ; GridMapMON2169Tests [1] ; GridMonsterMON13MakeLeverMarkerDefinition [1] ; GridMonsterMON178 [1]
GridMonsterMON178Death [1] ; GridMonsterMON178Persistence [1] ; GridMonsterMON179 [1] ; GridMonsterMON20102DeadRestoreOccupancy [1]
GridQuestMON212MakeDefinition [1] ; GridQuestMON2142 [1] ; GridQuestMON214Characterization [1] ; GridRPGPerf01Tests [1]
GridTD011Tests [1] ; GridTD012Tests [1] ; GridTD027MakeCursorItem [1] ; GridTD034MakeWalkableLevel [1]
GridTD052MakeDiagnosticsLevel [1] ; GridTD055GetObjectPropertyValue [1] ; GridTD062CreateInventory [1] ; GridTD062MakeEquipmentBinding [1]
GridTD062MakeQuickItemBinding [1] ; GridTD062MakeUniversalBinding [1] ; GridTD064CreateItem [1] ; GridTD064ValidateOwnership [1]
GridTD066CreateInventory [1] ; GridTD066CreateItem [1] ; GridTD066ValidateOwnership [1] ; GridTD068CreateDefinition [1]
GridTD068CreateInventory [1] ; GridTD068CreateItem [1] ; GridTD068MakeBinding [1] ; GridTD068PlaceInventoryItem [1]
GridTD0731SchemaAuditPrivate [1] ; GridTD073310Characterization [1] ; GridTD073310Normalization [1] ; GridTD07332Tests [1]
GridTD07333Characterization [1] ; GridTD07333Normalization [1] ; GridTD07334Characterization [1] ; GridTD07334Normalization [1]
GridTD07336Characterization [1] ; GridTD07336Normalization [1] ; GridTD07337Characterization [1] ; GridTD07337Normalization [1]
GridTD07338Characterization [1] ; GridTD07338Normalization [1] ; GridTD07342Normalization [1] ; GridTD07343Normalization [1]
GridTD07344Normalization [1] ; GridTD0734Characterization [1] ; GridTD07352Normalization [1] ; GridTD07353Normalization [1]
GridTD07354Normalization [1] ; GridTD0735Characterization [1] ; GridTD0736Characterization [1] ; GridTD0736Normalization [1]
GridTD0737Characterization [1] ; GridTD0737Normalization [1] ; GridTD0738StrictSchema [1] ; GridTD073ApprovedCanonicalLockAcceptance [6]
GridTD073ApprovedCanonicalStoneLoot [5] ; GridTD074ActivationCharacterization [1] ; GridTD075ReceptacleRecovery [1] ; GridWorldObjectMIG00Characterization [1]
GridWorldObjectMIG01 [1] ; GridWorldObjectMIG03RuntimeSpawn [1] ; HandleMON11RequestSelectedCharacterAttack [2] ; HandleMON5AbortCombat [2]
HandleMON5EndPlayerPhase [2] ; HandleMON5ForceVictory [2] ; HandleMON5LogTurnState [2] ; HandleMON5StartCombatFromPerception [2]
HandleMON5StartCombatWithAllMonsters [2] ; HasMON154ChoiceDependencyCycle [1] ; HasRPG0392ActedThisRound [1] ; HasRPG0392PhysicalControl [1]
InitializeMON9Movement [1] ; InstallMON135TransientPIEFixture [1] ; IsMON1285ClassActionSource [1] ; IsMON1286ExplicitTargetingPolicy [1]
IsMON1286TargetedSource [1] ; IsMON135EncounterSpawnId [1] ; IsMON154ClassValid [1] ; IsMON154LevelValid [1]
IsRPG0392RearArc [1] ; IsolateMON135EncounterFromUnrelatedMonsters [1] ; LoadMON176ProductionGoblin [1] ; LoadMON176ProductionStone [1]
LoadMON1891Save [1] ; LogGridTD0731SchemaAudit [1] ; LogMON5CommandResult [2] ; MON10AudioPitchSalt [1]
MON10AudioSeedSalt [1] ; MON10IdleDelaySalt [1] ; MON10IdleSeedSalt [1] ; MON10IdleVariationSalt [1]
MON10VFXSeedSalt [1] ; MON114PitchSeedSalt [1] ; MON114PresentationSeedSalt [1] ; MON11Source [1]
MON135EncounterId [1] ; MON135MapPath [1] ; MON135RatCell [1] ; MON135RatSpawnId [1]
MON135StartCell [1] ; MON135TriggerCell [1] ; MON135TriggerId [1] ; MON135Wave0SecondCell [1]
MON135Wave0SecondSpawnId [1] ; MON135Wave1Cell [1] ; MON135Wave1SpawnId [1] ; MON142HasErrorContaining [1]
MON142MakeDefinition [1] ; MON142MakeFloorLevel [1] ; MON142MakeSpawn [1] ; MON143MakeHorizontalRoute [1]
MON157Balance [1] ; MON161MakeDefinition [1] ; MON162MakeDefinition [1] ; MON163CreateMonster [1]
MON163CreateTurnManager [1] ; MON163MakeDefinition [1] ; MON164FindEntry [1] ; MON164MakeCharacter [1]
MON164MakeDefinition [1] ; MON164MakeEntry [1] ; MON165FindAction [1] ; MON165MakeCharacter [1]
MON165MakeControlDefinition [1] ; MON165MakePartyEntry [1] ; MON165MakeSelfClassAction [1] ; MON166MakeCharacter [1]
MON166MakeDefinition [1] ; MON166PartyEntry [1] ; MON168CountOccurrences [1] ; MON168ExtractSection [1]
MON168LoadProjectFile [1] ; MON168MakeDefinition [1] ; MON174AppendTurnsToward [1] ; MON174BuildValidFiringGoals [1]
MON174FindPathToFiringGoals [1] ; MON174HasLegalRangedShot [1] ; MON174MakeMoveAction [1] ; MON174MakeTurnAction [1]
MON174SelectRangedKeeperAttack [1] ; MON174TryPlanRangedKeeper [1] ; MON176GoblinExperienceReward [1] ; MON176GoblinId [1]
MON177Balance [1] ; MON177FinalBalance [1] ; MON195SourceId [1] ; MON198DoorMotionDuration [1]
MON8IndependentLootSeedSalt [1] ; MON8LootSeedSalt [1] ; MON9SingleLevelId [1] ; MakeMON10AudioDefinition [1]
MakeMON10AudioEvent [1] ; MakeMON10AudioParty [1] ; MakeMON10AudioSound [1] ; MakeMON10Definition [1]
MakeMON10IdleDefinition [1] ; MakeMON10IdleSavedState [1] ; MakeMON10IdleVariation [1] ; MakeMON10Monster [1]
MakeMON10Party [1] ; MakeMON10VFXAudioEvent [1] ; MakeMON10VFXDefinition [1] ; MakeMON10VFXEvent [1]
MakeMON10VFXParty [1] ; MakeMON11UnarmedProfile [1] ; MakeMON1285DirectSpell [1] ; MakeMON1285SelfAbility [1]
MakeMON1286AreaSpell [1] ; MakeMON1286CellSpell [1] ; MakeMON1286TargetingReason [1] ; MakeMON128EquipmentBinding [1]
MakeMON128UnarmedBinding [1] ; MakeMON12RestoredMonsterState [1] ; MakeMON13Definition [1] ; MakeMON13Level [1]
MakeMON13RuntimeDefinition [1] ; MakeMON13Spawn [1] ; MakeMON152Character [1] ; MakeMON152Class [1]
MakeMON152MonsterDefinition [1] ; MakeMON153Character [1] ; MakeMON153Class [1] ; MakeMON153Party [1]
MakeMON154CatalogContext [1] ; MakeMON154Class [1] ; MakeMON154EffectAction [1] ; MakeMON155Character [7]
MakeMON155ChoiceAction [1] ; MakeMON155Class [2] ; MakeMON155Inventory [16] ; MakeMON167Definition [1]
MakeMON167RuntimeState [1] ; MakeMON167SaveState [1] ; MakeMON171GoblinDefinition [1] ; MakeMON171Level [1]
MakeMON1741ThrowAttack [1] ; MakeMON176Character [1] ; MakeMON176Floor [1] ; MakeMON176Spawn [1]
MakeMON176SyntheticGoblin [1] ; MakeMON182Character [1] ; MakeMON182Spellbook [1] ; MakeMON187Spellbook [1]
MakeMON188Character [1] ; MakeMON188Party [1] ; MakeMON188SerializableParty [1] ; MakeMON188SpellBinding [1]
MakeMON1892AllyContext [1] ; MakeMON1892AllyRequest [1] ; MakeMON1892CasterResources [1] ; MakeMON1892Spellbook [1]
MakeMON1892StatusDefinition [1] ; MakeMON1892TurnState [1] ; MakeMON1971Event [1] ; MakeMON1971Host [1]
MakeMON1971Level [1] ; MakeMON1971Script [1] ; MakeMON197Script [1] ; MakeMON2062Character [1]
MakeMON2062Skill [1] ; MakeMON2063Character [1] ; MakeMON2063Skill [1] ; MakeMON2082Character [1]
MakeMON2082Grant [1] ; MakeMON2082Skill [1] ; MakeMON2083Action [1] ; MakeMON2083Character [1]
MakeMON2083Context [1] ; MakeMON2083Grant [1] ; MakeMON2083Skill [1] ; MakeMON2092Character [1]
MakeMON2092Id [1] ; MakeMON2092Skill [1] ; MakeMON2093Character [1] ; MakeMON2093Party [1]
MakeMON2093Skill [1] ; MakeMON2094Action [1] ; MakeMON2094Character [1] ; MakeMON2094Class [1]
MakeMON2094Context [1] ; MakeMON2094Grant [1] ; MakeMON2094Party [1] ; MakeMON2094Skill [1]
MakeMON2094Snapshot [1] ; MakeMON8LootEntry [1] ; MakeMON8MonsterDefinition [1] ; MakeMON9Definition [1]
MakeMON9Floor [1] ; MakeTD0132ItemSpawn [1] ; MakeTD0132Link [1] ; MakeTD0132WorldObject [1]
MakeTD013Link [1] ; MakeTD022EquippedItem [1] ; MakeTD0732Save [1] ; MakeTD07337Character [1]
MakeTD07337Facade [1] ; MakeTD07337Id [1] ; MakeTD07337NCharacter [1] ; MakeTD07337NFacade [1]
MakeTD07337SpellBinding [1] ; MakeTD07338Character [1] ; MakeTD07338Definition [1] ; MakeTD07338Id [1]
MakeTD07338NCharacter [1] ; MakeTD07338NDefinition [1] ; MakeTD07338NId [1] ; MakeTD07338NState [1]
MakeTD07338RuntimeState [1] ; MakeTD075Link [1] ; MatchesRPG0391MonsterTargetFilter [1] ; ProjectMON2094Requirements [1]
QuickSlot1 [9] ; QuickSlot2 [9] ; RPG031 [1] ; RPG0310 [1]
RPG0310PIE [1] ; RPG032 [1] ; RPG033 [1] ; RPG034 [1]
RPG035 [1] ; RPG036 [1] ; RPG037 [1] ; RPG038 [1]
RPG0390 [1] ; RPG0391Authoring [1] ; RPG0391Support [1] ; RPG0392Authoring [1]
RPG0393 [1] ; RPG0393Authoring [1] ; RPG0394E1 [1] ; RPG0394F1 [1]
RPG0394MageAuthoring [1] ; RPG0395 [1] ; RPG0395B1 [1] ; RPG0395B2 [1]
RPG0396 [1] ; RPG0396B1 [1] ; RPG0396B2 [1] ; RPGCustomRecruitMON205EventCommandTests [1]
RPGCustomRecruitMON205RuntimeIntegrationTests [1] ; RPGCustomRecruitMON205Tests [1] ; RPGCustomRecruitMON205WizardContextTests [1] ; RPGMON2074TalentPersistenceRegressionTests [1]
RPGMON2084SkillsPageReadModelTests [1] ; RPGMON2092Tests [1] ; RPGMON2093Tests [1] ; RPGMON2094Tests [1]
RPGStoryCompanionMON204EventCommandTests [1] ; RPGUIRPG013Tests [1] ; ReadMON187Hotbar [1] ; ResetMON12MonsterToFreshPlacedState [1]
ResetMON9CombatStateForPreBeginPlay [1] ; ResolveMON12AttackArmorEffects [1] ; ResolveMON12AttackStatusApplications [1] ; ResolveMON12ItemAttackProfile [1]
ResolveMON2092 [1] ; ResolveMON5TurnManager [2] ; ResolveMON8Quantity [1] ; ResolveRPG033CharacterArmorModifiers [1]
ResolveRPG038AttributeValue [1] ; RestoreMON2094Skill [1] ; SetMON2082Rank [1] ; SetMON2083Rank [1]
SetMON2094Rank [1] ; Slot0Id [1] ; Slot2Id [1] ; Slot4Id [1]
Slot5Id [1] ; SpawnMON10AudioMonster [1] ; SpawnMON10IdleMonster [1] ; SpawnMON10VFXMonster [1]
SpawnMON152Monster [1] ; SpawnMON152Party [1] ; SpawnMON176ManualGoblin [1] ; SpawnMON176Party [1]
SpawnMON8Monster [1] ; SpawnMON9DirectPlacedMonster [1] ; SpawnMON9Monster [1] ; SpawnMON9Party [1]
TD029YawMatches [1] ; TD075ReceptacleObjectId [1] ; TD075ReceptacleWorldObjectDefinitionId [1] ; TD075SourceObjectId [1]
TestTD073310CharacterizationDerivedStatsEqual [1] ; TestTD073310DerivedStatsEqual [1] ; TooltipClean01 [1] ; UGridArmClean064X64SaveCopyCommandlet [2]
UGridArmClean065RetirementAuditCommandlet [2] ; UGridArmClean06BaselineCommandlet [2] ; UIRPG014Production [1] ; UIRPG021Tests [1]
UIRPG022Production [1] ; UIRPG032Tests [1] ; UIRPG041Tests [1] ; UIRPG042Tests [1]
UIRPG043ATests [1] ; UIRPG044Tests [1] ; UIRPG061SkillCatalogAuthoringTests [1] ; UITooltipClean01 [1]
```

</details>

<details>
<summary>Legacy — 114 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
ApplyLegacyVisual [1] ; ConflictingLegacy [4] ; ContainsLegacyEnumValue [1] ; ContainsLegacyPrintfToken [1]
FARMCLEAN063BDLegacy [1] ; FARMEMENT01132Legacy [1] ; FARMEMENT015LegacyTest [1] ; FARMEMENT0162LegacyFallbackTest [1]
FARMEMENT0191LegacyCreationTest [1] ; FARMEMENT0192LegacyTest [1] ; FGridARMEMENT0110339NoLegacyBindings [1] ; FGridBECombatThrowLegacy [1]
FGridBFLegacy [1] ; FGridBGLevelAndLegacy [1] ; FGridClean063ASNoLegacyInventory [1] ; FGridClean063AVInvalidAndLegacy [1]
FGridClean064X10ELegacy [1] ; FGridClean065E4DLegacyLookup [1] ; FGridClean065H11LegacyActorGate [1] ; FGridEditorCppClean03BlueprintLegacyReferenceAuditTest [1]
FGridEditorWorldObjectMIG09ItemLegacyBlueprintReferenceAuditTest [1] ; FGridEditorWorldObjectMIG09MechanismLegacyBlueprintReferenceAuditTest [1] ; FGridLegacyInventoryRegistrationGate [3] ; FGridLegacyItemLookup [7]
FGridLegacyWorldActorInitGate [3] ; FGridMON128RejectLegacySaveWithoutHotbarTest [1] ; FGridTD012NoLegacyTorchFieldsTest [1] ; FGridTD0735LegacyItemAdapterRemovedTest [1]
FGridTD0736LegacyInfrastructureRemovedTest [1] ; FGridTD0736LegacyPlacementMirrorsTest [1] ; FGridTD0736LegacyRebuildModeTest [1] ; FGridTD0736LegacySymbolsAbsentTest [1]
FGridTD0738LegacySymbolsAbsentTest [1] ; FGridUIClean02NoGeneratedPaperDollLegacyTest [1] ; FGridUIClean03NoLegacyArmorAliasTest [1] ; FGridUIItem01Clean01NoLegacyTooltipProjectionTest [1]
FGridWeaponTooltip04NativeLegacyTest [1] ; FLegacyPackage [1] ; FOilExplorationLegacy [1] ; FRPGLEVELUX01NoSerializedLegacyReferenceTest [1]
FRPGUIRPG013LegacyMetadataBridgeTest [1] ; FUIRPGDESC01LegacyBindingsRemovedTest [1] ; FUITEXTCENTRAL012D2TalentLegacyPurgeTest [1] ; FUITEXTCENTRAL012D3LegacyMetadataTest [1]
FWeaponIndustrial05LegacyExceptions [1] ; FWeaponTooltipPassiveLegacyTest [1] ; FinishWithLegacy [1] ; ForbiddenLegacyTokens [1]
IdOnlyLegacy [1] ; ItemPlacementsUsingLegacyFallback [2] ; Legacy [43] ; LegacyAction [1]
LegacyActor [2] ; LegacyActual [1] ; LegacyAnchorPhrase [1] ; LegacyAnchorSharingName [1]
LegacyBonuses [1] ; LegacyBuilt [1] ; LegacyCapped [1] ; LegacyCategory [1]
LegacyCategoryCounts [1] ; LegacyCellSharingName [1] ; LegacyCount [1] ; LegacyD20 [1]
LegacyDamage [1] ; LegacyDefinition [19] ; LegacyExpected [1] ; LegacyFamilyRead [1]
LegacyField [1] ; LegacyFunctionNames [2] ; LegacyGate [1] ; LegacyIconPalettes [1]
LegacyId [1] ; LegacyInstance [1] ; LegacyItem [3] ; LegacyItemIdentity [1]
LegacyName [2] ; LegacyOnly [1] ; LegacyPackageName [1] ; LegacyPackages [1]
LegacyPotion [1] ; LegacyReferenceCount [1] ; LegacyReferences [3] ; LegacyResult [1]
LegacyRotation [1] ; LegacySpecific [1] ; LegacyStatus [1] ; LegacySuffixes [1]
LegacyTalentVariantPaths [1] ; LegacyTrigger [1] ; LegacyType [1] ; LegacyValidation [1]
LegacyView [3] ; LegacyVisualNames [1] ; LegacyWeapon [3] ; LegacyWeight [1]
MakeLegacy [1] ; MatchingLegacyArrow [1] ; MissingLegacyIds [1] ; NoLegacy [2]
PopulateLegacyStats [2] ; PresentLegacyStat [4] ; RealLegacy [1] ; SharedLegacyAction [4]
ShouldOmitLegacyArmor [5] ; WorldlessLegacy [1] ; bAllowLegacyFixture [2] ; bCanConsumeLegacy [5]
bHadLegacyPopup [1] ; bLegacy [4] ; bLegacyFallback [5] ; bLegacySpellbookAlly [1]
bMatchingLegacy [1] ; bShowLegacy [1]
```

</details>

<details>
<summary>Old / New (segments) — 211 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
AllOld [1] ; Button_NewGame [2] ; ClearPendingNewPartyState [4] ; CloseNewGameCharacterCreation [2]
CompletePendingNewGameDungeonBuild [4] ; ConflictingOld [1] ; ConsumePendingNewPartyState [5] ; CountedOldSize [1]
CreateNewWorldContext [222] ; EventNewLevel [3] ; FARMEMENT01163NewGameDisarmSave [1] ; FARMEMENTLOT022NewGame [1]
FGridMonsterMON9DirectPlacedNewGameStartsAliveTest [1] ; FNewLineData [1] ; FRPGMON205WizardNewGameSubmitRegressionTest [1] ; GridPartyInventoryDiagnosticsIsNewPaperDollSlot [1]
HandleNewGameClicked [2] ; HasPendingNewPartyState [3] ; InitialOld [2] ; InitializeNewStairsDefinition [1]
InvalidOld [1] ; IsNewElixirAction [1] ; IsNewGameDungeonBuildPending [6] ; LockOverrideOld [1]
LooseOld [1] ; MakeNewGameParty [1] ; MonsterOld [1] ; New [1]
NewAsset [1] ; NewAssets [1] ; NewBehavior [2] ; NewBoundary [1]
NewBrush [1] ; NewCellX [5] ; NewCellY [5] ; NewCharacter [3]
NewCharacterIndex [1] ; NewClass [1] ; NewClassTagged [1] ; NewConfig [2]
NewCursor [2] ; NewDefinition [2] ; NewDefinitionClass [1] ; NewDirectory [1]
NewEncounterGroupId [2] ; NewEncounterWaveIndex [2] ; NewEntry [4] ; NewEquipmentBinding [1]
NewExperience [2] ; NewFacing [2] ; NewFloorOffset [1] ; NewGame [8]
NewGameCharacterCreationWidget [2] ; NewGameMainHero [6] ; NewGamePartyInventory [2] ; NewGender [2]
NewGuid [389] ; NewHealth [1] ; NewId [5] ; NewIndex [5]
NewInitialMonsterState [2] ; NewInventory [6] ; NewItem [1] ; NewItemDefinitionId [2]
NewItemLevel [2] ; NewLevel [5] ; NewLevelAsset [1] ; NewLevelId [4]
NewLink [1] ; NewLocation [1] ; NewLogicId [1] ; NewMaximumHealth [2]
NewMaximumMana [2] ; NewMesh [1] ; NewMode [4] ; NewMonster [1]
NewMonsterDefinitionAsset [2] ; NewMonsterDefinitionId [2] ; NewNotes [2] ; NewObject [522]
NewObjectType [2] ; NewOilFire [1] ; NewPackage [3] ; NewPaperDollSlotUsageCount [1]
NewParty [1] ; NewPartyState [3] ; NewPath [1] ; NewPhase [8]
NewPhysicalLevel [4] ; NewPoolIndex [1] ; NewPotionMaterial [1] ; NewPtr [1]
NewQuickItemBinding [1] ; NewRank [10] ; NewReadTextOverride [2] ; NewReadTitleOverride [2]
NewReadableContentAsset [2] ; NewReadableContentId [2] ; NewReadableText [2] ; NewResources [1]
NewRule [1] ; NewScript [1] ; NewScriptId [4] ; NewSize [1]
NewSlot [2] ; NewStack [1] ; NewState [6] ; NewStats [2]
NewStep [2] ; NewTab [2] ; NewText [8] ; NewTorchNiagara [1]
NewTransform [1] ; NewValue [8] ; NewVariables [2] ; NewWaitSeconds [2]
NewWaypoint [1] ; NewWorldObjectDefinitionId [3] ; NewYaw [1] ; NoOld [2]
NoOldFacts [1] ; Old [36] ; OldAmmo [1] ; OldArrow [1]
OldArrows [1] ; OldAssets [1] ; OldBackedEntry [1] ; OldCellX [4]
OldCellY [4] ; OldClassTagged [1] ; OldDefault [1] ; OldDefinition [3]
OldDefinitionClass [1] ; OldDefinitions [1] ; OldDirectory [1] ; OldEquipmentBinding [1]
OldIndex [1] ; OldInventoryIcon [1] ; OldKey [1] ; OldKnown [1]
OldLevelId [1] ; OldLight [1] ; OldObject [2] ; OldOil [1]
OldPackage [2] ; OldPath [4] ; OldPaths [2] ; OldPointers [1]
OldPolicy [1] ; OldProperty [1] ; OldQuickItemBinding [1] ; OldScriptId [4]
OldShield [1] ; OldSize [1] ; OldSlot [1] ; OldSlots [1]
OldStack [1] ; OldSword [2] ; OldTargetId [1] ; OldTorch [1]
OldTorchNiagara [1] ; OldUnknown [2] ; OldWeapon [1] ; OldWeaponDefinitions [1]
OldWeapons [1] ; OldX [1] ; OldY [1] ; OnNewGameRequested [1]
OpenNewGameCharacterCreation [2] ; PaletteOld [1] ; PendingNewPartyState [2] ; ReceptacleOverrideOld [1]
ResetPartyForNewGame [7] ; RestorePendingNewGameParty [3] ; SAssignNew [6] ; SNew [19]
SafeNewMaximum [2] ; SetPendingNewPartyState [5] ; StartNewGame [2] ; SuppliedOld [1]
TransientOld [1] ; TryAdvanceNewGameRuntimeBuild [2] ; WrongOld [1] ; bHasPendingNewPartyState [2]
bNeedsNewHeldVisual [1] ; bNew [3] ; bNewAction [1] ; bNewActive [1]
bNewCanInsertItems [2] ; bNewCanRemoveItem [2] ; bNewDefeat [1] ; bNewGameBuildPreparationPresented [2]
bNewGameBuildRuntimeReady [2] ; bNewInitiallyActive [2] ; bNewInitiallyEnabled [2] ; bNewItemDefinition [1]
bNewObjectOnEdge [2] ; bNewOn [2] ; bNewPressed [2] ; bNewTorch [1]
bOld [1] ; bOldAction [1] ; bOldDirty [1] ; bOldOptIn [1]
bOldSourcesPresent [1] ; bOldTorch [1] ; bPendingNewGameDungeonBuild [2] ; bPotionOld [1]
bStartsNewRound [5] ; bWaitingForNewGameRuntimeBuild [2] ; new [2]
```

</details>

<details>
<summary>Stage — 11 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
EGridSpellCastPipelineRejectStage [6] ; FCompactStage [1] ; OutRejectStage [2] ; PipelineRejectStage [4]
RejectStage [1] ; RunStage [1] ; Stage [4] ; StageIndex [1]
Staged [1] ; Stages [1] ; bFinalStage [1]
```

</details>

<details>
<summary>Temporary — 6 identifiants distincts</summary>

Chaque entrée donne le nombre de fichiers utilisateurs entre crochets ; ce n’est pas un nombre d’appels. Le manifeste C donne les chemins complets ; rechercher le symbole pour ses sites exacts.

```text
Temporary [1] ; TemporaryCursorItem [1] ; TemporaryRuntimeId [1] ; TemporarySaveBytesBeforeFreshPIE [1]
TemporarySaveSlot [1] ; bTemporarySaveExists [1]
```

</details>

### C. Manifeste complet — 4 299 entrées Git
Toutes les entrées du parent sont affectées une seule fois aux cinq groupes suivants. Les noms listés sont relatifs au répertoire indiqué ; l’extension et la casse sont conservées. Les comptes n’incluent pas les trois livrables CLEANUP01. Pour les documents, la classe entre crochets est un tri initial fondé sur le chemin/titre : les constats d’obsolescence prouvés sont dans la section 6, pas déduits automatiquement de cette classe.

<details>
<summary>C++ et règles Source/ — 1445 entrées</summary>


**`Source/` — 2**

```text
GrimrockPrototype.Target.cs
GrimrockPrototypeEditor.Target.cs
```

**`Source/GrimrockLua/` — 1**

```text
GrimrockLua.Build.cs
```

**`Source/GrimrockLua/Private/` — 3**

```text
GridLuaVm.cpp
GrimrockLua.cpp
Lua54.cpp
```

**`Source/GrimrockLua/Private/Tests/` — 4**

```text
GridLuaMON1971PersistentAuthoringTests.cpp
GridLuaMON197SandboxPackagingTests.cpp
GridLuaPUZZLE01Lua01Tests.cpp
GridLuaVmTests.cpp
```

**`Source/GrimrockLua/Public/` — 2**

```text
GridLuaScriptTypes.h
GridLuaVm.h
```

**`Source/GrimrockPrototype/` — 2**

```text
GrimrockPrototype.Build.cs
GrimrockPrototype.cpp
```

**`Source/GrimrockPrototype/Private/Core/` — 4**

```text
GridDungeonAsset.cpp
GridLevelAsset.cpp
GridObjectPaletteAsset.cpp
GridWorldObjectDefinitionAsset.cpp
```

**`Source/GrimrockPrototype/Private/Magic/` — 11**

```text
GridPartySpellbookComponent.cpp
GridProductionSpellLibrary.cpp
GridSpellCastTransaction.cpp
GridSpellDefinitionAsset.cpp
GridSpellEffectResolver.cpp
GridSpellHotbarExecution.cpp
GridSpellPresentation.cpp
GridSpellPresentationComponent.cpp
GridSpellTargeting.cpp
GridSpellbookPersistence.cpp
GridSpellbookUI.cpp
```

**`Source/GrimrockPrototype/Private/Quests/` — 2**

```text
GridQuestDefinitionAsset.cpp
GridQuestSubsystem.cpp
```

**`Source/GrimrockPrototype/Private/RPG/` — 29**

```text
GridStartingKitEquipmentPolicyReadModel.h
GridStartingKitStaticMetadataReadModel.h
RPGAttributePointService.cpp
RPGAuthoringIdentityResolver.cpp
RPGCharacterIdentityPersistence.cpp
RPGCharacterPortraitSetAsset.cpp
RPGCharacterRulesLibrary.cpp
RPGClassAsset.cpp
RPGClassProgressionService.cpp
RPGClassProgressionTransactionService.cpp
RPGClassVisualAsset.cpp
RPGCustomRecruitService.cpp
RPGExperienceRewardService.cpp
RPGLevelUpService.cpp
RPGPartyProgressionResolver.cpp
RPGPartyRecruitmentService.cpp
RPGProgressionPIESimulator.cpp
RPGRaceAsset.cpp
RPGRecipeCatalogAsset.cpp
RPGSkillAsset.cpp
RPGSkillCheckService.cpp
RPGSkillPersistence.cpp
RPGSkillPointService.cpp
RPGSkillRequirementProjectionService.cpp
RPGSkillService.cpp
RPGStartingEquipmentPresetCatalog.cpp
RPGStartingEquipmentService.cpp
RPGStoryCompanionAsset.cpp
RPGStoryCompanionService.cpp
```

**`Source/GrimrockPrototype/Private/RPG/StatusEffects/` — 9**

```text
GridCombatStatusApplicationResolver.cpp
GridStatusEffectControlResolver.cpp
GridStatusEffectDefinitionAsset.cpp
GridStatusEffectInitiativeResolver.cpp
GridStatusEffectLifecycleSubsystem.cpp
GridStatusEffectPeriodicDamageResolver.cpp
GridStatusEffectPersistence.cpp
GridStatusEffectPresentation.cpp
GridStatusEffectTypes.cpp
```

**`Source/GrimrockPrototype/Private/Runtime/` — 103**

```text
GridActivationComponent.cpp
GridActivationComponentLinks.cpp
GridActivationComponentLua.cpp
GridButtonActor.cpp
GridCanonicalEquipmentSlotsReadModel.h
GridCanonicalExplorationItemReadModel.h
GridCanonicalInventoryDefinitionDiagnosticReadModel.h
GridCanonicalInventoryStackReadModel.h
GridCanonicalItemInteractionReadModel.h
GridCanonicalItemMeshReadModel.h
GridCanonicalItemSpawnReadModel.h
GridCanonicalProjectileVisualReadModel.h
GridCanonicalQuickItemUseReadModel.h
GridCanonicalThrowBallisticsReadModel.h
GridDoorActor.cpp
GridDoorSystemComponent.cpp
GridDungeonRuntimeState.cpp
GridDungeonTrapPerceptionService.cpp
GridEditorLooseItemPreviewReadModel.h
GridEditorPreviewComponent.cpp
GridEditorPreviewObjectActor.cpp
GridEquipmentArmorReadModel.h
GridEquipmentContributionReadModel.h
GridEquipmentPreviewItemReadModel.h
GridEquipmentSlotUtils.h
GridEventFeedPublisher.cpp
GridEventFeedSubsystem.cpp
GridEventFeedTypes.cpp
GridGenericObjectActor.cpp
GridInteractionUtils.cpp
GridInventoryDiagnosticWeightReadModel.h
GridItemActor.cpp
GridItemContextActionLibrary.cpp
GridItemDefinitionAsset.cpp
GridItemInstanceMaterializationReadModel.h
GridItemTransferService.cpp
GridItemVisualEffectsComponent.cpp
GridItemWeightReadModel.h
GridLegacyInventoryRegistrationGate.h
GridLegacyItemLookup.h
GridLegacyWorldActorInitGate.h
GridLevelRuntimeActor.cpp
GridLevelRuntimeActorDiagnostics.cpp
GridLevelRuntimeActorFeedbackUI.cpp
GridLevelRuntimeActorMap.cpp
GridLevelRuntimeActorMonsters.cpp
GridLevelRuntimeActorMonstersTyped.cpp
GridLevelRuntimeActorPersistence.cpp
GridLevelRuntimeActorSurfaces.cpp
GridLevelRuntimeActorTrapDamage.cpp
GridLevelRuntimeActorTrapPerception.cpp
GridLevelRuntimeActorTraps.cpp
GridLevelRuntimeActorWorldItems.cpp
GridLevelVariableStore.cpp
GridLeverActor.cpp
GridLightEmitterComponent.cpp
GridLogicRuntime.cpp
GridMechanismActor.cpp
GridMonsterEncounterComponent.cpp
GridPIEPlaytestRequest.cpp
GridPartyIlluminationComponent.cpp
GridPartyInventoryComponent.cpp
GridPartyInventoryComponentCursorTransfer.cpp
GridPartyInventoryComponentDiagnostics.cpp
GridPartyInventoryComponentEquipment.cpp
GridPartyInventoryComponentEquipmentPreview.cpp
GridPartyInventoryComponentHotbar.cpp
GridPartyInventoryComponentVisuals.cpp
GridPartyInventoryComponentWorldTransfer.cpp
GridPartyProgressionReminderState.cpp
GridPhysicalThrowPreflightReadModel.h
GridPhysicalThrowStrengthReadModel.h
GridPitTrapdoorActor.cpp
GridPlacementTransformResolver.cpp
GridPressurePlateActor.cpp
GridReadableContentAsset.cpp
GridReceptacleAcceptedCanonicalReadModel.h
GridReceptacleActor.cpp
GridReceptacleInitialCanonicalReadModel.h
GridReceptacleItemMetadataReadModel.h
GridRuntimeObjectActor.cpp
GridSecretDoorActor.cpp
GridThrownItemActor.cpp
GridTriggerActor.cpp
GridWallLockAcceptedKeyReadModel.h
GridWallLockActor.cpp
GridWorldItemLevelReadModel.h
GridWorldPhysicsMassReadModel.h
GridWorldPickupIdentityReadModel.h
GrimrockGameInstance.cpp
GrimrockGameMode.cpp
GrimrockPartyPawn.cpp
GrimrockPartyPawnCustomRecruit.cpp
GrimrockPartyPawnHeldItem.cpp
GrimrockPartyPawnInputBuffer.cpp
GrimrockPartyPawnInventoryEvents.cpp
GrimrockPartyPawnItemTransfer.cpp
GrimrockPartyPawnMovement.cpp
GrimrockPartyPawnRecruitment.cpp
GrimrockPartyPawnSave.cpp
GrimrockPartyPawnUI.cpp
GrimrockPlayerController.cpp
GrimrockStartupModeComponent.cpp
```

**`Source/GrimrockPrototype/Private/Runtime/Combat/` — 39**

```text
GridCanonicalActionIconReadModel.h
GridCanonicalEquippedAttackReadModel.h
GridCatalystAreaResolver.cpp
GridCombatActionCatalog.cpp
GridCombatArmorEffectResolver.cpp
GridCombatDiagnostics.cpp
GridCombatEventFeedFormatter.cpp
GridCombatLog.cpp
GridCombatModifierResolver.cpp
GridCombatMovementResolver.cpp
GridCombatPhysicalThrowSourceReadModel.h
GridCombatProjectileActor.cpp
GridCombatReactionResolver.cpp
GridCombatResolver.cpp
GridCombatSurfaceResolver.cpp
GridCombatTargetingResolver.cpp
GridDualWieldPolicy.cpp
GridEquippedGuardItemReadModel.h
GridEquippedGuardResolver.cpp
GridPlayerAttackPresentationComponent.cpp
GridPlayerAttackPresentationTypes.cpp
GridQuickItemResolver.cpp
GridRangedAmmunitionPolicy.cpp
GridTurnManagerActions.cpp
GridTurnManagerComponent.cpp
GridTurnManagerInitiative.cpp
GridTurnManagerPartyMovement.cpp
GridTurnManagerPhases.cpp
GridTurnManagerPlayerActionCatalog.cpp
GridTurnManagerPlayerActions.cpp
GridTurnManagerPlayerTurns.cpp
GridTurnManagerReactions.cpp
GridTurnManagerSurfaces.cpp
GridTurnManagerTypes.cpp
GridWeaponCatalogueAsset.cpp
GridWeaponCatalogueCompact.cpp
GridWeaponVisualCatalogueAsset.cpp
GridWeaponVisualResolver.cpp
GridWorldItemVisualResolver.cpp
```

**`Source/GrimrockPrototype/Private/Runtime/Items/` — 4**

```text
GridCanonicalItemCatalogues.cpp
GridCanonicalItemDefinitionAsset.cpp
GridItemCacheParityAudit.cpp
GridItemSubsystem.cpp
```

**`Source/GrimrockPrototype/Private/Runtime/Map/` — 2**

```text
GridMapReadModel.cpp
GridMapRevealService.cpp
```

**`Source/GrimrockPrototype/Private/Runtime/Monsters/` — 24**

```text
GridAutomaticPerceptionEngagementSubsystem.cpp
GridLootStaticMetadataReadModel.h
GridMonsterActor.cpp
GridMonsterAudioComponent.cpp
GridMonsterBalanceTypes.cpp
GridMonsterBehaviorComponent.cpp
GridMonsterCategoryAsset.cpp
GridMonsterCombatComponent.cpp
GridMonsterDeathComponent.cpp
GridMonsterDeathDissolve.cpp
GridMonsterDefinitionAsset.cpp
GridMonsterFastHarasserPlanner.cpp
GridMonsterHurtPresentation.cpp
GridMonsterIdleVariationComponent.cpp
GridMonsterLootResolver.cpp
GridMonsterMovementComponent.cpp
GridMonsterOccupancySubsystem.cpp
GridMonsterPathfinder.cpp
GridMonsterPatrolSubsystem.cpp
GridMonsterPatrolSubsystemAlarm.cpp
GridMonsterRangedAttackPlanner.cpp
GridMonsterRangedKeeperPlanner.cpp
GridMonsterSpawnConfiguration.cpp
GridMonsterVFXComponent.cpp
```

**`Source/GrimrockPrototype/Private/Save/` — 3**

```text
GridCombatSavePolicy.cpp
GrimrockPartySaveGame.cpp
GrimrockSaveSlotLibrary.cpp
```

**`Source/GrimrockPrototype/Private/Tests/` — 478**

```text
GridARMCLEAN061ItemSystemTests.cpp
GridARMCLEAN062CanonicalHandParityTests.cpp
GridARMCLEAN062ClassAffinityTests.cpp
GridARMCLEAN062HandActionTests.cpp
GridARMCLEAN062PresentationTests.cpp
GridARMCLEAN063AAWeaponTooltipTests.cpp
GridARMCLEAN063ABGuardIdentityTests.cpp
GridARMCLEAN063ACInventoryDiagnosticsTests.cpp
GridARMCLEAN063ADEquipmentContributionsTests.cpp
GridARMCLEAN063AEEquipmentPreviewTests.cpp
GridARMCLEAN063AFStartingKitDisplayTests.cpp
GridARMCLEAN063AGItemInstanceMaterializationTests.cpp
GridARMCLEAN063AHLootLevelResolutionTests.cpp
GridARMCLEAN063AIReceptacleMaterializationTests.cpp
GridARMCLEAN063AJWorldItemLevelTests.cpp
GridARMCLEAN063ALActionIconTests.cpp
GridARMCLEAN063AMStartingKitLevelsTests.cpp
GridARMCLEAN063AOStartingKitEquipmentTests.cpp
GridARMCLEAN063APDualWieldTests.cpp
GridARMCLEAN063AQEquipmentSlotsTests.cpp
GridARMCLEAN063ARCombatHudIconsTests.cpp
GridARMCLEAN063ASInventoryMaterializationTests.cpp
GridARMCLEAN063ATSavedItemRehydrationTests.cpp
GridARMCLEAN063AUInventoryTypesTests.cpp
GridARMCLEAN063AVCanonicalSpecializedThrowTests.cpp
GridARMCLEAN063AWItemInteractionsTests.cpp
GridARMCLEAN063AXCanonicalUseTests.cpp
GridARMCLEAN063AYCanonicalStatusRemovalTests.cpp
GridARMCLEAN063AZCanonicalStatusApplicationTests.cpp
GridARMCLEAN063BACanonicalAllyUseTests.cpp
GridARMCLEAN063BBCanonicalRangedAmmoTests.cpp
GridARMCLEAN063BCCanonicalAmmoPersistenceTests.cpp
GridARMCLEAN063BDCanonicalWorldPickupIdentityTests.cpp
GridARMCLEAN063BECombatPhysicalThrowTests.cpp
GridARMCLEAN063BFPhysicalThrowPreflightTests.cpp
GridARMCLEAN063BGReceptacleIdentityTests.cpp
GridARMCLEAN063HArmorRuntimeTests.cpp
GridARMCLEAN063IWeightRuntimeTests.cpp
GridARMCLEAN063JInventoryVisualTests.cpp
GridARMCLEAN063KInventoryMetadataTests.cpp
GridARMCLEAN063LHeldVisualTests.cpp
GridARMCLEAN063MWorldVisualTests.cpp
GridARMCLEAN063NWorldWeightTests.cpp
GridARMCLEAN063OInventorySortTests.cpp
GridARMCLEAN063PFreshReceptacleVisualTests.cpp
GridARMCLEAN063QWorldMassTests.cpp
GridARMCLEAN063RProjectileVisualTests.cpp
GridARMCLEAN063TThrowBallisticsTests.cpp
GridARMCLEAN063UPhysicalThrowStrengthTests.cpp
GridARMCLEAN063VUnlevelledAmmoTests.cpp
GridARMCLEAN063WCanonicalAmmoFlightMeshTests.cpp
GridARMCLEAN063XLootStaticMetadataTests.cpp
GridARMCLEAN063YStartingKitMetadataTests.cpp
GridARMCLEAN063ZEditorLooseItemPreviewTests.cpp
GridARMCLEAN064X10CanonicalItemSpawnsTests.cpp
GridARMCLEAN064X161ReceptacleRuleReaderTests.cpp
GridARMCLEAN064X169SaveWorldRoundTripTests.cpp
GridARMCLEAN064X17TorchNonArmEquipmentTests.cpp
GridARMCLEAN064X18TorchFloorLightTests.cpp
GridARMCLEAN064X20ArmWorldPresentationTests.cpp
GridARMCLEAN064X21PointerlessReceptacleTests.cpp
GridARMCLEAN064X24PointerlessStartingKitTests.cpp
GridARMCLEAN064X25PointerlessEditorPreviewTests.cpp
GridARMCLEAN064X29CanonicalWallLockKeysTests.cpp
GridARMCLEAN064X30PointerlessLockVisualTests.cpp
GridARMCLEAN064X31ReceptacleTypeTests.cpp
GridARMCLEAN064X32ReceptaclePhysicalPreflightTests.cpp
GridARMCLEAN064X6CanonicalPresentationProjectionTests.cpp
GridARMCLEAN064X7CanonicalActorInitializationTests.cpp
GridARMCLEAN064X8RealLooseItemCutoverTests.cpp
GridARMCLEAN064X91ReceptacleInitialIdentityTests.cpp
GridARMCLEAN064X92ReceptacleCanonicalVisualTests.cpp
GridARMCLEAN065ExplorationItemTests.cpp
GridARMCLEAN065H10LegacyRegistrationGateTests.cpp
GridARMCLEAN065H11WorldActorInitGateTests.cpp
GridARMCLEAN065H3ApprovedStarterGateTests.cpp
GridARMCLEAN065H6EditorStarterParityTests.cpp
GridARMEMENT011031EquipmentArmorTests.cpp
GridARMEMENT011033CombatTooltipTests.cpp
GridARMEMENT01132ApprovedStartingKitsTests.cpp
GridARMEMENT01141RangerStartingAmmunitionTests.cpp
GridARMEMENT01151ContextMenuTests.cpp
GridARMEMENT01151SelfConsumeTests.cpp
GridARMEMENT01152ExplorationHotbarTests.cpp
GridARMEMENT01162ADisarmTriggerTests.cpp
GridARMEMENT01162BDisarmContextTests.cpp
GridARMEMENT01163SaboteurStarterTests.cpp
GridARMEMENT011713OilCombatTests.cpp
GridARMEMENT011714OilSurfaceVisualTests.cpp
GridARMEMENT01171TransmuterOilTests.cpp
GridARMEMENT01172SurfaceCatalystKitTests.cpp
GridARMEMENT01173AreaTests.cpp
GridARMEMENT01174CInstanceNiagaraTests.cpp
GridARMEMENT01174CatalystFeedTests.cpp
GridARMEMENT013MultiDiceTests.cpp
GridARMEMENT014CatalogueTests.cpp
GridARMEMENT015FixedItemLevelsTests.cpp
GridARMEMENT0161VisualCatalogueTests.cpp
GridARMEMENT0162WeaponVisualResolverTests.cpp
GridARMEMENT0163PilotAssetTests.cpp
GridARMEMENT0164UiPresentationTests.cpp
GridARMEMENT0165WorldVisualTests.cpp
GridARMEMENT0166PilotItemAssetTests.cpp
GridARMEMENT0172TwoHandedEquipmentTests.cpp
GridARMEMENT0173TwoHandedContextActionsTests.cpp
GridARMEMENT0183ReceptacleItemLevelTests.cpp
GridARMEMENT0184MonsterLootLevelsTests.cpp
GridARMEMENT0191StartingEquipmentTests.cpp
GridARMEMENT0192SaveRehydrationTests.cpp
GridARMEMENT0192SixClassPresetsTests.cpp
GridARMEMENT0193LocalizationTests.cpp
GridARMEMENT0193StartingKitWizardTests.cpp
GridARMEMENT01952WeaponAffinityTests.cpp
GridARMEMENT0197KitAffinityTests.cpp
GridARMEMENTCLEAN03CompactTests.cpp
GridARMEMENTLOT02193GuardTests.cpp
GridARMEMENTLOT024AmmunitionTests.cpp
GridAutomationSaveIsolationTests.cpp
GridCanonicalLootTestScope.h
GridCanonicalWeightTestWorld.h
GridCeilingOverride01Tests.cpp
GridDoorAudioFeedbackTests.cpp
GridDoorPassageBlockingTests.cpp
GridDoorTestUtils.h
GridEditorPaletteIcon01Tests.cpp
GridEquipmentPreview043ATests.cpp
GridEquipmentTooltip043BTests.cpp
GridEquipmentTooltip043DTests.cpp
GridFireAudioClustersTests.cpp
GridFireProductionAssetWiringTests.cpp
GridFlask04ProductionMeshesTests.cpp
GridFlaskBallisticsTests.cpp
GridFlaskGlass01Tests.cpp
GridHotbarContext01Tests.cpp
GridIssue181ChaosWorldItemsTests.cpp
GridItemLIGHT01DataDrivenEmitterTests.cpp
GridItemLIGHT02CanonicalStateTests.cpp
GridItemLIGHT03PhysicsMeshEmitterTests.cpp
GridItemThrowCLEAN01Tests.cpp
GridItemVFX011Tests.cpp
GridItemVFX012StackLifecycleTests.cpp
GridItemVFX013TorchRuntimeTests.cpp
GridItemVFX014BLegacyRemovalTests.cpp
GridItemWorldPhysicsTests.cpp
GridItemWorldSparkleTests.cpp
GridLevelFade01PersistenceTests.cpp
GridLevelMON192VariableTests.cpp
GridLightCONFIG02SingleAuthorityTests.cpp
GridMON192LogicPrimitiveTests.cpp
GridMON194LuaBridgeTests.cpp
GridMON195LuaPersistenceTests.cpp
GridMON1971LuaLogicIdTests.cpp
GridMON198ProductionPuzzleTests.cpp
GridMagicMON181SpellContractTests.cpp
GridMagicMON182SpellbookTests.cpp
GridMagicMON183CastTransactionTests.cpp
GridMagicMON184TargetingTests.cpp
GridMagicMON185ProductionSpellTests.cpp
GridMagicMON186SpellPresentationTests.cpp
GridMagicMON187SpellbookUITests.cpp
GridMagicMON188SpellbookPersistenceTests.cpp
GridMagicMON1892SpellBalanceRegressionTests.cpp
GridMagicMON1893FinalDiagnosticsTests.cpp
GridMagicUI0143e2SpellExecutionTests.cpp
GridMagicUI0143eSpellCatalogTests.cpp
GridMapART01HandDrawnPolishTests.cpp
GridMapFLOOR01TexturedFloorTests.cpp
GridMapInput01MovementLockTests.cpp
GridMapMON21610ViewNavigationTests.cpp
GridMapMON21611ArtStyleTests.cpp
GridMapMON21612ArrivalRevealTests.cpp
GridMapMON21612SymbolsTests.cpp
GridMapMON21613ClosureTests.cpp
GridMapMON2162ExplorationStateTests.cpp
GridMapMON2163RevealTests.cpp
GridMapMON2164SecretDiscoveryTests.cpp
GridMapMON2165PersistenceTests.cpp
GridMapMON2166ReadModelTests.cpp
GridMapMON2167FloorProjectionTests.cpp
GridMapMON2168NativeRenderingTests.cpp
GridMapMON2169FloorNavigationTests.cpp
GridMapTHEME01TexturedRenderingTests.cpp
GridMapUI01StandaloneWindowTests.cpp
GridMapUI02TitleBarTests.cpp
GridMapUI03FloorNavigationOverlayTests.cpp
GridMaterialOwnershipAuditTests.cpp
GridMonsterAcousticHearingTests.cpp
GridMonsterMON10AudioTests.cpp
GridMonsterMON10IdleVariationTests.cpp
GridMonsterMON10OptimizationTests.cpp
GridMonsterMON10Tests.cpp
GridMonsterMON10VFXTests.cpp
GridMonsterMON1141ThrownWeaponTests.cpp
GridMonsterMON11EquipmentTests.cpp
GridMonsterMON11PresentationTests.cpp
GridMonsterMON11ResolutionTests.cpp
GridMonsterMON11Tests.cpp
GridMonsterMON128HotbarTests.cpp
GridMonsterMON12CombatActionPanelTests.cpp
GridMonsterMON12CombatHudTests.cpp
GridMonsterMON13SpawnTests.cpp
GridMonsterMON14_1Tests.cpp
GridMonsterMON14_2Tests.cpp
GridMonsterMON14_3Tests.cpp
GridMonsterMON14_4Tests.cpp
GridMonsterMON171DefinitionSpawnTests.cpp
GridMonsterMON172PresentationTests.cpp
GridMonsterMON1731RangedAttackTests.cpp
GridMonsterMON1732ProjectilePresentationTests.cpp
GridMonsterMON1733ThrowPresentationTests.cpp
GridMonsterMON1734CooldownTests.cpp
GridMonsterMON1741RangedKeeperTests.cpp
GridMonsterMON1751ExplorationIntegrationTests.cpp
GridMonsterMON1761EncounterRewardIntegrationTests.cpp
GridMonsterMON177BalanceTests.cpp
GridMonsterMON177FinalBalanceContractTests.cpp
GridMonsterMON178DeathPresentationTests.cpp
GridMonsterMON178PersistenceTests.cpp
GridMonsterMON178PresentationTests.cpp
GridMonsterMON179HurtPresentationTests.cpp
GridMonsterMON1Tests.cpp
GridMonsterMON20102DeadRestoreOccupancyTests.cpp
GridMonsterMON3Tests.cpp
GridMonsterMON4Tests.cpp
GridMonsterMON5Tests.cpp
GridMonsterMON6Tests.cpp
GridMonsterMON7Tests.cpp
GridMonsterMON8Tests.cpp
GridMonsterMON9Tests.cpp
GridMonsterPitTraversalTests.cpp
GridObjectGenericAudioTests.cpp
GridPUZZLE01Lua01RuntimeMaterialTests.cpp
GridPUZZLE01Lua02GuardianDoorRuntimeTests.cpp
GridPartyBlockedMovementFeedbackTests.cpp
GridPartyInventoryCC0Tests.cpp
GridPartyLIGHT01EquipmentIlluminationTests.cpp
GridPartyWeightMovementTests.cpp
GridPerfProfile01InstrumentationContractTests.cpp
GridPitPIT01Tests.cpp
GridPitPIT02Tests.cpp
GridPitPIT031Tests.cpp
GridPitPIT03Tests.cpp
GridPressurePlateMonsterActivationTests.cpp
GridQuestMON212FoundationTests.cpp
GridQuestMON213EventCommandTests.cpp
GridQuestMON2142PersistenceSchemaTests.cpp
GridQuestMON214PersistenceCharacterizationTests.cpp
GridRPGPerf01ReadModelTests.cpp
GridRPGShield022Tests.cpp
GridRPGShield023RestTests.cpp
GridRPGShield024ASurfaceTests.cpp
GridRPGShield024BRestoreTests.cpp
GridRPGShield025SaveTests.cpp
GridSaveMON1891CombatSavePolicyTests.cpp
GridSaveSAVEFIX2Tests.cpp
GridSemanticInitialStateTests.cpp
GridSemanticObjectAudioRoutingTests.cpp
GridSkillsStandaloneWindowTests.cpp
GridTD011ReceptaclePersistenceTests.cpp
GridTD012PartySelectionHeldVisualTests.cpp
GridTD0132EventCommandRuntimeHardeningTests.cpp
GridTD021WorldItemsContractTests.cpp
GridTD022PartyInventoryWorldTransferTests.cpp
GridTD023PartyInventoryDiagnosticsTests.cpp
GridTD027PartyItemTransferTests.cpp
GridTD028PartyUIFacadeTests.cpp
GridTD029PartyMovementFacadeTests.cpp
GridTD052RuntimeActorDiagnosticsTests.cpp
GridTD055RuntimeActorFeedbackUITests.cpp
GridTD062PartyInventoryHotbarTests.cpp
GridTD064InventoryStackMergeTests.cpp
GridTD064PartyInventoryCursorTransferTests.cpp
GridTD066PartyInventoryEquipmentCoreTests.cpp
GridTD068PartyInventoryItemDefinitionRegistryTests.cpp
GridTD072EngineCompatibilityTests.cpp
GridTD0732PrototypeSaveContractTests.cpp
GridTD073310CharacterStateClosureCharacterizationTests.cpp
GridTD073310CharacterStateClosureNormalizationTests.cpp
GridTD07332LegacyAttributeBridgeTests.cpp
GridTD07333DerivedStatsCharacterizationTests.cpp
GridTD07333DerivedStatsNormalizationTests.cpp
GridTD07334WeightCharacterizationTests.cpp
GridTD07334WeightNormalizationTests.cpp
GridTD07335ProgressionCharacterizationTests.cpp
GridTD07335ProgressionNormalizationB1Tests.cpp
GridTD07335ProgressionNormalizationB2Tests.cpp
GridTD07336SkillCharacterizationTests.cpp
GridTD07336SkillNormalizationTests.cpp
GridTD07337SpellbookCharacterizationTests.cpp
GridTD07337SpellbookNormalizationTests.cpp
GridTD07338StatusEffectCharacterizationTests.cpp
GridTD07338StatusEffectNormalizationTests.cpp
GridTD07342CanonicalAuthoringAssetIdentityTests.cpp
GridTD07343CharacterIdentityStateNormalizationTests.cpp
GridTD07344AuthoringIdentityClosureTests.cpp
GridTD0734AuthoringIdentityCharacterizationTests.cpp
GridTD07352ItemCombatActionsNormalizationTests.cpp
GridTD07353MonsterPresentationNormalizationTests.cpp
GridTD07354MonsterRangeNormalizationTests.cpp
GridTD0735CombatDataCharacterizationTests.cpp
GridTD0736LegacyApiDataCharacterizationTests.cpp
GridTD074ActivationComponentCharacterizationTests.cpp
GridTD075ReceptacleCommandRecoveryTests.cpp
GridTRAP011DungeonTrapContractTests.cpp
GridTRAP012PassivePerceptionTests.cpp
GridTRAP013DungeonTrapDamageTests.cpp
GridTRAP0141DisarmExperienceTests.cpp
GridTRAP014PartyDisarmTests.cpp
GridTRAP016HighlightTests.cpp
GridTRAP0171SkillCheckBreakdownTests.cpp
GridTRAP017DisarmFeedbackTests.cpp
GridTRAP018DisarmConsequencesTests.cpp
GridTRAP0191OddsTests.cpp
GridTRAP0192HoverPresentationTests.cpp
GridTRAP0193TooltipPlacementTests.cpp
GridTRAPInstance01Tests.cpp
GridTRAPPIT01Tests.cpp
GridTRAPSEARCH011ActiveTests.cpp
GridTRAPSEARCH012InputUITests.cpp
GridTeleporterRunes01Tests.cpp
GridUICharacter01Tests.cpp
GridUICharacter02Tests.cpp
GridUIClean01Tests.cpp
GridUIClean02Tests.cpp
GridUIClean03Tests.cpp
GridUIClean04Tests.cpp
GridUICombat01Tests.cpp
GridUIEquipmentTooltip043CIconTests.cpp
GridUIEventHUD0110ValidationTests.cpp
GridUIEventHUD0111GameplayTimestampTests.cpp
GridUIEventHUD0112EventConsequenceGroupingTests.cpp
GridUIEventHUD011CoreTests.cpp
GridUIEventHUD012ShellTests.cpp
GridUIEventHUD013InteractionTests.cpp
GridUIEventHUD014DungeonMessagesTests.cpp
GridUIEventHUD015PopupMigrationTests.cpp
GridUIEventHUD016SemanticColorsTests.cpp
GridUIEventHUD017CombatTraceTests.cpp
GridUIEventHUD018CombatRenderingTests.cpp
GridUIEventHUD018PCompactCombatTests.cpp
GridUIEventHUD019DialogueFeedTests.cpp
GridUIEventHUD01PresentationPolishTests.cpp
GridUIEventHUD01XP01ExperienceFeedTests.cpp
GridUIFeedback01Tests.cpp
GridUIFilter01Tests.cpp
GridUIGlobalHud01Tests.cpp
GridUIInventory01Tests.cpp
GridUIInventory02Tests.cpp
GridUIInventoryProjection02Tests.cpp
GridUIItem01Clean01Tests.cpp
GridUIItem01Tests.cpp
GridUINavigation01Tests.cpp
GridUIPartyHud01Tests.cpp
GridUIPartyHud02AllPortraitAlertsTests.cpp
GridUIPartyProgressionReminderTests.cpp
GridUISplit01Tests.cpp
GridUISplit02Tests.cpp
GridUISplit03Tests.cpp
GridUITooltipClean01ShieldTests.cpp
GridUIWeaponTooltip04NativeMigrationTests.cpp
GridUIWeaponTooltip04NativePresenterTests.cpp
GridUIWeaponTooltip04Tests.cpp
GridUIWeight01Tests.cpp
GridVfxArchitectureTests.cpp
GridWallAxisWALLAXIS01Tests.cpp
GridWorldObjectALIGNB51DirectPlacementResolverTests.cpp
GridWorldObjectALIGNB53PlacementBridgePurgeTests.cpp
GridWorldObjectDoorChainOverrideTests.cpp
GridWorldObjectGEUI09InteractiveInstanceOverrideTests.cpp
GridWorldObjectITEMCLASS01Tests.cpp
GridWorldObjectInstanceMotionOverrideTests.cpp
GridWorldObjectMIG00PlacementCharacterizationTests.cpp
GridWorldObjectMIG01PlacementTests.cpp
GridWorldObjectMIG02SpatialBehaviorTests.cpp
GridWorldObjectMIG03RuntimeSpawnTests.cpp
GridWorldObjectMIG03VisualCompositionTests.cpp
GridWorldObjectMIG04GenericMotionTests.cpp
GridWorldObjectMIG05CollectibleDefinitionTests.cpp
GridWorldObjectMIG06BehaviorOverrideTests.cpp
GridWorldObjectMIG07TypedAuthorityTests.cpp
GridWorldObjectMIG07TypedLifecycleTests.cpp
GridWorldObjectMIG07TypedPlacementTests.cpp
GridWorldObjectMIG09ItemIdentityTests.cpp
GridWorldObjectMIG09RuntimePayloadTests.cpp
GridWorldObjectMIG09SparseMarkerPurgeTests.cpp
GridWorldObjectMIG10DefinitionNamingTests.cpp
GridWorldObjectMovingParts01Tests.cpp
GridWorldObjectRecoveryClosureTests.cpp
GridWorldObjectReverseMotionDurationTests.cpp
GrimrockSTARTUPFLOW01Tests.cpp
GrimrockSTARTUPFLOW02Tests.cpp
RPGATTR01AttributePointTests.cpp
RPGCharacterChoicesCC6Tests.cpp
RPGCharacterCreationCC2Tests.cpp
RPGCharacterInventoryCC4Tests.cpp
RPGCharacterModelCC1Tests.cpp
RPGCharacterPersistenceCC5Tests.cpp
RPGCustomRecruitMON205EventCommandTests.cpp
RPGCustomRecruitMON205RuntimeIntegrationTests.cpp
RPGCustomRecruitMON205Tests.cpp
RPGCustomRecruitMON205WizardContextTests.cpp
RPGDEV01ProgressionPIESimulatorTests.cpp
RPGLEVELUX01NonModalLevelUpTests.cpp
RPGMON151ProgressionTests.cpp
RPGMON152ExperienceAwardTests.cpp
RPGMON153LevelUpTests.cpp
RPGMON154ClassProgressionTests.cpp
RPGMON155LevelUpInterfaceTests.cpp
RPGMON155ProgressionTransactionTests.cpp
RPGMON155TestHelpers.h
RPGMON157BalanceTests.cpp
RPGMON161StatusEffectTests.cpp
RPGMON162StatusEffectLifecycleTests.cpp
RPGMON163StatusEffectPeriodicDamageTests.cpp
RPGMON164StatusEffectInitiativeTests.cpp
RPGMON165StatusEffectControlTests.cpp
RPGMON166StatusEffectPresentationTests.cpp
RPGMON167StatusEffectPersistenceTests.cpp
RPGMON168StatusEffectMilestoneTests.cpp
RPGMON2062SkillRuntimeTests.cpp
RPGMON2063SkillCheckTests.cpp
RPGMON2074TalentPersistenceRegressionTests.cpp
RPGMON2082SkillRequirementProjectionTests.cpp
RPGMON2083ActionRequirementIntegrationTests.cpp
RPGMON2084SkillsPageReadModelTests.cpp
RPGMON2092SkillPersistenceTests.cpp
RPGMON2093ActivePoolPersistenceTests.cpp
RPGMON2094RestoredConsumerRegressionTests.cpp
RPGPartyRecruitmentMON202Tests.cpp
RPGRPG031CombatModifierTests.cpp
RPGRPG032StatusApplicationTests.cpp
RPGRPG033ArmorEffectTests.cpp
RPGRPG034ReactionTests.cpp
RPGRPG035MovementTests.cpp
RPGRPG036SurfaceTests.cpp
RPGRPG037QuickItemTests.cpp
RPGRPG038TargetingBatchTests.cpp
RPGRPG0390WeaponTalentAttackTests.cpp
RPGRPG0391SupportTests.cpp
RPGRPG0392SupportTests.cpp
RPGRPG0393SupportTests.cpp
RPGRPG0394E1ArcanistSupportTests.cpp
RPGRPG0394F1SurfaceWeaverSupportTests.cpp
RPGRPG0395SupportTests.cpp
RPGRPG0396SupportTests.cpp
RPGSKILL01SkillPointTests.cpp
RPGStoryCompanionMON203Tests.cpp
RPGStoryCompanionMON204EventCommandTests.cpp
RPGStoryCompanionMON204OfferSuppressionTests.cpp
RPGStoryCompanionMON204PaletteTests.cpp
RPGStoryCompanionMON204RecruitmentUITests.cpp
RPGStoryCompanionMON204RuntimeIntegrationTests.cpp
RPGUIRPG013TalentTreeReadModelTests.cpp
UIRPG021PresentationDataTests.cpp
UIRPG043ATalentAcquisitionTests.cpp
UIRPG044TalentVariantTests.cpp
UIRPG05ProgressionFeedbackTests.cpp
UIRPG062BSkillsRendererTests.cpp
UIRPGDESC01RichTalentDetailsTests.cpp
UIRPGTALENTCODEAUDIT023RuntimeCacheOwnershipTests.cpp
UIRPGTALENTCODEAUDIT024TalentDetailReadModelTests.cpp
UIRPGTALENTCODEAUDIT025PresentationDependencyTests.cpp
UIRPGTALENTCODEAUDIT026NodeStateStyleTests.cpp
UIRPGTALENTCODEAUDIT027SourceSplitTests.cpp
UIRPGTALENTCODEAUDIT028PresentationDataAuditTests.cpp
UIRPGTALENTOPTIONALLOCALIZATIONTests.cpp
UISKILLS0210SkillErrorBoundaryTests.cpp
UISKILLS021ArchitectureTests.cpp
UISKILLS022PresentationPipelineTests.cpp
UISKILLS023CompactSkillEntryTests.cpp
UISKILLS024SkillDetailWidgetTests.cpp
UISKILLS029SessionUndoReconciliationTests.cpp
UISKILLSAFFINITY01ClassAffinityTests.cpp
UISKILLSSORT01SkillSortingTests.cpp
UISKILLSSUMMARY01SkillSummaryTests.cpp
UITEXTCENTRAL012D1RecipeCatalogTests.cpp
UITEXTCENTRAL012D2TalentLocalizationTests.cpp
UITEXTCENTRAL013SkillVocabularyTests.cpp
```

**`Source/GrimrockPrototype/Private/UI/` — 70**

```text
GridCanonicalInventoryItemTypeReadModel.h
GridCharacterSheetWidget.cpp
GridCombatActionPanelWidget.cpp
GridCombatArmorRowWidget.cpp
GridCombatHotbarDragDropOperation.cpp
GridCombatHudCanonicalActionReadModel.h
GridCombatHudWidget.cpp
GridCombatTooltipWidget.cpp
GridDungeonBuildProgressWidget.cpp
GridEquipmentTooltipGlobalProjection.cpp
GridEventFeedWidget.cpp
GridInventoryBagWidget.cpp
GridInventoryDragDropOperation.cpp
GridInventoryItemMetadataReadModel.h
GridInventorySlotWidget.cpp
GridInventorySortReadModel.h
GridInventoryWidget.cpp
GridInventoryWidgetClassIcon.cpp
GridItemTooltipEquipmentComparisonWidget.cpp
GridItemTooltipWeaponLineWidget.cpp
GridItemTooltipWidget.cpp
GridMapWidget.cpp
GridNavigationTooltipWidget.cpp
GridPartyMemberWidget.cpp
GridPartyPortraitDragDropOperation.cpp
GridPartyPortraitHudWidget.cpp
GridPersistentHudWidget.cpp
GridQuickItemSurfaceTooltipText.cpp
GridRPGNotificationWidget.cpp
GridSkillDetailWidget.cpp
GridSkillEntryWidget.cpp
GridSkillsPageService.cpp
GridSkillsPageServiceInternal.h
GridSkillsPageServiceTalentDetail.cpp
GridSkillsPageServiceTalentTree.cpp
GridSkillsWidget.cpp
GridSkillsWidgetFeedbackInternal.h
GridSkillsWidgetPresentation.cpp
GridSkillsWidgetSessionUndoInternal.h
GridSkillsWidgetSkills.cpp
GridSkillsWidgetTalents.cpp
GridSpellbookEntryWidget.cpp
GridSpellbookWidget.cpp
GridStartingKitDisplayReadModel.h
GridTalentBranchWidget.cpp
GridTalentDetailLineWidget.cpp
GridTalentDetailWidget.cpp
GridTalentNodeWidget.cpp
GridTalentVariantFlyoutWidget.cpp
GridTalentVariantIconWidget.cpp
GridWeaponTooltipProjection.cpp
GridWeaponVisualUiResolver.cpp
GrimrockDesignSurfaceWidget.cpp
GrimrockLoadGameMenuWidget.cpp
GrimrockLoadGameSlotWidget.cpp
GrimrockMainMenuModalWidget.cpp
GrimrockMainMenuWidget.cpp
GrimrockMenuWidget.cpp
RPGCharacterCreationAttributesStepWidget.cpp
RPGCharacterCreationWidget.cpp
RPGCharacterCreationWizardWidget.cpp
RPGProgressionFeedbackService.cpp
RPGSkillPresentationAsset.cpp
RPGStoryCompanionRecruitmentWidget.cpp
RPGStoryCompanionRecruitmentWidgetSlate.cpp
RPGTalentDetailGlyphSet.cpp
RPGTalentNodeStyleSettings.cpp
RPGTalentPresentationAsset.cpp
ReadableMessageWidget.cpp
UILocalizationText.cpp
```

**`Source/GrimrockPrototype/Public/Core/` — 29**

```text
GridBoundary.h
GridCatalystVfxProfileAsset.h
GridDirectionUtils.h
GridDungeonAsset.h
GridDungeonTrapInstanceUtils.h
GridDungeonTrapPitUtils.h
GridDungeonTrapTypes.h
GridFireAudioClusters.h
GridFireIgnitionWave.h
GridFireNiagaraBindings.h
GridFireNiagaraGeometry.h
GridFlaskBallistics.h
GridGlassShardMotion.h
GridLevelAsset.h
GridLevelPlacementTypes.h
GridLevelVariableTypes.h
GridLogicTypes.h
GridNiagaraUserOverrideApplicator.h
GridObjectAudio.h
GridObjectBehavior.h
GridObjectInstanceBehavior.h
GridObjectPaletteAsset.h
GridRelocationUtils.h
GridSurfaceVfxProfileAsset.h
GridTypes.h
GridVfxProfileTypes.h
GridWorldObjectDefinitionAsset.h
GridWorldObjectInstanceVisual.h
GridWorldObjectVisual.h
```

**`Source/GrimrockPrototype/Public/Magic/` — 13**

```text
GridPartySpellbookComponent.h
GridProductionSpellLibrary.h
GridSpellCastTransaction.h
GridSpellDefinitionAsset.h
GridSpellEffectResolver.h
GridSpellHotbarExecution.h
GridSpellPresentation.h
GridSpellPresentationComponent.h
GridSpellTargeting.h
GridSpellTypes.h
GridSpellbookPersistence.h
GridSpellbookTypes.h
GridSpellbookUI.h
```

**`Source/GrimrockPrototype/Public/Quests/` — 3**

```text
GridQuestDefinitionAsset.h
GridQuestSubsystem.h
GridQuestTypes.h
```

**`Source/GrimrockPrototype/Public/RPG/` — 29**

```text
RPGAttributePointService.h
RPGAuthoringIdentityResolver.h
RPGCharacterIdentityPersistence.h
RPGCharacterPortraitSetAsset.h
RPGCharacterRulesLibrary.h
RPGCharacterTypes.h
RPGClassAsset.h
RPGClassProgressionService.h
RPGClassProgressionTransactionService.h
RPGClassVisualAsset.h
RPGCustomRecruitService.h
RPGExperienceRewardService.h
RPGLevelUpService.h
RPGPartyProgressionResolver.h
RPGPartyRecruitmentService.h
RPGProgressionPIESimulator.h
RPGRaceAsset.h
RPGRecipeCatalogAsset.h
RPGSkillAsset.h
RPGSkillCheckService.h
RPGSkillPersistence.h
RPGSkillPointService.h
RPGSkillRequirementProjectionService.h
RPGSkillService.h
RPGSkillTypes.h
RPGStartingEquipmentPresetCatalog.h
RPGStartingEquipmentService.h
RPGStoryCompanionAsset.h
RPGStoryCompanionService.h
```

**`Source/GrimrockPrototype/Public/RPG/StatusEffects/` — 9**

```text
GridCombatStatusApplicationResolver.h
GridStatusEffectControlResolver.h
GridStatusEffectDefinitionAsset.h
GridStatusEffectInitiativeResolver.h
GridStatusEffectLifecycleSubsystem.h
GridStatusEffectPeriodicDamageResolver.h
GridStatusEffectPersistence.h
GridStatusEffectPresentation.h
GridStatusEffectTypes.h
```

**`Source/GrimrockPrototype/Public/Runtime/` — 56**

```text
GridActivationComponent.h
GridButtonActor.h
GridDoorActor.h
GridDoorSystemComponent.h
GridDungeonRuntimeState.h
GridDungeonTrapDisarmFeedback.h
GridDungeonTrapDisarmOdds.h
GridDungeonTrapPerceptionService.h
GridDungeonTrapRewardRules.h
GridEditorPreviewComponent.h
GridEditorPreviewObjectActor.h
GridEquipmentPreviewTypes.h
GridEventFeedPublisher.h
GridEventFeedSubsystem.h
GridEventFeedTypes.h
GridGenericObjectActor.h
GridInteractableInterface.h
GridInteractionUtils.h
GridInventoryTypes.h
GridItemActionTypes.h
GridItemActor.h
GridItemContextActionLibrary.h
GridItemDefinitionAsset.h
GridItemTransferService.h
GridItemVisualEffectsComponent.h
GridItemVisualEffectsTypes.h
GridLevelRuntimeActor.h
GridLevelVariableStore.h
GridLeverActor.h
GridLightEmitterComponent.h
GridLightEmitterTypes.h
GridLogicRuntime.h
GridMechanismActor.h
GridMonsterEncounterComponent.h
GridPIEPlaytestRequest.h
GridPartyIlluminationComponent.h
GridPartyInventoryComponent.h
GridPartyPortraitTypes.h
GridPartyProgressionReminderState.h
GridPitTrapdoorActor.h
GridPlacementTransformResolver.h
GridPressurePlateActor.h
GridReadableContentAsset.h
GridReceptacleActor.h
GridReceptacleTypes.h
GridRuntimeObjectActor.h
GridRuntimeWorldObjectData.h
GridSecretDoorActor.h
GridThrownItemActor.h
GridTriggerActor.h
GridWallLockActor.h
GrimrockGameInstance.h
GrimrockGameMode.h
GrimrockPartyPawn.h
GrimrockPlayerController.h
GrimrockStartupModeComponent.h
```

**`Source/GrimrockPrototype/Public/Runtime/Combat/` — 29**

```text
GridCanonicalAttackPresentationReadModel.h
GridCatalystAreaResolver.h
GridCombatActionCatalog.h
GridCombatActionIdentityTypes.h
GridCombatArmorEffectResolver.h
GridCombatDiagnostics.h
GridCombatEventFeedFormatter.h
GridCombatLog.h
GridCombatModifierResolver.h
GridCombatMovementResolver.h
GridCombatProjectileActor.h
GridCombatReactionResolver.h
GridCombatResolver.h
GridCombatSurfaceResolver.h
GridCombatTargetingResolver.h
GridCombatTraceTypes.h
GridCombatTypes.h
GridDualWieldPolicy.h
GridEquippedGuardResolver.h
GridPlayerAttackPresentationComponent.h
GridPlayerAttackPresentationTypes.h
GridProjectileGeometry.h
GridQuickItemResolver.h
GridRangedAmmunitionPolicy.h
GridTurnManagerComponent.h
GridWeaponCatalogueAsset.h
GridWeaponVisualCatalogueAsset.h
GridWeaponVisualResolver.h
GridWorldItemVisualResolver.h
```

**`Source/GrimrockPrototype/Public/Runtime/Items/` — 11**

```text
GridCanonicalInventoryCategoryTags.h
GridCanonicalItemCatalogues.h
GridCanonicalItemDefinitionAsset.h
GridCanonicalNonArmamentReadPolicies.h
GridCanonicalNonArmamentThrowReadModel.h
GridCanonicalPuzzleCategoryTags.h
GridCanonicalWorldSnapshotReadModel.h
GridEquippedWeaponActionIconTypes.h
GridItemCacheParityAudit.h
GridItemSubsystem.h
GridNonArmamentThrowPolicy.h
```

**`Source/GrimrockPrototype/Public/Runtime/Map/` — 3**

```text
GridMapExplorationState.h
GridMapReadModel.h
GridMapRevealService.h
```

**`Source/GrimrockPrototype/Public/Runtime/Monsters/` — 24**

```text
GridAutomaticPerceptionEngagementSubsystem.h
GridMonsterActor.h
GridMonsterAudioComponent.h
GridMonsterAudioTypes.h
GridMonsterBalanceTypes.h
GridMonsterBehaviorComponent.h
GridMonsterCategoryAsset.h
GridMonsterCombatComponent.h
GridMonsterDeathComponent.h
GridMonsterDefinitionAsset.h
GridMonsterFastHarasserPlanner.h
GridMonsterIdleVariationComponent.h
GridMonsterIdleVariationTypes.h
GridMonsterLootResolver.h
GridMonsterMovementComponent.h
GridMonsterOccupancySubsystem.h
GridMonsterPathfinder.h
GridMonsterPatrolSubsystem.h
GridMonsterRangedAttackPlanner.h
GridMonsterRangedKeeperPlanner.h
GridMonsterTraversalUtils.h
GridMonsterTypes.h
GridMonsterVFXComponent.h
GridMonsterVFXTypes.h
```

**`Source/GrimrockPrototype/Public/Save/` — 3**

```text
GridCombatSavePolicy.h
GrimrockPartySaveGame.h
GrimrockSaveSlotLibrary.h
```

**`Source/GrimrockPrototype/Public/Tests/` — 3**

```text
GridAutomationSaveIsolation.h
GridCanonicalLootRegistryTestUtils.h
GridCanonicalRegistryFixture.h
```

**`Source/GrimrockPrototype/Public/UI/` — 61**

```text
GridCharacterSheetWidget.h
GridCombatActionPanelWidget.h
GridCombatArmorRowWidget.h
GridCombatHotbarDragDropOperation.h
GridCombatHudWidget.h
GridCombatTooltipWidget.h
GridDungeonBuildProgressWidget.h
GridEquipmentTooltipGlobalProjection.h
GridEquippedWeaponActionIconPalette.h
GridEventFeedWidget.h
GridInventoryBagWidget.h
GridInventoryDragDropOperation.h
GridInventorySlotWidget.h
GridInventoryUiTypes.h
GridInventoryWidget.h
GridItemTooltipEquipmentComparisonWidget.h
GridItemTooltipWeaponLineWidget.h
GridItemTooltipWidget.h
GridMapSurfaceWidget.h
GridMapVisualThemeAsset.h
GridMapWidget.h
GridNavigationTooltipWidget.h
GridPartyMemberWidget.h
GridPartyPortraitDragDropOperation.h
GridPartyPortraitHudWidget.h
GridPersistentHudWidget.h
GridQuickItemSurfaceTooltipText.h
GridRPGNotificationWidget.h
GridSkillDetailWidget.h
GridSkillEntryWidget.h
GridSkillsPageService.h
GridSkillsUiTypes.h
GridSkillsWidget.h
GridSpellbookEntryWidget.h
GridSpellbookWidget.h
GridTalentBranchWidget.h
GridTalentDetailLineWidget.h
GridTalentDetailWidget.h
GridTalentNodeWidget.h
GridTalentVariantFlyoutWidget.h
GridTalentVariantIconWidget.h
GridWeaponTooltipProjection.h
GridWeaponVisualUiResolver.h
GridWorldTooltipPlacement.h
GrimrockDesignSurfaceWidget.h
GrimrockLoadGameMenuWidget.h
GrimrockLoadGameSlotWidget.h
GrimrockMainMenuModalWidget.h
GrimrockMainMenuWidget.h
GrimrockMenuWidget.h
RPGCharacterCreationAttributesStepWidget.h
RPGCharacterCreationWidget.h
RPGCharacterCreationWizardWidget.h
RPGProgressionFeedbackService.h
RPGSkillPresentationAsset.h
RPGStoryCompanionRecruitmentWidget.h
RPGTalentDetailGlyphSet.h
RPGTalentNodeStyleSettings.h
RPGTalentPresentationAsset.h
ReadableMessageWidget.h
UILocalizationText.h
```

**`Source/GrimrockPrototypeEditor/` — 2**

```text
GrimrockPrototypeEditor.Build.cs
GrimrockPrototypeEditor.cpp
```

**`Source/GrimrockPrototypeEditor/Private/ARMEMENT/` — 21**

```text
GridArmClean064X371BackupReceiptGuard.h
GridArmClean064X64SaveCopyCommandlet.cpp
GridArmClean065RetirementAuditCommandlet.cpp
GridArmClean06BaselineCommandlet.cpp
GridEditorAlchemyActionIdentity.h
GridEditorAlchemySaveCopy.h
GridEditorInstanceCacheEvidence.h
GridEditorInstanceSchemaProjection.h
GridEditorNativeDormantOverridesPlan.h
GridEditorNativeInitialContentPlan.h
GridEditorNativePalettePlan.h
GridEditorReadableSaveRepair.h
GridEditorRetirementAudit.h
GridEditorWorldSaveProjection.h
GridOilSurfaceMaterialAuthoringCommandlet.cpp
GridStartingKitCoherenceAudit.cpp
GridStartingKitCoherenceAuditCommandlet.cpp
GridWeaponCatalogueImportCommandlet.cpp
GridWeaponNarrativeImport.cpp
GridWeaponPipelineManifest.cpp
GridWeaponPipelinePalette.cpp
```

**`Source/GrimrockPrototypeEditor/Private/Details/` — 3**

```text
GridEditorCanonicalArmamentPreview.h
GrimrockPartyDetailsCustomization.cpp
GrimrockWeaponDetailsCustomization.cpp
```

**`Source/GrimrockPrototypeEditor/Private/EditorTools/` — 10**

```text
GridEditorLinkPolicy.cpp
GridEditorLinkService.cpp
GridEditorLuaService.cpp
GridEditorUndoBridge.cpp
GridLevelEdMode.cpp
GridLevelEdModeToolkit.cpp
GridLevelEditorActor.cpp
GridLevelEditorActorPatrolRoute.cpp
GridLuaAuthoringCompiler.cpp
GridLuaSyntaxHighlighter.cpp
```

**`Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/` — 4**

```text
GridLevelEditorActor_CoreDungeon.inl
GridLevelEditorActor_EditingObjectsLinks.inl
GridLevelEditorActor_InteractionViewport.inl
GridLevelEditorActor_Validation.inl
```

**`Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/CoreDungeon/` — 10**

```text
GridLevelEditorActor_CoreDungeon_01.inl
GridLevelEditorActor_CoreDungeon_02.inl
GridLevelEditorActor_CoreDungeon_03.inl
GridLevelEditorActor_CoreDungeon_04.inl
GridLevelEditorActor_CoreDungeon_05.inl
GridLevelEditorActor_CoreDungeon_06.inl
GridLevelEditorActor_CoreDungeon_07.inl
GridLevelEditorActor_CoreDungeon_08.inl
GridLevelEditorActor_CoreDungeon_09.inl
GridLevelEditorActor_CoreDungeon_TRAP015.inl
```

**`Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/EditingObjectsLinks/` — 9**

```text
GridLevelEditorActor_EditingObjectsLinks_01.inl
GridLevelEditorActor_EditingObjectsLinks_02.inl
GridLevelEditorActor_EditingObjectsLinks_03.inl
GridLevelEditorActor_EditingObjectsLinks_04.inl
GridLevelEditorActor_EditingObjectsLinks_05.inl
GridLevelEditorActor_EditingObjectsLinks_06.inl
GridLevelEditorActor_EditingObjectsLinks_07.inl
GridLevelEditorActor_EditingObjectsLinks_08.inl
GridLevelEditorActor_EditingObjectsLinks_09.inl
```

**`Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/` — 14**

```text
GridEditorCanonicalPaletteReadModel.h
GridEditorWidgetHelpers.cpp
SGridEditorDoorMotionOverridePanel.cpp
SGridEditorDungeonLevelsPanel.cpp
SGridEditorLinksPanel.cpp
SGridEditorLuaScriptsPanel.cpp
SGridEditorObjectIdentityPanel.cpp
SGridEditorObjectInspectorPanel.cpp
SGridEditorOverviewMapPanel.cpp
SGridEditorPlaytestPanel.cpp
SGridEditorPressurePlateInstancePanel.cpp
SGridEditorToolPalettePanel.cpp
SGridEditorValidationPanel.cpp
SGridEditorWorkspaceTab.cpp
```

**`Source/GrimrockPrototypeEditor/Private/Effects/` — 1**

```text
GridEffectsOrgTorchMoveCommandlet.cpp
```

**`Source/GrimrockPrototypeEditor/Private/Localization/` — 6**

```text
UILocalizationCsvAuthoring.cpp
UILocalizationCsvAuthoring.h
UILocalizationEnglishSmokeCommandlet.cpp
UILocalizationEnglishSmokeCommandlet.h
UIStringTableAuthoringCommandlet.cpp
UIStringTableAuthoringCommandlet.h
```

**`Source/GrimrockPrototypeEditor/Private/RPG/` — 17**

```text
RPGAlchemistAuthoring.cpp
RPGAlchemistAuthoringCommandlet.cpp
RPGClassProgressionAuthoring.cpp
RPGClassProgressionAuthoringCommandlet.cpp
RPGMageAuthoring.cpp
RPGMageAuthoringCommandlet.cpp
RPGPriestAuthoring.cpp
RPGPriestAuthoringCommandlet.cpp
RPGRangerAuthoring.cpp
RPGRangerAuthoringCommandlet.cpp
RPGRogueAuthoring.cpp
RPGRogueAuthoringCommandlet.cpp
RPGSkillCatalogAuthoring.cpp
RPGSkillCatalogAuthoringCommandlet.cpp
RPGWarriorAuthoring.cpp
RPGWarriorAuthoringCommandlet.cpp
UIRPGTalentTreeAuthoringCommandlet.cpp
```

**`Source/GrimrockPrototypeEditor/Private/Tests/` — 213**

```text
GridARMCLEAN062E2WeaponPackageTests.cpp
GridEditorALIGNA3AssetReferenceAuditTests.cpp
GridEditorALIGNA4PreviewDefinitionSyncTests.cpp
GridEditorALIGNB52DirectPlacementTests.cpp
GridEditorARMCLEAN062ADSaveInstanceParityTests.cpp
GridEditorARMCLEAN062AEArmorReserveParityTests.cpp
GridEditorARMCLEAN062GActionPresentationTests.cpp
GridEditorARMCLEAN062HStandardActionTests.cpp
GridEditorARMCLEAN062JVisualParityTests.cpp
GridEditorARMCLEAN062KActionIconsTests.cpp
GridEditorARMCLEAN062LItemIdentityTests.cpp
GridEditorARMCLEAN062MClassAffinityTests.cpp
GridEditorARMCLEAN062PProductionCandidateTests.cpp
GridEditorARMCLEAN062QProductionActionsTests.cpp
GridEditorARMCLEAN062SNumericsSemanticsTests.cpp
GridEditorARMCLEAN062UFullProductionCandidateTests.cpp
GridEditorARMCLEAN062VProductionAmmoTests.cpp
GridEditorARMCLEAN062WProjectilePolicyTests.cpp
GridEditorARMCLEAN062XProjectileVisualTests.cpp
GridEditorARMCLEAN062YCompleteProductionPreviewTests.cpp
GridEditorARMCLEAN063ANItemLevelTests.cpp
GridEditorARMCLEAN063ARealRegistryTests.cpp
GridEditorARMCLEAN064UQuickActionTests.cpp
GridEditorARMCLEAN064VEquippedNonArmTests.cpp
GridEditorARMCLEAN064WWorldReferencesTests.cpp
GridEditorARMCLEAN064X11PaletteReadOnlyTests.cpp
GridEditorARMCLEAN064X12CanonicalPalettePlacementTests.cpp
GridEditorARMCLEAN064X13PalettePresentationParityTests.cpp
GridEditorARMCLEAN064X14CanonicalSlatePaletteTests.cpp
GridEditorARMCLEAN064X15RealPuzzleAcceptanceInventoryTests.cpp
GridEditorARMCLEAN064X162AcceptanceAuthoringTests.cpp
GridEditorARMCLEAN064X163ReceptacleInitialAuthoringTests.cpp
GridEditorARMCLEAN064X164AcceptanceMigrationPreviewTests.cpp
GridEditorARMCLEAN064X165LockKeyPreviewTests.cpp
GridEditorARMCLEAN064X1661RealAcceptanceManifestTests.cpp
GridEditorARMCLEAN064X26CanonicalValidationTests.cpp
GridEditorARMCLEAN064X27CanonicalSelectedObjectTests.cpp
GridEditorARMCLEAN064X28PointerlessPaletteTests.cpp
GridEditorARMCLEAN064X2AuthoredSourceEvidenceTests.cpp
GridEditorARMCLEAN064X371BackupGuardTests.cpp
GridEditorARMCLEAN064X37WorldProfileProjectionTests.cpp
GridEditorARMCLEAN064X3AuthoredLevelMaterializerTests.cpp
GridEditorARMCLEAN064X45NativeLoosePreflightTests.cpp
GridEditorARMCLEAN064X48PostNativeReferenceAuditTests.cpp
GridEditorARMCLEAN064X4RealAuthoredLevelsTests.cpp
GridEditorARMCLEAN064X50LockRuleAuthoringTests.cpp
GridEditorARMCLEAN064X51NativeSharedRulePreflightTests.cpp
GridEditorARMCLEAN064X55NativePaletteTests.cpp
GridEditorARMCLEAN064X58NativeInitialContentPlanTests.cpp
GridEditorARMCLEAN064X59NativeDormantOverridesPlanTests.cpp
GridEditorARMCLEAN064X60NativeStoneLootPlanTests.cpp
GridEditorARMCLEAN064X61LootProjectionTests.cpp
GridEditorARMCLEAN064X62AlchemyEvidenceTests.cpp
GridEditorARMCLEAN064X63AlchemyCutoverPlanTests.cpp
GridEditorARMCLEAN064X64SaveCopyTests.cpp
GridEditorARMCLEAN064X65AlchemyStarterTests.cpp
GridEditorARMCLEAN064X71RealReceptacleAuditTests.cpp
GridEditorARMCLEAN064X81RealLockAuditTests.cpp
GridEditorARMCLEAN064X841TD073LockSchemaWhitelistTests.cpp
GridEditorARMCLEAN065CanonicalStarterTests.cpp
GridEditorARMCLEAN065D3HandThrowTests.cpp
GridEditorARMCLEAN065D4ArmEquipmentRetirementTests.cpp
GridEditorARMCLEAN065FIconPaletteTests.cpp
GridEditorARMCLEAN065G2ArmamentPreviewTests.cpp
GridEditorARMCLEAN065G69PuzzleCategoryTests.cpp
GridEditorARMCLEAN065RetirementAuditTests.cpp
GridEditorARMEMENT01101ProductionReadinessTests.cpp
GridEditorARMEMENT011021WarriorVisualTests.cpp
GridEditorARMEMENT011023PaletteTests.cpp
GridEditorARMEMENT01102WarriorN1Tests.cpp
GridEditorARMEMENT01103WarriorFunctionalTests.cpp
GridEditorARMEMENT01111MeleeReadinessTests.cpp
GridEditorARMEMENT011121AssassinDaggerVisualTests.cpp
GridEditorARMEMENT011122AssassinDaggerItemTests.cpp
GridEditorARMEMENT01154ActiveItemAuditTests.cpp
GridEditorARMEMENT0115HealthPotionTests.cpp
GridEditorARMEMENT01161DisarmToolTests.cpp
GridEditorARMEMENT01172CVisualProductionTests.cpp
GridEditorARMEMENT01172SurfaceCatalystTests.cpp
GridEditorARMEMENT0171PalettePlacementTests.cpp
GridEditorARMEMENT0181PlacedItemLevelTests.cpp
GridEditorARMEMENT0182ItemSpawnLevelTests.cpp
GridEditorARMEMENT0194CoherenceAuditTests.cpp
GridEditorARMEMENT0195KitDecisionTests.cpp
GridEditorARMEMENT0196StarterKitRecipeTests.cpp
GridEditorARMEMENTINDUS02PipelineTests.cpp
GridEditorARMEMENTINDUS044NarrativeTests.cpp
GridEditorARMEMENTINDUS05MatrixTests.cpp
GridEditorARMEMENTLOT021322ArrowPaletteTests.cpp
GridEditorARMEMENTLOT02132WorldAcceptanceTests.cpp
GridEditorARMEMENTLOT02134RangedAmmoProjectileTests.cpp
GridEditorARMEMENTLOT02136AEmbeddedHitTests.cpp
GridEditorARMEMENTLOT02136BMissTrajectoryTests.cpp
GridEditorARMEMENTLOT0213ProductionFunctionalTests.cpp
GridEditorARMEMENTLOT0216WeaponDiskTests.cpp
GridEditorARMEMENTLOT022NewGameTests.cpp
GridEditorARMEMENTLOT025MarksmanAmmunitionTests.cpp
GridEditorARMEMENTLOT026ThrowingProductionTests.cpp
GridEditorARMEMENTLOT027MagicWeaponTests.cpp
GridEditorARMEMENTLOT028FocusPassiveTests.cpp
GridEditorARMEMENTLOT02ReadinessTests.cpp
GridEditorCPPClean03LegacySurfaceTests.cpp
GridEditorCPPClean04AssetConfigurationTests.cpp
GridEditorCanonicalIconFixture.h
GridEditorCanonicalStarterTestWorld.h
GridEditorEffectsOrg011InventoryTests.cpp
GridEditorEffectsOrg012DPowerShellSyntaxTests.cpp
GridEditorEffectsOrg012DTorchPreflightTests.cpp
GridEditorITEMSTACK01MultipleLooseItemsTests.cpp
GridEditorItemVFX012AuthoringTests.cpp
GridEditorItemVFX013AuthoringTests.cpp
GridEditorItemVFX014ProductionPolicyTests.cpp
GridEditorLUACOMP01CompilerTests.cpp
GridEditorLUAUX02IdentityTests.cpp
GridEditorLUAUX03HardPurgeTests.cpp
GridEditorLUAUX04SyntaxHighlightTests.cpp
GridEditorMON133LinkPolicyTests.cpp
GridEditorMON135RealPIETests.cpp
GridEditorMON14_3_1Tests.cpp
GridEditorMON192ExactMutationTests.cpp
GridEditorMON192LinkContractTests.cpp
GridEditorMON192LogicPolicyTests.cpp
GridEditorMON194LuaLinkTests.cpp
GridEditorMON196LuaEditorTests.cpp
GridEditorMON196ReferenceCountRegressionTest.cpp
GridEditorMON1971LuaAuthoringTests.cpp
GridEditorMON204RecruitmentLinkPolicyTests.cpp
GridEditorMON204StoryCompanionPlacementTests.cpp
GridEditorMON205CustomRecruitLinkPolicyTests.cpp
GridEditorMonsterSpawnInspectorContractTests.cpp
GridEditorPIT03LinkPolicyTests.cpp
GridEditorPUZZLE01Lua02ProductionAuditTests.cpp
GridEditorTD013EventCommandContractTests.cpp
GridEditorTD031LegacyDetailsCleanupTests.cpp
GridEditorTD032ObjectInspectorDetailsCleanupTests.cpp
GridEditorTD033DefinitionSyncDetailsCleanupTests.cpp
GridEditorTD034DungeonLevelDetailsCleanupTests.cpp
GridEditorTD0731CurrentSchemaAssetAuditTests.cpp
GridEditorTD0736LegacyApiDataNormalizationTests.cpp
GridEditorTD0737CurrentAssetNormalizationTests.cpp
GridEditorTD0737CurrentAssetRepairCharacterizationTests.cpp
GridEditorTD0738StrictCurrentSchemaTests.cpp
GridEditorTD073ApprovedCanonicalLockAcceptance.h
GridEditorTD073ApprovedCanonicalStoneLoot.h
GridEditorTRAP014SkillContractTests.cpp
GridEditorTRAP015StarterPlacementTests.cpp
GridEditorUIAssetClean01ReferenceAuditTests.cpp
GridEditorUIAssetOrg01MigrationAuditTests.cpp
GridEditorUITooltipClean01AuditTests.cpp
GridEditorUITooltipCppClean023BlueprintTests.cpp
GridEditorUIWBPAudit02InventoryTests.cpp
GridEditorWorldObjectMIG05CollectiblePlacementTests.cpp
GridEditorWorldObjectMIG06SparsePlacementTests.cpp
GridEditorWorldObjectMIG07TypedEditTests.cpp
GridEditorWorldObjectMIG09ItemBlueprintReferenceTests.cpp
GridEditorWorldObjectMIG09MechanismBlueprintReferenceTests.cpp
GridEditorWorldObjectMIG09TypedAuthorityTests.cpp
GridOilSurfaceMaterialProductionTests.cpp
GridRelocationACT01Tests.cpp
GridRelocationRELOC01Tests.cpp
GridStairsDefinitionIdempotencyTests.cpp
GridTypedPlacementAuditTestUtils.h
GridWorldObjectCLASS01Tests.cpp
GridWorldObjectMovingParts01AssetTests.cpp
RPGRPG0310GlobalBalanceTests.cpp
RPGRPG0310RealPIETests.cpp
RPGRPG0391WarriorAuthoringTests.cpp
RPGRPG0392RogueAuthoringTests.cpp
RPGRPG0393RangerAuthoringTests.cpp
RPGRPG0394MageAuthoringTests.cpp
RPGRPG0395PriestAuthoringTests.cpp
RPGRPG0395PriestExorcismTests.cpp
RPGRPG0395PriestProductionTests.cpp
RPGRPG0396AlchemistB1AuthoringTests.cpp
RPGRPG0396AlchemistB2AuthoringTests.cpp
RPGRPG0396AlchemistProductionTests.cpp
UILOCALIZATION0110ResidualPresentationMigrationTests.cpp
UILOCALIZATION0111RecruitmentErrorBoundaryTests.cpp
UILOCALIZATION0112FinalP0ClosureTests.cpp
UILOCALIZATION014StringTableCsvTests.cpp
UILOCALIZATION015CharacterCreationMigrationTests.cpp
UILOCALIZATION016LocalizationTargetTests.cpp
UILOCALIZATION017StatusEffectPresentationMigrationTests.cpp
UILOCALIZATION018InteractionPromptMigrationTests.cpp
UILOCALIZATION019CombatPresentationMigrationTests.cpp
UILOCALIZATION032DSharedWbpVocabularyTests.cpp
UILOCALIZATION032ERaceIllustrationOwnershipTests.cpp
UILOCALIZATION033WbpOwnershipRegressionTests.cpp
UIRPG014ProductionAssetTests.cpp
UIRPG022ProductionPresentationTests.cpp
UIRPG032TalentWidgetBindingTests.cpp
UIRPG041TalentSelectionTests.cpp
UIRPG042TalentDetailTests.cpp
UIRPG061SkillCatalogAuthoringTests.cpp
UIRPGCODEAUDIT01BlueprintCompatibilityTests.cpp
UIRPGCONTENTTEXT011OwnershipAuditTests.cpp
UIRPGCONTENTTEXT021ItemOwnershipAuditTests.cpp
UIRPGCONTENTTEXT031StatusOwnershipAuditTests.cpp
UIRPGCONTENTTEXT041MonsterCategoryOwnershipAuditTests.cpp
UIRPGCONTENTTEXT051RemainingAuthorityAuditTests.cpp
UIRPGCONTENTTEXT054SpellMaterializationAuditTests.cpp
UIRPGCONTENTTEXT056SpellRuntimeAssetAuthorityTests.cpp
UIRPGDESC013TalentDescriptionAuthoringTests.cpp
UIRPGDESC014TalentTypeAuthoringTests.cpp
UIRPGDESC016SixClassQATests.cpp
UIRPGTALENTCODEAUDIT022VariantFlyoutContractTests.cpp
UIRPGVISUAL014FinalAuditTests.cpp
UISKILLS028ProductionRegressionTests.cpp
UISKILLSAFFINITY013ProductionAuthoringTests.cpp
UITEXTCENTRAL012D1RecipeCatalogProductionTests.cpp
UITEXTCENTRAL012D2TalentLegacyPurgeTests.cpp
UITEXTCENTRAL012D3LegacyMetadataTests.cpp
UITEXTCENTRAL013SkillVocabularySourceTests.cpp
```

**`Source/GrimrockPrototypeEditor/Private/UI/` — 5**

```text
SkillAffinityAuditCommandlet.cpp
SkillAffinityAuthoringCommandlet.cpp
SkillAffinityProductionAuthoring.cpp
UIRPGPresentationAuthoringCommandlet.cpp
UIRPGTalentPresentationAuthoring.cpp
```

**`Source/GrimrockPrototypeEditor/Public/ARMEMENT/` — 16**

```text
GridArmClean064X3AuthoredLevelMaterializer.h
GridArmClean064X64SaveCopyCommandlet.h
GridArmClean065RetirementAuditCommandlet.h
GridArmClean06BaselineCommandlet.h
GridAssassinDaggerN1ItemAuthoringSpecs.h
GridAssassinDaggerVisualSpecs.h
GridOilSurfaceMaterialAuthoringCommandlet.h
GridStartingKitCoherenceAudit.h
GridStartingKitCoherenceAuditCommandlet.h
GridWarriorN1ItemAuthoringSpecs.h
GridWarriorN1VisualSpecs.h
GridWeaponCatalogueImportCommandlet.h
GridWeaponFireBombPalettePlan.h
GridWeaponNarrativeImport.h
GridWeaponPipelineManifest.h
GridWeaponPipelinePalette.h
```

**`Source/GrimrockPrototypeEditor/Public/EditorTools/` — 13**

```text
GridEditorCanonicalItemLevelPolicy.h
GridEditorCanonicalLockRuleAuthoring.h
GridEditorCanonicalReceptacleInitialAuthoring.h
GridEditorCanonicalReceptacleRuleAuthoring.h
GridEditorLinkPolicy.h
GridEditorLinkService.h
GridEditorLuaService.h
GridEditorWorkspaceTabs.h
GridLevelEdMode.h
GridLevelEdModeToolkit.h
GridLevelEditorActor.h
GridLuaAuthoringCompiler.h
GridLuaSyntaxHighlighter.h
```

**`Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/` — 13**

```text
GridEditorWidgetHelpers.h
SGridEditorDoorMotionOverridePanel.h
SGridEditorDungeonLevelsPanel.h
SGridEditorLinksPanel.h
SGridEditorLuaScriptsPanel.h
SGridEditorObjectIdentityPanel.h
SGridEditorObjectInspectorPanel.h
SGridEditorOverviewMapPanel.h
SGridEditorPlaytestPanel.h
SGridEditorPressurePlateInstancePanel.h
SGridEditorToolPalettePanel.h
SGridEditorValidationPanel.h
SGridEditorWorkspaceTab.h
```

**`Source/GrimrockPrototypeEditor/Public/Effects/` — 1**

```text
GridEffectsOrgTorchMoveCommandlet.h
```

**`Source/GrimrockPrototypeEditor/Public/RPG/` — 17**

```text
RPGAlchemistAuthoring.h
RPGAlchemistAuthoringCommandlet.h
RPGClassProgressionAuthoring.h
RPGClassProgressionAuthoringCommandlet.h
RPGMageAuthoring.h
RPGMageAuthoringCommandlet.h
RPGPriestAuthoring.h
RPGPriestAuthoringCommandlet.h
RPGRangerAuthoring.h
RPGRangerAuthoringCommandlet.h
RPGRogueAuthoring.h
RPGRogueAuthoringCommandlet.h
RPGSkillCatalogAuthoring.h
RPGSkillCatalogAuthoringCommandlet.h
RPGWarriorAuthoring.h
RPGWarriorAuthoringCommandlet.h
UIRPGTalentTreeAuthoringCommandlet.h
```

**`Source/GrimrockPrototypeEditor/Public/UI/` — 5**

```text
SkillAffinityAuditCommandlet.h
SkillAffinityAuthoringCommandlet.h
SkillAffinityProductionAuthoring.h
UIRPGPresentationAuthoringCommandlet.h
UIRPGTalentPresentationAuthoring.h
```

</details>

<details>
<summary>Documentation docs/ — 1439 entrées</summary>


**`docs/` — 20**

[référence ou spécification : relecture requise]

```text
00_README_Art_Materials_Textures.md
01_Content_Structure.md
02_Texture_Pipeline_BC_N_ORM.md
03_M_GrimrockSurface_Masked_Master.md
04_Material_Instances_Migration.md
05_Static_Mesh_Blender_UE5_5_4_Pipeline.md
Architecture_LevelAsset_Editor_Runtime.md
Architecture_Runtime_Editor_Split.md
GridObjectArchetypeAsset_Conventions.md
M_GrimrockSurface_Master.md
Runtime_EdgeInteractionRules.md
Runtime_ItemPickupRules.md
Runtime_Test_Button_Door.md
TRAP015_MOUSE_INTERACTION_AUDIT.md
```

[référence à maintenir]

```text
FONT_LICENCES.md
PROJECT_WORKFLOW_RULES.md
README.md
SCRIPTS_REFERENCE.md
UI_WBP_LOCALIZATION_WORKFLOW.md
```

[spécification à confronter]

```text
UI_UMG_PRESENTATION_CONTRACT.md
```


**`docs/Architecture/` — 565**

[historique à condenser, prescriptions à relire]

```text
ARMEMENT01_17_1_4_SHIPPING_OIL_COOK_FIX.md
ARM_CLEAN01_WEAPON_AUTHORITY_AUDIT.md
ARM_CLEAN03_COMPACT_WEAPON_PROGRESSION.md
ARM_CLEAN04_EDITOR_QUICK_VIEW.md
ARM_CLEAN05_EVERY_EXPOSED_PROPERTY.md
ARM_CLEAN05_WEAPON_ASSET_ERD.md
ARM_CLEAN05_WEAPON_DATA_ASSET_AUDIT.md
ARM_CLEAN06_0_BASELINE.md
ARM_CLEAN06_1_CANONICAL_CPP.md
ARM_CLEAN06_2AA_PROVISIONAL_PRESENTATION_ASSEMBLY.md
ARM_CLEAN06_2AB_AUTHORIZED_HAND_ATTACK_RESOLVER.md
ARM_CLEAN06_2AC_UNIFIED_PROJECTILE_READ_MODEL.md
ARM_CLEAN06_2AD_SAVE_INSTANCE_ROUNDTRIP.md
ARM_CLEAN06_2AE_ARMOR_RESERVE_READ_MODEL.md
ARM_CLEAN06_2AF_INSTANCE_ARMOR_RESERVE_RESOLVER.md
ARM_CLEAN06_2AG_EQUIPPED_INSTANCE_ATTACK_IDENTITY.md
ARM_CLEAN06_2AH_RETIRE_SUPERSEDED_AUDITS.md
ARM_CLEAN06_2AI_RETIRE_UNAUTHORED_PRESENTATION_PROPOSALS.md
ARM_CLEAN06_2AJ_EXPLICIT_PRESENTATION_AUTHORING.md
ARM_CLEAN06_2AK_UE_ACTION_READINESS_STATUS.md
ARM_CLEAN06_2AL_SHARED_NEUTRAL_PRESENTATION.md
ARM_CLEAN06_2AM1_AUTHORING_DRY_RUN_TEST.md
ARM_CLEAN06_2AM_CANONICAL_ASSET_STAGE.md
ARM_CLEAN06_2AN_NATIVE_STAGE_PUBLICATION.md
ARM_CLEAN06_2A_SOURCE_PARITY.md
ARM_CLEAN06_2B_PRESENTATION_PARITY.md
ARM_CLEAN06_2C_NUMERIC_PROJECTION.md
ARM_CLEAN06_2D_CLASS_AFFINITY.md
ARM_CLEAN06_2E_HAND_SEMANTICS.md
ARM_CLEAN06_2F_CANONICAL_ARM_HAND_ACTIONS.md
ARM_CLEAN06_2G_PRODUCTION_ACTION_PRESENTATION.md
ARM_CLEAN06_2H_PRODUCTION_ACTION_PROJECTION.md
ARM_CLEAN06_2I_SINGLE_BOUND_ACTION_RESOLVER.md
ARM_CLEAN06_2J_VISUAL_SOURCE_PROJECTION.md
ARM_CLEAN06_2K_ACTION_ICON_PALETTE.md
ARM_CLEAN06_2L_PRODUCTION_ITEM_IDENTITY.md
ARM_CLEAN06_2M_CLASS_AFFINITY_PROJECTION.md
ARM_CLEAN06_2N_ACTIVE_VISUAL_REFERENCES.md
ARM_CLEAN06_2P_PRODUCTION_CANDIDATE.md
ARM_CLEAN06_2Q_PRODUCTION_ACTIONS_CANDIDATE.md
ARM_CLEAN06_2R_HAND_KIND_AUTHORING_GATE.md
ARM_CLEAN06_2S_NUMERIC_AND_PRODUCTION_SEMANTICS.md
ARM_CLEAN06_2T_OFFENSIVE_ACTION_CUTOVER_GATE.md
ARM_CLEAN06_2U_FOUR_CATALOGUE_PREFLIGHT.md
ARM_CLEAN06_2V_REAL_ARROW_AMMUNITION.md
ARM_CLEAN06_2W_THROW_PROJECTILE_PHYSICS.md
ARM_CLEAN06_2X_PROJECTILE_VISUAL_PARITY.md
ARM_CLEAN06_2Y_COMPLETE_TRANSIENT_PREVIEW.md
ARM_CLEAN06_2Z_AUTHORED_ACTION_PRESENTATION_KEY_PREVIEW.md
ARM_CLEAN06_3AA_CANONICAL_TOOLTIP_PROFILES.md
ARM_CLEAN06_3AB_CANONICAL_GUARD_IDENTITY.md
ARM_CLEAN06_3AC_CANONICAL_INVENTORY_DIAGNOSTICS.md
ARM_CLEAN06_3AD_EQUIPMENT_DIAGNOSTIC_PARITY.md
ARM_CLEAN06_3AE_CANONICAL_EQUIPMENT_PREVIEW.md
ARM_CLEAN06_3AF_STARTING_KIT_DISPLAY.md
ARM_CLEAN06_3AG_CANONICAL_ITEM_MATERIALIZATION.md
ARM_CLEAN06_3AH_MONSTER_LOOT_LEVELS.md
ARM_CLEAN06_3AI_CANONICAL_RECEPTACLE_ITEMS.md
ARM_CLEAN06_3AJ_WORLD_ITEM_LEVELS.md
ARM_CLEAN06_3AK_EDITOR_PREVIEW_WORLD_LEVEL_PARITY.md
ARM_CLEAN06_3AL_EQUIPPED_ACTION_ICON_RUNTIME.md
ARM_CLEAN06_3AM_STARTING_KIT_CANONICAL_LEVELS.md
ARM_CLEAN06_3AN_EDITOR_AUTHORED_LEVELS.md
ARM_CLEAN06_3AO_STARTING_KIT_HAND_POLICY.md
ARM_CLEAN06_3AP_CANONICAL_DUAL_WIELD.md
ARM_CLEAN06_3AQ_EQUIPMENT_SLOT_UI.md
ARM_CLEAN06_3AR_COMBAT_HUD_ACTION_ICONS.md
ARM_CLEAN06_3AS_CANONICAL_INVENTORY_STACKS.md
ARM_CLEAN06_3AT_CANONICAL_SAVE_REHYDRATION.md
ARM_CLEAN06_3AU_RUNTIME_ITEM_CATEGORIES_AUDIT.md
ARM_CLEAN06_3AV_CANONICAL_SPECIALIZED_THROWS.md
ARM_CLEAN06_3AW_CANONICAL_CONTEXT_HOTBAR_READ.md
ARM_CLEAN06_3AX_CANONICAL_SELF_HEAL_USE_ACTIONS.md
ARM_CLEAN06_3AY_CANONICAL_USE_REMOVE_STATUS.md
ARM_CLEAN06_3AZ_CANONICAL_USE_APPLY_STATUS.md
ARM_CLEAN06_3A_NATIVE_STARTUP_REGISTRY.md
ARM_CLEAN06_3BA_CANONICAL_ALLY_USE_TARGETING.md
ARM_CLEAN06_3BB_CANONICAL_RANGED_AMMUNITION.md
ARM_CLEAN06_3BC_CANONICAL_ARROW_PERSISTENCE.md
ARM_CLEAN06_3BD_CANONICAL_WORLD_PICKUP_IDENTITY.md
ARM_CLEAN06_3BE_COMBAT_PHYSICAL_THROW.md
ARM_CLEAN06_3BF_SHARED_UTILITY_THROW_PREFLIGHT.md
ARM_CLEAN06_3BG_RECEPTACLE_AND_064_READINESS.md
ARM_CLEAN06_3B_REMOVE_ONE_SHOT_AUTHORING.md
ARM_CLEAN06_3C_CANONICAL_EQUIPPED_HOTBAR_BINDINGS.md
ARM_CLEAN06_3D_CANONICAL_PLAYER_ACTION_CONTRIBUTIONS.md
ARM_CLEAN06_3E_CANONICAL_OFFENSIVE_RESOLUTION.md
ARM_CLEAN06_3F_VERIFY_STARTUP_CONFIG.md
ARM_CLEAN06_3G2_MISSING_DEFINITION_DIAGNOSTIC.md
ARM_CLEAN06_3G_CANONICAL_HAND_EQUIPMENT.md
ARM_CLEAN06_3H_CANONICAL_EQUIPMENT_ARMOR.md
ARM_CLEAN06_3I_CANONICAL_CARRY_WEIGHT.md
ARM_CLEAN06_3J_CANONICAL_INVENTORY_ICONS.md
ARM_CLEAN06_3K_CANONICAL_INVENTORY_METADATA.md
ARM_CLEAN06_3L_CANONICAL_HELD_VISUAL.md
ARM_CLEAN06_3M_CANONICAL_WORLD_VISUAL.md
ARM_CLEAN06_3N_CANONICAL_WORLD_WEIGHT.md
ARM_CLEAN06_3O_CANONICAL_SORT_PROJECTION.md
ARM_CLEAN06_3P_RECEPTACLE_WORLD_VISUAL.md
ARM_CLEAN06_3Q_WORLD_PHYSICS_MASS.md
ARM_CLEAN06_3R_PROJECTILE_VISUALS.md
ARM_CLEAN06_3S_PROJECTILE_LIFETIME_AND_IMPACT.md
ARM_CLEAN06_3T_THROW_BALLISTICS.md
ARM_CLEAN06_3U_PHYSICAL_THROW_STRENGTH.md
ARM_CLEAN06_3V_LEVEL_ZERO_AMMUNITION.md
ARM_CLEAN06_3W1_INVALID_INSTANCE_CLASSIFICATION.md
ARM_CLEAN06_3W_AMMO_FLIGHT_MESH.md
ARM_CLEAN06_3X_CANONICAL_LOOT_METADATA.md
ARM_CLEAN06_3Y_STARTING_KIT_METADATA.md
ARM_CLEAN06_3Z_EDITOR_LOOSE_ITEM_PREVIEW.md
ARM_CLEAN06_3_REMAINING_LEGACY_READERS_AUDIT.md
ARM_CLEAN06_4A_READ_ONLY_ASSET_RECAPTURE.md
ARM_CLEAN06_4B_PRIVATE_PARITY_PREFLIGHT.md
ARM_CLEAN06_4C_PRIVATE_DELTA_QUALIFICATION.md
ARM_CLEAN06_4D_LIVE_EDITOR_DRYRUN.md
ARM_CLEAN06_4E_AUTHORED_PRESENTATION_PARITY.md
ARM_CLEAN06_4F_THREE_WAY_USER_AUDIO_PROVENANCE.md
ARM_CLEAN06_4G_BINARY_TRANSACTION_GUARD.md
ARM_CLEAN06_4H_WORLD_REFERENCES_READ_ONLY.md
ARM_CLEAN06_4I_DISTINCT_IDENTITY_GAPS.md
ARM_CLEAN06_4J_NONARM_TRANSIENT_PROJECTION.md
ARM_CLEAN06_4K_NONARM_FIELD_OWNERSHIP.md
ARM_CLEAN06_4L_NONARM_PRESENTATION_DATA_PARITY.md
ARM_CLEAN06_4M_NONARM_READING_WORLD_PHYSICS_PARITY.md
ARM_CLEAN06_4N_NONARM_GENERIC_THROW_PARITY.md
ARM_CLEAN06_4O_NONARM_ACTION_SEMANTIC_INVENTORY.md
ARM_CLEAN06_4P_NONARM_EQUIPMENT_PARITY.md
ARM_CLEAN06_4Q_NONARM_STACK_REGISTRY_PARITY.md
ARM_CLEAN06_4R_NONARM_ACTION_LOSSLESS_BRIDGE.md
ARM_CLEAN06_4S_TRANSACTION_REHEARSAL.md
ARM_CLEAN06_4T_REAL_NONARM_ASSET_MIGRATION.md
ARM_CLEAN06_4U_NONARM_QUICKITEM_REAL_MIGRATION.md
ARM_CLEAN06_4V_LAST_NONARM_EQUIPMENT_MIGRATION.md
ARM_CLEAN06_4W_REAL_WORLD_ITEM_REFERENCES.md
ARM_CLEAN06_4X10_CANONICAL_TYPED_ITEM_SPAWNS.md
ARM_CLEAN06_4X11_REAL_PALETTE_READONLY_PREFLIGHT.md
ARM_CLEAN06_4X12_CANONICAL_PALETTE_PLACEMENT.md
ARM_CLEAN06_4X13_PALETTE_VISIBLE_PRESENTATION_PARITY.md
ARM_CLEAN06_4X14_CANONICAL_SLATE_PALETTE.md
ARM_CLEAN06_4X14_PIE_SMOKE_RECIPE.md
ARM_CLEAN06_4X15_PUZZLE_ACCEPTANCE_READONLY_INVENTORY.md
ARM_CLEAN06_4X1610_CANONICAL_WORLD_RESTORE.md
ARM_CLEAN06_4X1611_CANONICAL_PIT_TRANSFER.md
ARM_CLEAN06_4X1612_REAL_ARM_PIT_RESTORE.md
ARM_CLEAN06_4X1613_PIT_FLOOR_ORDER_AND_LANDING.md
ARM_CLEAN06_4X161_CANONICAL_RECEPTACLE_RULE_READER.md
ARM_CLEAN06_4X162_EDITOR_ACCEPTANCE_AUTHORING.md
ARM_CLEAN06_4X163_MANUAL_EDITOR_ACCEPTANCE_RECIPE.md
ARM_CLEAN06_4X163_RECEPTACLE_INITIAL_ITEM_EDITOR_FIX.md
ARM_CLEAN06_4X164_ACCEPTANCE_TRANSACTION_PREFLIGHT.md
ARM_CLEAN06_4X165_LOCK_KEY_ACCEPTANCE_READONLY_PREFLIGHT.md
ARM_CLEAN06_4X1661_REAL_ACCEPTANCE_ASSET_MANIFEST.md
ARM_CLEAN06_4X1662_PUBLISH_VERIFIED_ACCEPTANCE_MANIFEST.md
ARM_CLEAN06_4X1663_EXACT_ACCEPTANCE_PACKAGE_GATE.md
ARM_CLEAN06_4X166_ACCEPTANCE_MIGRATION_GATE.md
ARM_CLEAN06_4X169_SAVEGAME_MEMORY_ROUNDTRIP.md
ARM_CLEAN06_4X17_TORCH_HAND_EQUIPMENT_REGRESSION.md
ARM_CLEAN06_4X19_FINAL_LEGACY_DEPENDENCY_AUDIT.md
ARM_CLEAN06_4X1_RUNTIME_EDITOR_CUTOVER_PREFLIGHT.md
ARM_CLEAN06_4X20_ARM_WORLD_PRESENTATION_PRECUTOVER.md
ARM_CLEAN06_4X21_RECEPTACLE_POINTERLESS_INITIAL_ARM.md
ARM_CLEAN06_4X22_RECEPTACLE_POINTERLESS_RESTORE.md
ARM_CLEAN06_4X23_POINTERLESS_HELD_TORCH_LIGHT.md
ARM_CLEAN06_4X24_POINTERLESS_CANONICAL_STARTING_KITS.md
ARM_CLEAN06_4X25_POINTERLESS_EDITOR_WORLD_PREVIEW.md
ARM_CLEAN06_4X26_EDITOR_POINTERLESS_VALIDATION.md
ARM_CLEAN06_4X27_EDITOR_CANONICAL_SELECTED_OBJECT.md
ARM_CLEAN06_4X28_POINTERLESS_CANONICAL_PALETTE.md
ARM_CLEAN06_4X29_CANONICAL_WALL_LOCK_KEYS.md
ARM_CLEAN06_4X30_POINTERLESS_LOCK_INSERTED_KEY.md
ARM_CLEAN06_4X31_CANONICAL_RECEPTACLE_ITEM_TYPE.md
ARM_CLEAN06_4X32_RECEPTACLE_PHYSICAL_ACCEPTANCE_PREFLIGHT.md
ARM_CLEAN06_4X33_CANONICAL_WORLD_ACTOR_PHYSICAL_LEVEL.md
ARM_CLEAN06_4X34_CANONICAL_PROJECTILE_PHYSICAL_LEVEL.md
ARM_CLEAN06_4X35_PHYSICAL_THROW_SPAWN_LEVEL.md
ARM_CLEAN06_4X36_POINTERLESS_PROJECTILE_PRESENTATION.md
ARM_CLEAN06_4X371_BACKUP_RECEIPT_GUARD.md
ARM_CLEAN06_4X372_LOCAL_EDITOR_BUILD_GATE.md
ARM_CLEAN06_4X37_NATIVE_ARM_WORLD_PROFILES.md
ARM_CLEAN06_4X38_NATIVE_ALREADY_EQUAL_NOOP.md
ARM_CLEAN06_4X39_REAL_ARM_LOOSE_RUNTIME_CUTOVER.md
ARM_CLEAN06_4X3_AUTHORED_LEVEL_RULE.md
ARM_CLEAN06_4X40_CANONICAL_RUNTIME_ENTRY_SOURCE.md
ARM_CLEAN06_4X41_EQUIPPED_CANONICAL_LIGHT.md
ARM_CLEAN06_4X42_CANONICAL_HELD_PHYSICAL_INSTANCE.md
ARM_CLEAN06_4X43_STARTING_KIT_CANONICAL_AUTHORITY.md
ARM_CLEAN06_4X44_EDITOR_POINTERLESS_AUTHORING.md
ARM_CLEAN06_4X45_NATIVE_LOOSE_POINTER_PREFLIGHT.md
ARM_CLEAN06_4X46B_NATIVE_LOOSE_RESULT.md
ARM_CLEAN06_4X46_NATIVE_LOOSE_PROTECTED_APPLY.md
ARM_CLEAN06_4X47_POST_NATIVE_LOOSE_TEST_AND_ROLLBACK.md
ARM_CLEAN06_4X48_POST_NATIVE_RESIDUAL_AUDIT.md
ARM_CLEAN06_4X49_POINTERLESS_RECEPTACLE_RULE_AUTHORING.md
ARM_CLEAN06_4X4_REAL_AUTHORED_LEVEL_PREFLIGHT.md
ARM_CLEAN06_4X50_POINTERLESS_LOCK_OVERRIDE_AUTHORING.md
ARM_CLEAN06_4X51_SHARED_RULE_NATIVE_PREFLIGHT.md
ARM_CLEAN06_4X52_DUAL_STATE_SHARED_RULE_TESTS.md
ARM_CLEAN06_4X53_LOCK_INSPECTOR_POINTER_GUARD.md
ARM_CLEAN06_4X54_FOUR_NATIVE_RULE_PROTECTED_TRANSACTION.md
ARM_CLEAN06_4X55_NATIVE_PALETTE_PREFLIGHT.md
ARM_CLEAN06_4X56_PALETTE_DUAL_STATE_AUDITS.md
ARM_CLEAN06_4X57_NATIVE_PALETTE_PROTECTED_TRANSACTION.md
ARM_CLEAN06_4X58_INITIAL_CONTENT_PROTECTED_TRANSACTION.md
ARM_CLEAN06_4X59_DORMANT_OVERRIDES_PROTECTED_TRANSACTION.md
ARM_CLEAN06_4X5_NATIVE_PHYSICAL_LEVELS.md
ARM_CLEAN06_4X60_STONE_LOOT_PROTECTED_TRANSACTION.md
ARM_CLEAN06_4X61_FOUR_LOOT_CUTOVER.md
ARM_CLEAN06_4X62_ALCHEMY_SOURCE_AUDIT.md
ARM_CLEAN06_4X63_ALCHEMY_JOINT_PROJECTION.md
ARM_CLEAN06_4X64_SAVE_COPY_CONVERSION.md
ARM_CLEAN06_4X65_ALCHEMY_STARTERS_AND_AUTHORING_GUARD.md
ARM_CLEAN06_4X66C_COLD_RPG_SAVE_RESOLUTION.md
ARM_CLEAN06_4X66E_AUTOMATION_SAVE_ISOLATION.md
ARM_CLEAN06_4X66F_NATIVE_ROLLBACK_SAFETY.md
ARM_CLEAN06_4X66_NATIVE_ALCHEMY_CUTOVER.md
ARM_CLEAN06_4X6_CANONICAL_PRESENTATION_PROJECTION.md
ARM_CLEAN06_4X71_RECEPTACLE_MIGRATOR_AUDIT_ONLY.md
ARM_CLEAN06_4X72B_ISOLATED_VALIDATION_HOOK.md
ARM_CLEAN06_4X72_NATIVE_RECEPTACLE_MIGRATION.md
ARM_CLEAN06_4X73_RECEPTACLE_PIE_HANDOFF.md
ARM_CLEAN06_4X7_CANONICAL_ACTOR_INITIALIZATION.md
ARM_CLEAN06_4X81_LOCK_MIGRATOR_AUDIT_ONLY.md
ARM_CLEAN06_4X83_LOCK_MIGRATION_PREPARE.md
ARM_CLEAN06_4X841_TD073_LOCK_CANONICAL_COMPATIBILITY.md
ARM_CLEAN06_4X85_LOCK_PIE_HANDOFF.md
ARM_CLEAN06_4X8_REAL_LOOSE_ITEM_CUTOVER.md
ARM_CLEAN06_4X91_RECEPTACLE_INITIAL_IDENTITIES.md
ARM_CLEAN06_4X92_INITIAL_RECEPTACLE_CANONICAL_VISUALS.md
ARM_CLEAN06_5A_RETIREMENT_EVIDENCE.md
ARM_CLEAN06_5B_READABLE_SAVE_REPAIR.md
ARM_CLEAN06_5C_CANONICAL_STARTERS.md
ARM_CLEAN06_5D1_NONARM_THROW_READERS.md
ARM_CLEAN06_5D2_RECEPTACLE_CUTOVER.md
ARM_CLEAN06_5D3_ARM_HAND_THROW.md
ARM_CLEAN06_5D4_NONARM_EQUIPMENT.md
ARM_CLEAN06_5D5_ARM_CONTRIBUTION_RETIREMENT.md
ARM_CLEAN06_5D_EXPLORATION_ITEM_READERS.md
ARM_CLEAN06_5E1_POINTERLESS_TOOLTIP.md
ARM_CLEAN06_5E2_ATTACK_CONTRIBUTIONS.md
ARM_CLEAN06_5E3_CANONICAL_AMMO_LAUNCH.md
ARM_CLEAN06_5E4A_REHYDRATION_PRECONDITIONS.md
ARM_CLEAN06_5E4B_CANONICAL_ATTACK_PRESENTATION.md
ARM_CLEAN06_5E4C_CANONICAL_REHYDRATION.md
ARM_CLEAN06_5E4D_PHYSICAL_TRANSFER_RETIREMENT.md
ARM_CLEAN06_5F_ICON_PALETTE_RETIREMENT.md
ARM_CLEAN06_5G100_EDITOR_PREVIEW_DURABLE_INSTANCE_ONLY.md
ARM_CLEAN06_5G101_POST_SCHEMA_INSTANCE_EVIDENCE.md
ARM_CLEAN06_5G103_CANONICAL_CARRY_FIXTURES.md
ARM_CLEAN06_5G104_CANONICAL_INSTANCE_FIXTURES.md
ARM_CLEAN06_5G105_CANONICAL_READERS_PROJECTILE_FIXTURES.md
ARM_CLEAN06_5G106_CANONICAL_LIFECYCLE_FIXTURES.md
ARM_CLEAN06_5G107_CANONICAL_CAPACITY_FIXTURES.md
ARM_CLEAN06_5G108_CANONICAL_OWNERSHIP_FIXTURES.md
ARM_CLEAN06_5G109_CANONICAL_CURSOR_STACK_FIXTURES.md
ARM_CLEAN06_5G10_RETIRE_NATIVE_MIGRATION_ENTRYPOINTS.md
ARM_CLEAN06_5G110_CANONICAL_EQUIPMENT_CORE_FIXTURE.md
ARM_CLEAN06_5G111_CANONICAL_TWO_HAND_FIXTURES.md
ARM_CLEAN06_5G112_CANONICAL_RECRUITMENT_FIXTURES.md
ARM_CLEAN06_5G113_DURABLE_SHIELD_VISUAL_FIXTURES.md
ARM_CLEAN06_5G114R_REACH_FIXTURE_GEOMETRY.md
ARM_CLEAN06_5G114_CANONICAL_WORLD_TRANSFER_FIXTURES.md
ARM_CLEAN06_5G115_CANONICAL_LIGHT_STATE_FIXTURE.md
ARM_CLEAN06_5G116R_FAIL_CLOSED_FIXTURE_PARITY.md
ARM_CLEAN06_5G116_NO_REGISTRYLESS_MASS_FALLBACK.md
ARM_CLEAN06_5G117_LEGACY_FIXTURE_READ_MODEL.md
ARM_CLEAN06_5G118R_LEVEL_MASS_TEST_CORRECTION.md
ARM_CLEAN06_5G118_RUNTIME_CACHE_WRITERS_RETIREMENT.md
ARM_CLEAN06_5G119_TEST_FIXTURE_CACHE_REMOVAL.md
ARM_CLEAN06_5G11_RETIRE_HISTORICAL_PROJECTION_ENTRYPOINTS.md
ARM_CLEAN06_5G120_UI_METADATA_FAIL_CLOSED.md
ARM_CLEAN06_5G121_INVENTORY_SORT_FIXTURE.md
ARM_CLEAN06_5G122_ITEM_NAME_FIXTURE_RETIREMENT.md
ARM_CLEAN06_5G123_MON12_PHYSICAL_INSTANCE_NAMES.md
ARM_CLEAN06_5G124_EDITOR_INSTANCE_FIXTURES.md
ARM_CLEAN06_5G125_THROW_INSTANCE_MASS_WRITER_RETIREMENT.md
ARM_CLEAN06_5G126_STARTING_KIT_CACHE_WRITER_RETIREMENT.md
ARM_CLEAN06_5G127_STATUS_FIXTURE_CACHE_REMOVAL.md
ARM_CLEAN06_5G128_PHYSICAL_INSTANCE_CACHE_WRITERS.md
ARM_CLEAN06_5G1291_BUILD_FIXTURE_WRITERS.md
ARM_CLEAN06_5G129_INSTANCE_CPP_MEMBER_REMOVAL.md
ARM_CLEAN06_5G12_CANONICAL_MONSTER_LOOT_SCHEMA.md
ARM_CLEAN06_5G130_GUARD_UNKNOWN_FAIL_CLOSED.md
ARM_CLEAN06_5G131_FRONTEND_NEW_GAME_REGISTRY.md
ARM_CLEAN06_5G132_UNKNOWN_EQUIPMENT_FAIL_CLOSED.md
ARM_CLEAN06_5G1331_PREVIEW_TEST_REGISTRY_POLICY.md
ARM_CLEAN06_5G133_ARMOR_CONTRIBUTIONS_FAIL_CLOSED.md
ARM_CLEAN06_5G134_UNKNOWN_STACK_ADMISSION_FAIL_CLOSED.md
ARM_CLEAN06_5G135_DIRECT_MATERIALIZATION_REGISTRY_GATE.md
ARM_CLEAN06_5G13_STRICT_LOCK_KEY_IDS.md
ARM_CLEAN06_5G14_LOCK_KEY_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G15_STRICT_RECEPTACLE_ACCEPTANCE.md
ARM_CLEAN06_5G16_RECEPTACLE_ACCEPTANCE_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G17_STRICT_RECEPTACLE_INITIAL_CONTENT.md
ARM_CLEAN06_5G18_INITIAL_CONTENT_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G19_CANONICAL_ITEM_SPAWN_SCHEMA.md
ARM_CLEAN06_5G1_HUD_PALETTE_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G20_STRICT_LOOSE_ITEM_PREVIEW.md
ARM_CLEAN06_5G21_STRICT_LOOSE_ITEM_RUNTIME.md
ARM_CLEAN06_5G22_STRICT_LOOSE_ITEM_AUTHORING.md
ARM_CLEAN06_5G23_RETIRE_LEVEL_ITEM_LOOKUP.md
ARM_CLEAN06_5G24_LOOSE_ITEM_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G25_ITEM_AUTHORING_SCRATCH.md
ARM_CLEAN06_5G26_STRICT_PALETTE_PRESENTATION.md
ARM_CLEAN06_5G27_CANONICAL_PALETTE_TOOLS.md
ARM_CLEAN06_5G28_PALETTE_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G29_EDITOR_CANONICAL_LEVEL_POLICY.md
ARM_CLEAN06_5G2_CANONICAL_ARMAMENT_DETAILS.md
ARM_CLEAN06_5G30_EDITOR_BLUEPRINT_REFERENCE_AUDIT.md
ARM_CLEAN06_5G31_EDITOR_ITEM_API_RETIREMENT.md
ARM_CLEAN06_5G32_CANONICAL_PARTY_REHYDRATION.md
ARM_CLEAN06_5G33_RUNTIME_RECIPE_LOOKUP_RETIREMENT.md
ARM_CLEAN06_5G34_WORLD_SNAPSHOT_PREFLIGHT.md
ARM_CLEAN06_5G35_RETIRE_STARTER_RECIPE_RESOLVER.md
ARM_CLEAN06_5G36_STRICT_REHYDRATION_ISOLATION.md
ARM_CLEAN06_5G37_INVENTORY_BLUEPRINT_REFERENCE_AUDIT.md
ARM_CLEAN06_5G38_INVENTORY_REGISTRY_UNREFLECT.md
ARM_CLEAN06_5G39_INVENTORY_REGISTRATION_AUTOMATION_ONLY.md
ARM_CLEAN06_5G3_RETIRE_LEGACY_WEAPON_WRITERS.md
ARM_CLEAN06_5G40_STRICT_STARTER_REGISTRY.md
ARM_CLEAN06_5G41_CANONICAL_INVENTORY_DIAGNOSTICS.md
ARM_CLEAN06_5G42_INVENTORY_INGRESS_GATE.md
ARM_CLEAN06_5G43_EQUIPPED_WORLD_TRANSFER_GATE.md
ARM_CLEAN06_5G44_CANONICAL_HELD_LIGHT_RETIREMENT.md
ARM_CLEAN06_5G45_CANONICAL_DISARM_TOOLS.md
ARM_CLEAN06_5G46_CANONICAL_SURFACE_ITEMS.md
ARM_CLEAN06_5G47_CANONICAL_WALL_LOCK_TRANSFER.md
ARM_CLEAN06_5G48_RECEPTACLE_RESTITUTION.md
ARM_CLEAN06_5G49_RECEPTACLE_CANONICAL_INSERTION.md
ARM_CLEAN06_5G4_REGISTERED_ID_MAP_RETIREMENT.md
ARM_CLEAN06_5G50_CANONICAL_RECEPTACLE_MATERIALIZATION.md
ARM_CLEAN06_5G51_UNIFIED_RECEPTACLE_EXTRACTION.md
ARM_CLEAN06_5G52_ATOMIC_RECEPTACLE_CAPTURE.md
ARM_CLEAN06_5G53_ATOMIC_WORLD_ITEM_CAPTURE.md
ARM_CLEAN06_5G54_CANONICAL_WORLD_PICKUP.md
ARM_CLEAN06_5G55_PHYSICAL_PIT_GUARDS.md
ARM_CLEAN06_5G56_INBOUND_OWNERSHIP_COMMIT.md
ARM_CLEAN06_5G57_CANONICAL_PROJECTILE_LAUNCH.md
ARM_CLEAN06_5G58_CANONICAL_THROW_PREFLIGHT.md
ARM_CLEAN06_5G59_IN_FLIGHT_ITEM_CAPTURE.md
ARM_CLEAN06_5G5_STRICT_PHYSICAL_INVENTORY_LEVELS.md
ARM_CLEAN06_5G60_RETRYABLE_IMPACT.md
ARM_CLEAN06_5G61_CANONICAL_WEIGHT_AUTHORITY.md
ARM_CLEAN06_5G62_CANONICAL_WORLD_PHYSICS.md
ARM_CLEAN06_5G63_PUZZLE_CATEGORY_EVIDENCE.md
ARM_CLEAN06_5G64_CANONICAL_WORLD_SPARKLE.md
ARM_CLEAN06_5G65_PHYSICAL_IDENTITY_TAGS.md
ARM_CLEAN06_5G66_CANONICAL_DISARM_CONTEXT.md
ARM_CLEAN06_5G67_HOTBAR_PHYSICAL_SOURCE.md
ARM_CLEAN06_5G68_PUZZLE_EVIDENCE_EXPORT.md
ARM_CLEAN06_5G69_PUZZLE_CATEGORY_PREVIEW.md
ARM_CLEAN06_5G6_STRICT_WORLD_AND_LOOT_LEVELS.md
ARM_CLEAN06_5G70_PUZZLE_TAG_ISOLATION.md
ARM_CLEAN06_5G71_NATIVE_PUZZLE_CATEGORIES.md
ARM_CLEAN06_5G72_CANONICAL_RECEPTACLE_CATEGORY.md
ARM_CLEAN06_5G73_CANONICAL_PHYSICAL_METADATA.md
ARM_CLEAN06_5G74_CANONICAL_INTERACTION_CAPABILITIES.md
ARM_CLEAN06_5G75_INVENTORY_FIXTURE_ISOLATION.md
ARM_CLEAN06_5G76_RETIRE_PUZZLE_MIGRATION.md
ARM_CLEAN06_5G77_RECEPTACLE_BLUEPRINT_PREFLIGHT.md
ARM_CLEAN06_5G78_RETIRE_RECEPTACLE_LEGACY_SCHEMA.md
ARM_CLEAN06_5G79_RECEPTACLE_STATIC_CACHE_RETIREMENT.md
ARM_CLEAN06_5G7_STRICT_AUTHORING_AND_KIT_PREVIEW.md
ARM_CLEAN06_5G80_STRICT_STARTER_PROJECTIONS.md
ARM_CLEAN06_5G81_WORLD_SAVE_POINTER_ISOLATION.md
ARM_CLEAN06_5G82_POINTERLESS_PIT_FIXTURES.md
ARM_CLEAN06_5G83_POINTERLESS_PROJECTILE_PERSISTENCE.md
ARM_CLEAN06_5G84_STRICT_INVENTORY_PRESENTATION.md
ARM_CLEAN06_5G85_WORLD_SAVE_PREFLIGHT.md
ARM_CLEAN06_5G86_WORLD_SAVE_POINTER_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G87_INSTANCE_CACHE_BLUEPRINT_EVIDENCE.md
ARM_CLEAN06_5G88_INSTANCE_CACHE_SAVE_EVIDENCE.md
ARM_CLEAN06_5G89_CACHE_REFUSAL_DIAGNOSTIC.md
ARM_CLEAN06_5G90_WEIGHT_CACHE_PROVENANCE.md
ARM_CLEAN06_5G91_SEMANTIC_CACHE_PARITY.md
ARM_CLEAN06_5G92_1_BOUNDED_SCHEMA_PROOF.md
ARM_CLEAN06_5G92_INSTANCE_SCHEMA_BASELINE.md
ARM_CLEAN06_5G93_INSTANCE_CACHE_SCHEMA_RETIREMENT.md
ARM_CLEAN06_5G94_CANONICAL_INVENTORY_CACHE_WRITER_RETIREMENT.md
ARM_CLEAN06_5G95_GENERATION_CACHE_WRITER_RETIREMENT.md
ARM_CLEAN06_5G96_WORLD_RECEPTACLE_CACHE_WRITER_RETIREMENT.md
ARM_CLEAN06_5G97_PROJECTILE_CACHE_WRITER_RETIREMENT.md
ARM_CLEAN06_5G98_PAWN_GRANT_BLUEPRINT_PREFLIGHT.md
ARM_CLEAN06_5G99_RETIRE_PAWN_WEIGHT_GRANT.md
ARM_CLEAN06_5G9_RETIRE_MIGRATION_HOOKS.md
ARM_CLEAN06_5H10_AUTOMATION_ONLY_LEGACY_REGISTRATION.md
ARM_CLEAN06_5H11_AUTOMATION_ONLY_ACTOR_INITIALIZATION.md
ARM_CLEAN06_5H12_EVIDENCE_ACCEPTANCE_AND_STATIC_CONSUMERS.md
ARM_CLEAN06_5H13_ICON_PALETTE_CANARY_READINESS.md
ARM_CLEAN06_5H14_ICON_PALETTE_PRIVATE_BINARY_SNAPSHOT.md
ARM_CLEAN06_5H15_PALETTE_CONTINUITY_PREFLIGHT.md
ARM_CLEAN06_5H16_NATIVE_PALETTE_CANARY_POSTCHECK.md
ARM_CLEAN06_5H17_PERMANENT_ICON_RETIREMENT_GUARD.md
ARM_CLEAN06_5H18_SHIPPING_C4702_FIX.md
ARM_CLEAN06_5H19_AUTOMATIC_SHIPPING_BUILD_GATE.md
ARM_CLEAN06_5H1_RETIREMENT_DISPOSITION.md
ARM_CLEAN06_5H20_SHIPPING_VISUAL_RECIPE_CORRECTION.md
ARM_CLEAN06_5H21_EDITOR_CANONICAL_LOOT_ALCHEMY_FIXTURES.md
ARM_CLEAN06_5H22_CANONICAL_MONSTER_LOOT_FIXTURES.md
ARM_CLEAN06_5H23_HISTORICAL_READER_INVENTORY.md
ARM_CLEAN06_5H24_CANONICAL_FLASK_TORCH_TESTS.md
ARM_CLEAN06_5H24_STARTER_SOURCE_RETIREMENT.md
ARM_CLEAN06_5H25_CANONICAL_SHURIKEN_PROJECTILE_TEST.md
ARM_CLEAN06_5H25_NATIVE_RETIREMENT_RECEIPT.md
ARM_CLEAN06_5H26_CANONICAL_ISSUE181_DISARM_FIXTURE.md
ARM_CLEAN06_5H26_SOURCE_CATALOGUE_RETIREMENT.md
ARM_CLEAN06_5H27_CANONICAL_TORCH_FLOOR_PICKUP_TEST.md
ARM_CLEAN06_5H28_CANONICAL_DUAL_HAND_TORCH_TEST.md
ARM_CLEAN06_5H29_CANONICAL_OIL_ACTION_TOOLTIP_TESTS.md
ARM_CLEAN06_5H2_EDITOR_ICON_FIXTURE_CANONICAL.md
ARM_CLEAN06_5H30_CANONICAL_TORCH_NIAGARA_RUNTIME_FIXTURE.md
ARM_CLEAN06_5H32_TRANSMUTER_CANONICAL_STARTER_SAVE.md
ARM_CLEAN06_5H33_SURFACEWEAVER_CANONICAL_STARTER_SAVE.md
ARM_CLEAN06_5H34_CANONICAL_SABOTEUR_DISARM_STARTER_SAVE.md
ARM_CLEAN06_5H35_CANONICAL_SHIELD_VICTORY_REFILL.md
ARM_CLEAN06_5H36_CANONICAL_SHIELD_REST_FIXTURE.md
ARM_CLEAN06_5H37_CANONICAL_SHIELD_ABILITY_RESTORE.md
ARM_CLEAN06_5H38_CANONICAL_SHIELD_SURFACE_TESTS.md
ARM_CLEAN06_5H39_CANONICAL_RECEPTACLE_DAGGER_LEVEL_TESTS.md
ARM_CLEAN06_5H3_APPROVED_STARTER_AUTOMATION_GATE.md
ARM_CLEAN06_5H40_CANONICAL_ARMOR_EQUIPMENT_COMBAT_TESTS.md
ARM_CLEAN06_5H41_CANONICAL_SHIELD_COMBAT_TOOLTIP_TESTS.md
ARM_CLEAN06_5H42_CANONICAL_SHORTBOW_HAND_POLICY_TEST.md
ARM_CLEAN06_5H43_CANONICAL_TOOLTIP_NATIVE_PRESENTER.md
ARM_CLEAN06_5H44_CANONICAL_EQUIPMENT_TOOLTIP_HAND_LABELS.md
ARM_CLEAN06_5H45_CANONICAL_GUARD_PARRY_BLOCK_TESTS.md
ARM_CLEAN06_5H46_CANONICAL_GREATAXE_SHIELD_PREVIEW.md
ARM_CLEAN06_5H47_CANONICAL_SHIELD_TOOLTIP_DEPLETION.md
ARM_CLEAN06_5H48_CANONICAL_MONSTER_GUARD_INTEGRATION.md
ARM_CLEAN06_5H49_CANONICAL_WEAPON_TOOLTIP_LEVELS_PASSIVE.md
ARM_CLEAN06_5H4_NATIVE_CANDIDATE_CLASSIFICATION.md
ARM_CLEAN06_5H50_CANONICAL_WEAPON_TOOLTIP_INVENTORY_SLOT.md
ARM_CLEAN06_5H51_CANONICAL_WEAPON_SHIELD_COMPARISON.md
ARM_CLEAN06_5H52_CANONICAL_PHYSICAL_SUBTYPE_TOOLTIP.md
ARM_CLEAN06_5H53_CANONICAL_ARCHER_KIT.md
ARM_CLEAN06_5H54_CANONICAL_DAGGER_SHIELD_STARTER.md
ARM_CLEAN06_5H55_CANONICAL_STARTER_KIT_ATOMIC_REJECTIONS.md
ARM_CLEAN06_5H56_CANONICAL_MON12_SHADOW_OFFHAND_COMBAT.md
ARM_CLEAN06_5H57_CANONICAL_MELEE_N1_EXECUTION.md
ARM_CLEAN06_5H58_CANONICAL_BOWS_AMMO_PROJECTILES.md
ARM_CLEAN06_5H59_CANONICAL_MAGIC_WEAPONS_N1.md
ARM_CLEAN06_5H5_FULL_PRIVATE_RETIREMENT_EVIDENCE.md
ARM_CLEAN06_5H60_CANONICAL_PASSIVE_N1_EQUIPMENT.md
ARM_CLEAN06_5H61_CANONICAL_EQUIPPED_THROW_N1.md
ARM_CLEAN06_5H62_CANONICAL_INVENTORY_PHYSICAL_THROW_AIMING.md
ARM_CLEAN06_5H63_CANONICAL_PIT_DAGGER_PROJECTILE.md
ARM_CLEAN06_5H64_CANONICAL_PILOT_EQUIPMENT_TESTS.md
ARM_CLEAN06_5H65_CANONICAL_DUAL_ATTACK_ICON_TEST.md
ARM_CLEAN06_5H66_CANONICAL_DUAL_WIELD_AUDIO_SEQUENCE.md
ARM_CLEAN06_5H67_CANONICAL_SHURIKEN_PHYSICAL_THROW_TEST.md
ARM_CLEAN06_5H68_PRODUCTION_UI_LEGACY_FALLBACK_REMOVAL.md
ARM_CLEAN06_5H69_HISTORICAL_VISUAL_RESOLVERS_AUTOMATION_ONLY.md
ARM_CLEAN06_5H6_SHIPPING_LEGACY_STARTER_RETIREMENT.md
ARM_CLEAN06_5H70_DUAL_WIELD_LEGACY_GATE.md
ARM_CLEAN06_5H71_PLAYER_COMBAT_CATALOGUE_SHIPPING_GATE.md
ARM_CLEAN06_5H72_RANGED_AMMO_HISTORICAL_GATE.md
ARM_CLEAN06_5H7_REGISTERED_WORLD_ITEM_SPAWN_GUARD.md
ARM_CLEAN06_5H8_LEGACY_INVENTORY_LOOKUP_GATE.md
ARM_CLEAN06_5H9_OLD_ITEM_ACTOR_DIRECT_INIT_GUARD.md
ARM_CLEAN06_5_COMBAT_EXECUTION_CUTOVER.md
ARM_CLEAN06_5_EDITOR_CANONICAL_TEST_CUTOVER.md
ARM_CLEAN06_5_EDITOR_PRODUCTION_FINAL_STATE_TESTS.md
ARM_CLEAN06_5_EDITOR_STARTER_KIT_CANONICAL_TEST_CUTOVER.md
ARM_CLEAN06_5_EDITOR_WORLD_ACCEPTANCE_CANONICAL_CUTOVER.md
ARM_CLEAN06_5_INDUSTRIAL_CANONICAL_TEST_CUTOVER.md
ARM_CLEAN06_5_INVENTORY_PRESENTATION_CUTOVER.md
ARM_CLEAN06_5_LOOT_WORLD_FINAL_STATE_TESTS.md
ARM_CLEAN06_5_OLD_WARRIOR_THROW_MAGIC_TESTS.md
ARM_CLEAN06_5_READINESS_LEVELS_KITS_FINAL_TESTS.md
ARM_CLEAN06_5_RETIRED_ALCHEMY_LOOT_PROJECTIONS.md
ARM_CLEAN06_5_RETIRED_OLD_RUNTIME_AUTHORING_TESTS.md
ARM_CLEAN06_5_RETIREMENT_CONSUMER_TRIAGE.md
ARM_CLEAN06_5_RETIRE_INTERMEDIATE_PROJECTION_HELPERS.md
ARM_CLEAN06_5_RETIRE_OLD_WEAPON_WRITERS.md
ARM_CLEAN06_5_RETIRE_RPG_LEGACY_ITEM_AUTHORS.md
ARM_CLEAN06_5_RETIRE_TRANSIENT_PRODUCTION_CANDIDATES.md
ARM_CLEAN06_5_UI_INVENTORY_READERS_CUTOVER.md
ARM_CLEAN06_CURRENT_HANDOFF.md
ITEM_THROW_MIG01_SHURIKEN_AUTHORITY.md
PAWN_CLEAN02_EDITOR_SURFACE.md
TD07_FINAL_QUANTITATIVE_AUDIT_BASELINE.md
WORLDOBJ_MIG04_GENERIC_MOTION.md
WORLDOBJ_MIG05_SINGLE_COLLECTIBLE_DEFINITION.md
WORLDOBJ_MIG06_DEFINITION_INSTANCE_BEHAVIOR.md
WORLDOBJ_MIG07B_TYPED_AUTHORITY_BRIDGE.md
WORLDOBJ_MIG07C_TYPED_LIFECYCLE_AND_EDITOR_WRITE_THROUGH.md
WORLDOBJ_MIG07_TYPED_LEVEL_PLACEMENTS.md
WORLDOBJ_MIG08_ASSET_MIGRATION.md
WORLDOBJ_MIG09_C_C1_RUNTIME_TEST_DETACH.md
WORLDOBJ_MIG09_E2B_TYPED_RUNTIME_CONSUMERS.md
WORLDOBJ_MIG09_E2C_EDITOR_AUTHORITY.md
WORLDOBJ_MIG09_FINAL_C.md
WORLDOBJ_MIG09_FINAL_D.md
WORLDOBJ_MIG09_LEGACY_PURGE.md
WORLDOBJ_MIG10_FINAL.md
```

[référence ou spécification : relecture requise]

```text
ADVANCED_DUNGEON_LOGIC_FOUNDATION.md
ALIGN_A_DEAD_CODE_AND_ASSET_HYGIENE_REPORT.md
ALIGN_B5_PLACEMENT_FINAL.md
ARCHITECTURE_CONSISTENCY_AUDIT.md
ARCHITECTURE_REBASELINE_2026_10_09.md
ARMEMENT_PIPELINE_GUIDE.md
COMBAT_MONSTER_AI_FOUNDATION.md
CORE_CLEANUP_NOTES.md
CORE_DUNGEON_LEVEL_GRID.md
CPP_GENERAL_AUDIT_2026_09_27.md
DOOR_MECHANISM_CLEANUP_NOTES.md
DOOR_MECHANISM_FOUNDATION.md
EFFECTS_ORG01_1_AUDIT_INVENTAIRE.md
EFFECTS_ORG01_2A_DEPENDENCY_PLAN.md
EFFECTS_ORG01_2B_AUTOMATED_POTION_RENAME.md
EFFECTS_ORG01_2D_TORCH_NIAGARA_RELOCATION.md
EFFECTS_ORG01_3_UE_SETUP_PIE.md
FIREFX01_FIRE_CRACKLE_AUDIO.md
FIREFX03A_LOCAL_PROFILE_OVERRIDE.md
FIREFX04_IGNITION_BURST.md
FIREFX05_VERSIONED_ASSETS.md
FIREFX09_PROFILE_OVERRIDES.md
FIREFX10_CANONICAL_FIRE_RUNTIME.md
FIREFX11_PROGRESSIVE_GROUND_FIRE.md
FLASK04_VISIBLE_BOTTLES.md
FLASK_FIREFX_GUIDE_UE554.md
FLASK_GLASS01_NATIVE_SHATTER.md
ISSUE181_CHAOS_WORLD_ITEMS.md
ITEM_LIGHT01_DATA_DRIVEN_ITEM_LIGHT.md
ITEM_PICKUP_AND_PLACEMENT_CLEANUP_NOTES.md
ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md
ITEM_VFX014A_LEGACY_AUDIT_AND_SPARKLE_POLICY.md
ITEM_VFX014B_REMOVE_LEGACY_LIGHT_NIAGARA.md
ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md
LEVEL_VALIDATION_PANEL_CLEANUP_NOTES.md
LEVEL_VALIDATION_PANEL_FOUNDATION.md
LIGHT_CONFIG02_SINGLE_POINT_LIGHT_AUTHORITY.md
LINK_EVENT_COMMAND_CLEANUP_NOTES.md
LINK_EVENT_COMMAND_FOUNDATION.md
MAGIC_STATUS_EFFECTS_FOUNDATION.md
MATERIAL_OWNERSHIP.md
MOUSE_INTERACTION_CLEANUP_NOTES.md
MOUSE_INTERACTION_FOUNDATION.md
OBJECT_SYSTEM_CLEANUP_NOTES.md
PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION.md
PARTY_RPG_RECRUITMENT_FOUNDATION.md
READABLE_OBJECTS_AND_FEEDBACK_CLEANUP_NOTES.md
READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md
RECEPTACLE_SYSTEM_CLEANUP_NOTES.md
RECEPTACLE_SYSTEM_FOUNDATION.md
SAVE_PERSISTENCE_FOUNDATION.md
STARTUP_FLOW01_FRONTEND_DUNGEON.md
SURFACE_AND_CATALYST_VFX_PROFILES.md
TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md
TECHNICAL_DEBT_REGISTER.md
TEST_AUTOMATION_FOUNDATION.md
UI_GAME_FLOW_FOUNDATION.md
VALIDATION_AUTO01_WINDOWS_UE_AGENT.md
WORLDOBJ_ITEMCLASS01_ITEM_ACTOR_AUTHORITY.md
WORLDOBJ_MIGRATION_ROADMAP_AND_TARGET_DATA_MODEL.md
WORLDOBJ_MOVINGPARTS01_GENERIC_ARRAY.md
WORLDOBJ_RECOVERY01_FINAL.md
WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md
```

[référence à maintenir]

```text
ARCHITECTURE_INDEX.md
PROJECT_SYNTHESIS.md
```

[spécification à confronter]

```text
ARMEMENT_PIPELINE_CONTRACT.md
ARM_CLEAN06_4X18_TORCH_LIGHT_STATE_CONTRACT.md
ARM_CLEAN06_5G8_SINGLE_PHYSICAL_LEVEL_CONTRACT.md
ARM_CLEAN06_5H31_CANONICAL_TRANSMUTER_OIL_CONTRACT.md
ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md
ARM_CLEAN06_MIGRATION_PLAN.md
```


**`docs/Architecture/Maps/` — 3**

[référence ou spécification : relecture requise]

```text
Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

[référence à maintenir]

```text
GRIMROCK_PROJECT_MAP.md
GRIMROCK_PROJECT_MAP_MERMAID.md
```


**`docs/ArtBook/` — 12**

[ressource / provenance à conserver]

```text
Bestiaire_des_Profondeurs_Volume_III_Les_Chambres_du_Pouvoir.md
Bestiaire_des_Profondeurs_Volume_III_Les_Chambres_du_Pouvoir.pdf
Bestiaire_des_Profondeurs_Volume_III_Les_Chambres_du_Pouvoir_base.pdf
Bestiaire_des_Profondeurs_Volume_II_Les_Salles_Interdites.md
Bestiaire_des_Profondeurs_Volume_II_Les_Salles_Interdites.pdf
Bestiaire_des_Profondeurs_Volume_II_Les_Salles_Interdites_base.pdf
Bestiaire_des_Profondeurs_Volume_IV_Les_Frontieres_Sauvages.md
Bestiaire_des_Profondeurs_Volume_IV_Les_Frontieres_Sauvages.pdf
Bestiaire_des_Profondeurs_Volume_IV_Les_Frontieres_Sauvages_base.pdf
Bestiaire_des_Profondeurs_Volume_I_FINAL.md
Bestiaire_des_Profondeurs_Volume_I_FINAL.pdf
Bestiaire_des_Profondeurs_Volume_I_illustre.md
```


**`docs/ArtBook/Images/` — 49**

[ressource / provenance à conserver]

```text
Gardien de la crypte.png
bestiaire_araignee_mineure.png
bestiaire_champignon_toxique.png
bestiaire_gardien_de_la_crypte.png
bestiaire_gargouille.png
bestiaire_golem_de_pierre.png
bestiaire_mimique.png
bestiaire_rat_geant.png
bestiaire_slime_vert.png
bestiaire_squelette_archer.png
bestiaire_squelette_guerrier.png
bestiaire_ver_des_cryptes.png
bestiaire_volume_ii_01_gobelin_pillard.png
bestiaire_volume_ii_02_gobelin_lanceur.png
bestiaire_volume_ii_03_cultiste.png
bestiaire_volume_ii_04_oeil_flottant.png
bestiaire_volume_ii_05_spectre_mineur.png
bestiaire_volume_ii_06_goule.png
bestiaire_volume_ii_07_serpent_venimeux.png
bestiaire_volume_ii_08_loup_des_cavernes.png
bestiaire_volume_ii_09_armure_animee.png
bestiaire_volume_ii_10_chien_infernal_mineur.png
bestiaire_volume_ii_11_elementaire_de_glace.png
bestiaire_volume_ii_12_reine_araignee.png
bestiaire_volume_iii_01_kobold_artificier.png
bestiaire_volume_iii_02_chaman_gobelin.png
bestiaire_volume_iii_03_scarabee_cuirasse.png
bestiaire_volume_iii_04_essaim_chauves_souris.png
bestiaire_volume_iii_05_ombre_rampante.png
bestiaire_volume_iii_06_basilic_mineur.png
bestiaire_volume_iii_07_gelee_acide.png
bestiaire_volume_iii_08_sentinelle_runique.png
bestiaire_volume_iii_09_miroir_vivant.png
bestiaire_volume_iii_10_troll_profondeurs.png
bestiaire_volume_iii_11_elementaire_foudre.png
bestiaire_volume_iii_12_dragonnet_caverne.png
bestiaire_volume_iv_01_bandit_eclaireur.png
bestiaire_volume_iv_02_harpie_des_falaises.png
bestiaire_volume_iv_03_sanglier_des_landes.png
bestiaire_volume_iv_04_sylvain_epineux.png
bestiaire_volume_iv_05_guepe_geante.png
bestiaire_volume_iv_06_feu_follet.png
bestiaire_volume_iv_07_homme_lezard_des_marais.png
bestiaire_volume_iv_08_crocodile_geant.png
bestiaire_volume_iv_09_crabe_des_recifs.png
bestiaire_volume_iv_10_sirene_des_brumes.png
bestiaire_volume_iv_11_elementaire_eau.png
bestiaire_volume_iv_12_hydre_des_marais.png
bestiaire_zombie.png
```


**`docs/Design/` — 731**

[historique à condenser, prescriptions à relire]

```text
ARMEMENT01_10_1_N1_PRODUCTION_READINESS.md
ARMEMENT01_10_2_1_WARRIOR_N1_VISUAL_REGISTRATION.md
ARMEMENT01_10_2_3_WEAPON_FIREBOMB_PALETTE.md
ARMEMENT01_10_2_WARRIOR_N1_AUTHORING.md
ARMEMENT01_10_3_1_EQUIPMENT_PHYSICAL_ARMOR.md
ARMEMENT01_10_3_3_10_DAMAGE_TOOLTIP_DETAILS.md
ARMEMENT01_10_3_3_COMBAT_TOOLTIP_STRUCTURE.md
ARMEMENT01_10_3_4_WARRIOR_PIE_ACCEPTANCE.md
ARMEMENT01_10_3_WARRIOR_FUNCTIONAL_VALIDATION.md
ARMEMENT01_11_1_MELEE_N1_PREFLIGHT.md
ARMEMENT01_11_2_1_ASSASSIN_DAGGER_VISUAL.md
ARMEMENT01_11_2_2_ASSASSIN_DAGGER_N1_AUTHORING.md
ARMEMENT01_13_STARTER_KITS_INTEGRATION.md
ARMEMENT01_15_STARTING_HEALTH_POTION.md
ARMEMENT01_16_3_SABOTEUR_STARTER_KIT.md
ARMEMENT01_16_SABOTEUR_DISARM_TOOLS.md
ARMEMENT01_17_1_3_OIL_COMBAT_AND_TOOLTIPS.md
ARMEMENT01_17_1_4_OIL_SURFACE_VISUALS.md
ARMEMENT01_17_3A_CATALYST_SQUARE_AREA.md
ARMEMENT01_17_3B_CATALYST_EXPLORATION_DAMAGE_TIME.md
ARMEMENT01_17_3C_CATALYST_PROJECTILE_VFX.md
ARMEMENT01_17_3D_CATALYST_NATIVE_DATA_MIGRATION.md
ARMEMENT01_17_4A_CATALYST_EVENT_FEED.md
ARMEMENT01_17_MAGE_ALCHEMIST_STARTER_COMPLEMENTS.md
ARMEMENT01_3_MULTI_DICE_IMPLEMENTATION.md
ARMEMENT01_4_WEAPON_CATALOGUE_AUTHORING.md
ARMEMENT01_5_FIXED_LEVEL_INSTANCE_INTEGRATION.md
ARMEMENT01_6_1_VISUAL_CATALOGUE_SCHEMA.md
ARMEMENT01_6_2_VISUAL_RESOLVER.md
ARMEMENT01_6_3_ARM001_MESHY7_DAGGER_PILOT.md
ARMEMENT01_6_3_FINAL_REALISTIC_WEAPONS.md
ARMEMENT01_6_3_PILOT_MATERIAL_REPAIR.md
ARMEMENT01_6_3_PILOT_VISUAL_ASSETS.md
ARMEMENT01_6_4_UI_VISUAL_INTEGRATION.md
ARMEMENT01_6_5_WORLD_AND_EDITOR_VISUALS.md
ARMEMENT01_6_CONTRAT_PRESENTATION_VISUELLE.md
ARMEMENT01_9_2_SIX_CLASS_STARTING_KITS.md
ARMEMENT01_9_3_WIZARD_KIT_SELECTION.md
ARMEMENT01_9_4_COHERENCE_AUDIT.md
ARMEMENT01_9_5_2_XLSX_WEAPON_CLASS_AFFINITIES.md
ARMEMENT01_9_5_KIT_DESIGN_DECISIONS.md
ARMEMENT01_9_6_1_CLEANUP_CONSOLIDATION.md
ARMEMENT01_9_6_EIGHTEEN_STARTING_KITS.md
ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md
ARMEMENT01_CONTRAT_ARMEMENT_V0_3.md
ARMEMENT_INDUS01_AUDIT.md
ARMEMENT_INDUS04_2C_TOOLTIP_VISUAL_FINISH.md
ARMEMENT_INDUS04_3A_EQUIPMENT_PREVIEW.md
ARMEMENT_INDUS04_3B_2_COMPARAISON_DEGATS_PHYSIQUES.md
ARMEMENT_INDUS04_3B_TOOLTIP_GLOBAL_PRESENTATION.md
ARMEMENT_INDUS04_3C_TOOLTIP_STAT_ICONS.md
ARMEMENT_INDUS04_3D_TWO_HANDS_LABEL.md
ARMEMENT_INDUS04_4B_PROTO_APPROVAL_BATCH.md
ARMEMENT_INDUS04_4_NARRATIVE_TOOLTIP.md
ARMEMENT_LOT02_10_PROMPTS_44_VUES_MESHY.md
ARMEMENT_LOT02_11_1_MANIFEST_PLANNED.md
ARMEMENT_LOT02_11_IMPORT_PREFLIGHT.md
ARMEMENT_LOT02_12_BATCH_AUTHORING.md
ARMEMENT_LOT02_13_1_RECURVE_AMMO.md
ARMEMENT_LOT02_13_2_2_ARROW_PALETTE.md
ARMEMENT_LOT02_13_2_WORLD_AND_PIE_ACCEPTANCE.md
ARMEMENT_LOT02_13_4_1_IMPACT_FIX.md
ARMEMENT_LOT02_13_4_RANGED_PROJECTILE.md
ARMEMENT_LOT02_13_6A_EMBEDDED_HIT.md
ARMEMENT_LOT02_13_6B_MISS_T.md
ARMEMENT_LOT02_13_6C_PROJECTILE_LANDING_PERSISTENCE.md
ARMEMENT_LOT02_13_PRODUCTION_FUNCTIONAL_PIE.md
ARMEMENT_LOT02_14_LEVEL_MIGRATION.md
ARMEMENT_LOT02_14_THROWING_KNIFE_REPLACEMENT_AUDIT.md
ARMEMENT_LOT02_15_ACTIVATION_CLASSES_ACCEPTANCE.md
ARMEMENT_LOT02_16_N1_FUNCTIONAL_ACCEPTANCE.md
ARMEMENT_LOT02_17_STARTER_KITS_ACCEPTANCE.md
ARMEMENT_LOT02_18_NEW_GAME_INVENTORY_VISUALS.md
ARMEMENT_LOT02_19_1_OFFHAND_COMBAT_HOTBAR.md
ARMEMENT_LOT02_19_2_1_DUAL_ATTACK_ICON.md
ARMEMENT_LOT02_19_2_2_SEQUENCED_COMBAT_PRESENTATION.md
ARMEMENT_LOT02_19_2_DUAL_WIELD_COMBAT.md
ARMEMENT_LOT02_19_3_PARRY_SHIELD_GUARD.md
ARMEMENT_LOT02_19_4_PARRY_EVENT_FEED.md
ARMEMENT_LOT02_20_EQUIPPED_ACTION_ICON_FOUNDATION.md
ARMEMENT_LOT02_21_SINGLE_EQUIPPED_ATTACK_HOTBAR.md
ARMEMENT_LOT02_3_BATCH_PRODUCTION.md
ARMEMENT_LOT02_5_MARKSMAN_AMMUNITION_PRODUCTION.md
ARMEMENT_LOT02_6_THROWING_WAVE.md
ARMEMENT_LOT02_7_MAGIC_ATTACKS.md
ARMEMENT_LOT02_8_FOCUS_PASSIVE.md
ARMEMENT_LOT02_9_BLENDER_ART_SOURCE.md
ARM_CLEAN06_DOCUMENTATION_INDEX.md
CPP_CLEAN01_REMOVE_LEGACY_DOOR_AUDIO.md
CPP_CLEAN02_ENFORCE_ITEM_DEFINITION_AUTHORITY.md
CPP_CLEAN03_REMOVE_PROVEN_DEAD_COMPATIBILITY_SURFACE.md
CPP_CLEAN04_REMOVE_HARD_CODED_ASSET_FALLBACKS.md
GEUI01_DOCKABLE_GRID_EDITOR_WORKSPACE_FOUNDATION.md
GEUI02_DUNGEON_LEVELS_WINDOW.md
GEUI03_PLAYTEST_VALIDATION_WINDOW.md
GEUI04_TOOLS_PALETTE_WINDOW.md
GEUI05_SELECTED_OBJECT_WORKSPACE.md
GEUI06_SLIM_MAIN_TOOLKIT.md
GEUI07_PALETTE_UX.md
GEUI08_VALIDATION_UX.md
GEUI09_REFRESH_STATE_CLEANUP.md
GEUI10_PLUGIN_PLAYER_LEVEL_EDITOR_READINESS_AUDIT.md
GEUI11_1_COMPACT_OVERVIEW_MAP_SIDEBAR_FIX.md
GEUI11_INTEGRATED_DUNGEON_LEVELS_OVERVIEW_MAP.md
MAP_UI01_STANDALONE_MAP_WINDOW.md
MAP_UI02_MAP_TITLE_BAR_CLOSE.md
MAP_UI03_FLOOR_NAVIGATION_OVERLAY.md
MON10_AUDIO_SETUP.md
MON10_BALANCING_OPTIMIZATION.md
MON10_COMBAT_FEEDBACK_SETUP.md
MON10_IDLE_VARIATIONS_SETUP.md
MON10_VFX_SETUP.md
MON11_1_PARTY_ATTACK_REQUEST_PIPELINE.md
MON11_2_PARTY_ATTACK_RESOLUTION.md
MON11_3_OFFENSIVE_EQUIPMENT.md
MON11_4_1_THROWN_WEAPON_PRESENTATION.md
MON11_4_PLAYER_ATTACK_PRESENTATION.md
MON12_10_ACTION_PALETTE_TARGETING.md
MON12_11_HOTBAR_VALIDATION.md
MON12_12_CLOSURE_VALIDATION.md
MON12_1_COMBAT_ACTION_PANEL.md
MON12_2_COMBAT_ACTION_CLICKS.md
MON12_3_CHARACTER_TURN_ACTION_POINTS.md
MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md
MON12_5_PARTY_MOVEMENT_ACTION_POINTS.md
MON12_6_COMBAT_ACTION_CATALOG.md
MON12_7_1_SLIDING_DYNAMIC_INITIATIVE.md
MON12_7_ACTION_ORIENTED_COMBAT_HUD.md
MON12_8_1_PERSISTENT_COMBAT_HOTBAR_MODEL.md
MON12_8_2_FIXED_COMBAT_HOTBAR_DRAG_DROP.md
MON12_8_3_COMBAT_HOTBAR_EXECUTION.md
MON12_8_4_QUICK_ITEM_COMBAT_ACTIONS.md
MON12_8_5_CLASS_ACTION_PALETTE.md
MON12_8_6_CELL_AREA_TARGETING.md
MON12_8_7_HOTBAR_AVAILABILITY_ASSIGNMENT.md
MON12_8_8_COMBAT_HUD_COHERENCE_AUDIT.md
MON12_8_9_HOTBAR_ITEM_LIFETIME.md
MON13_1_MONSTER_SPAWN_MODEL.md
MON13_2_MONSTER_SPAWN_PIPELINE.md
MON13_3_MONSTER_RUNTIME_COMMANDS.md
MON13_4_MONSTER_ENCOUNTER_WAVES.md
MON13_5_MONSTER_SPAWN_CLOSURE.md
MON13_6_ENCOUNTER_START_RELIABILITY.md
MON14_1_AUTOMATIC_PERCEPTION_ENGAGEMENT.md
MON14_2_DIRECTIONAL_PERCEPTION_PATROL_DATA.md
MON14_2_VALIDATION_CHECKLIST.md
MON14_3_1_VALIDATION_CHECKLIST.md
MON14_3_1_VISUAL_PATROL_ROUTE_EDITOR.md
MON14_3_2_PATROL_RUNTIME_RELIABILITY.md
MON14_3_RUNTIME_PATROL_INVESTIGATION.md
MON14_3_VALIDATION_CHECKLIST.md
MON14_4_EXPLORATION_ALARM_COORDINATION.md
MON14_4_VALIDATION_CHECKLIST.md
MON14_CLOSURE.md
MON15_1_VALIDATION_CHECKLIST.md
MON15_1_XP_LEVEL_MODEL.md
MON15_2_CLOSURE.md
MON15_2_VALIDATION_CHECKLIST.md
MON15_2_XP_AWARDING.md
MON15_3_LEVEL_UP_MODEL.md
MON15_3_VALIDATION_CHECKLIST.md
MON15_4_CLASS_PROGRESSION.md
MON15_4_CLOSURE.md
MON15_4_VALIDATION_CHECKLIST.md
MON15_5_CLOSURE.md
MON15_5_LEVEL_UP_INTERFACE.md
MON15_5_VALIDATION_CHECKLIST.md
MON15_6_CLOSURE.md
MON15_6_REOPEN_LOADING_MODAL.md
MON15_6_SAVE_MIGRATION.md
MON15_6_VALIDATION_CHECKLIST.md
MON15_7_BALANCING.md
MON15_7_VALIDATION_CHECKLIST.md
MON15_CLOSURE.md
MON16_1_STATUS_EFFECT_DEFINITION_RUNTIME_STATE.md
MON16_1_VALIDATION_CHECKLIST.md
MON16_2_DURATION_TURN_ROUND_LIFECYCLE.md
MON16_2_VALIDATION_CHECKLIST.md
MON16_3_DOT_POISON_BLEEDING_BURNING.md
MON16_3_VALIDATION_CHECKLIST.md
MON16_4_HASTE_SLOW_INITIATIVE_MODIFIER.md
MON16_4_VALIDATION_CHECKLIST.md
MON16_5_STUN_SILENCE_IMMOBILIZE.md
MON16_5_VALIDATION_CHECKLIST.md
MON16_6_STATUS_EFFECT_HUD_COMBAT_FEEDBACK.md
MON16_6_VALIDATION_CHECKLIST.md
MON16_7_STATUS_EFFECT_SAVE_RESTORE.md
MON16_7_VALIDATION_CHECKLIST.md
MON16_8_STATUS_EFFECT_MILESTONE_CLOSURE.md
MON16_8_VALIDATION_CHECKLIST.md
MON17_2_GOBLIN_THROWER_SKELETAL_ANIMATION.md
MON17_3_1_RANGED_ATTACK_EXECUTION.md
MON17_3_2_PROJECTILE_PRESENTATION.md
MON17_3_3_THROW_PRESENTATION.md
MON17_3_4_MONSTER_ATTACK_COOLDOWN.md
MON17_4_1_RANGED_KEEPER_POSITIONING.md
MON17_5_1_GOBLIN_EXPLORATION_INTEGRATION.md
MON17_5_CLOSURE.md
MON17_6_1_GOBLIN_ENCOUNTER_REWARD_INTEGRATION.md
MON17_6_CLOSURE.md
MON17_6_PRODUCTION_PIE_VALIDATION.md
MON17_7_BALANCE_AUDIT.md
MON17_7_FINAL_PIE_VALIDATION.md
MON17_8_1_GOBLIN_THROWER_ANIMATION_AUDIT.md
MON17_8_2_3_AUTOMATION_VALIDATION.md
MON17_8_2_GENERIC_MONSTER_WALK_SYNCHRONIZATION.md
MON17_8_3_GENERIC_MONSTER_PRESENTATION_STATE_INTEGRATION.md
MON17_8_4_GENERIC_MONSTER_DEATH_ANIMATION.md
MON17_8_5_GENERIC_MONSTER_CORPSE_DISSOLVE.md
MON17_8_5_MATERIAL_AUDIT_AND_AUTOMATION_VALIDATION.md
MON17_8_6_DEAD_MONSTER_SAVE_RESTORE.md
MON17_8_7_FINAL_VALIDATION_AND_CLOSURE.md
MON17_9_MONSTER_HURT_PRESENTATION.md
MON17_CLOSURE.md
MON17_FINAL_REGRESSION_PLAN.md
MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md
MON18_2_VALIDATION.md
MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md
MON18_3_VALIDATION.md
MON18_4_TARGETING_INTEGRATION.md
MON18_4_VALIDATION.md
MON18_5_FIRST_PRODUCTION_SPELLS.md
MON18_5_VALIDATION.md
MON18_6_SPELL_PRESENTATION.md
MON18_6_VALIDATION.md
MON18_7_SPELLBOOK_HOTBAR_UI.md
MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md
MON18_8_VALIDATION.md
MON18_9_1_COMBAT_SAVE_POLICY.md
MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md
MON18_9_3_FINAL_DIAGNOSTICS_GLOBAL_REGRESSION.md
MON18_CLOSURE.md
MON19_1_EVENT_COMMAND_AUDIT_LUA_FEASIBILITY.md
MON19_2_1B_CONDITIONAL_CONNECTORS.md
MON19_2_1R_GRID_LEVEL_EDITOR_ACTOR_STRUCTURE.md
MON19_2_2_PERSISTENT_LEVEL_VARIABLES.md
MON19_2_3_LOGIC_PRIMITIVES.md
MON19_3_1_LUA_RUNTIME_FOUNDATION.md
MON19_4_EVENT_LUA_COMMAND_BRIDGE.md
MON19_5_LUA_PERSISTENCE_RESTORE.md
MON19_6_LUA_EDITOR_VALIDATION.md
MON19_7_1_LUA_AUTHORING_API.md
MON19_7_LUA_SANDBOX_PACKAGING.md
MON19_8_FINAL_VALIDATION.md
MON19_8_PRODUCTION_PUZZLES_CLOSURE.md
MON19_CLOSURE.md
MON1_RAT_GIANT_DATA_ASSET_SETUP.md
MON20_10_1_BALANCE_REGRESSION_AUDIT.md
MON20_10_2_DEAD_MONSTER_RESTORE_OCCUPANCY_HARDENING.md
MON20_10_3_LOG_HYGIENE_KNOWN_DIAGNOSTICS.md
MON20_10_5_FINAL_PIE_MON20_CLOSURE.md
MON20_1_AUDIT_RECRUITMENT_SKILLS_TALENTS.md
MON20_2_ACTIVE_PARTY_RECRUITMENT_FOUNDATION.md
MON20_3_STORY_COMPANION_DEFINITION_POOL.md
MON20_4_2_RECRUITMENT_WIDGET_CORE.md
MON20_4_3_RECRUITMENT_MODAL_RUNTIME_INTEGRATION.md
MON20_4_4_EVENT_COMMAND_RECRUITMENT_BRIDGE.md
MON20_4_5_STORY_COMPANION_GRID_EDITOR_PLACEMENT.md
MON20_4_6_RECRUITMENT_OFFER_SUPPRESSION.md
MON20_5_2_CUSTOM_RECRUIT_TRANSACTION.md
MON20_5_3_WIZARD_CONTEXT_REUSE.md
MON20_5_4_CUSTOM_RECRUIT_MODAL_RUNTIME_INTEGRATION.md
MON20_5_5_CUSTOM_RECRUIT_EVENT_COMMAND_AUTHORING_BRIDGE.md
MON20_5_6_CUSTOM_RECRUITER_GRID_AUTHORING_PIE.md
MON20_5_CLOSURE.md
MON20_6_2_SKILL_DEFINITION_RUNTIME_RANKS.md
MON20_6_3_DETERMINISTIC_SKILL_CHECK_RESOLUTION.md
MON20_6_4_RUNTIME_ACCESS_CHARACTER_SELECTION.md
MON20_6_5_AUTOMATION_REGRESSION_CLOSURE.md
MON20_7_1_TALENTS_PROGRESSION_CHOICE_ARCHITECTURE.md
MON20_7_2_TALENT_RUNTIME_READ_MODEL.md
MON20_7_4_REQUIREMENT_PROJECTION_PERSISTENCE_REGRESSION.md
MON20_7_5_AUTOMATION_REGRESSION_CLOSURE.md
MON20_8_2_SKILL_DEFINITION_IDENTITY_REQUIREMENT_PROJECTION.md
MON20_8_3_AUTOMATION_VALIDATION.md
MON20_8_3_COMBAT_ACTION_REQUIREMENT_INTEGRATION_DIAGNOSTICS.md
MON20_8_4_AUTOMATION_VALIDATION.md
MON20_8_4_NATIVE_SKILLS_PRESENTATION.md
MON20_8_4_SKILLS_TALENTS_PAGE_READ_MODEL_MENU_INTEGRATION.md
MON20_8_5_AUTOMATION_PIE_REGRESSION_CLOSURE.md
MON20_9_2_AUTOMATION_VALIDATION.md
MON20_9_2_SKILL_RANK_SAVE_SNAPSHOT_V8_MIGRATION.md
MON20_9_3_ACTIVE_POOL_CHARACTER_PERSISTENCE_REGRESSION.md
MON20_9_3_AUTOMATION_VALIDATION.md
MON20_9_4_AUTOMATION_VALIDATION.md
MON20_9_4_SKILL_PROJECTION_SKILLS_PAGE_RESTORE_REGRESSION.md
MON20_9_5_AUTOMATION_PIE_REGRESSION_CLOSURE.md
MON20_START.md
MON21_1_QUESTS_JOURNAL_MAP_CODEX_ARCHITECTURE_AUDIT.md
MON21_2_QUEST_DEFINITION_CAMPAIGN_RUNTIME_STATE.md
MON21_3_QUEST_EVENT_COMMAND_INTEGRATION.md
MON21_4_QUEST_PERSISTENCE.md
MON21_6_10_ZOOM_PAN_RECENTER.md
MON21_6_11_HAND_DRAWN_PARCHMENT.md
MON21_6_12_MAP_SYMBOLS.md
MON21_6_13_MAP_CLOSURE.md
MON21_6_2_EXPLORATION_STATE.md
MON21_6_3_TOPOLOGY_AWARE_REVEAL.md
MON21_6_4_SECRET_DISCOVERY.md
MON21_6_5_EXPLORATION_PERSISTENCE.md
MON21_6_6_MAP_READ_MODEL.md
MON21_6_7_MULTI_TILE_FLOOR_PROJECTION.md
MON21_6_8_NATIVE_MAP_RENDERING.md
MON21_6_9_FLOOR_NAVIGATION.md
MON3_OCCUPANCY_MOVEMENT_SETUP.md
MON4_PATHFINDING_PERCEPTION_SETUP.md
MON5_NATIVE_DEBUG_INPUT.md
MON5_TURN_MANAGER_SETUP.md
MON6_COMBAT_RESOLUTION_SETUP.md
MON7_FAST_HARASSER_SETUP.md
MON8_MONSTER_DEATH_LOOT_SETUP.md
MON9_MONSTER_PERSISTENCE_SETUP.md
PRESSURE_MON01_OPTIONAL_MONSTER_PRESSURE_PLATE.md
PUZZLE01_CLEAN01_PRODUCTION_SCRIPT_VALIDATION.md
PUZZLE01_CLEAN02_FINAL_CONSOLIDATION.md
RPG03_10_BALANCE_AUTOMATION_PIE.md
RPG03_1_COMBAT_MODIFIER_PROFILE.md
RPG03_2_STATUS_APPLICATION_ARMOR_GATE.md
RPG03_3_ARMOR_EFFECTS.md
RPG03_4_TRIGGER_REACTION_PROFILE.md
RPG03_5_TACTICAL_FORCED_MOVEMENT.md
RPG03_6_PERSISTENT_SURFACES_REACTIONS.md
RPG03_7_ALCHEMY_QUICKITEM_MODIFIERS.md
RPG03_8_BATCH_TARGET_FILTERS_TARGETABILITY.md
RPG03_9_0_WEAPON_BASED_TALENT_ATTACKS.md
RPG03_9_1_WARRIOR_AUTHORING.md
RPG03_9_2_ROGUE_AUTHORING.md
RPG03_9_3_FIX_FAVORED_ENEMY_CATEGORY_TITLES.md
RPG03_9_3_RANGER_AUTHORING.md
RPG03_9_4C_MAGE_EVOKER_AUTHORING.md
RPG03_9_4D_MAGE_EVOKER_BRANCH.md
RPG03_9_4E_MAGE_ARCANIST_BRANCH.md
RPG03_9_4F1_SURFACE_WEAVER_SUPPORT.md
RPG03_9_4F2_MAGE_SURFACE_WEAVER_BRANCH.md
RPG03_9_5A_PRIEST_SUPPORT.md
RPG03_9_5B1_PRIEST_RESTORATION_PROTECTION_AUTHORING.md
RPG03_9_5B2_PRIEST_EXORCISM_AUTHORING.md
RPG03_9_5C_PRIEST_PRODUCTION_MATERIALIZATION.md
RPG03_9_6A_ALCHEMIST_SUPPORT.md
RPG03_9_6B1_ALCHEMIST_GRENADIER_APOTHECARY.md
RPG03_9_6B2_ALCHEMIST_TRANSMUTER.md
RPG03_9_6C_ALCHEMIST_PRODUCTION_MATERIALIZATION.md
RPG03_FINAL_AUDIT.md
RPG03_SKILL_TREE_SYNTHESIS.md
RPG_PERF01_LIGHTWEIGHT_READ_MODELS.md
STYLE01_CPP_FORMATTING_BASELINE.md
STYLE01_NEXT_THREAD_PROMPT.md
TD02_9_PARTY_MOVEMENT_AND_STOP_CONDITION.md
TD04_2_LOCAL_UE_VALIDATION_HARNESS.md
TD04_3_COOK_PACKAGE_VALIDATION.md
TD05_4_RUNTIMEACTOR_REAUDIT.md
TD05_7_RUNTIMEACTOR_POST_FEEDBACK_REAUDIT.md
TD05_9_RUNTIMEACTOR_STOP_CONDITION.md
TD06_1_PARTY_INVENTORY_REBASELINE.md
TD06_2_PARTY_INVENTORY_HOTBAR_CHARACTERIZATION.md
TD06_3_PARTY_INVENTORY_HOTBAR_EXTRACTION.md
TD06_4_PARTY_INVENTORY_CURSOR_TRANSFER_CHARACTERIZATION.md
TD06_5_PARTY_INVENTORY_CURSOR_TRANSFER_EXTRACTION.md
TD06_6_PARTY_INVENTORY_EQUIPMENT_CORE_CHARACTERIZATION.md
TD06_7_PARTY_INVENTORY_EQUIPMENT_CORE_EXTRACTION.md
TD06_8_PARTY_INVENTORY_ITEM_DEFINITION_REGISTRY_AUDIT.md
TD06_9_PARTY_INVENTORY_STOP_CONDITION.md
TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md
TD07_2_UE_DEPRECATION_WARNING_AUDIT.md
TD07_3_1_PROTOTYPE_DATA_MODEL_POLICY_AND_ASSET_AUDIT.md
TD07_3_2_SAVEGAME_RESET_NO_BACKWARD_MIGRATION.md
TD07_3_3_10_CURRENT_SAVE_SCHEMA_CHARACTERIZATION.md
TD07_3_3_10_CURRENT_SAVE_SCHEMA_NORMALIZATION.md
TD07_3_3_1_CHARACTER_STATE_AUTHORITY_AUDIT.md
TD07_3_3_2_REMOVE_LEGACY_ATTRIBUTE_BRIDGE.md
TD07_3_3_3_DERIVED_STATS_CHARACTERIZATION.md
TD07_3_3_3_DERIVED_STATS_NORMALIZATION.md
TD07_3_3_4_WEIGHT_STATE_CHARACTERIZATION.md
TD07_3_3_4_WEIGHT_STATE_NORMALIZATION.md
TD07_3_3_5_PROGRESSION_AUTHORITY_NORMALIZATION.md
TD07_3_3_5_XP_LEVEL_CLASS_PROGRESSION_CHARACTERIZATION.md
TD07_3_3_6_SKILL_STATE_CHARACTERIZATION.md
TD07_3_3_6_SKILL_STATE_NORMALIZATION.md
TD07_3_3_7_SPELLBOOK_STATE_CHARACTERIZATION.md
TD07_3_3_7_SPELLBOOK_STATE_NORMALIZATION.md
TD07_3_3_8_STATUS_EFFECT_STATE_CHARACTERIZATION.md
TD07_3_3_8_STATUS_EFFECT_STATE_NORMALIZATION.md
TD07_3_3_9_LEVEL_UP_NOTIFICATION_STATE_CHARACTERIZATION.md
TD07_3_3_9_LEVEL_UP_NOTIFICATION_STATE_NORMALIZATION.md
TD07_3_3_CHARACTER_STATE_NORMALIZATION_CLOSURE.md
TD07_3_4_2_CANONICAL_AUTHORING_ASSET_IDENTITY.md
TD07_3_4_3_CHARACTER_IDENTITY_STATE_NORMALIZATION.md
TD07_3_4_4_COMPANION_CREATION_AUTHORING_CLEANUP.md
TD07_3_4_AUTHORING_IDENTITY_CHARACTERIZATION.md
TD07_3_4_AUTHORING_IDENTITY_NORMALIZATION_CLOSURE.md
TD07_3_5_2_ITEM_COMBAT_ACTIONS_AUTHORITY.md
TD07_3_5_3_MONSTER_PRESENTATION_AUTHORITY.md
TD07_3_5_4_MONSTER_RANGE_SCHEMA.md
TD07_3_5_5_REGRESSIONS_SHIPPING_CLOSURE.md
TD07_3_5_COMBAT_DATA_SCHEMA_CHARACTERIZATION.md
TD07_3_6_REMAINING_LEGACY_API_DATA_PURGE.md
TD07_3_7_CURRENT_ASSET_REPAIR_RECREATION.md
TD07_3_8_STRICT_CURRENT_SCHEMA_VALIDATION.md
TD07_4_ACTIVATION_COMPONENT_CHARACTERIZATION.md
TD07_5_SUSPENDED_TEST_INFRASTRUCTURE_BRANCH_RECOVERY.md
TD07_7_TARGETED_LOG_FORMATTING_HYGIENE.md
TD07_8_FUTURE_PROOFING_STOP_CONDITION.md
UI_ASSET_CLEAN01_REFERENCE_VIEWER_CLEANUP.md
UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md
UI_CLEAN02_REMOVE_GENERATED_PAPERDOLL.md
UI_CLEAN03_REMOVE_LEGACY_ARMOR_ALIAS.md
UI_CLEAN04_REMOVE_PRIMITIVE_DISPLAY_HELPERS.md
UI_COMBAT_CLEAN01_SIMPLIFY_PARTY_MEMBER_COMBAT_PANELS.md
UI_ITEM01_CLEAN01_REMOVE_TOOLTIP_DUPLICATES.md
UI_ITEM01_ITEM_TOOLTIP_COMPARISON.md
UI_RPG01_3_TALENT_TREE_READ_MODEL.md
UI_RPG01_4_PRODUCTION_MATERIALIZATION.md
UI_RPG01_VISUAL_SYNTHESIS.md
UI_RPG02_2_PRODUCTION_PRESENTATION.md
UI_RPG03_1_DESIGNER_SHELL.md
UI_RPG03_2_TALENT_WIDGETS.md
UI_RPG03_3_REMOVE_NATIVE_RENDERER.md
UI_RPG03_4_STANDALONE_SKILLS_WINDOW.md
UI_RPG04_1_TALENT_SELECTION_ROUTING.md
UI_RPG04_2_TALENT_DETAIL_WIDGET.md
UI_RPG04_3_SIMPLE_TALENT_ACQUISITION.md
UI_RPG04_4_GENERIC_VARIANT_ACQUISITION.md
UI_RPG05_PROGRESSION_NOTIFICATIONS.md
UI_RPG06_1_SKILLS_AUDIT_CATALOG_AUTHORING.md
UI_RPG06_2B_SKILLS_PAGE_RENDERER.md
UI_RPG06_3A_SKILLS_FINALIZATION.md
UI_RPG06_3B_FINAL_CLOSURE.md
UI_RPG_CLEAN02_REMOVE_OBSOLETE_LEVEL_TOAST_FACTORY.md
UI_TOOLTIP_CLEAN01_AUDIT_SHIELD_COMPARISON.md
WORLDOBJ_MIG01_PLACEMENT_SURFACE.md
WORLDOBJ_MIG02_SPATIAL_BEHAVIOR.md
WORLDOBJ_MIG03_4B_AUTHORING_MIGRATION.md
```

[ressource / provenance à conserver]

```text
ARMEMENT01_9_5_KIT_DECISIONS.csv
ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv
ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx
ARMEMENT_FX01_ACTIVE22_CUE_MATRIX.csv
ARMEMENT_INDUS01_INVENTAIRE_ASSETS.csv
ARMEMENT_INDUS03_2_Narratif_DRAFT.csv
ARMEMENT_INDUS03_Narratif_Modele.csv
ARMEMENT_LOT02_11_ART_INTAKE.csv
ARMEMENT_LOT02_6_MISSING_WEAPON_WAVES.csv
ARMEMENT_LOT02_7_PRODUCTION_MODES.csv
ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.csv
Grimrock Items.xlsx
```

[référence ou spécification : relecture requise]

```text
00_PROJECT_OVERVIEW.md
01_GRID_OBJECT_SYSTEM.md
02_OBJECT_ARCHETYPES.md
03_EVENT_COMMAND_LINKS.md
04_IMPLEMENTATION_ROADMAP.md
05_CODEX_TASKS.md
06_GRID_EDITOR_UX_SPEC.md
07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md
08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md
09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md
10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md
10_MOUSE_INTERACTION_SYSTEM.md
11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md
11_MOUSE_INTERACTION_REGRESSION_CHECKLIST.md
12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md
99_DECISIONS_LOG.md
ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md
ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.md
CHARACTER_CREATION_CC1_UE5_CHECKLIST.md
CHARACTER_CREATION_CC2_UE5_CHECKLIST.md
CHARACTER_CREATION_CC3_UE5_CHECKLIST.md
CHARACTER_CREATION_CC4_1_UE5_CHECKLIST.md
CHARACTER_CREATION_CC4_UE5_CHECKLIST.md
CHARACTER_CREATION_CC5_UE5_CHECKLIST.md
CHARACTER_CREATION_CC6_1_UE5_CHECKLIST.md
CHARACTER_CREATION_CC6_2_UE5_CHECKLIST.md
CHARACTER_CREATION_CC6_3_UE5_CHECKLIST.md
CHARACTER_CREATION_CC6_4_UE5_CHECKLIST.md
CHARACTER_CREATION_CC6_5_UE5_CHECKLIST.md
CHARACTER_CREATION_CC7_1_WIZARD_SHELL.md
CHARACTER_CREATION_CC7_2_RACE_ILLUSTRATIONS.md
CHARACTER_CREATION_CC7_3_ATTRIBUTE_ALLOCATION.md
CHARACTER_CREATION_CC7_4_2_IDENTITY_SCREEN.md
CHARACTER_CREATION_CC7_6_VISUAL_SUMMARY.md
CHARACTER_CREATION_CC7_WIZARD_RECRUITMENT.md
CHARACTER_CREATION_ROADMAP.md
CHARACTER_CREATION_VISUAL_IDENTITY_ROADMAP.md
COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md
DOC_AUDIT02_DESIGN_CPP_COHERENCE.md
DOC_CLOSURE01_REBASELINE_DOCUMENTATION.md
DOOR_AUDIO_SETUP.md
EDITOR_PALETTE_ICON01.md
EVENTHUD01_XP01_EXPERIENCE_NOTIFICATIONS.md
GE_ERASE01_TARGETED_OBJECT_DELETION.md
GE_UNDO01_NATIVE_TRANSACTIONAL_UNDO_REDO.md
GE_UNDO02_GESTURE_TRANSACTION_GROUPING.md
GRID_EDITOR_ACTOR_UI_AUDIT.md
GRID_EDITOR_REFERENCE_DUNGEON_FIELD_TESTS.md
GRID_EDITOR_UI_CLEANUP.md
GRID_OBJECT_AUDIO_SYSTEM.md
GRID_RELOCATION_DATA.md
GRIMROCK_LOCK_SYSTEM.md
HOTBAR01_1_SHORTCUT_QUANTITY_BADGE.md
HOTBAR01_2_1_REMOVE_THROW_MAINHAND_LEGACY_COMPATIBILITY.md
HOTBAR01_2_REMOVE_LEGACY_ACTION_PALETTE.md
HOTBAR01_SIMPLIFIED_GENERAL_ACTION_BAR.md
HOTBAR_CONTEXT01_INVENTORY_CONTEXT_ACTION.md
INVENTORY_AND_ITEM_OWNERSHIP_DESIGN.md
INVENTORY_BLUEPRINT_CONSTRUCTION_GUIDE.md
INVENTORY_CONTEXT_ACTION_MVP_VALIDATION.md
INVENTORY_INTERACTION_ROUTING.md
ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md
ITEM_CONTEXT_ACTION_SYSTEM.md
ITEM_PHYSICS01_WORLD_PICKUP_SETTLING.md
ITEM_SPARKLE01_WORLD_ITEM_SPARKLE_PRESENTATION.md
JALON_RUNTIME_DUNGEON_STATE.md
LEVEL_FADE01_SMOOTH_TRANSITIONS.md
LUA_COMP01_AUTHORING_COMPILER.md
LUA_SCRIPTING_LANGUAGE_REFERENCE.md
LUA_UX01_EVENT_ACTION_AUTHORING.md
LUA_UX02_LOGIC_IDENTITY_CLEANUP.md
LUA_UX03_LEGACY_IDENTITY_CONDITION_PURGE.md
LUA_UX04_SYNTAX_HIGHLIGHTING.md
MAIN_MENU_AND_GAME_FLOW.md
MAIN_MENU_MM1_UE5_CHECKLIST.md
MAIN_MENU_MM2_1_GAMEINSTANCE.md
MAIN_MENU_MM2_2_STARTUP_MODE_COMPONENT.md
MAIN_MENU_MM2_UE5_CHECKLIST.md
MAIN_MENU_MM3_VALIDATION.md
MAIN_MENU_MM4_2_VALIDATION_AND_MULTI_SLOT_SAVE.md
MAIN_MENU_MM4_LOAD_GAME_SLOTS.md
MAIN_MENU_MM5_FINISHING.md
MAP_ART01_HAND_DRAWN_MAP_POLISH.md
MAP_FLOOR01_TEXTURED_EXPLORED_FLOOR.md
MAP_THEME01_TEXTURED_MAP_RENDERING.md
MIXAMO_ANIMATION_IMPORT_RETARGETING_UE5_5_4.md
MONSTERS_AI_ANIMATIONS_TURN_BASED_COMBAT_RAT_GIANT.md
MULTI_LEVEL_DUNGEON_ASSET.md
PARTY_WEIGHT01_OVERLOAD_MOVEMENT_LOCK.md
PERF_PROFILE01_UNREAL_INSIGHTS_CAPTURE.md
PIE_TEST_FROM_EDITOR.md
PIT01_STATIC_INTER_LEVEL_PIT.md
PIT02_WORLD_ITEMS_FALLING_THROUGH_PITS.md
PIT03_1_TRAPDOOR_OPENING_CLOSING_ANIMATION.md
PIT03_2_DUAL_LEAF_TRAPDOOR.md
PIT03_CONTROLLED_PIT_TRAPDOOR.md
PROJECT_COMPLETION_ROADMAP.md
PUZZLE01_LUA01_LUA_FIRST_VISUAL_PRIMITIVE.md
PUZZLE01_LUA02_PRODUCTION_AUDIT.md
RECEPTACLE_SYSTEM.md
RPG_ATTR01_1_ATTRIBUTE_POINT_ECONOMY.md
RPG_ATTR01_3_ATTRIBUTE_ALLOCATION_CLEANUP_AUDIT.md
RPG_ATTR01_FINAL_CLOSURE.md
RPG_DEV01_PIE_PROGRESSION_SIMULATOR.md
RPG_LEVELUX01_1_NON_MODAL_LEVEL_UP.md
RPG_LEVELUX01_3_REMOVE_LEGACY_LEVEL_UP_MODAL.md
RPG_LEVELUX01_FINAL_CLOSURE.md
RPG_SHIELD02_2_VICTORY_REFILL.md
RPG_SHIELD02_3_EXPLICIT_REST.md
RPG_SHIELD02_4A_SURFACE_ARMOR_DAMAGE.md
RPG_SHIELD02_4B_TARGETED_ARMOR_RESTORATION.md
RPG_SHIELD02_5_SAVE_PERSISTENCE.md
RPG_SKILL01_1_SKILL_POINT_ECONOMY.md
RPG_SKILL01_3_SAFE_SKILL_ALLOCATION_UNDO.md
RPG_SKILL01_FINAL_CLOSURE.md
RPG_TALENT_FIX01_D05_TURN_UNDEAD_FIXED_DAMAGE.md
RPG_TALENT_FIX02_D01_SECOND_WIND_FULL_HEALTH.md
RPG_TALENT_FIX03_D03_WORLD_SABOTAGE_CALLER.md
RPG_TALENT_FIX04_D06_CHAIN_REACTION_SURFACE_EFFECT_BRIDGE.md
RPG_TALENT_FIX05_D07_MAJOR_TRANSMUTATION_FIXED_DURATION.md
RPG_TALENT_FIX06_D08_MAJOR_TRANSMUTATION_REACTION_BRIDGE.md
RPG_TALENT_FIX07_D04_FAVORED_ENEMY_CATEGORY_PRESENTATION.md
SAVEFIX_2_CONTINUE_FAILURE_HANDLING.md
STARTUP_FLOW_SAVE_VALIDATION_AND_DUNGEON_BUILD.md
TEST_AUDIT01_FULL_AUTOMATION_REGRESSION_CLEANUP.md
TEST_AUDIT02_AUTOMATION_WARNING_LEGACY_RELEVANCE_AUDIT.md
TRAP01_2_PASSIVE_PERCEPTION_REVEAL.md
TRAP01_3_DUNGEON_TRAP_DAMAGE.md
TRAP01_4_1_DISARM_XP_REWARD.md
TRAP01_4_BEST_PARTY_DISARM_DEXTERITY.md
TRAP01_5_1_PIE_DISARM_INTERACTION_PERCEPTION_FEEDBACK.md
TRAP01_5_STARTER_HAZARDS_EDITOR_PLACEMENT.md
TRAP01_6_TRAP_HIGHLIGHT.md
TRAP01_7_1_SKILL_ROLL_BREAKDOWN.md
TRAP01_7_DISARM_FEEDBACK_AND_HOVER.md
TRAP01_8_DISARM_FAILURE_CONSEQUENCES.md
TRAP01_9_1_DISARM_PROBABILITIES.md
TRAP01_9_2_WORLD_HOVER_TOOLTIP.md
TRAP01_9_3_WORLD_TOOLTIP_POSITION.md
TRAP01_9_DISCOVERED_DISARM_DIFFICULTY.md
TRAP_EDITOR_MAP01.md
TRAP_INSTANCE01_PER_PLACEMENT.md
TRAP_PIT01_3_HIDE_TRIGGER_MESH_ON_OPEN_PIT.md
TRAP_PIT01_FLOOR_TRAP_TO_PIT.md
TRAP_SEARCH01_1_ACTIVE_RUNTIME.md
TRAP_SEARCH01_2_PLAYER_COMMAND_AND_UI.md
UI_ARCHITECTURE_CURRENT.md
UI_ARCHITECTURE_TARGET.md
UI_ASSET_AUDIT_2026_09_22.md
UI_ASSET_ORG01_UI_FOLDER_REORGANIZATION.md
UI_CHAR01_SINGLE_SELECTED_CHARACTER.md
UI_CHAR02_CHARACTER_SHEET_PROJECTION.md
UI_CODE_AUDIT01_COMBAT_INVENTORY_CPP.md
UI_COMBAT01_CLOSE_NON_COMBAT_UI_ON_COMBAT_START.md
UI_COMBAT_WIDGETS_CURRENT.md
UI_EVENTHUD01_18_INCREMENTAL_PRESENTATION.md
UI_EVENTHUD01_PARTY_PROGRESSION_WEIGHT_FEEDBACK.md
UI_FEEDBACK01_PARTY_STATUS_EFFECTS.md
UI_FEEDBACK01_PARTY_WEIGHT_ALERT.md
UI_FILTER01_CATEGORY_MAPPING.md
UI_FOUNDATION01_UNIFIED_INVENTORY_CHARACTER_WORKSPACE.md
UI_GLOBALHUD01_1_DYNAMIC_FULL_WIDTH_ACTION_BAR.md
UI_GLOBALHUD01_3_REMOVE_LEGACY_COMBAT_HUD_CHROME.md
UI_GLOBALHUD01_PERSISTENT_NAV_ACTION_BAR.md
UI_GRIMROCK_MENU_CURRENT.md
UI_INV01_SELECTED_CHARACTER_SINGLE_BAG.md
UI_INV02_PARTY_DRAG_TRANSFER.md
UI_INV2E_EQUIPMENT_COMPATIBILITY_MATRIX.md
UI_INV2F_EQUIPMENT_STAT_BONUSES.md
UI_INV2G_DAMAGE_RESISTANCES.md
UI_INV2_CHARACTER_EQUIPMENT_PANEL.md
UI_INVENTORY02_SORT_AND_PROJECTION_FEEDBACK.md
UI_LOCALIZATION01_10A_RESIDUAL_P0_PRESENTATION.md
UI_LOCALIZATION01_11A_RECRUITMENT_ERROR_BOUNDARY.md
UI_LOCALIZATION01_12_FINAL_P0_CLOSURE_AUDIT.md
UI_LOCALIZATION01_1_TALENT_TEXTSET_INTEGRITY.md
UI_LOCALIZATION01_2_PLAYER_FACING_CPP_TEXT_AUDIT.md
UI_LOCALIZATION01_4A_STRING_TABLE_CSV_IMPORTER.md
UI_LOCALIZATION01_5A_CHARACTER_CREATION_TEXT_MIGRATION.md
UI_LOCALIZATION01_6A_UNREAL_LOCALIZATION_TARGET.md
UI_LOCALIZATION01_7A_STATUS_EFFECT_PRESENTATION.md
UI_LOCALIZATION01_8A_INTERACTION_PROMPTS.md
UI_LOCALIZATION01_9A_COMBAT_PRESENTATION.md
UI_LOCALIZATION01_SUMMARY.md
UI_LOCALIZATION02_1_LOCALIZATION_COVERAGE_AUDIT.md
UI_LOCALIZATION02_2_FULL_GAME_GATHER.md
UI_LOCALIZATION02_SUMMARY.md
UI_LOCALIZATION03_1_WBP_TEXT_OWNERSHIP_AUDIT.md
UI_LOCALIZATION03_SUMMARY.md
UI_NAV01_PERSISTENT_BOTTOM_NAVIGATION.md
UI_NAVIGATION_MENU.md
UI_PARTYHUD01_PERSISTENT_PARTY_PORTRAIT_RAIL.md
UI_PARTYHUD02_PROGRESSION_WEIGHT_BADGES.md
UI_RPG_CODE_AUDIT01_DEAD_COMPATIBILITY_PATHS.md
UI_RPG_CONTENT_TEXT01_4_FINAL_AUDIT.md
UI_RPG_CONTENT_TEXT02_4_FINAL_AUDIT.md
UI_RPG_CONTENT_TEXT03_1_STATUS_OWNERSHIP_AUDIT.md
UI_RPG_CONTENT_TEXT03_4_FINAL_AUDIT.md
UI_RPG_CONTENT_TEXT04_1_MONCAT_OWNERSHIP_AUDIT.md
UI_RPG_CONTENT_TEXT04_4_FINAL_AUDIT.md
UI_RPG_CONTENT_TEXT05_1_REMAINING_CPP_TEXT_AUTHORITY_AUDIT.md
UI_RPG_CONTENT_TEXT05_4_SPELL_MATERIALIZATION_AUDIT.md
UI_RPG_CONTENT_TEXT05_5_SPELL_DATAASSET_AUTHORING.md
UI_RPG_CONTENT_TEXT05_6_SPELL_RUNTIME_ASSET_AUTHORITY.md
UI_RPG_CONTENT_TEXT05_7_FINAL_CLOSURE.md
UI_RPG_DESC01_10_PRIEST_TALENT_AUDIT.md
UI_RPG_DESC01_11_ALCHEMIST_TALENT_AUDIT.md
UI_RPG_DESC01_12_TALENT_NORMALIZATION_90.md
UI_RPG_DESC01_13_CPP_TALENT_AUDIT_90.md
UI_RPG_DESC01_14_1_STRUCTURED_READ_MODEL_SOURCE.md
UI_RPG_DESC01_16_2_SIX_CLASS_PIE_QA.md
UI_RPG_DESC01_17_FINAL_CLOSURE.md
UI_RPG_DESC01_1_RICH_TALENT_DETAIL_PROJECTION.md
UI_RPG_DESC01_4_UNIFIED_TALENT_DETAIL_UX.md
UI_RPG_DESC01_6_WARRIOR_TALENT_AUDIT.md
UI_RPG_DESC01_7_ROGUE_TALENT_AUDIT.md
UI_RPG_DESC01_8_RANGER_TALENT_AUDIT.md
UI_RPG_DESC01_9_MAGE_TALENT_AUDIT.md
UI_RPG_TALENT_CODE_AUDIT02_3_RUNTIME_CACHE_OWNERSHIP.md
UI_RPG_TALENT_CODE_AUDIT02_4_DETAIL_READ_MODEL.md
UI_RPG_TALENT_CODE_AUDIT02_5_PRESENTATION_DEPENDENCY.md
UI_RPG_TALENT_CODE_AUDIT02_6_NODE_STATE_STYLE.md
UI_RPG_TALENT_CODE_AUDIT02_7_SOURCE_SPLIT.md
UI_RPG_TALENT_CODE_AUDIT02_8_PRESENTATION_DATA_AUDIT.md
UI_RPG_TALENT_TREE_ARCHITECTURE.md
UI_RPG_TEXT01_10_FINAL_AUDIT.md
UI_RPG_TEXT01_LOCALIZATION_POLICY.md
UI_RPG_VARIANT_FLYOUT01.md
UI_RPG_VISUAL01_4_FINAL_AUDIT.md
UI_SKILLS02_1_ARCHITECTURE_DETAIL_READ_MODEL.md
UI_SKILLS02_2_SELECTION_PRESENTATION_PIPELINE.md
UI_SKILLS02_3_COMPACT_SKILL_ENTRY.md
UI_SKILLS02_4_SKILL_DETAIL_WIDGET.md
UI_SKILLS_AFFINITY01_CLASS_AFFINITY.md
UI_SKILLS_CODE_AUDIT02_1_DISCOURAGED_SKILL_DETAIL_PROJECTION.md
UI_SKILLS_CODE_AUDIT02_2_SKILL_READ_MODEL_INVARIANT_HARDENING.md
UI_SKILLS_CODE_AUDIT02_3_SESSION_UNDO_RECONCILIATION.md
UI_SKILLS_CODE_AUDIT02_4_DETERMINISTIC_LOCALIZED_SKILL_SORTING.md
UI_SKILLS_CODE_AUDIT02_5_CANONICAL_SKILL_RESOLUTION_LOADING.md
UI_SKILLS_CODE_AUDIT02_6_SKILL_SUMMARY_MIGRATION_CLOSURE.md
UI_SKILLS_CODE_AUDIT02_7_SKILL_ERROR_BOUNDARY_LOCALIZATION_POLISH.md
UI_SKILLS_CODE_AUDIT02_8_FINAL_CLOSURE.md
UI_SKILLS_SORT01_COLUMN_SORTING.md
UI_SKILLS_SUMMARY01_SPLIT_SUMMARY.md
UI_SPELLBOOK_HOTBAR_EXECUTION.md
UI_SPELLBOOK_HOTBAR_INTERACTION.md
UI_SPELLBOOK_WIDGET.md
UI_SPLIT01_INDEPENDENT_INVENTORY_WINDOWS.md
UI_SPLIT02_WORLD_INTERACTION_AND_CONTEXT_MENU.md
UI_SPLIT03_ROLE_AWARE_PAPERDOLL.md
UI_TEXT_CENTRAL01_1_SKILL_VOCABULARY_FOUNDATION.md
UI_TEXT_CENTRAL01_2A_TALENT_TEXTSET_AUDIT.md
UI_TEXT_CENTRAL01_2C_TALENT_RUNTIME_RESOLVER.md
UI_TEXT_CENTRAL01_2D1_RECIPE_CONTENT_AUTHORITY.md
UI_TEXT_CENTRAL01_2D2_PHYSICAL_TALENT_LEGACY_PURGE.md
UI_TEXT_CENTRAL01_2D3_LEGACY_METADATA_CLEANUP.md
UI_TEXT_CENTRAL01_3_SKILL_REMAINING_VOCABULARY.md
UI_TEXT_DIRECT01_DIRECT_ST_UI_BINDINGS.md
UI_TOOLTIP_CPP_DEAD_CODE_AUDIT_2026_10_26.md
UI_WBP_AUDIT02_REAL_REFERENCERS.md
UI_WEIGHT01_INVENTORY_WEIGHT_FEEDBACK.md
WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md
```

[référence à maintenir]

```text
DEVELOPMENT_ENVIRONMENT_SETUP.md
README.md
```

[spécification à confronter]

```text
ARMEMENT01_2_DICE_ENGINE_TECHNICAL_CONTRACT.md
ARMEMENT_LOT02_2_CANDIDATE_NEWGAME_CONTRACT.md
ARMEMENT_LOT02_4_RANGED_AMMUNITION_CONTRACT.md
ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md
MON17_1_GOBLIN_THROWER_DEFINITION_SPAWN_CONTRACT.md
MON17_7_FINAL_BALANCE_CONTRACT.md
MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md
MON19_2_1A_EVENT_COMMAND_CONNECTOR_CONTRACT.md
MON20_5_1_CUSTOM_RECRUIT_WIZARD_REUSE_CONTRACT.md
MON20_6_1_SKILLS_ARCHITECTURE_CONTRACT.md
MON20_7_3_LEVEL_UP_TALENT_PRESENTATION_CONTRACT.md
MON20_8_1_CROSS_SYSTEM_REQUIREMENTS_ACTIONS_UI_CONTRACT.md
MON20_9_1_PERSISTENCE_MIGRATION_ARCHITECTURE_CONTRACT.md
MON21_6_1_MAP_ARCHITECTURE_CONTRACT.md
PROJECTILE_FRAME01_GEOMETRY_COLLISION_CONTRACT.md
TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md
TRAP01_1_HIDDEN_DUNGEON_TRAP_CONTRACT.md
TRAP_SEARCH01_ACTIVE_SEARCH_CONTRACT.md
UI_EVENTHUD01_EVENT_FEED_CONTRACT.md
UI_LOCALIZATION01_3_UI_STRING_TABLE_CSV_AUTHORING_CONTRACT.md
UI_RPG02_1_PRESENTATION_DATA_CONTRACT.md
UI_RPG_DESC01_13_TALENT_READ_MODEL_CONTRACT.md
UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md
```


**`docs/Design/Images/Receptacle/` — 12**

[ressource / provenance à conserver]

```text
receptacle_04_interactive_inventory_receptacle.jpg
receptacle_05_typology.jpg
receptacle_07_item_compatibility.jpg
receptacle_08_visual_placement_modes.jpg
receptacle_09_inventory_relationships.jpg
receptacle_10_cursor_problem.jpg
receptacle_11_transfer_patterns.jpg
receptacle_12_transfer_service_architecture.jpg
receptacle_13_mouse_interactions.jpg
receptacle_14_events.jpg
receptacle_16_concrete_cases.jpg
receptacle_17_design_rules.jpg
```


**`docs/Design/Images/UI-RPG/` — 1**

[référence à maintenir]

```text
README.md
```


**`docs/Images/` — 30**

[ressource / provenance à conserver]

```text
CORE_DUNGEON_LEVEL_GRID_IMAGES.md
core_20_1_dungeon_binder.svg
core_20_2_level_grid_map.svg
core_20_3_cell_four_walls.svg
core_20_4_editor_flow.svg
core_20_5_runtime_flow.svg
door_10_1_command_state_flow.svg
door_10_2_mechanism_sources.svg
door_10_3_state_consistency.svg
item_10_1_pickup_accessibility.svg
item_10_2_cursor_transfer_flow.svg
item_10_3_torch_holder_model.svg
item_10_4_world_inventory_receptacle.svg
link_10_1_event_command_flow.svg
link_10_2_condition_evaluation.svg
link_10_3_editor_link_workflow.svg
mouse_10_1_click_priority.svg
mouse_10_2_visibility_trace.svg
mouse_10_3_edge_interaction_rules.svg
object_10_1_archetype_palette_placed_runtime.svg
object_10_2_copy_vs_reference.svg
object_10_3_editor_to_runtime_object_flow.svg
readable_10_1_message_lifecycle.svg
readable_10_2_click_priority.svg
readable_10_3_feedback_types.svg
readable_10_4_cursor_states.svg
receptacle_10_1_mouse_drop_flow.svg
receptacle_10_2_acceptance_rules.svg
receptacle_10_3_events_conditions_commands.svg
validation_10_1_message_flow.svg
```


**`docs/Images/LockSystem/` — 5**

[ressource / provenance à conserver]

```text
alternative_unlock_runtime_sequence.png
door_vs_lock_architecture.png
key_compatibility.png
lock_system_overview.png
trapped_locks.png
```


**`docs/Rules/` — 7**

[référence à maintenir]

```text
RPG_Class_Progression_1_20_v0_1.md
RPG_Core_Rules_v0_1.md
RPG_Damage_Types_Countermeasures_v0_1.md
RPG_Equipment_Sets_Tiers_1_5.md
RPG_Shields_Protection_Contract_v0_1.md
RPG_Shields_Protection_Contract_v0_2.md
RPG_Talents_Mechanics_v0_1.md
```


**`docs/Tests/` — 3**

[référence ou spécification : relecture requise]

```text
RECEPTACLE_RUNTIME_TESTS.md
TEST_DUNGEON_PASS_CHECKLIST.md
TEST_DUNGEON_PASS_REPORT.md
```


**`docs/UI/` — 1**

[référence ou spécification : relecture requise]

```text
UI_DESIGN_SURFACE_WIDGET.md
```


</details>

<details>
<summary>Assets Content/ — 1325 entrées</summary>


**`Content/GrimrockPrototype/Art/Materials/Catalyst/` — 3**

```text
M_SurfaceCatalyst_Ember.uasset
M_SurfaceCatalyst_Flask.uasset
M_SurfaceCatalyst_RunePulse.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Functions/` — 1**

```text
MF_MonsterDeathDissolveMask.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Assets/` — 20**

```text
MI_Button_01.uasset
MI_CopperKey.uasset
MI_CopperWallLock.uasset
MI_GoldGlyphe_01.uasset
MI_Granite_01.uasset
MI_Guardian_01.uasset
MI_IronKey.uasset
MI_IronWallLock.uasset
MI_Ivory_01.uasset
MI_LeverSupport_01.uasset
MI_Lever_01.uasset
MI_Rock_01.uasset
MI_SewerGrid_01.uasset
MI_Support_01.uasset
MI_Support_02.uasset
MI_TorchSupport_01.uasset
MI_Torch_01.uasset
MI_TrapFloor.uasset
MI_TrapWall.uasset
MI_WallPanel_01.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Ceiling/` — 4**

```text
MI_CeilEditing.uasset
MI_Ceil_Stones_01.uasset
MI_Ceil_Stones_02.uasset
MI_Ceil_Wood_01.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Decorations/` — 8**

```text
MI_FloorBloodStain_01.uasset
MI_FloorCarpet_01.uasset
MI_FloorDust_01.uasset
MI_FloorMoss_01.uasset
MI_FloorRoots_01.uasset
MI_FloorRuneCircle_Blue.uasset
MI_Roots_01.uasset
MI_Statue_Marble_01.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Door/` — 5**

```text
MI_DoorChain_01.uasset
MI_Door_ColumnStone_01.uasset
MI_Grating_01.uasset
MI_Iron_01.uasset
MI_Wood_02.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Equipment/` — 1**

```text
MI_T1_Boots_Thief.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Flame/` — 3**

```text
MI_Distortion_1.uasset
MI_Flame_4.uasset
MI_Flame_8.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Floor/` — 1**

```text
MI_Floor_Stone_02.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Items/` — 10**

```text
MI_Catalyst.uasset
MI_Catalyst_Reinforcement.uasset
MI_Flask_Glass.uasset
MI_Flask_Oil.uasset
MI_Flask_Reinforcement.uasset
MI_Gem_Blue.uasset
MI_Item_DisarmTools.uasset
MI_Potion_Healing.uasset
MI_Potion_LiquidFlow.uasset
MI_Potion_Reinforcement.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Wall/` — 6**

```text
MI_Wall_Back_01.uasset
MI_Wall_Stone_01.uasset
MI_Wall_Stone_02.uasset
MI_Wall_Stone_03.uasset
MI_Wall_Stone_04.uasset
MI_Wall_Stone_05.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Instances/Weapons/` — 24**

```text
MI_ArcaneOrb.uasset
MI_ArcaneWand.uasset
MI_Arrow.uasset
MI_AssassinDagger.uasset
MI_Bow.uasset
MI_Dagger.uasset
MI_Dart.uasset
MI_ElementalStaff.uasset
MI_FireWand.uasset
MI_GreatAxe.uasset
MI_HolyAegis.uasset
MI_HolyScepter.uasset
MI_Javelin.uasset
MI_LongSword.uasset
MI_Mace.uasset
MI_MainGauche.uasset
MI_RecurveBow.uasset
MI_Shield.uasset
MI_ShortSpear.uasset
MI_Shuriken.uasset
MI_Stiletto.uasset
MI_StraightSword.uasset
MI_ThrowingKnife_02.uasset
MI_WarPick.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Item_Materials/` — 6**

```text
M_Flask_Glass.uasset
M_Flask_Oil.uasset
M_Gem_Transparent.uasset
M_Item_WorldSparkle.uasset
M_Potion_LiquidFlow.uasset
T_Potion_FlowWisps_512.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Masters/` — 9**

```text
M_Ceiling_Editor.uasset
M_EditorGrid.uasset
M_FloorRuneCircle_Additive.uasset
M_GrimrockSurface_Masked_Master.uasset
M_GrimrockSurface_Master.uasset
M_Master_Distortion.uasset
M_Master_Flame.uasset
M_Object_Hover.uasset
M_PP_EditorOutline.uasset
```

**`Content/GrimrockPrototype/Art/Materials/Surfaces/` — 2**

```text
MI_Surface_Oil.uasset
M_Surface_Oil.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Button/` — 3**

```text
T_ButtonSigil_BC.uasset
T_ButtonSigil_N.uasset
T_ButtonSigil_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/CopperKey/` — 3**

```text
T_CopperKey_01_BC.uasset
T_CopperKey_01_N.uasset
T_CopperKey_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/CopperWallLock/` — 3**

```text
T_CopperWallLock_01_BC.uasset
T_CopperWallLock_01_N.uasset
T_CopperWallLock_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Granite_01/` — 3**

```text
T_Granite_01_BC.uasset
T_Granite_01_N.uasset
T_Granite_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/IronKey/` — 3**

```text
T_IronKey_01_BC.uasset
T_IronKey_01_N.uasset
T_IronKey_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/IronWallLock/` — 3**

```text
T_IronWallLock_01_BC.uasset
T_IronWallLock_01_N.uasset
T_IronWallLock_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Ivory/` — 3**

```text
T_Ivory_01_BC.uasset
T_Ivory_01_N.uasset
T_Ivory_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Lever/` — 8**

```text
T_LeverSupport_01_BC.uasset
T_LeverSupport_01_E.uasset
T_LeverSupport_01_N.uasset
T_LeverSupport_01_ORM.uasset
T_Lever_01_BC.uasset
T_Lever_01_E.uasset
T_Lever_01_N.uasset
T_Lever_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Rock/` — 3**

```text
T_Rock_01_BC.uasset
T_Rock_01_N.uasset
T_Rock_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/SewerGrid/` — 4**

```text
T_SewerGrid_01_BC.uasset
T_SewerGrid_01_E.uasset
T_SewerGrid_01_N.uasset
T_SewerGrid_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Statue/` — 4**

```text
T_StatueMarble_01_BC.uasset
T_StatueMarble_01_E.uasset
T_StatueMarble_01_N.uasset
T_StatueMarble_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/Torch/` — 4**

```text
T_Torch_01_BC.uasset
T_Torch_01_E.uasset
T_Torch_01_N.uasset
T_Torch_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Assets/TorchSupport/` — 4**

```text
T_TorchSupport_01_BC.uasset
T_TorchSupport_01_E.uasset
T_TorchSupport_01_N.uasset
T_TorchSupport_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Ceiling/Stone_01/` — 3**

```text
T_Ceil_Stone_01_BC.uasset
T_Ceil_Stone_01_N.uasset
T_Ceil_Stone_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Ceiling/Wood_01/` — 3**

```text
T_Wood_01_BC.uasset
T_Wood_01_N.uasset
T_Wood_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/Blood_01/` — 4**

```text
T_FloorBloodStain_01.uasset
T_FloorBloodStain_01_N.uasset
T_FloorBloodStain_02.uasset
T_FloorBloodStain_03.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/Carpet_01/` — 2**

```text
T_Deco_FloorCarpet_01_N.uasset
T_Deco_FloorCarpet_01_RGBA.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/FloorDust_01/` — 4**

```text
T_Deco_FloorDust_01_BC.uasset
T_Deco_FloorDust_01_M.uasset
T_Deco_FloorDust_01_N.uasset
T_Deco_FloorDust_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/Moss_01/` — 2**

```text
T_Deco_FloorMoss_01_N.uasset
T_Deco_FloorMoss_01_RGBA.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/Roots_01/` — 1**

```text
T_FloorRoots_01_RGBA.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Floor/RuneCircle_01/` — 1**

```text
T_FloorRuneCircle_RGBA.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Decorations/Wall/` — 3**

```text
T_Vine_Dry_01_BC.uasset
T_Vine_Dry_01_N.uasset
T_Vine_Dry_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Door/Grating/` — 8**

```text
T_Door_ChainMoving_01_BC.uasset
T_Door_ChainMoving_01_E.uasset
T_Door_ChainMoving_01_N.uasset
T_Door_ChainMoving_01_ORM.uasset
T_Grating_01_BC.uasset
T_Grating_01_E.uasset
T_Grating_01_N.uasset
T_Grating_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Door/Iron/` — 3**

```text
T_IronDoor_BC.uasset
T_IronDoor_N.uasset
T_IronDoor_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Door/Wood_02/` — 3**

```text
T_Wood_02_BC.uasset
T_Wood_02_N.uasset
T_Wood_02_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Door/Wood_03/` — 3**

```text
T_Wood_03_BC.uasset
T_Wood_03_N.uasset
T_Wood_03_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Equipment/T1_Thief/` — 4**

```text
T_T1_Boots_Thief_BC.uasset
T_T1_Boots_Thief_E.uasset
T_T1_Boots_Thief_N.uasset
T_T1_Boots_Thief_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Flame/` — 6**

```text
T_Flame_1.uasset
T_Flame_4.uasset
T_Flame_8.uasset
T_RefractFlowmap_1.uasset
T_RefractMask_1.uasset
T_RefractNormal_1.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Floor/Stone_01/` — 3**

```text
T_Floor_Stone_01_BC.uasset
T_Floor_Stone_01_N.uasset
T_Floor_Stone_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Floor/Teleporter_01/` — 4**

```text
T_Teleporter_BC_01.uasset
T_Teleporter_E_01.uasset
T_Teleporter_N_01.uasset
T_Teleporter_ORM_01.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Items/Bottles/` — 9**

```text
T_BottleCatalyst_BC.uasset
T_BottleCatalyst_N.uasset
T_BottleCatalyst_ORM.uasset
T_Flask_Reinforcement_BC.uasset
T_Flask_Reinforcement_N.uasset
T_Flask_Reinforcement_ORM.uasset
T_Potion_Reinforcement_BC.uasset
T_Potion_Reinforcement_N.uasset
T_Potion_Reinforcement_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Items/DisarmTools/` — 3**

```text
T_Item_DisarmTools_01_BC.uasset
T_Item_DisarmTools_01_N.uasset
T_Item_DisarmTools_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Metalness/Metal_01/` — 3**

```text
T_Metal_01_BC.uasset
T_Metal_01_N.uasset
T_Metal_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Metalness/Metal_02/` — 3**

```text
T_Metal_02_BC.uasset
T_Metal_02_N.uasset
T_Metal_02_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Metalness/Metal_03/` — 3**

```text
T_Metal_03_BC.uasset
T_Metal_03_N.uasset
T_Metal_03_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Metalness/Metal_04/` — 3**

```text
T_Metal_04_BC.uasset
T_Metal_04_N.uasset
T_Metal_04_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Objects/Statue/` — 3**

```text
T_Guardian_01_BC.uasset
T_Guardian_01_N.uasset
T_Guardian_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Objects/Traps/` — 6**

```text
T_TrapFloor_01_BC.uasset
T_TrapFloor_01_N.uasset
T_TrapFloor_01_ORM.uasset
T_TrapWall_01_BC.uasset
T_TrapWall_01_N.uasset
T_TrapWall_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Granite_01/` — 3**

```text
T_Wall_Stone_05_BC.uasset
T_Wall_Stone_05_N.uasset
T_Wall_Stone_05_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Ground_01/` — 3**

```text
T_Wall_BackGround_BC.uasset
T_Wall_BackGround_N.uasset
T_Wall_BackGround_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Marble_01/` — 3**

```text
T_Marble_01_BC.uasset
T_Marble_01_N.uasset
T_Marble_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Stone_01/` — 3**

```text
T_Wall_Stone_01_BC.uasset
T_Wall_Stone_01_N.uasset
T_Wall_Stone_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Stone_02/` — 3**

```text
T_Wall_Stone_02_BC.uasset
T_Wall_Stone_02_N.uasset
T_Wall_Stone_02_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Wall/Stone_03/` — 3**

```text
T_Wall_Stone_03_BC.uasset
T_Wall_Stone_03_N.uasset
T_Wall_Stone_03_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/ArcaneOrb/` — 3**

```text
T_ArcaneOrb_01_BC.uasset
T_ArcaneOrb_01_N.uasset
T_ArcaneOrb_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/ArcaneWand/` — 3**

```text
T_ArcaneWand_01_BC.uasset
T_ArcaneWand_01_N.uasset
T_ArcaneWand_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Arrow/` — 3**

```text
T_Item_Arrows_BC.uasset
T_Item_Arrows_N.uasset
T_Item_Arrows_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/AssassinDagger/` — 3**

```text
T_AssassinDagger_01_BC.uasset
T_AssassinDagger_01_N.uasset
T_AssassinDagger_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Bow/` — 3**

```text
T_Bow_01_BC.uasset
T_Bow_01_N.uasset
T_Bow_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Dagger/` — 3**

```text
T_Dagger_01_BC.uasset
T_Dagger_01_N.uasset
T_Dagger_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Dart/` — 3**

```text
T_Dart_01_BC.uasset
T_Dart_01_N.uasset
T_Dart_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/ElementalStaff/` — 3**

```text
T_ElementalStaff_01_BC.uasset
T_ElementalStaff_01_N.uasset
T_ElementalStaff_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/FireWand/` — 3**

```text
T_FireWand_01_BC.uasset
T_FireWand_01_N.uasset
T_FireWand_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/GreatAxe/` — 3**

```text
T_GreatAxe_01_BC.uasset
T_GreatAxe_01_N.uasset
T_GreatAxe_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/HolyAegis/` — 3**

```text
T_HolyAegis_01_BC.uasset
T_HolyAegis_01_N.uasset
T_HolyAegis_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/HolyScepter/` — 3**

```text
T_HolyScepter_01_BC.uasset
T_HolyScepter_01_N.uasset
T_HolyScepter_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Javelin/` — 3**

```text
T_Javelin_01_BC.uasset
T_Javelin_01_N.uasset
T_Javelin_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/LongSword/` — 3**

```text
T_LongSword_01_BC.uasset
T_LongSword_01_N.uasset
T_LongSword_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Mace/` — 3**

```text
T_Mace_01_BC.uasset
T_Mace_01_N.uasset
T_Mace_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/MainGauche/` — 3**

```text
T_MainGauche_01_BC.uasset
T_MainGauche_01_N.uasset
T_MainGauche_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/RecurveBow/` — 3**

```text
T_RecurveBow_01_BC.uasset
T_RecurveBow_01_N.uasset
T_RecurveBow_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Shield/` — 3**

```text
T_Shield_01_BC.uasset
T_Shield_01_N.uasset
T_Shield_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/ShortSpear/` — 3**

```text
T_ShortSpear_01_BC.uasset
T_ShortSpear_01_N.uasset
T_ShortSpear_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Shuriken/` — 4**

```text
T_Shuriken_01_BC.uasset
T_Shuriken_01_E.uasset
T_Shuriken_01_N.uasset
T_Shuriken_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/Stiletto/` — 3**

```text
T_Stiletto_01_BC.uasset
T_Stiletto_01_N.uasset
T_Stiletto_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/StraightSword/` — 3**

```text
T_StraightSword_01_BC.uasset
T_StraightSword_01_N.uasset
T_StraightSword_01_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/` — 3**

```text
T_ThrowingKnife_02_BC.uasset
T_ThrowingKnife_02_N.uasset
T_ThrowingKnife_02_ORM.uasset
```

**`Content/GrimrockPrototype/Art/Textures/Final/Weapons/WarPick/` — 3**

```text
T_WarPick_01_BC.uasset
T_WarPick_01_N.uasset
T_WarPick_01_ORM.uasset
```

**`Content/GrimrockPrototype/Audio/Button/` — 2**

```text
S_Button_Secret.uasset
S_PressButton.uasset
```

**`Content/GrimrockPrototype/Audio/Environment/Doors/` — 8**

```text
SA_Door_3D.uasset
S_Door_Grating_01.uasset
S_Door_Grating_Close_01.uasset
S_Door_Stone_Close_01.uasset
S_Door_Stone_Open_01.uasset
S_Door_Wood_Open_01.uasset
S_IronDoorOpenClose.uasset
S_Pull_Chain.uasset
```

**`Content/GrimrockPrototype/Audio/Fire/` — 3**

```text
S_BottleBreak.uasset
S_BottleThrowSound.uasset
S_FireCrackleLoopSound.uasset
```

**`Content/GrimrockPrototype/Audio/Floor/` — 1**

```text
S_PressurePlate.uasset
```

**`Content/GrimrockPrototype/Audio/Lever/` — 1**

```text
S_Lever.uasset
```

**`Content/GrimrockPrototype/Audio/Party/Movement/` — 10**

```text
S_Party_BlockedImpact_Stone_01.uasset
S_Party_BlockedImpact_Stone_02.uasset
S_Party_FallImpact_Stone_01.uasset
S_Party_FallImpact_Stone_02.uasset
S_Party_Fall_Scream_01.uasset
S_Party_Fall_Scream_02.uasset
S_Party_Footstep_Stone_01.uasset
S_Party_Footstep_Stone_02.uasset
S_Party_Footstep_Stone_03.uasset
S_Party_Footstep_Stone_04.uasset
```

**`Content/GrimrockPrototype/Audio/WallLock/` — 1**

```text
S_WallLock.uasset
```

**`Content/GrimrockPrototype/Audio/Weapons/` — 1**

```text
S_WeaponThinBlade.uasset
```

**`Content/GrimrockPrototype/Audio/Weapons/Sword/Swing/` — 4**

```text
SFX_Sword_Swing_Standard_01.uasset
SFX_Sword_Swing_Standard_02.uasset
SFX_Sword_Swing_Standard_03.uasset
SFX_Sword_Swing_Standard_04.uasset
```

**`Content/GrimrockPrototype/Blueprints/Editor/` — 2**

```text
BP_GridEditorPreviewObjectActor.uasset
BP_GridLevelEditorActor.uasset
```

**`Content/GrimrockPrototype/Blueprints/Runtime/` — 15**

```text
BP_GridButtonActor.uasset
BP_GridDoorActor.uasset
BP_GridGenericObjectActor.uasset
BP_GridLevelRuntimeActor.uasset
BP_GridLeverActor.uasset
BP_GridPressurePlateActor.uasset
BP_GridReceptacleActor.uasset
BP_GridRuntimeObjectActor.uasset
BP_GridSecretDoor.uasset
BP_GridTriggerActor.uasset
BP_GridWallLockActor.uasset
BP_GrimrockGameMode.uasset
BP_GrimrockPartyPawn.uasset
BP_GrimrockPlayerController.uasset
BP_Receptacle_WallTorchHolder.uasset
```

**`Content/GrimrockPrototype/Blueprints/System/` — 3**

```text
BP_GrimrockGameInstance.uasset
BP_MainMenuGameMode.uasset
BP_MainMenuPlayerController.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Buttons/Action/` — 4**

```text
T_Border_CombatHUD_Disabled.uasset
T_Border_CombatHUD_Hovered.uasset
T_Border_CombatHUD_Normal.uasset
T_Border_CombatHUD_Pressed.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Buttons/Generic/` — 4**

```text
T_ButtonGeneric_Disabled.uasset
T_ButtonGeneric_Hovered.uasset
T_ButtonGeneric_Normal.uasset
T_ButtonGeneric_Pressed.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Buttons/Wizard/` — 12**

```text
T_ButtonMenu_Disabled_480x100.uasset
T_ButtonMenu_Hovered_480x100.uasset
T_ButtonMenu_Normal_480x100.uasset
T_ButtonMenu_Pressed_480x100.uasset
T_GenderFemale_ButtonIcon_Disabled_512x512.uasset
T_GenderFemale_ButtonIcon_Hovered_512x512.uasset
T_GenderFemale_ButtonIcon_Normal_512x512.uasset
T_GenderFemale_ButtonIcon_Pressed_512x512.uasset
T_GenderMaleButtonIcon_Disabled_512x512.uasset
T_GenderMaleButtonIcon_Hovered_512x512.uasset
T_GenderMaleButtonIcon_Normal_512x512.uasset
T_GenderMaleButtonIcon_Pressed_512x512.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Combat/` — 4**

```text
WBP_GridCombatActionPanel.uasset
WBP_GridCombatHud.uasset
WBP_GridCombatHudAction.uasset
WBP_GridCombatHudInitiativeSlot.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Cursor/` — 11**

```text
Cursor_Aim.uasset
Cursor_CannotPlaceItem.uasset
Cursor_Default.uasset
Cursor_Forbidden.uasset
Cursor_None.uasset
Cursor_PlaceItem.uasset
Cursor_Pull.uasset
Cursor_Push.uasset
Cursor_Read.uasset
Cursor_Take.uasset
Cursor_Use.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/EventFeed/` — 3**

```text
WBP_GridCombatArmorRow.uasset
WBP_GridCombatTooltip.uasset
WBP_GridEventFeed.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Fonts/` — 2**

```text
F_Carolingia.uasset
carolingia.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Icons/` — 30**

```text
T_Background_Menu_01.uasset
T_BorderCharacter.uasset
T_Border_Character.uasset
T_Border_Character_01.uasset
T_Border_Character_02.uasset
T_Border_Character_Combat.uasset
T_Border_CombatHUD.uasset
T_Border_GoldFine.uasset
T_Border_Menu_02.uasset
T_CheckBox_Checked.uasset
T_CheckBox_CheckedHovered.uasset
T_CheckBox_CheckedPressed.uasset
T_CheckBox_Unchecked.uasset
T_CheckBox_UncheckedHovered.uasset
T_CheckBox_UncheckedPressed.uasset
T_Hand_01.uasset
T_Icon_PA.uasset
T_Icon_Unarmed.uasset
T_LevelUpAlert.uasset
T_Light_01.uasset
T_Mana.uasset
T_PV.uasset
T_Resistance_Arcane.uasset
T_Resistance_Fire.uasset
T_Resistance_Holy.uasset
T_Resistance_Ice.uasset
T_Resistance_Lightning.uasset
T_Resistance_Necrotic.uasset
T_Resistance_Poison.uasset
T_Weight.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Icons/MenuBag/` — 22**

```text
T_Bag.uasset
T_BooksAndKeys.uasset
T_Button_Border.uasset
T_Button_BorderFill.uasset
T_Button_CharacterSheet.uasset
T_Button_Close_Hovered_01.uasset
T_Button_Close_Normal_01.uasset
T_Button_Close_Pressed_01.uasset
T_Characteristis.uasset
T_Consumables.uasset
T_Equipment.uasset
T_Escape.uasset
T_Help.uasset
T_Ingredients.uasset
T_Inventory_Bag_01.uasset
T_Journal.uasset
T_Magic.uasset
T_Map.uasset
T_MapIcon.uasset
T_Misc.uasset
T_Receipts.uasset
T_Subwindow_01.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/InGameMenu/` — 7**

```text
WBP_GridCodex.uasset
WBP_GridJournal.uasset
WBP_GridMap.uasset
WBP_GridRecipes.uasset
WBP_GridSpellbook.uasset
WBP_GridSpellbookEntry.uasset
WBP_GrimrockMenu.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Interaction/` — 2**

```text
WBP_GridMouseCursor.uasset
WBP_ReadableMessage.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Inventory/` — 13**

```text
WBP_CharacterSheet.uasset
WBP_GridNavigationTooltip.uasset
WBP_GridPersistentHud.uasset
WBP_InventoryBag.uasset
WBP_InventorySlot.uasset
WBP_ItemActionButton.uasset
WBP_ItemActionMenu.uasset
WBP_ItemReadPanel.uasset
WBP_ItemTooltip.uasset
WBP_ItemTooltipNativeComparison.uasset
WBP_ItemTooltipNativeLine.uasset
WBP_PartyMember.uasset
WBP_PartyPortraitHud.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/MainMenu/` — 6**

```text
WBP_CreditsMenu.uasset
WBP_LicenseMenu.uasset
WBP_LoadGameMenu.uasset
WBP_LoadGameSlotRow.uasset
WBP_MainMenu.uasset
WBP_OptionsMenu.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/RPG/` — 4**

```text
T_CharacterCreationWizard_RGB.uasset
WBP_CCStep_Attributes.uasset
WBP_CharacterCreationWizard.uasset
WBP_RPGStoryCompanionRecruitment.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/ScrollBar/` — 1**

```text
T_UI_Scrollbar_Track_Vertical.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/` — 1**

```text
WBP_GridSkills.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/` — 1**

```text
T_Talent_ProgressBar.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/` — 8**

```text
T_BannerAlchimist.uasset
T_FlagAlchemist.uasset
T_TalentPanel_AlchemistApothecary.uasset
T_TalentPanel_AlchemistGenadier.uasset
T_TalentPanel_AlchemistTransmuter.uasset
T_Talent_AlchemistApothecary.uasset
T_Talent_AlchemistGrenadier.uasset
T_Talent_AlchemistTransmuter.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/Apothicaire/` — 5**

```text
Antidote.uasset
Diffusion.uasset
Elixir_défensif.uasset
Panacée.uasset
Potion_renforcée.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/Grenadier/` — 5**

```text
Bombe_incendiaire.uasset
Bombe_toxique.uasset
Charge_précise.uasset
Maître_grenadier.uasset
Réaction_en_chaîne.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Alchemist/Transmutateur/` — 5**

```text
Catalyseur.uasset
Flasque_acide.uasset
Huile_glissante.uasset
Nuage_corrosif.uasset
Transmutation_majeure.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Glyphs/` — 20**

```text
T_UI_AffinityStar_Barred.uasset
T_UI_AffinityStar_Filled.uasset
T_UI_AffinityStar_Outline.uasset
T_UI_SkillGlyph_AvalaiblePoints.uasset
T_UI_SkillGlyph_MaxRank.uasset
T_UI_TalentGlyph_Action.uasset
T_UI_TalentGlyph_Area.uasset
T_UI_TalentGlyph_AvalaiblePoints.uasset
T_UI_TalentGlyph_Cooldown.uasset
T_UI_TalentGlyph_Cost.uasset
T_UI_TalentGlyph_Exclusivity.uasset
T_UI_TalentGlyph_ItemCost.uasset
T_UI_TalentGlyph_LevelRequirement.uasset
T_UI_TalentGlyph_LineOfSight.uasset
T_UI_TalentGlyph_ManaCost.uasset
T_UI_TalentGlyph_Prerequisite.uasset
T_UI_TalentGlyph_Range.uasset
T_UI_TalentGlyph_Recipe.uasset
T_UI_TalentGlyph_ResolutionCount.uasset
T_UI_TalentGlyph_Target.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Mage/` — 8**

```text
T_BannerMage.uasset
T_FlagMage.uasset
T_TalentPanel_MageArcanist.uasset
T_TalentPanel_MageEvoker.uasset
T_TalentPanel_MageSurfaceWeaver.uasset
T_Talent_MageArcanist.uasset
T_Talent_MageEvoker.uasset
T_Talent_MageSurfaceWeaver.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Mage/Arcanist/` — 5**

```text
Bouclier_arcaniste.uasset
Dissipation.uasset
Manipulation_runique.uasset
Maîtrise_de_l_arcane.uasset
Téléportation_courte.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Mage/Evoker/` — 9**

```text
Affinité_élémentaire-Air.uasset
Affinité_élémentaire-Feu.uasset
Affinité_élémentaire-Glace.uasset
Affinité_élémentaire-Terre.uasset
Affinité_élémentaire.uasset
Cataclysme.uasset
Chaîne_élémentaire.uasset
Explosion_contrôlée.uasset
Surcharge_élémentaire.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Mage/SurfaceWeaver/` — 9**

```text
Architecte_du_terrain.uasset
Conduction.uasset
Conversion_élémentaire.uasset
Imprégnation-Air.uasset
Imprégnation-Feu.uasset
Imprégnation-Glace.uasset
Imprégnation-Terre.uasset
Imprégnation.uasset
Surface_persistante.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Priest/` — 8**

```text
T_BannerPriest.uasset
T_FlagPriest.uasset
T_TalentPanel_PriestExorcism.uasset
T_TalentPanel_PriestProtection.uasset
T_TalentPanel_PriestRestoration.uasset
T_Talent_PriestExorcism.uasset
T_Talent_PriestProtection.uasset
T_Talent_PriestRestoration.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Priest/Exorcism/` — 5**

```text
Châtiment.uasset
Dissipation_sacrée.uasset
Exorcisme_majeur.uasset
Lumière_sacrée.uasset
Repousser_les_morts-vivants.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Priest/Protection/` — 5**

```text
Bastion_divin.uasset
Bénédiction.uasset
Egide.uasset
Protection_sacrée.uasset
Sanctuaire.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Priest/Restoration/` — 5**

```text
Miracle.uasset
Purification.uasset
Régénération.uasset
Soin_de_groupe.uasset
Soin_renforcé.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/` — 8**

```text
T_BannerRanger.uasset
T_FlagRanger.uasset
T_TalentPanel_RangerHunter.uasset
T_TalentPanel_RangerMarksman.uasset
T_TalentPanel_RangerScout.uasset
T_Talent_RangerHunter.uasset
T_Talent_RangerMarksman.uasset
T_Talent_RangerScout.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/Hunter/` — 7**

```text
Chasseur_alpha.uasset
Ennemi_juré-Gobelin.uasset
Ennemi_juré-Vermine.uasset
Ennemi_juré.uasset
Frappe_du_prédateur.uasset
Marque_de_la_proie.uasset
Tir_immobilisant.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/Marksman/` — 5**

```text
Oeil_d_aigle.uasset
Tir_perforant.uasset
Tir_précis.uasset
Tir_rapide.uasset
Volée.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Ranger/Scout/` — 5**

```text
Guide_du_groupe.uasset
Maître_du_terrain.uasset
Piège_de_chasse.uasset
Repli_tactique.uasset
Vigilance.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Rogue/` — 8**

```text
T_BannerRogue.uasset
T_FlagRogue.uasset
T_TalentPanel_RogueAssassin.uasset
T_TalentPanel_RogueSaboteur.uasset
T_TalentPanel_RogueShadow.uasset
T_Talent_RogueAssassin.uasset
T_Talent_RogueSaboteur.uasset
T_Talent_RogueShadow.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Rogue/Assassin/` — 5**

```text
Attaque_sournoise.uasset
Frappe_dans_le_dos.uasset
Hémorragie.uasset
Mise_à_mort.uasset
Point_faible.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Rogue/Saboteur/` — 5**

```text
Bombe_fumigène.uasset
Désamorçage_expert.uasset
Maître_des_serrures.uasset
Piège_rapide.uasset
Sabotage.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Rogue/Shadow/` — 5**

```text
Disparition_courte.uasset
Esquive.uasset
Insaisissable.uasset
Ombre_parfaite.uasset
Pas_de_l’ombre.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Warrior/` — 8**

```text
T_BannerWarrior.uasset
T_FlagWarrior.uasset
T_TalentPanel_WarriorBeaker.uasset
T_TalentPanel_WarriorGuardian.uasset
T_TalentPanel_WarriorWeaponMaster.uasset
T_Talent_WarriorBreaker.uasset
T_Talent_WarriorGuardian.uasset
T_Talent_WarriorWeaponMaster.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Warrior/Breaker/` — 5**

```text
Balayage.uasset
Brise-armure.uasset
Coup_puissant.uasset
Exécution.uasset
Ravage.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Warrior/Guardian/` — 5**

```text
Coup_de_bouclier.uasset
Forteresse.uasset
Interception.uasset
Posture_défensive.uasset
Rempart.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/Images/Warrior/WeaponMaster/` — 8**

```text
Maîtrise_critique.uasset
Riposte.uasset
Second_souffle.uasset
Seigneur_de_guerre.uasset
Spécialisation_martiale-Contondant.uasset
Spécialisation_martiale-Perforant.uasset
Spécialisation_martiale-Tranchant.uasset
Spécialisation_martiale.uasset
```

**`Content/GrimrockPrototype/Blueprints/UI/Skills/RPG/` — 9**

```text
WBP_RPGNotification.uasset
WBP_RPGSkillDetail.uasset
WBP_RPGSkillEntry.uasset
WBP_RPGTalentBranch.uasset
WBP_RPGTalentDetail.uasset
WBP_RPGTalentDetailLine.uasset
WBP_RPGTalentNode.uasset
WBP_RPGTalentVariantFlyout.uasset
WBP_RPGTalentVariantIcon.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/` — 15**

```text
DA_Button_Normal.uasset
DA_Button_Secret.uasset
DA_CeilingShaft.uasset
DA_Door_Grating.uasset
DA_Door_Iron.uasset
DA_Door_Secret.uasset
DA_Door_Wood.uasset
DA_Lever.uasset
DA_Lock_CopperWall.uasset
DA_Lock_IronWall.uasset
DA_ObjectPalette_Default.uasset
DA_PressurePlate.uasset
DA_Receptacle_TorchHolder.uasset
DA_Trigger.uasset
DA_WallInscription.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/` — 5**

```text
DA_ActionCatalogue.uasset
DA_ActionPresentationCatalogue.uasset
DA_CanonicalWeaponCatalogue.uasset
DA_CanonicalWeaponVisualCatalogue.uasset
DA_ItemSystem.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/` — 48**

```text
DA_Canonical_EmptyVial.uasset
DA_Canonical_Gem_Blue.uasset
DA_Canonical_GoblinKnife.uasset
DA_Canonical_Item_Antidote.uasset
DA_Canonical_Item_Arrows_01.uasset
DA_Canonical_Item_Bomb_Fire.uasset
DA_Canonical_Item_Bomb_Toxic.uasset
DA_Canonical_Item_DefensiveElixir_Fire.uasset
DA_Canonical_Item_DefensiveElixir_Ice.uasset
DA_Canonical_Item_DefensiveElixir_Lightning.uasset
DA_Canonical_Item_DefensiveElixir_Poison.uasset
DA_Canonical_Item_DisarmTools_01.uasset
DA_Canonical_Item_Flask_Acid.uasset
DA_Canonical_Item_Flask_CorrosiveCloud.uasset
DA_Canonical_Item_Flask_Oil.uasset
DA_Canonical_Item_Panacea.uasset
DA_Canonical_Item_Potion_Healing_01.uasset
DA_Canonical_Item_RatMeat.uasset
DA_Canonical_Item_RatTooth.uasset
DA_Canonical_Item_SurfaceCatalyst_01.uasset
DA_Canonical_Item_TestNote.uasset
DA_Canonical_Item_Torch.uasset
DA_Canonical_Key_Copper.uasset
DA_Canonical_Key_Iron.uasset
DA_Canonical_Shuriken.uasset
DA_Canonical_Stone.uasset
DA_Canonical_Weapon_ArcaneOrb_01.uasset
DA_Canonical_Weapon_ArcaneWand_01.uasset
DA_Canonical_Weapon_AssassinDagger_01.uasset
DA_Canonical_Weapon_Dagger_01.uasset
DA_Canonical_Weapon_Dart_01.uasset
DA_Canonical_Weapon_ElementalStaff_01.uasset
DA_Canonical_Weapon_FireWand_01.uasset
DA_Canonical_Weapon_GreatAxe_01.uasset
DA_Canonical_Weapon_HolyAegis_01.uasset
DA_Canonical_Weapon_HolyScepter_01.uasset
DA_Canonical_Weapon_Javelin_01.uasset
DA_Canonical_Weapon_LongSword_01.uasset
DA_Canonical_Weapon_Mace_01.uasset
DA_Canonical_Weapon_MainGauche_01.uasset
DA_Canonical_Weapon_RecurveBow_01.uasset
DA_Canonical_Weapon_RoundShield_01.uasset
DA_Canonical_Weapon_ShortBow_01.uasset
DA_Canonical_Weapon_ShortSpear_01.uasset
DA_Canonical_Weapon_Stiletto_01.uasset
DA_Canonical_Weapon_StraightSword_01.uasset
DA_Canonical_Weapon_ThrowingKnife_01.uasset
DA_Canonical_Weapon_WarPick_01.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/Companions/` — 1**

```text
DA_StoryCompanion_Scout.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/GridDungeons/` — 1**

```text
DA_Dungeon_01.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/` — 24**

```text
DA_Archetype_CustomRecruiter_Service.uasset
DA_Archetype_StoryCompanion_Recruit.uasset
DA_FloorBloodStain.uasset
DA_FloorBones.uasset
DA_FloorCarpet.uasset
DA_FloorDebris.uasset
DA_FloorDust.uasset
DA_FloorMoss.uasset
DA_FloorRoots.uasset
DA_FloorRubble.uasset
DA_FloorRuneCircle.uasset
DA_Guardian.uasset
DA_MonsterSpawn.uasset
DA_Pit_Stone.uasset
DA_Stairs_Down.uasset
DA_Stairs_Up.uasset
DA_Statue_CryptGuardian.uasset
DA_Teleporter.uasset
DA_Trap_Ceiling_01.uasset
DA_Trap_Ceiling_Fire_01.uasset
DA_Trap_Floor_01.uasset
DA_Trap_Floor_Spikes_01.uasset
DA_Trap_Wall_01.uasset
DA_Trap_Wall_Darts_01.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/Decoration/` — 4**

```text
DA_Vine_Wall_Card_01.uasset
DA_Vine_Wall_Card_02.uasset
DA_Vine_Wall_Card_03.uasset
DA_Vine_Wall_Card_04.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/` — 4**

```text
DA_GridLevel_00.uasset
DA_GridLevel_01.uasset
DA_GridLevel_New_Level_03.uasset
DA_GridLevel_Pillars_of_Light.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/Magic/Spells/` — 4**

```text
DA_Spell_ArcaneBolt.uasset
DA_Spell_CurePoison.uasset
DA_Spell_Haste.uasset
DA_Spell_LesserHeal.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/` — 12**

```text
DA_Class_Alchemist.uasset
DA_Class_Mage.uasset
DA_Class_Priest.uasset
DA_Class_Ranger.uasset
DA_Class_Rogue.uasset
DA_Class_Warrior.uasset
DA_Race_Dwarf.uasset
DA_Race_Elf.uasset
DA_Race_Gnome.uasset
DA_Race_HalfOrc.uasset
DA_Race_Halfling.uasset
DA_Race_Human.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/Crafting/` — 1**

```text
DA_RPGRecipeCatalog.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/Skills/` — 25**

```text
DA_Skill_Acrobatics.uasset
DA_Skill_Alchemy.uasset
DA_Skill_Arcana.uasset
DA_Skill_Armor.uasset
DA_Skill_Athletics.uasset
DA_Skill_Crafting.uasset
DA_Skill_Deception.uasset
DA_Skill_HeavyWeapons.uasset
DA_Skill_History.uasset
DA_Skill_Intimidation.uasset
DA_Skill_LightWeapons.uasset
DA_Skill_Lockpicking.uasset
DA_Skill_Mechanics.uasset
DA_Skill_Medicine.uasset
DA_Skill_Nature.uasset
DA_Skill_Perception.uasset
DA_Skill_Persuasion.uasset
DA_Skill_RangedWeapons.uasset
DA_Skill_Religion.uasset
DA_Skill_Runes.uasset
DA_Skill_Shield.uasset
DA_Skill_Stealth.uasset
DA_Skill_Survival.uasset
DA_Skill_Throwing.uasset
DA_Skill_Traps.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/` — 32**

```text
DA_Status_Banished.uasset
DA_Status_Bleeding.uasset
DA_Status_Blessed.uasset
DA_Status_Burning.uasset
DA_Status_Corroded.uasset
DA_Status_DefensiveElixir_Fire.uasset
DA_Status_DefensiveElixir_Ice.uasset
DA_Status_DefensiveElixir_Lightning.uasset
DA_Status_DefensiveElixir_Poison.uasset
DA_Status_DivineBastion.uasset
DA_Status_ElementalImbuement.uasset
DA_Status_ElementalOverload.uasset
DA_Status_Elusive.uasset
DA_Status_Evasive.uasset
DA_Status_ExposedPhysical.uasset
DA_Status_Fortified.uasset
DA_Status_Guarded.uasset
DA_Status_Hidden.uasset
DA_Status_HiddenPerfect.uasset
DA_Status_HolyProtection.uasset
DA_Status_Immobilized.uasset
DA_Status_KnockedDown.uasset
DA_Status_MarkedByRanger.uasset
DA_Status_Poison.uasset
DA_Status_Regeneration.uasset
DA_Status_Sabotaged.uasset
DA_Status_Sanctuary.uasset
DA_Status_ShadowReach.uasset
DA_Status_Slow.uasset
DA_Status_Stunned.uasset
DA_Status_TurnedUndead.uasset
DA_Status_Warlord.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/` — 7**

```text
DA_ClassVisual_Alchemist.uasset
DA_ClassVisual_Mage.uasset
DA_ClassVisual_Priest.uasset
DA_ClassVisual_Ranger.uasset
DA_ClassVisual_Rogue.uasset
DA_ClassVisual_Warrior.uasset
DA_RPGTalentDetailGlyphSet.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/PortraitSets/` — 6**

```text
DA_PortraitSet_Dwarf.uasset
DA_PortraitSet_Elf.uasset
DA_PortraitSet_Gnome.uasset
DA_PortraitSet_HalfOrc.uasset
DA_PortraitSet_Halfling.uasset
DA_PortraitSet_Human.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/Tests/Readables/` — 2**

```text
DA_BookPage_Test.uasset
DA_Scroll_Test.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/UI/RPG/` — 2**

```text
DA_RPGSkillPresentation.uasset
DA_RPGTalentPresentation.uasset
```

**`Content/GrimrockPrototype/Core/DataAssets/Wall/` — 5**

```text
DA_WallGrid.uasset
DA_Wall_Stone_Alcove.uasset
DA_Wall_Stone_Alcove_02.uasset
DA_Wall_Stone_Cracked.uasset
DA_Wall_Stone_SewerDrain.uasset
```

**`Content/GrimrockPrototype/Core/Fonts/` — 15**

```text
AlegreyaSans-Black.uasset
AlegreyaSans-BlackItalic.uasset
AlegreyaSans-Bold.uasset
AlegreyaSans-BoldItalic.uasset
AlegreyaSans-ExtraBold.uasset
AlegreyaSans-ExtraBoldItalic.uasset
AlegreyaSans-Italic.uasset
AlegreyaSans-Light.uasset
AlegreyaSans-LightItalic.uasset
AlegreyaSans-Medium.uasset
AlegreyaSans-MediumItalic.uasset
AlegreyaSans-Regular.uasset
AlegreyaSans-Thin.uasset
AlegreyaSans-ThinItalic.uasset
F_AlegreyaSans.uasset
```

**`Content/GrimrockPrototype/Core/Input/` — 8**

```text
IA_MoveBackward.uasset
IA_MoveForward.uasset
IA_StrafeLeft.uasset
IA_StrafeRight.uasset
IA_TurnLeft.uasset
IA_TurnRight.uasset
IA_Use.uasset
IMC_Grimrock.uasset
```

**`Content/GrimrockPrototype/Effects/Alchimie/` — 2**

```text
M_Potion_Spark.uasset
NS_LiquideSparkle.uasset
```

**`Content/GrimrockPrototype/Effects/Fire/Local/` — 1**

```text
DA_GroundFire.uasset
```

**`Content/GrimrockPrototype/Effects/Fire/Torch/` — 1**

```text
NS_Flame_8_Torch.uasset
```

**`Content/GrimrockPrototype/Effects/Teleporter/` — 7**

```text
MI_Teleporter_Active.uasset
MI_Teleporter_Inactive.uasset
M_Teleporter_Spark.uasset
M_Teleporter_Volume.uasset
M_Teleporter_Wisp.uasset
NS_Teleporter_Blue.uasset
T_Teleporter_Noise_01.uasset
```

**`Content/GrimrockPrototype/Environment/Vines/Materials/` — 4**

```text
MI_Mossy_Root_05.uasset
MI_Mossy_Root_06.uasset
MI_Mossy_Root_07.uasset
M_Vine_Dry_Master.uasset
```

**`Content/GrimrockPrototype/Environment/Vines/Textures/` — 9**

```text
T_Mossy_Root_01_BC.uasset
T_Mossy_Root_01_N.uasset
T_Mossy_Root_01_ORM.uasset
T_Mossy_Root_02_BC.uasset
T_Mossy_Root_02_N.uasset
T_Mossy_Root_02_ORM.uasset
T_Mossy_Root_03_BC.uasset
T_Mossy_Root_03_N.uasset
T_Mossy_Root_03_ORM.uasset
```

**`Content/GrimrockPrototype/Icons/` — 6**

```text
T_Tool_Erase.uasset
T_Tool_Link.uasset
T_Tool_PaintCell.uasset
T_Tool_PaintObject.uasset
T_Tool_PaintWall.uasset
T_Tool_Select.uasset
```

**`Content/GrimrockPrototype/Icons/DungeonEditorIcons/` — 20**

```text
I_Button_Wall.uasset
I_Door_Grating.uasset
I_Door_Iron.uasset
I_Door_Secret.uasset
I_Door_Wood.uasset
I_GreatAlcove.uasset
I_Guardian.uasset
I_GuardianStatue.uasset
I_Lever.uasset
I_PressurePlate.uasset
I_SecretButton.uasset
I_Stairs_Down.uasset
I_Stairs_Up.uasset
I_StoneAlcove.uasset
I_StonePit.uasset
I_Teleporter.uasset
I_TorchSupport.uasset
I_WallInscription.uasset
I_WallLock_Copper.uasset
I_WallLock_Iron.uasset
```

**`Content/GrimrockPrototype/Maps/` — 2**

```text
L_Dungeon.umap
L_MainMenu.umap
```

**`Content/GrimrockPrototype/Meshes/` — 2**

```text
SM_EditorGridPlane.uasset
SM_SecretButton_03.uasset
```

**`Content/GrimrockPrototype/Meshes/Button/` — 4**

```text
SM_Button_Mettalic_Mobile.uasset
SM_Button_Mettalic_Static.uasset
SM_LeverStatic_01.uasset
SM_Lever_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Ceil/` — 2**

```text
SM_CeilVault_01.uasset
SM_CeilingShaft_Stone_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Decorations/Floor/` — 10**

```text
SM_FloorBloodStain_01.uasset
SM_FloorBones.uasset
SM_FloorCarpet_02.uasset
SM_FloorDebris_01.uasset
SM_FloorDecalPlane_01.uasset
SM_FloorDust_01.uasset
SM_FloorRoots_01.uasset
SM_FloorRubble.uasset
SM_FloorRunePlane_01.uasset
SM_Statue_CryptGuardian.uasset
```

**`Content/GrimrockPrototype/Meshes/Decorations/Wall/` — 5**

```text
SM_WallInscription.uasset
SM_Wall_Decoration3D_01.uasset
SM_Wall_Decoration_02.uasset
SM_Wall_Decoration_03.uasset
SM_Wall_Decoration_04.uasset
```

**`Content/GrimrockPrototype/Meshes/Door/` — 9**

```text
SM_DoorStone_Static_02.uasset
SM_Door_Chain_Moving_01.uasset
SM_Door_Chain_Support_01.uasset
SM_Door_Grating_Mobile_01.uasset
SM_Door_IronDoubleLeft_01.uasset
SM_Door_IronDoubleRight_01.uasset
SM_Door_Wood_Mobile_01.uasset
SM_Stairs_Down_01.uasset
SM_Stairs_Up_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Equipment/` — 2**

```text
SM_Leather_Boots_01.uasset
SM_T1_Boots_Thief.uasset
```

**`Content/GrimrockPrototype/Meshes/Floor/` — 7**

```text
SM_Floor_Stone_01.uasset
SM_Grid_PressurePlate_Moving.uasset
SM_Grid_PressurePlate_Static.uasset
SM_Pit_Stone_01.uasset
SM_Pit_Trapdoor_Left_01.uasset
SM_Pit_Trapdoor_Right_01.uasset
SM_Teleporter_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Items/` — 4**

```text
SM_Catalyst_01.uasset
SM_Flasque.uasset
SM_Item_DisarmTools_01.uasset
SM_Potion_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Object/` — 12**

```text
SM_BlueGem_01.uasset
SM_Guardien_Face_01.uasset
SM_Item_TestNote.uasset
SM_Key_Copper.uasset
SM_Key_Iron.uasset
SM_Lock_CopperWallLock_.uasset
SM_Lock_IronWall.uasset
SM_Stone_01.uasset
SM_Torch.uasset
SM_Torch_Support.uasset
SM_TrapFloor_01.uasset
SM_TrapWall_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Wall/` — 8**

```text
SM_Wall_Alcove_02.uasset
SM_Wall_Grating_02.uasset
SM_Wall_Stone_05.uasset
SM_Wall_Stone_Alcove_01.uasset
SM_Wall_Stone_Cracked_02.uasset
SM_Wall_Stone_SecretDoor-01.uasset
SM_Wall_Stone_SecretDoorStatic-01.uasset
SM_Wall_Stone_SewerDrain_01.uasset
```

**`Content/GrimrockPrototype/Meshes/Weapons/` — 24**

```text
SM_Item_Arrows_01.uasset
SM_Shuriken_01.uasset
SM_Weapon_ArcaneOrb_01.uasset
SM_Weapon_ArcaneWand_01.uasset
SM_Weapon_AssassinDagger_01.uasset
SM_Weapon_Dagger_01.uasset
SM_Weapon_Dart_01.uasset
SM_Weapon_ElementalStaff_01.uasset
SM_Weapon_FireWand_01.uasset
SM_Weapon_GreatAxe_01.uasset
SM_Weapon_HolyAegis_01.uasset
SM_Weapon_HolyScepter_01.uasset
SM_Weapon_Javelin_01.uasset
SM_Weapon_LongSword_01.uasset
SM_Weapon_Mace_01.uasset
SM_Weapon_MainGauche_01.uasset
SM_Weapon_RecurveBow_01.uasset
SM_Weapon_RoundShield_01.uasset
SM_Weapon_ShortBow_01.uasset
SM_Weapon_ShortSpear_01.uasset
SM_Weapon_Stiletto_01.uasset
SM_Weapon_StraightSword_01.uasset
SM_Weapon_ThrowingKnife_01.uasset
SM_Weapon_WarPick_01.uasset
```

**`Content/GrimrockPrototype/Monsters/Categories/` — 2**

```text
DA_MONCAT_Goblin.uasset
DA_MONCAT_Vermin.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Animation/` — 10**

```text
ABP_MON_GoblinThrower.uasset
AM_GoblinThrower_Death.uasset
AM_GoblinThrower_Hurt.uasset
AM_GoblinThrower_ThrowKnife.uasset
A_GoblinThrower_Death.uasset
A_GoblinThrower_Hurt.uasset
A_GoblinThrower_Idle.uasset
A_GoblinThrower_ThrowKnife.uasset
A_GoblinThrower_Walk.uasset
A_GoblinThrower_Walk_Fwd.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Blueprints/` — 1**

```text
BP_MON_GoblinThrower.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Data/` — 1**

```text
DA_MON_GoblinThrower.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Materials/` — 3**

```text
M_Cloth_Bomber.uasset
M_Goblin_Bomber.uasset
M_Hair_Bomber.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Meshes/` — 3**

```text
PHYS_GoblinThrower.uasset
SKEL_GoblinThrower.uasset
SK_GoblinThrower.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Textures/` — 1**

```text
T_Icon_GoblinThrower.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Textures/Body/` — 3**

```text
T_Goblin_Bomber_BaseColor.uasset
T_Goblin_Bomber_Normal.uasset
T_Goblin_Bomber_Roughness.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Textures/Cloth/` — 5**

```text
T_Cloth_Bomber_BaseColor.uasset
T_Cloth_Bomber_Metallic.uasset
T_Cloth_Bomber_Normal.uasset
T_Cloth_Bomber_Opacity.uasset
T_Cloth_Bomber_Roughness.uasset
```

**`Content/GrimrockPrototype/Monsters/GoblinThrower/Textures/Hair/` — 4**

```text
T_albedo_alpha.uasset
T_alpha.uasset
T_ambient.uasset
T_normal.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Animation/` — 1**

```text
ABP_MON_RatGiant.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Animation/Montages/` — 3**

```text
AM_MON_RatGiant_Attack_01.uasset
AM_MON_RatGiant_Death.uasset
AM_MON_RatGiant_Hurt_01.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Animation/Sequences/` — 8**

```text
A_RatGiant_Attack_01.uasset
A_RatGiant_Death.uasset
A_RatGiant_Hurt_01.uasset
A_RatGiant_Idle.uasset
A_RatGiant_Idle_02.uasset
A_RatGiant_Idle_03.uasset
A_RatGiant_Idle_04.uasset
A_RatGiant_Walk.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Audio/` — 2**

```text
S_GiantRat_Attack.uasset
S_GiantRat_ImpactHit.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Blueprints/` — 1**

```text
BP_MON_RatGiant.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Data/` — 1**

```text
DA_MON_RatGiant.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Materials/` — 2**

```text
M_RatGiant_Body.uasset
M_RatGiant_Fur.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Meshes/` — 3**

```text
PA_RatGiant.uasset
SKEL_RatGiant.uasset
SK_RatGiant.uasset
```

**`Content/GrimrockPrototype/Monsters/RatGiant/Textures/` — 10**

```text
T_Icon_RatGiant.uasset
T_RatGiant_Body_AO.uasset
T_RatGiant_Body_BC.uasset
T_RatGiant_Body_E.uasset
T_RatGiant_Body_MR.uasset
T_RatGiant_Body_N.uasset
T_RatGiant_Fur_AO.uasset
T_RatGiant_Fur_BC.uasset
T_RatGiant_Fur_MR.uasset
T_RatGiant_Fur_N.uasset
```

**`Content/GrimrockPrototype/UI/` — 1**

```text
T_Border_SlotOuter.uasset
```

**`Content/GrimrockPrototype/UI/Icons/Combat/` — 12**

```text
T_Action_DefenseBlock.uasset
T_Action_DefenseParry.uasset
T_Action_MagicOneHand.uasset
T_Action_MagicTwoHanded.uasset
T_Action_MeleeDualWield.uasset
T_Action_MeleeOneHand.uasset
T_Action_MeleePolearm.uasset
T_Action_MeleeTwoHanded.uasset
T_Action_RangedBow.uasset
T_Action_RangedCrossbow.uasset
T_Action_RangedOther.uasset
T_Action_ThrownWeapon.uasset
```

**`Content/GrimrockPrototype/UI/Icons/Items/` — 5**

```text
T_Catalyst_Icon.uasset
T_DisarmTools_01_Icon.uasset
T_Flask_Oil_Icon.uasset
T_Potion_Healing_Icon.uasset
T_TrapFloor_01_Icon.uasset
```

**`Content/GrimrockPrototype/UI/Icons/Weapons/` — 24**

```text
T_Item_Arrows_01.uasset
T_Weapon_ArcaneOrb_01.uasset
T_Weapon_ArcaneWand_01.uasset
T_Weapon_AssassinDagger_01.uasset
T_Weapon_Dagger_01.uasset
T_Weapon_Dart_01.uasset
T_Weapon_ElementalStaff_01.uasset
T_Weapon_FireWand_01.uasset
T_Weapon_GreatAxe_01.uasset
T_Weapon_HolyAegis_01.uasset
T_Weapon_HolyScepter_01.uasset
T_Weapon_Javelin_01.uasset
T_Weapon_LongSword_01.uasset
T_Weapon_Mace_01.uasset
T_Weapon_MainGauche_01.uasset
T_Weapon_RecurveBow_01.uasset
T_Weapon_RoundShield_01.uasset
T_Weapon_ShortBow_01.uasset
T_Weapon_ShortSpear_01.uasset
T_Weapon_Shuriken.uasset
T_Weapon_Stiletto_01.uasset
T_Weapon_StraightSword_01.uasset
T_Weapon_ThrowingKnife_01.uasset
T_Weapon_WarPick_01.uasset
```

**`Content/GrimrockPrototype/UI/Items/` — 6**

```text
T_Icon_CopperKey.uasset
T_Icon_IronKey.uasset
T_Item_BlueGem.uasset
T_Item_ScrollEmpty.uasset
T_Item_Stone.uasset
T_Item_Torch.uasset
```

**`Content/GrimrockPrototype/UI/Map/Symbols/` — 4**

```text
T_Map_PartyMarker.uasset
T_Map_Pit.uasset
T_Map_StairsDown.uasset
T_Map_StairsUp.uasset
```

**`Content/GrimrockPrototype/UI/Map/Textures/` — 8**

```text
T_Floor.uasset
T_Map_Door.uasset
T_Map_DoorOpen.uasset
T_Map_Parchment.uasset
T_Map_PointOfInterest.uasset
T_Map_Relocation.uasset
T_Map_Wall.uasset
T_Map_WallPillar.uasset
```

**`Content/GrimrockPrototype/UI/Map/Themes/` — 1**

```text
DA_MapVisualTheme_Default.uasset
```

**`Content/GrimrockPrototype/UI/Menus/` — 6**

```text
T_ButtonMinus_Hovered.uasset
T_ButtonMinus_Normal.uasset
T_ButtonMinus_Pressed.uasset
T_ButtonPlus_Hovered.uasset
T_ButtonPlus_Normal.uasset
T_ButtonPlus_Pressed.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/` — 2**

```text
T_GenderFemaleButtonIcon.uasset
T_GenderMaleButtonIcon.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Attributes/` — 6**

```text
T_Attribute_Charisma.uasset
T_Attribute_Constitution.uasset
T_Attribute_Dexterity.uasset
T_Attribute_Intelligence.uasset
T_Attribute_Strength.uasset
T_Attribute_Wisdom.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/ClassIcons/` — 6**

```text
T_ClassIcon_Alchemist.uasset
T_ClassIcon_Mage.uasset
T_ClassIcon_Priest.uasset
T_ClassIcon_Ranger.uasset
T_ClassIcon_Rogue.uasset
T_ClassIcon_Warrior.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/` — 18**

```text
T_DwarfEnvirionment.uasset
T_Dwarf_Female_01.uasset
T_Dwarf_Male_01.uasset
T_ElfEnvironement.uasset
T_Elf_Female_01.uasset
T_Elf_Male_01.uasset
T_GnomeEnvironment.uasset
T_Gnome_Female_01.uasset
T_Gnome_Male_01.uasset
T_HalfOrc_Environment.uasset
T_HalfOrc_Female_01.uasset
T_HalfOrc_Male_01.uasset
T_HalflingEnvironement.uasset
T_Halfling_Female_01.uasset
T_Halfling_Male_01.uasset
T_HumanEnvironment.uasset
T_Human_Female_01.uasset
T_Human_Male_01.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/Dwarf/` — 12**

```text
T_Portrait_Dwarf_Female_01.uasset
T_Portrait_Dwarf_Female_02.uasset
T_Portrait_Dwarf_Female_03.uasset
T_Portrait_Dwarf_Female_04.uasset
T_Portrait_Dwarf_Female_05.uasset
T_Portrait_Dwarf_Female_06.uasset
T_Portrait_Dwarf_Male_01.uasset
T_Portrait_Dwarf_Male_02.uasset
T_Portrait_Dwarf_Male_03.uasset
T_Portrait_Dwarf_Male_04.uasset
T_Portrait_Dwarf_Male_05.uasset
T_Portrait_Dwarf_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/Elf/` — 12**

```text
T_Portrait_Elf_Female_01.uasset
T_Portrait_Elf_Female_02.uasset
T_Portrait_Elf_Female_03.uasset
T_Portrait_Elf_Female_04.uasset
T_Portrait_Elf_Female_05.uasset
T_Portrait_Elf_Female_06.uasset
T_Portrait_Elf_Male_01.uasset
T_Portrait_Elf_Male_02.uasset
T_Portrait_Elf_Male_03.uasset
T_Portrait_Elf_Male_04.uasset
T_Portrait_Elf_Male_05.uasset
T_Portrait_Elf_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/Gnome/` — 12**

```text
T_Portrait_Gnome_Female_01.uasset
T_Portrait_Gnome_Female_02.uasset
T_Portrait_Gnome_Female_03.uasset
T_Portrait_Gnome_Female_04.uasset
T_Portrait_Gnome_Female_05.uasset
T_Portrait_Gnome_Female_06.uasset
T_Portrait_Gnome_Male_01.uasset
T_Portrait_Gnome_Male_02.uasset
T_Portrait_Gnome_Male_03.uasset
T_Portrait_Gnome_Male_04.uasset
T_Portrait_Gnome_Male_05.uasset
T_Portrait_Gnome_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/HalfOrc/` — 11**

```text
T_Portrait_HalfOrc_Female_01.uasset
T_Portrait_HalfOrc_Female_02.uasset
T_Portrait_HalfOrc_Female_03.uasset
T_Portrait_HalfOrc_Female_04.uasset
T_Portrait_HalfOrc_Female_05.uasset
T_Portrait_HalfOrc_Male_01.uasset
T_Portrait_HalfOrc_Male_02.uasset
T_Portrait_HalfOrc_Male_03.uasset
T_Portrait_HalfOrc_Male_04.uasset
T_Portrait_HalfOrc_Male_05.uasset
T_Portrait_HalfOrc_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/Halfling/` — 12**

```text
T_Portrait_Halfling_Female_01.uasset
T_Portrait_Halfling_Female_02.uasset
T_Portrait_Halfling_Female_03.uasset
T_Portrait_Halfling_Female_04.uasset
T_Portrait_Halfling_Female_05.uasset
T_Portrait_Halfling_Female_06.uasset
T_Portrait_Halfling_Male_01.uasset
T_Portrait_Halfling_Male_02.uasset
T_Portrait_Halfling_Male_03.uasset
T_Portrait_Halfling_Male_04.uasset
T_Portrait_Halfling_Male_05.uasset
T_Portrait_Halfling_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Portraits/Races/Human/` — 12**

```text
T_Portrait_Human_Female_01.uasset
T_Portrait_Human_Female_02.uasset
T_Portrait_Human_Female_03.uasset
T_Portrait_Human_Female_04.uasset
T_Portrait_Human_Female_05.uasset
T_Portrait_Human_Female_06.uasset
T_Portrait_Human_Male_01.uasset
T_Portrait_Human_Male_02.uasset
T_Portrait_Human_Male_03.uasset
T_Portrait_Human_Male_04.uasset
T_Portrait_Human_Male_05.uasset
T_Portrait_Human_Male_06.uasset
```

**`Content/GrimrockPrototype/UI/Skills/` — 1**

```text
Emblème_ancien_des_cinq_voies.uasset
```

**`Content/GrimrockPrototype/UI/Skills/Icons/` — 25**

```text
T_Skill_Acrobatics.uasset
T_Skill_Alchemy.uasset
T_Skill_Arcana.uasset
T_Skill_Armor.uasset
T_Skill_Athletics.uasset
T_Skill_Crafting.uasset
T_Skill_Deception.uasset
T_Skill_HeavyWeapons.uasset
T_Skill_History.uasset
T_Skill_Intimidation.uasset
T_Skill_LightWeapons.uasset
T_Skill_Lockpicking.uasset
T_Skill_Mechanics.uasset
T_Skill_Medicine.uasset
T_Skill_Nature.uasset
T_Skill_Perception.uasset
T_Skill_Persuasion.uasset
T_Skill_RangedWeapons.uasset
T_Skill_Religion.uasset
T_Skill_Runes.uasset
T_Skill_Shield.uasset
T_Skill_Stealth.uasset
T_Skill_Survival.uasset
T_Skill_Throwing.uasset
T_Skill_Traps.uasset
```

**`Content/GrimrockPrototype/UI/Skills/Images/` — 2**

```text
PerlinNoise2d.uasset
T_Athletism.uasset
```

**`Content/Localization/Game/` — 4**

```text
Game.csv
Game.locmeta
Game.manifest
Game_Conflicts.txt
```

**`Content/Localization/Game/en/` — 3**

```text
Game.archive
Game.locres
Game.po
```

**`Content/Localization/Game/fr/` — 3**

```text
Game.archive
Game.locres
Game.po
```

**`Content/Localization/StringTables/` — 1**

```text
ST_UI.uasset
```

</details>

<details>
<summary>Scripts versionnés (tous formats) — 66 entrées</summary>


**`Scripts/` — 63**

```text
ARM_CLEAN06_4X66_Rollback.ps1
AnalyzeARMWeaponProgression.ps1
AssertARM_CLEAN06_5H18_ShippingGuards.ps1
AuditARM_CLEAN06_4S_TransactionRehearsal.ps1
AuditARM_CLEAN06_5_Retirement.ps1
AuditStarterWeaponArt.ps1
AuthorARMEMENT014.ps1
AuthorOilSurfaceMaterial.ps1
AuthorRPGAlchemist.ps1
AuthorRPGClassProgression.ps1
AuthorRPGMage.ps1
AuthorRPGPriest.ps1
AuthorRPGRanger.ps1
AuthorRPGRogue.ps1
AuthorRPGWarrior.ps1
AuthorUIRPGPresentation.ps1
AuthorUIRPGTalentTree.ps1
CaptureARM_CLEAN06_Baseline.ps1
CapturePerfProfile01.ps1
CheckCppFormat.ps1
CheckEffectsOrgMigrationScripts.ps1
CheckProjectDependencies.ps1
CleanProject.ps1
CompareARM_CLEAN06_4B_Baselines.ps1
CompareARM_CLEAN06_4F_ThreeWayProvenance.ps1
ExportARMEMENT014Catalogue.ps1
ExportARMEMENT01952Affinities.ps1
FixRPGRangerFavoredEnemyTitles.ps1
FormatCpp.ps1
GenerateStarterWeaponArt.ps1
GenerateStarterWeaponArt.py
GetARM_CLEAN06_5G89_CacheRefusal.ps1
InspectARM_CLEAN06_5H23_HistoricalSources.ps1
InstallARM_CLEAN06_5B_ReadableSaveCopy.ps1
InstallUEValidationAgent.ps1
MigrateEffectsOrgTorch.ps1
MigrateWorldObjectAssets.ps1
PlanARM_CLEAN06_4D_EditorDryRun.ps1
PlanARM_CLEAN06_5H12_Retirement.ps1
PreflightARM_CLEAN06_2.ps1
PreflightARM_CLEAN06_5H15_PaletteContinuity.ps1
PrepareARM_CLEAN06_4G_BinarySnapshot.ps1
PrepareARM_CLEAN06_4X64_SaveCopies.ps1
PrepareARM_CLEAN06_5H14_IconPaletteSnapshot.ps1
QualifyARM_CLEAN06_4C_Deltas.ps1
README.md
RepairCanonicalSkillTraps.ps1
RunEffectsOrgTorchStandalone.ps1
StageARMEMENT0163Assets.ps1
TestARM_CLEAN06_4X66_Rollback.ps1
TestARM_CLEAN06_5G85_WorldSavePreflight.ps1
TextureClean01_1_Rollback.py
TextureClean01_1_SafeMetadata.py
UEValidationAgent.ps1
UEValidationBootstrap.ps1
UEValidationSelectSDK.ps1
UEValidationSilentTask.ps1
UninstallUEValidationAgent.ps1
ValidatePackage.ps1
ValidateRPG03.ps1
ValidateUE.ps1
ValidateUIRPGVisual.ps1
VerifyARM_CLEAN06_5H16_Canary.ps1
```

**`Scripts/Localization/` — 3**

```text
ApplyEnglishSmokeTranslations.ps1
RunLocalizationSmoke.ps1
TestLocalizationScripts.ps1
```

</details>

<details>
<summary>Configuration, racine, localisation et gitlink — 24 entrées</summary>


**`./` — 13**

```text
.clang-format
.editorconfig
.git-blame-ignore-revs
.gitattributes
.gitignore
.gitmodules
.vsconfig
AGENTS.md
GrimrockPrototype.sln
GrimrockPrototype.uproject
LICENSE.txt
PROJECT_CONTEXT.md
README.md
```

**`Config/` — 4**

```text
DefaultEditor.ini
DefaultEngine.ini
DefaultGame.ini
DefaultInput.ini
```

**`Config/Armement/` — 1**

```text
WeaponProductionManifest.csv
```

**`Config/Localization/` — 4**

```text
Game_Compile.ini
Game_Export.ini
Game_Gather.ini
Game_Import.ini
```

**`Localization/Source/` — 1**

```text
UI.csv
```

**`ThirdParty/` — 1**

```text
Lua54
```

</details>

### Reproduction des comptes

```powershell
# Lecture seule ; exécuter sur le parent de CLEANUP01 pour retrouver ces comptes.
git ls-tree -r --name-only a8de4f58b6c7ecc42e62aef0dbbe8fa9b7ede272
git ls-files -z
rg --files --hidden -g "!.git/**" -g "*.ps1" -g "*.psm1" -g "*.psd1"
# Pour inclure aussi les scripts ignorés (66 fichiers dans ce relevé) :
rg --files --hidden --no-ignore -g "!.git/**" -g "*.ps1" -g "*.psm1" -g "*.psd1"
```

La comparaison du manifeste est faite sur les chemins complets, sans interpréter les guillemets d’un affichage Git non NUL. Les lignes C++ utilisent `splitlines()` UTF-8, commentaires et blancs inclus ; elles ne mesurent pas la complexité. Les assets sont comptés comme packages, pas comme exports Unreal. Les quatre scripts ignorés ne sont pas comptés dans les 4 299 entrées Git.
