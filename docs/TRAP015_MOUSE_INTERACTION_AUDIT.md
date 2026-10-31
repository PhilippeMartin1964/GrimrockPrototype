# TRAP01.5 — Audit de l'interaction souris (UE 5.5.4)

## Cause démontrée

Le monde transitoire de `RuntimeDetectionAndPersistence` était créé avec
`UWorld::CreateWorld`, sans `InitializeActorsForPlay`. Cela initialise le monde,
mais pas ses acteurs pour le jeu. Dans UE 5.5.4, `AActor::ProcessEvent`
(`Engine/Private/Actor.cpp`, ligne 1155) ne transmet l'événement à UObject que si
`AreActorsInitialized()` est vrai (hors autorisation d'exécution éditeur/CDO).
Cette condition concerne aussi les fonctions natives.

Une instrumentation temporaire sur le HEAD de départ `3bb648af` a montré, sur le
même acteur et avant toute modification du gameplay :

- classe `/Script/GrimrockPrototype.GridGenericObjectActor` ;
- `CanInteract` et `GetInteractionCursor` résolues sur cette classe,
  `FUNC_Native`, zéro octet de script Blueprint ;
- monde non initialisé : appel direct vrai, `Execute_CanInteract` faux,
  `Execute_GetInteractionCursor` égal à `None` ;
- après le seul appel `InitializeActorsForPlay(FURL())` : les assertions publiques
  passent, ainsi que les deux tests TRAP01.2.

Preuve locale : `Saved/Automation/TRAP015Audit/TD04-20261031-112054/Automation.log`.
Le diagnostic temporaire a ensuite été remplacé par une initialisation correcte
du monde dès la préparation du test, une assertion sur cette précondition et des
contrôles permanents de résolution d'interface. Aucune assertion existante retirée.

## Chemin vérifié

1. `RefreshDungeonTrapRevealVisual` conserve le GUID et le propriétaire runtime,
   rend le mesh visible et active une collision QueryOnly bloquant Visibility.
2. `TryGetInteractableUnderCursor` déprojette la souris et teste le premier
   obstacle Visibility. Le contrôleur vérifie distance, pawn et composant,
   puis appelle `Execute_CanInteract` ; le survol utilise
   `Execute_GetInteractionCursor`, le clic `Execute_InteractWithHit`.
3. Le code UHT de l'interface appelle `FindFunction`, puis `ProcessEvent` si une
   fonction existe. Son repli C++ n'est utilisé qu'en l'absence de fonction,
   pas lorsque `ProcessEvent` refuse l'exécution. Les valeurs de retour restent
   alors à leur valeur initiale : faux et zéro (`None`).
4. `UClass::FindFunctionByName` recherche aussi dans les interfaces implémentées
   (`CoreUObject/Private/UObject/Class.cpp`). Une redéclaration dans l'acteur
   n'est donc pas nécessaire. `UFunction::Invoke` ajuste l'adresse vers
   l'interface ; les thunks UHT appellent virtuellement `_Implementation`.
5. L'implémentation native générique consulte l'état du piège dans son runtime.
   `InteractWithHit_Implementation` hérité de l'interface délègue virtuellement
   à `Interact_Implementation`, qui appelle `TryInteractWithRevealedDungeonTrap`.

Le test impose la classe native exacte et vérifie les UFunction résolues : aucun
override Blueprint n'intervient dans l'échec reproduit. L'outil d'authoring
`GridLevelEditorActor_CoreDungeon_TRAP015.inl` configure également la classe native.
Cela ne vaut pas inspection de tous les Blueprints personnalisés du projet.

## Nettoyage et limites

- Suppression des cinq redéclarations BlueprintNativeEvent et de la délégation
  InteractWithHit ajoutées par `3bb648a` : elles dupliquaient l'interface et ne
  pouvaient pas ouvrir la barrière d'exécution du monde.
- Conservation des contrôles d'identité de `d58b17e`.
- Conservation de `af1cec2` : l'état runtime fait autorité et ses contrôles
  propriétaire/état caché restent utiles ; ce changement ne corrigeait toutefois
  pas le cycle de vie du monde de test.
- Aucun changement des règles de piège, du contrôleur, de l'interface, des fichiers
  générés manuellement ou des assets utilisateur.
- Aucun texte joueur ni WBP modifié : pipeline `UI.csv → ST_UI → WBP → Game/fr/en`
  hors périmètre, aucune matérialisation nouvelle nécessaire.

Une validation PIE reste nécessaire pour le rayon souris réel, l'occultation,
la distance, le curseur affiché et le clic sur les pièges sol/mur/plafond des
assets locaux. Les tests sans RHI ne prouvent pas le rendu ni la déprojection.

## Validation finale

- Compilation réelle `GrimrockPrototypeEditor Win64 Development`, moteur
  `D:\UE_5.5` version 5.5.4 : code 0. UHT a régénéré ses sorties après nettoyage
  de l'en-tête. Toolchain v143 sous
  `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207`
  (version compilateur affichée par UBT : 14.44.35229). Seul avertissement de
  sélection : version non préférée par UBT 5.5 ; toolchain demandée conservée.
- TRAP01.2 final : **2 succès**, zéro échec, zéro avertissement d'automatisation.
  Rapport : `Saved/Automation/TRAP015Final/TD04-20261031-112337/index.json`.
  Le clic via `Execute_InteractWithHit` produit le feedback runtime de portée et
  conserve l'état Detected. La résolution des trois événements est native,
  portée par `UGridInteractableInterface`, sans bytecode Blueprint.
- Régressions TRAP01.1, TRAP01.3, TRAP01.4 (dont 4.1),
  ARMEMENT01.16.TrapInteraction et Runtime.Objects.GenericAudioContract :
  **9 succès**, zéro échec, zéro avertissement d'automatisation, code 0.
  Rapport : `Saved/Automation/TRAP015Regression/TD04-20261031-112400/index.json`.
- `git diff --check` : aucun défaut. Journaux et rapports restent sous `Saved/`,
  non versionnés. Les modifications d'assets préexistantes restent locales.
