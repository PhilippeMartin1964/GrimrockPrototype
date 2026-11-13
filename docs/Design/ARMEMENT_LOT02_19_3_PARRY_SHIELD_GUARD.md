# ARMEMENT-LOT02.19.3 — Parade et blocage liés aux armes équipées

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/tests historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


**Date : 10 novembre 2026. Statut : C++ + tests implémentés, validation Windows/PIE à vérifier sur le SHA du commit.**

## Autorités auditées

1. `docs/Rules/RPG_Shields_Protection_Contract_v0_2.md` : les boucliers sont déjà des **réserves d'armure physiques consommables**. ARM075 N1 donne 5 points (5/5 → 0/5), restaurés au repos/victoire, persistés par GUID. Le contrat interdit d'en faire automatiquement +5 Défense, +5 Esquive ou une seconde mitigation. Il continue de prévaloir.
2. `Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerActions.cpp` : les attaques des monstres utilisent `UGridMonsterCombatComponent::ResolveAndApplyPartyAttack`, avec un hook `FGridPartyAttackResultTransform` **avant** application des dégâts. Ce hook appelle déjà `ApplyIncomingPartyDamageInterception` (réactions RPG03.9.1).
3. `Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerReactions.cpp`, `GridCombatReactionResolver` : les talents et statuts disposent déjà d'une architecture C4. Ce ticket n'ajoute ni second framework de réactions, ni nouveau `ReactionId` issu d'une chaîne narrative.
4. `Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentEquipment.cpp` : `ApplyPhysicalArmorDamage` conserve des traces de la **source d'absorption** (`PhysicalArmorSources`, `ItemDefinitionId`, `Absorbed`). C'est l'autorité de la quantité effectivement bloquée par le bouclier.
5. Catalogue canonique `ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx` : ARM013 est la Main-gauche ; ARM072–082 sont des boucliers ; leur catégorie d'icônes `DefenseParry`/`DefenseBlock` ne rend pas une action active disponible par magie.

## Règles retenues, provisoires et conservatrices

### Parade — ARM013 en OffHand

- Aucune action supplémentaire en hotbar. La parade est une **réaction défensive passive**, uniquement lorsque le personnage vivant est la cible d'un coup de mêlée **physique qui aurait touché**.
- Une **vraie** Main-gauche `WeaponArchetypeId == ARM013`, `ItemType == Weapon`, `bUseWeaponCatalogue` en **OffHand** est requise ; ni une arme rangée dans le sac, ni une arme tenue en MainHand, ni un OffHand alias d'arme à deux mains ne suffit.
- **Chance provisoire : 15 %**, propriété `UGridTurnManagerComponent::MainGaucheParryChancePercent` modifiable dans les détails (0–100). Cette valeur n'est PAS un talent N1 acquis ni un changement d'Esquive. La règle temporaire s'applique à toute classe équipée de cet objet, y compris Voleur/Ombre N1, sans nouvel arbre de compétences.
- Une attaque **critique**, **à distance**, **magique** ou **déjà ratée** ne déclenche aucune parade. La probabilité est testée sur le flux pseudo-aléatoire de combat **seulement pour un coup éligible**. Pas de tirage aléatoire supplémentaire pour les attaques sans Main-gauche.
- En cas de réussite, `bEquipmentParried = true`, `bHit = false`, dégâts d'armure et PV = 0, et le résultat conserve son identité de jet. Aucune dépense de PA, de charge ou de durabilité ; aucune application de status offensif de l'attaque parée. Le journal de combat signale explicitement la parade au lieu d'un simple « échec ».

### Blocage — boucliers ARM072–082 en OffHand

- Le bouclier ne reçoit **aucune réserve ni réduction nouvelle**. Il bloque **exactement les points d'armure physique réellement consommés sur cet objet** par `ApplyPhysicalArmorDamage`. La source est vérifiée contre le vrai `ItemDefinitionId` du bouclier tenu en OffHand.
- Après l'absorption, le résultat autoritaire porte `bEquipmentBlocked` et `EquipmentBlockedDamage`. Le combat journalise le blocage avec le nombre de points réellement absorbés. L'historique, la jauge, la persistance et la recharge de la réserve conservent le contrat RPG-SHIELD02 v0.2.
- Quand la réserve est épuisée, le bouclier ne peut pas afficher de blocage fantôme ; les coups magiques ne sont pas convertis en absorption physique ; une attaque ratée ne débite aucune protection.
- Une capacité de **blocage actif** (posture, PA, chances et conditions) n'est **pas** introduite : elle nécessitera une véritable règle de gameplay distincte. L'icône `DefenseBlock` reste disponible dans la palette pour cette évolution future et n'est pas placée automatiquement dans le HUD.

