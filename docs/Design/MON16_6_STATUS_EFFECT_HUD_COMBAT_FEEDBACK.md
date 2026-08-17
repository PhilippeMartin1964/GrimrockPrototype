# MON16.6 — HUD / Combat Feedback des status effects

## Statut

**VALIDÉ ET CLOS — 17 août 2026.**

Base :

```text
b153a5d48f709f5b86d8d125e7cd61daa095966b
Close MON16.5 status control
```

Implémentation :

```text
969839d2546eea399cd2403dee2c977628efe2fc
Add MON16.6 status HUD feedback
```

Correctif compilation UE5.5.4 :

```text
8f11c2641e64f46f5ebe31a162404fd58ffae22e
Fix MON16.6 UE5.5 compilation
```

MON16.6 ajoute une couche de présentation strictement read-only au système de statuts MON16.1–5. Les règles de durée, DoT, initiative et contrôle restent dans leurs systèmes autoritatifs existants.

## 1. Projection data-driven

Nouveau type :

```text
FGridStatusEffectPresentationView
FGridStatusEffectPresentationBuilder
```

La projection expose notamment :

- EffectId, nom et description ;
- icône optionnelle ;
- Buff / Debuff / Neutral ;
- durée restante et unité Turns/Rounds/Permanent ;
- stack count et potency ;
- contribution d'initiative calculée à partir des données existantes ;
- présence de dégâts périodiques ;
- capacités de contrôle SkipActivation / BlockSpellActions / BlockTranslation ;
- libellé compact et tooltip.

Aucun nom de statut particulier n'est interprété. La présentation reste valable pour Poison, Haste, Stun ou tout futur effet créé dans les données.

## 2. Icône optionnelle

`UGridStatusEffectDefinitionAsset` reçoit :

```cpp
TSoftObjectPtr<UTexture2D> Icon;
```

Cette propriété est purement visuelle. Une définition sans icône reste valide et utilisable.

Aucun `.uasset` n'est modifié dans MON16.6.

## 3. HUD groupe

`UGridCombatActionPanelWidget` projette désormais la collection de statuts du personnage dans :

```text
View.StatusEffects
View.StatusSummary
View.LatestStatusFeedback
```

Le résumé natif suit une forme compacte, par exemple :

```text
Poison x2 | T2   Haste | R1
```

Le tooltip donne la description, la durée et les propriétés pertinentes.

Deux bindings sont optionnels :

```text
Text_StatusEffects
Text_StatusFeedback
```

S'ils n'existent pas dans le WBP, le widget C++ crée des `UTextBlock` de fallback sous le conteneur vertical déjà existant. Il n'est donc pas nécessaire de modifier un WBP pour tester MON16.6.

## 4. Rafraîchissement event-driven

Aucun tick UI n'est ajouté.

Après une mutation autoritative d'un statut de personnage, le lifecycle réutilise :

```cpp
UGridPartyInventoryComponent::NotifyPartyInventoryChanged(CharacterIndex)
```

Le HUD MON12 est déjà abonné à cet événement et reconstruit ses vues normalement.

## 5. Combat feedback

MON16.6 étend le vocabulaire `FGridCombatLogEntry` avec :

```text
StatusApplied
StatusRefreshed
StatusTicked
StatusExpired
```

et les champs structurés :

```text
StatusEffectId
StatusEffectDisplayName
StatusEffectStackCount
StatusEffectDurationText
```

Le formatter combat existant fournit les textes Apply / Refresh / Tick / Expire.

### Autorité du CombatLog

Le `UGridTurnManagerComponent` reste l'unique propriétaire de son historique `CombatLogEntries` et de sa séquence. MON16.6 ne contourne pas son API privée et ne crée pas de second ring-buffer.

Le lifecycle publie uniquement le dernier `FGridCombatLogEntry` de statut sous forme d'événement transitoire :

```text
LastStatusEffectFeedback
OnStatusEffectFeedback
```

Ce payload réutilise le schéma et le formatter du combat log, mais n'introduit pas un second historique concurrent.

## 6. Ordre des événements périodiques

Le contrat MON16.3 est conservé :

