# Inventory and Item Ownership Design

## 1. Objet du document

Ce document fixe la vision cible du système d’inventaire et de possession des objets pour **GrimrockPrototype**.

Il a pour objectif d’éviter une implémentation par petites couches successives incohérentes. Il doit servir de référence avant toute tâche Codex liée aux objets transportables, à l’équipement, aux personnages, au curseur, aux réceptacles et à la future sauvegarde du groupe.

Ce document décrit le design fonctionnel et architectural de l’onglet **Inventaire**. Il ne décrit pas encore le détail des règles de combat, des compétences, des classes, des recettes ou de la progression.

---

## 2. Référence visuelle cible

La maquette validée pour l’onglet **Inventaire** est organisée en trois grandes zones :

1. **Colonne gauche — Personnages**  
   Liste verticale des personnages du groupe actif, jusqu’à 6 personnages.

2. **Zone centrale — Personnage sélectionné**  
   Fiche détaillée du personnage actif : portrait ou rendu, classe, niveau, expérience, équipement et attributs.

3. **Zone droite — Inventaires personnels**  
   Vue verticale défilante affichant plusieurs inventaires personnels en même temps.

Cette maquette valide les choix suivants :

- le menu principal possède plusieurs onglets ;
- l’onglet **Inventaire** est centré sur un personnage sélectionné ;
- plusieurs inventaires personnels sont visibles simultanément ;
- il n’existe pas d’inventaire de groupe principal ;
- chaque personnage porte ses propres objets ;
- l’écran sert à la fois à consulter, transférer, équiper et organiser les objets.

La maquette cible doit être comprise comme une référence de structure, d’ergonomie et d’intention, pas comme une obligation de reproduire chaque détail graphique à l’identique.

---

## 3. Onglets du menu général

Le menu général du jeu comporte les onglets suivants :

- **Inventaire** ;
- **Compétences** ;
- **Journal** ;
- **Carte** ;
- **Recettes** ;
- **Codex**.

Ce document ne couvre que l’onglet **Inventaire**.

Les autres onglets seront décrits ultérieurement dans des documents dédiés.

---

## 4. Décisions validées

Les décisions suivantes sont validées :

- le jeu commence avec **1 personnage actif** ;
- le groupe actif peut contenir de **1 à 6 personnages** ;
- le groupe actif ne peut pas dépasser **6 personnages** ;
- plus tard, les personnages supplémentaires seront placés dans une réserve ou un pool, par exemple à l’auberge ;
- chaque personnage possède son propre inventaire ;
- il n’y a **pas d’inventaire de groupe principal** ;
- le personnage sélectionné est le **récepteur par défaut** des objets ramassés ;
- le personnage sélectionné est celui qui **tient l’objet** ;
- l’inventaire utilise une **grille simple à cases homogènes** ;
- plusieurs inventaires personnels doivent être visibles en même temps ;
- la zone des inventaires personnels possède une barre de défilement verticale ;
- la charge maximale dépend de la Force du personnage ;
- dépasser la charge maximale provoque pour l’instant un **malus de déplacement** ;
- l’écran Inventaire est un écran de logistique du groupe, pas seulement une fiche individuelle.

---

## 5. Groupe actif et réserve de personnages

Le jeu commence avec un seul personnage actif.

Le groupe actif représente les personnages actuellement présents dans le donjon. Ce sont les personnages qui :

- explorent ;
- combattent ;
- portent des objets ;
- apparaissent dans l’interface principale ;
- peuvent être sélectionnés dans l’onglet **Inventaire** ;
- possèdent chacun leur inventaire personnel et leur équipement.

Le groupe actif peut évoluer au cours de l’aventure :

- le joueur commence seul ;
- il peut rencontrer de nouveaux compagnons ;
- il peut créer ou recruter d’autres personnages ;
- le groupe actif peut atteindre jusqu’à 6 personnages.

La limite du groupe actif est fixée à 6 personnages.

Plus tard, le joueur pourra posséder plus de personnages au total. Les personnages qui ne sont pas dans le groupe actif seront placés dans un **pool de personnages**, une **réserve** ou une **auberge**.

La réserve de personnages permettra à terme :

- de stocker les compagnons non actifs ;
- de recomposer le groupe ;
- de gérer les personnages excédentaires ;
- éventuellement de conserver leurs inventaires personnels et équipements.

Le système doit donc être conçu pour ne jamais supposer que le groupe actif contient toujours exactement 6 personnages.

Règle cible :

> Groupe actif : 1 à 6 personnages.  
> Réserve / auberge : personnages supplémentaires, hors groupe actif.

---

## 6. Principe fondamental : un seul propriétaire à la fois

Règle centrale :

> Un item ne doit appartenir qu’à un seul propriétaire à la fois.

Un objet ne peut pas être simultanément :

- dans le monde ;
- dans un réceptacle ;
- dans l’inventaire d’un personnage ;
- équipé sur un personnage ;
- tenu au curseur ;
- supprimé du niveau ;
- ou restauré comme objet runtime dans le donjon.

Ce principe doit guider toute l’architecture C++.

---

## 7. Absence d’inventaire de groupe principal

Le jeu ne doit pas utiliser de grand inventaire commun comme stockage principal.

Chaque personnage possède :

- son inventaire personnel ;
- son équipement ;
- sa charge actuelle ;
- sa capacité maximale ;
- ses limitations éventuelles de classe, de Force ou de compétence.

Cette règle rend le groupe plus tactique et plus crédible :

- le guerrier peut porter l’armure lourde ;
- le voleur peut garder les outils et les clés ;
- l’alchimiste peut transporter les composants ;
- le mage peut porter les parchemins et objets magiques ;
- le personnage le plus fort peut devenir porteur principal.

Il peut exister plus tard des contenants particuliers, comme des coffres, sacs, réserves ou banques, mais ils ne doivent pas remplacer le principe de base : **chaque personnage possède son propre inventaire personnel**.

---

## 8. Structure de l’écran Inventaire

### 8.1 Bandeau supérieur

Le bandeau supérieur affiche les onglets du menu :

- Inventaire ;
- Compétences ;
- Journal ;
- Carte ;
- Recettes ;
- Codex.

L’onglet actif est clairement mis en évidence.

### 8.2 Colonne gauche : personnages actifs

La colonne gauche affiche les personnages du groupe actif.

Elle doit supporter :

- 1 personnage au début du jeu ;
- 2 à 5 personnages pendant la progression ;
- 6 personnages lorsque le groupe est complet.

Chaque entrée doit pouvoir afficher :

- numéro ou position du personnage ;
- portrait ;
- nom ;
- classe ;
- niveau ;
- icône de rôle ou de spécialisation ;
- éventuellement charge actuelle et statut.

Interaction :

- cliquer sur un personnage le rend sélectionné ;
- le personnage sélectionné devient le récepteur par défaut ;
- le personnage sélectionné est celui dont la fiche centrale est affichée.

Les emplacements non occupés pourront rester vides dans une première version. Plus tard, ils pourront afficher :

- emplacement libre ;
- recruter ;
- créer un personnage ;
- gérer le groupe.

### 8.3 Zone centrale : personnage sélectionné

La zone centrale affiche la fiche détaillée du personnage sélectionné.

Elle contient :

- nom ;
- classe ;
- niveau ;
- expérience ;
- portrait ou rendu complet ;
- équipement ;
- attributs ;
- statistiques principales ;
- charge actuelle et capacité si utile.

L’équipement visible doit au minimum inclure :

