# UI-LOCALIZATION01.11A — Recruitment error boundary

> **Statut : CLOSED.** Migration : `d98bd006e148a240b7130d3ff988dfb30b1de45c`. Matérialisation 01.11B : `e8e987659b8bc3e400470107c1065fd687bc1162`.


Date : **13 octobre 2026**

## Objet

Empêcher les diagnostics techniques des services de recrutement d'être
affichés directement au joueur.

Avant ce ticket :

~~~text
RPGStoryCompanionService / RPGPartyRecruitmentService
    -> FString Error en anglais
    -> RPGStoryCompanionRecruitmentWidget
    -> FText::FromString(Error)
    -> joueur
~~~

Après ce ticket :

~~~text
service
    -> enum stable + FString Error diagnostic
    -> widget
         enum -> Recruitment.* -> FText joueur
         Error -> UE_LOG uniquement
~~~

Aucune nouvelle couche métier n'est créée.

## Autorités conservées

Les services restent responsables :

- de la décision transactionnelle ;
- de `ERPGStoryCompanionRegistrationStatus` ;
- de `ERPGPartyRecruitmentRejectReason` ;
- du diagnostic technique détaillé `FString Error`.

Le widget reste responsable du texte joueur.

Le champ `Error` n'est volontairement ni supprimé ni renommé afin de ne pas
casser les diagnostics, tests et outils techniques existants.

## Catalogue joueur

Le ticket ajoute 15 clés `Recruitment.*`.

### Préparation du compagnon

- `Recruitment.Registration.InvalidInventory`
- `Recruitment.Registration.InvalidDefinition`
- `Recruitment.Registration.IdentityCollision`
- `Recruitment.Registration.Failed`

### Activation dans le groupe

- `Recruitment.Activation.InvalidInventory`
- `Recruitment.Activation.InitialCharacterMissing`
- `Recruitment.Activation.InvalidPartyState`
- `Recruitment.Activation.InvalidCharacterId`
- `Recruitment.Activation.CandidateNotFound`
- `Recruitment.Activation.AmbiguousCandidate`
- `Recruitment.Activation.DuplicateActiveCharacter`
- `Recruitment.Activation.PartyFull`
- `Recruitment.Activation.InvalidCandidate`
- `Recruitment.Activation.OwnershipValidationFailed`
- `Recruitment.Activation.Failed`

Le cas `PartyFull`, auparavant un `LOCTEXT` spécial dans le widget, utilise
maintenant la même autorité `Recruitment.*` que les autres rejets.

## Diagnostics

Sur rejet, le widget journalise explicitement :

~~~text
registration: Status + Error
recruitment : RejectReason + Error
~~~

Le diagnostic anglais reste donc disponible pour le développeur sans franchir
la frontière de présentation.

## Garde-fou

Le test :

~~~text
Game.UI.Localization.LOCALIZATION01.RecruitmentErrorBoundary
~~~

vérifie :

- présence des 15 clés ;
- projection exacte des 15 clés par le widget ;
- absence de `FText::FromString(Error)` ;
- absence d'un paramètre `FString Error` dans les mappers de présentation ;
- conservation des diagnostics de service ;
- journalisation du détail technique ;
- couverture du fixture anglais et du smoke runtime.

## 01.11B

Après validation source :

1. régénérer `ST_UI.uasset` ;
2. Gather / Export / Import / Compile ;
3. valider le smoke fr/en ;
4. exécuter les régressions MON20.2, MON20.3 et MON20.4 RecruitmentUI ;
5. versionner uniquement les projections générées.
