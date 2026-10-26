# RPG-SHIELD02 — Contrat des armures physique et magique v0.2

**Décision de conception :** 26 octobre 2026 ; inspiration explicite *Divinity: Original Sin 2* (DOS2).  
**Portée :** armures physiques et magiques, boucliers, personnages, capacités de restauration, fin de combat, repos et état sauvegardé.  
**État : CONTRAT CONSOLIDÉ.** RPG-SHIELD02.2 publie le code C++ de recharge sur victoire normale, mais **la compilation, les tests UE5.5.4 et la recette PIE restent à valider par les logs utilisateur**. La recharge au repos et la correction des chemins de dégâts/restauration ne sont pas encore implémentées. Cette v0.2 **remplace la politique de récupération de v0.1**, sans modifier silencieusement les autres mécaniques déjà validées.

## 1. Inspiration DOS2 vérifiée, frontières

Le **manuel officiel** de DOS2 indique que les deux armures absorbent les dégâts correspondants avant les PV, protègent des effets d'état physiques/magiques et **se réinitialisent à la fin du combat**. Des compétences et potions peuvent restaurer les réserves pendant le combat. Les paillasses / lits de camp restaurent les armures hors combat, d'après les guides d'utilisation.

Sources :
- Manuel officiel français DOS2 : https://dlassets-ssl.xboxlive.com/public/content/6778dff3-8d6c-4615-a8a3-1f1770057d09/GameManual/39d28f7f-8182-499b-8d16-2c7a4d784607/fr-CH/index.html
- Guide DOS2, paillasse et restauration : https://www.rpgsoluce.com/soluces/ps4/dos2/guides/debutant.htm
- Exception de dégâts perforants et gardes d'états : https://divinity.fandom.com/wiki/Physical_and_Magical_Armor_%28Original_Sin_2%29

**Adaptation du prototype :** reprendre *deux réserves rechargeables et leur comportement entre les combats*, pas les statistiques exactes, l'économie de compétences ou la formule de touche de DOS2. Le jet d20 du projet, la Défense = 10 + Esquive effective et les résistances authorées restent en vigueur.

## 2. Règles de jeu approuvées pour le futur runtime

1. **Deux réserves indépendantes :** armure physique contre dégâts physiques ; armure magique contre dégâts magiques/élémentaires. Les dégâts excédentaires affectent les PV conformément au résolveur actuel. Les deux pools ne se compensent pas.
2. **Absorption consommable uniquement pendant les affrontements :** par exemple, un bouclier ARM075 N1 confère +5 points physiques disponibles (5/5 puis 1/5 puis 0/5). Aucune recharge automatique à chaque round, attaque ou changement de main.
3. **Recharge intégrale après une victoire normalement conclue :** armure physique et magique de chaque personnage **vivant** reviennent à leur **référence effective courante**. Aucun soin PV/mana, aucune suppression automatique de statuts, aucune destruction ou réparation d'item.
4. **Repos explicite validé hors combat :** même recharge intégrale des deux pools. Une simple immobilité du joueur sur la grille n'est **pas** un repos ; le gameplay de repos doit faire l'objet d'un événement/commande autorisé et testé. Il n'est pas actuellement implémenté.
5. **En combat :** seules les capacités, effets C3 et consommables explicitement authorés peuvent restaurer la protection, suivant leurs plafonds et cibles. Les mécanismes de soin PV/mana demeurent séparés.
6. **Bouclier ordinaire non jetable :** 0/5 signifie *protection temporairement épuisée*, pas durabilité 0, objet cassé, perte permanente, ni obligation d'acheter un nouveau bouclier. L'armure d'équipement ne donne toujours pas de bonus automatique au jet de Défense ou à l'Esquive.
7. **États :** les gardes physiques/magiques existantes s'appliquent au **moment où l'attaque est résolue** ; regonfler les réserves après combat n'efface pas rétroactivement un effet subi. Les effets traversant l'armure ou évitant son type nécessitent une exception **authorée explicitement**. Ce contrat ne copie pas automatiquement tous les status/effets de DOS2.

### Transitions et prévention d'exploits

- Seule une transition normale réussie `FinishCombat(Victory)` ou un véritable repos autorisé déclenche la recharge. Brancher l'opération **une seule fois**, sur l'autorité de combat, avant que les observateurs de fin de combat lisent les ressources restaurées.
- `FinishCombat(Defeat)` ne ranime personne et ne rétablit pas automatiquement des réserves en fin de partie. Une éventuelle règle pour des survivants après défaite serait un autre contrat.
- `AbortCombat()`, abandon de niveau, restauration d'un checkpoint ou chargement de sauvegarde ne déclenchent pas cette recharge. `ForceVictory` est une commande de diagnostic qui emprunte aujourd'hui le chemin Victory : prévenir/tester explicitement son effet pour ne pas créer de source de recharge exploitable.
- La jauge ne revient pas en arrière simplement parce qu'un ennemi sort du champ de vision, perd l'aggro ou cesse temporairement d'attaquer.
- Ni deux impacts successifs, ni deux notifications `OnCombatEnded` ne doivent recharger plus que la référence autorisée. Aucun cap de durabilité ou script périodique nouveau.
- Les monstres ne reçoivent pas gratuitement une recharge via la victoire du joueur ; leur restauration sur respawn ou reset d'encounter demeure régie par leurs données de rencontre.

