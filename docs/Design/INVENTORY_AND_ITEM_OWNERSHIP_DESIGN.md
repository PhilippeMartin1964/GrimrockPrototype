# Inventory and Item Ownership Design

## 1. Objet du document

Ce document fixe la vision cible du système d’inventaire et de possession des objets pour **GrimrockPrototype**.

Il a pour objectif d’éviter une implémentation par petites couches successives incohérentes. Il doit servir de référence avant toute tâche Codex liée aux objets transportables, à l’équipement, aux personnages, au curseur, aux réceptacles et à la future sauvegarde du groupe.

Ce document décrit le design fonctionnel et architectural de l’onglet **Inventaire**. Il ne décrit pas encore le détail des règles de combat, des compétences, des classes, des recettes ou de la progression.

---

## 2. Référence visuelle cible

La maquette validée pour l’onglet **Inventaire** est organisée en trois grandes zones :

1. **Colonne gauche — Personnages**  
   Liste verticale des 6 personnages du groupe.

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

- le groupe peut contenir **6 personnages** ;
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

## 5. Principe fondamental : un seul propriétaire à la fois

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

## 6. Absence d’inventaire de groupe principal

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

Il peut exister plus tard des contenants particuliers, comme des coffres, sacs, réserves ou banques, mais ils ne doivent pas remplacer le principe de base : **6 personnages = 6 inventaires personnels**.

---

## 7. Structure de l’écran Inventaire

### 7.1 Bandeau supérieur

Le bandeau supérieur affiche les onglets du menu :

- Inventaire ;
- Compétences ;
- Journal ;
- Carte ;
- Recettes ;
- Codex.

L’onglet actif est clairement mis en évidence.

### 7.2 Colonne gauche : personnages

La colonne gauche affiche les 6 personnages du groupe.

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

### 7.3 Zone centrale : personnage sélectionné

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

### 7.4 Zone droite : inventaires personnels multiples

La zone droite affiche plusieurs inventaires personnels en même temps.

Elle est organisée comme une liste verticale défilante :

- Inventaire personnage 1 ;
- Inventaire personnage 2 ;
- Inventaire personnage 3 ;
- Inventaire personnage 4 ;
- Inventaire personnage 5 ;
- Inventaire personnage 6.

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

---

## 8. Grille d’inventaire

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

## 9. Personnage sélectionné

Le personnage sélectionné joue un rôle central.

Il est :

- le destinataire par défaut d’un objet ramassé ;
- celui qui tient l’objet ;
- celui qui équipe l’objet si l’action est valide ;
- celui dont on affiche les statistiques et l’équipement ;
- la cible principale des actions rapides d’inventaire.

Exemples :

- si le personnage 4 est sélectionné et que le joueur ramasse une torche, la torche appartient au personnage 4 ;
- si le joueur prend une épée dans l’inventaire du personnage 2 puis la dépose sur la main droite du personnage sélectionné, l’épée est transférée et équipée sur le personnage sélectionné ;
- si le joueur double-clique sur une potion, l’action par défaut concerne le personnage sélectionné, sauf règle contraire.

---

## 10. Objet au curseur

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

## 11. États de possession d’un item

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

## 12. Flux d’objet principaux

### 12.1 Monde vers personnage sélectionné

Quand un objet est ramassé dans le monde :

1. l’objet quitte le niveau ;
2. l’objet est attribué au personnage sélectionné ;
3. l’objet peut être placé au curseur, équipé ou rangé ;
4. le Runtime Dungeon State doit savoir que l’objet n’est plus dans le niveau.

### 12.2 Réceptacle vers personnage sélectionné

Quand un objet est pris depuis un support, une alcôve ou un réceptacle :

1. le réceptacle perd son contenu ;
2. l’objet devient propriété du personnage sélectionné ;
3. le réceptacle déclenche les événements nécessaires ;
4. le niveau conserve son état runtime.

### 12.3 Inventaire personnel vers équipement

Quand un objet est pris depuis l’inventaire d’un personnage et déposé sur l’équipement du personnage sélectionné :

1. l’objet quitte son inventaire d’origine ;
2. le slot cible est vérifié ;
3. si l’objet est compatible, il est équipé ;
4. si un objet était déjà équipé, il doit être échangé ou déplacé selon les règles UX retenues.

### 12.4 Inventaire personnel vers inventaire personnel

Quand un objet est transféré entre deux personnages :

