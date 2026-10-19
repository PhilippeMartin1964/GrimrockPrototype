# EVENTHUD01.18 — Presentation incrementale du Flux d'evenements

Date : 19 octobre 2026
Statut : **code publie, validation locale UE5.5.4 attendue**.

## Pourquoi

Avant EVENTHUD01.18, `UGridEventFeedWidget::HandleEventPublished` appelait
`RebuildEntryPresentation(true)` pour chaque publication visible. L'ancienne
routine effacait puis recreait tous les UMG rows, `UTextBlock` timestamps et
messages, et les tooltips de combat du buffer (200 entrees par defaut). Les
publications repetees, comme une surcharge ou les logs de combat, induisaient
des allocations de Widgets sans changement des anciens messages.

## Solution

Le widget suit en memoire, **exclusivement pour la presentation**, le tableau
`PresentedSequenceIds` des `FGridHudEventEntry::SequenceId` affiches.

Sur `OnEventPublished`, `SynchronizePublishedEntryPresentation` compare
la projection filtree du buffer authoritative avec les IDs des widgets
actuellement presentes. Le chemin nominal :

1. Conserve les rows encore dans le buffer, et donc leurs tooltips ;
2. Retire seulement les anciens rows sortis du FIFO, dans l'ordre ;
3. Ajoute seulement les nouvelles entrees visibles par
   `AppendEntryPresentation` ;
4. Recentre le dernier message pour une publication visible avec le mecanisme
   de quatre passes de layout `EVENTHUD01.16`.

L'inspection est bornee par le buffer : **O(N)** en lecture d'IDs, mais **O(1)**
creations de widget pour une nouvelle entree (et, a capacite atteinte,
suppression de l'entree la plus ancienne). Les reconstructions completes
restent limitees aux chargements initiaux, changements de filtres, et a la
recuperation si la presentation ne correspond plus au buffer (reset,
structure externe modifiee ou IDs incoherents).

Une publication dans une categorie masquee ne recentre pas la liste ; elle
reste cependant capable d'expulser une ancienne ligne visible quand le FIFO
deborde. La position de lecture lors d'un changement de filtre reste
determinee par `RebuildEntryPresentation`.

`AppendEntryPresentation` conserve les elements graphiques existants :
format `[J 1, HH:MM:SS]`, styles semantiques, retour a la ligne, details des
evenements non combat dans le texte, details de combat dans un tooltip partage.

Le `FGridEventFeedBuffer` et `UGridEventFeedSubsystem` restent les seules
autorites des evenements. Aucun schema de sauvegarde, DataAsset, WBP ou niveau
n'est modifie. La sequence n'est **jamais** serialisee par ce cache local.

## Tests

Test ajoute :
`Grimrock.UI.EventHUD01.Interaction.IncrementalProjection`.

Il publie des evenements dans un vrai `UGridEventFeedSubsystem` et verifie :

- nouvelle ligne uniquement, conservation d'identite du widget precedant ;
- preservation du tooltip de combat ;
- suppression du seul ancien row quand le buffer atteint sa capacite ;
- reconstruction apres filtre et restoration d'un filtre ;
- publication masquee ne declenchant pas un nouveau scroll ;
- eviction par un evenement masque ;
- reconstitution de la presentation apres desynchronisation du WidgetTree ;
- conservation de l'autoscroll sur une publication visible.

### Validation locale

Fermer Unreal Editor, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.EventHUD01.Interaction"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.EventHUD01"
```

### Smoke PIE indispensable

1. Produire plusieurs dizaines de messages de types Dungeon, Combat et Dialogue.
2. Verifier le scroll vers la derniere ligne lors de toute nouvelle publication
   visible, meme apres remontée manuelle de l'historique.
3. Verifier que la roulette reste utilisable entre publications.
4. Masquer Combat, generer des evenements Combat, puis restaurer le filtre ;
   verifier chronologie, tooltip combat et absence de duplication.
5. Laisser depasser la limite de 200 evenements ; verifier suppression des
   plus anciens sans clignotement de toutes les lignes.
6. Tester les messages multilignes et le redimensionnement du panel.

## Limites

Le test d'automatisation sans rendu Slate confirme la conservation des instances
UMG et les regles de synchronisation, mais ne mesure ni la position en pixels de
la ScrollBox ni des temps GPU/CPU PIE. Les performances perçues et l'autoscroll
en reel necessitent les smoke tests ci-dessus.
