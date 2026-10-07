# UI-RPG01 — Visual Synthesis

Date : **7 octobre 2026**  
Statut : **DIRECTION UI-RPG01 VALIDÉE — production graphique détaillée transférée à UI-RPG02**

## Objectif

Cette page fixe la lecture visuelle retenue avant la production UMG finale. Elle ne décrit aucune nouvelle règle gameplay.

## Écran cible

```text
┌────────────────────────────────────────────────────────────────────────────┐
│ [Portrait] NOM                CLASSE — Niveau N         ◆ X Talent Points  │
│            identité courte de classe                                      │
├────────────────────────────────────────────────────────────────────────────┤
│        COMPÉTENCES                           TALENTS                       │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│     BRANCHE 1                BRANCHE 2                BRANCHE 3             │
│     identité                 identité                  identité              │
│                                                                            │
│       [ I ]                    [ I ]                    [ I ]                │
│         │                        │                        │                   │
│       [ II ]                   [ II ]                   [ II ]               │
│         │                        │                        │                   │
│      [ III ]                  [ III ]                  [ III ]               │
│         │                        │                        │                   │
│       [ IV ]                   [ IV ]                   [ IV ]               │
│         │                        │                        │                   │
│       [ V ]                    [ V ]                    [ V ]                │
│                                                                            │
├──────────────────────────────────────────┬─────────────────────────────────┤
│ progression / prochain Talent Point     │ détail du nœud sélectionné      │
└──────────────────────────────────────────┴─────────────────────────────────┘
```

Principes validés :

- un seul écran et un seul ensemble de widgets pour les six classes ;
- trois branches visibles simultanément ;
- cinq paliers verticaux lisibles sans scrolling obligatoire à 1920×1080 ;
- panneau de détail séparé de l'arbre ;
- états distinguables autrement que par la couleur seule ;
- variante = second niveau de décision, jamais plusieurs nœuds concurrents dans l'arbre principal.

## Les six classes

| Classe | Branche 1 | Branche 2 | Branche 3 |
|---|---|---|---|
| Guerrier | Gardien | Brise-ligne | Maître d'armes |
| Voleur | Assassin | Ombre | Saboteur |
| Rôdeur | Tireur | Chasseur | Éclaireur |
| Mage | Évocateur | Arcaniste | Tisseur de surfaces |
| Prêtre | Restauration | Protection | Exorcisme |
| Alchimiste | Grenadier | Apothicaire | Transmutateur |

UI-RPG01 valide la structure et le vocabulaire. Les palettes, motifs, illustrations et emblèmes définitifs sont du périmètre UI-RPG02.

## États visuels

| Read model | Intention visuelle |
|---|---|
| Acquired | nœud éclairé + liaison active |
| Available | contour/halo d'appel à l'action |
| LockedLevel | désaturé + niveau requis visible |
| LockedPrerequisite | sombre + liaison interrompue |
| LockedPoints | verrouillage lié au budget |
| LockedExclusive | variante sœur déjà résolue |
| Pending | futur anneau/surbrillance UI ; non autoritaire |

## Talent à variantes

```text
          [ NŒUD CONCEPTUEL ]
        Spécialisation martiale
                  │
           « 3 variantes »
                  ▼
        ┌───────────────────┐
        │ Tranchant         │
        │ Perforant         │
        │ Contondant        │
        └───────────────────┘
```

Le nœud affiche l'acquisition globale ; `SelectedChoiceId` permet d'indiquer ensuite la variante concrète retenue.

## Direction artistique commune

La famille visuelle validée est :

- dark fantasy ;
- pierre sombre et métal ;
- accents bronze/or ;
- ornements fins ;
- textes lisibles avant l'effet décoratif ;
- médaillons/runes pour les nœuds ;
- contraste suffisant pour les états ;
- interface conçue comme un écran du jeu, pas comme une infographie externe.

## Livrables graphiques planifiés

La production visuelle est organisée sous `docs/Design/Images/UI-RPG/`.

| Livrable | État à la clôture de UI-RPG01 |
|---|---|
| wireframe écran complet 3×5 | direction validée |
| diagramme Data Contract | direction validée |
| diagramme Runtime Projection | direction validée |
| diagramme Production Validation | direction validée |
| 6 illustrations de classe | UI-RPG02 |
| 18 emblèmes de branche | UI-RPG02 |
| planche des états finaux | UI-RPG02 |
| icônes de talents | production progressive après langage visuel |
| mockups finaux 6 classes | après intégration UI-RPG02/UI-RPG03 |

## Règle de documentation

Une image de conception ne devient **référence canonique** dans les documents que lorsqu'elle est :

1. validée visuellement ;
2. exportée sans texte erroné ;
3. ajoutée dans `docs/Design/Images/UI-RPG/` ;
4. référencée depuis ce document et, si utile, depuis `RPG03_SKILL_TREE_SYNTHESIS.md`.

Cette règle évite qu'une image générée mais non retenue devienne accidentellement une spécification.

## Prochaine étape visuelle

UI-RPG02 doit produire d'abord :

1. une planche de langage visuel des six classes ;
2. les 18 emblèmes de branche ;
3. la spécification de présentation data-driven qui associe ces assets à `ClassId` / `TalentBranchId`.

Le C++ gameplay n'est pas concerné par ces choix artistiques.
