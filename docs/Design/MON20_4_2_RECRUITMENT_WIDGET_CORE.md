# MON20.4.2 — Story Companion Recruitment Widget Core

## Statut

Implémentation C++ du noyau UI de recrutement. Aucun `.uasset` / `.umap` n'est requis dans cette étape.

Validation UE5.5.4 effectuée le 23 août 2026 : **8/8 tests `Grimrock.MON20.4.RecruitmentUI` Success** pour le périmètre MON20.4.2.

## Responsabilités

`URPGStoryCompanionRecruitmentWidget` est une couche de présentation et d'orchestration uniquement.

Il projette `URPGStoryCompanionAsset` dans `FRPGStoryCompanionRecruitmentView`, puis délègue toute mutation aux services existants :

```text
Recruter
  -> FRPGStoryCompanionService::EnsureCandidateRegistered()
  -> FRPGPartyRecruitmentService::TryRecruitFromPool()
```

Le widget ne modifie jamais directement `FGridPartyInventoryState`.

## États UI

```text
Uninitialized
Ready
Recruited
AlreadyActive
PartyFull
Declined
Invalid
Failed
```

`AlreadyActive` est idempotent : aucun deuxième membre n'est ajouté.

`PartyFull` préserve le candidat éventuellement créé dans `CharacterPool`; seul `ActiveCharacters` doit rester inchangé. Il n'existe volontairement aucun rollback UI parallèle de MON20.3.

## Présentation

La vue expose :

- identité stable (`CompanionId`, `CharacterId`) ;
- nom, description, race, classe et niveau ;
- portrait, silhouette complète et icône de classe ;
- texte de condition de recrutement ;
- état, message de statut et disponibilité du bouton Recruter ;
- indicateur de candidat déjà présent dans `CharacterPool` ;
- état d'affichage de la fiche détaillée.

Le WBP futur pourra utiliser les bindings optionnels :

```text
Button_Recruit
Button_Decline
Button_ShowDetails
Text_Name
Text_Identity
Text_Description
Text_Status
Text_Details
Text_ShowDetailsAction
Image_Portrait
Image_FullBody
Image_ClassIcon
Panel_Details
```

La logique des boutons reste en C++.

## Modal

Le widget reprend le garde d'entrée utilisé par le Level Up :

```text
SetInventoryUiOpen(true)
DisableInput()
Pause si nécessaire
FInputModeUIOnly
curseur visible
```

À la fermeture, l'état précédent est restauré et le jeu n'est dépausé que si ce modal l'avait lui-même mis en pause.

## Fallback Slate

En l'absence de Widget Blueprint, `RebuildWidget()` construit une interface native minimale et fonctionnelle avec :

- nom ;
- race / classe / niveau ;
- description ;
- bouton Voir/Masquer la fiche ;
- message de statut ;
- boutons Recruter / Refuser.

Cela permet de compiler et tester MON20.4.2 avant toute création d'asset UMG.

## Delegates natifs

```text
OnAccepted
OnDeclined
OnClosed
```

`OnAccepted` n'est émis qu'après un nouveau recrutement effectivement commité par MON20.2.

## Tests

Filtre :

```text
Grimrock.MON20.4.RecruitmentUI
```

Cas MON20.4.2 validés :

```text
ViewProjection
NominalRecruitment
AlreadyInPool
AlreadyActiveNoDoubleRecruitment
PartyFull
IdentityCollision
InvalidDefinition
DeclineNoMutation
```

Résultat UE5.5.4 fourni après exécution locale : **8/8 Success**.

## Hors périmètre

MON20.4.2 n'ajoute pas encore :

- raccord `Event -> Command` ;
- nouveau `EGridLevelObjectType` compagnon ;
- commande `OfferRecruitment` ;
- placement d'un compagnon dans le Grid Editor ;
- Widget Blueprint de production.

Ces points appartiennent aux étapes suivantes de MON20.4.
