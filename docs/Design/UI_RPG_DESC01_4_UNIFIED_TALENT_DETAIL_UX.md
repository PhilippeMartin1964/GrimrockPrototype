# UI-RPG-DESC01.4 — Contrat unifié des Talents

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Périmètre : **les 6 classes / 18 branches / 90 nœuds conceptuels**

## Statut historique

La validation Automation de DESC01.4 reste utile comme régression technique, mais la validation PIE a rejeté son contrat UX : confusion entre focus de consultation, acquisition et nature mécanique ; vocabulaire d'action ambigu ; hiérarchie variable selon les Talents. Le nouveau contrat est `docs/Design/UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`.

## Problème corrigé

Le panneau historique mélangeait trois notions :

1. **consulter** un Talent dans l'arbre ;
2. **prévisualiser** une variante ;
3. **acquérir** réellement un Choice avec un point de Talent.

Cela produisait trois ambiguïtés visibles :

- une ComboBox était nécessaire pour comprendre un Talent à variantes ;
- le titre « ACTION DÉBLOQUÉE » était affiché avant acquisition ;
- la structure visuelle changeait entre Talent passif, réaction, capacité active
  et Talent à variantes.

UI-RPG-DESC01.4 fixe un contrat unique.

## Vocabulaire canonique

**Talent consulté** : nœud sur lequel le joueur vient de cliquer.  
Aucun point n'est dépensé et aucun effet gameplay n'est accordé.

**Talent disponible** : le personnage satisfait niveau, prérequis, exclusivité et
points nécessaires. Il peut être acquis, mais ne l'est pas encore.

**Talent acquis** : le Choice concret est enregistré par l'autorité de progression.

**Variante** : Choice concret exclusif appartenant à un même TalentNodeId conceptuel.
Consulter un Talent à variantes ne sélectionne aucune variante.

**Variante candidate** : choix temporaire effectué uniquement pendant le flux
d'acquisition. Elle n'est persistée qu'après confirmation réussie.

## Autorités

```text
FRPGClassProgressionChoiceDefinition
    -> DisplayName / Description
    -> CombatModifiers
    -> CombatReactions
    -> SkillModifiers
    -> PartyModifiers
    -> FirstRoundInitiativeModifier
    -> GrantedRequirementIds

URPGClassAsset::CombatActions
    -> action réelle
    -> PA / Mana / portée / cible / zone / cooldown
    -> description mécanique

FRPGClassProgressionService
    -> état Locked / Available / Acquired

FGridSkillsPageService
    -> projection read-only

UGridTalentDetailWidget
    -> mise en forme uniquement
```

UMG ne calcule aucune règle et ne possède aucune valeur numérique gameplay.

## Structure obligatoire de la fiche

L'ordre est identique pour tous les Talents :

```text
NOM

TYPE
<type du Talent>

FONCTIONNEMENT
<explication joueur>

Niveau requis / Coût
État / Prérequis

[VARIANTES]
<toutes les variantes, simultanément>

[ACTION]
<action accordée après acquisition ou action déjà disponible>
```

Les sections inexistantes sont masquées, mais l'ordre et le vocabulaire ne changent
jamais.

## Types affichés

Les catégories décrivent la **nature** du Talent et non son état d'acquisition :

```text
CAPACITÉ ACTIVE
CAPACITÉ ACTIVE + BONUS PASSIF
CAPACITÉ ACTIVE + RÉACTION AUTOMATIQUE
BONUS PASSIF
RÉACTION AUTOMATIQUE
RECETTE
TALENT
CHOIX DE VARIANTE
```

« CAPACITÉ DÉBLOQUÉE » est abandonné : ce texte confondait type et état.

## Actions

Une action associée est toujours montrée afin que le joueur puisse décider avant
de dépenser son point.

Avant acquisition :

```text
ACTION ACCORDÉE APRÈS ACQUISITION
```

Après acquisition :

```text
ACTION DISPONIBLE
```

Ainsi, afficher une action n'affirme jamais qu'elle est déjà utilisable.

Les valeurs viennent exclusivement de `FGridCombatActionDefinition`.

## Talents à variantes

La consultation ne nécessite plus de ComboBox.

Dès le clic sur le Talent :

```text
VARIANTES

Tranchant
Type : BONUS PASSIF
Fonctionnement : ...
Effets : ...

Perforant
Type : BONUS PASSIF
Fonctionnement : ...
Effets : ...

Contondant
Type : BONUS PASSIF
Fonctionnement : ...
Effets : ...
```

Une variante déjà acquise porte le suffixe :

