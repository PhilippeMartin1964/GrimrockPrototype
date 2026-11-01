# TRAP-SEARCH01.2 — Recherche active : commande joueur et HUD

**Statut : C++ et CSV publiés sur master, compilation UE5.5.4 et matérialisation ST_UI/WBP/fr-en à valider localement.**

## Entrées

- **F (Fouiller)** : binding natif dans `AGrimrockPlayerController::SetupInputComponent`, `bConsumeInput=true`, non exécuté en pause.
- Bouton facultatif `Button_SearchSurroundings` : la classe native `UGridPersistentHudWidget` y relie `OnClicked` à la **même** API C++ `RequestActiveTrapSearch()`.
- La commande ne dépend d'aucun slot de hotbar et ne dépense pas de PA hors combat.
- Vérifier dans UE que `IMC_Grimrock` n'assigne pas F à une autre action ; les assets binaires ne sont pas inspectables via la recherche GitHub.
- Refus silencieux si inventaire, création de personnage, Compétences, carte, objet tenu, ciblage, lancer, message lisible, combat, chute ou relocation. Le runtime conserve ses propres garde-fous.
- Une recherche acceptée publie le même message neutre quelle que soit la présence ou la détection d'un piège ; seul un succès réel ajoute `Dungeon.Trap.Detected` et révèle l'objet.
- Aucun changement de dégâts, portée, désamorçage, anti-reroll ou sauvegarde de TRAP-SEARCH01.1.

## Autorités UMG et localisation

- WBP existant : `/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud`, parent `UGridPersistentHudWidget` ; chemin confirmé dans les tests de production du dépôt.
- Designer WBP : ajouter **après matérialisation ST_UI** un `Button` nommé exactement `Button_SearchSurroundings`, cocher `Is Variable`, dans la barre de navigation, **hors de `Panel_ActionBar`**. Garder les dimensions, icônes et styles des boutons existants. Compiler et sauvegarder WBP.
- Le bouton est `BindWidgetOptional`, donc la touche F fonctionne dès compilation même si l'asset WBP n'a pas encore été complété.
- Tooltip : présentateur `WBP_GridNavigationTooltip` existant via `NavigationTooltipWidgetClass`. Aucun nouveau WBP, TextBlock C++ ou Event Graph.
- **Clés nouvelles dans `Localization/Source/UI.csv`** : `Navigation.Tooltip.SearchSurroundings` = `Examiner les alentours (F)` ; `Exploration.Search.Performed` = `Le groupe examine attentivement les alentours.`
- Les textes sont résolus via `FUILocalizationText::Get`. Le texte de découverte existant reste le `NSLOCTEXT` du gameplay.

## Protocole obligatoire (docs/UI_WBP_LOCALIZATION_WORKFLOW.md)

1. `git status --short`, `git pull --ff-only origin master`, fermer Unreal Editor, **compiler l'éditeur** (Build.bat ci-dessous). Ne lancer la suite de tests .2 qu'après la matérialisation de ST_UI : elle vérifie volontairement la présence réelle des deux clés.
Compilation de l'éditeur avec UBT :

~~~powershell
& 'D:\\UE_5.5\\Engine\\Build\\BatchFiles\\Build.bat' GrimrockPrototypeEditor Win64 Development '-Project=D:\\Development\\GrimrockPrototype\\GrimrockPrototype.uproject' -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Echec compilation UE' }
~~~

2. Régénérer **l'asset réel**, éditeur fermé :

~~~powershell
cd D:\Development\GrimrockPrototype
& 'D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\Development\GrimrockPrototype\GrimrockPrototype.uproject' -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) { throw 'Echec UIStringTableAuthoring' }
Test-Path .\Content\Localization\StringTables\ST_UI.uasset
git status --short -- .\Content\Localization\StringTables\ST_UI.uasset
~~~

3. Vérifier dans les logs `ST_UI materialization succeeded`, rouvrir Unreal, ouvrir `/Game/Localization/StringTables/ST_UI` et vérifier **les deux clés**. **STOP WBP si elles ne sont pas présentes.**
4. Dans le WBP utilisé par le jeu, ajouter le bouton facultatif (hors hotbar), sauvegarder, vérifier tooltip et la largeur restante de la hotbar.
5. Faire Gather, export PO en, traduire les deux clés, Import, Compile des locres. Le smoke `RunLocalizationSmoke.ps1` n'assure pas la traduction des nouvelles clés. Tester Game/fr et Game/en.
6. Inspecter `git status` : les projections locales `ST_UI.uasset`, WBP et ressources Game ne doivent être ni perdues ni prétendues déjà commitées ; gérer leur publication cohérente avant clôture du jalon.

## Validation UE5.5.4 (après matérialisation ST_UI)

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.TRAP_SEARCH01.2'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.TRAP_SEARCH01.1'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.UI.GlobalHud01'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.TRAP01'
.\Scripts\Localization\RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5
~~~

## Recette PIE

1. Dans `L_Dungeon`, appuyer sur **F** en exploration : confirmation neutre, même dans une pièce sans piège.
2. Sur un piège caché à 1-2 cases : succès = révélation/événement/tooltip existant ; échec = aucune information sur sa présence.
3. Répéter F au même endroit, tourner sur place, Save/Continue : les tentatives ne sont pas renouvelées à la même distance.
4. Changer d'une à deux cases permet au plus la seconde tentative persistée.
5. Tester le bouton `Button_SearchSurroundings` après création locale : même effet et mêmes blocages que F.
6. Inventaire, Compétences, carte, création, item au curseur, combat, visée et chute : aucune recherche, aucun événement neutre.
7. Vérifier le mapping F dans IMC, le style du bouton, la hotbar intacte et les textes en Game/fr et Game/en.

## Statut checklist UI.csv → ST_UI → WBP → Game/fr/en

| Étape | État |
|---|---|
| UI.csv | Modifié et versionné dans ce commit |
| ST_UI.uasset | **Non matérialisé** dans cet environnement ; commandlet local indispensable |
| WBP_GridPersistentHud | **Bouton Designer à créer localement** ; binding C++ déjà en place |
| Game/fr/en | **Gather, traductions et PIE à vérifier** |

**TRAP-SEARCH01.2 ne sera pas clôturé avant ces preuves, même si la commande F est opérationnelle.**