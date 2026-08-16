# MON16.1 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.1 : EN ATTENTE
Régression globale : EN ATTENTE
Clôture             : NON
```

Ne pas déclarer MON16.1 validé ou clos avant réception et analyse des logs UE5.

---

## 1. Vérification Git

- [x] base auditée sur `master` au commit `941ae876899c26e5587ed279be50a71a501fa361`
- [x] branche de travail logique : `master` uniquement
- [x] aucun `.uasset`
- [x] aucun `.umap`
- [x] aucun WBP
- [x] aucune modification `Documentation/`
- [x] documentation MON16.1 sous `docs/Design/`

---

## 2. Vérification architecture

- [x] `UGridStatusEffectDefinitionAsset` est data-driven
- [x] `EffectId` est un `FName` stable
- [x] `PrimaryAssetId = GridStatusEffect:EffectId`
- [x] `FGridStatusEffectRuntimeState` ne stocke aucun pointeur UObject/Actor source
- [x] `SourceId` utilise un `FGuid`
- [x] `FGridStatusEffectCollection` est commune personnages/monstres
- [x] `FGridCharacterInventoryState` possède sa collection
- [x] `AGridMonsterActor` possède sa collection
- [x] les deux propriétés de cible sont `Transient` en MON16.1
- [x] `FGridRuntimeMonsterState` n'est pas modifié
- [x] aucun système de statistiques parallèle n'est introduit
- [x] `InitiativeModifier` de la définition est déclaratif seulement
- [x] aucune logique gameplay ne dépend d'un Widget

---

## 3. Vérification périmètre

- [x] aucune diminution automatique de durée
- [x] aucune expiration automatique
- [x] aucun DoT
- [x] aucun comportement Poison/Bleeding/Burning
- [x] aucun comportement Haste/Slow
- [x] aucun comportement Stun/Silence/Immobilize
- [x] aucun HUD/icône
- [x] aucune sauvegarde/restauration des effets
- [x] politiques de stacking déclarées mais pas exécutées

---

## 4. Compilation à effectuer par l'utilisateur

Compiler le projet UE5.5.4 normalement avec la configuration de développement habituelle.

Résultat attendu avant de poursuivre :

```text
0 erreur C++
0 erreur UHT liée à MON16.1
0 erreur de link liée aux nouveaux types
```

- [ ] compilation réussie confirmée par log utilisateur

---

## 5. Automation ciblée

Dans l'Automation Test Framework, exécuter :

```text
Grimrock.RPG.MON16.1
```

Tests attendus :

```text
Grimrock.RPG.MON16.1.DefinitionValidation
Grimrock.RPG.MON16.1.StableIdentity
Grimrock.RPG.MON16.1.RuntimeStateCreation
Grimrock.RPG.MON16.1.TargetIsolation
Grimrock.RPG.MON16.1.DeterministicCollection
Grimrock.RPG.MON16.1.AtomicInvalidAdd
Grimrock.RPG.MON16.1.NoUIDependency
```

- [ ] DefinitionValidation — Success
- [ ] StableIdentity — Success
- [ ] RuntimeStateCreation — Success
- [ ] TargetIsolation — Success
- [ ] DeterministicCollection — Success
- [ ] AtomicInvalidAdd — Success
- [ ] NoUIDependency — Success

---

## 6. Régression après succès MON16.1

Après succès des tests ciblés, relancer au minimum les familles qui touchent directement les deux modèles étendus :

```text
Grimrock.RPG.MON15
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14
Grimrock.RPG.MON16.1
```

Puis, pour la validation de clôture, exécuter la campagne globale du projet utilisée à la fin de MON15.

- [ ] régression RPG MON15 réussie
- [ ] régression Monster MON13 réussie
- [ ] régression Monster MON14 réussie
- [ ] campagne globale réussie

---

## 7. Critère de clôture

MON16.1 pourra être marqué **VALIDÉ ET CLOS** uniquement quand :

- [ ] le projet compile dans UE5.5.4 ;
- [ ] tous les tests `Grimrock.RPG.MON16.1.*` sont Success ;
- [ ] aucune régression pertinente n'est observée ;
- [ ] les logs ont été fournis et examinés ;
- [ ] toute correction éventuelle a été poussée sur `origin/master`.
