# RPG-SHIELD02.4A — Dégâts de surface et réserve physique effective

**État : C++ et automatisation publiés sur master ; build et validation UE5.5.4 locale attendus.**  
**Contrat :** [RPG-SHIELD02 v0.2](../Rules/RPG_Shields_Protection_Contract_v0_2.md).  
**Périmètre strict :** surfaces périodiques touchant les personnages ; résolution d'attaque, états, surfaces affectant les monstres, sorts de restauration et UMG inchangés.

## Diagnostic confirmé

Dans `UGridTurnManagerComponent::ResolveCombatSurfaceRound()`, la résolution des dégâts sur le groupe ne lisait que `Character.Resources.CurrentPhysicalArmor`, ignorant les protections des boucliers et armures équipés.

En outre, elle diminuait directement `Resources.CurrentPhysicalArmor`, sans passer par `UGridPartyInventoryComponent::ApplyPhysicalArmorDamage()` : `FGridItemInstance::AbsorbedEquipmentPhysicalArmor` ne progressait pas, et la réserve physique effective affichée dans les infobulles ne correspondait pas aux dégâts de surface reçus.

**Exemple du défaut :** bouclier rond N1 +5, base physique 2, feu/ou surface physique à 4 dégâts ; l'ancien calcul débitait 2 points de base, infligeait 2 PV, tout en laissant le bouclier intact. L'attaque normale aurait absorbé les 4 dégâts dans le bouclier.

## Correction minimale

Le chemin des **surfaces touchant un personnage** est désormais aligné sur le commit normal des attaques de monstre :

```cpp
TargetBefore.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(CharacterIndex);
// ... modificateurs, résistances et ResolveDirectDamage inchangés ...
Inventory->ApplyPhysicalArmorDamage(
    CharacterIndex, Damage.PhysicalArmorDamage, &Damage.Trace.Damage.PhysicalArmorSources);
```

Le résultat calculé conserve ses champs `PhysicalArmorDamage`, `MagicalArmorDamage`, `HealthDamage` et ses traces. L'API d'inventaire consomme d'abord les points physiques des objets équipés, puis ceux de la classe ; elle conserve la déplétion sur la véritable instance et notifie l'inventaire. Les dommages aux PV restent la partie excédentaire calculée par `FGridCombatResolver`.

Le chemin **magique** est laissé inchangé : dommages Fire, Ice, Lightning, Poison, etc. consomment `Resources.CurrentMagicalArmor` (sans attaquer la réserve physique des équipements) puis les PV. Les protections magiques provenant d'items ne sont pas ajoutées sans autorité catalogue explicite.

Les **résistances** et `FGridCombatModifierResolver::ApplyIncomingAttackModifiers()` demeurent appliqués avant la séparation des dégâts. Les `PeriodicStatusApplications` continuent de recevoir **le même** `TargetBefore` et `Damage` pour les armor gates C1 : l'armure physique effective figure désormais bien dans `TargetBefore` pour les surfaces physiques.

Les **surfaces touchant les monstres** restent traitées via `Monster->ApplyAttackResult(Damage)`, sans transformation.

## Nouveaux tests d'intégration

Filtre : `Grimrock.RPG.SHIELD02.4A` (**3 tests attendus**).

1. `PhysicalSurfaceConsumesEquippedArmorBeforeHP` : vrai asset `DA_Weapon_RoundShield_01` N1 ; classe synthétique base physique 2 et armure magique 3. Une surface physique inflige 4 points absorbés par le bouclier, laissant 20 PV et 3 points de protection effective ; un second tick inflige 5 points, vide le bouclier et la base et ne retire que 2 PV. Le GUID du bouclier, la magie et la vue inventaire restent cohérents.
2. `MagicalSurfaceUsesOnlyMagicPool` : surface de feu, magie 3/3, ticks de 2 puis 4 : magie 1/3 puis 0/3, PV 20 puis 17, bouclier à 5/5 et base physique intacte.
3. `ZeroArmorAndOffCellDoNotInventProtection` : la surface sur une autre case n'affecte pas le groupe ; avec les deux réserves à zéro, les dégâts physiques vont directement aux PV sans inventer d'armure.

Ces tests appellent **le véritable `ResolveCombatSurfaceRound()`** sur un petit monde UE avec vraie surface persistante, vrai bouclier, party et TurnManager. La fonction reste privée : seuls les trois tests sont déclarés `friend` dans l'en-tête.

## Validation locale : Unreal Editor fermé

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SHIELD02.4A"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.6"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.3"
git status --short
```

**Attendu : 3/3 + 8/8 + 3/3 + 2/2 + 2/2 + 8/8 = 26/26,** sans nouvel échec ni avertissement. Aucun test n'est déclaré réussi tant que le log UE5.5.4 n'est pas fourni.

## Recette en PIE

- Installer une surface *physique authorée* sous le groupe, avec un bouclier rond N1 équipé ; comparer la protection avant/après un tick, PV, armure et icône tooltip.
- Vérifier qu'une surface magique consomme l'armure magique mais **pas** les cinq points physiques du bouclier.
- Vérifier la fin de combat (recharge après victoire), puis le repos explicite via `GrimrockRest` hors combat.
- Vérifier les états C1 par surface et les éventuels dégâts de surface sur les monstres.
- Le SaveGame disque complet reste le jalon RPG-SHIELD02.5.

### Exclusions

Aucun changement de durabilité ou d'équipement, d'armure de classe, de formule `ResolveDirectDamage`, de vitesse d'animation des surfaces ou de contrat DOS2. Aucune modification WBP, DataAsset, fichier `.uasset`, CSV, ni texte de localisation.

Les **restaurations C3 / multi-cibles** demeurent **RPG-SHIELD02.4B**. Il est interdit d'en profiter pour corriger ce chemin dans le présent commit.
