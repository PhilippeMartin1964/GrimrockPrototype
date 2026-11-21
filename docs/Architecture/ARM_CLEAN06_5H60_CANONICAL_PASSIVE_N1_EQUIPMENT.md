# ARM-CLEAN06.5H.60 — Passifs ARM070 / ARM075 / ARM081 canoniques

Le test `Grimrock.RPG.ARMEMENT.LOT02.16.5.PassiveN1Execution` résout `Weapon_ArcaneOrb_01`, `Weapon_RoundShield_01` et `Weapon_HolyAegis_01` au niveau N1 depuis `UGridItemSubsystem`, attaché par `FGridCanonicalLootTestScope` au monde MON12 réel.

Le scénario ne charge plus `DA_Weapon_*` et n'enregistre aucune ancienne définition. Il valide les identités, les archétypes, la nature passive, le slot OffHand et l'absence d'action d'attaque. L'armure nominale physique/magique provient de `FGridResolvedItem.Weapon`, tandis que l'usure reste sur l'instance physique.

Contrôles conservés : équipement depuis le sac, absence de raccourci offensif, armure effective sans duplication, absorption d'un point physique ou magique si disponible, compteurs d'usure, GUID identique après retrait/rééquipement et interdiction de recharge gratuite.

Aucun changement de runtime, Blueprint, `.uasset`, `.umap` ou `.sav`; les 53 packages historiques H.23 restent présents. H.61 est bloqué jusqu'aux validations Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` toutes deux SUCCESS sur H.60 (Editor, Shipping, Automation `Grimrock.RPG.ARMEMENT.LOT02.16.5.PassiveN1Execution`). Les sept sauvegardes natives nécessitent toujours l'audit préalable à toute suppression.
