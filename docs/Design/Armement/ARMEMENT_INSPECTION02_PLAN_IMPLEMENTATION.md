# ARMEMENT-INSPECTION02 — Plan d’implémentation détaillé

**Version 1.0 — plan approuvé pour lancement, 26.11.2026**
GrimrockPrototype · UE 5.5.4 · Visual Studio 2026 · master uniquement
Base documentaire et code : `c6915461f3ec56b476ccf3803796411f2c0086be`.

## 1. Contrat et objectif

Références approuvées dans `docs/Design/Armement/` : DESIGN01 v1.1, INSPECTION01 v1.1 et INSPECTION01A v1.0. L'addendum prévaut. L'atelier d'objets (nom proposé : Item Workshop) est générique dès le départ ; la première recette artistique porte sur ARM014, puis les 21 autres armements existants. Les 60 archétypes non matérialisés ne deviennent pas des objets fictifs. Le catalogue est énuméré depuis les définitions concrètes du registre. ARM exige N1–20 ; autres objets N0 en interne, sans contrôle de niveau. Aucun changement de mécanique, de propriété d'instance, de sauvegarde v27, de rareté ou de progression d'XP. Le niveau physique reste fixe. Aucune recréation automatique des cinq Niagara prototypes rejetés. Les scènes techniques auteur ne constituent pas le décor du mode Examiner.

## 2. Existant vérifié, décisions et limites

- `UGridItemSubsystem::GetItemIds`, `ResolveItem`, `ResolvePhysicalItem`, `ResolveArmorReserveForInstance`, `ResolveBoundAction`, `ResolveBoundAttackForHand` sont déjà disponibles. Ne pas créer de registre parallèle. `FGridResolvedItem` porte notamment WorldMesh, WorldVisualTransform, EquippedMesh, ProjectileMesh, ProjectileAppearance, WeaponFinish, VisualEffects, LightEmitter, ItemIcon, bReadable, ReadText, NonArmEquipmentSlots/Bonuses/Resistances. Les capacités de lecture et d'affichage sont dérivées de ces données et des règles existantes.
- `GridEditorArmamentPreview.h` montre la construction d'un registre transitoire à partir de la racine configurée dans l'éditeur. Généraliser son usage, pas les règles de résolution. Ne pas multiplier les sous-systèmes au changement de niveau. Reconstruire la session après édition native pertinente ; en cas de données incohérentes, signaler l'erreur sans conserver une image fausse.
- `AGridItemActor::SetPresentationMesh/ApplyWeaponFinish/ClearWeaponFinish` possède déjà la logique MID (RoughnessMultiplier, RoughnessOverride, NormalStrength, BaseColorTint) et préserve les overrides explicites. `UGridItemVisualEffectsComponent` porte les effets persistants. Factoriser uniquement la partie réutilisable et démontrer la parité avec le runtime, pas un deuxième acteur jouable.
- `FGridAttackPresentationReadModel::TryResolve` exige une source de présentation avec GUID valide. L'atelier ne fabriquera pas de fausse source équipée. Extraire la conversion de présentation d'une action déjà résolue et les sélecteurs purs de médias seulement lorsque nécessaire ; conserver les contrôles de possession du runtime. Les protections utilisent les véritables événements de défense, jamais une attaque fictive.
- `UGridInventoryWidget::ExecuteResolvedInventoryContextAction` appelle `PresentItemExamination(LastContextItem, TooltipText)`. Préserver le point d'entrée BlueprintImplementableEvent tant que les WBP réels ne sont pas vérifiés ; ne pas parser le tooltip. `PresentItemReading` et `WBP_ItemReadPanel` restent distincts. Les deux Readables sont protégés.
- Le module `GrimrockPrototypeEditor` inclut déjà Slate, UnrealEd, PropertyEditor, ToolMenus et Niagara ; AdvancedPreviewScene est une dépendance candidate à ajouter uniquement au module éditeur. Le module runtime a déjà UMG, Niagara, Engine et Slate. Ne pas importer de module éditeur en Shipping.
- Le rendu runtime isolé est une hypothèse à démontrer, pas un résultat acquis. Epic UE5.5 : [USceneCaptureComponent2D](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Components/USceneCaptureComponent2D?application_version=5.5), [USceneCaptureComponent / ShowOnlyComponents](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Components/USceneCaptureComponent?application_version=5.5), [FAdvancedPreviewScene](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/AdvancedPreviewScene/FAdvancedPreviewScene/__ctor?application_version=5.5). La liste show-only isole les primitives capturées, pas à elle seule les lumières, l'environnement ou l'audio.

