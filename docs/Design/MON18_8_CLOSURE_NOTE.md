# MON18.8 — Closure Note

Date : **22 août 2026**

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

## Résumé

MON18.8 ajoute la persistance versionnée du Spellbook par identité stable `CharacterId + KnownSpellIds`, la migration explicite v5 -> v6, la restauration atomique et l'intégration d'un `UGridPartySpellbookComponent` natif au Pawn.

Validation fournie depuis UE5.5.4 :

```text
Grimrock.Magic.MON18.8                 12/12 Success
Grimrock.RPG.MON16.5                   9/9 Success
Grimrock.UI.UI01.4.3e.2                6/6 Success
```

Validation PIE réelle :

```text
Seed initial : Added=4 AlreadyKnown=0
Save v6      : Spellbooks=1
arrêt PIE
Continue
Seed après load : Added=0 AlreadyKnown=4
```

Le second seed prouve que les quatre sorts de production sont restaurés depuis le SaveGame et non recréés par le runtime.

## Régression détectée pendant la validation

La campagne globale a identifié une régression MON16.5 dans le chemin d'exécution des sorts de statut : comparaison directe de `EffectId` dans `GridTurnManagerPlayerActionCatalog.cpp`.

Correction poussée :

```text
d25cf26e0d7c052fe30ba772e50e02b7debaa918
Fix MON16.5 status identity regression
```

La résolution utilise désormais l'identité primaire canonique du Status Effect. Le test `Grimrock.RPG.MON16.5.NoParallelSystem` puis l'ensemble MON16.5 ont été validés.

## Note sur les diagnostics de slots secondaires

Le log du menu principal contient deux rejets de snapshots sans état de progression. Le slot principal `GrimrockParty`, chargé ensuite avec `Continue`, est accepté et restauré correctement.

`UGrimrockGameInstance` énumère également les slots configurés `GrimrockParty_2` et `GrimrockParty_3`. Des fichiers auxiliaires anciens peuvent donc produire ces diagnostics lors de la construction de la liste de sauvegardes sans invalider le slot principal.

Ce point est considéré comme bruit de compatibilité de slots auxiliaires et pourra être nettoyé en MON18.9 si nécessaire.

## Références

- `docs/Design/MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md`
- `docs/Design/MON18_8_VALIDATION.md`
- `docs/Design/PROJECT_COMPLETION_ROADMAP.md`

**MON18.8 est clôturé. Prochaine étape : MON18.9 — Balance / Regression / Closure.**
