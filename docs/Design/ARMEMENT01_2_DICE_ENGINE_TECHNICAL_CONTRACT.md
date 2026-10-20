# ARMEMENT01.2 — Contrat technique : jets de dégâts multi-dés

**Projet :** GrimrockPrototype — Unreal Engine 5.5.4  
**Date :** 20 octobre 2026  
**Statut :** SPÉCIFICATION RÉDIGÉE — implémentation C++ et validation locale non effectuées  
**Référence fonctionnelle :** ARMEMENT01_CONTRAT_ARMEMENT_V0_3.md  
**Jalon suivant :** ARMEMENT01.3 — implémentation C++ et tests

## 1. Périmètre et règles héritées

ARMEMENT01.2 définit **comment implémenter**, sans encore l'implémenter, la résolution d'un véritable profil NdM + K, où les seuls nombres de faces autorisés pour les **dégâts des nouvelles armes** sont 4, 6, 8, 10, 12 et 20. Les N dés sont lancés indépendamment. Les 20 niveaux d'équipement constituent des versions de butin **immuables** ; ni forge ni rune ne font monter le niveau d'une instance.

Le catalogue de référence conserve **la structure complète à 12 onglets issue de la v0.2** : Lisez-moi, Catalogue, Niveaux 1-20, Raretés, Classes, Améliorations, Uniques, Paramètres, Implémentation UE5, Affixes, Pénalités de niveau, Contrat synthèse. Les éventuelles colonnes de validation sont ajoutées à droite des colonnes existantes, sans suppression des lignes de référence. Un classeur condensé à cinq onglets ne remplace jamais ce catalogue.

Les 82 archétypes et 1 640 profils incluent des boucliers et focaliseurs sans attaque : **260 lignes non offensives** du catalogue étudié ne doivent pas recevoir un faux profil de dés. Les six classes, affinités, familles, raretés et recettes du contrat v0.3 restent inchangées.

## 2. Audit du C++ existant — autorité à préserver

Sources observées sur master au 20.10.2026 :

- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTypes.h : FGridAttackDefinition contient MinDamage, MaxDamage, AccuracyBonus, DamageType, PhysicalSubtype.
- Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatResolver.cpp : ResolveAttack() consomme un d20 de toucher puis **un seul RandRange(MinDamage,MaxDamage)** pour les dégâts si la définition est valide ; le tirage de dégâts est consommé même lorsque l'attaque échouera, puis ignoré par ResolveAttackFromRolls().
- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatResolver.h : ResolveAttackFromRolls(..., NaturalAttackRoll, DamageRoll) permet l'injection d'un résultat de dégâts entier pour les tests.
- Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTraceTypes.h : FGridCombatRollTrace supporte déjà Kind=Die/Range, DiceCount, DiceSides, IndividualRolls, Minimum/Maximum et FinalValue.
- Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatEventFeedFormatter.cpp : le cas Range peut être présenté sous forme de **dé virtuel**, parfois non standard. Ce comportement de présentation historique ne prouve PAS une résolution par vrais dés.
- docs/Design/RPG03_9_0_WEAPON_BASED_TALENT_ATTACKS.md : le coefficient WD du talent est appliqué après le jet de base, le bonus plat et l'attribut, avant critiques et résistances.

**Interdiction d'une deuxième autorité de dégâts** : ni UMG, ni les talismans, ni les talents, ni un Blueprint d'arme ne lancent leurs propres dés pour recalculer une attaque déjà résolue.

## 3. Schéma de données minimal, opt-in et compatible

Proposer un USTRUCT BlueprintType **FGridDamageDiceProfile** accessible depuis **FGridAttackDefinition** :

| Champ | Type envisagé | Contrat |
|---|---|---|
| bEnabled | bool | false par défaut ; conserve les attaques existantes en mode Range |
| DiceCount | int32 | 1 à 32 lorsque activé ; au plus 6 dans le catalogue courant |
| DiceSides | int32 ou enum | exclusivement 4, 6, 8, 10, 12, 20 lorsque activé |
| FlatBonus | int32 | K supérieur ou égal à 0 pour le catalogue actuel |

Une attaque doit avoir **exactement un mode de dégâts** :