- main droite ;
- main gauche ;
- tête ;
- torse ;
- jambes ;
- pieds ;
- amulette ;
- anneau 1 ;
- anneau 2.

Des emplacements supplémentaires pourront être ajoutés plus tard :

- épaules ;
- gants ;
- ceinture ;
- cape ;
- talisman ;
- munitions ;
- objet rapide.

### 8.4 Zone droite : inventaires personnels multiples

La zone droite affiche plusieurs inventaires personnels en même temps.

Elle est organisée comme une liste verticale défilante des personnages actifs :

- Inventaire personnage 1 ;
- Inventaire personnage 2 ;
- Inventaire personnage 3 ;
- etc., jusqu’à 6 personnages actifs.

Chaque bloc contient :

- nom du personnage ;
- indicateur de charge ;
- grille d’inventaire ;
- objets portés.

Cette zone ne doit pas être remplacée par un affichage limité au seul personnage sélectionné.

L’objectif est d’avoir une vue d’ensemble pour :

- repérer rapidement un objet ;
- choisir un objet à équiper sur le personnage sélectionné ;
- transférer des objets entre personnages ;
- vérifier la charge de chacun ;
- organiser le groupe avant ou après un combat.

Si le groupe actif ne contient qu’un seul personnage, la zone droite affiche uniquement son inventaire personnel. Elle s’enrichit naturellement au fur et à mesure que de nouveaux personnages rejoignent le groupe actif.

---

## 9. Grille d’inventaire

L’inventaire utilise une grille à slots simples.

Règles :

- chaque case est homogène ;
- un objet occupe une case ;
- pas d’inventaire type Tetris ;
- pas d’objets 2x3, 1x4 ou de tailles variables dans la première version ;
- certains objets peuvent être empilables ;
- les piles affichent une quantité.

Cette décision privilégie :

- la lisibilité ;
- la simplicité ;
- la robustesse du code ;
- la compatibilité avec le drag & drop.

---

## 10. Personnage sélectionné

Le personnage sélectionné joue un rôle central.

Il est :

- le destinataire par défaut d’un objet ramassé ;
- celui qui tient l’objet ;
- celui qui équipe l’objet si l’action est valide ;
- celui dont on affiche les statistiques et l’équipement ;
- la cible principale des actions rapides d’inventaire.

Exemples :

- si le personnage 1 est sélectionné au début du jeu et que le joueur ramasse une torche, la torche appartient au personnage 1 ;
- si le personnage 4 est sélectionné plus tard et que le joueur ramasse une clé, la clé appartient au personnage 4 ;
- si le joueur prend une épée dans l’inventaire du personnage 2 puis la dépose sur la main droite du personnage sélectionné, l’épée est transférée et équipée sur le personnage sélectionné ;
- si le joueur double-clique sur une potion, l’action par défaut concerne le personnage sélectionné, sauf règle contraire.

---

## 11. Objet au curseur

Le système doit prévoir un objet actuellement manipulé par le joueur.

Cet objet est appelé ici **CursorItem**.

Le CursorItem représente l’objet pris par la souris pendant une opération de déplacement, d’équipement ou de transfert.

Il peut provenir :

- du monde ;
- d’un réceptacle ;
- de l’inventaire d’un personnage ;
- d’un slot d’équipement ;
- d’un futur conteneur.

Le CursorItem n’est pas un inventaire. C’est un état temporaire de manipulation.

Règle :

> Tant qu’un objet est au curseur, il n’appartient plus à son ancien emplacement, mais il n’est pas encore définitivement placé dans un nouvel emplacement.

Il faudra décider plus tard ce qui se passe si le menu est fermé alors qu’un CursorItem existe.

Options possibles :

- remettre l’objet dans son emplacement d’origine ;
- le ranger dans l’inventaire du personnage sélectionné ;
- empêcher la fermeture tant que l’objet est au curseur ;
- valider automatiquement une destination par défaut.

La première version peut rester simple, mais le modèle doit prévoir ce cas.

---

## 12. États de possession d’un item

Un item peut se trouver dans un seul des états suivants :

- **World** : l’objet existe dans le niveau ;
- **Receptacle** : l’objet est contenu dans un réceptacle ;
- **CharacterInventory** : l’objet est dans l’inventaire personnel d’un personnage ;
- **EquipmentSlot** : l’objet est équipé sur un personnage ;
- **Cursor** : l’objet est actuellement tenu au curseur ;
- **HeldBySelectedCharacter** : l’objet est tenu / manipulé par le personnage sélectionné ;
- **Removed** : l’objet est retiré du monde ou consommé.

Ces états doivent être exclusifs.

---

## 13. Flux d’objet principaux

### 13.1 Monde vers personnage sélectionné

Quand un objet est ramassé dans le monde :

1. l’objet quitte le niveau ;
2. l’objet est attribué au personnage sélectionné ;
3. l’objet peut être placé au curseur, équipé ou rangé ;
4. le Runtime Dungeon State doit savoir que l’objet n’est plus dans le niveau.

### 13.2 Réceptacle vers personnage sélectionné

Quand un objet est pris depuis un support, une alcôve ou un réceptacle :

1. le réceptacle perd son contenu ;
2. l’objet devient propriété du personnage sélectionné ;
3. le réceptacle déclenche les événements nécessaires ;
4. le niveau conserve son état runtime.

### 13.3 Inventaire personnel vers équipement

Quand un objet est pris depuis l’inventaire d’un personnage et déposé sur l’équipement du personnage sélectionné :

1. l’objet quitte son inventaire d’origine ;
2. le slot cible est vérifié ;
3. si l’objet est compatible, il est équipé ;
4. si un objet était déjà équipé, il doit être échangé ou déplacé selon les règles UX retenues.

### 13.4 Inventaire personnel vers inventaire personnel

Quand un objet est transféré entre deux personnages :

1. l’objet quitte l’inventaire source ;
2. la destination est vérifiée ;
3. l’objet rejoint l’inventaire cible ;
4. la charge des deux personnages est recalculée.

### 13.5 Équipement vers inventaire

Quand un objet équipé est retiré :

1. le slot d’équipement est vidé ;
2. l’objet retourne au curseur ou dans l’inventaire personnel ;
3. les statistiques du personnage sont recalculées.

### 13.6 Personnage vers réceptacle

Quand un objet est déposé dans un réceptacle :

1. l’objet quitte le personnage ou le curseur ;
2. le réceptacle vérifie s’il accepte l’objet ;
3. l’objet devient propriété du réceptacle ;
4. le réceptacle déclenche ses événements éventuels.

### 13.7 Réserve vers groupe actif

Plus tard, lorsqu’un personnage est transféré depuis la réserve ou l’auberge vers le groupe actif :

1. vérifier que le groupe actif contient moins de 6 personnages ;
2. retirer le personnage du pool ;
3. l’ajouter au groupe actif ;
4. afficher son entrée dans la colonne gauche ;
5. afficher son inventaire personnel dans la zone droite.

### 13.8 Groupe actif vers réserve

Plus tard, lorsqu’un personnage quitte le groupe actif pour rejoindre la réserve :

1. vérifier qu’il n’est pas nécessaire au minimum de groupe actif ;
2. retirer le personnage du groupe actif ;
3. le placer dans le pool ;
4. conserver son inventaire personnel et son équipement selon les règles retenues ;
5. mettre à jour l’interface.

---

## 14. Charge, poids et Force

Chaque objet possède un poids.

La charge d’un personnage est la somme :

