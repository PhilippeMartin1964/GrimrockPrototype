# ARMEMENT01.10.3.1 — Consommation réelle de l'armure physique des équipements

**État : correctif C++ publié pour validation UE5.5.4, tests locaux et PIE NON ENCORE EXÉCUTÉS.**

## Motif

Incident PIE signalé avec Dague N1 et Bouclier rond N1 (ARM075) : le Rat géant affiche "Défense : 10 + 1 = 11" et inflige 4/5 dégâts directement aux PV alors que le bouclier possède 5 points d'armure physique dans le catalogue N1.

Le moteur utilise deux étapes distinctes :
1. Jet pour toucher : `Défense = 10 + Esquive` (inchangé par ce correctif).
2. Si touche : absorption par armure physique/magique puis reliquat aux PV.

Le document RPG_Core_Rules_v0_1 propose également un bonus éventuel de bouclier dans la Défense ; cette question d'équilibrage reste une décision explicite future. **Ce ticket n'invente pas un bonus d'esquive supplémentaire de +5.**

## Source autoritaire

Le fichier `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx` alimente `DA_WeaponCatalogue`. Le DataAsset ARM075 est généré à partir de `Profile->PhysicalArmor`, avec `EquipmentStatBonus.ArmorBonus`. Aucune valeur de dégâts/armure n'est dupliquée dans les nouveaux calculs.

## Changement

Ancien comportement :
- `GetCharacterSummary` affichait `Resources.CurrentPhysicalArmor + EquipmentStatBonus.ArmorBonus`.
- Les attaques de monstres et quelques autres sources prenaient `Resources.CurrentPhysicalArmor` **sans bonus équipé**.
- Ajouter le bonus dans chaque frappe sans le consommer le ferait renaître en permanence.

Nouvelle autorité :
- `FGridItemInstance::AbsorbedEquipmentPhysicalArmor` compte l'armure déjà absorbée par **cet objet individuel** (0 sur les anciens objets et les nouvelles instances).
- `GetEffectiveCurrentPhysicalArmor` produit le total base persistée + bonus encore disponible de chaque équipement, sans dupliquer MainHand/OffHand pour les armes à deux mains.
- `ApplyPhysicalArmorDamage` consomme l'armure équipée avant la réserve de classe, sans dépasser les points disponibles.
- `RestorePhysicalArmor` restaure en priorité les bonus équipés déjà épuisés ; les règles/caps de restauration RPG03.3 sont toujours calculés par le système appelant.
- Les attaques de monstres, réactions d'interception, dégâts périodiques et dégâts dirigés vers un allié utilisent ces méthodes. Les capacités de restauration d'armure les utilisent également.
- La fiche personnage lit le **même total restant** que le combat.
- Le champ de déplétion se déplace avec l'instance d'objet dans le sac, sur le curseur et entre les équipements ; il suit le modèle de sauvegarde `FGridItemInstance`, mais une vraie partie sauvegardée et rechargée reste à tester.

**Pas de modification** du XLSX, DA_WeaponCatalogue, DA_Weapon_RoundShield_01, meshes, contenu UMG, données de dégâts, règles de mains ou classes RPG.

## Exemple prévu pour ARM075 N1 (bouclier = 5, base = 0, PV = 20)

| Attaque | Armure avant → après | PV avant → après |
|---|---:|---:|
| 4 dégâts physiques | 5 → 1 | 20 → 20 |
| 5 dégâts physiques | 1 → 0 | 20 → 16 |

Le journal compact du combat doit mentionner `Armure : -4` puis `Armure : -1`.

## Validation locale

Fermer l'éditeur Unreal puis, depuis `master` et après examen du statut Git :

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.1"
~~~

**Attendu : 3 nouveaux tests**, zéro avertissement/échec, code 0. Ces tests chargent le vrai asset ARM075 N1 : vérification du profil de 5, deux morsures, persistance à l'équipement/déséquipement et à la copie d'état, combinaison avec l'armure de classe, restauration partielle. La copie d'état **ne suffit pas** à certifier SaveGame sur disque.

Régressions obligatoires, séparément :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.RPG03.3"
~~~

Conserver les fichiers TD04 et ne pas déclarer PASS en cas de compilation ou test non effectué.

## Recette PIE ciblée

Créer une nouvelle partie de test, Guerrier N1 équipé de Dague et Bouclier rond ARM075 N1 ; vérifier l'armure physique affichée dans la fiche/portrait. Affronter un Rat géant :
1. relever les PV/armure avant la première morsure ;
2. confirmer Défense = 10 + Esquive (pas 10 + Esquive + 5) ;
3. première morsure réussie : contrôler "Armure : -N" et les transitions d'armure/PV ;
4. deuxième morsure : constater qu'une armure déjà perdue ne revient pas ;
5. retirer/rééquiper le bouclier sans remise à neuf ; vérifier son effet protecteur restant ;
6. sauvegarder, quitter, recharger, confirmer les valeurs persistantes ;
7. tester la restauration d'armure via une capacité existante si disponible.

**Clôture seulement après** compilation, tests automatisés et témoignage PIE.
