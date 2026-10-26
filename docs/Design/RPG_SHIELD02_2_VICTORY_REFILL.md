# RPG-SHIELD02.2 — Recharge des armures à la victoire

**État : C++ publié sur master ; compilation, automatisation UE5.5.4 et recette PIE en attente de validation par les logs utilisateur.**  
**Autorité :** [Contrat v0.2](../Rules/RPG_Shields_Protection_Contract_v0_2.md).

## Fonctionnement mis en place

Après une transition **normale et autoritaire** de `UGridTurnManagerComponent::FinishCombat(Victory)`, le composant d'inventaire exécute `RefillArmorReservesAfterVictory()` **avant** `OnCombatEnded`. Le même calcul de référence d'armure physique/magique que celui du HUD portrait est utilisé : aucune seconde définition de maximum n'est introduite.

Pour chaque personnage actif **vivant**, l'opération :
- rétablit `Resources.CurrentPhysicalArmor` à la **partie base** de la référence physique (la contribution des équipements est toujours ajoutée séparément) ;
- rétablit `Resources.CurrentMagicalArmor` à la référence magique ;
- remet à zéro `AbsorbedEquipmentPhysicalArmor` sur ses objets équipés **et dans son sac** ;
- préserve les `RuntimeObjectId`, quantités, emplacements, ressources de PV/mana, statuts et compétences ;
- notifie le read-model UI en cas de changement, avant la diffusion de la fin du combat.

Le curseur est un stockage possédé par le groupe (`OwnerCharacterIndex == INDEX_NONE`). Son instance est remise à neuf lorsqu'au moins un membre du groupe est vivant. Elle n'est pas dupliquée ni attribuée fictivement à un personnage ; sa position/identité reste intacte.

Les instances sont parcourues avec une déduplication par GUID pour éviter une double remise à zéro accidentelle. Un personnage vaincu ou dont la classe autoritaire ne peut être résolue ne reçoit **aucun maximum inventé** ; ce dernier cas produit un avertissement diagnostic.

Le combat terminé en `Defeat`, `AbortCombat()` et la commande de diagnostic `ForceVictory()` **ne rechargent pas** les réserves. Le raccourci `ForceVictory` passe un argument explicite `bDiagnosticVictory = true` à `FinishCombat` ; les chemins normaux continuent à utiliser la valeur implicite `false`.

**Attention** : la méthode non-Blueprint `RefillArmorReservesAfterVictory()` reste un service C++ interne : ne pas l'appeler depuis les opérations ordinaires de l'inventaire, l'équipement, le chargement de partie ou les effets C3.

## Tests inclus

- `Grimrock.RPG.SHIELD02.2.VictoryRefillsPhysicalAndMagical` : véritable asset `DA_Weapon_RoundShield_01` (+5 N1), classe synthétique (+2 base physique, +3 magique), shield équipé épuisé, shield en sac épuisé, shield sur curseur épuisé, restauration, UI, GUID, PV/mana inchangés, idempotence et deux combats successifs simulés.
- `Grimrock.RPG.SHIELD02.2.DownedCharactersAreNotRefilled` : personnage avec PV=0, armure et bouclier épuisés, pas de soin ni restauration d'un curseur sans survivant.

Ces tests vérifient le **service de restauration**, sans prétendre remplacer une recette PIE de la transition d'état réelle ou une validation SaveGame sur disque.

## Commandes locales (Unreal Editor fermé)

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.3"
git status --short
```

**Attendus : 2/2 nouveaux tests, 3/3 ARMEMENT01.10.3.1 et 8/8 RPG03.3**. Aucun succès local ne peut être affirmé avant les rapports UE.

## Recette PIE

1. Dague et Bouclier rond N1 : relever l'armure physique 5/5 initiale, les PV, le mana et l'armure magique.
2. Subir des morsures de Rat géant : la protection diminue **pendant** le combat et ne se restaure pas spontanément entre deux tours.
3. Gagner normalement la rencontre : la réserve physique retourne à 5/5 et la magique à sa référence, sans changer les PV/mana.
4. Affronter un deuxième monstre : la réserve recommence pleine, sans bonus doublé.
5. Si possible, avec un shield usé dans le sac ou sur curseur à l'instant de victoire, vérifier sa recharge au rééquipement ; ne pas jeter/recréer l'objet.
6. Inspecter le journal de combat, portrait, fiche et tooltip ; les modificateurs de classe/talents restent autoritaires.
7. Vérifier `ForceVictory` debug, `AbortCombat`, défaite et SaveGame selon le support de la version.

## Limites non traitées ici

- **RPG-SHIELD02.3** : l'action de repos explicite n'existe pas ; `IsPartyAtRest` (immobilité sur grille) n'est pas un repos.
- **RPG-SHIELD02.4** : les dégâts de surface et certaines actions C3 ciblées passent encore par des écritures directes de la réserve physique au lieu de l'API d'absorption/restauration d'équipement.
- **RPG-SHIELD02.5** : vérification exhaustive de persistance sur disque, références sujettes aux effets temporaires, variantes N2–N20.

Aucune modification de WBP, DataAsset, `.uasset`, catalogue Excel, `ST_UI` ni UI n'est incluse dans RPG-SHIELD02.2.