- des objets de son inventaire ;
- des objets équipés ;
- éventuellement de l’objet tenu si celui-ci est considéré comme séparé.

La capacité maximale dépend principalement de la Force.

Règle validée :

> Si la charge maximale est dépassée, le personnage subit un malus de déplacement.

La première version peut utiliser une règle simple :

- charge <= capacité : aucun malus ;
- charge > capacité : malus de déplacement.

Plus tard, le malus pourra être progressif :

- léger dépassement ;
- surcharge moyenne ;
- surcharge importante ;
- surcharge extrême.

---

## 15. Types d’objets prévus

Le système doit pouvoir gérer au minimum :

- armes ;
- boucliers ;
- armures ;
- bijoux ;
- torches ;
- clés ;
- gemmes ;
- potions ;
- parchemins ;
- livres ;
- nourriture ;
- composants ;
- objets de quête ;
- recettes ;
- objets divers.

Tous ces objets partagent une logique commune de possession, mais certains auront des comportements spécifiques.

---

## 16. Torche comme cas de référence

La torche est le cas test principal du système.

Elle peut être :

- placée dans le monde ;
- placée sur un support mural ;
- prise par le personnage sélectionné ;
- placée au curseur ;
- équipée en main ;
- rangée dans un inventaire personnel ;
- déposée dans une alcôve ;
- éteinte ou allumée.

La torche permet de tester :

- l’ownership ;
- la lumière ;
- les réceptacles ;
- l’équipement ;
- l’inventaire ;
- la persistance runtime ;
- les transitions de niveau.

---

## 17. Relations avec le Runtime Dungeon State

Le **Runtime Dungeon State** gère l’état vivant des niveaux.

Le futur système d’inventaire doit gérer l’état vivant du groupe et des personnages.

La séparation cible est :

- **DungeonRuntimeState** : portes, objets de niveau, réceptacles, items au sol, transitions, états interactifs ;
- **PartyRuntimeState** : personnages actifs, réserve de personnages, inventaires, équipements, personnage sélectionné, CursorItem, charge, objets possédés.

Un objet porté par un personnage ne doit plus être considéré comme appartenant au niveau.

Un objet déposé dans un réceptacle redevient un élément du niveau courant.

---

## 18. Architecture conceptuelle cible

Le modèle final devrait prévoir les concepts suivants.

### 18.1 Item Definition

Données statiques d’un type d’objet.

Exemples :

- Item_Torch ;
- Item_IronKey ;
- Item_RubyGem ;
- Item_ShortSword ;
- Item_WoodenShield.

Contenu possible :

- identifiant ;
- nom affiché ;
- description ;
- icône ;
- mesh monde ;
- mesh équipé ;
- poids ;
- type ;
- tags ;
- empilable ou non ;
- slots compatibles ;
- effets ;
- comportement d’usage.

### 18.2 Item Instance

Objet réel dans une partie.

Contenu possible :

- RuntimeObjectId ;
- ItemDefinitionId ;
- quantité ;
- propriétaire actuel ;
- propriétaire précédent éventuel ;
- état allumé / éteint ;
- durabilité ;
- état spécifique ;
- transform si dans le monde ;
- données nécessaires à la sauvegarde.

### 18.3 Character Inventory State

État de l’inventaire d’un personnage.

Contenu possible :

- CharacterId ;
- liste ou grille de slots ;
- poids porté ;
- capacité maximale ;
- état de surcharge.

### 18.4 Character Equipment State

État d’équipement d’un personnage.

Contenu possible :

- main droite ;
- main gauche ;
- tête ;
- torse ;
- jambes ;
- pieds ;
- amulette ;
- anneau 1 ;
- anneau 2 ;
- autres slots futurs.

### 18.5 Active Party State

État du groupe actif.

Contenu possible :

- personnages actifs ;
- maximum de personnages actifs = 6 ;
- personnage sélectionné ;
- CursorItem ;
- formation ;
- niveau courant ;
- position et orientation du groupe ;
- inventaires et équipements des personnages actifs.

### 18.6 Character Pool State

État de la réserve de personnages.

Contenu possible :

- personnages hors groupe actif ;
- lieu de réserve, par exemple auberge ;
- inventaires personnels conservés ou transférés selon règle future ;
- équipement conservé ou transféré selon règle future ;
- disponibilité du personnage.

### 18.7 Party State global

État global du groupe et de sa réserve.

Contenu possible :

- ActivePartyState ;
- CharacterPoolState ;
- personnage sélectionné actif ;
- CursorItem ;
- règles de transfert entre groupe actif et réserve.

---

## 19. Interaction souris prévue

L’interface doit être pensée pour la souris.

Actions prévues :

- clic gauche pour sélectionner ou prendre ;
- glisser-déposer pour transférer ;
- dépôt sur slot d’équipement ;
- dépôt sur case d’inventaire ;
- dépôt sur réceptacle ;
- survol pour tooltip ;
- clic droit éventuel pour utiliser ou examiner ;
- double-clic éventuel pour équiper rapidement.

Le comportement exact sera précisé dans un document ou une section UX dédiée avant implémentation UI complète.

---

## 20. Sauvegarde future

Le système doit être conçu pour être sérialisable.

À terme, le SaveGame devra contenir :

- l’état du donjon ;
- l’état du groupe actif ;
- l’état de la réserve de personnages ;
- l’état des personnages ;
- les inventaires ;
- l’équipement ;
- les items au curseur ou règles de résolution du CursorItem ;
- la position du groupe ;
- le niveau courant.

La sauvegarde n’est pas l’objectif immédiat, mais le modèle doit être compatible avec elle.

---

## 21. Ce que ce document exclut pour l’instant

Ce document ne fixe pas encore :

- les règles complètes de classes ;
- les arbres de compétences ;
- les règles de combat ;
- le détail des recettes ;
- le détail du journal ;
- le détail de la carte ;
- le détail du codex ;
- l’équilibrage complet des poids ;
- les règles complètes de surcharge ;
- les règles finales de SaveGame ;
- les règles finales de recrutement ;
- les règles finales de création de personnages ;
- le fonctionnement complet de l’auberge ou de la réserve.

Il fixe uniquement la vision de l’inventaire, de la possession d’objets, du groupe actif et de l’écran Inventaire.

---

## 22. Plan d’implémentation recommandé

### Phase 1 — Modèle de possession d’objets et groupe actif minimal

- définir les états de possession ;
- définir l’Item Instance ;
- gérer le propriétaire actuel ;
- gérer le personnage sélectionné ;
- gérer le CursorItem ;
- initialiser un groupe actif de 1 personnage ;
- prévoir MaxActiveCharacters = 6 ;
- ne pas supposer que le groupe contient toujours 6 personnages.

### Phase 2 — Inventaires personnels

- ajouter un inventaire par personnage actif ;
- gérer les slots homogènes ;
- gérer la charge ;
- gérer l’ajout, le retrait et le transfert.

### Phase 3 — Équipement

- ajouter les slots d’équipement ;
- gérer les compatibilités ;
- transférer inventaire vers équipement ;
- recalculer les statistiques.

### Phase 4 — UI de l’onglet Inventaire

- construire le menu supérieur ;
- construire la colonne personnages actifs ;
- construire la fiche centrale ;
- construire la zone droite défilante d’inventaires personnels ;
- adapter l’UI au nombre réel de personnages actifs ;
- ajouter drag & drop.

### Phase 5 — Intégration gameplay

