# UI-RPG01.4 — Production Class Materialization & Global Invariants

Date : **6 octobre 2026**  
Parent : **UI-RPG01 — UX Contract & Talent Tree Read Model**

## Découpage atomique

Afin de respecter simultanément :

- `master` uniquement ;
- un ticket = un commit atomique ;
- aucune modification aveugle de `.uasset` ;

le jalon est exécuté en deux sous-tickets :

- **UI-RPG01.4A** — tooling source : commandlet, script, tests de production ;
- **UI-RPG01.4B** — six binaires `DA_Class_*` régénérés localement via Unreal Editor.

Le jalon UI-RPG01.4 n'est terminé qu'après validation de 01.4B.

## UI-RPG01.4A

Le commandlet `UIRPGTalentTreeAuthoring` réutilise directement les six authorings RPG03 canoniques :

```text
FRPGWarriorAuthoring
FRPGRogueAuthoring
FRPGRangerAuthoring
FRPGMageAuthoring
FRPGPriestAuthoring
FRPGAlchemistAuthoring
```

Il sauvegarde uniquement les six `DA_Class_*`. Aucun status, item ou autre asset n'est rematérialisé par ce commandlet.

## Tests de production

Filtre :

```text
Grimrock.UI.RPG01.ProductionAssets
```

Cinq invariants sont testés :

1. les six classes chargent et tous leurs Choice records possèdent `TalentBranchId` + `TalentNodeId` ;
2. les données forment exactement **18 branches / 90 nœuds conceptuels** ;
3. chaque branche possède les cinq paliers canoniques **2 / 6 / 10 / 14 / 18** avec chaîne de prérequis cohérente ;
4. les variantes connues sont regroupées sous Spécialisation martiale, Ennemi juré, Affinité élémentaire et Imprégnation ;
5. `FGridSkillsPageService` construit réellement un arbre **3 × 5** pour chacune des six classes de production.

## Script local

Depuis un `master` propre :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\AuthorUIRPGTalentTree.ps1 -EngineRoot D:\UE_5.5
```

Le script compile l'Editor, exécute le commandlet, exige exactement six binaires `DA_Class_*` modifiés, exécute `Grimrock.UI.RPG01.ProductionAssets`, puis la régression complète `Grimrock.RPG.RPG03`.

Les six `.uasset` ne doivent être commités qu'après lecture de la sortie utilisateur.

## Critères de fermeture

```text
ProductionAssets  : 5/5
RPG03             : 193/193 minimum
Warnings          : 0
Failures          : 0
Generated binaries: exactly 6 DA_Class_*
```

Après ce point, UI-RPG01.5 pourra fermer le jalon par la documentation/synthèse avant l'ouverture de UI-RPG02.
