# UI-RPG-DESC01.17 — Documentation & Final Closure

Date : **9 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **VALIDÉ / CLOS**  
Périmètre : **fiche Talent — 6 classes / 18 branches / 90 Talents**

## 1. Objet

DESC01.17 clôt officiellement le chantier UI-RPG-DESC01.

Le chantier a commencé par l'audit de la fiche Talent historique, a défini un
contrat joueur unique, a audité les 90 Talents, a construit le read-model
canonique, a reconstruit la fiche UMG, a supprimé les chemins legacy puis a
validé les six classes en Automation et en PIE.

Aucune nouvelle couche architecturale n'est introduite par DESC01.17.

## 2. État final du domaine Talent

```text
6 classes
18 branches
90 Talents conceptuels
├── 86 Talents simples
└── 4 nœuds à variantes exclusives
    ├── Guerrier — Spécialisation martiale
    ├── Rôdeur — Ennemi juré
    ├── Mage — Affinité élémentaire
    └── Mage — Imprégnation
```

Les recettes multiples d'un Talent ne constituent jamais des variantes.

## 3. Contrat final de la fiche

Ordre canonique :

```text
NOM
TYPE
STATUT
PRINCIPE
EFFETS
[UTILISATION]
[VARIANTES]
ACQUISITION
```

Invariants :

- consulter un Talent n'acquiert rien ;
- TYPE décrit la nature mécanique ;
- STATUT décrit uniquement l'acquisition ;
- les informations gameplay sont visibles avant acquisition ;
- UTILISATION n'existe que pour un déclenchement volontaire ;
- VARIANTES n'existe que sur les quatre vrais nœuds multi-choix ;
- toutes les variantes restent visibles simultanément ;
- ACQUISITION est toujours la dernière section ;
- un seul scroll vertical porte toute la fiche ;
- aucun ComboBox de variante ;
- aucun identifiant technique n'est présenté comme texte joueur ;
- le panneau est vide tant qu'aucun Talent n'a été consulté.

## 4. Taxonomie finale

TYPE autorisés :

```text
ACTIF
SORT ACTIF
PASSIF
RÉACTION AUTOMATIQUE
RECETTE + OBJET RAPIDE
RECETTE + ACTIF
```

STATUT autorisés :

```text
ACQUIS
DISPONIBLE
VERROUILLÉ — niveau X requis
VERROUILLÉ — nécessite « Talent X »
VERROUILLÉ — nécessite N point(s) de Talent
INDISPONIBLE — autre variante déjà choisie
```

La disponibilité instantanée d'une action en combat n'est jamais un STATUT
Talent.

## 5. Acquisition des variantes

Flux final :

```text
CHOISIR
    -> CHOIX EN COURS
    -> ANNULER
       ou
    -> CONFIRMER
        -> Talent conceptuel : ACQUIS
        -> variante choisie : ACQUISE
        -> variantes sœurs : INDISPONIBLE
```

La transaction reste autoritaire. L'UMG ne fabrique aucun état d'acquisition.

## 6. Langage joueur et recettes

DESC01.16.4 a supprimé du texte joueur les formulations techniques restantes
telles que `Accuracy`, `AoE`, `DoT`, `ArmorGate`,
`InitiativeModifier`, `ActionPointCost`, `AreaRadius`,
`SourcePolicy`, `Evasion`, `tick`, etc.

DESC01.16.4B fixe la règle suivante :

> Un ID `Recipe_*` est une identité gameplay et n'est jamais une source de nom
> joueur.

Les 15 recettes de production de l'Alchimiste sont projetées sous des noms
canoniques français via `GrantedRecipeNames` dans ACQUISITION. Elles sont
réparties sur 9 Talents.

La projection courante utilise des clés `NSLOCTEXT` stables pour ces noms de
recettes. Cela prépare une future localisation sans créer maintenant un nouveau
système transversal.

## 7. Architecture finale

