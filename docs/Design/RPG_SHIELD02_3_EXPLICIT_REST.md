# RPG-SHIELD02.3 — Repos explicite, recharge physique et magique

**Statut : Implémentation C++ publiée ; compilation Unreal Engine 5.5.4, tests et recette PIE restent à valider localement.**  
**Autorité :** [Contrat RPG-SHIELD02 v0.2](../Rules/RPG_Shields_Protection_Contract_v0_2.md).

## Action de repos : contrat

Le repos n'est **jamais déclenché** par un Tick, un délai sans déplacement ni par la fonction existante `IsPartyAtRest()` (qui ne teste que l'alignement géométrique de la party). Il résulte **uniquement d'une demande explicite** :

```text
Joueur / futur bouton UMG / console "GrimrockRest"
  -> AGrimrockPlayerController::RequestPartyRest()
  -> UGridTurnManagerComponent::TryRestParty()
  -> validation état de la party
  -> UGridPartyInventoryComponent::RefillArmorReservesAfterRest()
  -> OnPartyRested.Broadcast()
```

### Conditions d'acceptation (autorité TurnManager)

- TurnManager initialisé et associé au **LevelRuntimeActor et PartyPawn** courants ;
- pas de combat actif, phase `Exploration` ou `Victory` seulement (pas `Defeat`) ;
- pas de combat en démarrage, d'action active ni de verrou d'input autoritaire ;
- groupe présent, vivant (au moins un membre), pas en chute de fosse ni en création de personnage ;
- partie physiquement immobile sur sa case, correctement orientée (predicate `IsPartyAtRest()` comme **garde**, jamais comme événement).

Une demande rejetée ne change **aucune** réserve ni élément de l'inventaire et n'envoie **aucun** événement `OnPartyRested`.

Une demande acceptée :
- restaure les réserves physique et magique aux références de classe/équipement/modificateurs déjà utilisées par RPG-SHIELD02.2, pour les membres **vivants** ;
- restaure également le bonus consommé des objets équipés, en sac ou sur le curseur commun du groupe ;
- conserve les PV, mana, statuts, compétences, inventaires, ids, possesseurs et objets au sol ;
- peut être répétée sans créer d'armure au-delà des références (idempotence) ;
- diffuse `OnPartyRested` après la mutation pour de futures présentations UMG, sons ou effets.

**Aucun point de vie ni point de mana automatiquement restauré.** Le repos RPG-SHIELD02.3 ne copie pas les actions de `Bedroll` de DOS2 liées aux PV ; il ne touche qu'aux armures, conformément au contrat v0.2.

### Un seul calcul d'armure pour victoire et repos

Le service d'inventaire `RefillArmorReservesForRecovery()` est maintenant le noyau partagé : 
- `RefillArmorReservesAfterVictory()` (existant RPG-SHIELD02.2) délègue au noyau ;
- `RefillArmorReservesAfterRest()` (nouveau RPG-SHIELD02.3) délègue au **même** noyau.

Aucun second calcul de maximum ni duplicata de règles d'objets. Le contrôle de la permission **reste** dans le TurnManager, pas dans le composant d'inventaire.

## Déclenchement dans le jeu et futur UMG

Unreal Editor en **PIE**, hors combat, ouvrir la console UE avec `~` (clavier selon configuration) et saisir :

```text
GrimrockRest
```

Le PlayerController écrit une ligne de diagnostic `[RPG-SHIELD02.3] GrimrockRest Accepted=true|false` et une confirmation/refus temporaire à l'écran. L'input `R` ou une autre touche **n'est pas affecté** automatiquement.

Lorsque l'écran de repos sera conçu, un bouton pourra appeler `RequestPartyRest()` sur `AGrimrockPlayerController` (BlueprintCallable), sans duplicata de logique RPG. `TryRestParty()` est aussi disponible en BlueprintCallable sur le TurnManager pour les séquences de jeu légitimes. Le signal `OnPartyRested` sert uniquement à la présentation après succès.

**Cette étape ne prétend pas fournir un campement animé, une durée simulée, une ressource consommable de repos ni un écran UMG.** Seul l'acte de repos explicitement demandé et sa recharge sont ajoutés.

## Tests livrés

Filtre `Grimrock.RPG.SHIELD02.3` :
- `ExplicitRestRefillsBothPools` : monde UE de test, vrai `DA_Weapon_RoundShield_01` +5, classe synthétique +2 physiques / +3 magiques, armure absorbée, repos explicite, PV/mana conservés, cohérence lecture UI/combat, restauration idempotente et repos post-victoire.
- `RestRejectsCombatMovementAndDefeat` : refus pendant combat, phase `Defeat`, groupe en mouvement/décalé de sa case et groupe entièrement vaincu, sans mutation des réserves.

Régressions recommandées : `Grimrock.RPG.SHIELD02.2` (2/2), `Grimrock.RPG.ARMEMENT01.10.3.1` (3/3), `Grimrock.RPG.RPG03.3` (8/8).

### Exécution locale

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SHIELD02.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.3"
git status --short
```

**Attendu : 2/2 nouveaux + 2/2 + 3/3 + 8/8 = 15/15.** Aucun succès ou échec ne doit être annoncé avant la compilation et les logs utilisateur.

## Recette manuelle PIE

1. En exploration, bouclier équipé et déchargé après une rencontre ou via un test contrôlé ; noter armure physique/magique, PV et mana.
2. Exécuter `GrimrockRest` : armure physique/magique à leurs références ; PV et mana **inchangés**.
3. Refaire `GrimrockRest` : accepté mais **aucun point d'armure supplémentaire**.
4. Tenter pendant un vrai combat : demande refusée, aucune recharge.
5. Éprouver un repos après victoire, et vérifier l'absence de double comptage en rééquipant le bouclier et une arme à deux mains.
6. Aucun champ de texte n'est imposé dans un WBP existant ; les assets locaux restent intacts.

### Limitations intentionnelles

- Aucun repos automatique après déplacement immobile ou lors de Save/Load ; `IsPartyAtRest()` n'est qu'une précondition spatiale.
- Aucun contrôle de « zone de repos sûre » authoré, coût en nourriture, risque d'embuscade ou repos chronométré introduit.
- Les incohérences de dégâts de surface et de restauration C3 multi-cibles restent le jalon **RPG-SHIELD02.4**.
- La validation de sauvegarde disque et d'intégration entre plusieurs combats reste **RPG-SHIELD02.5**.
