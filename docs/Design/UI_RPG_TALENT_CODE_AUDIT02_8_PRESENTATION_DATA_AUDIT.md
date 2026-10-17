# UI-RPG-TALENT-CODE-AUDIT02.8 — Dead/obsolete Talent presentation data audit

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Objet

Auditer `URPGTalentPresentationAsset` après la finalisation de
`URPGClassVisualAsset`, afin de supprimer les anciennes données de présentation
qui n'ont plus d'autorité ni de consommateur.

Aucun `.uasset` n'est modifié par ce ticket.

## Classement

### Vivant

```text
FRPGClassPresentationDefinition
    ClassId
    AccentColor
    Branches[3]

FRPGTalentBranchPresentationDefinition
    TalentBranchId
    DisplayName
    AccentColor
    NodePresentationOverrides[]

FRPGTalentNodePresentationDefinition
    TalentNodeId
    DisplayName
    Description
```

Usages :

- `ClassId` : résolution du catalogue ;
- `Class.AccentColor` : couleur du libellé classe/niveau ;
- `Branches[3]` : ordre gauche / centre / droite ;
- `Branch.DisplayName` : titre de branche ;
- `Branch.AccentColor` : branche + panneau détail ;
- `NodePresentationOverrides` : identité/principe des quatre familles à variantes.

### Intentionnel mais non affiché aujourd'hui

```text
Branch.ShortDescription
```

Ce champ est conservé parce qu'il fait partie de l'autorité de texte sérialisée
et que `FUIRPGTalentPresentationAuthoring::ConfigureCatalog()` le préserve
explicitement par `TalentBranchId`. Le supprimer dans un simple cleanup de
code ferait perdre un contenu authoré utile à une future présentation de
branche.

### Obsolète supprimé

```text
Class.PortraitTexture
Class.ClassEmblemTexture
Class.PrimaryColor
Class.SecondaryColor
Class.GlowColor
Class.MotifTextures
Branch.BranchEmblemTexture
```

Constats :

- aucun consommateur runtime ;
- aucun impact sur `IsValidDefinition()` ;
- `PortraitTexture`, `ClassEmblemTexture`, `MotifTextures` et
  `BranchEmblemTexture` ne sont plus alimentés par l'authoring ;
- `PrimaryColor`, `SecondaryColor` et `GlowColor` étaient encore écrits
  par l'authoring mais jamais lus ;
- la production visuelle finalisée utilise `URPGClassVisualAsset`.

## Autorités après cleanup

```text
DA_RPGTalentPresentation
    -> ordre de branches
    -> texte de branches
    -> accents classe / branche
    -> overrides conceptuels des variantes

DA_ClassVisual_*
    -> ClassIcon
    -> Banner
    -> Flag
    -> TalentBranchVisuals
       -> Background / DetailPanelImage
    -> TalentNodeVisuals
    -> TalentVariantVisuals
```

## Authoring

`MakeClass()` ne reçoit plus quatre couleurs dont trois étaient mortes.

Avant :

```text
Primary
Secondary
Accent
Glow
```

Après :

```text
Accent
```

Les six valeurs `Accent` existantes sont conservées à l'identique.

## Compatibilité sérialisée

Les propriétés supprimées étaient optionnelles et hors validation structurelle.
UE peut ignorer les anciennes données sérialisées correspondant à des propriétés
qui n'existent plus dans le type courant. Le ticket ne resave aucun
`DA_RPGTalentPresentation`.

La compatibilité Blueprint doit néanmoins être vérifiée localement avec le test
de recompilation des WBP Talent, car ces propriétés étaient réfléchies
`BlueprintReadOnly`.

## Régressions 02.8

Le filtre `Grimrock.UI.RPG.CODEAUDIT02` ajoute :

```text
TalentPresentationDataAudit.ReflectedContract
TalentPresentationDataAudit.SourceOwnership
```

Le premier verrouille le contrat réfléchi courant ; le second vérifie que les
anciens champs ne reviennent ni dans le contrat ni dans l'authoring, tout en
confirmant l'autorité de `URPGClassVisualAsset`.