- ramassage monde vers personnage sélectionné ;
- prise depuis réceptacle ;
- dépôt dans réceptacle ;
- effet des torches ;
- effets de surcharge ;
- intégration avec le Runtime Dungeon State.

### Phase 6 — Recrutement et réserve

- recruter ou créer de nouveaux personnages ;
- ajouter des personnages au groupe actif ;
- limiter le groupe actif à 6 ;
- placer les personnages excédentaires dans une réserve ou une auberge ;
- gérer le transfert entre groupe actif et réserve.

### Phase 7 — Sauvegarde

- sérialiser le Party State ;
- sérialiser les personnages actifs ;
- sérialiser la réserve de personnages ;
- sérialiser les inventaires ;
- sérialiser l’équipement ;
- relier avec le SaveGame du donjon.

---

## Etat d'implementation

Tranche 1 appliquee :

- types de base d'inventaire et de possession ajoutes ;
- composant `UGridPartyInventoryComponent` ajoute ;
- groupe actif initial = 1 personnage ;
- `MaxActiveCharacters = 6` ;
- personnage selectionne et `CursorItem` minimal prevus ;
- `CharacterPool` prevu mais non utilise ;
- diagnostics runtime disponibles.

Tranche 2 appliquee :

- `LogPartyInventoryDiagnostics` expose directement sur `AGrimrockPartyPawn` ;
- pickup d'item au sol branche vers l'inventaire du personnage selectionne ;
- pickup depuis receptacle/support branche progressivement vers l'inventaire du personnage selectionne ;
- `CursorItem` toujours disponible dans le modele, mais pas encore utilise comme destination gameplay principale ;
- poids des items encore provisoire a `0.0` tant qu'aucune definition item dediee ne porte cette donnee.

Tranche 3 appliquee :

- equipement minimal `MainHand` / `OffHand` ajoute ;
- fonctions `EquipItemFromInventorySlot` et `UnequipItemToInventory` ajoutees ;
- relais BlueprintCallable ajoutes sur `AGrimrockPartyPawn` pour equiper/des-equiper le personnage selectionne ;
- diagnostics enrichis avec `Equipment: MainHand=... OffHand=...` ;
- compatibilite item/slot encore provisoire et permissive sur les mains.

Tranche 4 appliquee :

- `UGridItemDefinitionAsset` ajoute ;
- definitions item minimales prevues avec type, poids, nom affiche, icone future, meshes futurs, stack et slots compatibles ;
- `UGridPartyInventoryComponent` resout `ItemDefinitionId` vers une definition via `ItemDefinitions` ;
- le poids et les proprietes simples d'item sont appliques aux instances si une definition existe ;
- compatibilite d'equipement basee sur `CompatibleEquipmentSlots` si definition presente ;
- fallback permissif `MainHand` / `OffHand` conserve si definition absente.

## Definitions d'items minimales

Assets recommandes a creer manuellement dans UE :

`Content/GrimrockPrototype/Core/DataAssets/Items/`

- `DA_Item_Torch` : `ItemDefinitionId=Item_Torch`, `DisplayName=Torche`, `ItemType=Torch`, `Weight=1.0`, `bStackable=false`, `CompatibleEquipmentSlots=MainHand,OffHand`, `bCanEmitLight=true`, `bDefaultLightEnabled=true`.
- `DA_Item_ShortSword` : `ItemDefinitionId=Item_ShortSword`, `DisplayName=Epee courte`, `ItemType=Weapon`, `Weight=2.5`, `CompatibleEquipmentSlots=MainHand`.
- `DA_Item_WoodenShield` : `ItemDefinitionId=Item_WoodenShield`, `DisplayName=Bouclier en bois`, `ItemType=Shield`, `Weight=3.0`, `CompatibleEquipmentSlots=OffHand`.
- `DA_Item_IronKey` : `ItemDefinitionId=Item_IronKey`, `DisplayName=Cle en fer`, `ItemType=Key`, `Weight=0.1`, `CompatibleEquipmentSlots` vide.
- `DA_Item_RubyGem` : `ItemDefinitionId=Item_RubyGem`, `DisplayName=Gemme rubis`, `ItemType=Gem`, `Weight=0.2`, `CompatibleEquipmentSlots` vide.
- `DA_Item_HealthPotion` : `ItemDefinitionId=Item_HealthPotion`, `DisplayName=Potion de soin`, `ItemType=Potion`, `Weight=0.5`, `bStackable=true`, `MaxStackSize=10`, `CompatibleEquipmentSlots` vide.

Non implemente a ce stade :

- UI finale d'inventaire ;
- drag & drop ;
- SaveGame ;
- migration complete des torches, depots et receptacles ;
- equipement gameplay complet ;
- branchement visuel torche / arme / bouclier ;
- compatibilite definitive des items ;
- migration complete des anciens archetypes vers definitions dediees.

---

## Tranche 4B - Placements d'items bases sur ItemDefinition

Objectif :

- les items transportables sont definis par `UGridItemDefinitionAsset` ;
- un item transportable ne doit plus necessiter un `UGridObjectArchetypeAsset` dedie ;
- un placement d'item au sol reference une ItemDefinition ;
- le contenu initial d'un receptacle reference une ItemDefinition ;
- `UGridObjectArchetypeAsset` reste reserve aux objets de niveau fixes/interactifs : portes, boutons, leviers, plaques, escaliers, receptacles, supports, alcoves et decorations.

Etat applique :

- `FGridLevelObjectData` expose `ItemDefinitionAsset` et `ItemDefinitionId` pour les objets `Type=Item` ;
- `FGridObjectBehaviorParams.Item` expose aussi `ItemDefinitionAsset` et `ItemDefinitionId` pour les valeurs par defaut d'archetype ;
- `FGridReceptacleBehaviorParams` expose `InitialContainedItemDefinition` et `InitialContainedItemDefinitionId` ;
- `AGridItemActor` peut etre initialise directement depuis un `UGridItemDefinitionAsset` ou depuis un `ItemDefinitionId` ;
- le spawn runtime d'un item au sol resout la definition dans l'ordre : placement asset, placement id, archetype/default behavior asset, archetype/default behavior id, fallback `ArchetypeId` ;
- le retrait d'un item depuis le monde ou un receptacle cree une instance avec le vrai `ItemDefinitionId` quand il est disponible ;
- `ArchetypeId` reste supporte temporairement comme fallback pour les anciens niveaux et assets.

Compatibilite :

- les anciens objets `Item` bases sur `ArchetypeId=Item_Torch` restent ramassables ;
- les anciens supports et receptacles avec `InitialContainedItemArchetypeId=Item_Torch` restent fonctionnels ;
- les anciens assets qui doublonnent des items ne sont pas supprimes automatiquement ;
- si un conflit de nom existe dans UE, renommer progressivement l'ancien asset ou placer le nouvel asset dans le dossier approprie, sans migration destructive.

Nomenclature officielle :

- `UGridItemDefinitionAsset` : `DA_Item_[NomItem]`.
- Exemples : `DA_Item_Torch`, `DA_Item_IronKey`, `DA_Item_RubyGem`, `DA_Item_HealthPotion`, `DA_Item_ShortSword`, `DA_Item_WoodenShield`.
- Ne pas utiliser `DA_ItemDef_*`.
- Ne pas utiliser `DA_Object_Item_*` comme modele final.
- `UGridObjectArchetypeAsset` reste pour les objets fixes/interactifs, par exemple `DA_Object_TorchHolder`, `Receptacle_TorchHolder`, `Door_Stone`, `Button_Normal`, `Lever`, `PressurePlate`, `Stairs_Down`, `Stairs_Up`.

