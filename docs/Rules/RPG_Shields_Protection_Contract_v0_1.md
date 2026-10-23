# RPG — Contrat général des boucliers et de la protection (v0.1)

**Décision :** ARMEMENT01.10.3.2 — 23 octobre 2026  
**Portée :** tous les boucliers, tous archétypes et niveaux d'objet (N1 à N20)  
**Statut :** règle RPG de référence ; distinguer systématiquement fonctionnalités en place et intentions futures. Ce jalon est **documentaire** : aucun gameplay, asset ou catalogue modifié.

## 1. Trois rôles à distinguer

Un bouclier est un **équipement défensif passif** qui apporte principalement une réserve d'**armure physique absorbante**. Il n'accorde **ni bonus automatique au jet de Défense, ni jet de blocage, ni attaque gratuite**.

| Rôle | Règle | Comportement actuel |
|---|---|---|
| Défense / chance d'être touché | `10 + Esquive effective` ; modificateurs explicitement authorés seulement | Oui, sans bonus passif automatique du bouclier |
| Protection physique | Réserve fixe selon le profil de l'item équipable ; consommée avant les PV | Oui pour le bouclier rond ARM075 N1 |
| Actions spéciales | Parade, garde, riposte, Coup de bouclier si talent/action le permet | Uniquement selon les actions réellement authorées ; aucune action passive ajoutée par cette règle |

**Interdiction :** `PhysicalArmor=5` ne veut **jamais** dire `+5 Défense` ni `+5 Esquive`.

## 2. Source autoritaire : catalogue ARMEMENT01

Classeur de production : `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`. Autorité chargée en jeu : `UGridWeaponCatalogueAsset`, `FGridWeaponLevelProfile` (`ArchetypeId`, `ItemLevel`, `HandUsage`, `PhysicalArmor`, `MagicalArmor`).

- Un bouclier récupéré possède un **niveau d'objet fixe** N1–N20. Le niveau RPG du personnage ne recalcule pas implicitement son armure.
- L'ItemDefinition équipée doit correspondre au **profil de son niveau d'objet**. Le pilote `ARM075` N1 (`DA_Weapon_RoundShield_01`) copie les **5 points** de `Profile->PhysicalArmor` dans `EquipmentStatBonus.ArmorBonus`.
- L'existence de profils N2–N20 dans le catalogue **ne prouve pas** que leurs ItemDefinitions équipables sont déjà générées ; la correspondance niveau/armure reste à vérifier à l'authoring de chacun.
- Le profil contient également `MagicalArmor`, mais **l'authoring pilote ARM075 observé ne copie que l'armure physique**. Ne déclarer un bouclier magique opérationnel que lorsque son DataAsset, son mapping et sa résolution auront été vérifiés.
- Les boucliers **passifs** ne possèdent pas de dés d'attaque par défaut : jamais de faux `0d0` ni de jet de blocage automatique.

La protection du bouclier n'est ni un multiplicateur de dégâts, ni une résistance en pourcentage, ni un modificateur de précision.

## 3. Calcul de la Défense : règle effective

Le résolveur courant (`GridCombatResolver.cpp`) calcule :

```text
Attaque = d20 + modificateurs de précision / attaque
Défense = 10 + Esquive effective
Touché = résolution du jet, y compris règles de réussite/échec naturels
```

L'ancienne formulation « Défense = 10 + esquive + bonus d'armure légère + bouclier éventuel » signifie désormais que ces derniers bonus **peuvent exister uniquement en tant qu'effets distincts et explicitement authorés**, jamais automatiquement par la valeur d'armure de base. Un effet `+1 Défense` d'un futur set ne résulte donc pas d'un `ArmorBonus=1` : il doit fournir un modificateur de défense/esquive dédié.

## 4. Absorption physique et traçabilité par instance

Ordre général d'une attaque autorisée :
1. Validité de l'attaque, de la cible et jet pour toucher contre `Défense`.
2. Calcul des dégâts et des modificateurs, notamment multiplicateurs et résistances.
3. En cas de touche : consommation de l'**armure correspondante** puis application du seul excédent aux PV.
4. Application éventuelle des états, selon leurs propres conditions.

Les dégâts **physiques** utilisent la réserve d'armure **physique**. Les autres types utilisent normalement l'armure **magique** et leurs résistances ; un bouclier ordinaire ne convertit pas les dégâts magiques en dégâts physiques.

```text
Armure physique effective
  = Ressources.CurrentPhysicalArmor du personnage
  + somme des protections physiques encore disponibles des équipements légaux

Protection restante d'une instance avec bonus physique positif
  = max(0, ArmorBonus - AbsorbedEquipmentPhysicalArmor)

Armure absorbée = min(Dégâts après modificateurs et résistances, armure disponible)
PV perdus = max(0, dégâts restants après armure)
```

