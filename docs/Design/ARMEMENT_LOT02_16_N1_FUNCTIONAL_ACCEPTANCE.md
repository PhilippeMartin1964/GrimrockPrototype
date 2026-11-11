# ARMEMENT-LOT02.16 — Audit N1, matrice fonctionnelle et recette PIE

**Périmètre :** les 22 lignes `Active` de `Config/Armement/WeaponProductionManifest.csv` sur `master`.  
**Révision auditée initialement :** `5e964dfa47857da0175fe9749c55f9653280c88e` (8 novembre 2026).  
**Règle de preuve :** `Active`, un test Automation et une véritable session PIE sont trois niveaux de preuve différents.

## 1. Audit initial et autorités

- **22/22 Active** dans le manifeste : 16 armes des cinq familles fonctionnelles ci-dessous et six armes historiques supplémentaires.
- **6 historiques** : ARM001, ARM002, ARM014, ARM015, ARM025, ARM031. **16 en campagne ciblée** : ARM003, ARM013, ARM020, ARM038, ARM045, ARM048, ARM054, ARM057, ARM058, ARM061, ARM062, ARM067, ARM069, ARM070, ARM075, ARM081.
- **Base technique vérifiée sur GitHub :** le SHA initial `5e964dfa...` affiche `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success`, description `Build + Automation OK (Grimrock)`. Cela atteste le build et les assertions présentes au moment du test, sans preuve d'une exécution PIE humaine.
- Les tests `Grimrock.RPG.ARMEMENT.Industrial05.ActiveN1ToN20TooltipAndNarrative` parcourent tous les `Active`, donc **440 profils N1–N20 attendus** au manifeste actuel ; le CSV produit sous `Saved/` doit être contrôlé pour prétendre avoir vu ses 440 lignes. La matrice LOT02.13 existante traite **11 nouvelles armes et 220 profils**, pas 22 attaques réelles.
- `Grimrock.RPG.ARMEMENT.LOT02.ProductionFunctionalMatrix` charge les onze vrais DataAssets, vérifie slots, action N1, portée et coût déclarés, tooltip, bag → équipement → bag et identité. **Il ne déclenche pas une attaque contre un monstre en PIE.**
- Les tests `Grimrock.RPG.ARMEMENT.LOT02.AmmunitionInventoryContract` et `RealWorldPickupAndReturn`, les scénarios de projectiles `EmbeddedRangedArrowHit` / `MON11.Presentation.ThrownWeaponLifecycle` et les tests `Grimrock.RPG.SHIELD02.5` donnent des garanties **sur des scénarios déterminés**, pas sur toutes les combinaisons des 22 armes.
- La liaison réelle des deux arcs à `Item_Arrows_01` est assertée par `GridEditorARMEMENTLOT02132WorldAcceptanceTests.cpp`. La dette `BLOCKED_MISSING_ARROWS_LINK` documentée en LOT02.13 est **historique**, non un blocage à réouvrir sans échec contemporain.
- `GridARMEMENT01132ApprovedStartingKitsTests.cpp` vérifie désormais la création des **18 kits** en mémoire et leurs deux dotations de 12 flèches ; le texte du jalon LOT02.15 qui parle de kits non distribués est antérieur. **La validation PIE et Save/Continue des 18 kits relève d'un autre chantier.**
- Aucun défaut C++ N1 nouveau n'est confirmé par la lecture statique. **Pas de correctif comportemental spéculatif.**

## 2. Matrice canonique des 22 armes

Dans les tableaux, **Données** signifie présence `Active` et couverture industrielle ; **Automation** signifie tests préexistants ciblés (génériques ou par famille). **PIE : À tester** ne signifie pas défaillant. La nouvelle campagne disque de LOT02.16 est décrite en section 3 : son succès ne doit être annoncé qu'après publication du statut Windows pour le SHA du jalon.

