# 11 - Mouse Interaction Regression Checklist

## Objectif

Cette checklist sert de baseline de stabilité pour le système d'interaction souris après MI1 à MI7.

Elle doit être exécutée avant toute modification future touchant :

- interaction souris ;
- inventaire ;
- item tenu au curseur ;
- menu d'action item ;
- réceptacle ou support de torche ;
- wall lock ;
- bouton, levier, porte ou chaîne logique ;
- readable message.

Le but est de détecter rapidement les régressions de priorité UI / monde, de hover, de routage d'item tenu et de fermeture de menu.

## Préconditions Git / Build

- Être sur `master`.
- Vérifier `git status -sb` avant test.
- Partir d'un état propre ou comprendre explicitement chaque fichier déjà modifié.
- Vérifier qu'aucun `.uasset` ou `.umap` non voulu n'est modifié après un simple test PIE.
- Fermer Unreal Editor avant build si Live Coding bloque la compilation.
- Faire un rebuild propre si Unreal signale une erreur de DLL, de module ou `UnrealEditor-GrimrockPrototype.dll` avec image incorrecte.
- Ne jamais utiliser `git add .` pour ces étapes. Préparer seulement les fichiers réellement voulus.

## Filtres de logs utiles

Rechercher ces signatures dans les logs PIE ou la console :

```text
LogGridMouse
GridMouse
HoverCursorItem
WallLockAttempt
ReceptacleAttempt
WorldDropAttempt
ThrowAttempt
GridItemActionMenu
RemoveFromParent
GridItemTransfer
GridFacingTarget
```

Les logs de clic `LogGridMouse` doivent rester visibles au niveau `Log`. Les logs de hover sont normalement au niveau `Verbose`.

Pour réactiver temporairement les logs de hover :

```text
Log LogGridMouse Verbose
```

ou au lancement :

```text
-LogCmds="LogGridMouse Verbose"
```

## Matrice de tests manuels