## Implémentation

- `FGridEquippedGuardResolver` : fonctions pures/testables `CanParry`, `IsSuccessfulParryRoll`, `ApplyParry` et `ResolveAbsorbedShieldDamage`. Aucune nouvelle classe d'acteur ni état sauvegardé.
- `UGridTurnManagerComponent::ApplyIncomingPartyDamageInterception` : garde à l'entrée, tirage déterministe pour ARM013, retour anticipé avant interception alliée lorsque la parade réussit ; une interception de talent existante reste inchangée quand elle échoue.
- `UGridMonsterCombatComponent::ResolveAndApplyPartyAttack` : enrichit le résultat avec l'absorption du bouclier **après** son débit par le service existant. Les profils d'attaques, dommages magiques, ressource PV et effets de statut n'ont aucun second calcul.
- `FGridAttackResult` : trois champs de résultat **transitoires de combat** `bEquipmentParried`, `bEquipmentBlocked`, `EquipmentBlockedDamage`. Ce n'est ni une statistique de personnage ni un champ de SaveGame.
- `FGridCombatLogFormatter::FormatMonsterAttack` : différencie « attaque parée » et « bouclier : N dégâts absorbés » avec `LOCTEXT` natives pour ne pas coder une chaîne brute en UMG.

## Validation C++ et PIE

Tests automatisés du groupe `Grimrock.RPG.ARMEMENT.LOT02.19.3` :
- `MainGaucheParryEligibility` : vrai ARM013 N1, équipement physique, tirages 15 % et bornes, parades sans dégâts, refus critiques / distance / magie / ratés / MainHand seule / OffHand vide.
- `ShieldBlockActualAbsorption` : vrai ARM075 N1, 5 points → absorption 4 puis 1 → 0, pas de blocage quand épuisé ou déséquipé, aucun bonus d'armure inventé, journal lisible.
- Régressions `Grimrock.RPG.SHIELD02`, `Grimrock.RPG.RPG03.4`, `Grimrock.Monsters.MON11` et suite `Grimrock`. La validation à distance ne constitue pas une recette audiovisuelle.

Recette PIE : avec ARM013 OffHand, combattre un monstre qui attaque physiquement au contact et observer des coups parés (chance temporairement réglable à 100 % **dans un niveau de test**, puis remise à 15 %). Vérifier aucun dégât et un événement « parée », puis tester magie/critique et une Main-gauche rangée. Avec ARM075 OffHand, vérifier une absorption totale de 5 points, puis un passage aux PV ; le journal doit rapporter uniquement les points physiquement absorbés. Vérifier sauvegarde et retour en exploration, bouclier remis à neuf seulement selon RPG-SHIELD02 (repos/victoire), et aucune nouvelle touche ou icône de défense active.

## UI/localisation

Lecture de `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` : aucun widget ajouté, aucune disposition modifiée, aucune nouvelle clé WBP/ST_UI. Les **deux nouveaux messages dynamiques** sont des `LOCTEXT` dans `GridCombatLog.cpp`. Leurs traductions anglaises devront être recueillies/vérifiées dans la cible `Game/en` au prochain cycle de gather de localisation ; la compilation C++ ne matérialise pas ces traductions. Chaîne `UI.csv` **inchangée** → `ST_UI` **inchangée** → WBP **inchangés** → `Game/fr` source native → `Game/en` **nouveaux messages à traduire**. Ne pas les présenter comme validés en anglais avant la recette.

## Hors périmètre

Pas d'augmentation de protection du bouclier, pas de riposte gratuite, pas d'animation de parade ou bruitage fictif, pas de gestion de capacités spéciales/grades, pas d'ajout de boutons ni d'icône automatique. Les animations et sons d'attaque double 02.19.2.2.1 restent inchangés.
