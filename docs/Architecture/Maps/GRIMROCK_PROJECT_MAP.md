# GrimrockPrototype — Carte détaillée du projet

> Carte d’architecture textuelle, diffable et autoritaire du projet.
> État : 23 août 2026, après validation de MON20.3 et avant MON20.4.

## Légende

- ✅ implémenté et validé dans le périmètre indiqué ;
- 🟡 implémenté en partie ou contenu à généraliser ;
- ⬜ à concevoir ou à implémenter ;
- ⚠️ dette, risque ou contrôle manuel important ;
- 🎯 priorité immédiate.

## 00 — Vue générale

- ✅ Dungeon crawler en vue subjective, déplacement case par case, grille 32 × 32.
- ✅ Donjons multi-niveaux, exploration, secrets, énigmes et mécanismes.
- ✅ Architecture data-driven centrée sur `UGridDungeonAsset` / `UGridLevelAsset`.
- ✅ Combat tactique au tour par tour avec initiative, PA et PAM.
- ✅ Groupe RPG, inventaire, progression, Status Effects et magie.
- ✅ Scripting avancé via variables, Logic nodes et Lua sandboxé.
- ✅ MON13 à MON19 clos.
- ✅ MON20.1 terminé ; MON20.2 et MON20.3 validés 6/6.
- 🎯 MON20.4 — Story Companion Recruitment UI.
- ⬜ À terme : création, packaging et partage de niveaux par les joueurs.

## 01 — Données, modules et contrats

- ✅ `UGridDungeonAsset` organise les niveaux du donjon.
- ✅ `UGridLevelAsset` porte cellules, objets, liens, variables et scripts.
- ✅ `FGridLevelObjectData` reste le modèle persistant des objets placés.
- ✅ `FGridObjectLink` porte Source + Event → Target + Command, avec condition optionnelle.
- ✅ `ObjectId` est l’identité persistante d’objet ; `LogicId` est un alias authoring.
- ✅ `CharacterId` identifie durablement personnages actifs, réserve et compagnons.
- ✅ `RuntimeObjectId` identifie les instances d’items.
- ✅ Trois modules : `GrimrockLua`, `GrimrockPrototype`, `GrimrockPrototypeEditor`.
- ✅ Runtime dépend de `GrimrockLua`; Editor dépend du Runtime et de `GrimrockLua`.
- ✅ Blueprint configure et compose ; la logique métier testable reste en C++.

## 02 — Grid Editor

- ✅ `FGridLevelEdMode` et `FGridLevelEdModeToolkit`.
- ✅ `AGridLevelEditorActor` comme façade centrale.
- ✅ Implémentation fractionnée dans `GridLevelEditorActorParts/*.inl` et services spécialisés.
- ✅ Paint Cell / Wall, Erase, Select, rotation et déplacement d’objets.
- ✅ Point de départ et orientation du groupe.
- ✅ Palette data-driven et archétypes d’objets.
- ✅ Inspecteur contextuel avec données communes et spécialisées.
- ✅ Connecteurs Event → Command avec conditions.
- ✅ MonsterSpawn, patrol routes, Logic nodes et variables de niveau.
- ✅ Authoring Lua via `GridEditorLuaService`, callbacks, `persistent` et `LogicId`.
- ✅ Preview géométrie/statique/squelettique, sélection, stencil et mini-carte.
- ✅ Validation par ObjectId, placement, références, MonsterSpawn, scripts et liens.
- 🟡 Validation et panneaux Slate restent complexes.
- ⬜ Outils joueurs standalone, templates de zones et publication de niveaux.

## 03 — Runtime et exploration

- ✅ `AGridLevelRuntimeActor` construit le niveau depuis les DataAssets.
- ✅ Sols, plafonds, murs, instancing et objets runtime.
- ✅ Reconstruction/restauration depuis l’état vivant sauvegardé.
- ✅ `AGrimrockPartyPawn` : avant/arrière, strafe, rotations 90°, buffer et interpolation.
- ✅ Head bob et free look sans remettre en cause la grille autoritaire.
- ✅ Blocage par murs, portes et occupation monstre.
- ✅ Multi-niveaux et transitions existantes.
- ✅ Interaction clavier/souris avec portée et priorité de traces.
- ✅ État d’exploration cohérent avec combat et sauvegarde.
- ⚠️ `AGridLevelRuntimeActor` reste très volumineux et central.

