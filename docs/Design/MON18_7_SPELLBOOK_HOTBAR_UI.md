# MON18.7 — Spellbook / Hotbar UI

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Relier le Spellbook MON18.2 à la hotbar MON12.8 et au runtime de cast MON18.3–MON18.6 sans créer de deuxième système de raccourcis ni déplacer la logique gameplay dans l'UI.

## Résultat final

MON18.7 est clos. Le parcours joueur réel est désormais :

```text
WBP_GrimrockMenu
    -> onglet Sorts
    -> WBP_GridSpellbook / UGridSpellbookWidget
    -> entrées de sorts connus
    -> drag & drop vers la hotbar MON12
    -> clic / touche 0-9
    -> résolution Spellbook
    -> ciblage MON18.4
    -> transaction PA/mana MON18.3
    -> effets MON18.5
    -> présentation MON18.6
```

## Découpage réalisé

```text
MON18.7a / UI01.4.3a-b-c-d — modèle UI natif, page Spellbook, présentation et drag/drop   CLOS
UI01.4.3e.1             — résolution des bindings Spell dans le catalogue                CLOS
UI01.4.3e.2             — exécution réelle depuis la hotbar                              CLOS
```

Les assets WBP concernés ont été modifiés manuellement dans Unreal lorsque nécessaire ; la logique autoritaire reste en C++.

## Modèle UI natif

`UGridSpellbookUILibrary` projette chaque `SpellId` connu vers `FGridSpellbookEntryView` avec :

- nom / description ;
- école ;
- coût mana / PA ;
- portée ;
- politique de ciblage ;
- exigence LOS ;
- état de résolution de la définition ;
- slot hotbar déjà assigné ;
- `FGridCombatActionDefinition` UI-ready.

Les sorts connus dont la définition n'est pas résolue restent visibles mais explicitement non assignables. Aucun `SpellId` n'est silencieusement supprimé de l'UI.

## Identité canonique dans la hotbar

Un sort utilise exactement :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
SourceRuntimeId    = invalid
EquipmentSlot      = None
```

Aucun état de sort n'est copié dans la hotbar. Les dix slots MON12 restent l'unique stockage de raccourcis.

`AssignKnownSpellToHotbar()` :

- vérifie le `CharacterId` ;
- refuse un sort inconnu ;
- refuse une définition de production absente/invalide ;
- réutilise `SetCharacterCombatHotbarBinding()` ;
- réutilise `MoveOrSwapCharacterCombatHotbarBinding()` pour éviter les doublons.

`UnassignSpellFromHotbar()` retire uniquement le raccourci et ne retire jamais le sort du Spellbook.

## Drag & Drop

`UGridCombatHotbarDragDropOperation` porte le payload Spellbook natif et le drop final délègue au bridge C++ existant. Aucun déplacement ou consommation d'objet d'inventaire n'intervient pour un sort.

## Exécution depuis la hotbar

`UGridTurnManagerComponent::RequestCharacterCombatAction()` reconnaît les actions réellement issues du Spellbook et les route vers `FGridSpellHotbarExecutionService`.

Pipeline :

```text
Hotbar
    -> catalogue MON12
    -> FGridSpellHotbarExecutionService
    -> FGridSpellCastPipelineService
        -> ciblage MON18.4
        -> transaction MON18.3
    -> FGridSpellEffectResolver MON18.5
    -> commit autoritaire
    -> présentation MON18.6
```

Le catalogue a été corrigé après validation PIE afin qu'une projection Spellbook `Effect + SpellId` ne soit plus rejetée comme `ExecutionNotImplemented`.

## Atomicité

Les coûts et effets sont calculés sur des copies. Les PA/mana ne sont commités que si le ciblage, la transaction et les effets aboutissent. Un sort inconnu, une cible invalide ou une définition de statut absente n'engage aucun coût réel.

## Ciblage actuel

```text
Spell_ArcaneBolt   -> première cible hostile axiale suggérée
Spell_LesserHeal   -> allié ; hotbar directe = lanceur
Spell_Haste        -> allié ; hotbar directe = lanceur
Spell_CurePoison   -> allié ; hotbar directe = lanceur
```

La sélection explicite d'un autre membre via portrait pourra être ajoutée ultérieurement sans modifier le contrat MON18.

## Validation Automation

Validation UI01.4.3e.2 sous UE5.5.4 : **6/6 Success**.

```text
ArcaneBoltExecution
LesserHealExecution
MissingStatusNoCostCommit
SpellbookCatalogAvailability
SpellbookCatalogExecutorGate
UnknownSpellNoCostCommit
```

Les tests historiques de projection, binding, assignation et drag/drop MON18.7a restent la couverture du bridge Spellbook/hotbar.

## Validation PIE

Validation manuelle réussie :

- `Grimrock.Spellbook.SeedProduction` ajoute les quatre sorts de production ;
- `Lesser Heal` lancé depuis la hotbar consomme 2 PA et 4 mana et soigne 5 PV ;
- `Arcane Bolt` lancé depuis la hotbar consomme 2 PA et 3 mana et inflige 4 dégâts ;
- un `Arcane Bolt` létal déclenche correctement mort, loot, XP et libération d'occupation du monstre ;
- `Arcane Bolt` est refusé proprement lorsque le mana est insuffisant.

Référence détaillée : `docs/Design/UI_SPELLBOOK_HOTBAR_EXECUTION.md`.

## Hors périmètre restant

MON18.7 est clos, mais les points suivants restent volontairement séparés :

- persistance de `KnownSpellIds` : `MON18.8` ;
- migration SaveGame associée : `MON18.8` ;
- icônes finales de sorts : finition graphique ;
- sélection d'un allié autre que le lanceur depuis la hotbar : évolution UI future ;
- nouveaux sorts / équilibrage : jalons ultérieurs.

## Conclusion

```text
MON18.7 — Spellbook / Hotbar UI
VALIDÉ ET CLOS sous UE5.5.4
```

Le prochain sous-jalon autoritaire est `MON18.8 — Persistence / Migration`.
