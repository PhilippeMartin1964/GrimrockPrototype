# ARM-CLEAN06.5H.51 — Comparaisons ARM002 / ARM075 sans ancien catalogue

## Conversion

`Grimrock.UI.WeaponTooltip04.RealWeaponComparison` ne charge plus `DA_WeaponCatalogue`, `DA_Weapon_AssassinDagger_01` ou `DA_Weapon_RoundShield_01`. Une fixture `FGridCanonicalRegistryFixture` fournit les quatre profils réels : ARM002 N9/N1 et ARM075 N12/N1. `FGridWeaponTooltipProjection::BuildComparison` est appelée avec deux pointeurs de définitions historiques nuls et le `UGridItemSubsystem` de production.

La comparaison de dégâts utilise les dés canoniques N9 moins N1. Les formules textuelles de dés restent non arithmétiques, un bouclier passif n'acquiert pas d'attaque, la comparaison offensive contre un bouclier reste impossible, et le delta d'armure est celui des deux niveaux canoniques N12/N1.

## Préservation

Tous les tests du fichier autres que `RealWeaponComparison` sont inchangés, y compris les scénarios synthétiques caractérisant l'ancien format. Aucun code runtime/UI, `.uasset`, `.umap`, `.sav` ou fichier de production n'est modifié; les 53 packages H.23 restent présents.

Avant H.52, exiger `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, compilations Editor/Shipping et Automation `Grimrock.UI.WeaponTooltip04`. Aucun retrait de packages sans nouvel audit natif des sept sauvegardes.