Le système actuel utilise `GetEffectiveCurrentPhysicalArmor`, `ApplyPhysicalArmorDamage` et `RestorePhysicalArmor`. L'armure des **équipements** est consommée avant la réserve de base du personnage ; **la jauge effective est commune**, il ne faut pas confondre cette priorité interne avec des pools RPG indépendants.

L'instance d'équipement porte `FGridItemInstance::AbsorbedEquipmentPhysicalArmor`. Son état suit son `RuntimeObjectId` entre équipement, inventaire et curseur : déséquiper/rééquiper ne recharge pas l'armure. Un état partiellement absorbé n'est **pas** un objet détruit ; aucune durabilité ni réparation automatique n'est introduite. La récupération requiert un **effet de restauration autorisé**. Les mécanismes de restauration existants imposent leurs propres plafonds.

La sérialisation de la propriété est disponible dans l'instance ; **le cycle SaveGame sur disque et le rechargement du niveau restent à valider manuellement**.

## 5. Exemple : Bouclier rond ARM075 N1

Hypothèses : Elias possède 20 PV, aucune armure de classe, et le bouclier fournit 5 points intacts.

| Événement | Protection encore disponible | PV |
|---|---:|---:|
| Avant les attaques | 5 / 5 | 20 |
| Morsure réussie, 4 dégâts physiques | 1 / 5 | 20 |
| Morsure réussie, 5 dégâts physiques | 0 / 5 | 16 |

La Défense peut rester `10 + 1 = 11` pendant tout l'exemple. Le journal peut afficher `Armure : -4; PV Elias : 20 → 20`. Si plusieurs équipements et la classe protègent Elias, cette ligne de journal décrit l'**absorption totale**, pas forcément le seul bouclier.

## 6. Garde, parade, orientation et types de dégâts

**Comportement actuel :**
- L'armure passive du bouclier n'a pas de restriction frontale automatique, ni contre les attaques de flanc ou d'arrière ; sa disponibilité dépend de l'équipement réellement porté et du type de dégâts.
- Un bouclier n'est pas équipé simultanément avec une arme à deux mains qui réserve `OffHand`.
- Pas de dé de blocage, de parade gratuite, de consommation de PA par morsure, de riposte automatique.
- La protection physique de base n'accorde pas de protection magique, de résistance élémentaire ou d'immunité mentale.
- `Shield Bash` / **Coup de bouclier** dépend d'une action ou d'un talent expressément autorisé ; ses dégâts, coûts, recharge et prérequis sont indépendants des points de protection passive.

**Intentions de conception futures à ne pas considérer comme implémentées :**
- garde frontale, contre-projectiles pour un pavois, bouclage latéral limité ;
- blocs et parades distincts selon les types tranchant/perforant/contondant ;
- états `Brise-garde`, renversement, usure, capacités de contre-attaque ;
- bonus particuliers de `Défense` ou de résistance pour boucliers enchantés, talents et sets.

Chacun exige un paramètre métier propre, des règles de cumul et des tests. Ne jamais déduire ces effets du nom, du mesh ou de `PhysicalArmor`.

## 7. Affichage, validation et limitations connues

- **En place :** le journal `Défense : 10 + 1 = 11`, puis `Armure : -4; PV Elias : 11 → 11` décrit correctement une attaque touchée dont l'armure absorbe les dégâts. La fiche du personnage lit l'armure effective de combat.
- **Non en place :** ventilation du log par item (`Bouclier rond : -4`), détail `1 / 5` restant par bouclier, bonus de parade positionnelle automatique.
- **Tests fournis le 23.10.2026 :** ARMEMENT01.10.3.1 **3/3**, ARMEMENT01.10 **14/14**, TD07.3.3.3 **8/8**, RPG03.3 **8/8**, aucun échec. Capture PIE : 4 dégâts du Rat géant entièrement absorbés par l'armure, sans perte de PV.
- **Non encore démontré :** scénario exhaustif des boucliers N1–N20, sauvegarde/chargement sur disque de protections partiellement consommées, cas complexes de restauration et de magie.

## 8. Documents associés

- [Règles de base RPG](RPG_Core_Rules_v0_1.md)
- [Types de dégâts et contre-mesures](RPG_Damage_Types_Countermeasures_v0_1.md)
- [Correctif C++ ARMEMENT01.10.3.1](../Design/ARMEMENT01_10_3_1_EQUIPMENT_PHYSICAL_ARMOR.md)
- [Contrat d'armement historique v0.3](../Design/ARMEMENT01_CONTRAT_ARMEMENT_V0_3.md)
- [Effets d'armure RPG03.3](../Design/RPG03_3_ARMOR_EFFECTS.md)
