# MON21.3 â€” Quest Event â†’ Command Integration

Date : 26 aoÃ»t 2026

MON21.3 connects the existing `FGridObjectLink` Event â†’ Command bus to the
campaign-authoritative `UGridQuestSubsystem` introduced by MON21.2.

Commands keep explicit serialized values:

- `QuestStart = 25`
- `QuestCompleteObjective = 26`
- `QuestComplete = 27`
- `QuestFail = 28`

`FGridObjectLink` now carries stable `QuestId` and `QuestObjectiveId` values.
Quest commands are campaign-global and therefore do not require `TargetObjectId`.

`UGridLevelAsset::QuestDefinitions` references the definitions used by a level.
`UGridActivationComponent::RebuildIndexes()` registers them in the
GameInstance-owned `UGridQuestSubsystem`. The subsystem remains the only runtime
source of truth; the level asset stores definitions and link parameters only.

`AlreadyInState` is treated as a successful idempotent command adaptation.
Other mutation failures remain failures.

Automation:

`Grimrock.Quests.MON21_3.EventCommandIntegration`

Out of scope: SaveGame, Journal UI, Map, Codex, direct Lua quest syntax, and all
`.uasset` / `.umap` changes.

Next: MON21.4 â€” Quest Persistence / Migration.