- **Legacy Range** (bEnabled=false) : MinDamage/MaxDamage existants demeurent autoritaires et ResolveAttack() conserve l'ancienne distribution uniforme.
- **Real Dice** (bEnabled=true) : DiceCount/DiceSides/FlatBonus sont autoritaires. Minimum = DiceCount + FlatBonus ; maximum = DiceCount × DiceSides + FlatBonus ; moyenne = DiceCount × (DiceSides+1)/2 + FlatBonus. Les champs historiques MinDamage/MaxDamage sont des projections de compatibilité **dérivées et validées**, et ne déclenchent jamais un RandRange supplémentaire.

Le validateur refuse un DiceSides non canonique, DiceCount hors bornes, FlatBonus négatif, débordement d'int32, incohérence de projections Min/Max et sous-type physique illégal pour un dommage non physique. Une définition sans attaque de base (bouclier/focaliseur passif) n'utilise **aucun faux 0d0**.

**Important :** le K de NdM+K est inclus dans la valeur tirée ; il ne doit pas être ajouté une deuxième fois au FlatDamageBonus de FGridOffensiveEquipmentProfile ou au Source.DamageBonus.

## 4. Résolution RNG et API déterministe

Conserver l'API existante et ajouter une voie multi-dés **dans le même FGridCombatResolver** :

1. ResolveAttack(..., FRandomStream&) reçoit le même flux pseudo-aléatoire autoritaire.
2. Le d20 de toucher est tiré avant le résultat de dégâts.
3. Si bEnabled=false, appeler la résolution Legacy sans changement.
4. Si bEnabled=true, effectuer exactement DiceCount appels RandRange(1,DiceSides), stocker chaque résultat, additionner et ajouter FlatBonus **une seule fois**.
5. Utiliser la même résolution habituelle pour touche, RawDamagePercent/WD, critique, modificateurs, résistances et pools d'armure.
6. Aucun second tirage ne doit avoir lieu au formatage du flux d'événements, au tooltip, au replay d'une trace, ni au calcul du talent.

**Ordre RNG sur échec :** pour respecter la logique de consommation du moteur existant, la proposition technique conserve la consommation des dés de dégâts **même en cas de raté** ; elle n'applique pas ces dégâts et ne présente pas un jet comme résolu. Ce choix préserve le principe de consommation actuelle (d20 puis dégâts) tout en rendant explicite le coût RNG du mode multi-dés. Il devra être verrouillé par un test de graine, car la séquence future diffère légitimement lorsqu'une attaque utilise N>1 dés.

Proposer un overload pur **ResolveAttackFromDiceRolls(..., NaturalAttackRoll, IndividualDamageRolls)**. Il valide le nombre et les faces des résultats injectés et permet des tests déterministes. Ne pas remplacer ResolveAttackFromRolls(..., NaturalAttackRoll, DamageRoll) : les tests et chemins Legacy en dépendent.

## 5. Traces et présentation : une seule vérité

Étendre la structure existante **FGridCombatRollTrace** par **DiceFlatBonus** (int32, zéro par défaut) ; ne pas créer une structure de trace parallèle.

Pour une attaque 2d8 + 4 avec IndividualRolls = [3,7] :

| Champ | Valeur |
|---|---|
| Kind | Die |
| DiceCount / DiceSides | 2 / 8 |
| DiceFlatBonus | 4 |
| Minimum / Maximum | 6 / 20 |
| IndividualRolls | [3,7] |
| FinalValue | 14 |

Le Event Feed doit pouvoir formater : **2d8 (3 + 7) + 4 = 14**. FGridCombatRollTrace conserve le d20 de toucher indépendamment dans la trace d'attaque. Les étapes suivantes (SourceDamageBonus, RawDamagePercent, critique, armure, résistances) restent explicites et sont affichées à partir du résultat résolu, sans recalcul dans les widgets.

La branche **Kind=Range** du formatter est maintenue pour les anciens profils, y compris la représentation historique d'un « dé virtuel » non standard ; cette représentation est **interdite comme authoring pour les nouvelles armes ARMEMENT01**. Un nouveau profil Real Dice ne doit jamais être présenté sous une fausse distribution uniforme.

## 6. Probabilités de référence

Pour 2d8 + 4 : min=6, max=20, espérance=13. Les deux dés ont 8×8=64 issues équiprobables. Le total 13 correspond à **8/64 = 12,5 %** des issues, tandis que le minimum 6 correspond à **1/64 = 1,5625 %**. Un simple RandRange(6,20) serait erroné.

