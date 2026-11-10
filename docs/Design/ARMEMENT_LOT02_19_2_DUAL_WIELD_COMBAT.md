# ARMEMENT-LOT02.19.2 — Attaque double, deux armes réelles et un coût unique

> **Séquence 02.19.2.2 (10.11.2026) :** la double frappe conserve ses deux calculs autoritaires synchrones, mais la présentation, les bruitages configurés, le feedback et la publication au flux des deux coups sont désormais **étalés dans le temps** (première frappe → impact → deuxième frappe → impact). L'ancienne affirmation « deux événements instantanés dans le flux » décrit uniquement l'historique immédiat, pas l'affichage. Voir [ARMEMENT-LOT02.19.2.2](ARMEMENT_LOT02_19_2_2_SEQUENCED_COMBAT_PRESENTATION.md).

> **MISE À JOUR 10.11.2026 — ARMEMENT-LOT02.21 :** les raccourcis temporaires 2/3 de frappe simple MainHand/OffHand mentionnés ci-dessous étaient une étape de prototype ; **ils ne sont plus projetés automatiquement**. Une seule attaque normale contextuelle est affichée en raccourci 1, y compris avec ARM001 + ARM013. Les deux profils physiques restent dans le moteur pour résoudre l'attaque combinée. Les raccourcis manuellement affectés restent respectés. Voir [contrat courant](ARMEMENT_LOT02_21_SINGLE_EQUIPPED_ATTACK_HOTBAR.md).

> **ÉTAT COURANT (10.11.2026) :** ce document décrit la mécanique autoritaire livrée en 02.19.2. La décision provisoire du joueur est maintenant **explicite** : le Voleur/Ombre N1, ainsi que toute autre classe portant deux armes offensives compatibles, **ne requiert pas de talent de maîtrise** pour une attaque double. Aucun talent RPG n'est octroyé ou court-circuité. Les **icônes de toutes les actions ARMXXX** relèvent de [ARMEMENT-LOT02.20](ARMEMENT_LOT02_20_EQUIPPED_ACTION_ICON_FOUNDATION.md) ; la mention historique « icône principale provisoire » ci-dessous ne prévaut plus quand la palette possède sa texture. Le séquencement audio et visuel demeure ouvert (02.19.2.2).

## Décision de gameplay

À la suite de 02.19.1 validé en PIE, le Voleur / Ombre équipé de ARM001 (MainHand) et ARM013 (OffHand) obtient une action contextuelle **Attaque double**.

| Action | Condition | Coût | Résolution |
| --- | --- | --- | --- |
| Attaque double (raccourci 1) | Deux armes de mêlée à une main éligibles, sources physiques distinctes | **3 PA une seule fois** | MainHand 100 %, puis OffHand 50 % avec **son propre jet d'attaque**, sans toucher gratuitement |
| Attaque MainHand (raccourci temporaire 2 si libre) | Arme principale offensive | Coût natif de l'arme (2 PA N1) | Profil MainHand uniquement |
| Attaque OffHand (raccourci temporaire 3 si libre) | Arme secondaire offensive | Coût natif de l'arme (2 PA N1) | Profil OffHand uniquement |
| Attaque à mains nues (raccourci 1 seulement si aucune arme) | Aucun équipement offensif | Coût natif | Profil universel |

Les raccourcis de mains séparées sont **des suggestions non persistantes**, dans les premiers emplacements réellement libres ; les affectations du joueur prévalent. L'alias `PrimaryAttack` déjà sérialisé est conservé. Ni le format SaveGame ni les bindings existants ne changent.

Le lancement de l'attaque double est **une action** avec **deux résultats** : PA dépensés une seule fois ; deux événements `OnPlayerAttackRequested` / `OnPlayerAttackResolved` et deux lignes de combat distinctes, avec `AttackId`, `ItemDefinitionId` et `OffensiveEquipmentSlot` de **chaque arme**. Le groupe reste actif avec 1 PA après une attaque double à partir de 4 PA. Le second coup est ignoré si le premier tue la cible. Chaque coup résout séparément la cible et ses armures dans leur état courant. La réduction OffHand de 50 % est appliquée au **dégât brut de son propre jet avant les protections**, pas au résultat combiné après armure.

L'action n'est pas offerte par des équipements passifs (bouclier ARM075, orbes), une arme à deux mains (ARM031), des attaques à distance, des projectiles consommables, deux copies du même RuntimeObjectId, ou une source interdite dans le slot (notamment ARM002 en OffHand). Les autres effets ou talents à prérequis qui ne rendent pas une arme de mêlée N1 éligible ne doivent pas être forcés. Aucun talent N2 n'est accordé implicitement.

## Implémentation C++

1. `FGridDualWieldPolicy::TryResolve` (nouveau `Public/Runtime/Combat/GridDualWieldPolicy.h` et `Private/Runtime/Combat/GridDualWieldPolicy.cpp`) résout les deux **objets physiques** : instance (GUID, niveau, slot), définition DataAsset, action autorisée de mêlée simple, profil offensif autoritaire du catalogue. La même fonction est appelée **pour proposer** l'action et **avant** de débiter les PA. Aucune mutation dans cette politique.
2. `UGridTurnManagerComponent::BuildPlayerCombatActionContributions` fournit `Attack_DualWield` comme action Equipment de MainHand (`ResolutionCount=2`, coût 3 PA, range 1, provenance GUID). Les noms et descriptions sont des `NSLOCTEXT` natifs, les icônes et la présentation proviennent de la dague principale.
3. `RequestCharacterAttackInternal` conserve la transaction d'attaque existante et vérifie les **deux** GUID avant toute dépense. Le premier jet suit la chaîne habituelle MainHand. Pour le second, il remplace effectivement l'attaque par le **profil OffHand**, applique `RawDamagePercent=50`, recalcule les modificateurs d'attaque depuis les statuts actualisés (notamment furtivité déjà consommée), puis utilise les effets propres à OffHand et son entrée de journal. Les deux jets restent sous **un même ActionInstanceId** et l'événement `ActionResolved` clôt la combinaison une seule fois. Aucun paiement supplémentaire ni consommation artificielle n'intervient sur le second coup.
4. `FGridCombatHudViewModelBuilder::ProjectEquippedHandAttacks` résout `PrimaryAttack` en `Attack_DualWield` uniquement si présent dans le catalogue ; sinon conservation de l'ordre MainHand, OffHand, mains nues. Il expose les coups simples MainHand et OffHand sans toucher au stockage. Les deux suggestions temporaires ont les mêmes protections clic droit/glisser-déposer qu'en 02.19.1.

