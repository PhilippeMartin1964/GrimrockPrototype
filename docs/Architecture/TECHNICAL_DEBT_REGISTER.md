# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **9 octobre 2026**  
Baseline C++ auditée : **6f98a0ef5599f37d3d544529aa5a790d647e8251**  
Statut : **registre rebaseliné DOC-ARCH01**

Ce document décrit uniquement les dettes **encore actives, surveillées ou différées**. Les campagnes TD01–TD07 closes restent documentées dans leurs snapshots historiques et ne sont pas réouvertes par leur seule ancienneté.

## 1. Politique

Une dette technique suppose un risque actuel : duplication d'autorité, régression/persistance, concentration de responsabilité réellement gênante, dette d'outillage/diagnostic ou dépendance environnementale non reproductible.

Ne sont pas, à eux seuls, des dettes : contenu de production limité, feature de roadmap non commencée, document historique mentionnant une ancienne API ou classe longue mais stable/testée.

## 2. Priorités

```text
P0 : 0
P1 : 0

P2 / surveillées ou différées
  TD-BUILD-002  toolchain MSVC non figée
  TD-ARCH-001   AGridLevelRuntimeActor — stop condition atteinte
  TD-ARCH-002   UGridPartyInventoryComponent — stop condition atteinte
  TD-ARCH-003   AGrimrockPartyPawn — stop condition atteinte
  TD-ARCH-004   AGrimrockPlayerController volumineux
  TD-ARCH-005   UGridActivationComponent concentré
  TD-EDITOR-001 complexité Slate / Grid Editor
  TD-LOG-001    hygiène LogTemp résiduelle/opportuniste
  TD-TOOL-001   CI UE distante différée

P3
  TD-UI-001     nomenclature historique Inventory
  TD-RPG-001    lancer manuel / future intégration Skills
```

## 3. TD-BUILD-002 — Toolchain MSVC

**P2 — surveillée.**

La machine de développement compile avec Visual Studio 2022 / MSVC 14.44 alors que UE 5.5.4 peut avertir qu'une version antérieure est préférée. Ne pas imposer un downgrade tant que build Editor et Shipping restent fonctionnels ; réouvrir uniquement sur incompatibilité concrète ou CI Windows reproductible.

`TD-BUILD-001` Meshy est résolu : Meshy reste un outil local optionnel, pas une dépendance first-party requise.

## 4. TD-ARCH-001 — AGridLevelRuntimeActor

**P2 — surveillée / stop condition TD05 atteinte.**

Le RuntimeActor reste la façade du niveau. Des extractions ciblées ont déjà isolé diagnostics, feedback UI et monstres. Ne pas poursuivre un split préventif sans nouveau signal : duplication, bugs répétés, frontière autonome ou difficulté de test.

## 5. TD-ARCH-002 — UGridPartyInventoryComponent

**P2 — surveillée / stop condition TD06 atteinte.**

Il reste l'autorité Party/Inventory. Hotbar, cursor transfers, equipment et diagnostics sont déjà séparés en unités dédiées. Ne pas créer de second service d'état groupe sous prétexte de réduire le nombre de lignes.

## 6. TD-ARCH-003 — AGrimrockPartyPawn

**P2 — surveillée / stop condition atteinte.**

Input buffer, held item, save/load façade, transfers, UI et mouvement ont été séparés en fichiers dédiés. Réouvrir uniquement sur responsabilité réellement autonome.

## 7. TD-ARCH-004 — AGrimrockPlayerController

**P2 — surveillée.**

Le controller centralise volontairement souris, curseur, résolution d'intention, targeting et délégation vers les systèmes métier. MI1–MI6 ont centralisé `ResolveLeftMouseInteraction()` et `ResolveCursorItemHoverCursor()`. La taille seule ne justifie pas une extraction.

## 8. TD-ARCH-005 — UGridActivationComponent

**P2 — surveillée / caractérisée.**

Event → Command reste le bus unique. Logic, Lua, Recruitment et Quest réutilisent ce dispatcher. L'extraction est différée tant qu'aucune duplication ou difficulté de test concrète n'apparaît.

