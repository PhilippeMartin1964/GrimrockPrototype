# UI-LOCALIZATION01.12 — Final P0 closure audit

> **Statut : CLOSED — P0 localization complete.** Source closure 01.12A : `62203a68f9d122cc64fe965c0389bb473960c59f`. Matérialisation finale 01.12B : `6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e`.


Date : **13 octobre 2026**

## Résultat

Le rescan du `master` après `UI-LOCALIZATION01.11B` a trouvé deux reliquats
P0 réellement visibles par le joueur :

1. feedback du lancer physique dans `GrimrockPlayerController.cpp` ;
2. nom de repli `Personnage %d` du toast Level Up.

Ils sont corrigés par `UI-LOCALIZATION01.12A`.

La validation locale et la matérialisation `01.12B` sont terminées. Aucun autre
P0 brut confirmé n'est retenu à la clôture de la campagne.

## Reliquat corrigé : lancer physique

Le PlayerController envoyait directement des `FText::FromString` à
`ShowInteractionFeedback`. Ces textes franchissaient donc sans ambiguïté la
frontière joueur.

Le ticket migre :

- absence de niveau actif ;
- changement de personnage ;
- main principale vide ;
- changement d'objet tenu ;
- définition/personnage indisponible ;
- objet non lançable à une main ;
- objet trop lourd ;
- aucun objet sélectionné ;
- objet disparu de l'inventaire ;
- aucune cible sous le curseur ;
- pose impossible ;
- cible trop éloignée/invalide ;
- lancer impossible ;
- instruction de sélection d'une cellule ;
- fallback de ciblage invalide.

La clé `Interaction.Throw.TooHeavy` est également réutilisée par
`GridItemContextActionLibrary`, supprimant la double autorité de la même
phrase.

## Reliquat corrigé : Level Up

`RPGLevelUpNotificationSubsystem` pouvait afficher :

~~~text
Personnage %d
~~~

lorsqu'un nom de personnage était absent.

Le fallback devient :

~~~text
Progression.LevelUp.CharacterFallback
~~~

avec argument nommé `{Index}`.

## Éléments bruts conservés intentionnellement

### Diagnostics Save / persistence

Les `OutError` de sauvegarde, restauration, validation de SaveGame,
progression persistante et checkpoint pré-combat sont conservés comme
diagnostics techniques.

Le rescan n'a trouvé aucun chemin direct où ces détails sont convertis en texte
de présentation joueur. Les consommateurs observés les journalisent ou les
propagent dans des API techniques.

Ils ne doivent pas être déplacés dans `UI.csv` sans besoin de présentation
réel.

### Outils développeur

`RPGProgressionPIESimulator` est un outil PIE / console développeur. Ses
messages d'écran et de log sont hors périmètre P0 joueur.

### Notations mécaniques

Sont conservés sans clé linguistique :

- `-` ;
- `xN` / `+N` ;
- `N / M` lorsqu'aucun mot n'est associé ;
- chiffres de raccourcis ;
- chiffres romains de tiers ;
- séparateurs et puces.

### Identités techniques de secours

Des fallbacks comme `Hero_01` ou `FText::FromName(Id)` subsistent dans
quelques chemins de contenu invalide ou incomplet.

Ils ne constituent pas une autorité de wording normal : un Data Asset valide
doit fournir son `DisplayName`. Ils sont classés diagnostic/fallback
technique, pas P0 de localisation.

### Validation et runtime diagnostics

Les messages de validation d'assets, Lua, quêtes, status persistence,
définitions de monstres, level variables et diagnostics runtime restent des
chaînes techniques destinées aux développeurs.

## LOCTEXT / NSLOCTEXT — P1

Les `LOCTEXT/NSLOCTEXT` encore présents ne sont pas classés P0 brut : leur
intention de localisation est explicite.

Ils constituent toutefois un chantier P1 distinct. Le target `Game` actuel
est volontairement centré sur `ST_UI`; une localisation complète de toutes
les surfaces demandera soit :

1. d'étendre le Gather aux sources C++ ;
2. soit de migrer les textes UI génériques restants vers `UI.csv`.

Ce choix ne bloque pas la fermeture P0.

## Traduction anglaise complète

`ApplyEnglishSmokeTranslations.ps1` est un fixture de smoke, pas un catalogue
anglais exhaustif.

La fermeture P0 signifie :

- plus de prose joueur P0 brute identifiée dans les chemins audités ;
- autorité et pipeline de localisation stables ;
- smoke fr/en fonctionnel.

Elle ne signifie pas encore que chaque clé du jeu possède une traduction
anglaise éditoriale complète.

## Garde-fou

Le test :

~~~text
Game.UI.Localization.LOCALIZATION01.FinalP0Closure
~~~

vérifie notamment :

- les 16 clés utilisées par le reliquat final, dont la clé de ciblage déjà
  existante ;
- zéro `FText::FromString(TEXT(...))` restant dans le PlayerController ;
- zéro source brute formatée dans ce PlayerController ;
- réutilisation de `Interaction.Throw.TooHeavy` dans les deux surfaces ;
- disparition de `Personnage %d` du toast Level Up ;
- couverture fixture anglais + smoke runtime.

## Fermeture

Après validation de `01.12A` et matérialisation `01.12B`, le chantier
**UI-LOCALIZATION01 P0 est terminé** sur la baseline `6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e`.

Les suites sont des campagnes séparées :

- P1 : unification/gather des `LOCTEXT/NSLOCTEXT` ;
- traduction anglaise exhaustive ;
- autres cultures ;
- outils de traduction / export pour créateurs de niveaux.