## 04 — Interactions et mécanismes

- ✅ Boutons, boutons secrets, leviers, plaques de pression et triggers.
- ✅ Portes ordinaires et portes secrètes : Open / Close / Toggle.
- ✅ Réceptacles : profils d’acceptation, dépôt, retrait et événements.
- ✅ Serrures murales et profils de clés.
- ✅ Items monde : pickup, dépôt ciblé, dépôt au sol, torches, objets lisibles.
- ✅ Armes de jet visibles et récupérables.
- ✅ Escaliers et transitions de niveaux.
- ✅ MonsterSpawn manipulable par commandes runtime.
- 🟡 Réceptacles/containers de production encore à densifier.
- ⬜ Fosses, eau, dangers, obstacles mobiles, portails avancés et pièges de production.

## 05 — Event, Logic et Lua

- ✅ `UGridActivationComponent` reste le dispatcher Event → Command.
- ✅ Chemin simple : `Event -> Command`.
- ✅ Chemin logique : `Event -> Logic -> Event -> Command`.
- ✅ Chemin scripté : `Event -> Lua -> grid.command(...) -> Command`.
- ✅ Variables persistantes `Bool` et `Int32` dans l’état du niveau.
- ✅ Conditions de liens sur variables.
- ✅ Logic nodes : Relay, Set/Toggle Bool, Set/Add/Subtract Int, Reset, Compare, Latch.
- ✅ `GrimrockLua` embarque Lua 5.4 dans une VM sandboxée.
- ✅ Table déclarative `persistent` synchronisée avec les LevelVariables.
- ✅ `LogicId` unique et lisible, résolu vers l’`ObjectId` autoritaire.
- ✅ Budget d’actions partagé pour éviter cycles et contournements.
- ✅ MON19.8 : 4/4 ; `Grimrock.MON19` : 55/55 ; puzzle PIE validé.

## 06 — Groupe et personnages

- ✅ `FGridPartyInventoryState` est l’autorité unique du groupe.
- ✅ Jusqu’à six personnages actifs.
- ✅ `ActiveCharacters`, `ActiveEquipment`, `CharacterPool` et sélection courante.
- ✅ Identité persistante par `CharacterId`.
- ✅ Création initiale via le Character Creation Wizard.
- ✅ Race, classe, attributs, statistiques dérivées, portrait et hotbar.
- ✅ Formation utilisée par ciblage et combat.
- 🟡 Gestion active/réserve complète à finaliser dans MON20.

## 07 — Inventaire, équipement et items

- ✅ `UGridPartyInventoryComponent` centralise l’état d’inventaire du groupe.
- ✅ 40 slots personnels par défaut, piles, poids et surcharge.
- ✅ Drag-and-drop, curseur d’item et transferts interpersonnages.
- ✅ Ownership exclusif et validations transactionnelles.
- ✅ `UGridItemTransferService` pour monde/inventaire/équipement/curseur/réceptacles.
- ✅ Paper doll logique, MainHand, OffHand, armures, bijoux/accessoires.
- ✅ Bonus de caractéristiques, résistances et profils offensifs.
- ✅ Torche, clés, pierre, note, shuriken et objets de test.
- 🟡 Armes, armures, consommables et objets magiques de production à densifier.
- ⚠️ `UGridPartyInventoryComponent` reste volumineux.

## 08 — Combat

- ✅ Initiative globale mélangeant personnages et monstres.
- ✅ Rounds globaux et combattant actif unique.
- ✅ PA individuels et PAM communs ; rotations gratuites selon contrat actuel.
- ✅ Catalogue d’actions universelles, équipement, capacités, sorts et quick items.
- ✅ Transactions de coûts PA, mana, items et cooldowns.
- ✅ Aucun coût en cas d’échec transactionnel.
- ✅ Ciblage automatique, cellule et zone.
- ✅ Attaques groupe, équipement offensif, portée axiale et blocage par murs/portes.
- ✅ Armes de jet et projectile visible.
- ✅ HUD combat, timeline initiative, barre PV, palette et hotbar 0–9.
- ✅ Présentation séparée : animation, audio, VFX, journal et feedback.
- 🟡 Équilibrage à généraliser à un bestiaire et équipement de production plus larges.

