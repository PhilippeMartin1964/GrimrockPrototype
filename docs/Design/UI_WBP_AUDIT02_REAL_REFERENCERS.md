# UI-WBP-AUDIT02 — Inventaire des references reelles (read-only)

Date : 2026-10-19
Statut : **implementation publiee ; validation locale UE5.5.4 attendue**.
Base de l'inventaire : 46 `WBP_*.uasset` dans `Content/GrimrockPrototype/Blueprints/UI/` et sous-dossiers, a la revision `80305c78`.

## Objet et limite de responsabilite

Cet audit **ne supprime ni ne modifie aucun asset**. Son objectif est de
generer un inventaire reproductible des 46 WBP connus, des referencers reels
suivant l'AssetRegistry Unreal Engine 5.5.4, et des indices textuels de reference
dans les sources de production et la configuration.

La consultation de GitHub seule ne permet pas de lire les references Unreal
serialisees dans les `.uasset`. Le resultat verifiable ne sera connu qu'apres
execution locale dans Unreal Editor.

## Test Editor-only et emplacement des rapports

Le test est implemente dans
`Source/GrimrockPrototypeEditor/Private/Tests/GridEditorUIWBPAudit02InventoryTests.cpp`.

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.UIWBPAudit02"
```

Un seul run produit, meme si la baseline de 46 WBP est invalide :

- `Saved/Audits/UI-WBP-AUDIT02/UI-WBP-AUDIT02.csv`
- `Saved/Audits/UI-WBP-AUDIT02/UI-WBP-AUDIT02.md`

Le CSV comprend pour chaque WBP : nom, package, classe, nombre et liste
complete des **referencers hard**, **soft**, **other** (manage/searchable
names), dependances sortantes hard/soft, mentions textuelles dans
`Source/` hors `Tests/` et `Config/`, et classification motivee.

Le Markdown contient la synthese et le detail des references pour chaque
widget. Aucun rapport n'est versionne par defaut : `Saved/` reste local.

## Classification prudente

| Etat | Sens | Action |
| --- | --- | --- |
| `REFERENCED_KEEP` | Au moins un referencer AssetRegistry on-disk, quelle que soit la categorie | Conserver ; inspecter les usages si consolidation souhaitee |
| `SOURCE_CONFIG_REVIEW` | Aucun referencer detecte, mais nom present dans les fichiers de production ou Config | Verifier chargements dynamiques et assignations dans Class Defaults |
| `DYNAMIC_ROOT_REVIEW` | Racine connue de l'UI, non referencee dans les donnees inspectees | Conserver provisoirement ; verifier Pawn, Maps, GameMode, Blueprints |
| `ZERO_REFERENCER_REVIEW` | Aucun referencer detecte et aucun indice textuel | **Candidat a la verification**, jamais automatiquement supprimable |
| `REDIRECTOR_REVIEW` | Redirector dans le namespace WBP | Analyser et Fix Up Redirectors dans Unreal, jamais supprimer par Git |

Le test echoue si le total differe de 46, ou si un redirector WBP est
encore present ; un ajout/suppression legitime ulterieur necessite d'actualiser
la baseline. Les rapports sont produits avant ces deux assertions.

## Verifications manuelles a faire apres lecture du rapport

Pour chaque candidat `ZERO_REFERENCER_REVIEW` ou `SOURCE_CONFIG_REVIEW` :

1. Ouvrir l'asset dans **Unreal Editor**, faire `Reference Viewer` et
   inspecter les **Referencers** (Hard/Soft ; options de recherche).
2. Examiner les class variables exposees dans les **Class Defaults** du
   Pawn, HUD, GameMode, level actors, maps et tout WBP parent.
3. Verifier les chargements dynamiques et les chemins construits, les
   chargements via les Data Assets et les maps de developpement utilisees.
4. Confirmer l'absence d'utilisation en **PIE** (menu, inventaire, Skills,
   flux d'evenements, combat, portraits, recrutement et chargement).
5. Seulement ensuite creer un ticket **distinct** de suppression, atomique,
   avec `Fix Up Redirectors`, validation automation et nouvelle smoke PIE.

### Protection contre les fausses deductions

- Un WBP de racine, un BP attribue dans une map ou un contenu charge
  dynamiquement peut ne pas avoir de referencer que le filtre de l'AssetRegistry
  on-disk suffit a etablir.
- Une mention dans le code peut n'etre qu'un commentaire ; elle est donc
  un **indice** et pas une preuve de chargement.
- Les dependances des assets de plugins ou generees a l'execution ne sont
  pas prouvees exhaustivement par l'AssetRegistry du jeu.
- L'audit ne prouve pas les dependances transitives (un referencer peut
  n'etre atteint par aucune racine runtime) ; cela releve de l'analyse
  des graphes et du parcours PIE.

## Widget Blueprints a surveiller specialement

- `WBP_RPGNotification` : conserve des appels C++ d'attribution de points,
  malgre le retrait des toasts de Level Up.
- `WBP_GrimrockMenu` : conserve des pages non autonomes
  (Spellbook, Journal, Recipes, Codex).
- `WBP_GridCombatHudAction` : appartient a la barre generale persistante
  depuis UI-GLOBALHUD01 ; **ne pas confondre avec code obsolete**.
- `WBP_ItemTooltipComparisonRow` : l'ancien audit UI-ASSET-CLEAN01
  constatait un referencer `WBP_ItemTooltip`.

Les observations doivent etre reconfirmees dans le rapport local et le
Reference Viewer. **Aucun de ces WBP n'est propose a la suppression dans
UI-WBP-AUDIT02.**

## Cloture

Le jalon d'**inventaire** sera valide lorsque l'utilisateur aura confirme
`Grimrock.Editor.UIWBPAudit02` OK et partage les deux rapports.
Le classement final `supprimable` demandera une seconde phase
Reference Viewer, avant un ticket de nettoyage separe.

## Correctif UI-WBP-AUDIT02.1 — scan Source/Config (2026-10-19)

Le premier run vert des 46 WBP a releve **8 fichiers sources scannes**, en fait les seuls `Config/*.ini`. Dans `ScanSourceFiles`, les appels successifs a `FindFilesRecursive` reutilisaient le meme tableau et seul le dernier resultat restait disponible. Les colonnes `SourceConfigCount` du premier rapport etaient donc incompletes. Les donnees **AssetRegistry hard/soft restent independantes de cette erreur**.

Le correctif collecte separement `*.cpp`, `*.h`, `*.cs` et `*.ini`, fusionne puis dedoublonne les chemins, et ajoute un garde-fou de **plus de 100 fichiers de production Source/** et au moins un fichier `Config/`. Le nombre de sources et de fichiers config est affiche explicitement dans le log et le rapport Markdown.

**Il faut regénérer les deux rapports en relançant le meme filtre TD04.2**. Ne pas classer definitivement `WBP_ItemReadPanel` avant cette nouvelle verification et le test de la fonction Lire dans `WBP_InventoryBag`.

## Resultat local valide — 19 octobre 2026, 20h51

Le filtre `Grimrock.Editor.UIWBPAudit02` a ete execute avec succes (1 test passe,
0 avertissement, 0 echec, exit code 0). Rapport :
`Saved/Automation/TD04/TD04-20261019-205158`.

Les rapports locaux `Saved/Audits/UI-WBP-AUDIT02/UI-WBP-AUDIT02.csv`
et `UI-WBP-AUDIT02.md` ont ete fournis et examines.

| Metrique | Resultat |
| --- | ---: |
| WBP dans l'arborescence UI | 46 / 46 |
| Avec au moins un referencer AssetRegistry | 45 |
| Sans referencer AssetRegistry | 1 |
| Referencers hard cumules | 50 |
| Referencers soft entrants | 0 |
| Redirectors WBP | 0 |
| Fichiers de production Source analyses | 554 |
| Fichiers de configuration analyses | 8 |

**Seul `WBP_ItemReadPanel`** est classe `ZERO_REFERENCER_REVIEW` :
`/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemReadPanel`.
Aucune reference hard, soft, autre, ni mention Source/Config n'a ete detectee.
Il ne faut pas le supprimer sans verifier dans Reference Viewer et en PIE
le parcours `WBP_InventoryBag -> action Lire -> PresentItemReading()` :
la fonctionnalite de lecture d'items conserve un contrat C++ actif et la
presentation peut etre incomplete ou chargee de maniere indirecte.

Les 45 autres WBP restent classes `REFERENCED_KEEP`. Notamment :
- `WBP_RPGNotification` : referents `WBP_GridPersistentHud` et `WBP_GridSkills` ; migration de feedback requise avant suppression.
- `WBP_GrimrockMenu` : referent `BP_GrimrockPartyPawn` ; les pages Spellbook, Journal, Recipes et Codex sont encore integrees.
- `WBP_GridCombatHudAction` : referent `WBP_GridPersistentHud` ; barre d'actions toujours active.
- `WBP_ItemTooltipComparisonRow` : referent `WBP_ItemTooltip` ; conserve.

**Statut : inventaire AssetRegistry UI-WBP-AUDIT02 valide ; aucune suppression
autorisee a ce stade.** Etape suivante : confirmation PIE de la fonction
Lire et Reference Viewer sur `WBP_ItemReadPanel`. La fermeture du chantier
de suppression reste independante de la validation de l'inventaire.
