# Audit transversal de cohérence de l’architecture

## Référence

- **Code audité** : `0b8bab8f86f3a7f9df4979f1df4259838a93023d`
- **Date** : 23 août 2026
- **Périmètre** : modules C++, documentation `docs/Architecture`, synthèses `docs/Design`.
- **Contexte** : après MON19 fermé et validation UE5.5.4 de MON20.2 et MON20.3, avant MON20.4.

## Verdict

Le code reste cohérent avec les principes fondateurs. La documentation d’architecture était en retard sur l’évolution réelle du projet : `GrimrockLua`, MON14–MON19, SaveGame v7, XP/progression, Status Effects, Spellbook et recrutement devaient être intégrés à la synthèse. Aucun refactor de production n’est requis par cet audit.

## Écarts documentaires corrigés

- **Modules** : trois modules actuels — `GrimrockLua`, `GrimrockPrototype`, `GrimrockPrototypeEditor`.
- **Event/Command** : variables persistantes `Bool`/`Int32`, conditions, Logic nodes, Lua, `persistent`, `LogicId` et authoring Editor sont documentés.
- **Grid Editor** : `AGridLevelEditorActor` reste une façade importante mais son implémentation est maintenant fractionnée en parts/services.
- **Runtime** : `AGridLevelRuntimeActor` reste un point de concentration majeur.
- **RPG** : XP/niveaux/progression MON15 et Status Effects MON16 sont fermés.
- **Monstres** : MON14 ajoute perception/dormance/patrouille/investigation/alarmes ; MON17 ajoute le Gobelin lanceur et le ranged combat.
- **Magic** : Spellbook/cast pipeline MON18 est un système de production.
- **Persistance** : contrat courant SaveGame v7.
- **Recruitment** : `CharacterPool` est réutilisé ; MON20.2 et MON20.3 sont validés 6/6.

## Cohérence code ↔ décisions

- DataAsset source de conception ; runtime state séparé.
- Grille autoritaire.
- Event → Command reste la voie d’effet gameplay.
- Logic/Lua orchestrent sans devenir un second dispatcher.
- `ObjectId` autoritaire, `LogicId` alias.
- `CharacterId` stable pour groupe/réserve/compagnons.
- Ownership inventaire exclusif.
- Combat déterministe séparé de la présentation.
- Editor hors runtime shipping.
- Réutilisation des systèmes existants avant nouvelle abstraction.

## Dettes et risques confirmés

1. `AGridLevelRuntimeActor` très volumineux.
2. `UGridPartyInventoryComponent`, PartyPawn et PlayerController volumineux.
3. Grid Editor complexe malgré son meilleur découpage.
4. Assets binaires dépendants de UE/PIE pour validation.
5. CI/Shipping non encore complet.
6. Architecture plus mature que le volume de contenu de production.

## Validation récente prise en compte

```text
MON19.8                         4/4 Success
Grimrock.MON19                55/55 Success
MON19 PIE final                VALIDÉ
MON20.2 Recruitment             6/6 Success
MON20.3 StoryCompanion          6/6 Success
```

## Documents de référence produits / augmentés

- `PROJECT_SYNTHESIS.md` — synthèse actuelle ;
- `ARCHITECTURE_INDEX.md` — ordre de lecture et fondations ;
- `Maps/GRIMROCK_PROJECT_MAP.md` — carte détaillée textuelle et autoritaire, couvrant 19 domaines actuels ;
- `Maps/GRIMROCK_PROJECT_MAP_MERMAID.md` — vues visuelles Mermaid dérivées de la carte détaillée ;
- `ADVANCED_DUNGEON_LOGIC_FOUNDATION.md` ;
- `COMBAT_MONSTER_AI_FOUNDATION.md` ;
- `PARTY_RPG_RECRUITMENT_FOUNDATION.md` ;
- `MAGIC_STATUS_EFFECTS_FOUNDATION.md` ;
- `SAVE_PERSISTENCE_FOUNDATION.md` ;
- `UI_GAME_FLOW_FOUNDATION.md` ;
- `TEST_AUTOMATION_FOUNDATION.md`.

## Politique de format et d’historique

La carte courante est maintenue en Markdown/Mermaid, formats textuels diffables et fiables dans Git. Le fichier XMind n’est plus maintenu comme source courante : les anciennes versions restent récupérables naturellement dans l’historique Git, sans copies `*_YYYY-MM-DD`.

## Conclusion

Le risque principal n’est plus l’absence de fondations techniques, mais la concentration de certaines classes historiques, la densité encore limitée du contenu et l’achèvement des systèmes de campagne. La prochaine étape de développement reste MON20.4.