## 09 — Monstres et IA

- ✅ `AGridMonsterActor` + composants mouvement, behavior, combat, mort, audio et VFX.
- ✅ `UGridMonsterDefinitionAsset` data-driven.
- ✅ Occupation/réservation de grille et pathfinding déterministe.
- ✅ Perception directionnelle et dernière position connue.
- ✅ Dormance et réveil à la perception du groupe.
- ✅ Patrouille, investigation et alarmes.
- ✅ Aggro par groupe / EncounterGroupId.
- ✅ MonsterSpawn persistant, lifecycle, encounters et vagues.
- ✅ Rat géant comme baseline mêlée.
- ✅ Gobelin lanceur comme famille ranged distincte avec projectile / `RangedKeeper`.
- ✅ Mort, butin, XP, audio/VFX et persistance.
- 🟡 Bestiaire de production encore limité.
- ⬜ Soutien, invocateur, boss multi-phase et familles supplémentaires.

## 10 — Progression RPG

- ✅ Attributs RPG et statistiques dérivées.
- ✅ XP, courbe de niveaux et attribution d’XP — MON15.
- ✅ Level Up et recalcul des statistiques.
- ✅ Progression de classe et choix de progression.
- ✅ Coûts, niveau minimum, prérequis et `GrantedRequirementIds`.
- ✅ Modal Level Up et persistance/migration.
- ✅ Les choix de progression constituent le socle privilégié des talents MON20.
- ⬜ Modèle Skills autonome seulement si rangs/tests indépendants le justifient.
- ⬜ Talents de production et équilibrage final.

## 11 — Recrutement

- ✅ MON20.1 : audit et contrat architectural.
- ✅ Réutilisation de `CharacterPool` au lieu d’un second système de réserve.
- ✅ MON20.2 : `FRPGPartyRecruitmentService::TryRecruitFromPool`.
- ✅ Transaction atomique `CharacterPool -> ActiveCharacters`.
- ✅ Validation capacité, identité, progression, hotbar et ownership.
- ✅ Normalisation des inventaires et rollback complet sur échec.
- ✅ MON20.2 : 6/6 tests.
- ✅ `URPGStoryCompanionAsset` : identité, race, classe, niveau, portrait, équipement authoring.
- ✅ `FRPGStoryCompanionService::EnsureCandidateRegistered` idempotent.
- ✅ `CharacterId` stable réutilisé pour la persistance sans SaveGame v8.
- ✅ MON20.3 : 6/6 tests.
- 🎯 MON20.4 : Story Companion Recruitment UI.
- ⬜ Custom Recruit / Wizard reuse, réserve, skills, talents et régression transversale.

## 12 — Magie et Spellbook

- ✅ Spell definitions data-driven.
- ✅ Spellbook persistant par personnage.
- ✅ Apprentissage et disponibilité des sorts.
- ✅ Intégration au catalogue d’actions et à la hotbar MON12.
- ✅ Ciblage, transaction PA/mana et résolution d’effets.
- ✅ Présentation séparée de la logique.
- ✅ Sorts de production : Arcane Bolt, Lesser Heal, Haste, Cure Poison.
- ✅ UI Spellbook intégrée au GrimrockMenu.
- ✅ Persistance SaveGame.
- 🟡 Bibliothèque de sorts à densifier.

## 13 — Status Effects

- ✅ Modèle générique commun groupe/monstres.
- ✅ Durée, stacking et lifecycle.
- ✅ Damage over Time.
- ✅ Haste / Slow et impact initiative selon contrat MON16.
- ✅ Effets de contrôle supportés par l’architecture.
- ✅ Présentation/HUD et feedback.
- ✅ Save/Restore.
- ✅ Intégration avec les sorts MON18.
- 🟡 Catalogue et équilibrage de production à étendre.

## 14 — Sauvegarde et persistance

