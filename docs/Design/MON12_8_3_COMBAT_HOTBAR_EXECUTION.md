# MON12.8.3 — Exécution de la barre de combat

## Résultat

Les dix raccourcis configurés par MON12.8.2 peuvent maintenant être demandés
par clic gauche ou avec la rangée numérique principale :

```text
1 2 3 4 5 6 7 8 9 0
```

Les indices persistants restent `0` à `9`. La touche `1` demande l'index `0`
et la touche `0` demande l'index `9`.

## Chemin autoritaire unique

Le clic et le clavier appellent tous deux
`UGridCombatHudWidget::RequestHotbarSlot()`. Cette fonction reconstruit la vue
du HUD avant chaque tentative, puis transmet l'identité résolue à
`RequestCharacterCombatAction()`.

Le binding ne devient donc jamais une autorité de gameplay. Le TurnManager
revalide encore :

- le combat et la phase du joueur ;
- le personnage actuellement actif ;
- l'état de repos du groupe ;
- la présence et la provenance de l'arme ;
- les PA et les autres coûts exposés par le catalogue ;
- la cible et la portée de l'attaque.

Un slot vide, non résolu, grisé ou obsolète est refusé sans dépense. Une arme
qui a changé de main est retrouvée par son identifiant runtime, puis exécutée
depuis son emplacement actuel.

## Clic et glisser-déposer

Le bouton ne lance pas l'action au moment de l'appui. Le widget demande d'abord
la détection de drag :

- relâchement sans drag : exécution du raccourci ;
- dépassement du seuil de drag : création de l'opération MON12.8.2, sans
  exécution ;
- clic droit : suppression du raccourci, inchangée.

Cette séparation évite qu'une tentative de déplacement ou d'échange déclenche
accidentellement une attaque.

## Interception des interfaces modales

Les touches sont liées dans `AGrimrockPartyPawn`, propriétaire du HUD. Elles
sont consommées mais ne sont jamais exécutées pendant une pause. La demande
est ignorée si :

- le menu ou l'inventaire est visible ;
- la création de personnage est active ;
- le `GrimrockPlayerController` signale qu'une interface modale possède les
  entrées ;
- l'instance du HUD n'existe pas.

Les champs de texte et les commandes des écrans concernés gardent ainsi la
priorité sur la barre de combat.

## Actions exécutables à ce jalon

MON12.8.3 déclenche tous les bindings que le catalogue et le TurnManager
marquent déjà exécutables. Dans l'état actuel, cela couvre les attaques
`Equipment` et l'attaque universelle à mains nues lorsqu'un binding correspondant
est configuré.

Les capacités et sorts de profil `Effect`, ainsi que les sources `QuickItem`,
restent volontairement grisés : leur consommation de mana, leurs cooldowns,
leurs effets et leurs objets sources ne doivent pas être simulés par le HUD.
Les potions et parchemins appartiennent à MON12.8.4.

## Widget Blueprint

Aucune modification `.uasset` n'est requise. `Button_Action` doit rester
hit-testable dans `WBP_GridCombatHudAction`. Il ne faut ajouter aucun événement
`OnClicked` dans le Graph : le C++ distingue déjà clic et drag, puis route
l'action vers l'autorité.

## Tests automatisés

Le filtre suivant couvre ce jalon :

```text
Grimrock.Monsters.MON12.8.3
```

Il vérifie :

1. l'exécution d'une arme par le widget et le paiement exact des PA ;
2. le refus sans dépense d'un slot vide ;
3. les dix liaisons clavier `1–9, 0` ;
4. l'interception par l'inventaire et la création de personnage ;
5. l'exécution après fermeture de l'interface modale.

## Suite

MON12.8.4 ajoute les contributions et les exécuteurs autoritaires des potions
et parchemins : résolution par définition, quantité totale et consommation
après réussite. Depuis MON12.8.9, chaque consommation acceptée supprime aussi
le binding de la barre.
