# RPG02 — Mécanique des 90 Talents de classe — v0.1

Date : **5 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : `RPG_Class_Progression_1_20_v0_1.md`  
Statut : **SPÉCIFICATION MÉCANIQUE AUTORITAIRE — IMPLEMENTATION À VENIR**  
Nombre de Talents : **90 = 6 classes × 3 branches × 5 paliers**

## 1. Objet

RPG01 fixe les arbres et les niveaux d'accès. RPG02 fixe maintenant le comportement de chacun des 90 Talents : identité, prérequis, coût, ciblage, cooldown, dégâts/soins, ArmorGate, statuts et dépendances techniques.

Les valeurs sont un **baseline de balance** destiné à être testable. Elles ne doivent pas être dispersées dans le code ; elles devront vivre dans les DataAssets/structures existants ou dans de petites extensions data-driven génériques.

## 2. Invariants du projet conservés

- **4 PA de base** par personnage et par tour ; les actions restent dans la borne 0..6 du contrat existant.
- Talents = `FRPGClassProgressionChoiceDefinition` ; aucun second système de Talent.
- Le `ChoiceId` sélectionné est déjà un RequirementId satisfait : les actions utilisent donc directement le `ChoiceId` dans `FGridCombatActionDefinition::Requirements`.
- Une action martiale de Talent est une contribution de classe, généralement `SourcePolicy=Ability`.
- Une action magique de Mage/Prêtre est une contribution de classe `SourcePolicy=Spell` afin que Silence continue de fonctionner sans taxonomie parallèle.
- Les QuickItems d'Alchimiste restent des items réels et consomment `SourceItemQuantityCost=1`.
- Physical damage → PhysicalArmor → HP ; tous les autres DamageTypes actuels → MagicalArmor → HP.
- Un contrôle physique dangereux ne s'applique que si **PhysicalArmor=0 après les dégâts de l'action**.
- Un contrôle magique/toxique/mystique dangereux ne s'applique que si **MagicalArmor=0 après les dégâts de l'action**.
- Marque, posture et buffs alliés ne sont pas des contrôles hostiles et n'utilisent pas d'ArmorGate.
- Aucun RequirementId dérivé n'est sauvegardé.

## 3. Conventions de calcul

### 3.1 Dégâts d'arme

`WD` = dégâts de l'attaque de l'arme actuellement utilisée, après son roll Min/Max, bonus plat et attribut de scaling, **avant** le coefficient du Talent.

Exemple : un Talent à 150 % WD multiplie le RawDamage de l'attaque par 1,50 avant armure/résistance.

Les multiplicateurs sont arrondis à l'entier inférieur avec minimum 1 lorsqu'une attaque ayant touché produit un RawDamage positif.

### 3.2 Magie

Les magnitudes de RPG02 utilisent les modificateurs déjà définis par :

`floor((Attribut - 10) / 2)`

et le Rank du Skill indiqué. Un résultat de soin/dégât calculé inférieur à 1 est ramené à 1.

### 3.3 Cooldown

`CooldownRounds=N` suit le contrat actuel : après utilisation, N rounds complets doivent s'écouler avant nouvelle disponibilité.

### 3.4 Critiques

Les actions d'arme peuvent critiquer sauf mention contraire. Les sorts et effets directs ne critiquent pas en v0.1 sauf règle future explicite.

### 3.5 DamageTypes réellement disponibles

RPG02 utilise l'enum runtime actuel uniquement :

`Physical, Fire, Ice, Lightning, Poison, Holy, Necrotic, Arcane`.

Le document historique Damage Types mentionne aussi Earth/Air/Water/Psychic comme vocabulaire étendu, mais ces valeurs **n'existent pas** actuellement dans `EGridDamageType`. Une School `Earth` peut donc produire Physical ou Poison selon le sort, sans inventer un neuvième type dans RPG02.

## 4. Codes de dépendance technique

RPG02 définit le **comportement cible**, pas encore son implémentation. Les codes indiquent la primitive générique nécessaire au-delà du shell d'action déjà existant :

| Code | Besoin générique | État audité |
|---|---|---|
| `C0` | Shell action : PA, mana, ciblage, portée, zone, cooldown, Requirements | existe |
| `C1` | Application d'un Status depuis action/attaque + ArmorGate post-dégâts | Status system existe, pont générique à compléter |
| `C2` | Modificateurs génériques dégâts/Accuracy/Evasion/crit/résistance/coûts | **implémenté par RPG03.1 — validation UE utilisateur requise** |
| `C3` | Modification/restauration directe des pools PhysicalArmor/MagicalArmor | à ajouter |
| `C4` | Triggers/réactions : once-per-round, on-hit, on-miss, on-kill, consume-on-action | à ajouter |
| `C5` | Déplacement tactique/forced movement/formation | à compléter |
| `C6` | Surfaces persistantes et réactions élémentaires | non implémenté comme système RPG autoritaire |
| `C7` | Recettes/paramètres d'alchimie et modification générique de QuickItems | à compléter |
| `C8` | Batch multi-cible, filtre de statut, targetability ou sélection secondaire | à compléter |

Aucun de ces codes n'autorise un `switch(TalentId)` de 90 cas. Les extensions futures doivent être **data-driven et réutilisables**.


## 4.1 RPG03.1 — C2 implémenté

Le contrat C2 est désormais porté par `FGridCombatModifierProfile`, authorable directement sur un `FRPGClassProgressionChoiceDefinition`.

Le runtime reconstruit les profils actifs depuis `SelectedClassProgressionChoiceIds` ; aucun agrégat de combat supplémentaire n'est sauvegardé. Le catalogue applique PA/mana/portée sur sa copie runtime, tandis que les attaques utilisent le même resolver pour Accuracy, Evasion, dégâts, critiques et résistances.

Le pont `StatusEffect -> CombatModifierProfile` reste volontairement hors RPG03.1 et appartient à RPG03.2/C1.

## 5. Identité et prérequis

Règle commune :

