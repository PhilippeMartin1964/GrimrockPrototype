# MON16.5 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.5 : EN ATTENTE
Régressions        : EN ATTENTE
Clôture            : NON
```

Base : `65f3c3bae7e52d05a6708be1591351166d823964`.

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

## Compilation UE5.5.4

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation / chargement confirmé par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.5
```

- [ ] `ControlAggregation` — Success
- [ ] `PermanentSkipActivationRejected` — Success
- [ ] `StackBooleanSemantics` — Success
- [ ] `TurnSkipLifecycle` — Success
- [ ] `RoundSkipLifecycle` — Success
- [ ] `SilenceCatalogIsolation` — Success
- [ ] `SilenceRequestAtomic` — Success
- [ ] `PartyImmobilizeTranslation` — Success
- [ ] `PartyImmobilizeRotation` — Success
- [ ] `TargetParity` — Success
- [ ] `NoParallelSystem` — Success

Attendu : **11/11 Success**.

## Régressions minimales

Après MON16.5 vert :

```text
Automation RunTests Grimrock.RPG.MON16.4
Automation RunTests Grimrock.RPG.MON16.3
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

- [ ] MON16.4 : 11/11 Success
- [ ] MON16.3 : 11/11 Success
- [ ] MON16.2 : 10/10 Success
- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Clôture

MON16.5 pourra être marqué **VALIDÉ ET CLOS** après chargement/compilation UE5.5.4, 11/11 MON16.5 et régressions appropriées sans échec dans les logs utilisateur.

Prochaine étape : `MON16.6 — HUD / Combat Feedback des status effects`.
