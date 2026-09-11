# LUA-UX01 — Event / Action authoring

Date : 11.09.2026
Statut : implémentation C++ à valider sous UE 5.5.4

## Objectif

Simplifier l'authoring des énigmes :

- la fenêtre **Lua Scripts** ne gère plus que les scripts Lua ;
- la page de l'objet sélectionné devient l'unique surface pour relier un événement à une action ;
- une action directe `Event -> Command` est toujours inconditionnelle ;
- dès qu'une règle demande une condition, un compteur, une séquence ou un calcul, elle passe par `Event -> Lua Callback` ;
- `LogicId` est l'identité humaine utilisée par Lua et les actions ;
- les anciennes conditions stockées dans les liens restent lisibles et supprimables afin de ne pas casser les assets existants.

## Nouveau flux d'authoring

### Commande directe

```text
Selected Object
└── Events & Actions
    └── Activated
        └── Command
            ├── Target : DoorA
            └── Open
```

### Logique Lua

```text
Selected Object
└── Events & Actions
    └── Item Inserted
        └── Lua Callback
            ├── Script   : GuardianGemDoor
            └── Function : on_gem_inserted
```

```lua
persistent = {
    GuardianGemCount = 0
}

function on_gem_inserted(event)
    -- logique d'énigme ici
end
```

## Lua Scripts

La fenêtre Lua ne possède plus de notion de source objet, d'événement, de cible ou de condition. Elle permet seulement :

- ajouter / activer / renommer / supprimer un script ;
- éditer son source ;
- valider le Lua ;
- afficher les callbacks détectés ;
- afficher les variables `persistent` détectées.

Les déclarations `persistent = { ... }` restent synchronisées automatiquement avec l'état persistant du niveau par `GridEditorLuaService`.

## Compatibilité

`FGridObjectLink` et le runtime conservent les champs historiques de condition. LUA-UX01 ne supprime donc pas les données existantes. Une action héritée contenant une condition est affichée comme **Legacy condition — move this logic to Lua** et peut être supprimée, mais l'UI ne permet plus d'en créer de nouvelle.

## Validation recommandée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.6.Editor"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.7.1.Editor"
```

Validation manuelle :

1. ouvrir **Lua Scripts** et vérifier qu'aucune zone de binding/condition n'est affichée ;
2. sélectionner un objet et ouvrir la page des connecteurs/actions ;
3. vérifier que les seuls types d'action proposés sont `Command` et `Lua Callback` ;
4. créer un `Event -> Command` direct ;
5. créer un `Event -> Lua Callback` vers une fonction détectée ;
6. vérifier qu'un ancien lien conditionnel reste visible et supprimable.
