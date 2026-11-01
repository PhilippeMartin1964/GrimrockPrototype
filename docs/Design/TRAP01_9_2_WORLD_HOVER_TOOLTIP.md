# TRAP01.9.2 — Raccordement de l'infobulle au survol du monde

**Statut : C++ publié sur master ; compilation UE 5.5.4 et recette PIE à valider localement.**

## Diagnostic
En PIE, le piège détecté affiche le curseur Use et réagit au clic gauche, mais aucun texte de survol n'apparaît.
`GridGenericObjectActor::GetInteractionText_Implementation` produit le DC et les probabilités TRAP01.9.1.
`GrimrockPlayerController::UpdateHoveredInteractable` ne lisait que `Execute_GetInteractionCursor`, jamais `Execute_GetInteractionText`.

## Correctif
- Après une trace ECC_Visibility autorisée et `CanInteract`, le PlayerController récupère `Execute_GetInteractionText`.
- La présentation réutilise `UGridNavigationTooltipWidget` et le WBP existant `WBP_GridNavigationTooltip`, sans nouveau WBP ni logique de piège dans le contrôleur.
- Source de classe : override `WorldInteractionTooltipWidgetClass` dans le PlayerController ; sinon `PartyPawn->PersistentHudWidgetInstance->NavigationTooltipWidgetClass`.
- Une seule instance est créée, sous le curseur personnalisé (`Z=9998` contre `Z=9999`), sans interception des clics (`HitTestInvisible`).
- Le FText est réactualisé uniquement lorsqu'il change et la position suit la souris, avec prise en compte du DPI et bornage au viewport.
- Le texte est masqué en cas de cible absente, portée dépassée, refus d'interaction, UI modale, item au curseur, visée ou combat.
- `MaxInteractionDistance` demeure 300 cm et toutes les règles de désamorçage sont inchangées.
- Le contrôleur retire l'instance UMG lors de `EndPlay`.

## WBP et localisation
- WBP existant : `/Game/GrimrockPrototype/Blueprints/UI/Interaction/WBP_GridNavigationTooltip`, parent `UGridNavigationTooltipWidget`.
- Designer : vérifier `Text_Tooltip` et sa largeur/son retour à la ligne ; conserver ses styles dans l'UMG.
- `UI.csv` inchangé : aucune chaîne nouvelle dans ce jalon.
- `ST_UI` inchangé : aucune nouvelle clé à matérialiser.
- WBP inchangé côté code source : contrôler sa configuration réelle dans UE.
- Game fr/en : aucun nouveau contenu, traduction des textes `NSLOCTEXT` TRAP01.9.1 à confirmer indépendamment.

## Validation locale

PowerShell :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.9.2"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.GlobalHud01"

## Recette PIE sur L_Dungeon
1. Piège caché : ni infobulle ni fuite du DC.
2. Piège détecté à une case : curseur Use, infobulle affichant difficulté et risques.
3. Sans trousse : difficulté et « Évaluation indisponible ».
4. Avec personnage formé et outils personnels : nom et trois pourcentages totalisant 100 %.
5. Retrait du curseur, UI modale, item au curseur ou combat : infobulle masquée.
6. Le clic gauche continue de résoudre une seule tentative, sans régression.
7. Vérifier l'absence de dépassement d'écran en plusieurs tailles et DPI.
8. Save/Continue : état et texte actualisés, sans infobulle périmée.

**Ne pas clôturer le jalon sans validation de la compilation et captures PIE.**