## 3. Architecture cible et responsabilités

Les noms de classes et dossiers nouveaux ci-dessous sont des candidats ; rechercher l'équivalent existant avant création et limiter le nombre de types. Inventaire cible ; l'état réellement livré du premier lot est documenté dans [INSPECTION02.1](ARMEMENT_INSPECTION02_1_SOCLE_COMMUN.md).

```text
Source/GrimrockPrototype/
  Public/Runtime/Inspection/
    GridItemInspectionTypes.h              # projection transitoire, capacités, état
    GridItemInspectionReadModel.h          # résolution et sections applicables
    GridItemPresentationComponent.h        # mesh/MID/effets visuels partagés, si extraction justifiée
    GridItemPreviewActor.h                 # représentation isolée non jouable
    GridItemPreviewPlaybackComponent.h     # séquence média locale (02.4)
    GridItemInspectionCaptureHost.h        # hôte runtime (02.5, après spike)
  Private/Runtime/Inspection/              # implémentations correspondantes
  Public/UI/
    GridItemExaminationWidget.h            # parent UMG
    GridItemExaminationStatRowWidget.h     # seulement si ligne existante non réutilisable
  Private/UI/                              # implémentations UI
  Private/Tests/                           # GridItemInspection*Tests.cpp
Source/GrimrockPrototypeEditor/
  Private/EditorTools/Inspection/
    GridItemWorkshop.*                     # enregistrement onglet/menu, état session
    SGridItemWorkshop.*                    # layout Slate et commandes
    SGridItemWorkshopViewport.*            # SEditorViewport
    GridItemWorkshopViewportClient.*       # FEditorViewportClient
    GridItemWorkshopSession.*              # registre transitoire, A/B, essais
  Private/Tests/                           # GridEditorItemWorkshop*Tests.cpp
```

Une projection de lecture est obtenue depuis le registre par ItemId + niveau exact ; en jeu, depuis l'instance physique courante revalidée. Elle comporte l'état de résolution, la référence canonique, les champs pertinents, les capacités de preview et l'état réel (quantité, réserve lorsqu'applicable) séparé de la projection de définition. Aucun snapshot statique sauvegardé. Les contrôles sont dérivés : niveau, lecture, équipements, actions, réactions, projectile, présence de mesh, etc. Pas de seconde taxonomie persistante.

Le présentateur commun applique le mesh approprié et sa transformation de contexte, restaure les slots, utilise des MID propres et les effets persistants réellement résolus. Inspection : orientation d'observation indépendante du pivot métier ; caméra ajustée aux bounds (ornements pertinents inclus). Au sol : pose réelle. En main/vol : représentation prévue par la politique visuelle, sans inventer un mesh tenu en première personne si le jeu ne l'affiche pas. Toute ressource visuelle absente du read model (ex. ornement non encore projeté) est relevée comme limite ; étendre la projection seulement si l'usage approuvé le nécessite. Ne pas confondre un champ de catalogue avec une fonction effectivement raccordée.

