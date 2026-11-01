# TRAP-SEARCH01.0 — Contrat de recherche active de pièges

**État : TRAP-SEARCH01.1 (service runtime et masque SaveGame) publié sur `master`, compilation UE 5.5.4 et tests locaux à confirmer. TRAP-SEARCH01.2 (commande clavier et HUD facultatif) publié en C++, en attente de matérialisation locale ST_UI, raccordement WBP et recette PIE.**

Date : 1er novembre 2026.

## Objectif

Offrir au joueur une action volontaire « Examiner les alentours » pour rechercher des pièges qui n'ont pas été découverts automatiquement par la Perception passive (TRAP01.2). Cette action ne doit ni permettre de lancer le d20 sans limite, ni révéler indirectement une cible encore cachée, ni dérégler le désamorçage et la visualisation TRAP01.9.1–9.3 / TRAP-PIT01.3.

## État du code audité

- `AGridLevelRuntimeActor::EvaluatePassiveDungeonTrapPerception` est déclenché sur les événements de mouvement/placement stable, sans Tick de recherche.
- `FGridDungeonTrapPerceptionService` choisit le meilleur observateur vivant sur `Skill_Perception` + Sagesse ; le jet final passe par `FRPGSkillCheckService`.
- La détection passive n'observe que des axes cardinaux sans mur/porte bloquante, jusqu'à quatre cases selon compétence, lumière et définition.
- `FGridLevelRuntimeState::TrapPerceptionAttemptedRangeMask` enregistre les quatre distances d'observation passive par GUID et par niveau, avec `SaveGame`.
- `TryRecordDungeonTrapDetection` et `Dungeon.Trap.Detected` conservent la seule autorité de révélation et d'annonce de découverte.
- Aucun chemin « recherche active » n'existe actuellement. Ne pas réutiliser le masque passif pour simuler l'action.

## Règles proposées pour TRAP-SEARCH01.1

| Élément | Proposition v0.1 |
|---|---|
| Déclenchement | Action explicite « Examiner les alentours », jamais automatique, pas de Tick |
| Zone | Quatre axes cardinaux autour de la cellule actuelle ; 1 ou 2 cases, sans diagonale |
| Surface | Tous les pièges authored `DungeonTrap` : sol, mur et plafond |
| Limites | Plafond de distance de la définition respecté ; murs et portes bloquants ; pas de recherche sous ses propres pieds |
| Compétence | Meilleur observateur vivant, `Skill_Perception` avec Sagesse ; résolveur RPG existant |
| Jet | Un d20 par cible éligible non encore testée à cette distance |
| Avantage actif | Bonus fixe proposé de +2 au test de Perception (à garder séparé des mods de classe et de groupe) |
| Difficulté | Même DC effectif distance + obscurité que TRAP01.2 ; seul le bonus de recherche distingue l'action |
| Résultat positif | `TryRecordDungeonTrapDetection`, mesh révélable, événement `Dungeon.Trap.Detected` avec informations du véritable jet |
| Résultat négatif | Aucune information sur l'existence, la position ou le nombre de pièges ; pas de « aucun danger repéré » |
| Combat et transitions | Indisponible pendant combat, chute PIT01, ciblage et UI bloquante |
| Coûts/économie | Aucun PA ni ressource hors combat pour v0.1 ; limitation par tentatives persistées |

La recherche active ne doit pas être bornée par la portée dynamique passive calculée d'après le score ; sa portée propre (2) représente l'observation volontaire. Elle reste bornée par le `MaxDetectionRangeCells` authoré de chaque piège et par la géométrie réelle. L'obscurité continue d'ajouter sa pénalité au DC de détection visuelle et ne se contourne pas.

## Anti-reroll et SaveGame

Ajouter dans `FGridLevelRuntimeState` une **nouvelle** propriété `UPROPERTY(SaveGame)` :

```cpp
TMap<FGuid, uint8> TrapActiveSearchAttemptedRangeMask;
```

- bit 0 : recherche active effectuée à une case ; bit 1 : recherche active effectuée à deux cases ;
- enregistré avant chaque jet réellement autorisé ;
- distinct de `TrapPerceptionAttemptedRangeMask` : l'action active peut donner une seconde chance après un échec passif, mais jamais une infinité de chances ;
- pas de nouveau jet à la même distance après déplacement, rotation ou réouverture du niveau ; pas de reset en Save/Continue ;
- une nouvelle distance autorise une tentative active supplémentaire, au maximum deux par GUID ;
- ne pas mémoriser de résultat visible/consultable sur les pièges cachés ;
- vérifier la compatibilité avec la politique de version des sauvegardes courantes ; ne pas ajouter de migration legacy injustifiée.

Un appel sans piège, avec des pièges déjà connus, hors portée ou masqués ne doit consommer aucun RNG ni révéler de métadonnées. L'ordre des pièges doit rester déterministe pour un même niveau et une même séquence.