## 9. TD-EDITOR-001 — Grid Editor / Slate

**P2 — surveillée.**

Le Grid Editor possède de nombreuses surfaces, mais les responsabilités sont organisées par panels/services et placements typés. Éviter les refactors cosmétiques massifs.

## 10. TD-LOG-001 — Logs

**P2 — opportuniste.**

Le code courant contient encore quelques usages `UE_LOG(LogTemp)`, concentrés notamment dans des commandlets/tests. Ils ne constituent pas un blocage fonctionnel. Migrer vers une catégorie nommée lorsqu'un domaine est réellement touché ; pas de grand remplacement sans bénéfice diagnostique.

## 11. TD-TOOL-001 — CI distante UE

**P2 — différée.**

Les harness locaux `Scripts/ValidateUE.ps1` et `Scripts/ValidatePackage.ps1` restent autoritaires. Une CI réelle ne doit être ajoutée que lorsqu'un runner Windows + UE5.5.4 + toolchain est disponible.

## 12. TD-UI-001 — nomenclature Inventory

**P3 — opportuniste.**

Des noms historiques tels que `EInventoryTopTab` restent utilisés par le shell résiduel. Aucun renommage transversal ne se justifie tant qu'il risquerait des références Blueprint/serialized sans gain fonctionnel.

## 13. TD-RPG-001 — lancer / Skills

**P3 — intégration fonctionnelle.**

Le lancer physique fonctionne sans scaling Skill complet. Lorsque le design définira précisément vitesse/précision/dégâts pilotés par Skills, intégrer au système existant plutôt que créer une mécanique parallèle.

## 14. Dettes résolues majeures

```text
TD-BUILD-001   Meshy dependency                         RÉSOLU
TD-COMPAT-001  deprecated Skeleton API                  RÉSOLU
TD-COMPAT-002  Python ItemTransfer collision            RÉSOLU
TD-DATA-001    legacy schema / backward compatibility   RÉSOLU
TD-PERSIST-001 Receptacle runtime permissions           RÉSOLU
TD-PARTY-001   selection / held visual notification     RÉSOLU
TD-EVENT-001   Event -> Command semantics               RÉSOLU
TD-STYLE-001   formatting tooling                       RÉSOLU
TD05           RuntimeActor stop condition              CLOS
TD06           PartyInventory stop condition            CLOS
TD07           future-proofing / schema reset           CLOS
CPP-CLEAN01    architecture cleanup                     CLOS
RUNTIME-TRANSFORM-DIAG01                                CLOS
```

## 15. Évolutions fonctionnelles non classées comme dette

```text
MON21.4 Quest Persistence
MON21.5 Journal
MON21.7 Codex
MON21.8 Cross-System Closure
MON22 Vertical Slice
Crafting / Recipes
Player Level Editor / publication
```

## 16. Contrats courants à ne pas rouvrir

- SaveGame **v24 exact-match** ;
- `LastAcknowledgedLevel` supprimé ;
- Skills : `FRPGSkillPointService`, pas de monnaie persistée ;
- Talents : transaction MON15 + `FGridSkillsPageService`, pas de façade `FRPGTalentRuntimeService` ;
- World Objects : placements typés ;
- Event → Command unique ;
- Persistent HUD = navigation/action bar ;
- Combat HUD = combat-only ;
- Map MON21.6 = close.

## 17. Validation

```text
9045ef2d
Grimrock 1026/1026
0 warning
0 failed
0 not run
Shipping Win64 OK
```

Les travaux postérieurs ont été validés par campagnes ciblées ; DOC-ARCH01 est documentation-only et n'invente aucune nouvelle validation UE.

## 18. Références

- `PROJECT_SYNTHESIS.md`
- `ARCHITECTURE_INDEX.md`
- `Maps/GRIMROCK_PROJECT_MAP.md`
- `../Design/PROJECT_COMPLETION_ROADMAP.md`
- `../Design/DOC_AUDIT02_DESIGN_CPP_COHERENCE.md`
- les tickets TD datés pour l'historique détaillé.