| Famille | ID | ItemDefinitionId | Données | Automation existante | PIE |
| --- | --- | --- | --- | --- | --- |
| Mêlée, historique | ARM001 | Weapon_Dagger_01 | Active | Industrial05 / tests mêlée | À tester si clôture exhaustive |
| Mêlée, historique | ARM002 | Weapon_AssassinDagger_01 | Active | Industrial05 / tests mêlée | À tester si clôture exhaustive |
| Mêlée, historique | ARM014 | Weapon_StraightSword_01 | Active | Industrial05 / tests mêlée | À tester si clôture exhaustive |
| Mêlée, historique | ARM015 | Weapon_LongSword_01 | Active | Industrial05 / tests mêlée | À tester si clôture exhaustive |
| Mêlée, historique | ARM025 | Weapon_WarPick_01 | Active | Industrial05 / tests mêlée | À tester si clôture exhaustive |
| Mêlée, historique 2 mains | ARM031 | Weapon_GreatAxe_01 | Active | Industrial05 / équipement 2 mains | À tester si clôture exhaustive |
| Mêlée | ARM003 | Weapon_Stiletto_01 | Active | Industrial05 | À tester |
| Mêlée, main secondaire | ARM013 | Weapon_MainGauche_01 | Active | Industrial05 | À tester |
| Mêlée | ARM020 | Weapon_Mace_01 | Active | Industrial05 | À tester |
| Mêlée, 2 mains | ARM038 | Weapon_ShortSpear_01 | Active | LOT02.13 / équipement 2 mains | À tester |
| Arcs | ARM045 | Weapon_ShortBow_01 | Active | AmmunitionInventoryContract / liaison flèches | À tester |
| Arcs | ARM048 | Weapon_RecurveBow_01 | Active | LOT02.13 / liaison réelle `Item_Arrows_01` | À tester |
| Jet | ARM054 | Weapon_ThrowingKnife_01 | Active | LOT02.13 / QuickItem / MON11 | À tester |
| Jet | ARM057 | Weapon_Javelin_01 | Active | LOT02.13 / QuickItem / MON11 | À tester |
| Jet | ARM058 | Weapon_Dart_01 | Active | LOT02.13 / QuickItem / MON11 | À tester |
| Magie | ARM061 | Weapon_ArcaneWand_01 | Active | LOT02.7 / LOT02.13 | À tester |
| Magie | ARM062 | Weapon_FireWand_01 | Active | LOT02.7 / LOT02.13 | À tester |
| Magie, 2 mains | ARM067 | Weapon_ElementalStaff_01 | Active | LOT02.7 / LOT02.13 | À tester |
| Magie | ARM069 | Weapon_HolyScepter_01 | Active | LOT02.7 / LOT02.13 | À tester |
| Protection magique | ARM070 | Weapon_ArcaneOrb_01 | Active | LOT02.8 / LOT02.13 (passif OffHand) | À tester |
| Bouclier physique | ARM075 | Weapon_RoundShield_01 | Active | SHIELD02.2–02.5, dont disque | À tester visuellement |
| Bouclier physique | ARM081 | Weapon_HolyAegis_01 | Active | LOT02.13 / bouclier générique | À tester |

**Lecture stricte du bilan initial :**
- **Validé :** 22 déclarations Active, couverture industrielle et build/Automation du SHA initial ; tests réutilisables de contrat déjà présents.
- **À tester :** attaques effectivement observées, PA/portée/LOS par famille, état des projectiles et protections en PIE, Save → quitter le programme → Continue.
- **Défaillant :** aucun cas reproductible contemporain identifié au stade de l'audit.
- **Bloqué pour clôture finale :** absence d'observation PIE/Continue complète, et résultat du nouveau test disque à relever sur son propre SHA.

## 3. Couverture disque ajoutée par LOT02.16

Le test **`Grimrock.RPG.ARMEMENT.LOT02.DiskPersistence22N1`**, réutilisant les classes de sauvegarde existantes, doit :
1. Charger le **manifeste actuel**, les **22 vrais DataAssets** et créer 22 instances physiques N1 à GUID distinct dans le composant réel d'inventaire ; créer un stack de **3 vraies flèches**.
2. Équiper ARM003 en MainHand + ARM070 en OffHand, dépenser jusqu'à une unité de protection magique si la réserve N1 canonique le permet (sinon conserver zéro), sauvegarder via `AGrimrockPartyPawn::SaveCurrentGame` dans un **slot disque isolé**, relire avec `UGameplayStatics::LoadGameFromSlot` puis charger via `AGrimrockPartyPawn::LoadCurrentGame`.
3. Vérifier, après réhydratation, **22 identités ItemDefinitionId uniques, 22 GUID inchangés, niveau 1, quantité 1, aucune duplication**, plus GUID/niveau/quantité du stack de flèches et réserve magique non doublée.
4. Déséquiper ARM070, équiper ARM081, dépenser jusqu'à une unité de protection physique si la réserve N1 canonique le permet, répéter l'aller-retour disque et vérifier toutes les identités et la réserve physique non doublée.
5. Détruire seulement le slot de test. Ne pas toucher aux sauvegardes du joueur ni aux assets.

