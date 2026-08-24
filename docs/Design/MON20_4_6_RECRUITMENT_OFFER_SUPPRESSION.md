# MON20.4.6 — Recruitment Offer Suppression

## Statut

Implémenté côté source — compilation, Automation Tests et PIE UE5.5.4 à valider.

## Problème observé en PIE

Les scénarios MON20.4.5 ont validé le pipeline réel :

```text
Trigger.Activated
    -> StoryCompanion.OfferRecruitment
    -> WBP_RPGStoryCompanionRecruitment
```

Deux comportements restaient indésirables lorsqu'on repassait ensuite sur le même Trigger :

1. après un recrutement réussi, la popup revenait alors que le compagnon était déjà actif ;
2. après `Refuser`, la popup revenait à chaque nouveau passage sur le Trigger.

Le premier cas est inutile. Le second transforme un refus temporaire en spam d'interface.

## Décision de gameplay

Les deux cas sont volontairement distingués.

### Compagnon déjà recruté

`ActiveCharacters` reste l'autorité persistante.

Avant d'ouvrir le modal, `OfferRecruitment` vérifie l'identité complète :

```text
CharacterId + RaceId + ClassId
```

Si le compagnon correspondant est déjà actif :

```text
OfferRecruitment
    -> Success silencieux
    -> aucune popup
```

La commande est considérée comme traitée avec succès afin qu'un lien valide ne soit pas diagnostiqué en erreur uniquement parce que son effet est désormais déjà satisfait.

Un simple GUID identique avec Race/Class différents n'est pas considéré comme `AlreadyActive`; les protections d'IdentityCollision existantes restent donc intactes.

### Refus temporaire

`Refuser` ne devient pas une décision permanente.

La suppression est mémorisée uniquement pour la paire runtime :

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

Cela permet à une autre scène, un autre bouton, un autre mécanisme ou un autre callback Lua de reproposer plus tard le même compagnon sans introduire un refus définitif.

## Persistance

La mémoire de refus est volontairement :

- runtime seulement ;
- non `UPROPERTY` ;
- non SaveGame ;
- effacée par `UGridActivationComponent::ResetRuntimeState()`.

Aucune migration SaveGame n'est donc nécessaire.

Après rechargement/réinitialisation du runtime, un compagnon refusé peut être proposé à nouveau. En revanche, un compagnon réellement recruté reste détecté via `ActiveCharacters`, qui est déjà persistant.

## Runtime

`UGridActivationComponent` possède désormais une mémoire interne :

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

## Logs attendus

Après un premier refus :

```text
[GridRecruitmentOffer] Suppressed Source=<SourceId> Companion=<CompanionId> Reason=Declined
```

Lors d'un nouveau passage sur la même source :

```text
[GridRecruitmentOffer] Skipped Source=<SourceId> Companion=<CompanionId> Reason=DeclinedFromSource
Grid link executed: ... Command=OfferRecruitment Success=true
```

Après recrutement, lors d'un nouveau passage :

```text
[GridRecruitmentOffer] Skipped Source=<SourceId> Companion=<CompanionId> Reason=AlreadyActive
Grid link executed: ... Command=OfferRecruitment Success=true
```

Aucun `WBP_RPGStoryCompanionRecruitment` ne doit alors être ajouté au viewport.

## Groupe complet

`PartyFull` n'est pas assimilé à un refus.

Le candidat peut rester dans `CharacterPool` / réserve et `IsStoryCompanionAlreadyActive()` ne regarde que `ActiveCharacters`.

Le joueur peut donc toujours recevoir une nouvelle offre si aucune décision `Refuser` n'a été prise pour cette source.

## Lua

Aucune API Lua supplémentaire n'est introduite.

`grid.command(..., "OfferRecruitment")` conserve le même chemin `ApplyLinkCommand()` et bénéficie automatiquement des mêmes règles. La source Lua courante fournit le `SourceObjectId`, donc la suppression d'un refus reste elle aussi source-scoped.

## Tests ajoutés

Le filtre principal reste :

```text
Grimrock.MON20.4.RecruitmentUI
```

Deux tests supplémentaires sont ajoutés :

```text
OfferDeclineSourceScope
OfferAlreadyActiveSuppression
```

### OfferDeclineSourceScope

Vérifie :

- aucune suppression initiale ;
- un refus supprime la même paire Source + Character ;
- une autre source peut reproposer le même Character ;
- `ResetRuntimeState()` efface la suppression non persistante.

### OfferAlreadyActiveSuppression

Vérifie :

- identité exacte dans `ActiveCharacters` -> déjà actif ;
- collision GUID avec ClassId différent -> pas déjà actif ;
- personnage uniquement dans `CharacterPool` -> reste offerable.

Le filtre MON20.4 doit donc passer de **16 à 18 tests**.

## Validation PIE demandée

Après 18/18 Automation Tests :

### Cas A — Refuser

```text
1. entrer sur le Trigger
2. popup visible
3. cliquer Refuser
4. sortir de la case
5. revenir sur le même Trigger
```

Attendu :

```text
aucune popup
Reason=DeclinedFromSource
```

### Cas B — Recruter

```text
1. repartir d'un état où le compagnon n'est pas actif
2. entrer sur le Trigger
3. cliquer Recruter
4. sortir de la case
5. revenir sur le même Trigger
```

Attendu :

```text
aucune popup
Reason=AlreadyActive
```

### Cas C — refus non définitif

Le contrat automatisé vérifie qu'une autre source peut réémettre l'offre. Un test PIE multi-source pourra être ajouté lorsqu'une deuxième scène de recrutement réelle existera ; il n'est pas nécessaire de créer un asset temporaire uniquement pour MON20.4.6.

## Hors périmètre

MON20.4.6 n'ajoute pas :

- refus permanent ;
- migration SaveGame ;
- nouveau type de Trigger ;
- Trigger one-shot spécial recrutement ;
- nouvelle commande Event/Command ;
- API Lua parallèle ;
- dialogue général ;
- Actor NPC compagnon.
