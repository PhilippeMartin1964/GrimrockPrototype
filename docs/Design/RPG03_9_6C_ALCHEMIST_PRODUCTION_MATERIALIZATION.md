# RPG03.9.6C — Alchimiste : matérialisation production

Date : **6 octobre 2026**  
Dépendances validées : `RPG03.9.6A` 8/8, `RPG03.9.6B1` 7/7, `RPG03.9.6B2` 7/7  
Statut : **VALIDÉ / MATÉRIALISÉ — 18 binaires Alchimiste matérialisés ; campagne globale 193/193 le 6 octobre 2026**

## Binaires attendus

Le commandlet Unreal matérialise exactement :

- `DA_Class_Alchemist` ;
- 11 `DA_Item_*` QuickItems Alchimiste ;
- 6 `DA_Status_*` Alchimiste.

Total attendu dans `git status` : **18 fichiers**.

`DA_Status_Burning` est une dépendance partagée déjà matérialisée par le Mage. Le commandlet la valide mais ne la réécrit pas.

## QuickItems

Grenadier / Apothicaire :

- Item_Bomb_Fire
- Item_Bomb_Toxic
- Item_Antidote
- Item_DefensiveElixir_Fire
- Item_DefensiveElixir_Ice
- Item_DefensiveElixir_Lightning
- Item_DefensiveElixir_Poison
- Item_Panacea

Transmutateur :

- Item_Flask_Oil
- Item_Flask_Acid
- Item_Flask_CorrosiveCloud

## Status

- Status_Poison
- Status_DefensiveElixir_Fire
- Status_DefensiveElixir_Ice
- Status_DefensiveElixir_Lightning
- Status_DefensiveElixir_Poison
- Status_Corroded

## Transmutation majeure

Aucun faux asset de recette ou duplicata de `Item_Catalyst_Rare` n'est créé.

Le talent, les quatre RequirementIds de recette et les quatre profils de conversion sont déjà authorés/testés en B2. Leur sélection concrète attend le futur système Crafting.

## Exécution

```powershell
.\Scripts\AuthorRPGAlchemist.ps1 -EngineRoot D:\UE_5.5
```

Le script compile l'Editor, exécute le commandlet puis lance :

```text
Grimrock.RPG.RPG03.9.6
```

Les binaires ne seront commités qu'après lecture de la sortie utilisateur.