### Règles d'autorité des références et instances

- `ReferencePhysicalArmor` : classe/base/progression + bonus physiques **valides des objets réellement équipés**, au niveau réel des objets, puis modificateurs de référence authorés. Éviter tout double comptage (notamment les armes à deux mains).
- `ReferenceMagicalArmor` : référence de classe et contributions magiques **effectivement implémentées** ; ne pas déduire la magie d'un champ de catalogue non raccordé. Le bouclier rond ARM075 N1 apporte actuellement +5 physiques, pas de bonus magique effectif confirmé.
- `CurrentPhysicalArmor` et `CurrentMagicalArmor` sont bornées par leurs références respectives ; ne pas créer de second maximum persistant concurrent. Les limites et effets temporaires au-delà du maximum nécessiteront une décision séparée.
- Garder `FGridItemInstance::AbsorbedEquipmentPhysicalArmor` tant que le système d'absorption par objet existe et pour préserver la compatibilité SaveGame. Pendant le combat, cette déplétion suit le `RuntimeObjectId` : rééquiper un bouclier usé ne rend aucun point gratuitement.
- Lors de la recharge de victoire/repos, remettre à zéro l'absorption des **objets effectivement possédés** : équipés, sacs et objet tenu par le curseur si sa propriété est clairement établie, même si le bouclier n'est pas porté au moment précis de la victoire. Ne pas modifier les instances laissées dans le niveau. Dédupliquer par `RuntimeObjectId`.
- Restaurer **la réserve de base et les contributions d'objets** sans dépasser la référence effective totale. Les capacités de restauration partielle en combat ne doivent restaurer que le montant autorisé et utiliser des méthodes communes.
- Les échanges d'équipement pendant une rencontre ne doivent pas produire de rechargement illimité. Leur impact sur les jauges courantes doit être caractérisé et testé.

## 3. Exemple de référence — Bouclier rond N1

Hypothèses : Elias possède 20 PV, aucune réserve physique de classe, 3 points de protection magique de classe et le bouclier ARM075 +5 physiques ; aucune résistance ni autre équipement.

| Moment | Armure physique | Armure magique | PV |
| --- | ---: | ---: | ---: |
| Avant le combat | 5/5 | 3/3 | 20 |
| Morsure réussie : 4 physiques | 1/5 | 3/3 | 20 |
| Morsure réussie : 5 physiques | 0/5 | 3/3 | 16 |
| Fin de combat : victoire | **5/5** | **3/3** | **16** |
| Combat suivant : 2 magiques | 5/5 | 1/3 | 16 |
| Repos explicitement autorisé, hors combat | **5/5** | **3/3** | **16** |

Une référence magique de 0 signifie 0/0 après recharge, sauf source magique additionnelle déjà authorée. Les deux réserves restent indépendantes.

## 4. Audit du code réel (26 octobre 2026)

| Code ou règle en place | État constaté | Conséquence RPG-SHIELD02 |
| --- | --- | --- |
| `UGridPartyInventoryComponent::GetEffectiveCurrentPhysicalArmor` | Somme des protections d'objets restantes + la réserve de base | Conserver pendant le combat et normaliser la référence maximale |
| `ApplyPhysicalArmorDamage` | Débite les instances équipées avant la réserve de classe | Conserver les traces d'absorption et les identités |
| `RestorePhysicalArmor` | Répare partiellement les équipements **équipés**, puis la réserve de base | Ne pas détourner tel quel pour recharger aussi sacs et curseur |
| `FinishCombat` / `OnCombatEnded` dans `GridTurnManagerPhases.cpp` | Transition autoritaire et événement déjà présents | Point d'entrée futur ; recharger avant l'événement, seulement sur fin normale |
| `IsPartyAtRest()` | Groupe immobile et aligné sur la cellule, non action de sommeil | **Ne pas** s'en servir comme événement de repos |
| `FGridCombatArmorEffectResolver::ApplyRestoreEffects` | Évalue les restaurations C3 sous leurs plafonds | Préserver les effets ciblés et le mana/PA, ne pas y mettre la recharge automatique |
| `GridTurnManagerPlayerActionCatalog.cpp` (multi-cibles) | Ajoute directement du physique à `Resources.CurrentPhysicalArmor` | Risque de contourner la réserve d'item : corriger par l'autorité de restauration |
| `GridTurnManagerSurfaces.cpp` | Enlève directement les dégâts physiques de la base | Risque de contourner les objets équipés : unifier le débit |
| SaveGame / `FGridItemInstance` | Déplétion portée par l'instance ; tests de copie du modèle réussis | Vérifier Save/Load sur disque au combat, hors combat et après recharge |
| ARM075 N1 | +5 physiques réellement authorés | Les N2-N20 et magie d'autres boucliers doivent être audités avant de les déclarer couverts |

