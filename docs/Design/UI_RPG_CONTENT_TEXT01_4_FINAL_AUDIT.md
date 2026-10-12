# UI-RPG-CONTENT-TEXT01.4 — Final text ownership audit and residual authority cleanup

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT01.4**

## 1. Objet

Clore le chantier CONTENT-TEXT01 après le transfert d'autorité des textes
`DA_Class_*` hors des six `RPG*Authoring.cpp`.

Ce ticket audite et nettoie les dernières dépendances de présentation Talent
qui pouvaient encore reprendre une autorité textuelle en C++.

## 2. Autorités finales

```text
DA_Class_*
    -> DisplayName / Description de classe
    -> Choice.DisplayName / Choice.Description
    -> CombatAction.DisplayName / CombatAction.Description

DA_RPGTalentPresentation
    -> DisplayName / ShortDescription des 18 branches
    -> DisplayName / Description des 4 nœuds conceptuels groupés
    -> ordre des branches et données visuelles de présentation

DA_MONCAT_*
    -> DisplayName canonique des catégories de monstres

DA_Status_*
    -> texte des statuts

DA_Item_*
    -> texte des items et actions QuickItem associées

DA_RPGTalentDetailTextSet
    -> vocabulaire UI générique Talent

C++
    -> IDs stables
    -> mécanique
    -> read-model
    -> structure / couleurs
    -> bootstrap technique uniquement pour un nouvel ID
```

## 3. DA_RPGTalentPresentation

Avant TEXT01.4, `FUIRPGTalentPresentationAuthoring::ConfigureCatalog()`
réinitialisait le catalogue et reconstruisait encore en C++ :

- les 18 noms de branches ;
- les 4 noms de nœuds conceptuels groupés ;
- les 4 descriptions de ces nœuds.

Après TEXT01.4, le C++ ne contient plus cette prose joueur.

Le rematérialiseur :

1. mémorise les textes sérialisés existants ;
2. reconstruit les six classes, les dix-huit branches, les quatre overrides,
   leur ordre et leurs couleurs ;
3. restaure les textes par `ClassId / TalentBranchId / TalentNodeId`.

Un nouvel ID sans texte sérialisé reçoit uniquement :

```text
DisplayName = StableId
Description = AUTHOR_TEXT_REQUIRED:<StableId>
```

Pour une branche, le marqueur est stocké dans `ShortDescription`.

## 4. Variantes Talent

`UGridTalentDetailWidget` ne découpe plus le `DisplayName` joueur pour
fabriquer un libellé compact.

L'ancien comportement :

```text
"Spécialisation martiale — Tranchant"
    -> "Tranchant"
```

analysait un texte localisable.

Le widget transmet maintenant le `Variant.DisplayName` authoré tel quel.
Aucune règle UI ne dépend donc plus d'un séparateur, d'un préfixe français ou
du texte d'un nœud conceptuel.

## 5. Ranger / Favored Enemy

La frontière d'autorité est désormais explicite :

- `DA_MONCAT_*` possède le nom de la catégorie de monstre ;
- `DA_Class_Ranger` possède le texte complet de chaque variante de Talent ;
- le C++ possède les `CategoryId`, les filtres et les modificateurs ;
- aucun writer C++ de `ConfigureClass()` ne reconstruit le libellé
  `Ennemi juré — ...`.

Les tests de production peuvent vérifier la cohérence entre ces deux contenus
sérialisés sans faire du C++ une troisième autorité.

## 6. Commandlets

`UIRPGTalentTreeAuthoringCommandlet` reste autorisé à rematérialiser les six
`DA_Class_*` car TEXT01.2/01.3 préservent leurs textes par ID.

`UIRPGPresentationAuthoringCommandlet` reste autorisé à rematérialiser
`DA_RPGTalentPresentation` car TEXT01.4 préserve désormais les textes de
branches et de nœuds conceptuels par ID.

Aucun de ces chemins ne doit réinjecter de prose joueur C++ dans un
enregistrement existant.

## 7. Validation automatisée

Le filtre :

```text
Grimrock.UI.RPG.CONTENTTEXT01
```

contient désormais quatre tests :

```text
OwnershipAudit
Preservation
BootstrapOnly
FinalAudit
```

`FinalAudit` couvre notamment :

- le chargement du `DA_RPGTalentPresentation` sérialisé ;
- 18 branches ;
- 4 overrides conceptuels ;
- la préservation de sentinelles texte lors d'une rematérialisation ;
- le bootstrap technique d'un catalogue neuf.

La régression dédiée aux variantes vérifie également que le texte authoré est
retourné sans parsing.

## 8. Hors périmètre restant

Le problème identifié dans l'authoring Alchemist des `DA_Item_*` reste
volontairement hors CONTENT-TEXT01 : certaines routines d'authoring d'items
peuvent encore remettre à zéro une `Description` d'item.

Ce point concerne l'autorité texte des items et doit être traité dans un
ticket atomique distinct.

Aucun `.uasset` n'est modifié par TEXT01.4.