Le preview est non possédable, non sélectionnable par interactions monde, transitoire et isolé. Pas de GridItemActor inscrit dans le registre du donjon, pas d'identité métier active, pas de collision ou transfert dans le jeu, pas de loot/PA/IA/munitions/save. Les essais appartiennent à la session. Une nouvelle sélection annule les opérations en cours ; vérifier un jeton de génération pour les callbacks asynchrones. Ordre de fermeture : arrêter séquences et callbacks, détacher références actives, libérer audio/Niagara avec validation de pointeurs après désactivation (HOTFIX01), libérer MID/mesh/capture, détruire hôte/scène puis session. Cleanup idempotent sur Stop, changement d'objet, niveau, map, fermeture et shutdown.

## 4. Découpage exécutable

Conserver les identifiants 02.1 à 02.6 de la spécification. Scinder seulement les lots qui ont des portes natives différentes. Un commit atomique par lot cohérent, code + tests + documentation ; aucune branche secondaire. Après publication, attendre SilentTask et LocalAuto SUCCESS sur le SHA exact avant le lot technique suivant. Une validation GPU et une validation artistique sont distinctes de NullRHI. Le propriétaire n'est sollicité que pour une décision artistique ou un test natif réellement inaccessible.

### 02.1 — Socle commun générique, sans UI native

**Entrées :** registre et schéma actuels. Préserver les 48 définitions concrètes de la base approuvée (22 ARM) et leur identité ; vérifier le nombre réel au SHA du lot. 440 résolutions ARM ; toutes les non ARM à N0. Exclure les archétypes numériques non matérialisés.

**Travaux :** projection d'inspection et capacités à partir de la résolution exacte ; distinction instance réelle / niveau simulé. Réutiliser `ResolveArmorReserveForInstance` uniquement pour une vraie instance ; montrer maximum simulé clairement étiqueté dans l'atelier. Énumération et choix explicite des erreurs. Extraire une application de finition commune à `AGridItemActor` et au preview, en conservant strictement le comportement actuel. Créer la représentation transitoire minimale, avec mesh, matériaux, effets persistants et libération sûre ; ne pas modifier les cinq bandes à ce stade. Politique fond noir prévue dès le contrat d'hôte, mise en œuvre native dans 02.2/02.5. Aucune modification de package binaire attendue.

**Tests candidats :** `Grimrock.Items.Inspection.Resolve`, `.Capabilities`, `.PresentationParity`, `.Isolation`, `.Lifecycle`. Cas ARM N1/N20, N0/N21 refusés, ID absent, N0 non ARM/N1 refusé, objet sans mesh, passifs, équipement N0, livre/clé/pierre, MID indépendants, réinitialisation N20→N1, cleanup répété, callbacks invalidés. Assertions de non-mutation (registre, instance, propriétaire, monde). **Sortie :** parité sur données et comportement de finition actuel ; pas encore de promesse de vingt apparences distinctes.

### 02.2 — Atelier éditeur V1 (hors PIE)

**Travaux :** onglet dockable `GridItemWorkshop` via ToolMenus / tab manager du module existant ; désenregistrement au shutdown. Libellé proposé Outils → Grimrock → Atelier d'objets. Liste de tous les ItemId réels avec recherche et filtre ARM, catégorie et états d'erreur. Initialiser registre éditeur transitoire depuis la racine configurée. Sélection ItemId et niveau, N1–20 entier pour ARM ; non ARM N0 sans sélecteur. `SEditorViewport` + `FEditorViewportClient` et `FAdvancedPreviewScene` (ou équivalent natif validé localement). Fond noir, sky/floor/grid visibles désactivés par défaut, lumière/reflets indépendants, exposition constante, orbite/zoom/pan, recentrage, face/profil/dos. Mesures d'échelle en cm ; pas de grille décorative dans Examen. Panneaux Slate : identité, données calculées, mesh/slots/transform, matériau/paramètres existants, provenance et liens d'ouverture des assets natifs. Notification de modification native, reconstruction du read model, absence d'écriture implicite.