- `MinimumLevel` = 2 / 6 / 10 / 14 / 18 ;
- `PointCost=1` ;
- le palier I n'a pas de prérequis de branche ;
- chaque palier II→V exige le `ChoiceId` précédent ;
- les variantes explicites (spécialisation d'arme, affinité élémentaire, ennemi juré) utilisent des ChoiceIds distincts mais ne changent pas le nombre de paliers.

## Guerrier

### Gardien

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Posture défensive**<br>`Talent_Warrior_Guardian_DefensiveStance` | `—` | Active | `Action_Warrior_DefensiveStance` | 1 / 0 | Self | 3 | Applique `Status_Guarded` pendant 2 rounds : dégâts physiques reçus -20 %, Evasion +2, dégâts d'arme infligés -10 %. Réapplication = RefreshDuration. | C2 |
| 2 (niv.6) | **Coup de bouclier**<br>`Talent_Warrior_Guardian_ShieldBash` | `Talent_Warrior_Guardian_DefensiveStance` | Active | `Action_Warrior_ShieldBash` | 2 / 0 | FirstAxialTarget R1 | 2 | Requiert bouclier équipé. Inflige 80 % WD, Contondant. Après dégâts, si PhysicalArmor=0, applique `Status_Stunned` pour 1 Turn (`bSkipActivation=true`). | C1,C2 |
| 3 (niv.10) | **Interception**<br>`Talent_Warrior_Guardian_Interception` | `Talent_Warrior_Guardian_ShieldBash` | Passive | `—` | — | Front-row ally | — | 1 fois/round, lorsqu'un allié du rang avant subit une attaque physique ciblée : 50 % des dégâts finaux sont redirigés vers le Guerrier et résolus contre sa propre armure physique. Ignore AoE, DoT et surfaces. | C4 |
| 4 (niv.14) | **Rempart**<br>`Talent_Warrior_Guardian_Bulwark` | `Talent_Warrior_Guardian_Interception` | Passive | `—` | — | Self | — | Armure physique fournie par équipement+bouclier +25 %. Le bonus modifie le pool projeté au démarrage/refresh, pas une seconde armure parallèle. | C2,C3 |
| 5 (niv.18) | **Forteresse**<br>`Talent_Warrior_Guardian_Fortress` | `Talent_Warrior_Guardian_Bulwark` | Active | `Action_Warrior_Fortress` | 3 / 0 | Front row party | 5 | Restaure 40 % du pool d'armure physique de référence à chaque allié vivant du rang avant puis applique `Status_Fortified` 2 rounds : dégâts physiques reçus -25 %. | C2,C3,C8 |

### Brise-ligne

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Coup puissant**<br>`Talent_Warrior_Breaker_PowerStrike` | `—` | Active | `Action_Warrior_PowerStrike` | 3 / 0 | FirstAxialTarget R1 | 1 | Attaque d'arme lourde à 150 % WD, Accuracy -2. Critique autorisé. Aucun contrôle. | C2 |
| 2 (niv.6) | **Brise-armure**<br>`Talent_Warrior_Breaker_ArmorBreak` | `Talent_Warrior_Breaker_PowerStrike` | Active | `Action_Warrior_ArmorBreak` | 2 / 0 | FirstAxialTarget R1 | 2 | Attaque à 100 % WD. En plus, inflige à PhysicalArmor uniquement un bonus égal à 50 % du RawDamage de l'attaque ; l'excédent de ce bonus ne déborde jamais sur les PV. | C2,C3 |
| 3 (niv.10) | **Balayage**<br>`Talent_Warrior_Breaker_Sweep` | `Talent_Warrior_Breaker_ArmorBreak` | Active | `Action_Warrior_Sweep` | 3 / 0 | Area R1 autour cible R1 | 2 | Chaque hostile dans la zone reçoit une attaque à 85 % WD avec un jet séparé. Aucun friendly fire sur les membres du groupe occupant la case de départ. | C2,C8 |
| 4 (niv.14) | **Exécution**<br>`Talent_Warrior_Breaker_Execution` | `Talent_Warrior_Breaker_Sweep` | Active | `Action_Warrior_Execution` | 2 / 0 | FirstAxialTarget R1 | 2 | Disponible uniquement si PhysicalArmor=0 et HP <=35 % MaxHP. Inflige 200 % WD. L'éligibilité est recalculée à la requête ; pas d'exécution automatique. | C2 |
| 5 (niv.18) | **Ravage**<br>`Talent_Warrior_Breaker_Devastation` | `Talent_Warrior_Breaker_Execution` | Active | `Action_Warrior_Devastation` | 4 / 0 | Area R1 autour cible R1 | 5 | Inflige 140 % WD à tous les hostiles de la zone. La cible primaire, si PhysicalArmor=0 après dégâts, reçoit `Status_KnockedDown` 1 Turn (`bSkipActivation=true`). | C1,C2,C8 |

### Maître d'armes

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Spécialisation martiale**<br>`Talent_Warrior_WeaponMaster_MartialSpecialization` | `—` | Passive | `—` | — | Self | — | Choisir `Slashing`, `Piercing` ou `Bludgeoning`. Avec une arme correspondante : Accuracy +1 et dégâts d'arme finaux +10 %. Variante stockée par ChoiceId distinct. | C2 |
| 2 (niv.6) | **Riposte**<br>`Talent_Warrior_WeaponMaster_Riposte` | `Talent_Warrior_WeaponMaster_MartialSpecialization` | Passive | `—` | — | Attacker R1 | — | 1 fois/round après l'échec d'une attaque de mêlée ciblée contre le Guerrier : contre-attaque immédiate à 75 % WD, sans PA et sans déclencher une nouvelle Riposte. | C4,C2 |
| 3 (niv.10) | **Second souffle**<br>`Talent_Warrior_WeaponMaster_SecondWind` | `Talent_Warrior_WeaponMaster_Riposte` | Active | `Action_Warrior_SecondWind` | 1 / 0 | Self | 4 | Restaure 20 % MaxHP, arrondi au supérieur, minimum 1. Indisponible à PV max. Ne restaure ni mana ni armure. | C2 |
| 4 (niv.14) | **Maîtrise critique**<br>`Talent_Warrior_WeaponMaster_CriticalMastery` | `Talent_Warrior_WeaponMaster_SecondWind` | Passive | `—` | — | Self | — | Avec la spécialisation martiale choisie : chance de critique +10 points de pourcentage et multiplicateur de dégâts critiques +25 points de pourcentage. | C2 |
| 5 (niv.18) | **Seigneur de guerre**<br>`Talent_Warrior_WeaponMaster_Warlord` | `Talent_Warrior_WeaponMaster_CriticalMastery` | Active | `Action_Warrior_Warlord` | 2 / 0 | All active party | 4 | Applique `Status_Warlord` 2 rounds à tous les alliés actifs : Accuracy +2 et InitiativeModifier +4. Non cumulable ; nouvelle application rafraîchit la durée. | C1,C2,C8 |

## Voleur

### Assassin

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Attaque sournoise**<br>`Talent_Rogue_Assassin_SneakAttack` | `—` | Active | `Action_Rogue_SneakAttack` | 2 / 0 | FirstAxialTarget R1 | 1 | Requiert arme légère. 125 % WD. Si la cible n'a pas encore agi ce round, subit un contrôle physique, ou si le groupe se trouve dans son arc arrière : 175 % WD à la place. | C2,C4 |
| 2 (niv.6) | **Frappe dans le dos**<br>`Talent_Rogue_Assassin_Backstab` | `Talent_Rogue_Assassin_SneakAttack` | Passive | `—` | — | Rear-arc target | — | Avec arme légère contre une cible dont le groupe occupe l'arc arrière : dégâts +20 % et chance de critique +20 points. Ne s'applique pas aux AoE. | C2 |
| 3 (niv.10) | **Hémorragie**<br>`Talent_Rogue_Assassin_Hemorrhage` | `Talent_Rogue_Assassin_Backstab` | Active | `Action_Rogue_Hemorrhage` | 2 / 0 | FirstAxialTarget R1 | 2 | 100 % WD Tranchant/Perforant. Si PhysicalArmor=0 après dégâts, applique `Status_Bleeding` 3 Turns : 2 dégâts Physical par tick, RefreshDuration, MaxStacks=1. | C1,C2 |
| 4 (niv.14) | **Point faible**<br>`Talent_Rogue_Assassin_WeakPoint` | `Talent_Rogue_Assassin_Hemorrhage` | Active | `Action_Rogue_WeakPoint` | 1 / 0 | FirstAxialTarget R1 | 3 | Aucun dégât. Disponible seulement si PhysicalArmor=0. Applique `Status_ExposedPhysical` 2 rounds : dégâts Physical reçus +20 %. | C1,C2 |
| 5 (niv.18) | **Mise à mort**<br>`Talent_Rogue_Assassin_Finisher` | `Talent_Rogue_Assassin_WeakPoint` | Active | `Action_Rogue_Finisher` | 3 / 0 | FirstAxialTarget R1 | 4 | Disponible seulement si PhysicalArmor=0 et HP <=30 % MaxHP. Inflige 220 % WD. Si la cible survit, aucun effet secondaire automatique. | C2 |

### Ombre

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Esquive**<br>`Talent_Rogue_Shadow_Dodge` | `—` | Active | `Action_Rogue_Dodge` | 1 / 0 | Self | 3 | Applique `Status_Evasive` 1 round : Evasion +4. RefreshDuration, non cumulable. | C1,C2 |
| 2 (niv.6) | **Disparition courte**<br>`Talent_Rogue_Shadow_ShortVanish` | `Talent_Rogue_Shadow_Dodge` | Active | `Action_Rogue_ShortVanish` | 2 / 0 | Self | 4 | Applique `Status_Hidden` jusqu'au début du prochain tour du Voleur ou jusqu'à sa première action offensive. Les attaques ciblées hostiles ne peuvent pas le sélectionner ; AoE/DoT/surfaces continuent de l'affecter. | C1,C4,C8 |
| 3 (niv.10) | **Pas de l'ombre**<br>`Talent_Rogue_Shadow_ShadowStep` | `Talent_Rogue_Shadow_ShortVanish` | Active | `Action_Rogue_ShadowStep` | 1 / 0 | Self | 2 | Applique `Status_ShadowReach` jusqu'à la fin du tour : la prochaine attaque de mêlée avec arme légère peut être exécutée depuis le rang arrière et gagne +1 cellule de portée ; l'effet est consommé à l'attaque. | C1,C4 |
| 4 (niv.14) | **Insaisissable**<br>`Talent_Rogue_Shadow_Elusive` | `Talent_Rogue_Shadow_ShadowStep` | Passive | `—` | — | Self | — | Après utilisation réussie d'Esquive, Disparition courte ou Pas de l'ombre, applique `Status_Elusive` 1 round : Evasion +2 et InitiativeModifier +4. Ne se cumule pas avec lui-même. | C1,C2,C4 |
| 5 (niv.18) | **Ombre parfaite**<br>`Talent_Rogue_Shadow_PerfectShadow` | `Talent_Rogue_Shadow_Elusive` | Active | `Action_Rogue_PerfectShadow` | 2 / 0 | Self | 5 | `Status_HiddenPerfect` 2 rounds. La première action offensive bénéficie de +50 % dégâts et Accuracy +2 puis rompt l'effet ; recevoir des dégâts directs rompt aussi l'effet après résolution. | C1,C2,C4,C8 |

### Saboteur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Désamorçage expert**<br>`Talent_Rogue_Saboteur_ExpertDisarm` | `—` | Passive | `—` | — | Skill checks | — | Jets `Skill_Traps` +2. 1 fois par piège, un échec de 1 ou 2 points devient un échec sûr : le piège reste armé mais ne se déclenche pas. | C4 |
| 2 (niv.6) | **Piège rapide**<br>`Talent_Rogue_Saboteur_QuickTrap` | `Talent_Rogue_Saboteur_ExpertDisarm` | Active | `Action_Rogue_QuickTrap` | 2 / 0 | Cell R1 | 2 | Pose `Trap_Quick` pour 3 rounds. Premier hostile entrant : dégâts Physical Piercing = 6 + DEX mod ; si PhysicalArmor=0 après dégâts, `Status_Immobilized` 1 round. Le piège est ensuite consommé. | C1,C4,C5 |
| 3 (niv.10) | **Bombe fumigène**<br>`Talent_Rogue_Saboteur_SmokeBomb` | `Talent_Rogue_Saboteur_QuickTrap` | Active | `Action_Rogue_SmokeBomb` | 2 / 0 | Cell R3, Area1 | 3 | Crée `Surface_Smoke` 2 rounds. Une ligne de tir traversant la fumée invalide les attaques/spells ciblés à distance ; un occupant de la fumée gagne Evasion +2 contre les attaques Ranged. | C2,C6 |
| 4 (niv.14) | **Maître des serrures**<br>`Talent_Rogue_Saboteur_MasterLocksmith` | `Talent_Rogue_Saboteur_SmokeBomb` | Passive | `—` | — | Lock interactions | — | Jets `Skill_Lockpicking` +2. Pour les RequirementGrants de Crochetage, le Rank effectif vaut Rank+1, plafonné à 5. Un échec de <=2 ne bloque/jamme jamais la serrure. | C2,C4 |
| 5 (niv.18) | **Sabotage**<br>`Talent_Rogue_Saboteur_Sabotage` | `Talent_Rogue_Saboteur_MasterLocksmith` | Active | `Action_Rogue_Sabotage` | 2 / 0 | Interaction/target R1 | 3 | Sur cible `Mechanical|Construct` : test INT+Mechanics contre DD de la cible ; succès = `Status_Sabotaged` 2 rounds (Accuracy -2, InitiativeModifier -4). Hors combat, un interactable explicitement `bCanBeSabotaged` reçoit son événement de sabotage. | C1,C2,C4 |

## Rôdeur

### Tireur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Tir précis**<br>`Talent_Ranger_Marksman_PreciseShot` | `—` | Active | `Action_Ranger_PreciseShot` | 3 / 0 | Ranged target, portée arme+2 | 1 | 150 % WD, Accuracy +2. Requiert arme à distance. Portée finale clampée à 32 cellules. | C2 |
| 2 (niv.6) | **Tir perforant**<br>`Talent_Ranger_Marksman_PiercingShot` | `Talent_Ranger_Marksman_PreciseShot` | Active | `Action_Ranger_PiercingShot` | 2 / 0 | Ranged target | 2 | 110 % WD, Physical/Piercing. Inflige en plus 50 % du RawDamage à PhysicalArmor uniquement, sans débordement vers HP. | C2,C3 |
| 3 (niv.10) | **Tir rapide**<br>`Talent_Ranger_Marksman_RapidShot` | `Talent_Ranger_Marksman_PiercingShot` | Active | `Action_Ranger_RapidShot` | 2 / 0 | Ranged target | 2 | Deux attaques successives indépendantes à 65 % WD chacune. La seconde a Accuracy -1. Les deux peuvent critiquer ; la mort après le premier tir annule le second. | C2,C8 |
| 4 (niv.14) | **Volée**<br>`Talent_Ranger_Marksman_Volley` | `Talent_Ranger_Marksman_RapidShot` | Active | `Action_Ranger_Volley` | 3 / 0 | Cell R5, Area1 | 3 | Chaque hostile de la zone reçoit une attaque indépendante à 80 % WD. Aucun statut. Les obstacles/ligne de vue sont évalués vers la cellule cible. | C2,C8 |
| 5 (niv.18) | **Œil d'aigle**<br>`Talent_Ranger_Marksman_EagleEye` | `Talent_Ranger_Marksman_Volley` | Passive | `—` | — | Self | — | Actions Ranged : portée +1 et Accuracy +1. Jets de Perception à distance +2. Les limites globales de portée restent applicables. | C2 |

### Chasseur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Marque de la proie**<br>`Talent_Ranger_Hunter_MarkPrey` | `—` | Active | `Action_Ranger_MarkPrey` | 1 / 0 | Hostile R5 | 1 | Applique `Status_MarkedByRanger` 3 rounds, sans ArmorGate. Une seule marque active par Rôdeur. Contre sa propre marque : Accuracy +2, dégâts +15 %. | C1,C2,C4 |
| 2 (niv.6) | **Ennemi juré**<br>`Talent_Ranger_Hunter_FavoredEnemy` | `Talent_Ranger_Hunter_MarkPrey` | Passive | `—` | — | Chosen monster category | — | Choisir une `CategoryId` de bestiaire autorisée. Contre cette catégorie : dégâts +15 % et tests de connaissance/perception liés +2. Variante = ChoiceId distinct. | C2 |
| 3 (niv.10) | **Tir immobilisant**<br>`Talent_Ranger_Hunter_PinningShot` | `Talent_Ranger_Hunter_FavoredEnemy` | Active | `Action_Ranger_PinningShot` | 2 / 0 | Ranged target | 2 | 100 % WD. Si PhysicalArmor=0 après dégâts, applique `Status_Immobilized` 1 round (`bBlockTranslation=true`). | C1,C2 |
| 4 (niv.14) | **Frappe du prédateur**<br>`Talent_Ranger_Hunter_PredatorStrike` | `Talent_Ranger_Hunter_PinningShot` | Active | `Action_Ranger_PredatorStrike` | 3 / 0 | Own marked target | 2 | Requiert `Status_MarkedByRanger` provenant du lanceur. Inflige 170 % WD avec Accuracy +1. La marque n'est pas consommée. | C2,C4 |
| 5 (niv.18) | **Chasseur alpha**<br>`Talent_Ranger_Hunter_AlphaHunter` | `Talent_Ranger_Hunter_PredatorStrike` | Passive | `—` | — | Marked death | — | 1 fois/round, à la mort de la cible marquée, transfère automatiquement la marque vers l'hostile vivant le plus proche dans un rayon de 3 cellules ; nouvelle durée 2 rounds. Aucun PA. | C4,C8 |

### Éclaireur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Vigilance**<br>`Talent_Ranger_Scout_Vigilance` | `—` | Passive | `—` | — | Party | — | Le Rôdeur apporte +2 au meilleur jet de groupe de Perception et gagne InitiativeModifier +2 au premier round de chaque combat. Ne se cumule pas entre plusieurs Rôdeurs. | C2,C4 |
| 2 (niv.6) | **Piège de chasse**<br>`Talent_Ranger_Scout_HuntingTrap` | `Talent_Ranger_Scout_Vigilance` | Active | `Action_Ranger_HuntingTrap` | 2 / 0 | Cell R1 | 2 | Pose `Trap_Hunting` 4 rounds. Premier hostile entrant : 5 + WIS mod dégâts Physical/Piercing ; si PhysicalArmor=0, `Status_Immobilized` 1 round ; piège consommé. | C1,C4,C5 |
| 3 (niv.10) | **Repli tactique**<br>`Talent_Ranger_Scout_TacticalRetreat` | `Talent_Ranger_Scout_HuntingTrap` | Active | `Action_Ranger_TacticalRetreat` | 1 / 0 + 1 PAM | Party movement backward 1 | 3 | Déplace tout le groupe d'une cellule en arrière si la translation est légale. Ne paie pas le coût personnel normal de translation, mais paie 1 PAM. Aucun franchissement d'obstacle. | C5 |
| 4 (niv.14) | **Maître du terrain**<br>`Talent_Ranger_Scout_TerrainMaster` | `Talent_Ranger_Scout_TacticalRetreat` | Passive | `—` | — | Self | — | Si le groupe n'a effectué aucune translation depuis la précédente activation du Rôdeur : attaques Ranged +10 % dégâts et Accuracy +1. Le bonus disparaît immédiatement après une translation. | C2,C4 |
| 5 (niv.18) | **Guide du groupe**<br>`Talent_Ranger_Scout_GroupGuide` | `Talent_Ranger_Scout_TerrainMaster` | Passive | `—` | — | Party | — | Tant qu'un Rôdeur vivant possède ce talent : meilleurs jets de groupe Perception/Survie +2 et MaximumMobilityActionPoints +1 par round. Effet global non cumulable. | C2,C4 |

## Mage

### Évocateur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Affinité élémentaire**<br>`Talent_Mage_Evoker_ElementalAffinity` | `—` | Passive | `—` | — | Chosen school | — | Choisir Fire, Frost, Air ou Earth. Les actions `SourcePolicy=Spell` de cette School infligent +15 % dégâts. `Earth` reste une School : ses sorts utilisent les DamageTypes réellement définis (Physical ou Poison), aucun nouveau `EGridDamageType::Earth`. | C2 |
| 2 (niv.6) | **Surcharge élémentaire**<br>`Talent_Mage_Evoker_ElementalOverload` | `Talent_Mage_Evoker_ElementalAffinity` | Active | `Action_Mage_ElementalOverload` | 1 / 4 | Self | 3 | Applique `Status_ElementalOverload` jusqu'à la fin du tour. Le prochain sort de l'Affinité gagne +35 % dégâts puis consomme l'effet. | C1,C2,C4 |
| 3 (niv.10) | **Explosion contrôlée**<br>`Talent_Mage_Evoker_ControlledExplosion` | `Talent_Mage_Evoker_ElementalOverload` | Passive | `—` | — | Area spells | — | Le Mage ne subit aucun dégât direct de ses propres sorts de zone ; ses alliés en subissent 50 % de moins. Les surfaces et statuts créés restent inchangés et peuvent toujours affecter le groupe. | C2,C8 |
| 4 (niv.14) | **Chaîne élémentaire**<br>`Talent_Mage_Evoker_ElementalChain` | `Talent_Mage_Evoker_ControlledExplosion` | Active Spell | `Action_Mage_ElementalChain` | 3 / 8 | Hostile R5 + chain | 3 | Cible primaire puis jusqu'à 2 autres hostiles à <=1 cellule du précédent. Chaque impact inflige `7 + INT mod + Skill_Arcana Rank` du DamageType associé au sort d'affinité. Une cible ne peut être touchée qu'une fois. | C2,C8 |
| 5 (niv.18) | **Cataclysme**<br>`Talent_Mage_Evoker_Cataclysm` | `Talent_Mage_Evoker_ElementalChain` | Active Spell | `Action_Mage_Cataclysm` | 4 / 16 | Cell R5, Area2 | 5 | Chaque hostile : `12 + INT mod + Arcana Rank` dégâts d'affinité. Si MagicalArmor=0 après dégâts : Fire→Burning 2 rounds ; Frost→Slow (Initiative -6) 2 rounds ; Air/Lightning→Stunned 1 Turn ; Earth→Immobilized 1 round ou Poison 3 Turns selon le sort choisi. | C1,C2,C8 |

### Arcaniste

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Bouclier arcanique**<br>`Talent_Mage_Arcanist_ArcaneShield` | `—` | Active Spell | `Action_Mage_ArcaneShield` | 2 / 5 | Self/Ally R3 | 2 | Restaure MagicalArmor de `6 + INT mod + Arcana Rank`, sans dépasser le pool d'armure magique de référence de la cible. | C3,C8 |
| 2 (niv.6) | **Dissipation**<br>`Talent_Mage_Arcanist_Dispel` | `Talent_Mage_Arcanist_ArcaneShield` | Active Spell | `Action_Mage_Dispel` | 2 / 6 | Ally/Hostile R4 | 2 | Allié : retire 1 Debuff magique amovible. Hostile : retire 1 Buff magique amovible. Priorité déterministe : Potency la plus élevée puis EffectId lexical. | C8 |
| 3 (niv.10) | **Manipulation runique**<br>`Talent_Mage_Arcanist_RunicManipulation` | `Talent_Mage_Arcanist_Dispel` | Passive | `—` | — | Rune/Construct | — | Jets `Skill_Runes` +2. Les dégâts Arcane contre cibles taguées `Rune|Construct` +20 %. Les interactions de mécanisme runique gagnent +2 au test, sans auto-réussite. | C2 |
| 4 (niv.14) | **Téléportation courte**<br>`Talent_Mage_Arcanist_ShortTeleport` | `Talent_Mage_Arcanist_RunicManipulation` | Active Spell | `Action_Mage_ShortTeleport` | 3 / 8 | Cell R2 | 4 | Téléporte le groupe entier vers une cellule visible, libre et marchable. Ne traverse ni mur solide, ni porte fermée, ni frontière de niveau et ne déclenche aucune transition. Aucun PAM dépensé. | C5 |
| 5 (niv.18) | **Maîtrise de l'Arcane**<br>`Talent_Mage_Arcanist_ArcaneMastery` | `Talent_Mage_Arcanist_ShortTeleport` | Passive | `—` | — | Arcane spells | — | Actions Spell de School Arcane : ManaCost -1 (minimum 1), portée +1 (max32) et dégâts Arcane +15 %. | C2 |

### Tisseur de surfaces

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Imprégnation**<br>`Talent_Mage_SurfaceWeaver_Imbuement` | `—` | Active Spell | `Action_Mage_Imbuement` | 1 / 4 | Self/Ally R3 | 1 | Applique `Status_ElementalImbuement` 2 rounds. La prochaine attaque d'arme ajoute `3 + INT mod` dégâts du DamageType associé à l'affinité puis consomme le statut. | C1,C2,C4 |
| 2 (niv.6) | **Conversion élémentaire**<br>`Talent_Mage_SurfaceWeaver_ElementalConversion` | `Talent_Mage_SurfaceWeaver_Imbuement` | Active Spell | `Action_Mage_ElementalConversion` | 2 / 5 | Cell R4, Area1 | 2 | Convertit une surface selon table canonique : Water→Ice (Frost), Water/Blood→Electrified (Air), Oil/Poison→Fire (Fire), Water→Poison ou terrain neutre→Oil selon la variante Earth. Pas d'effet si conversion invalide. | C6 |
| 3 (niv.10) | **Conduction**<br>`Talent_Mage_SurfaceWeaver_Conduction` | `Talent_Mage_SurfaceWeaver_ElementalConversion` | Passive | `—` | — | Surface/status interaction | — | Une attaque élémentaire exploitant une surface/état compatible gagne +20 % dégâts. Si elle détruit MagicalArmor, son contrôle associé peut s'appliquer immédiatement après dégâts selon l'ArmorGate normal. | C2,C6 |
| 4 (niv.14) | **Surface persistante**<br>`Talent_Mage_SurfaceWeaver_PersistentSurface` | `Talent_Mage_SurfaceWeaver_Conduction` | Passive | `—` | — | Own surfaces | — | Surfaces créées par le Mage : durée +2 rounds, plafonnée à 6 ; dégâts périodiques de surface +15 %. Ne prolonge pas les surfaces du décor ou d'un autre auteur. | C2,C6 |
| 5 (niv.18) | **Architecte du terrain**<br>`Talent_Mage_SurfaceWeaver_TerrainArchitect` | `Talent_Mage_SurfaceWeaver_PersistentSurface` | Active Spell | `Action_Mage_TerrainArchitect` | 4 / 12 | Cell R5, Area2 | 5 | Crée pendant 3 rounds une grande surface maîtrisée liée à l'affinité (Fire, Ice, Electrified Water, Oil/Poison). Les réactions standards sont ensuite résolues par le système de surfaces, sans effet spécial hard-codé au Talent. | C6,C8 |

## Prêtre

### Restauration

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Soin renforcé**<br>`Talent_Priest_Restoration_EnhancedHealing` | `—` | Passive | `—` | — | Priest healing | — | Tous les soins issus d'une action `SourcePolicy=Spell` du Prêtre sont multipliés par 1,25 après calcul de la magnitude, arrondi inférieur, minimum +1 si soin positif. | C2 |
| 2 (niv.6) | **Régénération**<br>`Talent_Priest_Restoration_Regeneration` | `Talent_Priest_Restoration_EnhancedHealing` | Active Spell | `Action_Priest_Regeneration` | 2 / 5 | Ally R3 | 2 | Applique `Status_Regeneration` 3 Turns : à la fin de chaque activation de la cible, soigne `3 + WIS mod`, minimum 1, puis décrémente la durée. | C1,C8 |
| 3 (niv.10) | **Soin de groupe**<br>`Talent_Priest_Restoration_GroupHeal` | `Talent_Priest_Restoration_Regeneration` | Active Spell | `Action_Priest_GroupHeal` | 3 / 8 | All living party | 3 | Chaque membre vivant du groupe récupère `5 + WIS mod + Skill_Medicine Rank` PV, clampé à MaxHP. Les personnages vaincus ne sont pas ciblés. | C8 |
| 4 (niv.14) | **Purification**<br>`Talent_Priest_Restoration_Purification` | `Talent_Priest_Restoration_GroupHeal` | Active Spell | `Action_Priest_Purification` | 2 / 6 | Ally R3 | 2 | Retire jusqu'à 2 Debuffs amovibles parmi Poison, Burning, Bleeding, Slow, Silence, Immobilize et effets explicitement tagués `Purifiable`. Priorité : Potency puis EffectId. | C8 |
| 5 (niv.18) | **Miracle**<br>`Talent_Priest_Restoration_Miracle` | `Talent_Priest_Restoration_Purification` | Active Spell | `Action_Priest_Miracle` | 4 / 15 | Ally R3 | 5 | Soigne le maximum entre `12 + 2×WIS mod + Religion Rank` et la quantité nécessaire pour atteindre 50 % MaxHP. Retire jusqu'à 3 Debuffs Purifiable et restaure 25 % du pool MagicalArmor de référence. Ne ressuscite pas. | C2,C3,C8 |

### Protection

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Bénédiction**<br>`Talent_Priest_Protection_Blessing` | `—` | Active Spell | `Action_Priest_Blessing` | 2 / 5 | Self/Ally R3 | 2 | `Status_Blessed` 2 rounds : Accuracy +2 et InitiativeModifier +4. RefreshDuration, non cumulable. | C1,C2 |
| 2 (niv.6) | **Égide**<br>`Talent_Priest_Protection_Aegis` | `Talent_Priest_Protection_Blessing` | Active Spell | `Action_Priest_Aegis` | 2 / 6 | Self/Ally R3 | 2 | Restaure `8 + WIS mod + Religion Rank` MagicalArmor, clampé au pool de référence. | C3,C8 |
| 3 (niv.10) | **Protection sacrée**<br>`Talent_Priest_Protection_HolyProtection` | `Talent_Priest_Protection_Aegis` | Active Spell | `Action_Priest_HolyProtection` | 2 / 7 | Self/Ally R3 | 3 | `Status_HolyProtection` 3 rounds : résistances Holy/Necrotic/Arcane +25 % et Evasion +2 contre actions SourcePolicy=Spell ciblées. | C1,C2 |
| 4 (niv.14) | **Sanctuaire**<br>`Talent_Priest_Protection_Sanctuary` | `Talent_Priest_Protection_HolyProtection` | Active Spell | `Action_Priest_Sanctuary` | 3 / 10 | Ally R3 | 4 | Jusqu'au début de la prochaine activation de la cible, max 2 rounds : attaques hostiles ciblées ne peuvent pas la sélectionner. Si la cible inflige des dégâts, le statut disparaît après cette action. AoE/DoT/surfaces restent valides. | C1,C4,C8 |
| 5 (niv.18) | **Bastion divin**<br>`Talent_Priest_Protection_DivineBastion` | `Talent_Priest_Protection_Sanctuary` | Active Spell | `Action_Priest_DivineBastion` | 4 / 12 | All living party | 5 | Restaure 35 % du pool MagicalArmor de référence à tous les alliés vivants puis applique `Status_DivineBastion` 2 rounds : dégâts non-Physical reçus -20 %. | C1,C2,C3,C8 |

### Exorcisme

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Lumière sacrée**<br>`Talent_Priest_Exorcism_HolyLight` | `—` | Active Spell | `Action_Priest_HolyLight` | 2 / 4 | Hostile R5 | 0 | Inflige `5 + WIS mod + Religion Rank` dégâts Holy. Contre `Undead|Demon`, dégâts ×1,5. Aucun statut. | C2 |
| 2 (niv.6) | **Repousser les morts-vivants**<br>`Talent_Priest_Exorcism_TurnUndead` | `Talent_Priest_Exorcism_HolyLight` | Active Spell | `Action_Priest_TurnUndead` | 3 / 7 | Undead Area2 autour party | 3 | Chaque Undead dans la zone subit 4 Holy. Si MagicalArmor=0 après dégâts, il est poussé d'1 cellule à l'opposé du groupe si possible et reçoit InitiativeModifier -4 pendant 1 round. | C1,C2,C5,C8 |
| 3 (niv.10) | **Dissipation sacrée**<br>`Talent_Priest_Exorcism_HolyDispel` | `Talent_Priest_Exorcism_TurnUndead` | Active Spell | `Action_Priest_HolyDispel` | 2 / 6 | Ally R3 / Undead R3 | 2 | Allié : retire jusqu'à 2 Debuffs `Necrotic|Curse`. Undead hostile : retire 1 Buff magique amovible. Ordre déterministe Potency puis EffectId. | C8 |
| 4 (niv.14) | **Châtiment**<br>`Talent_Priest_Exorcism_Smite` | `Talent_Priest_Exorcism_HolyDispel` | Active Spell | `Action_Priest_Smite` | 3 / 8 | Hostile R4 | 2 | Inflige `10 + 2×WIS mod + Religion Rank` Holy. Contre `Undead|Demon`, dégâts ×1,5. Critique non autorisé. | C2 |
| 5 (niv.18) | **Exorcisme majeur**<br>`Talent_Priest_Exorcism_MajorExorcism` | `Talent_Priest_Exorcism_Smite` | Active Spell | `Action_Priest_MajorExorcism` | 4 / 14 | Cell R4, Area2 | 5 | N'affecte que `Undead|Demon|Summoned`. Inflige `14 + 2×WIS mod + Religion Rank` Holy. Si MagicalArmor=0, applique `Status_Banished` 1 Turn (`bSkipActivation=true`). Une future règle d'invocation pourra détruire les Summoned faibles, sans hard-code ici. | C1,C2,C8 |

## Alchimiste

### Grenadier

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Bombe incendiaire**<br>`Talent_Alchemist_Grenadier_FireBomb` | `—` | Recipe + QuickItem | `Action_Alchemist_FireBomb` | 2 / 0 + 1 item | Cell R4, Area1 | 0 | Débloque `Recipe_Bomb_Fire` / `Item_Bomb_Fire`. Chaque hostile : `6 + Alchemy Rank` Fire. Si MagicalArmor=0, `Status_Burning` 2 Turns à 2 Fire/tick. Crée `Surface_Fire` 2 rounds. | C1,C6,C7,C8 |
| 2 (niv.6) | **Bombe toxique**<br>`Talent_Alchemist_Grenadier_ToxicBomb` | `Talent_Alchemist_Grenadier_FireBomb` | Recipe + QuickItem | `Action_Alchemist_ToxicBomb` | 2 / 0 + 1 item | Cell R4, Area1 | 0 | Débloque `Recipe_Bomb_Toxic`. Chaque hostile : `6 + Alchemy Rank` Poison. Si MagicalArmor=0, `Status_Poison` 3 Turns à 2 Poison/tick. Crée `Surface_Poison` 3 rounds. | C1,C6,C7,C8 |
| 3 (niv.10) | **Charge précise**<br>`Talent_Alchemist_Grenadier_PreciseCharge` | `Talent_Alchemist_Grenadier_ToxicBomb` | Passive | `—` | — | Bomb actions | — | Toutes les bombes du lanceur gagnent +1 cellule de portée. Les alliés subissent 50 % de dégâts directs en moins des bombes du lanceur ; surfaces/statuts restent normaux. | C2,C7,C8 |
| 4 (niv.14) | **Réaction en chaîne**<br>`Talent_Alchemist_Grenadier_ChainReaction` | `Talent_Alchemist_Grenadier_PreciseCharge` | Passive | `—` | — | Surface reactions | — | 1 fois par action de bombe, lorsqu'elle déclenche une réaction de surface, les dégâts de cette réaction +25 % et son AreaRadius +1, plafonné à 2. Ne chaîne jamais récursivement. | C2,C4,C6 |
| 5 (niv.18) | **Maître grenadier**<br>`Talent_Alchemist_Grenadier_MasterGrenadier` | `Talent_Alchemist_Grenadier_ChainReaction` | Passive | `—` | — | Bomb actions | — | Bombes : ActionPointCost -1 (minimum 1) et dégâts directs +20 %. Aucun effet sur le coût en items, les DoT déjà appliqués ou les dégâts de surface ultérieurs. | C2,C7 |

### Apothicaire

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Potion renforcée**<br>`Talent_Alchemist_Apothecary_EnhancedPotion` | `—` | Passive | `—` | — | Positive potions used by self | — | Magnitude positive Health/Mana/Armor des potions utilisées par l'Alchimiste +25 %. La durée des statuts n'augmente pas. Aucun effet sur bombes/poisons offensifs. | C2,C7 |
| 2 (niv.6) | **Antidote**<br>`Talent_Alchemist_Apothecary_Antidote` | `Talent_Alchemist_Apothecary_EnhancedPotion` | Recipe + QuickItem | `Action_Alchemist_Antidote` | 1 / 0 + 1 item | Self/Ally R1 | 0 | Débloque `Recipe_Antidote`. Retire `Status_Poison` et 1 autre Debuff tagué `Toxin`. Si aucun effet applicable, l'action est indisponible et l'objet n'est pas consommé. | C7,C8 |
| 3 (niv.10) | **Élixir défensif**<br>`Talent_Alchemist_Apothecary_DefensiveElixir` | `Talent_Alchemist_Apothecary_Antidote` | Recipe + QuickItem | `Action_Alchemist_DefensiveElixir` | 1 / 0 + 1 item | Self/Ally R1 | 0 | Débloque quatre variantes craftées Fire/Ice/Lightning/Poison. Restaure 4 MagicalArmor et applique +25 % résistance au type choisi pendant 3 rounds. | C1,C2,C3,C7 |
| 4 (niv.14) | **Diffusion**<br>`Talent_Alchemist_Apothecary_Diffusion` | `Talent_Alchemist_Apothecary_DefensiveElixir` | Passive | `—` | — | Positive potion | — | 1 fois par utilisation, après une potion positive sur un membre du groupe, choisir un second allié vivant : il reçoit 50 % de la magnitude et de la durée, sans consommer d'objet supplémentaire. Ne se réplique pas récursivement. | C4,C7,C8 |
| 5 (niv.18) | **Panacée**<br>`Talent_Alchemist_Apothecary_Panacea` | `Talent_Alchemist_Apothecary_Diffusion` | Recipe + QuickItem | `Action_Alchemist_Panacea` | 2 / 0 + 1 item | Self/Ally R1 | 3 | Débloque `Recipe_Panacea`. Soigne `10 + 2×Alchemy Rank`, restaure 8 MagicalArmor et retire jusqu'à 3 Debuffs parmi Poison/Burning/Bleeding/Slow/Silence/Immobilize/Toxin/Purifiable. | C3,C7,C8 |

### Transmutateur

| Palier | Talent / ChoiceId | Prérequis | Type | ActionId | PA / Mana / Item | Cible | CD | Mécanique autoritaire | Tech |
|---:|---|---|---|---|---|---|---:|---|---|
| 1 (niv.2) | **Huile glissante**<br>`Talent_Alchemist_Transmuter_OilSlick` | `—` | Recipe + QuickItem | `Action_Alchemist_OilSlick` | 2 / 0 + 1 item | Cell R4, Area1 | 0 | Débloque `Recipe_Flask_Oil`. Crée `Surface_Oil` 4 rounds, sans dégâts directs. Traverser une cellule huilée coûte +1 point de mouvement/PAM selon l'acteur. Fire transforme Oil en Fire. | C6,C7 |
| 2 (niv.6) | **Flasque acide**<br>`Talent_Alchemist_Transmuter_AcidFlask` | `Talent_Alchemist_Transmuter_OilSlick` | Recipe + QuickItem | `Action_Alchemist_AcidFlask` | 2 / 0 + 1 item | Hostile R4 | 1 | Pas de nouveau DamageType Acide : réduit directement PhysicalArmor de `6 + 2×Alchemy Rank`; l'excédent ne touche jamais HP. Applique `Status_Corroded` 2 rounds : restaurations d'armure physique reçues -20 %. | C1,C2,C3,C7 |
| 3 (niv.10) | **Nuage corrosif**<br>`Talent_Alchemist_Transmuter_CorrosiveCloud` | `Talent_Alchemist_Transmuter_AcidFlask` | Recipe + QuickItem | `Action_Alchemist_CorrosiveCloud` | 3 / 0 + 1 item | Cell R4, Area1 | 2 | Impact initial : `4 + Alchemy Rank` Poison. Crée `Surface_PoisonCloud` 3 rounds : 2 Poison/round. Si MagicalArmor=0, les occupants reçoivent `Status_Poison` 2 Turns. | C1,C6,C7,C8 |
| 4 (niv.14) | **Catalyseur**<br>`Talent_Alchemist_Transmuter_Catalyst` | `Talent_Alchemist_Transmuter_CorrosiveCloud` | Active | `Action_Alchemist_Catalyst` | 1 / 0 | Surface R4 | 2 | Disponible seulement si la cellule contient une réaction canonique possible. Déclenche immédiatement une réaction sans consommer artificiellement toute la durée de la surface. Une même action Catalyseur ne peut produire qu'une réaction. | C4,C6 |
| 5 (niv.18) | **Transmutation majeure**<br>`Talent_Alchemist_Transmuter_MajorTransmutation` | `Talent_Alchemist_Transmuter_Catalyst` | Recipe + Active | `Action_Alchemist_MajorTransmutation` | 4 / 0 + 1 rare catalyst | Cell R4, Area2 | 5 | Consomme `Item_Catalyst_Rare`. Convertit toutes les cellules de la zone vers une sortie valide choisie par recette (Fire/Ice/Poison/Oil) et fixe leur durée à 4 rounds. Les dégâts de réaction déclenchés pendant cette conversion +50 %. | C2,C6,C7,C8 |

## 12. Statuts canoniques nécessaires à RPG02

Ces identités sont des **DataAssets de statut à authorer**, pas des branches de code :

| EffectId | Disposition | Durée de référence | Payload principal |
|---|---|---|---|
| `Status_Guarded` | Buff | 2 rounds | -20 % Physical reçu, Evasion +2, -10 % WD sortant |
| `Status_Stunned` | Debuff | 1 Turn | `bSkipActivation=true` |
| `Status_Fortified` | Buff | 2 rounds | -25 % Physical reçu |
| `Status_KnockedDown` | Debuff | 1 Turn | `bSkipActivation=true` |
| `Status_Bleeding` | Debuff | 3 Turns | 2 Physical/tick |
| `Status_ExposedPhysical` | Debuff | 2 rounds | Physical reçu +20 % |
| `Status_Evasive` | Buff | 1 round | Evasion +4 |
| `Status_Hidden` | Buff | max 1 round | non-targetable direct, break on offense |
| `Status_ShadowReach` | Buff | fin du tour | prochain melee léger depuis arrière/+1 range |
| `Status_Elusive` | Buff | 1 round | Evasion +2, Initiative +4 |
| `Status_HiddenPerfect` | Buff | 2 rounds | Hidden + première offense +50 % / Accuracy +2 |
| `Status_Immobilized` | Debuff | 1 round | `bBlockTranslation=true` |
| `Status_Sabotaged` | Debuff | 2 rounds | Accuracy -2, Initiative -4 |
| `Status_MarkedByRanger` | Neutral/Debuff | 3 rounds | source-specific mark |
| `Status_ElementalOverload` | Buff | fin du tour | prochain sort d'affinité +35 % |
| `Status_ElementalImbuement` | Buff | 2 rounds | prochain weapon hit + elemental damage |
| `Status_Regeneration` | Buff | 3 Turns | heal périodique |
| `Status_Blessed` | Buff | 2 rounds | Accuracy +2, Initiative +4 |
| `Status_HolyProtection` | Buff | 3 rounds | résistances +25 %, spell Evasion +2 |
| `Status_Sanctuary` | Buff | max 2 rounds | non-targetable direct, break on damage dealt |
| `Status_DivineBastion` | Buff | 2 rounds | non-Physical reçu -20 % |
| `Status_Burning` | Debuff | 2 Turns | Fire 2/tick |
| `Status_Poison` | Debuff | 2–3 Turns | Poison 2/tick |
| `Status_Corroded` | Debuff | 2 rounds | PhysicalArmor restoration -20 % |
| `Status_Banished` | Debuff | 1 Turn | `bSkipActivation=true` |

Le système MON16 existant fournit déjà durée, stacking, DoT, InitiativeModifier et contrôles booléens. Les autres modificateurs devront passer par une extension générique de projection de statut, pas par des tests sur EffectId.

## 13. Surfaces canoniques nécessaires

RPG02 ne prétend pas qu'elles existent déjà au runtime. Le futur contrat Surface doit au minimum supporter :

`Surface_Fire, Surface_Water, Surface_Ice, Surface_Poison, Surface_Oil, Surface_ElectrifiedWater, Surface_Smoke, Surface_PoisonCloud`.

Règles minimales de réaction visées :

| Entrée | Action | Sortie |
|---|---|---|
| Oil | Fire | Fire |
| Poison | Fire | Fire + réaction explosive |
| Water | Ice | Ice |
| Water | Lightning | ElectrifiedWater |
| Ice | Fire | Water |
| PoisonCloud | Fire | réaction explosive puis Fire |
| Smoke | vent/dispersion future | suppression/réduction |

La surface reste un état de cellule du niveau/runtime, distinct d'un Status Effect porté par un personnage.

## 14. Règles de groupe et formation

- `Front row` = slots 0..2 ; `Back row` = slots 3..5.
- Une capacité ciblant le rang avant n'affecte que les personnages actifs occupant ces slots.
- Une capacité « All active party » cible les personnages vivants actifs, jamais le pool de recrutement.
- `Téléportation courte` et `Repli tactique` déplacent **le groupe**, jamais un personnage isolé, afin de préserver le modèle de dungeon crawler à case unique.
- Les Talents ne créent donc pas de positions individuelles libres sur la grille.

## 15. Conditions d'implémentation

RPG02 sera considéré **implémenté** seulement lorsque :

1. les 90 `ProgressionChoices` sont authorés dans les six classes ;
2. les actions actives ont leurs `Requirements=[ChoiceId]` ;
3. aucune action verrouillée ne peut être exécutée via hotbar/catalogue ;
4. les extensions C1..C8 sont génériques et testées séparément ;
5. aucune logique de production ne compare un TalentId pour décider d'un effet ;
6. les ArmorGates sont évalués après les dégâts de la même action ;
7. les coûts PA/mana/item sont atomiques avec la résolution ;
8. un effet invalide ne consomme pas ses ressources ;
9. les passifs sont projetés depuis les ChoiceIds durables, jamais sauvegardés une seconde fois ;
10. Automation couvre au minimum chaque Talent isolément puis six builds niveau 20 ;
11. PIE valide les interactions UI/hotbar/targeting pour chaque famille de mécanique.

## 16. Découpage recommandé après RPG02

RPG02 est volontairement une **spécification**, pas un refactor massif.

Ordre d'implémentation recommandé :

- **RPG03.1** — Combat Modifier Profile (C2) ;
- **RPG03.2** — Status Application + ArmorGate (C1) ;
- **RPG03.3** — Armor Effects (C3) ;
- **RPG03.4** — Trigger/Reaction Profile (C4) ;
- **RPG03.5** — Tactical Movement (C5) ;
- **RPG03.6** — Surface Runtime (C6) ;
- **RPG03.7** — Alchemy/QuickItem modifiers (C7) ;
- **RPG03.8** — Batch/Target Filters (C8) ;
- **RPG03.9** — Authoring des 90 Talents ;
- **RPG03.10** — Balance/Automation/PIE.

Chaque tranche doit réduire ou réutiliser des autorités existantes ; aucune ne doit créer un second moteur de combat, de statut, de spellbook ou d'inventaire.

## 17. Références de design

- D&D 3.5 / d20 : caractéristiques, compétences, prérequis, progression.
- Divinity: Original Sin 2 : 4 PA de base, spécialisation, armures physique/magique, Crowd Control après destruction de l'armure correspondante, surfaces et synergies.
- GrimrockPrototype : architecture data-driven, groupe 2×3, grille, Requirements, Status Effects MON16, Combat Actions MON12, Magic MON18.

Les chiffres de RPG02 sont propres au projet et restent soumis aux futurs tests de balance.
