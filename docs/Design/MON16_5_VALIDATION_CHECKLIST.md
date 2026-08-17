# MON16.5 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
Chargement UE5     : CONFIRMÉ PAR EXÉCUTION AUTOMATION
Automation MON16.5 : 11/11 SUCCESS
Régressions        : CAMPAGNE GLOBALE VALIDÉE HORS 2 FIXTURES MON16.5 CORRIGÉES
Clôture            : OUI — 17 AOÛT 2026
```

Base : `65f3c3bae7e52d05a6708be1591351166d823964`.

Implémentation validée : `f9429734f249d995dbb173b206d161dc00a3c615`.

## Architecture

- [x] profil `FGridStatusEffectControlProfile` data-driven
- [x] resolver pur `FGridStatusEffectControlResolver`
- [x] aucun hard-code d'EffectId Stun/Silence/Immobilize
- [x] aucun second lifecycle
- [x] aucun second catalogue d'actions
- [x] aucun second système de mouvement
- [x] personnages et monstres utilisent le même profil
- [x] aucune dépendance UI

## Stun / SkipActivation

- [x] `bSkipActivation` évalué à la frontière d'activation
- [x] aucune action Player/Monster démarrée si actif
- [x] activation consommée comme `Completed`
- [x] party : PA utilisables ramenés à 0 sur l'activation sautée
- [x] `Completed` réutilise le lifecycle MON16.2
- [x] durée `Turns=1` expire après une activation sautée
- [x] durées `Rounds` restent gouvernées par MON16.2
- [x] `Incapacitated` n'est pas détourné pour stocker Stun
- [x] `SkipActivation` permanent rejeté par validation
- [x] aucun arrêt rétroactif d'un combattant déjà actif

## Silence / BlockSpellActions

- [x] `bBlockSpellActions` utilise `SourcePolicy::Spell`
- [x] sort conservé dans le catalogue mais désactivé
- [x] requête autoritative rejetée par `ActionUnavailable`
- [x] disponibilité rejetée par `MissingRequirement`
- [x] aucun PA dépensé lors du rejet
- [x] aucun mana dépensé lors du rejet
- [x] aucun effet appliqué lors du rejet
- [x] Ability non bloquée
- [x] Equipment non bloqué
- [x] QuickItem non bloqué
- [x] Universal non bloqué
- [x] les chemins ciblés réutilisent le même catalogue

## Immobilize / BlockTranslation

- [x] translation party bloquée avant dépense PA/PAM
- [x] rotation party reste autorisée
- [x] Move monstre bloqué dans le TurnManager
- [x] Turn monstre non bloqué
- [x] MeleeAttack monstre non bloqué
- [x] Wait monstre non bloqué
- [x] planner monstre existant conservé
- [x] raison de rejet UI dédiée différée à MON16.6

## Composition / stacking

- [x] plusieurs effets combinent leurs capacités par OR logique
- [x] un effet peut combiner plusieurs capacités
- [x] `StackCount` reste conservé dans le runtime commun
- [x] une capacité booléenne n'est pas multipliée numériquement par les stacks
- [x] coexistence avec PeriodicDamage MON16.3
- [x] coexistence avec InitiativeModifier MON16.4

## Hors périmètre respecté

- [x] aucun HUD/icône/WBP
- [x] aucun `.uasset`/`.umap`
- [x] aucune persistance ajoutée
- [x] aucune immunité/résistance au contrôle ajoutée
- [x] aucun jet de sauvegarde ajouté
- [x] aucun dispel/cleanse ajouté
- [x] aucune application automatique par attaque/sort ajoutée
- [x] aucun nouveau type d'action magique monstre inventé

## Validation UE5.5.4

Le namespace MON16.5 a été exécuté avec succès dans UE5.5.4 après correction de la fixture de positionnement du pawn.

- [x] chargement/exécution UE5 confirmé par log utilisateur

## Automation ciblée finale

Commande :

```text
Automation RunTests Grimrock.RPG.MON16.5
```

Résultat du 17 août 2026 :

- [x] `ControlAggregation` — Success
- [x] `PermanentSkipActivationRejected` — Success
- [x] `StackBooleanSemantics` — Success
- [x] `TurnSkipLifecycle` — Success
- [x] `RoundSkipLifecycle` — Success
- [x] `SilenceCatalogIsolation` — Success
- [x] `SilenceRequestAtomic` — Success
- [x] `PartyImmobilizeTranslation` — Success
- [x] `PartyImmobilizeRotation` — Success
- [x] `TargetParity` — Success
- [x] `NoParallelSystem` — Success

**Résultat final : 11/11 Success, 0 Fail.**

## Campagne de régression

La campagne globale immédiatement précédente a exécuté 145 tests :

```text
143 Success
2 Fail
```

Les deux seuls échecs étaient :

```text
Grimrock.RPG.MON16.5.PartyImmobilizeRotation
Grimrock.RPG.MON16.5.SilenceCatalogIsolation
```

Tous les tests hors MON16.5 étaient donc déjà Success.

La cause des deux échecs MON16.5 était une fixture incohérente : `CurrentCellX`, `CurrentCellY` et `Facing` étaient renseignés sans synchroniser la transform physique du pawn. `IsPartyAtRest()` retournait alors faux et provoquait `PartyBusy` avant la règle réellement testée.

Correction :

```cpp
Party->SnapToCurrentCell();
```

Le rerun ciblé final confirme les 11 tests MON16.5. Le log confirme notamment :

```text
PartyImmobilizeRotation : Accepted=true Type=Rotation
PartyImmobilizeTranslation : TranslationBlocked
SilenceRequestAtomic : MissingRequirement
```

La correction finale ne modifie que la fixture de test MON16.5 ; les 134 autres tests déjà verts ne nécessitent donc pas une nouvelle campagne complète pour cette clôture.

## Clôture

**MON16.5 — VALIDÉ ET CLOS le 17 août 2026.**

Prochaine étape : `MON16.6 — HUD / Combat Feedback des status effects`.