**Tests :** `Grimrock.Editor.Items.Inspection.Catalogue`, `.Selection`, `.Session`, `.Refresh`, `.Close`. Vérifier 22 ARM et tous les autres objets réels, navigation N1–20 et N0, absence de package dirty par simple consultation, sélection d'un mesh manquant avec repli. Recette native hors PIE avec ARM014, ARM075, clé, pierre et Readable si matérialisés ; absence de fuite à la fermeture/réouverture. Le rendu actuel peut rester identique dans une bande : indiquer explicitement « progression fine non authorée », aucune validation artistique de l'arme à ce stade.

**Porte utilisateur :** vous disposez d'un premier outil exploitable pour observer l'existant, sans lancer PIE. Ce n'est pas encore le laboratoire complet.

### 02.3A — Comparateur et contextes de présentation

A/B sur deux instances et MID indépendants ; synchroniser cadrage, angle et exposition si voulu. Sélecteur niveau B et voisin ±1 ; N1/N20 ; planche de captures et relevé des différences. Aucun changement d'éclairage, de seed ou de framing pour simuler une amélioration. Presets Studio, Torche, Donjon ; contextes Examen sur noir, Sol, Décor, Main et Vol. Sol/Décor : supports techniques isolés, non interactifs ; grille de référence 200 cm optionnelle uniquement en contexte technique. Main/Vol : projections existantes, politique stable/tumble/spin, mode inapplicable masqué. Ne pas créer de scènes de donjon dans un niveau réel. Aucune ambiance 2.5D imposée en V1.

Tests `Grimrock.Editor.Items.Inspection.Compare`, `.Context`, `.Camera`, `.IndependentMaterials`. Vérification des différences mesurables, stabilité des identités, pas de fuite d'une vue à l'autre. Captures GPU sous mêmes conditions.

### 02.3B — Progression fine : prototype technique ARM014

**Décision recommandée, à valider sur les vrais matériaux ARM014 :** cinq ancres visuelles et progression locale authorée à l'intérieur des bandes, avec réglages par zone lorsque le mesh le permet. Un même mesh de base ; ajouter masques, gravures ou ornements seulement selon besoins réels. Niveau entier déterministe ; données authorées dans `DA_WeaponVisualCatalogue` et matériaux/MI natifs, jamais dans les fiches concrètes ou un cinquième catalogue. Le resolver produit une finition effective complète ; le présentateur commun applique cette finition aux contextes pertinents. Si les champs actuels suffisent au pilote, ne pas introduire de structure superflue. Sinon extension ciblée du profil de finition, avec valeurs par défaut qui conservent le rendu des autres armes jusqu'à authoring. Aucune évolution SaveGame ni profils numériques.

Option de conception pour un scalaire continu x : bande b=floor((N-1)/4), position locale u=((N-1) mod 4)/3, x=lerp(début[b],fin[b],courbe[b](u)). Pour les seuils discrets (gravures, ornements) : niveau d'apparition explicite. Ne pas imposer une interpolation à travers un changement de matière ou de mesh. Pas de bruit aléatoire, pas de modification globale du matériau. Les couleurs/multiplicateurs exacts et la courbe ne sont pas choisis à ce stade. Exposer la valeur effective dans l'atelier et la provenance, pas un calcul UI parallèle. Une rampe numérique n'est pas une preuve de perceptibilité.

**Travaux ordonnés :** relever slots, paramètres réellement supportés, partages et état du mesh d'ARM014 ; choisir avec le propriétaire un premier canal lisible ; créer une démonstration minimale sur ressources ciblées ; observer N1→2 et N4→5, puis étendre aux 20 niveaux ; comparer N1/N20 et les 19 transitions, vérifier au sol/donjon. Une implémentation peut être fonctionnelle sans arme artistiquement terminée. Les 21 autres restent marquées en attente de production ; ne pas générer des variations génériques non approuvées. Les contrôles de clôture artistique complets ont lieu avec la fiche ARM014 et sont reproduits pour chaque arme.

Tests `Grimrock.Items.Inspection.LevelFinish`, `.BandBoundaries`, `.Determinism`, `.RuntimeParity`, `.StatInvariance`. Valeurs finies, limites, N1/N20, restauration, isolation A/B, absence de changement dans les 1 640 profils mécaniques et les identités. Si extension de données, valider les assets existants, les valeurs par défaut, les références et le cook. Preuves de différences perceptibles exclusivement en recette GPU + propriétaire.