Prochaines etapes :

- exposer proprement les nouveaux champs dans l'inspecteur du Grid Editor Mode ;
- nettoyer progressivement les anciens archetypes d'items en double apres validation des ItemDefinitions ;
- ne pas faire de migration automatique destructive.

---

## Tranche 4C - Nettoyage du workflow editeur des ItemDefinitions

Objectif :

- clarifier le workflow editeur des items transportables ;
- rendre visible la separation officielle entre `UGridItemDefinitionAsset` et `UGridObjectArchetypeAsset` ;
- diagnostiquer les anciens placements et receptacles qui utilisent encore les fallbacks legacy ;
- preparer la suppression progressive des anciens doublons sans modifier ni supprimer automatiquement les assets UE.

Nomenclature officielle :

- les nouveaux items transportables sont des `UGridItemDefinitionAsset` ;
- leur nom d'asset suit `DA_Item_[NomItem]` ;
- exemples : `DA_Item_Torch`, `DA_Item_IronKey`, `DA_Item_RubyGem`, `DA_Item_HealthPotion`, `DA_Item_ShortSword`, `DA_Item_WoodenShield` ;
- ne pas recommander `DA_ItemDef_*`, `DA_Object_Item_*` ou `DA_Archetype_Item_*`.

Separation officielle :

- `UGridItemDefinitionAsset` est la source de verite des items transportables : torches, cles, gemmes, armes, boucliers, potions, parchemins, nourriture, composants ;
- `UGridObjectArchetypeAsset` reste reserve aux objets de niveau fixes/interactifs : portes, boutons, leviers, plaques, escaliers, receptacles, supports, alcoves, decorations ;
- les anciens archetypes qui representent uniquement un item transportable sont legacy ;
- `ArchetypeId` reste temporairement supporte comme fallback pour ne pas casser les niveaux existants.

Workflow recommande pour creer un item :

1. Creer un DataAsset de classe `UGridItemDefinitionAsset`.
2. Le nommer `DA_Item_[NomItem]`.
3. Renseigner `ItemDefinitionId`.
4. Renseigner `DisplayName`, `Weight`, `ItemType`, `WorldMesh`, `Icon` et les slots compatibles.
5. Ajouter l'asset dans `PartyInventoryComponent.ItemDefinitions` si l'item doit appliquer poids, type et compatibilites d'equipement.
6. Utiliser cet asset dans les placements `Type=Item` ou les receptacles.

Workflow recommande pour placer un item au sol :

1. Creer ou selectionner un objet de niveau `Type=Item`.
2. Renseigner `ItemDefinitionAsset`.
3. Synchroniser `ItemDefinitionId` depuis l'asset si necessaire.
4. Garder `ArchetypeId` seulement comme fallback legacy ou comme aide temporaire de placement/visualisation.

Workflow recommande pour mettre un item dans un receptacle :

1. Selectionner le receptacle dans le Grid Editor.
2. Renseigner `InitialContainedItemDefinition`.
3. Synchroniser `InitialContainedItemDefinitionId` depuis l'asset si necessaire.
4. Garder `InitialContainedItemArchetypeId` seulement comme fallback legacy.

Diagnostics et helpers editeur :

- l'inspecteur affiche une section `Item Definition` pour les objets `Type=Item` ;
- l'inspecteur affiche une section `Initial Contained Item` pour les receptacles ;
- `LogItemWorkflowDiagnostics` liste les placements et receptacles avec un statut `OK_*`, `LEGACY_*` ou `ERROR_*` ;
- les helpers `SyncSelectedItemDefinitionIdFromAsset` et `SyncSelectedReceptacleInitialItemDefinitionIdFromAsset` recopient uniquement les ids depuis les assets ; ils ne suppriment aucun champ legacy.

## Nettoyage progressif des anciens items doublons

Les anciens assets de type `UGridObjectArchetypeAsset` utilises uniquement pour representer un item transportable deviennent obsoletes. Ils ne doivent plus etre utilises pour creer de nouveaux items.

| Cas | Statut |
| --- | --- |
| `DA_Item_Torch` comme `UGridObjectArchetypeAsset` | obsolete |
| `DA_Item_Torch` comme `UGridItemDefinitionAsset` | officiel |

Procedure de nettoyage d'un ancien doublon :

1. Identifier l'ancien asset item base sur `UGridObjectArchetypeAsset`.
2. Verifier ses references avec Reference Viewer.
3. Migrer les placements au sol vers `DA_Item_[NomItem]` de type `UGridItemDefinitionAsset`.
4. Migrer les receptacles vers `InitialContainedItemDefinition`.
5. Si un conflit de nom existe, renommer temporairement l'ancien asset en `Legacy_Item_[NomItem]` ou `Archetype_Legacy_Item_[NomItem]`.
6. Supprimer manuellement l'ancien asset legacy seulement quand plus aucune reference ne subsiste.

Cette tranche ne supprime pas automatiquement les assets, ne renomme pas automatiquement les assets, et ne bloque pas le gameplay legacy. Les diagnostics encouragent la migration progressive.

---

## Tranche 4D appliquee

Objectif :

- supprimer l'ancien inventaire legacy porte par `AGrimrockPartyPawn` ;
- faire de `UGridPartyInventoryComponent` l'unique source de verite de l'inventaire du groupe ;
- eviter les doubles ajouts lors des pickups monde et receptacle ;
- garder l'auto-equipement de la torche apres validation de l'ajout au nouvel inventaire.

Etat applique :

- `InventoryItems` legacy est supprime de `AGrimrockPartyPawn` ;
- `AddInventoryItem` et `RemoveInventoryItem` legacy sont supprimes ;
- `HasInventoryItem` reste uniquement comme facade vers l'inventaire du personnage selectionne ;
- `UGridPartyInventoryComponent` expose les helpers de presence, comptage et retrait par `ItemDefinitionId` ;
- les pickups monde alimentent directement l'inventaire du personnage selectionne ;
- les pickups depuis receptacle alimentent directement l'inventaire du personnage selectionne ;
- le depot dans un receptacle retire l'item depuis l'inventaire du personnage selectionne ;
- l'auto-equipement de la torche est declenche apres ajout reussi dans `UGridPartyInventoryComponent`.

Resultat attendu :

- une prise de torche produit une seule ligne `GridInventory Pickup AddedToSelectedCharacter` ;
- une prise de torche ne produit aucune ligne `Held item equipped: Item_Torch` ;
- aucune ligne `Inventory Add` legacy ne doit apparaitre.

---

## Tranche 4E appliquee

Etat applique :

- le vocabulaire held item utilise desormais `ItemDefinitionId` ;
- `DefaultHeldItemDefinitionId`, `HeldItemDefinitionId` et `GetHeldItemDefinitionId` remplacent les anciens noms historiques bases sur `ArchetypeId` ;
- les noms `ArchetypeId` restent reserves aux objets de niveau fixes/interactifs et aux fallbacks legacy explicites ;
- la torche en main reste le cas de reference `Item_Torch`.

---

## Tranche 5A appliquee

Etat applique :

- `CursorItem` devient un vrai etat temporaire exclusif de possession ;
- prendre un item depuis l'inventaire vers `CursorItem` retire l'item de l'inventaire source ;
- deposer `CursorItem` dans un inventaire transfere l'item au personnage cible ;
- `HeldItemActor` reste une representation visuelle et ne porte pas l'ownership reelle de l'item ;
- la torche tenue en main reste un held visual, pas un `EquipmentSlot` ;
- les diagnostics anti-doublons `ValidateInventoryOwnership` et `LogInventoryOwnershipDiagnostics` sont disponibles.