```text
— CHOISIE
```

Pendant une acquisition en cours seulement, la candidate porte :

```text
— SÉLECTIONNÉE
```

La `Combo_VariantChoice` est **Collapsed** hors de ce flux. Elle devient visible
uniquement après `Button_ChooseVariant`.

## Couverture des six classes

| Classe | Branches | Nœud(s) à variantes |
|---|---|---|
| Guerrier | Gardien / Brise-ligne / Maître d'armes | Spécialisation martiale : Tranchant / Perforant / Contondant |
| Voleur | Assassin / Ombre / Saboteur | aucun |
| Rôdeur | Tireur / Chasseur / Éclaireur | Ennemi juré : catégories issues du bestiaire |
| Mage | Évocateur / Arcaniste / Tisseur de surfaces | Affinité élémentaire ; Imprégnation |
| Prêtre | Restauration / Protection / Exorcisme | aucun |
| Alchimiste | Grenadier / Apothicaire / Transmutateur | aucun nœud Talent exclusif ; les variantes d'objets restent des recettes/items |

Le nombre de Choice records concrets peut dépasser 90 à cause des variantes.
L'invariant UI reste **90 nœuds conceptuels**.

## Langage joueur

Le panneau normalise les principaux termes techniques avant affichage :

```text
Accuracy           -> Précision
PhysicalArmor      -> armure physique
MagicalArmor       -> armure magique
RawDamage          -> dégâts bruts
MaxHP              -> PV maximum
Status_Stunned     -> Étourdi
Status_Burning     -> Brûlure
Status_Immobilized -> Immobilisé
```

Les préfixes internes `Status_`, `Skill_`, `Action_`, `Recipe_`, `Item_`
et `Surface_` ne doivent pas être exposés comme tels au joueur.

La documentation exhaustive des mécaniques des 90 Talents reste :

```text
docs/Rules/RPG_Talents_Mechanics_v0_1.md
```

Ce document est la référence de mécanique ; DESC01.4 est la référence de
présentation et de vocabulaire UI.

## États

```text
Acquired           -> État : acquis
Available          -> État : disponible
LockedLevel        -> État : niveau insuffisant
LockedPrerequisite -> Prérequis manquant : <Talent>
LockedPoints       -> État : points de talent insuffisants
LockedExclusive    -> État : choix exclusif déjà effectué
```

Un état ne change jamais la quantité d'informations affichées sur la mécanique.

## Contrat UMG existant

Aucun nouveau widget binaire n'est requis.

```text
Text_DetailName
Text_DetailDescription
Text_DetailLevel
Text_DetailCost
Text_DetailState
Text_DetailVariants
Text_DetailVariantName
Text_DetailVariantDescription
Text_DetailActionSummary

Button_AcquireTalent
Button_ChooseVariant
Combo_VariantChoice
Button_ConfirmAcquire
Button_CancelAcquire
```

`Text_DetailVariantName` devient un titre stable « VARIANTES ».  
`Text_DetailVariantDescription` contient l'ensemble des variantes.  
`Text_DetailActionSummary` reste réservé au Talent simple.

## Invariants de sécurité

- consulter ne dépense aucun point ;
- consulter ne change aucun ChoiceId ;
- une variante verrouillée reste entièrement lisible ;
- une variante ne peut être sélectionnée que pendant un flux d'acquisition valide ;
- `ConfirmAcquire()` exige toujours `bAvailable && !bSelected` ;
- les données numériques restent autoritaires en C++/DataAssets ;
- aucun Event Graph UMG n'acquiert un Talent directement.

## Validation

Filtre principal :

```text
Grimrock.UI.RPG.DESC01
```

Le filtre couvre notamment :

- projection d'action ;
- détail simple ;
- toutes les variantes visibles sans interaction ;
- variante acquise visible sans masquer les autres ;
- variante verrouillée lisible ;
- catégories d'effets ;
- résumé d'action structuré ;
- réactions ;
- langage joueur unifié ;
- descriptions d'authoring des 90 nœuds conceptuels.

La validation PIE doit vérifier au minimum :

1. Guerrier / Spécialisation martiale ;
2. Guerrier / Riposte ;
3. Guerrier / Coup de bouclier ;
4. Rôdeur / Ennemi juré ;
5. Mage / Affinité élémentaire ;
6. Mage / Imprégnation ;
7. un Talent du Voleur ;
8. un Talent du Prêtre ;
9. un Talent de l'Alchimiste.

Aucune icône UI-RPG-VISUAL01 ne fait partie de ce ticket.