### 02.4A — Simulateur audiovisuel, lecture et cycle de vie

Exposer les bindings réellement résolus et les événements valides, y compris défense passive sans attaque. Faire une projection commune des variantes et de la sélection de résultat sans contourner les validations d'exécution. Le simulateur manipule une séquence factice, non un TurnManager de combat réel. Résultat scénarisé hit/miss/crit/parade/absorption, matériau de contact, cible neutre, point/normal/distance. Le cas d'absorption magique de l'ARM070 reste une fixture de démonstration si réserve réelle nulle ; jamais de bonus ajouté. Rejouer attaque, contact, mouvement, projectile (release/flight/collision/stop) selon capacité. Pas de fausse promesse de VFX spécialisé par matière ou de son de vol lorsqu'absents : marquer « non raccordé » / « non configuré » et permettre l'aperçu isolé du média réellement disponible.

Sélection de variante déterministe, lecture séparée puis séquence synchronisée, stop/replay, volume/pitch, offsets/rotation/scale/socket, durée et timeline. Réutiliser les paramètres MotionDurationSeconds, ImpactDelaySeconds, RecoveryDelaySeconds, PeakLocationOffset, PeakRotationOffset, MinimumStrikeStartIntervalSeconds du chemin réel ; vérifier le scheduler plutôt que supposer une formule d'impact commune aux frappes simples et doubles. Ralenti selon capacités réelles de la simulation, pas de promesse de time dilation audio fidèle. Séquence double frappe légitime, sans action de jeu. Arrêt et invalidation sur changement de sélection/niveau ; aucun son parasite.

Tests `Grimrock.Items.Inspection.Playback.EventRouting`, `.Variant`, `.Sequence`, `.Defense`, `.Projectile`, `.Cleanup`. Réutiliser le test HOTFIX01 pour vérifier les destructions Niagara auto, références périmées, callbacks, stop répété. Pas de génération payante ni remplacement des médias rejetés. Recette D3D12 + écoute propriétaire sur l'arme pilote, une défense, un projectile, un focaliseur et dual-wield valide.

### 02.4B — Essais locaux et inspecteur natif

Panneau de valeurs actuelles/effectives/proposées avec unités et origine. Essayer un paramètre par session via MID, copies transitoires de paramètres de présentation, composants de preview ; reset par champ et reset global. Pas de Set/Edit sur un asset réel pendant un essai ; signaler les paramètres que seul l'éditeur natif peut modifier. Boutons ouvrir asset, retrouver dans Content Browser, inspecter les partages connus et avertir avant authoring. Les notifications Undo/Redo et changements d'assets reconstruisent la projection ; pas de duplication de Niagara Editor ou Material Editor. Export Markdown/CSV simple des valeurs et observations vers Saved/DevTemp ou chemin choisi, aucune écriture automatique dans docs. Les modifications natives réellement effectuées sont des modifications utilisateur à préserver.

Tests de non-mutation, reset, références partagées, changement d'objet et refresh. Recette utilisateur : changer un réglage, comparer, écouter, revenir à l'état initial, ouvrir la source. 02.6 reste facultatif : ne pas implémenter un bouton Apply non autorisé.

### 02.5A — Spike natif de capture runtime