---

## Tranche 5B appliquee

Etat applique :

- pickup monde et receptacle = transfert vers l'inventaire du personnage actif uniquement ;
- une torche dans l'inventaire ne declenche pas la lumiere du groupe ;
- la lumiere de torche depend uniquement d'un item equipe/tenu explicitement ;
- l'ancien auto-held-on-pickup est supprime ;
- le pickup torche attend une seule ligne `GridInventory Pickup AddedToSelectedCharacter` et aucune ligne `Held item equipped`.

---

## Tranche 5C appliquee

Etat applique :

- l'equipement explicite `MainHand` ou `OffHand` synchronise `HeldItemActor` depuis l'equipement du personnage selectionne ;
- le pickup ne declenche toujours pas `HeldItemActor` ;
- `HeldItemActor` represente l'equipement reel en main, pas un item simplement possede ;
- la torche eclaire uniquement lorsqu'elle est equipee dans une main ;
- le slot `MainHand` est prioritaire pour le visuel tenu, puis `OffHand`.

---

## Tranche 5D appliquee

Etat applique :

- `CursorItem` peut etre equipe vers `MainHand` ou `OffHand` ;
- deposer `CursorItem` sur un slot vide transfere l'ownership vers `EquipmentSlot` ;
- deposer `CursorItem` sur un slot occupe effectue un swap simple avec l'ancien item equipe ;
- `HeldItemActor` est synchronise depuis l'equipement apres equipement depuis `CursorItem` ;
- le pickup reste inventaire uniquement.

---

## Tranche 5E appliquee

Etat applique :

- `CursorItem` peut etre depose dans un receptacle ;
- l'insertion reussie transfere l'ownership vers le receptacle ;
- `CursorItem` est vide uniquement apres acceptation par le receptacle ;
- en cas de refus, `CursorItem` reste intact ;
- support de torche : inventaire -> `CursorItem` -> support est le cas de reference ;
- le depot direct depuis inventaire selectionne ou item equipe n'est plus autorise ;
- le pickup reste inventaire uniquement ;
- `HeldItemActor` n'est pas concerne par le depot receptacle.

---

## Tranche 5F appliquee

Etat applique :

- un item equipe peut etre pris au `CursorItem` ;
- le slot source est vide immediatement apres prise ;
- le held visual est resynchronise apres retrait d'equipement ;
- une torche au `CursorItem` n'eclaire pas le groupe ;
- une torche peut maintenant suivre le flux `MainHand` -> `CursorItem` -> receptacle ;
- l'ownership exclusif est conserve.

---

## Tranche 6A appliquee

Etat applique :

- premiere UI runtime UMG minimale via `UGridInventoryWidget` ;
- affichage possible de l'inventaire du personnage selectionne ;
- affichage possible de `MainHand`, `OffHand` et `CursorItem` ;
- clic slot inventaire : inventaire vers `CursorItem`, ou `CursorItem` vers premier slot libre ;
- clic main : main vers `CursorItem`, ou `CursorItem` vers `EquipmentSlot` ;
- retour `CursorItem` vers inventaire expose ;
- pas encore de drag and drop UMG final ;
- pas encore d'UI multi-personnage complete ;
- pas encore de style final.

## Tranche 6B appliquee

Etat applique :

- stabilisation UI minimale ;
- curseur dore custom = seul curseur autorise ;
- curseur Windows interdit ;
- `WBP_GridInventory` root Canvas Panel doit avoir `Cursor=None` ;
- `CustomCursorWidget` visible et `HitTestInvisible` ;
- boutons UMG cliquables ;
- `RefreshInventory` fiable ;
- `Slot 0`, `MainHand`, `OffHand` et `Return Cursor` valides ;
- pas encore de slot widget dedie ;
- pas encore icones ;
- pas encore drag and drop UMG ;
- pas encore UI multi-personnages complete.

## Tranche 6C appliquee

Etat applique :

- creation d'un widget de slot dedie ;
- slots `Inventory`, `MainHand`, `OffHand` et `Cursor` ;
- affichage nom, quantite et icone optionnelle ;
- tooltip simple ;
- root `Cursor=None` a appliquer dans le Blueprint pour eviter le curseur Windows ;
- pas encore de vrai drag and drop ;
- pas encore UI multi-personnages complete ;
- ownership inchange.

## Tranche 6D appliquee

Etat applique :

- UI multi-personnages visible exposee au Blueprint ;
- affichage des personnages actifs via un resume personnage ;
- selection du personnage actif depuis l'UI ;
- `RefreshInventory` affiche l'inventaire et l'equipement du personnage selectionne ;
- `CursorItem` reste global ;
- le changement de personnage ne deplace pas automatiquement `CursorItem` ;
- depot `CursorItem` vers inventaire utilise le personnage selectionne ;
- pas encore de drag and drop ;
- pas encore fiche personnage complete ;
- curseur dore custom reste seul curseur autorise.

## Tranche 6E appliquee

Etat applique :

- ajout du drag and drop UMG entre slots d'inventaire, d'equipement et `CursorItem` ;
- l'operation de drag transporte le slot source et des donnees item de debug ;
- l'ownership reel reste dans `CursorItem` et `UGridPartyInventoryComponent` ;
- le clic simple est conserve pour debug et accessibilite ;
- curseur dore custom = seul curseur autorise ;
- curseur Windows interdit ;
- pas encore de drop monde ou receptacle ;
- pas encore de drag visual final ;
- `TargetIndex` inventaire est informatif tant que le placement precis dans un slot cible n'est pas supporte.

Dans WBP_GridInventory, l’override Blueprint RefreshInventory doit appeler :
- RefreshRegisteredPartyMemberWidgets
- RefreshRegisteredSlotWidgets

Sinon les slots dédiés peuvent afficher du texte correct via bindings, mais leur état interne CachedItem/bHasItem ne sera pas mis à jour, ce qui bloque CanStartDrag.

---

## Tranche 6E.1 appliquee

Etat applique :

- generation automatique des `InventorySlot` widgets depuis `UGridInventoryWidget` ;
- `InventorySlotWidgetClass` definit la classe de widget a instancier, typiquement `WBP_InventorySlot` ;
- `InventorySlotsGridPanel` est le `UniformGridPanel` UMG cible pour les slots generes ;
- `InventorySlotColumnCount` controle le nombre de colonnes de la grille ;
- `InventorySlotCountOverride` permet de forcer un nombre de slots, avec resolution automatique depuis `MaxInventorySlots` du personnage selectionne quand il vaut `0` ;
- les slots speciaux `MainHand`, `OffHand` et `Cursor` restent explicites et ne sont pas supprimes par la generation ;
- les inventory slots n'ont plus besoin d'etre enregistres un par un dans le Blueprint ;
- `RefreshInventory` ne doit pas lire manuellement chaque slot, il doit continuer a appeler `RefreshRegisteredPartyMemberWidgets` et `RefreshRegisteredSlotWidgets` ;
- `RefreshRegisteredSlotWidgets` reste le point central de rafraichissement des slots ;
- le drag and drop de la Tranche 6E est conserve ;
- le modele d'ownership reste inchange ;
- pas encore de multi-inventaires simultanes pour les 6 personnages ;
- pas encore de placement precis par `TargetIndex` dans un slot d'inventaire cible.

