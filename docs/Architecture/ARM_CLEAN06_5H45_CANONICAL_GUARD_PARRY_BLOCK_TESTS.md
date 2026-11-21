# ARM-CLEAN06.5H.45 — Parade ARM013 et blocage ARM075 via ItemSystem

## Conversion

Les tests `Grimrock.RPG.ARMEMENT.LOT02.19.3.MainGaucheParryEligibility` et `ShieldBlockActualAbsorption` ne chargent plus `DA_Weapon_MainGauche_01` ni `DA_Weapon_RoundShield_01`. Une fixture `FGridCanonicalWeightTestWorld` fournit le registre de production via son GameInstance et un véritable inventaire lié au monde.

La Main-gauche `Weapon_MainGauche_01` N1 est vérifiée comme arme utilisable en OffHand. Le bouclier `Weapon_RoundShield_01` N1 est vérifié en OffHand avec 5 points physiques. Aucune ancienne définition n'est enregistrée dans l'inventaire ; les objets équipés sont définis par leurs seules identités canoniques N1.

## Invariants conservés

La parade reste refusée pour coup critique, échec, dégâts magiques, attaque à distance, arme absente ou placée seulement en MainHand ; la réussite de dés 1–100 et la neutralisation des dégâts sont conservées. Le bouclier n'annonce que les points d'armure effectivement absorbés, conserve l'usure après coups successifs et refuse les dégâts magiques/les boucliers retirés. Les journaux de combat restent contrôlés.

Aucun changement gameplay, HUD, `.uasset`, `.umap`, `.sav` ou ressource sonore. Aucun des 53 packages historiques H.23 n'est supprimé.

H.46 est bloqué jusqu'aux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur ce SHA, compilations Editor + Shipping et Automation `Grimrock.RPG.ARMEMENT.LOT02.19.3`. Les sept sauvegardes nécessitent toujours un audit natif avant suppression.