Démontrer avant création WBP : mesh réel éclairé sur fond noir, capture `USceneCaptureComponent2D` et RT transitoire ; composition UI sans alpha défectueux ni exposition variable. Approche préférée : scène de présentation isolée et cycle de vie maîtrisé, si elle est viable en Shipping. Alternative dans monde courant uniquement si isolement des primitives, de l'éclairage, des ombres, des réflexions, du son et des interactions démontré ; show-only seul est insuffisant. Ne pas dépendre de FAdvancedPreviewScene en Shipping. Pas de carte de production modifiée. Prototypes et ressources jetables sous Saved/DevTemp/ ou isolées, retirer après preuve ; ne conserver que le chemin choisi et les assets utiles. Captures à ouverture/changement, rafraîchissement lors de mouvement/effet actif uniquement. RT initial 1024² (candidat 512–2048 selon mesure), pas de Tick de capture quand fermé. Vérifier le rendu du vrai métal, les matériaux et les effets persistants, le coût GPU en jeu, les changements de map, le teardown et l'absence de lumière/objet parasite. Choix d'hôte documenté après essai PIE et Shipping, pas avant.

### 02.5B — Examiner joueur, C++ / localisation / WBP

Raccordement de l'action Examine, revalidation de la source (GUID, propriétaire et slot par les règles existantes) et capture du snapshot lecture seule. Conserver le contrat Blueprint existant tant que le raccord natif n'est pas inspecté. Aucun déplacement d'objet, débit, modification niveau/équipement/réserve. Si le contexte devient invalide, fermer ou indiquer indisponibilité selon comportement retenu ; jamais examiner silencieusement un autre objet. Préserver lecture via PresentItemReading et tooltip passif. Mode modal pour l'input : prioriser UI sans faire progresser une action par clic/roulette dans le viewport ; Escape ferme d'abord Examiner ; restaurer focus, curseur, input mode et inventaire. Respecter les règles existantes de combat et de pause ; pas d'introduction de pause de gameplay sans décision. Fermeture sur changement de monde et destruction.

Assets candidats, à vérifier avant création : `/Game/GrimrockPrototype/Blueprints/UI/Inspection/WBP_ItemExamination`, `WBP_ItemExaminationStatRow` si aucune ligne compatible ; matériau UI de rendu `M_UI_ItemInspection` (dossier UI approprié à confirmer), RT transitoire plutôt que package par objet. Pas de WBP auteur dans Shipping. Hiérarchie Designer : Overlay racine + fond noir/optional ambiance 2.5D (non requis), cadre, titre/fermeture, HorizontalBox viewport et panneau détails scrollable ; Image_Preview, bouton recentrage/face/profil/dos, Text_Name, Section_Level/Text_Level, Text_Description, Text_Weight, sections statistiques, protection, actions/réactions/conditions, lecture si pertinente, état indisponible, footer. Noms exacts et types BindWidget à figer en début de lot, avant native authoring. Les styles, ancrages, tailles et polices sont dans le Designer. C++ affecte les textes dynamiques, gère visibilité des sections et instancie les lignes WBP complètes ; pas de WidgetTree de TextBlocks ou de padding/police natifs. Rendu fidèle et zoom sur détails, pas de décor proche. Aucune commande de niveau, debug, action simulée ou écriture en mode joueur. N0 masqué intégralement.

Séquence obligatoire : inventaire exact des libellés et clés ; réutiliser `Localization/Source/UI.csv` ; ajouter seulement les clés nécessaires ; compiler le parent natif, exécuter `UIStringTableAuthoring` (code 0), contrôler ST_UI dans l'éditeur AVANT WBP, authoring natif, branchement puis Game/fr/en et cook. Relire `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` avant/après. Aucun fake .uasset via GitHub. Si édition native non accessible, arrêter à la porte native et fournir une procédure précise plutôt que publier un WBP supposé.

Tests `Grimrock.UI.Items.Inspection.Route`, `.Capabilities`, `.Input`, `.ReadOnly`, `.Fallback`, `.Lifecycle`. Cas sac/équipement, ARM offensif/passif, clé/pierre/livre/parchemin, équipement non ARM disponible, objet sans mesh. Tests focus, escape, action Lire, menu fermé, transition map, invalidation et réouverture. DPI et fr/en, 720p/1080p/1440p/4K/ultrawide, GPU PIE et Shipping. Vérifier l'absence d'accès à l'atelier en Shipping. Préserver sept sauvegardes v27 et deux Readables ; tests sur copies isolées avec hashes des originaux. L'examen des objets dans le décor n'est raccordé que via une route existante validée, sans interaction nouvelle anticipée.

