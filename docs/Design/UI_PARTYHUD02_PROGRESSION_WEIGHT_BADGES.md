# UI-PARTYHUD02 — Indicateurs de progression et surcharge sur les portraits

Statut : implémentation C++ publiée ; tests locaux et validation visuelle PIE attendus (UE 5.5.4).

## Choix d'interface

- Suppression de la popup **montée de niveau**, sans supprimer les événements `FRPGLevelUpService`, les niveaux, les points RPG ni leur sauvegarde.
- `Image_LevelUpAlert` est une image UMG d'un **+ dessiné** ; elle reste visible lorsqu'au moins un point de compétence, de talent ou d'attribut reste à attribuer. Le symbole disparaît uniquement quand les trois soldes sont nuls ou invalides. À niveau 1, le RPG attribue déjà quatre points de compétence : le + peut donc apparaître immédiatement.
- `Image_WeightAlert` réutilise l'icône de poids et le seuil existant `Overloaded`. `Heavy` ne déclenche pas l'alerte.
- Les deux images sont indépendantes et peuvent coexister. Elles doivent être `Hit Test Invisible` lorsqu'elles sont affichées pour préserver le clic et le drag & drop du portrait.

## Source des données

`UGridPartyInventoryComponent::GetPartyPortraitViews()` fournit, pour chaque `CharacterId`, les booléens de présentation non persistés :
`bHasUnspentProgressionPoints` (soldes canoniques `FRPGSkillPointService::TryGetBalance`, `FRPGAttributePointService::TryGetBalance`, `FRPGClassProgressionService::TryGetChoicePointBalance`) et `bOverloaded` (depuis `GetCharacterSummary()`).

Aucun compteur temporaire, acknowledgement, polling Tick ou propriété sauvegardée n'est ajouté. `OnPartyInventoryChanged` pilote déjà la reconstruction des portraits après gains/dépenses/équipement. Les états se reconstruisent aussi au rechargement et après un réordonnancement.

## WBP_PartyPortraitHud — Designer UE5

Dans l'Overlay existant qui contient `Image_Portrait`, créer deux `Image` natives, `Is Variable = true` :

1. `Image_LevelUpAlert` avec un brush RGBA d'un signe « + » médiéval, dans un coin du portrait, par défaut `Collapsed`.
2. `Image_WeightAlert` avec la même texture `T_Weight` que `WBP_PartyMember`, dans un autre coin, par défaut `Collapsed`.

Ne pas créer de TextBlock Unicode « + » et ne pas réutiliser la police pour dessiner le symbole. Donner aux deux images `Not Hit-Testable (Self & All Children)` ; le C++ contrôle leur visibilité. Ne pas recouvrir le cadre de sélection, les barres ni les zones d'interaction. Adapter positions/taille après capture PIE.

Le `WBP_RPGNotification` historique ne doit plus être sollicité lors d'un Level Up. Son autre usage générique (feedback de dépense d'attribut) reste intact : **ne pas supprimer l'asset UMG** tant que ses références n'ont pas été migrées.

## Validation

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.PartyHud01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.LEVELUX01"
```

Puis vérifier PIE : personnage avec points restants (plus affiché), dépense complète (plus masqué), nouvel échelon (plus réaffiché), sauvegarde/rechargement, surcharge et désurcharge, deux icônes simultanées, clic/sélection/drag du portrait, aucune popup en haut à droite lors du Level Up.