```text
URPGClassAsset / ProgressionChoices
    -> autorité identité + mécanique

FRPGClassProgressionService
FRPGClassProgressionTransactionService
    -> disponibilité / acquisition

CombatActions / modifiers / reactions / skills / party modifiers
    -> valeurs gameplay

FGridSkillsPageService
    -> read-model Talent canonique read-only

UGridTalentDetailWidget
UGridTalentVariantBlockWidget
    -> presenter natif

WBP_RPGTalentDetail
WBP_RPGTalentVariantBlock
    -> composition et rendu UMG
```

DESC01.15.5 a supprimé les champs, bindings et chemins de projection legacy.
Aucune double autorité Talent n'est conservée pour compatibilité.

## 8. Validation finale

Validation locale rapportée par l'utilisateur le 9 octobre 2026 :

```text
Grimrock.UI.RPG.DESC01.Detail.EmptyInitialState   1/1
Grimrock.UI.RPG.DESC01.QA16                      4/4
Grimrock.UI.RPG.DESC01                          19/19
Succeeded with warnings                            0
Failed                                             0
PIE six classes                                  validé
PIE recettes                                     validé
```

Le contrôle PIE a couvert les six classes et l'utilisateur a passé en revue tous
les Talents. La cohérence générale de la fiche a été validée.

Les textes français matérialisés dans les DataAssets ont été poussés dans :

```text
06fa15a9 UI-RPG-DESC01.16.4 materialize French Talent text
```

## 9. Dettes et travaux différés

Ne pas rouvrir DESC01 pour ces sujets :

- **D02 — Désamorçage / Crochetage** : différé au chantier Lock/Trap ;
- **D03 — Sabotage monde** : différé aux actions hors combat ;
- **D09 — Crafting** : futur système Recipes/Crafting ;
- **UI-RPG-VISUAL01** : style final, couleurs, icônes, typographie et polish ;
- **LOC01 — Localization Foundation** : future infrastructure multilingue.

LOC01 n'est pas une dette de DESC01. Il devra préserver les IDs gameplay stables
(`Talent_*`, `Recipe_*`, `Status_*`, `Item_*`) et déplacer les textes
joueur vers les mécanismes de localisation Unreal : `FText`, clés stables,
String Tables lorsque pertinentes, Localization Dashboard et ressources
localisées.

## 10. Critère de non-réouverture

UI-RPG-DESC01 ne doit être rouvert que si un défaut remet en cause l'un de ses
contrats structurants :

- séparation consultation / TYPE / STATUT ;
- ordre canonique des sections ;
- autorité du read-model ;
- sémantique des quatre familles à variantes ;
- transaction d'acquisition ;
- absence de double autorité ou de chemin legacy.

Un défaut découvert pendant une vraie partie doit recevoir un nouveau ticket
fonctionnel ou de QA s'il ne viole pas ces contrats.

## 11. Suite recommandée

DESC01 étant clos, la prochaine étape utile n'est plus une inspection statique
des Talents mais un **playtest vertical réel** : démarrage d'une partie, création
du groupe, chargement du donjon, exploration, inventaire/hotbar, interactions,
combat, progression et acquisition de Talents dans une même session.

Le but est désormais de découvrir les défauts d'intégration et de rythme qui
n'apparaissent pas dans les tests isolés.

**UI-RPG-DESC01 : CLOS.**


## 12. État post-clôture — UI-RPG-CODE-AUDIT01

Après la clôture DESC01, l'audit de code a supprimé la dernière compatibilité
Talent plate qui ne faisait pas partie du contrat de détail final :

```text
FRPGTalentRuntimeService              supprimé
FGridTalentEntryView                  supprimé
FGridSkillsPageView::Talents          supprimé
GetTalentEntryCount / GetTalentEntry  supprimés
```

`WBP_GridSkills` a été recompilé par Automation après suppression et les
régressions DESC01 19/19, MON20.8 8/8, MON15.4 7/7 ainsi que le PIE six classes
sont restés verts.

Le contrat sémantique de DESC01.17 est inchangé ; cette note confirme simplement
que la promesse « aucune double autorité / aucun chemin legacy » est désormais
également vraie pour la projection C++ plate historique.