### 02.6 — Application transactionnelle des essais (option, non autorisée par ce plan)

Si le besoin est confirmé après usage : diff ancien/nouveau, liste des profils/armes/ressources touchés, refus des champs métier hors périmètre, confirmation explicite, Modify/transaction/Undo/Redo native, validation des références, pas d'autosave ni de commit implicite. La voie initiale d'authoring est l'ouverture de l'éditeur natif. Aucun besoin de 02.6 pour commencer ARM014. Tests changement partagé, annulation, Undo/Redo, sauvegarde explicite et parité. Garder cette option fermée jusqu'à décision.

## 5. Recette artistique ARM014 et généralisation

Utiliser la fiche maîtresse approuvée, relever l'état réel de chaque asset avant modification. Valider d'abord géométrie et identité, puis matériaux N1, puis vingt finitions avec 19 différences perçues, puis pose/décor/main/vol lorsque pertinent, SFX, VFX, mouvement, gameplay et examen. Pour chaque modification noter valeur initiale, cible, unité, provenance, impact sur ressources partagées, preuve et décision. Comparer sous mêmes caméra/éclairage, sans indication de niveau si nécessaire pour vérifier que la différence est intrinsèque. Les seuils N4→5, N8→9, N12→13 et N16→17 méritent un accent plus marqué. Valider aussi en donjon réel ; aucun rendu studio n'autorise à conclure à la lisibilité dans toutes les conditions. Une arme rejetée reste ouverte. Pour 22 ARM : 440 rendus et 418 transitions à approuver progressivement ; pas un prérequis à la livraison du viewport ou du simulateur. Les futurs objets N0 reçoivent leur propre recette adaptée (pas de progression fictive). Ne pas fabriquer les 60 ARM manquants dans ce jalon.

## 6. Portes techniques communes et autonomie

Avant lot : git status, branche master, HEAD/origin/master, préserver travail utilisateur, relire AGENTS.md et contrats concernés. Publication atomique avec parent vérifié, diff et git diff --check, pas de force push ni de nettoyage destructif. UE 5.5.4 précompilé protégé D:\UE_5.5, v143 MSVC 14.44.35207, clang-format 19.1.5 ; pas de Clean/Rebuild Solution moteur. Compiler et tests ciblés puis campagne Grimrock via Scripts/ValidateUE.ps1, Shipping/ValidatePackage.ps1 lorsque requis ; suivre le dispositif Windows existant. Aucune écriture native distante de .uasset/.umap. Les tests sans rendu ne prouvent ni le GPU ni l'esthétique. Les preuves comprennent SHA, statuts UE5.5.4/SilentTask et UE5.5.4/LocalAuto, compteurs réels, captures/écoute et points ouverts. Pas de nouveaux commandlets/audits génériques. Les fichiers temporaires sont isolés et nettoyés après vérification, le worktree UEValidation permanent et les données uniques protégés.

État constaté à la vérification finale : `c6915461f3ec56b476ccf3803796411f2c0086be` sur master, SilentTask success, LocalAuto success. La porte de validation du prérequis est satisfaite ; vérifier de nouveau HEAD avant le premier lot technique. Le présent document est un plan, pas du code compilé ; les classes candidates, le modèle de progression fine et l'hôte de capture restent à confirmer. La validation artistique est réservée au propriétaire. L'agent Windows valide le code publié ; il ne modifie pas la copie de développement et ne réalise pas une recette artistique.

## 7. Ordre de réalisation et jalons de disponibilité

02.1 → 02.2 (premier atelier utilisable) → 02.3A (comparaison) → 02.3B (prototype progression fine ARM014) → 02.4A/B (laboratoire) → 02.5A/B (examen joueur) → recette ARM014 → autres ARM une à une. L'auteur peut commencer à observer et annoter ARM014 dès 02.2, sans attendre le WBP ou la production audiovisuelle. 02.6 facultatif, seulement si approuvé. Les prochaines décisions artistiques concernent les matériaux et valeurs du pilote, pas les mécaniques déjà validées.