**Limite volontaire :** une touche combinée affiche temporairement l'icône de l'arme principale, sans asset composite nouveau. Le rendu/animation de deux lames reste à revoir graphiquement en PIE. `FGridCombatActionRequestResult` expose le résultat de la frappe la plus récente ; le journal et les delegates contiennent **les deux frappes**.

## Compatibilité, refus et effets

- Une action non disponible (PA, cible, mauvais tour, paire invalide) ne consomme rien.
- Deux frappes ne partagent **ni leurs dés, ni leurs dégâts, ni leurs statuts d'arme**, et ne contournent pas l'armure physique/magique.
- Pour préserver les effets de réactions, les événements de coups utilisent le **même identifiant d'action**, puis une clôture d'action unique. Un coup critique ou une application de statut sur le premier coup peut influencer les défenses de la seconde résolution ; les effets de furtivité expirés ne sont pas recyclés.
- Les profils N1 et les 18 kits ne sont pas modifiés. L'attaque autonome OffHand de 02.19.1 reste utilisable (2 PA) ; cela est une mécanique distincte de la future parade ARM013 (02.19.3).
- Ne pas étendre par assimilation cette règle aux attaques à distance ou aux deux mains.

## Tests automatisés

`Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost` :

- vraie paire ARM001 / ARM013, deux GUID et profils différents, action composite N1 issue du catalogue ;
- raccourcis contextuels 1 double / 2 principal simple / 3 secondaire simple, sans altération du SaveGame ;
- attaque double acceptée, **deux résolutions** et **un paiement de 3 PA**, seconde frappe identifiée par ARM013, coefficient 50 % dans la trace ;
- refus sans ressources supplémentaires, retrait de la double attaque après bouclier ARM075 et arme deux mains ARM031 ;
- anciens tests `LOT02.19.1` mis à jour pour la nouvelle projection contextuelle (OffHand seule continue de remplacer l'attaque à mains nues).

Suite complète `Grimrock` via VALIDATION-AUTO01 pour le SHA publié ; valider séparément `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.

## Contrat UI et localisation — checklist d'ouverture et clôture

Documents relus : `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.

- **WBP existants** : `WBP_GridCombatHud` (`UGridCombatHudWidget`) et `WBP_GridCombatHudAction` (`UGridCombatHudActionWidget`) ; aucun changement d'arbre Designer ni de `.uasset`, aucune nouvelle géométrie C++.
- **Textes** : nom/description de l'action système dans le C++ en `NSLOCTEXT("GridDualWield", ...)`, comme `Attack_Unarmed` ; coûts, disponibilité et libellés de raccourcis réutilisent les composants et clés existants. Noms/icônes d'objets restent gouvernés par DataAsset. Pas de nouvelle ligne `Localization/Source/UI.csv`.
- **ST_UI** : aucune clé nouvelle, donc aucune matérialisation ni changement de `ST_UI.uasset` requis. Les libellés `NSLOCTEXT` seront à confirmer dans le prochain **gather Game + traduction anglaise** : la matérialisation d'une clé ST_UI ne les couvre pas. Ne pas affirmer une traduction anglaise complète sans ce contrôle.
- **État chaîne** : `UI.csv` inchangé → `ST_UI` inchangé → WBP existants inchangés → `Game/fr/en` **traduction anglaise de nouveaux NSLOCTEXT non encore attestée**.
- **Tests** : C++/NullRHI vérifient le combat et la projection, **pas** les icônes et animations graphiques en PIE.

## Recette PIE pour approbation utilisateur

1. Nouveau Voleur / Ombre, entrer dans le donjon, engager un monstre avec plus de PV qu'une frappe : raccourci 1 = Attaque double (3 PA), raccourci 2 = dague seule, raccourci 3 = Main-gauche seule si ces emplacements sont libres.
2. Exécuter 1 : deux coups MainHand/OffHand distincts dans le journal, coûts **4 → 1 PA**, aucune double consommation. L'armure et les points de vie doivent évoluer entre les deux attaques.
3. Comparer avec les attaques seules dans les tours suivants ; inspecter la trace des dégâts OffHand à 50 % et les identifiants d'arme différents.
4. Mettre un raccourci manuel dans les slots 2/3 : il est conservé, les suggestions se déplacent vers les emplacements libres ; Save → Quitter PIE → Continue conserve les assignations manuelles, pas les suggestions calculées.
5. Déséquiper une dague et essayer bouclier/orbe puis arme deux mains : la double attaque disparaît immédiatement ; le raccourci 1 redevient l'attaque simple appropriée.
6. Tester absence de cible, portée, mauvais tour, PA insuffisants et ennemi tué par le premier coup ; **pas de deuxième coup sur un cadavre** et aucune dépense en cas de refus.

**Acceptation :** compilation et Automation `Grimrock` réussis sur le SHA exact, plus ces vérifications visuelles PIE. Les deux conditions sont distinctes.
