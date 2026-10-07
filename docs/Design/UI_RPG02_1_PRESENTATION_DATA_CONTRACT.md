# UI-RPG02.1 — Presentation Data Contract

Date : **7 octobre 2026**  
Parent : **UI-RPG02 — Visual Language & Presentation Data**  
État : **SOURCE PRÊTE — validation locale UE5.5.4 en attente**

## Pourquoi ce ticket existe

UI-RPG01 a livré l'arbre gameplay/read-only complet. UI-RPG02.1 ajoute uniquement les données nécessaires pour lui donner une identité visuelle data-driven.

Aucun écran WBP final n'est créé dans ce ticket. Le vrai remplacement de la présentation native provisoire de `WBP_GridSkills` commence en **UI-RPG03.1**.

Roadmap resserrée :

```text
UI-RPG02.1  Presentation Data Contract
UI-RPG02.2  Minimal Production Presentation Catalog
UI-RPG03.1  REAL WBP_GridSkills Talent Tree shell
UI-RPG03.2  Branch / Node / Detail reusable widgets
UI-RPG03.3  Full read-model binding + remove native renderer
```

Les illustrations et emblèmes finaux pourront continuer à être produits en parallèle ; ils ne bloquent pas la construction UMG grâce aux fallbacks visuels.

## Simplification retenue

La proposition initiale séparait :

```text
Classes[]
Branches[]
OwningClassId
SortIndex
OrderedBranchIds[]
```

Cette structure dupliquait ownership et ordre.

Le contrat final est plus simple :

```text
URPGTalentPresentationAsset
└── Classes[]
    └── FRPGClassPresentationDefinition
        └── Branches[3]
```

L'ordre du tableau `Branches` est directement :

```text
index 0 = gauche
index 1 = centre
index 2 = droite
```

Il n'existe donc ni `OwningClassId`, ni `SortIndex`, ni `OrderedBranchIds`.

## Contrat

### FRPGClassPresentationDefinition

```text
ClassId
PortraitTexture
ClassEmblemTexture
PrimaryColor
SecondaryColor
AccentColor
GlowColor
MotifTextures[]
Branches[3]
```

### FRPGTalentBranchPresentationDefinition

```text
TalentBranchId
DisplayName
ShortDescription
BranchEmblemTexture
AccentColor
```

## Frontière d'autorité

La présentation ne contient jamais :

```text
PointCost
MinimumLevel
Prerequisites
SelectedChoiceId
Acquired
Available
transactions
combat effects
```

Ces informations restent dans les autorités RPG03/UI-RPG01.

## Direction visuelle validée

La planche UI-RPG02 validée fixe provisoirement les familles suivantes :

| Classe | Direction |
|---|---|
| Guerrier | acier, bronze, rouge sombre |
| Voleur | anthracite, argent, violet désaturé |
| Rôdeur | vert mousse, cuir, ambre |
| Mage | bleu nuit, cyan, violet |
| Prêtre | ivoire, or pâle, bleu clair |
| Alchimiste | cuivre, ambre, vert alchimique |

Branches :

```text
Guerrier   : Gardien / Brise-ligne / Maître d'armes
Voleur     : Assassin / Ombre / Saboteur
Rôdeur     : Tireur / Chasseur / Éclaireur
Mage       : Évocateur / Arcaniste / Tisseur de surfaces
Prêtre     : Restauration / Protection / Exorcisme
Alchimiste : Grenadier / Apothicaire / Transmutateur
```

## Textures

Les références utilisent des `TSoftObjectPtr<UTexture2D>`.

Elles sont volontairement facultatives dans `IsValidDefinition()` pendant UI-RPG02.1 : le contrat peut être validé avant que les 24+ assets graphiques finaux aient été importés.

UI-RPG02.2 imposera la couverture structurelle de production. Les validations de contenu graphique final pourront être renforcées progressivement sans bloquer le WBP.

## Tests

Filtre :

```text
Grimrock.UI.RPG02.PresentationData
```

Tests source :

```text
ValidSixClassCatalog
LeftCenterRightOrder
RejectDuplicateClass
RejectDuplicateBranch
RequireExactlyThreeBranches
```

## Critères de fermeture

```text
5/5 tests
0 warning
0 failed
```

Après validation, UI-RPG02.2 matérialisera un catalogue minimal réel couvrant 6 classes / 18 branches. Ensuite commence immédiatement UI-RPG03.1 et le vrai travail WBP.