## Architecture / découpages

### TRAP-SEARCH01.1 — Service C++ et état (implémentation publiée)
1. `FGridDungeonTrapPerceptionService` : garder les règles communes de sélection de l'observateur, topologie et DC.
2. `AGridLevelRuntimeActor` : introduire une API explicite de recherche active, avec `FRandomStream&` injecté pour des tests déterministes.
3. `FGridLevelRuntimeState` : nouveau masque SaveGame distinct, sans modification de l'asset de niveau et sans conséquence sur les probabilités de désamorçage.
4. Éviter deux copies divergentes du parcours de visibilité et de construction de l'événement de découverte : extraire la partie commune si elle reste courte et contrôlée.
5. Tester jets, refus, masques, succès, murs, portes, lumière, bonus et restauration.

### TRAP-SEARCH01.2 — Commande utilisateur / présentation (code publié, WBP et localisation locale à valider)
1. Exposer la commande au joueur **sans choisir arbitrairement une touche déjà employée**, en auditant `DefaultMappingContext`, les raccourcis Pawn et la barre d'actions persistante.
2. Réutiliser le système de commandes existant ; l'exécution demeure en C++.
3. Fournir une confirmation neutre de l'action si nécessaire, sans négation de présence des pièges et sans texte de localisation non matérialisé.
4. Vérifier l'arbitrage des menus/inputs et la non-consommation de PA en exploration.
5. Pas de nouveau WBP sans justification ; si un texte est ajouté, suivre `UI.csv → ST_UI.uasset → WBP → Game(fr/en)` avant authoring.

## Tests automatisés attendus

Préfixe `Grimrock.TRAP_SEARCH01` :
- première tentative active à distance 1 ; deuxième essai même distance silencieux et sans d20 ;
- possibilité d'un essai séparé à distance 2, mais pas de troisième essai à distance 1 ;
- échec passif puis succès actif autorisé ; le masque passif reste inchangé ;
- meilleur vivant ; bonus +2 réellement inclus dans le total sans modifier la projection de compétence de référence ;
- murs fermés, portes fermées, diagonales, sous les pieds, limites de définition : aucun jet consommé ;
- aucun événement « piège introuvable » ni fuite de GUID/DC sur échec ;
- révélation 3D et événement `Dungeon.Trap.Detected` uniquement sur réussite ;
- reconstructions et Save/Continue : même nombre de tentatives restantes ;
- blocages exploration/combat/chute/UI vérifiés sur la commande utilisateur ;
- régressions `Grimrock.TRAP01`, `Grimrock.TRAP_PIT01`, `Grimrock.MON20.6.Skills`, HUD si nécessaire.

## Recette PIE prévue

Placer des pièges de sol, mur et plafond à une et deux cases, ainsi qu'un piège caché derrière une porte fermée. Après un échec passif, demander « Examiner les alentours ». Une vraie réussite révèle le mécanisme et l'infobulle habituelle. Un échec ne confirme ni n'exclut aucun piège. Répéter la recherche au même endroit, changer d'orientation puis Save/Continue : aucune relance de jet à distance déjà essayée. Vérifier qu'un déplacement à une autre distance autorise au plus le second jet prévu. Tester également une zone sans piège, le combat et la chute.

## Hors périmètre

- Modifications des DataAssets de pièges, de la carte, des textures ou du système PIT03.
- Dégâts, calcul de désamorçage, prix d'outils, PA de combat, quête, XP de découverte.
- Nouveau widget graphique imposé, apprentissage d'une nouvelle compétence, recherche autour des angles.
- Recherche de secrets/objets invisibles autres que les pièges authored.

## Livraison

TRAP-SEARCH01.1 ajoute une API C++ sans entrée joueur : `EvaluateActiveDungeonTrapSearch(PartyPawn, RandomStream)`.
Elle réutilise le balayage cardinal et la notification de découverte TRAP01.2, mais conserve des tentatives indépendantes, applique +2 au total du résultat RPG et limite la portée active à deux cases (bornée par l'instance).
Le test `Grimrock.TRAP_SEARCH01.1` couvre les règles, refus, sauvegarde et mutations. Il faut le compiler/valider localement avant de déclarer .1 achevé.
TRAP-SEARCH01.2 : F dans le PlayerController appelle `RequestActiveTrapSearch`, comme l'option `Button_SearchSurroundings` dans le HUD permanent (après ajout local dans le Designer). Rejet silencieux sur UI modale/combat/chute/visée/items tenus. Un événement neutre `Dungeon.Trap.ActiveSearch` précède le scan accepté, sans fuite de présence ou de nombre de pièges. Les clés CSV `Navigation.Tooltip.SearchSurroundings` et `Exploration.Search.Performed` restent à matérialiser réellement en ST_UI avant finalisation UI et traduction fr/en.