**Ne pas masquer les anomalies :** les deux chemins cités (surface et effet ciblé) demandent des tests et probablement une correction ; ils ne sont pas réparés par ce document.

## 5. Décomposition technique et critères de validation

### RPG-SHIELD02.1 — Présent jalon documentaire

- v0.2 rend la recharge automatique et le repos explicites et cite la source DOS2.
- v0.1 reste conservée comme état historique.
- **Aucun gameplay, test C++, WBP, DataAsset ni Excel modifié** ; ne pas déclarer les réserves rechargeables déjà opérationnelles.

### RPG-SHIELD02.2 — Runtime : fin normale de combat

**Code publié sur master, validation locale en attente.** `RefillArmorReservesAfterVictory()` est invoqué dans `FinishCombat(Victory)` avant `OnCombatEnded`. `ForceVictory()` de diagnostic passe un marqueur explicite empêchant la recharge. `GetPartyPortraitViews()` et la nouvelle opération partagent les mêmes références. Voir [RPG_SHIELD02_2_VICTORY_REFILL.md](../Design/RPG_SHIELD02_2_VICTORY_REFILL.md).

Un commit C++ atomique sur `master` :
- rétablir physique et magique à leur référence pour les vivants après une victoire ;
- inclure les instances appartenant au personnage équipées/sac/curseur, sans dupliquer les mains ni les GUID ;
- garder PV, mana, statuts, valeurs de talents et inventaire inchangés ;
- vérifier idempotence, 2 combats consécutifs, défaite, abort, `ForceVictory` et changements d'équipement ;
- vérifier les valeurs données aux portraits, feuille, tooltip et `OnCombatEnded` ; pas d'UI obsolète ;
- inclure tests avec vraie définition `DA_Weapon_RoundShield_01`, rapports UE5.5.4 utilisateur et recette PIE.

### RPG-SHIELD02.3 — Repos explicite

Créer ou brancher un véritable événement de repos **hors combat** (pas `IsPartyAtRest`), avec contraintes métier définies et tests de refus pendant combat, idempotence, deux réserves et aucun soin PV/mana implicite. Un autre commit et validation PIE.

### RPG-SHIELD02.4 — Unification des chemins de dégâts / restauration

Corriger les chemins de surfaces et restaurations C3 ciblées multi-personnages, sans réécrire le résolveur de combat. Tester physique/magique, résistance, résidu PV, armor gates/CC, scores du combat, talents, monstres, niveau de l'objet et sauvegarde. Les changements peuvent être séparés en tickets atomiques si nécessaire.

### RPG-SHIELD02.5 — Persistance et recette intégrée

Vérifier par **sauvegarde disque** l'épuisement pendant combat, le nouveau combat après victoire, le repos et les chargements en plein combat si supportés. Ne pas regonfler des réserves simplement lors d'un chargement. Relancer suites `Grimrock.RPG.ARMEMENT01.10.3.1`, `Grimrock.RPG.RPG03.3`, compétences, MON16, tooltip et smoke de combat, et vérifier l'absence de changements non désirés sur `.uasset`.

## 6. Périmètre explicitement exclu

- Pas de réduction permanente de dégâts par points d'armure : ici l'armure **absorbe** et s'épuise *à l'intérieur* de l'affrontement, comme DOS2.
- Pas de conversion des 5 points physiques en +5 Défense, blocage gratuit ou résistance en pourcentage.
- Pas de changement du d20, de l'Esquive, de la précision ou de la table Excel.
- Pas de nouvelles mécaniques de casse/réparation, durée de vie d'objet, régénération continue, pénalités de poids ou malus de blessure.
- Pas de reconstitution automatique des PV/mana par le seul mécanisme de victoire/repos d'armure.
- Pas de nouvelle capacité, sort, recette, animation ou page UMG. Le WBP Designer reste maître de la présentation future de 1/5, 5/5, etc.
- Pas d'affirmation de tests passés ou de recharge implémentée tant que les correctifs ne sont pas validés localement.

**Autorité :** la présente v0.2 remplace les paragraphes v0.1 imposant un épuisement durable jusqu'à un effet de restauration. Les descriptions des combats et les tests du 23 octobre restent des constats historiques valides pour v0.1.