**Limites :** un test Automation dans un monde transitoire avec relecture d'un vrai slot disque **n'est pas** un redémarrage du jeu, une session PIE, une animation Chaos ni une preuve des collisions/ramassages à l'écran. Les flèches au sol, en vol et fichées ont leurs propres scénarios antérieurs ; leur cycle Quitter → Continue réel demeure à observer. Le nouveau test ne valide pas la consommation unitaire pendant un vrai tir. Ses assertions de protection respectent la valeur N1 du catalogue sans inventer de points d'armure.

## 4. Recette PIE compacte : cinq séances par famille

Utiliser une **session de test isolée** avec les armes de la palette existante, un mannequin/monstre vivant, trois distances axiales (1, 2, hors portée), un mur/porte pour la LOS, des flèches, une main secondaire et une sauvegarde jetable. Ne pas modifier `L_Dungeon`, les DataAssets partagés, les maps ou le catalogue pour obtenir ce décor.

| Séance | Recette réelle en PIE | Critères |
| --- | --- | --- |
| **Mêlée** | ARM003/013/020/038 + échantillon des six historiques ; coup N1 à portée, hors portée, PA insuffisants ; ARM013 en OffHand et ARM038 avec OffHand occupée | Dégâts/échec réels ; PA prélevés **une seule fois** sur action acceptée, **pas** sur refus ; deux mains exclusives, pas de double objet |
| **Arcs** | ARM045 puis ARM048, 2 flèches → 1 → 0 ; tirer sans flèche, derrière un mur/une porte, en portée puis hors portée | Une flèche par tir accepté, aucun prélèvement sur refus, aucune attaque gratuite, LOS et portée N1 conformes |
| **Jet** | ARM054/057/058 depuis MainHand puis QuickItem/sac ; impact monstre, mur, sol, récupérer, relancer ; tester un stack de 2 | Une unité dépensée, un seul projectile et un seul pickup récupérable ; collision visible, GUID d'unité stable pendant projectile → pickup et pas de duplication |
| **Magie** | ARM061/062/067/069, portée N1 et distance invalide, PA/ressources ; ARM067 à deux mains | Dégâts/type et PA déclarés uniquement ; ni talent N2, ni brûlure, soin, surface, aura ou sort gratuit non explicitement autorisé |
| **Passifs** | ARM070/075/081 équipés séparément en OffHand ; mesurer jauges avant/après dégâts correspondants, déséquiper/rééquiper, repos/victoire | Aucun bouton d'attaque passive ; protection du bon type, dépense et recharge sans bonus doublé ; incompatibilité avec deux mains |

### Save → Quitter → Continue (une seule sauvegarde de référence)

1. Équiper un objet N1 à une main, un passif en OffHand, conserver un arc avec une quantité de flèches connue et un exemplaire de jet ; noter les `ItemDefinitionId`, `RuntimeObjectId`, niveaux, quantités et absorption à l'aide des diagnostics existants.
2. Dépenser au moins une flèche, un projectile de jet et une portion d'armure ; conserver **un pickup au sol** et **une flèche fichée** si le scénario permet un monstre vivant. Sauvegarder en **exploration**, pas en combat (`GridCombatSavePolicy`).
3. **Quitter réellement** le jeu, relancer, utiliser **Continue** et contrôler les valeurs notées, les objets au sol et les flèches fichées sans duplication.
4. Répéter après un repos/victoire si nécessaire pour comparer la recharge nominale. Ne pas confondre refonte visuelle d'un projectile et duplication de son item physique.

Documenter l'observation par **famille**, pas par 22 micro-rapports. En cas d'échec : ID, source MainHand/OffHand/sac, position, distance, PA avant/après, quantité avant/après, GUID, SaveVersion et capture du journal ; corriger uniquement la cause démontrée.

## 5. Commandes et publication

La suite Automation complète `Grimrock` est exécutée par **`UE5.5.4/LocalAuto`** depuis le worktree Windows après le push du commit, avec `UE5.5.4/SilentTask` contrôlé séparément. Le test `DiskPersistence22N1` appartient au préfixe `Grimrock.RPG.ARMEMENT.LOT02` et est donc inclus dans la suite globale. Les rapports `Saved/` ne sont pas suivis dans Git ; leur absence du dépôt ne prouve ni PASS ni FAIL.

