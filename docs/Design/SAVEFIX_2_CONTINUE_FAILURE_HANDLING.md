# SAVEFIX.2 — Échec de Continue sans réinitialisation du groupe

## Objectif

Un échec de chargement demandé par **Continuer** ne doit jamais être interprété comme une nouvelle partie.

## Contrat runtime

Pour un `EGrimrockPartyStartupMode::Continue` avec un fichier de sauvegarde existant :

- `LoadCurrentGameData()` tente de restaurer le groupe et le donjon ;
- en cas de succès, le flux normal `PartySave Continued` reste inchangé ;
- en cas d'échec, `ResetPartyForNewGame()` n'est plus appelé ;
- la sauvegarde sur disque n'est ni supprimée ni remplacée ;
- le pawn en cours désarme son autosave de `EndPlay` afin de ne pas écraser le slot pendant le retour au menu ;
- `UGrimrockGameInstance::RequestReturnToMainMenu()` renvoie au menu principal ;
- `ShowInitialCharacterCreationWidget()` n'est jamais atteint dans ce flux d'échec.

Le comportement spécifique des playtests PIE frais reste inchangé : leur chargement de profil peut toujours retomber sur un état de nouveau groupe sans altérer le contrat du bouton **Continuer** du menu principal.

## Diagnostics attendus

Un échec de Continue produit :

```text
PartySave Load Failed Slot=<slot> Reason=<cause>
PartySave ContinueAborted Slot=<slot> Reason=<cause> Action=ReturnToMainMenu
```

Il ne doit pas être suivi de :

```text
CharacterCreation UI Shown
```

## Protection contre l'écrasement lors du teardown

Avant le changement de niveau, `PartySaveSlotName` est vidé uniquement sur le pawn qui est en train d'être détruit. `EndPlay()` n'effectue désormais l'autosave que si le nom du slot du pawn est non vide. Le fichier de sauvegarde réel et la configuration du slot dans `UGrimrockGameInstance` restent intacts.

## Test automatisé non destructif

Le contrat d'échec est couvert par :

```text
Grimrock.Save.SAVEFIX.2.ContinueFailureIsNonDestructive
```

Le test utilise un slot temporaire unique et reproduit le cas qui avait révélé le problème : le snapshot du groupe est suffisamment valide pour être proposé par le menu principal, mais il référence volontairement une définition d'objet absente du niveau runtime. Le chargement échoue donc après l'entrée dans le vrai flux `Continue`, sans modifier la sauvegarde du joueur.

Le test vérifie que :

- le pawn courant désarme son autosave `EndPlay` ;
- le groupe runtime précédent n'est pas remplacé par un état de nouvelle partie ;
- le modal de création de personnage ne s'ouvre pas ;
- `RequestReturnToMainMenu()` est effectivement atteint, vérifié par la remise à zéro du pending load slot du `UGrimrockGameInstance` ;
- le fichier temporaire existe encore après l'échec et après le teardown du monde ;
- le snapshot original reste lisible et conserve le même `CharacterId`.

Le niveau de menu principal est volontairement neutralisé dans le fixture afin d'empêcher tout vrai `OpenLevel()` pendant l'automation ; l'erreur attendue `NoMainMenuLevelName` est déclarée explicitement au framework de test.

## Validation manuelle

1. Vérifier qu'une sauvegarde valide continue à produire `PartySave Continued` et charge la partie normalement.
2. Avec une sauvegarde volontairement non chargeable, cliquer **Continuer**.
3. Vérifier le couple de logs `PartySave Load Failed` / `PartySave ContinueAborted`.
4. Vérifier le retour automatique à `L_MainMenu`.
5. Vérifier l'absence de `CharacterCreation UI Shown`.
6. Vérifier que le fichier de sauvegarde existe toujours après le retour au menu.

La validation manuelle du chemin nominal a été effectuée sur une sauvegarde réelle version 5 : `PartySave Continued Slot=GrimrockParty CharacterCount=1`, sans apparition du menu de création. Le chemin d'échec reste validé par le test isolé ci-dessus afin de ne jamais corrompre volontairement une sauvegarde utilisateur.