| ID | Scénario | Préparation | Action | Résultat attendu | Logs attendus | Régression typique |
|---|---|---|---|---|---|---|
| A01 | Clic vide monde | Se placer face à une zone sans interactable | Clic gauche dans le vide | Aucune mutation, fallback `NoInteractable` | `GridMouse`, `NoInteractable` | Activation fantôme ou drop non demandé |
| A02 | Item monde pickup | Placer un item au sol à portée | Hover puis clic gauche sur l'item | Curseur `Take`, item ajouté à l'inventaire, acteur retiré | `GridMouse`, pickup item | Premier item de cellule ramassé au lieu de l'acteur visé |
| A03 | Levier | Se placer à portée d'un levier | Clic gauche sur la partie mobile | Curseur `Pull`, bascule via runtime | `GridMouse`, interaction edge | Appel direct ou clic sur mauvais composant |
| A04 | Bouton | Se placer à portée d'un bouton | Clic gauche sur la partie mobile | Curseur `Push`, activation via runtime | `GridMouse`, interaction edge | Bouton ignoré ou porte ouverte directement |
| A05 | Porte / chaîne | Se placer face à une porte liée à un mécanisme | Cliquer la porte, puis actionner bouton/levier | Porte non cliquable directement, ouverture seulement via lien | `GridMouse` côté mécanisme | Porte devenue interactable directe |
| A06 | Readable message | Cliquer un objet lisible | Afficher le message, puis clic gauche | Premier clic affiche, clic suivant ferme le message sans action derrière | `GridMouse`, readable close | Fermeture et interaction monde déclenchées ensemble |
| B01 | Inventaire ouvert sans item curseur | Ouvrir l'inventaire, ne rien tenir au curseur | Clic gauche dans le monde | Clic monde ignoré | `GridMouse` inventaire ouvert | Pickup, activation ou drop à travers l'inventaire |
| B02 | Menu action item ouvert | Ouvrir un menu par clic droit sur item | Clic gauche vers le monde hors panneau | Monde bloqué ou menu fermé proprement selon click catcher | `GridItemActionMenu Closed` | Clic monde qui passe derrière le menu |
| B03 | ClickOutside | Menu action item visible | Cliquer hors `Border_MenuPanel` | Menu retiré, inventaire parent conservé | `GridItemActionMenu Closed Reason=ClickOutside` | `WBP_GridInventory` retiré au lieu du menu |
| B04 | Action Equip | Item équipable en inventaire | Clic droit, choisir `Equiper` | Item équipé dans le slot exact, refresh UI et visuel | `GridItemActions Execute Equip` | Action ambiguë ou mauvaise main choisie |
| B05 | Action Unequip | Item équipé | Clic droit sur slot équipé, choisir `Enlever` | Item retourne en inventaire si place disponible | `GridItemActions Execute Unequip` | Item perdu, dupliqué ou visuel non synchronisé |
| B06 | Action PlaceOnTarget | Item compatible en inventaire, cible face au groupe | Clic droit item, choisir `Placer` | Transfert atomique vers réceptacle cible | `GridItemActions Execute PlaceOnTarget`, `GridItemTransfer` | Passage par cursor ou retrait avant refus cible |
| B07 | Absence RemoveFromParent | Ouvrir puis fermer menu plusieurs fois | ClickOutside, action réussie, fermeture répétée | Aucun warning `RemoveFromParent` | Absence de warning | Warning ou disparition de `Page_Inventory` / `TopTabs` |
| C01 | Pierre curseur sur sol proche | Tenir une pierre throwable au curseur | Cliquer un sol valide proche | `WorldDrop` si dépôt valide | `WorldDropAttempt` | Lancer prioritaire alors qu'un drop valide existe |
| C02 | Pierre hors portée dépôt | Tenir une pierre throwable, viser hors portée de dépôt | Clic gauche | Tentative de `ThrowAttempt` si lancer possible, sinon refus propre | `ThrowAttempt` | Item supprimé ou déposé hors portée |
| C03 | Torche curseur sur sol proche | Tenir une torche au curseur | Cliquer un sol valide proche | Dépôt monde valide | `WorldDropAttempt` | Refus alors que le sol est valide |
| C04 | Torche curseur cible invalide | Tenir une torche au curseur, viser une cible incompatible | Clic gauche | `CannotPlaceItem` ou refus propre, torche reste au curseur | `HoverCursorItem`, refus | Dépôt monde accidentel derrière la cible |
| C05 | Item non throwable | Tenir un item non throwable | Viser hors dépôt valide | Pas de `AimThrow`, refus explicite | Pas de `ThrowAttempt` réussi | `AimThrow` affiché pour item non throwable |
| D01 | Torche vers support compatible | Tenir une torche au curseur | Clic support de torche vide compatible | Support prime sur drop, torche placée | `ReceptacleAttempt`, `GridItemTransfer` | Torche déposée au sol derrière le support |
| D02 | Item incompatible vers support | Tenir un item incompatible | Clic support/réceptacle | Refus propre, item conservé | `ReceptacleAttempt`, refus compatibilité | Fallback WorldDrop derrière le réceptacle |
| D03 | Réceptacle plein ou non compatible | Préparer réceptacle plein si possible | Tenter dépôt d'item | Refus sans mutation | `ReceptacleAttempt`, refus capacité/compatibilité | Item perdu, dupliqué ou déplacé au sol |
| D04 | Pas de dépôt derrière réceptacle | Viser précisément le support/réceptacle avec item tenu | Clic gauche | Le hit réceptacle décide, pas le sol derrière | `ReceptacleAttempt` avant `WorldDropAttempt` | Dépôt monde accidentel derrière la cible |
| E01 | Clé compatible vers wall lock | Tenir la bonne clé au curseur | Clic sur serrure verrouillée | Wall lock prime, clé insérée après validation, `Activated` émis | `WallLockAttempt`, `UnlockSuccess` | Drop de clé au sol derrière la serrure |
| E02 | Mauvaise clé vers wall lock | Tenir une mauvaise clé | Clic sur serrure | Refus, aucune ouverture, clé conservée | `WallLockAttempt`, `MissingKey` ou refus | Ouverture avec mauvaise clé |
| E03 | Item non clé vers wall lock | Tenir une torche, pierre ou autre item | Clic sur serrure | Refus propre, aucun drop derrière | `WallLockAttempt`, refus type | Item déposé derrière la serrure |
| E04 | Pas d'auto-unlock inventaire | Bonne clé dans inventaire, rien au curseur | Cliquer directement la serrure | Pas d'ouverture silencieuse depuis inventaire | `InventoryAutoUnlockBlocked` ou aide | Serrure scanne l'inventaire et s'ouvre seule |
| F01 | Priorité readable | Message lisible actif, interactable derrière | Clic gauche | Fermeture du message seulement | Readable close avant world interaction | Activation derrière le message |
| F02 | Priorité UI modale | Menu action item ou UI modale visible | Clic gauche vers monde | Monde bloqué | `GridItemActionMenu` ou blocage UI | Interaction monde traversante |
| F03 | Priorité inventaire ouvert | Inventaire ouvert, aucun item curseur | Clic gauche monde | Monde ignoré | `GridMouse` inventaire ouvert | Drop ou pickup involontaire |
| F04 | Priorité item curseur | Item tenu, viser successivement wall lock, réceptacle, sol, hors portée | Cliquer chaque cible | Ordre stable : wall lock > réceptacle > world drop > throw > échec | `WallLockAttempt`, `ReceptacleAttempt`, `WorldDropAttempt`, `ThrowAttempt` | Ordre inversé ou fallback trop tôt |
| F05 | World interactable final | Aucun readable, UI, inventaire bloquant ou item curseur | Cliquer bouton/levier/item | `IGridInteractableInterface` reçoit l'interaction | `GridMouse`, interactable | World interactable bloqué par un état UI obsolète |

## Signatures de régression à surveiller

- Warning `RemoveFromParent`.
- Clic monde qui passe derrière le menu d'action item.
- Item incompatible déposé au sol derrière une serrure ou un réceptacle.
- `AimThrow` affiché pour un item non throwable.
- Oscillation permanente du curseur entre `Default`, `PlaceItem`, `CannotPlaceItem` et `AimThrow`.
- Disparition des logs de clic `GridMouse` / `LogGridMouse`.
- `.uasset` modifié après simple test PIE.
- `.umap` modifiée sans intention explicite.
- `UnrealEditor-GrimrockPrototype.dll` signalée comme image incorrecte après build.
- Hover interactif affiché alors que le clic échoue sans raison claire.
- `WorldDropAttempt` exécuté avant `WallLockAttempt` ou `ReceptacleAttempt` sur une cible valide.

## Procédure de validation avant commit

1. Exécuter `git status -sb`.
2. Vérifier `git diff --stat`.
3. Exécuter `git diff --check`.
4. Si du C++ a été modifié, compiler avec UnrealBuildTool ou Visual Studio 2022.
5. Lancer un smoke test PIE avec la matrice minimale adaptée au changement.
6. Vérifier qu'aucun asset, Blueprint, DataAsset ou binaire non voulu n'a changé.
7. Préparer un commit ciblé uniquement avec les fichiers nécessaires. Ne pas utiliser `git add .`.
8. Pousser explicitement avec `git push origin master` seulement si le commit est validé et que le push est demandé.

