# HOTBAR01.1 — Shortcut Quantity Badge

Date : 31.08.2026

## Objectif

La barre `1–9,0` affiche désormais le stock courant des raccourcis soutenus par une quantité d'inventaire. Cette présentation est générique et ne dépend pas spécifiquement de la pierre.

## Règle d'affichage

Un badge `xN` apparaît dans le coin inférieur droit lorsque l'action résolue :

- provient d'une source `QuickItem` ;
- consomme au moins une unité (`CurrentSourceItemQuantityCost > 0`) ;
- possède encore un stock strictement positif.

Le dernier exemplaire reste explicitement visible sous la forme `x1`.

Exemples :

- pierre : `x7` ;
- potion : `x3` ;
- parchemin : `x1` ;
- `PrimaryAttack`, arme équipée, sort ou capacité sans stock : aucun badge.

Quand le stock atteint zéro, HOTBAR01 supprime le binding du raccourci ; le badge disparaît donc en même temps que l'icône du slot.

## Widget

`UGridCombatHudActionWidget` expose un `Text_Quantity` optionnel (`BindWidgetOptional`).

Si le Widget Blueprint ne contient pas encore ce widget, le C++ crée automatiquement un badge natif dans `Overlay_ActionContent` : fond sombre, texte blanc, ombre et alignement en bas à droite. Cette compatibilité permet d'obtenir le compteur sans modification immédiate du `.uasset`.

Pour une future finalisation du WBP, un `TextBlock` nommé exactement `Text_Quantity` peut être ajouté manuellement et stylé dans le Designer ; le C++ continuera uniquement à écrire le texte et la visibilité.

## Autorité des données

Le badge n'effectue aucun comptage. Il affiche exclusivement `FGridAvailableCombatAction::CurrentSourceItemQuantity`, valeur déjà calculée par le catalogue/runtime. Il n'introduit donc aucune deuxième autorité d'inventaire.

## Validation

Le filtre dédié est :

`Grimrock.Hotbar.HOTBAR01_1`

Il vérifie `x7`, `x1`, le masquage à zéro et l'absence de badge pour les sources non consommables.
