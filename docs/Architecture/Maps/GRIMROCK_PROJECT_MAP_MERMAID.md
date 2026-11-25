# Carte Mermaid du prototype

[Responsabilités](GRIMROCK_PROJECT_MAP.md) · [modèle relationnel détaillé](../DataModel.md).

```mermaid
flowchart LR
    Editor["Grid Editor / GrimrockPrototypeEditor"] --> Level["UGridLevelAsset"]
    Level --> Runtime["AGridLevelRuntimeActor"]
    Controller["Pawn / PlayerController"] --> Runtime
    Runtime --> World["Monde, réceptacles et placements"]
    Runtime --> Turns["UGridTurnManagerComponent"]
    Turns --> Party["UGridPartyInventoryComponent"]
    World <-->|"transferts par instance"| Party
    Registry["UGridItemSubsystem"] --> Root["DA_ItemSystem + quatre catalogues"]
    World --> Registry
    Party --> Registry
    Turns --> Registry
    Activation["Event → Command / Logic / Lua"] --> Runtime
    Activation --> Quest["UGridQuestSubsystem"]
    Party --> Save["UGrimrockPartySaveGame v27"]
    Runtime --> Save
    Party --> UI["Projections C++ → WBP Designer"]
    Turns --> UI
    Runtime --> Map["Exploration → modèle de carte"]
    Map --> UI
```

La flèche vers Save représente la capture d’état. Le champ durable des quêtes existe et le raccordement Pawn de sa capture reste suspendu dans [MON21.4](../../Design/PROJECT_COMPLETION_ROADMAP.md). La [carte historique détaillée](https://github.com/PhilippeMartin1964/GrimrockPrototype/blob/2941d211ded3ffca873065d93eb05a9a2c51c7b3/docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md) décrit les vues par domaine au checkpoint DOC-ARCH01.
