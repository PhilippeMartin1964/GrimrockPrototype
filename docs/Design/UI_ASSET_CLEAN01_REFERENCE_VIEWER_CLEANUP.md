# UI-ASSET-CLEAN01 — Reference Viewer cleanup

Date : **22 septembre 2026**  
Statut : **AUDIT READ-ONLY PRÊT — validation locale requise avant toute suppression**

## Objectif

Confirmer les referencers réels des neuf candidats issus de UI-ASSET-AUDIT01, puis supprimer uniquement les assets réellement morts.

Aucun `.uasset` n'est supprimé à l'aveugle.

## Candidats

```text
Buttons/TopTabs/T_ButtonTab_Normal_480x100
Buttons/TopTabs/T_ButtonTab_Hovered_480x100
Buttons/TopTabs/T_ButtonTab_Pressed_480x100
Buttons/TopTabs/T_ButtonTab_Disabled_480x100
Buttons/TopTabs/T_ButtonTab_Selected_480x100
WBP_ItemTooltipComparisonRow
Icons/T_BorderCharacter
Icons/T_Border_Character
Buttons/T_RootFrame
```

## Étape 1 — audit AssetRegistry automatique

Le test éditeur :

```text
Grimrock.Editor.UIAssetClean01.ReferenceAudit
```

interroge l'AssetRegistry on-disk avec toutes les catégories de dépendance et affiche, pour chaque candidat :

```text
Exists
ReferencerQuery
Referencers=N
DeletionReady=true|false
```

Il ne modifie aucun asset.

`DeletionReady=true` signifie uniquement : l'asset existe et l'AssetRegistry ne trouve aucun referencer. Ce résultat doit encore être confirmé une fois dans le **Reference Viewer** avant suppression physique.

## Étape 2 — confirmation Reference Viewer

Pour chaque candidat marqué `DeletionReady=true` :

1. Content Browser -> sélectionner l'asset.
2. Clic droit -> **Reference Viewer**.
3. Vérifier la partie **Referencers**.
4. Si aucun referencer runtime/projet utile n'existe, l'asset peut être supprimé.
5. S'il existe un referencer, conserver l'asset et noter le chemin.

Pour `T_BorderCharacter` / `T_Border_Character`, comparer également les deux textures avant suppression : le but est de conserver la variante réellement utilisée, pas de supprimer arbitrairement l'une des deux.

## Étape 3 — suppression

La suppression doit être faite depuis Unreal Editor.

Après chaque lot :

```text
Save All
Content/GrimrockPrototype/Blueprints/UI -> Fix Up Redirectors in Folder
Save All
```

Ne pas déplacer les assets pendant ce ticket. UI-ASSET-CLEAN01 est uniquement un nettoyage.

## Étape 4 — validation

Après suppression :

- rouvrir les surfaces touchées dans l'éditeur ;
- smoke PIE inventaire/tooltip ;
- smoke PIE navigation + HUD ;
- smoke PIE curseur monde ;
- si TopTabs est supprimé, vérifier que Skills/Spellbook/Journal/Map/Recipes/Codex continuent à s'ouvrir depuis la barre basse.

Relancer ensuite l'audit :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.UIAssetClean01"
```

Les assets supprimés apparaîtront alors avec `Exists=false`. Les assets conservés continueront à lister leurs referencers.

## Critère de clôture

UI-ASSET-CLEAN01 est clos lorsque :

- les neuf candidats ont été classés `SUPPRIMÉ` ou `CONSERVÉ` avec justification ;
- les suppressions ont été réalisées dans UE5 ;
- les redirectors ont été corrigés ;
- le test read-only repasse ;
- les smoke PIE concernés sont verts.
