# MON20.4.6 — Recruitment Offer Suppression

## Statut

**VALIDÉ UE5.5.4 — CLOS**  
Date de clôture : **24 août 2026**

MON20.4.6 ferme les deux derniers défauts de comportement observés en PIE après l'intégration du recrutement scénarisé :

1. ne plus afficher la popup lorsqu'un compagnon est déjà recruté ;
2. ne plus répéter automatiquement une offre refusée en repassant sur la même source.

## Décision de gameplay

### Compagnon déjà recruté

`ActiveCharacters` reste l'autorité persistante. Avant d'ouvrir le modal, `OfferRecruitment` vérifie l'identité complète :

```text
CharacterId + RaceId + ClassId
```

Si le compagnon correspondant est déjà actif :

```text
OfferRecruitment
    -> Success silencieux
    -> aucune popup
```

Un GUID identique avec Race/Class différents n'est pas considéré comme `AlreadyActive`; les protections d'`IdentityCollision` existantes restent intactes.

### Refus temporaire

`Refuser` ne devient pas une décision permanente. La suppression est mémorisée uniquement pour la paire runtime :

```text
SourceObjectId + CharacterId
```

Exemple :

```text
Trigger_A -> OfferRecruitment(Edrik)
    -> Refuser

Trigger_A -> OfferRecruitment(Edrik)
    -> supprimé silencieusement

Button_B -> OfferRecruitment(Edrik)
    -> autorisé
```

Une autre scène, un autre bouton, un autre mécanisme ou un autre callback Lua peut donc reproposer plus tard le même compagnon.

## Persistance

La mémoire de refus est volontairement :

- runtime seulement ;
- non `UPROPERTY` ;
- non SaveGame ;
- effacée par `UGridActivationComponent::ResetRuntimeState()`.

Aucune migration SaveGame n'est nécessaire. Après rechargement/réinitialisation du runtime, un compagnon refusé peut être proposé à nouveau. En revanche, un compagnon réellement recruté reste détecté via `ActiveCharacters`, déjà persistant.

## Runtime

`UGridActivationComponent` possède la mémoire interne :

```text
DeclinedStoryCompanionOfferKeys
```

Lors d'un `OfferRecruitment` :

```text
1. définition valide ?
2. PartyPawn valide ?
3. compagnon déjà actif ?
      oui -> Success, aucune popup
4. même SourceObjectId + CharacterId déjà refusé ?
      oui -> Success, aucune popup
5. sinon ouvrir le modal
6. si OnDeclined : mémoriser SourceObjectId + CharacterId
```

Le callback `OnDeclined` du widget existant est réutilisé. Aucune logique Blueprint n'est ajoutée.

## Groupe complet

`PartyFull` n'est pas assimilé à un refus. Le candidat peut rester dans `CharacterPool` / réserve et `IsStoryCompanionAlreadyActive()` ne regarde que `ActiveCharacters`.

## Lua

Aucune API Lua supplémentaire n'est introduite. `grid.command(..., "OfferRecruitment")` conserve le chemin `ApplyLinkCommand()` et bénéficie automatiquement des mêmes règles.

## Validation Automation UE5.5.4 — 24 août 2026

Le filtre complet :

```text
Grimrock.MON20.4.RecruitmentUI
```

a été exécuté avec le résultat :

```text
18 tests / 18 Success
0 Fail
```

Les deux tests MON20.4.6 sont validés :

```text
OfferAlreadyActiveSuppression   Success
OfferDeclineSourceScope         Success
```

Les seize tests précédents restent également verts. Le warning émis par `EventCommandMissingDefinition` est intentionnel et le test correspondant termine en `Success`.

## Validation PIE finale — 24 août 2026

### Refuser

Scénario validé :

```text
1. entrer sur le Trigger
2. popup visible
3. cliquer Refuser
4. sortir de la case
5. revenir sur le même Trigger
```

Résultat : **aucune nouvelle popup** sur la même source. Le refus reste temporaire et source-scoped.

### Recruter

Scénario validé :

```text
1. partir d'un état où le compagnon n'est pas actif
2. entrer sur le Trigger
3. cliquer Recruter
4. sortir de la case
5. revenir sur le même Trigger
```

Résultat : **aucune nouvelle popup** une fois le compagnon présent dans `ActiveCharacters`.

### Focus modal

Le diagnostic UE5 :

```text
InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget
```

a été corrigé puis revalidé en PIE : l'anomalie ne réapparaît plus.

## Conclusion MON20.4

Avec MON20.4.2 à MON20.4.6, le recrutement scénarisé couvre désormais :

```text
URPGStoryCompanionAsset
    -> palette / Grid Editor
    -> Event -> Command OfferRecruitment
    -> WBP_RPGStoryCompanionRecruitment
    -> Voir la fiche / Refuser / Recruter
    -> CharacterPool
    -> ActiveCharacters
```

Le comportement anti-spam et la reconnaissance des compagnons déjà actifs sont validés en PIE.

**MON20.4 — Story Companion Recruitment UI est VALIDÉ ET CLOS.**

## Hors périmètre

MON20.4 n'ajoute pas :

- refus permanent ;
- migration SaveGame dédiée au refus ;
- dialogue général ;
- Actor NPC compagnon ;
- IA/déplacement de compagnon dans le donjon ;
- équipement de départ matérialisé ;
- gestion complète actif/réserve ;
- compétences/talents.

Ces sujets restent destinés aux tranches MON20 suivantes.