```text
Periodic damage
-> StatusTicked feedback
-> duration decrement
-> StatusExpired feedback éventuel
```

Un effet `Turns=1` produit donc son dernier Tick avant Expire.

Les dégâts affichés sont ceux du `FGridAttackResult` déjà calculé par MON16.3. MON16.6 ne recalcule aucune résistance, armure ou quantité de dégâts.

## 7. Groupe / monstres

Apply / Refresh / Tick / Expire utilisent le même payload pour le groupe et les monstres.

Le HUD de groupe affiche directement les statuts des personnages. Pour les monstres, le feedback structuré est disponible via `OnStatusEffectFeedback`; MON16.6 n'altère pas la timeline d'initiative et n'ajoute pas de widget monstre parallèle.

## 8. Contrôles MON16.5

MON16.6 ne duplique pas les règles de Stun / Silence / Immobilize :

- Silence continue à rendre les actions `Spell` indisponibles via le catalogue ;
- Immobilize continue à refuser la translation ;
- Stun continue à consommer l'activation au niveau initiative.

Le tooltip de statut explique ces capacités, mais le HUD n'en devient jamais l'autorité.

## 9. Hors périmètre

MON16.6 n'ajoute pas :

- de logique gameplay de statut ;
- de nouveau lifecycle ;
- de second CombatLog/ring-buffer ;
- de tick/polling UI ;
- de VFX/audio spécifique ;
- de `.uasset`, `.umap` ou modification WBP ;
- de persistance SaveGame, réservée à MON16.7.

## 10. Automation

Namespace :

```text
Grimrock.RPG.MON16.6
```

Tests :

```text
PresentationProjection
DurationFormatting
DeterministicProjection
FallbackProjection
PartyApplyFeedback
RefreshFeedback
TickAndExpirationFeedback
MonsterFeedbackParity
PartyPanelProjection
NoParallelSystem
```

Validation : **10/10 Success**.

Le feedback observé confirme notamment :

- application groupe et monstre ;
- refresh avec durée mise à jour ;
- DoT présenté depuis le résultat MON16.3 ;
- deux ticks successifs puis expiration ;
- projection HUD groupe ;
- absence de système parallèle.

## 11. Validation finale du 17 août 2026

Après le correctif UE5.5.4 `8f11c2641e64f46f5ebe31a162404fd58ffae22e`, les tests ont été exécutés avec succès dans l'éditeur.

Le log utilisateur contient quatre campagnes successives (`Test Run 5` à `Test Run 8`) :

```text
Test Completed : 293
Success        : 293
Fail           : 0
Error          : 0
```

La dernière campagne complète pertinente (`Test Run 8`) donne :

```text
MON16.6 : 10/10 Success
MON16.5 : 11/11 Success
MON16.4 : 11/11 Success
MON16.3 : 11/11 Success
MON16.2 : 10/10 Success
MON16.1 :  7/7 Success
MON15   : 42/42 Success
MON14   : 19/19 Success
-----------------------
Total   : 121/121 Success
```

Les warnings présents dans certaines fixtures historiques (par exemple `MissingMonsterMovement`, `MissingRuntimeActor` ou `InvalidClassDefinition`) appartiennent aux chemins négatifs attendus de leurs tests respectifs ; aucun `Test Completed` n'est en échec.

## Contrat gelé MON16.6

À la clôture :

- la présentation reste read-only et data-driven ;
- aucune règle gameplay de statut n'est déplacée dans le HUD ;
- le TurnManager reste l'unique propriétaire de `CombatLogEntries` ;
- le lifecycle n'expose qu'un feedback transitoire, sans second historique ;
- le HUD groupe projette nom, durée, stacks et dernier feedback ;
- les monstres utilisent le même payload de feedback ;
- le DoT est présenté depuis le `FGridAttackResult` déjà résolu ;
- aucun WBP/.uasset/.umap n'est requis ;
- aucune persistance des statuts n'est ajoutée ici.

MON16.6 est **VALIDÉ ET CLOS**.

Prochaine étape : **MON16.7 — Save / Restore des status effects**.