Configuration Blueprint attendue :

- dans `WBP_GridInventory`, le `UniformGridPanel` d'inventaire doit s'appeler `InventorySlotsGridPanel`, ou etre assigne via `SetInventorySlotsGridPanel` ;
- `InventorySlotWidgetClass` doit pointer vers `WBP_InventorySlot` ;
- `InventorySlotColumnCount = 6` pour la grille actuelle ;
- `InventorySlotCountOverride = 24` pour la validation initiale ;
- les anciens `RegisterInventorySlotWidget` pour `SlotWidget_0`, `SlotWidget_1`, etc. doivent etre supprimes ou ignores ;
- les registrations Blueprint pour `SlotWidget_MainHand`, `SlotWidget_OffHand` et `SlotWidget_Cursor` restent necessaires ;
- `WBP_GridInventory` root Canvas Panel doit rester `Cursor=None` ;
- `WBP_InventorySlot` root doit rester `Cursor=None`.

---

## Tranche 6F appliquee

Etat applique :

- `CursorItem` peut etre depose dans un receptacle monde compatible ;
- le depot monde est prioritaire sur l'interaction normale quand `CursorItem` est occupe ;
- `CanAcceptItemInstance` et `CanAcceptCursorItemFromParty` separent le test de compatibilite de la mutation ;
- le curseur visuel expose les etats `PlaceItem` et `CannotPlaceItem` ;
- le curseur dore custom reste le seul curseur autorise ;
- le curseur Windows reste interdit ;
- le modele d'ownership reste inchange ;
- pas de depot direct depuis `MainHand` ou `OffHand` vers le monde ;
- pas encore de drag direct UI -> monde sans passer par `CursorItem` ;
- `RefreshInventory` reste centralise apres les mutations d'inventaire.

Flux valide :

1. L'objet est pris vers `CursorItem` depuis l'inventaire ou l'equipement.
2. L'inventaire peut etre ferme sans afficher le curseur Windows.
3. Le survol d'un receptacle compatible affiche `PlaceItem`.
4. Le survol d'une cible incompatible ou vide affiche `CannotPlaceItem`.
5. Le clic monde tente `TryPlaceCursorItemInReceptacle`.
6. Le receptacle accepte uniquement si le test de compatibilite est valide.
7. `CursorItem` est vide uniquement apres insertion acceptee.
8. Les diagnostics d'ownership restent le controle central apres mutation.

---

## Tranche 6G appliquee

Etat applique :

- `TargetIndex` d'inventaire devient effectif pendant les drops UMG ;
- `Inventory -> Inventory` deplace precisement vers le slot cible quand il est vide ;
- `Inventory -> Inventory` swap les deux slots quand la cible est occupee ;
- `Cursor -> InventorySlot` place l'objet au slot cible exact ;
- `Cursor -> InventorySlot` swap avec l'ancien item du slot cible et conserve cet ancien item dans `CursorItem` ;
- `MainHand -> InventorySlot` et `OffHand -> InventorySlot` passent par `CursorItem` puis utilisent le slot cible exact ;
- si le slot cible est occupe pendant un depot depuis l'equipement, l'ancien item cible reste dans `CursorItem` ;
- `RefreshInventory` reste centralise apres les drops ;
- `RebuildInventorySlotWidgets` n'est pas appele pendant les drops ;
- le modele d'ownership reste inchange ;
- aucun systeme d'inventaire Blueprint parallele n'est ajoute.

Limites conservees :

- le placement precis concerne le personnage selectionne ;
- le drop direct UI -> monde passe toujours par `CursorItem` ;
- `FGridItemInstance` ne porte pas d'index de slot persistant, donc seul l'array `InventorySlots` definit la position.

---

## Tranche 6H appliquee

Etat applique :

- `RefreshInventory_Implementation` reste limite a `RefreshRegisteredPartyMemberWidgets` et `RefreshRegisteredSlotWidgets` ;
- le log C++ de `RefreshInventory` passe en `Verbose` pour eviter le spam ;
- `RebuildInventorySlotWidgets` possede un guard C++ contre les reconstructions identiques ;
- le guard compare le nombre de slots, le nombre de colonnes, la classe de slot et le grid panel cible ;
- `ClearGeneratedInventorySlotWidgets` remet le guard de rebuild a zero ;
- `RebuildInventorySlotWidgets` n'est pas appele pendant les drops ;
- les slots vides ne renvoient plus de nom affiche C++ ;
- `GetQuantityText` affiche maintenant `1` quand un item est present avec quantite 1 ;
- le log `DragStarted` passe en `Verbose` ;
- le curseur Windows reste interdit et le curseur custom reste `HitTestInvisible` ;
- le modele d'ownership reste inchange ;
- aucun systeme d'inventaire Blueprint parallele n'est ajoute.

### Reglages Blueprint obligatoires

`WBP_GridInventory` :

- Canvas Panel root `Cursor=None` ;
- `InventorySlotsGridPanel` doit etre nomme exactement ainsi si `BindWidgetOptional` est utilise ;
- `InventorySlotWidgetClass = WBP_InventorySlot` ;
- `InventorySlotColumnCount = 6` ;
- `InventorySlotCountOverride = 24` pour les tests actuels ;
- `Event Construct` doit enregistrer `MainHand`, `OffHand`, `Cursor` et les `PartyMember` widgets ;
- `Event Construct` peut appeler `RebuildInventorySlotWidgets`, le guard C++ evite les reconstructions identiques ;
- `Event RefreshInventory` doit appeler `RefreshRegisteredPartyMemberWidgets` et `RefreshRegisteredSlotWidgets` ;
- `Event RefreshInventory` ne doit pas faire de `GetInventoryAtSlot` manuel slot par slot.

`WBP_InventorySlot` :

- root `Cursor=None` ;
- surface dragable hit-testable ;
- enfants texte/image `Not Hit-Testable` ;
- `OnMouseButtonDown` doit mener a `DetectDragIfPressed` ou laisser le chemin C++ natif le faire ;
- `OnDragDetected` doit creer une operation compatible ou laisser le chemin C++ natif le faire ;
- `OnDrop` doit transmettre le `TargetIndex` reel, ou laisser `NativeOnDrop` C++ le faire ;
- `RefreshSlotVisual` doit mettre a jour `Text_Name`, `Text_Quantity`, tooltip et icone ;
- `Image_Icon` doit rester dans la hierarchie et etre cachee si aucune icone n'est disponible.

`WBP_PartyMember` :

- root `Cursor=None` ;
- bouton ou surface hit-testable ;
- enfants `Not Hit-Testable` ;
- `RefreshMemberVisual` doit mettre a jour nom, classe, poids et selection.

`WBP_GridMouseCursor` :

- doit gerer `Default`, `Use`, `Take`, `Push`, `Pull`, `Read`, `PlaceItem` et `CannotPlaceItem` ;
- doit rester visible, enabled et `HitTestInvisible` ;
- ne doit jamais reintroduire le curseur Windows.

---

## Tranche 7A - intervention Blueprint requise

Objectif :

- ameliorer visuellement `WBP_GridInventory` selon la maquette inventaire ;
- conserver l'ownership existant ;
- conserver les slots generes automatiquement ;
- conserver le drag and drop C++ existant ;
- conserver le curseur custom comme seul curseur autorise.

Etat verifie cote depot :