### Correctif 02.3B.3A — fond noir de l'atelier

Signalement utilisateur : retour du panorama standard de l'éditeur de mesh. Le fond noir reste obligatoire ; les supports Sol/Décor sont seuls autorisés dans leurs contextes explicites. La visibilité des composants stock de FAdvancedPreviewScene est réaffirmée localement avant chaque rendu, sans modification du profil partagé ni suppression de l'éclairage cubemap. Test `Grimrock.Editor.Items.Inspection.BlackBackdrop` : réactivation simulée des composants, puis masquage. La recette GPU reste nécessaire. Le blocage de compilation de l'issue #243 (C2672, assertion TestNotNull du test d'ornement) est corrigé par une comparaison de pointeur explicite, sans affaiblir l'assertion.


### Correctif de recette BlackBackdrop (28.11.2026)

Le journal local du propriétaire confirme 2039 succès, 48 succès avec avertissements, un échec et zéro non exécuté. L'assertion d'identification de deux composants visibles par parcours global des UObject est invalide sous Automation. La scène capture maintenant ses propres composants statiques stock lors de sa construction (avant tout support de décor ou objet). Leur visibilité est forcée localement à false avant le rendu, y compris si un profil la rétablit. Le test remet explicitement à true le flag de chaque composant capturé, puis vérifie le retour à false, le maintien d'un composant de décor ajouté ultérieurement et les intensités lumineuses inchangées. Aucune dépendance au rendu GPU ni au nombre de composants visibles selon le profil ; au moins un composant stock est obligatoire. Le test ne constitue pas une preuve GPU du panorama. Aucun test supprimé ni filtré. Nouvelle validation Windows et recette visuelle requises.


### Correctif de compilation C2248 — issue #245

UE 5.5.4 rend le tableau interne Components privé. Le socle noir utilise désormais l'accesseur public GetFloorMeshComponent comme ancre et un instantané des composants avant/après la construction synchrone de la scène pour identifier les nouveaux composants stock (monde/outer correspondant), sans accéder au tableau privé ni masquer les objets préexistants. Les deux API officielles SetFloorVisibility et SetEnvironmentVisibility restent appelées avant le rendu. Le test vérifie explicitement l'ancre sol ; la couverture visuelle du panorama reste à confirmer sous RHI. Aucune modification d'asset.


### 02.5A — socle natif de capture runtime, 28.11.2026

Priorité avancée à la demande du propriétaire : Examiner dans l'inventaire ; progression N1–N20/ornements/PBR suspendue. Socle 02.5A.1–4 regroupé atomiquement : `FGridItemRuntimePreview` utilise `FPreviewScene` (module Engine, pas AdvancedPreviewScene) en GamePreview, un acteur commun de présentation et une capture 2D à la demande avec RT transitoire 1024² et exposition manuelle. Aucun mesh/lumière/capture ajouté au monde joueur ; pas de capture périodique. Le modèle de caméra validé dans l'atelier est déplacé vers Runtime/Inspection/GridItemInspectionOrbit.h et réutilisé, sans modification de son calcul. La session ne possède aucune donnée de gameplay ni WBP. Le test Grimrock.UI.Items.Inspection.RuntimePreview couvre isolation, instance N5, navigation, repli, fermeture/réouverture, indépendance des sessions. Il ne lance pas de capture GPU. La première illumination (deux directions, ciel de FPreviewScene) et la composition LDR/UMG restent des hypothèses de spike à vérifier sur RHI réel, particulièrement les métaux et l'absence de fond parasite. Les effets animés et nettoyage automatique lors des changements de map seront raccordés à la durée de vie du widget ; pas de tick global. Les sources sont publiées pour validation Windows exacte avant le lot 02.5B. Aucun asset modifié ni nouveau texte visible.
