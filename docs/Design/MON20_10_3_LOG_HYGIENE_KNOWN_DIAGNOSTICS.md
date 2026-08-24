# MON20.10.3 — Log Hygiene / Known Diagnostics

Date : **24 août 2026**  
Statut : **CLOS — diagnostic/audit uniquement, aucun changement runtime requis**  
Jalon parent : **MON20.10 — Balance / Regression / Closure**

---

## 1. Objectif

Classer les warnings et diagnostics observés pendant la validation finale de MON20 afin de distinguer :

- les vraies régressions fonctionnelles ;
- les rejets fail-closed attendus ;
- les objets volontairement data-only ;
- les warnings propres au harness Automation / cleanup UE ;
- les anomalies qui doivent bloquer la clôture de MON20.

La règle retenue est volontairement conservatrice : **ne pas modifier le runtime uniquement pour faire disparaître un message attendu si ce changement risque de masquer un diagnostic utile ailleurs**.

---

## 2. MON20.10.2 — validation enregistrée

La validation UE5.5.4 fournie le 24 août 2026 confirme :

```text
Grimrock.MON20.10.2
    DeadRestoreOverPartyCell              Success
    LivingRestoreStillRejectsPartyCell    Success

TOTAL 2/2 Success
```

Régression de sécurité :

```text
Grimrock.Monsters.MON17.8
TOTAL 8/8 Success
```

Le comportement attendu est donc confirmé :

```text
snapshot mort + case du groupe
    -> Actor restauré
    -> Dead
    -> HP = 0
    -> caché
    -> collision désactivée
    -> aucune occupation

snapshot vivant + case du groupe
    -> rejet
    -> Reason=PartyOccupiesCell
```

MON20.10.2 est considéré **VALIDÉ UE5.5.4**.

---

## 3. Diagnostic : CustomRecruiter_Service sans RuntimeActorClass

Message observé pendant un Continue PIE précédent :

```text
Runtime object skipped: archetype CustomRecruiter_Service has no RuntimeActorClass.
```

### Classification

**NON BLOQUANT — attendu par le contrat d'authoring MON20.5.5.**

`CustomRecruiter` est explicitement un target logique/data-only :

```text
Gameplay Type       = CustomRecruiter
Runtime Actor Class = None
```

Le wizard est ouvert par :

```text
Event -> Command
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn
```

Aucun Actor runtime propre au recruteur n'est nécessaire.

Le même principe existe pour `StoryCompanion`, qui reste lui aussi un target logique/data-only pour `OfferRecruitment`.

### Décision MON20.10.3

Aucun changement de code n'est appliqué dans cette tranche.

Raison : le warning générique `Runtime object skipped` protège également contre de vrais archetypes incomplets. Le supprimer ou l'affaiblir globalement uniquement pour deux targets data-only risquerait de masquer une erreur d'authoring sur d'autres familles d'objets.

Le message est donc classé comme **warning connu accepté pour MON20**.

Une évolution future pourra introduire une classification explicite `data-only target` dans le diagnostic runtime si ce bruit devient gênant à grande échelle, mais ce n'est pas nécessaire pour la fermeture fonctionnelle de MON20.

---

## 4. Diagnostic : GrimrockParty_2 rejeté pendant SlotProbe

Message observé :

```text
[GridSaveMigration] LoadValidation ... Result=Rejected
[MON18.9.3] SlotProbe Slot=GrimrockParty_2 ... Result=Rejected
```

### Classification

**NON BLOQUANT — comportement fail-closed correct.**

Le slot réellement continué est :

```text
GrimrockParty
```

et celui-ci est accepté en SaveVersion 8.

`GrimrockParty_2` est un slot secondaire ancien/incohérent. La validation ne doit surtout pas être relâchée pour le rendre artificiellement compatible.

### Décision MON20.10.3

- aucune migration permissive supplémentaire ;
- aucun fallback silencieux ;
- aucun changement de contrat SaveGame ;
- le slot peut être supprimé manuellement plus tard s'il n'a plus d'utilité.

Un vieux slot invalide doit rester visible comme diagnostic lorsqu'il est sondé.

---

## 5. Diagnostic : PartyOccupiesCell / MissingActor sur monstre mort

Messages historiques :

```text
[GridMonsterSpawn] ... Reason=PartyOccupiesCell
[GridMonsterState] MissingActor ...
```

### Classification

**ANCIENNE RÉGRESSION — CORRIGÉE ET VALIDÉE par MON20.10.2.**

Ces messages ne sont plus acceptables pour le cas d'un snapshot mort restauré.

Ils restent en revanche légitimes pour un snapshot vivant qui tente de restaurer un monstre sur la cellule du groupe.

### Politique de clôture

Pendant le dernier Continue PIE de MON20 :

- `PartyOccupiesCell` sur un **monstre vivant** peut être un rejet légitime à analyser selon le contexte ;
- `PartyOccupiesCell` suivi de `MissingActor` sur un **snapshot mort** doit être considéré comme une régression et bloque la clôture.

---

## 6. Diagnostic : FlushRenderingCommands called recursively

Pendant le test Automation `DeadRestoreOverPartyCell`, le cleanup du monde de test a émis :

```text
LogRendererCore: Warning: FlushRenderingCommands called recursively! 2 calls on the stack.
```

Le test lui-même termine en `Success` et le warning apparaît pendant :

```text
UWorld::CleanupWorld
InvalidateAllWidgets
```

après chargement du SkeletalMesh de test.

### Classification

**NON BLOQUANT POUR MON20.10.2 — warning de cleanup/harness UE observé hors gameplay.**

Il ne correspond ni à un échec de restore, ni à une violation d'occupation, ni à un Fail Automation.

### Garde

Si ce warning apparaît dans le PIE normal du jeu, hors Automation/cleanup de monde de test, il devra être réévalué séparément. Il n'est pas déclaré globalement "inoffensif" dans tous les contextes.

---

## 7. Matrice de clôture

| Diagnostic | Classification | Bloque MON20 ? | Action |
|---|---|---:|---|
| `CustomRecruiter_Service has no RuntimeActorClass` | target data-only attendu | Non | accepter/documenter |
| `GrimrockParty_2 ... Rejected` | vieux slot invalide, fail-closed | Non | ne pas relâcher la validation |
| dead restore `PartyOccupiesCell -> MissingActor` | vraie régression historique | Oui si elle réapparaît | corrigé MON20.10.2 |
| living restore `PartyOccupiesCell` | garde d'occupation attendue | Non | conserver |
| `FlushRenderingCommands called recursively` pendant cleanup Automation | harness/cleanup UE | Non dans ce contexte | surveiller seulement hors tests |

---

## 8. Résultat MON20.10.3

Aucun défaut fonctionnel MON20 supplémentaire n'est identifié.

Aucun C++, `.uasset` ou `.umap` n'est modifié dans cette tranche.

MON20.10.3 est donc **CLOS**.

La suite autoritaire est :

```text
MON20.10.4 — Full MON20 Automation Regression
```

Baseline attendue :

```text
MON20.2      6
MON20.3      6
MON20.4     18
MON20.5     23
MON20.6     24
MON20.7     24
MON20.8     24
MON20.9     24
MON20.10.2   2
----------------
TOTAL       151 tests
```

Cible de MON20.10.4 :

```text
Grimrock.MON20
151/151 Success
0 Fail
0 Error
```