- ✅ `UGrimrockPartySaveGame` courant : version 7, compatibilité minimale 1.
- ✅ `FGridDungeonRuntimeState` et états par niveau.
- ✅ Groupe, inventaire, équipement, hotbar et `CharacterPool`.
- ✅ Progression de classe et notifications Level Up.
- ✅ Status Effects et Spellbooks.
- ✅ Variables persistantes de niveau.
- ✅ Monster placements, mort, Despawn et Teleport persistants.
- ✅ Position/facing et niveau courant.
- ✅ `FRPGSaveMigrationService` pour migrations legacy.
- ✅ Sauvegarde régulière refusée pendant un combat actif.
- ✅ La VM Lua n’est pas sérialisée ; seules les données autoritaires le sont.
- ✅ MON20.3 ne nécessite pas de v8.
- ⬜ Autosave/checkpoints et politique Shipping à finaliser.

## 15 — UI et flux de jeu

- ✅ Menu principal : New Game, Continue, Load, options MVP, crédits, quit.
- ✅ Character Creation Wizard.
- ✅ `WBP_GrimrockMenu` / contrat C++ `GrimrockMenuWidget`.
- ✅ Inventory / paper doll / tooltip / read panel / context actions.
- ✅ Party members et portraits.
- ✅ Combat HUD et initiative.
- ✅ Level Up modal.
- ✅ Spellbook.
- 🟡 Skills, Journal, Map, Recipes et Codex disposent de surfaces mais pas de métier complet.
- 🎯 Recruitment UI MON20.4.
- ⬜ Quêtes, dialogues, journal/map/codex fonctionnels MON21.

## 16 — Tests et validation

- ✅ Automation Tests organisés par systèmes et jalons.
- ✅ Tests déterministes, refus atomiques, rollback et non-régression.
- ✅ Tests editor-only pour contrats du Grid Editor.
- ✅ Tests persistence/migration selon jalons.
- ✅ Namespaces nommés pour éviter collisions Unity Build dans les nouveaux tests.
- ✅ MON19.8 : 4/4 Success.
- ✅ `Grimrock.MON19` : 55/55 Success.
- ✅ MON20.2 Recruitment : 6/6 Success.
- ✅ MON20.3 StoryCompanion : 6/6 Success.
- ✅ PIE final MON19 validé.
- ⚠️ Aucun succès UE5 n’est déclaré sans log utilisateur.
- ⬜ CI Windows/UE5 et packaging tests autoritaires à mettre en place.

## 17 — Dette technique et documentation

- ⚠️ `AGridLevelRuntimeActor` très volumineux.
- ⚠️ `UGridPartyInventoryComponent`, PartyPawn et PlayerController volumineux.
- 🟡 Grid Editor mieux découpé, mais validation et Slate restent conséquents.
- ⚠️ Assets `.uasset/.umap` non auditables intégralement hors UE.
- ✅ Pas de refactor massif : extraire seulement les contrats stabilisés.
- ✅ `PROJECT_SYNTHESIS.md` = synthèse globale.
- ✅ `ARCHITECTURE_INDEX.md` = index des contrats.
- ✅ `Maps/GRIMROCK_PROJECT_MAP.md` = carte détaillée autoritaire.
- ✅ `Maps/GRIMROCK_PROJECT_MAP_MERMAID.md` = vues visuelles maintenables.
- ✅ Git est l’unique historique documentaire ; aucun doublon daté nécessaire.

## 18 — Roadmap et production

- ✅ MON13 — Monster Spawn / Encounters / Persistence — CLOS.
- ✅ MON14 — Engagement / Perception / Patrol / Alarm — CLOS.
- ✅ MON15 — XP / Level Progression — CLOS.
- ✅ MON16 — Status Effects — CLOS.
- ✅ MON17 — Gobelin lanceur / ranged family — CLOS.
- ✅ MON18 — Magic & Spellbook — CLOS.
- ✅ MON19 — Advanced Dungeon Logic / Lua — CLOS.
- ✅ MON20.1 Audit — terminé.
- ✅ MON20.2 Recruitment Foundation — 6/6.
- ✅ MON20.3 Story Companion — 6/6.
- 🎯 MON20.4 Recruitment UI — prochain.
- ⬜ MON20 suite : Custom Recruit, Skills, Talents, Reserve, regression/closure.
- ⬜ MON21 : Quests / Journal / Map / Codex.
- ⬜ MON22 : Vertical Slice 45–90 minutes.
- 🟡 Bestiaire, items, sorts, environnement, audio et VFX à densifier.
- ⬜ CI, Shipping, installateur, performance finale et tests matériels.
- ⬜ Éditeur standalone et format de publication de niveaux joueurs.