**Critère de clôture LOT02.16 :** build et Automation du SHA publiés réussis **et** les cinq séances PIE/Continue consignées. Si seul le premier critère est obtenu, statut **validation automatisée acquise — clôture PIE en attente**, jamais « 22 armes validées en jeu ».

**Hors périmètre :** réouverture des 18 kits, création d'effets spéciaux, suppression du legacy ThrowingKnife, modification des .uasset / .umap, nouvelle abstraction de combat ou script par arme.


## 6. ARMEMENT-LOT02.16.1 — Exécution des dix armes de mêlée N1

**11 novembre 2026 — couverture ajoutée ; validation automatique Windows à relever sur le commit de ce jalon.**

La campagne de mêlée ne s'appuie désormais pas uniquement sur les 22 entrées de manifeste et sur le tooltip : le test `Grimrock.RPG.ARMEMENT.LOT02.16.1.MeleeN1Execution`, utilisant la fixture réelle de `GridMonsterMON12CombatHudTests.cpp`, charge successivement les **dix DataAssets physiques** ARM001, ARM002, ARM003, ARM013, ARM014, ARM015, ARM020, ARM025, ARM031 et ARM038. L'acteur de monstre est vivant, adjacent au groupe, et chaque arme est réellement équipée à N1 avec GUID distinct, ARM013 en OffHand sans arme principale, les autres en MainHand.

Pour chaque arme, le test exige un profil offensif du catalogue autoritaire, une action principale de raccourci liée au **GUID effectivement équipé**, une attaque acceptée et résolue par le moteur contre le monstre, le coût réel en PA débité **une seule fois**, une entrée de journal de combat, l'identité et la main correctes de l'attaque, une trace de jet réelle et l'absence de soin fictif. Il force ensuite zéro PA et exige un refus sans entrée de combat supplémentaire. Il vérifie également le contrat deux mains de ARM031 et ARM038. Il **ne transforme pas** un simple profil d'arme en test de parade ou en compétence de classe.

**Limites de preuve :** un monde de test Unreal headless et des appels réels au TurnManager ne remplacent pas une observation PIE avec animations, effets de collision, ciblage à plusieurs distances ou inventaire à l'écran. Le parcours manuel mêlée en section 4 reste ouvert ; le test d'armes à distance, de jet, magiques et de passifs sera traité par famille, sans modifier leurs règles de gameplay par anticipation.

Aucun texte joueur, UMG, DataAsset ni sauvegarde du joueur n'est modifié. `UI.csv → ST_UI → WBP → Game/fr/en` : **inchangés**.


## 7. ARMEMENT-LOT02.16.2 — Arcs N1, tirs réels et refus sans dépense

**11 novembre 2026 — nouveau test automatisé de combat ; vérifier les statuts GitHub sur le SHA final.**

Le test `Grimrock.RPG.ARMEMENT.LOT02.16.2.BowN1Execution` couvre les **deux vrais arcs N1** : ARM045 (ShortBow) et ARM048 (RecurveBow), le vrai DataAsset `DA_Item_Arrows_01` et le pipeline `UGridTurnManagerComponent` / raccourci principal / monstre vivant. Il ne remplace pas les tests de projectile embarqué ni la recette visuelle.

Pour **chaque arc**, il contrôle :

- identité du catalogue, niveau N1, main principale, occupation deux mains, portée réelle, action distante et nécessité d'une ligne de vue ;
- tir réellement accepté à une case : une seule flèche prélevée au stock 2 → 1, identité GUID du stack conservée, un seul projectile physique `AGridThrownItemActor`, débit des PA une seule fois, une trace de d20 et une seule entrée de combat ;
- après retrait de la dernière flèche du *stock transitoire de test*, refus d'une nouvelle attaque par le raccourci, sans dépense de PA ni journal supplémentaire ;
- mur solide devant le groupe : refus avant consommation, stock de deux flèches inchangé, zéro PA perdu et rejet `PassageBlocked` par l'autorité des attaques ;
- cible déplacée à **portée N1 + 1** sur une grille de test agrandie : refus `TargetOutOfRange` avec deux flèches conservées, sans débit de PA ni nouvelle entrée de combat.

Le test n'écrit ni carte de production, ni DataAsset ni sauvegarde utilisateur. Il n'effectue **aucune validation GPU/PIE du vol, du son, de l'impact visuel ou de Save → Quitter → Continue**, qui demeurent dans la recette de la section 4. Le maillon `UI.csv → ST_UI → WBP → Game/fr/en` est **inchangé** : aucun nouveau texte visible par le joueur.