- `WBP_GridInventory` contient deja des elements de structure attendus : onglets superieurs, colonne party, panneau personnage, panneau inventaire, `InventorySlotsGridPanel`, `SlotWidget_MainHand`, `SlotWidget_OffHand`, `SlotWidget_Cursor` et `PartyMember_0..5` ;
- `InventorySlotsGridPanel` reste le nom obligatoire pour le bind C++ ;
- `WBP_InventorySlot` contient encore des traces Blueprint de debug visibles dans l'asset binaire : `PrintString`, `SLOT MOUSE DOWN`, `SLOT CAN DRAG`, `SLOT CANNOT DRAG`, `Hello`, ainsi que des libelles de variables `Drag Enabled` et `Has Item` ;
- ces traces doivent etre supprimees dans l'editeur UE, pas par edition binaire directe du `.uasset`.

### Reglages Blueprint 7A a appliquer

`WBP_GridInventory` :

- root Canvas Panel `Cursor=None` ;
- `Border_RootFrame` ancre plein ecran ou quasi plein ecran ;
- fond sombre bleu/noir avec alpha eleve ;
- `VerticalBox_Root` avec onglets en haut et contenu principal dessous ;
- onglets visibles : Inventaire, Competences, Journal, Carte, Recettes, Codex ;
- seul l'onglet Inventaire est actif en 7A ;
- colonne party a gauche avec `PartyMember_0..5` ;
- panneau personnage selectionne au centre avec titre, resume, portrait placeholder, stats placeholder et slots `MainHand`, `OffHand`, `Cursor` ;
- panneau inventaire a droite avec titre et `InventorySlotsGridPanel` ;
- `InventorySlotsGridPanel` doit rester vide dans le Designer ;
- ne pas ajouter de slots inventaire manuels ;
- `InventorySlotWidgetClass = WBP_InventorySlot` ;
- `InventorySlotColumnCount = 6` ;
- `InventorySlotCountOverride = 24` pour les tests actuels.

`WBP_InventorySlot` :

- root Canvas Panel `Cursor=None` ;
- surface principale hit-testable ;
- enfants texte/image `Not Hit-Testable` ;
- taille cible 64x64 ;
- `Text_Name` centre avec police 9-10 ;
- `Text_Quantity` en bas a droite avec police 10-11 ;
- `Image_Icon` conservee dans la hierarchie ;
- `RefreshSlotVisual` doit mettre a jour `Text_Name`, `Text_Quantity`, tooltip et icone ;
- cacher `Image_Icon` si `GetIconTexture` retourne null ;
- supprimer tous les `PrintString` debug : `SLOT MOUSE DOWN`, `SLOT CAN DRAG`, `SLOT CANNOT DRAG`, `Hello`.

`WBP_PartyMember` :

- root `Cursor=None` ;
- surface principale hit-testable ;
- textes enfants `Not Hit-Testable` ;
- largeur cible 145-160 et hauteur 80-90 ;
- afficher nom, classe/niveau, poids ;
- afficher visuellement l'etat selectionne.

`WBP_GridMouseCursor` :

- rester visible, enabled et `HitTestInvisible` ;
- gerer `Default`, `Use`, `Take`, `Push`, `Pull`, `Read`, `PlaceItem` et `CannotPlaceItem` ;
- ne jamais reintroduire le curseur Windows.

Contraintes 7A :

- pas de changement d'ownership ;
- pas de changement de `UGridPartyInventoryComponent` ;
- pas de nouveau systeme d'inventaire Blueprint ;
- pas de drop direct UI -> monde sans `CursorItem` ;
- pas de multi-inventaires simultanes dans cette tranche.

---

## Tranche 7A.1 — Cadre responsive 1600x900

Objectif :

- rendre `WBP_GridInventory` adaptable aux resolutions ecran ;
- eviter un inventaire etire en 4K ou trop grand en Full HD ;
- poser une base commune pour Inventaire, Competences, Journal, Carte, Recettes, Codex.

Hierarchie Blueprint cible :

```text
Canvas Panel
└── SafeZone_Inventory
    └── ScaleBox_InventoryRoot
        └── SizeBox_InventoryDesign
            └── Border_RootFrame
                └── VerticalBox_Root
```

Reglages :

- Canvas root `Cursor=None` ;
- SafeZone ancre plein ecran ;
- ScaleBox `Stretch=Scale To Fit` ;
- `SizeBox_InventoryDesign = 1600 x 900` ;
- `Border_RootFrame` Fill + Padding 12/16 ;
- `SizeBox_TopTabs` en Auto ;
- `HorizontalBox_MainContent` en Fill.

Contraintes :

- ne pas modifier ownership ;
- ne pas modifier drag/drop ;
- ne pas renommer `InventorySlotsGridPanel` ;
- ne pas ajouter de slots inventaire manuels ;
- conserver le curseur custom.

---

## Tranche 7A.2 - TopTabs consolides

Objectif :

- fournir la navigation entre les six pages principales avant l'ajout de leur contenu ;
- conserver la structure responsive 1600 x 900 ;
- laisser la logique d'inventaire, l'ownership et le drag and drop inchanges.

Fonctionnement :

- `EInventoryTopTab` contient `Inventory`, `Skills`, `Journal`, `Map`, `Recipes` et `Codex` ;
- `WidgetSwitcher_MainContent` est l'enfant unique de `HorizontalBox_MainContent` ;
- le switcher contient exactement six widgets nommes `Page_Inventory`, `Page_Skills`, `Page_Journal`, `Page_Map`, `Page_Recipes` et `Page_Codex` ;
- le contenu d'inventaire existant reste dans `Page_Inventory` sans modification de sa logique ;
- `SetActiveTopTab` utilise une association explicite entre chaque valeur de `EInventoryTopTab` et son widget nomme, sans dependre de la valeur numerique de l'enum ;
- `CurrentTopTab` conserve la page active lorsque `NativeConstruct` est appele de nouveau sur la meme instance ;
- `bTopTabsInitialized` empeche la recreation du switcher et des pages apres une initialisation valide ;
- `Inventory` est la page active lors de la premiere initialisation.

Styles des boutons :

- chaque bouton conserve ses styles `Normal`, `Hovered` et `Pressed` configures dans `WBP_GridInventory` ;
- le bouton actif utilise `T_ButtonTab_Selected_480x100` pour ses etats `Normal`, `Hovered` et `Pressed` ;
- les boutons inactifs recuperent leur style d'origine ;
- les bindings `OnClicked` sont reposes de maniere idempotente avec `RemoveDynamic` puis `AddDynamic`.

Contraintes conservees :

- aucun changement d'ownership ;
- aucun changement du drag and drop ;
- `InventorySlotsGridPanel` conserve son nom et son fonctionnement ;
- aucun slot inventaire manuel n'est ajoute.

---

## 23. Conclusion

La vision retenue est celle d’un système d’inventaire RPG complet, centré sur les personnages.

Le jeu ne repose pas sur un inventaire commun, mais sur un groupe actif de 1 à 6 personnages possédant chacun leur propre équipement, leur propre inventaire et leur propre charge.

Le joueur commence avec un seul personnage, puis compose progressivement son groupe jusqu’à un maximum de 6 personnages actifs.

Les personnages supplémentaires seront à terme gérés dans une réserve ou une auberge.

L’écran Inventaire doit permettre une gestion globale et fluide du groupe : sélectionner un personnage, consulter son équipement, voir les inventaires de tous les membres actifs, transférer des objets, équiper, organiser, et préparer l’exploration ou le combat.

Ce document doit rester la référence de conception avant toute implémentation Codex liée à l’inventaire.
