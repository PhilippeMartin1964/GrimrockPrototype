# RPG-SHIELD02.4B — Restaurations C3 ciblées / groupe, équipement et plafonds

**État : implémentation C++ publiée sur master ; compilation UE5.5.4 et tests locaux attendus.**
**Contrat :** [RPG-SHIELD02 v0.2](../Rules/RPG_Shields_Protection_Contract_v0_2.md).

## Défaut corrigé

Dans `GridTurnManagerPlayerActionCatalog.cpp`, `RequestCharacterClassActionEffect` (Self) restaurait la protection physique par `Inventory->RestorePhysicalArmor`, mais `RequestCharacterBatchPartyEffect` (Ally/Party/FrontRowParty, Spell/Ability/QuickItem) créditait **directement** `TargetCharacter.Resources.CurrentPhysicalArmor`.

Exemple : Bouclier rond N1 (+5) complètement déchargé, réserve physique de classe 2/2 épuisée. Un sort de groupe restaurant quatre points accordait **4 points de base** sans restaurer le bouclier, tandis que l'autorité de `GetEffectiveCurrentPhysicalArmor` continuait d'additionner le bonus d'objet épuisé. L'inventaire conservait la mauvaise répartition de protection.

De plus, les instantanés `FGridAttackTargetStats` des cibles amies ne consultaient que `Resources.CurrentPhysicalArmor` pour les états C1, ignorant le bouclier équipé.

## Correction ciblée

- `CommitRPG033PartyArmorRestoration` est un utilitaire local et non-Blueprint partagé par les chemins **Self** et **Ally/Party**. Il applique **uniquement le delta positif** résolu par `FGridCombatArmorEffectResolver::ApplyRestoreEffects`.
- La part physique passe par `UGridPartyInventoryComponent::RestorePhysicalArmor(CharacterIndex, Delta)` : récupération des points consommés par les objets équipés avant l'armure de classe. Aucun appel à la recharge complète Victory/Rest (RPG-SHIELD02.2/.3).
- La part magique reste dans le pool `Resources.CurrentMagicalArmor`, avec addition sûre et plafond égal à `After.ReferenceMagicalArmor` autorisé par le résolveur C3. Aucun nouveau pool d'équipement magique fictif.
- Les cibles amies, avant et pendant application des statuts, sont lues avec `GetEffectiveCurrentPhysicalArmor` et reçoivent ensuite les valeurs `ArmorSnapshot` validées par le résolveur. Le traitement C1 existant reste l'autorité.
- Le pré-contrôle `WouldAnyRestore`, les coûts PA/mana/items, les multiplicateurs C3 de source, les modificateurs positifs de la cible, l'ordre de la liste des cibles, la sélection et les signaux du combat restent inchangés.

**Périmètre exclu :** aucune modification du catalogue d'armes, des profils de sorts, des statuts, des réserves à la fin du combat/repos, des dégâts de surface, de l'armure des monstres, des WBP ou de la sauvegarde. Les bonus de référence issus des niveaux d'objet, et les références physiques/magiques de C3 qui ne seraient pas équivalentes au read-model du portrait, nécessitent une caractérisation dédiée ; ne pas supposer qu'ils sont entièrement normalisés par ce jalon.

## Tests ajoutés

Filtre `Grimrock.RPG.SHIELD02.4B` (**2 tests**) :

1. `PartyRestorationUsesEquippedArmorAndCaps` : vrai `DA_Weapon_RoundShield_01` N1, trois personnages dont un inconscient, réserve physique de classe 2 et magique 3. Une capacité `Party` (4 physiques et 2 magiques) restaure d'abord 4 points **dans le bouclier**, laissant la base à 0 ; seconde application complète les 5 points du bouclier puis les 2 de base, plafonne les deux jauges, conserve les PV, l'objet, son GUID et le read-model de l'inventaire. Le personnage inconscient n'est pas restauré.
2. `ExplicitAllyRestoresOnlySelectedCharacter` : même capacité ciblée sur un seul allié, sous politique `Ally`. Elle ne restaure ni l'équipement ni les ressources physiques/magiques du lanceur ou d'un autre membre.

Les tests vérifient la véritable requête d'action du TurnManager, pas uniquement le résolveur pur. Les tests préexistants `Grimrock.RPG.RPG03.3` (Self), `Grimrock.RPG.RPG03.8` (sélection et C8), `Grimrock.RPG.SHIELD02.2` (Victory), `Grimrock.RPG.SHIELD02.3` (Rest) et `Grimrock.RPG.SHIELD02.4A` (surfaces) protègent les contrats voisins.

## Validation locale UE5.5.4

Fermer Unreal Editor, préserver les quatre assets binaires locaux, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SHIELD02.4B"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.8"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.4A"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.3"
git status --short
```

**Attendu :** 2 nouveaux tests, plus toutes les suites de non-régression sans nouvel échec. Ne jamais déclarer la validation avant les logs d'automatisation Windows.

## Recette en PIE

Dans une session autorisant une capacité de restauration physique (autorisée par le catalogue) : vider partiellement le bouclier d'un allié, lancer la restauration de groupe ou ciblée, vérifier que le bouclier récupère ses points **avant** la réserve physique de classe. S'assurer qu'un sort sur un autre personnage ne modifie pas cet allié, qu'aucune action n'augmente les pools au-delà de leur référence, et que les PV, le mana et les PA ne changent que selon les coûts/effects explicites.

**Limites restantes :** RPG-SHIELD02.5 (sauvegarde disque / cohérence intégrée) ; audit des références C3 sous modificateurs et objets de niveau supérieur.