Pour 2d6 + 3 : min=5, max=15, espérance=10 ; le total 10 correspond à **6/36** issues. Les tableaux de probabilité ou simulations de tests doivent confirmer la distribution combinatoire, sans substituer un tirage uniforme.

## 7. Compatibilité et migration contrôlée

- Ne pas convertir silencieusement les attaques des monstres, sorts, projectiles, pièges ou objets existants : **mode Legacy par défaut**.
- Ne pas retoucher le d20 de toucher, l'évasion, la plage de critiques, la logique d'armures, les résistances ni les talents WD du RPG03.9.0.
- L'attaque d'une arme de jet est résolue une fois ; récupérer le projectile ne relance aucun dé.
- L'armure apportée par un bouclier reste fixe ; Shield Bash peut avoir un profil offensif si une action l'autorise.
- Le niveau, la rareté, les runes et la forge restent un travail d'instances/SaveGame des jalons ultérieurs ; pas de mutation de ItemLevel dans ARMEMENT01.2.
- Les pénalités de sur-niveau d'objet restent **proposées et non approuvées** ; ne pas les coder implicitement dans ce jalon.
- Les nouveaux effets runiques élémentaires devront utiliser une source identifiée distincte et un ordre de résolution documenté, sans jet de dégâts de base caché.

## 8. Matrice minimale de tests à réaliser lors d'ARMEMENT01.3

Filtre Automation **proposé**, non exécuté : Grimrock.RPG.ARMEMENT01.2

1. **LegacyUnchanged** : mêmes résultats et RNG que l'ancienne branche pour un profil non opt-in.
2. **DiceValidation** : accepte d4/d6/d8/d10/d12/d20, rejette d3/d5/d7/d15, comptes invalides et K négatif.
3. **BoundsAndMean** : 2d8+4 => 6/20/13, 2d6+3 => 5/15/10.
4. **DeterministicIndividualRolls** : [3,7] + 4 = 14, trace Kind=Die complète et aucune relance.
5. **AllMinAllMax** : [1,1]+4=6 ; [8,8]+4=20.
6. **HitD20Preserved** : le d20 de toucher reste indépendant du d8 de dégâts.
7. **MissRngOrder** : confirme N appels de dés après le d20, sans dégâts appliqués.
8. **DamageBonusExactlyOnce** : ni K ni bonus plat de l'arme ne sont dupliqués.
9. **WeaponTalentWDOrder** : coefficient WD appliqué avant critique et défense, selon RPG03.9.0.
10. **TraceEventFeed** : format NdM+K exact ; Range Legacy inchangé.
11. **LegacyMonstersAndSpells** : tests de non-régression des autres sources de dégâts.
12. **NoDiceForPassiveShield** : aucune résolution de dégâts forcée pour un objet non offensif.

Campagne de régression à exécuter localement **après implémentation** : filtre Grimrock.RPG.RPG03 et tests du flux événementiel. Ne jamais annoncer « validé » sans logs UE5/TD04.2 transmis par l'utilisateur.

## 9. Critères d'acceptation documentaires d'ARMEMENT01.2

- Le contrat schéma/RNG/traces est sans ambiguïté et le mode Legacy reste indépendant des profils de dés.
- Les nouvelles armes n'utilisent **jamais** un dé autre que d4/d6/d8/d10/d12/d20.
- Le catalogue de référence conserve **ses douze onglets** ; des contrôles peuvent être ajoutés, sans éliminer les sections du classeur v0.2.
- Ce jalon ne modifie **aucun .cpp, .h, .uasset, .umap ou WBP**. Les tests décrits sont prévus, pas passants.
- Le commit se limite à une mise à jour documentaire atomique sur master.

## 10. Découpage à suivre

**ARMEMENT01.3** : ajout FGridDamageDiceProfile, voie RNG multi-dés, injection déterministe, extension de FGridCombatRollTrace et du Event Feed, tests unitaires de non-régression. **ARMEMENT01.4+** : instances persistantes (ItemLevel/Rarity), affixes et présentation ; plus tard forge, runes et équilibrage final.

**Fin du contrat technique ARMEMENT01.2.**