1. l’objet quitte l’inventaire source ;
2. la destination est vérifiée ;
3. l’objet rejoint l’inventaire cible ;
4. la charge des deux personnages est recalculée.

### 12.5 Équipement vers inventaire

Quand un objet équipé est retiré :

1. le slot d’équipement est vidé ;
2. l’objet retourne au curseur ou dans l’inventaire personnel ;
3. les statistiques du personnage sont recalculées.

### 12.6 Personnage vers réceptacle

Quand un objet est déposé dans un réceptacle :

1. l’objet quitte le personnage ou le curseur ;
2. le réceptacle vérifie s’il accepte l’objet ;
3. l’objet devient propriété du réceptacle ;
4. le réceptacle déclenche ses événements éventuels.

---

## 13. Charge, poids et Force

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

## 14. Types d’objets prévus

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

## 15. Torche comme cas de référence

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

## 16. Relations avec le Runtime Dungeon State

Le **Runtime Dungeon State** gère l’état vivant des niveaux.

Le futur système d’inventaire doit gérer l’état vivant du groupe et des personnages.

La séparation cible est :

- **DungeonRuntimeState** : portes, objets de niveau, réceptacles, items au sol, transitions, états interactifs ;
- **PartyRuntimeState** : personnages, inventaires, équipements, personnage sélectionné, CursorItem, charge, objets possédés.

Un objet porté par un personnage ne doit plus être considéré comme appartenant au niveau.

Un objet déposé dans un réceptacle redevient un élément du niveau courant.

---

## 17. Architecture conceptuelle cible

Le modèle final devrait prévoir les concepts suivants.

### 17.1 Item Definition

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

### 17.2 Item Instance

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

### 17.3 Character Inventory State

État de l’inventaire d’un personnage.

Contenu possible :

- CharacterId ;
- liste ou grille de slots ;
- poids porté ;
- capacité maximale ;
- état de surcharge.

### 17.4 Character Equipment State

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

### 17.5 Party State

État global du groupe.

Contenu possible :

- personnages ;
- personnage sélectionné ;
- CursorItem ;
- formation ;
- niveau courant ;
- position et orientation du groupe ;
- inventaires et équipements des personnages.

---

## 18. Interaction souris prévue

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

## 19. Sauvegarde future

Le système doit être conçu pour être sérialisable.

À terme, le SaveGame devra contenir :

- l’état du donjon ;
- l’état du groupe ;
- l’état des personnages ;
- les inventaires ;
- l’équipement ;
- les items au curseur ou règles de résolution du CursorItem ;
- la position du groupe ;
- le niveau courant.

La sauvegarde n’est pas l’objectif immédiat, mais le modèle doit être compatible avec elle.

---

## 20. Ce que ce document exclut pour l’instant

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
- les règles finales de SaveGame.

Il fixe uniquement la vision de l’inventaire, de la possession d’objets et de l’écran Inventaire.

---

## 21. Plan d’implémentation recommandé

### Phase 1 — Modèle de possession d’objets

- définir les états de possession ;
- définir l’Item Instance ;
- gérer le propriétaire actuel ;
- gérer le personnage sélectionné ;
- gérer le CursorItem.

### Phase 2 — Inventaires personnels

- ajouter un inventaire par personnage ;
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
- construire la colonne personnages ;
- construire la fiche centrale ;
- construire la zone droite défilante d’inventaires personnels ;
- ajouter drag & drop.

### Phase 5 — Intégration gameplay

- ramassage monde vers personnage sélectionné ;
- prise depuis réceptacle ;
- dépôt dans réceptacle ;
- effet des torches ;
- effets de surcharge ;
- intégration avec le Runtime Dungeon State.

### Phase 6 — Sauvegarde

- sérialiser le Party State ;
- sérialiser les inventaires ;
- sérialiser l’équipement ;
- relier avec le SaveGame du donjon.

---

## 22. Conclusion

La vision retenue est celle d’un système d’inventaire RPG complet, centré sur les personnages.

Le jeu ne repose pas sur un inventaire commun, mais sur une compagnie de 6 personnages possédant chacun leur propre équipement, leur propre inventaire et leur propre charge.

L’écran Inventaire doit permettre une gestion globale et fluide du groupe : sélectionner un personnage, consulter son équipement, voir les inventaires de tous les membres, transférer des objets, équiper, organiser, et préparer l’exploration ou le combat.

Ce document doit rester la référence de conception avant toute implémentation Codex liée à l’inventaire.
