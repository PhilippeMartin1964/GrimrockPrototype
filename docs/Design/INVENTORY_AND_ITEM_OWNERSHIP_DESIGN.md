# Inventory and Item Ownership Design

## 1. Objet du document

Ce document fixe la vision cible du système d’inventaire et de possession des objets pour le projet **GrimrockPrototype**.

L’objectif est de définir une architecture claire avant toute implémentation plus poussée, afin d’éviter une accumulation de petites étapes incohérentes et des nettoyages de code répétés.

Ce document sert de référence pour les futurs prompts Codex et pour les choix d’implémentation C++ / Unreal Engine 5.

---

## 2. Vision générale

Le jeu ne repose pas sur un inventaire de groupe principal.

Le groupe est composé de **six personnages**, et **chaque personnage possède son propre inventaire personnel**, son propre équipement, et sa propre capacité de charge.

Le système d’inventaire doit permettre :

- de visualiser rapidement l’équipement et les statistiques du personnage sélectionné ;
- de visualiser en même temps plusieurs inventaires personnels pour avoir une vue d’ensemble ;
- de transférer des objets entre personnages ;
- d’équiper un objet sur le personnage sélectionné ;
- de tenir ou manipuler un objet avec le personnage sélectionné ;
- de conserver une cohérence stricte de possession entre le monde, les réceptacles, les inventaires, l’équipement et le curseur.

---

## 3. Principes fondamentaux

### 3.1 Un objet a un seul propriétaire à la fois

Règle centrale :

> Un item ne doit appartenir qu’à un seul propriétaire à la fois.

Un objet ne peut pas exister simultanément :

- dans le monde ;
- dans un réceptacle ;
- dans l’inventaire d’un personnage ;
- sur le curseur ;
- équipé sur un personnage ;
- tenu par un personnage ;
- ou supprimé du niveau.

Cette règle doit guider tout le code futur lié aux items, afin d’éviter les duplications et les ambiguïtés de possession.

### 3.2 Le personnage sélectionné est la référence principale

Le personnage sélectionné est :

- le **récepteur par défaut** des objets ramassés ;
- le personnage qui **tient l’objet** ;
- le personnage sur lequel on **équipe** les objets par défaut ;
- le personnage dont on affiche la **fiche détaillée** ;
- le personnage utilisé comme référence principale dans l’écran d’inventaire.

### 3.3 Aucun inventaire de groupe principal

Il n’existe pas de sac commun principal.

Chaque personnage possède :

- son inventaire personnel ;
- sa charge portée ;
- sa capacité maximale de portage ;
- son équipement.

Le groupe peut avoir un état global, mais pas un inventaire commun servant de conteneur principal.

---

## 4. Interface globale

Le menu général du jeu comporte les onglets suivants :

- **Inventaire** ;
- **Compétences** ;
- **Journal** ;
- **Carte** ;
- **Recettes** ;
- **Codex**.

Ce document détaille uniquement l’onglet **Inventaire**.

---

## 5. Structure de l’écran Inventaire

L’écran d’inventaire est divisé en trois grandes zones :

1. la liste des personnages à gauche ;
2. la fiche du personnage sélectionné au centre ;
3. les inventaires personnels multiples à droite.

### 5.1 Colonne gauche : liste des six personnages

La colonne gauche contient la liste des six personnages.

Chaque entrée pourra afficher à terme :

- portrait ;
- nom ;
- classe ;
- niveau ;
- points de vie ;
- état éventuel ;
- charge portée / capacité maximale.

Fonction principale :

- cliquer sur un personnage le rend **sélectionné**.

Le personnage sélectionné devient alors le récepteur par défaut et le personnage dont la fiche centrale est affichée.

### 5.2 Zone centrale : personnage sélectionné

La zone centrale affiche le personnage sélectionné avec :

- portrait ou rendu grand format ;
- nom ;
- race ;
- classe ;
- niveau ;
- résumé des caractéristiques ;
- statistiques de combat ;
- charge portée ;
- équipement.

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

Des emplacements supplémentaires pourront être ajoutés plus tard, mais ils ne sont pas requis pour la première version.

### 5.3 Zone droite : inventaires personnels multiples

La zone droite affiche **plusieurs inventaires personnels en même temps**, avec une **barre de défilement verticale**.

Chaque bloc d’inventaire affiche :

- le nom du personnage ;
- sa charge portée ;
- sa capacité maximale ;
- une grille d’inventaire ;
- les objets contenus dans cette grille.

Objectif de cette vue multiple :

- avoir une vue d’ensemble du groupe ;
- repérer rapidement quel personnage possède quel objet ;
- transférer des objets entre personnages ;
- choisir un objet à équiper sur le personnage sélectionné ;
- gérer la logistique du groupe depuis un seul écran.

Cette décision est volontaire et fait partie de la vision finale de l’inventaire.

L’écran d’inventaire n’est donc pas seulement une fiche de personnage : c’est un **hub de gestion logistique du groupe**.

---

## 6. Philosophie de grille

L’inventaire utilise une **grille à slots simple avec cases homogènes**.

Choix retenu :

- pas d’inventaire de type Tetris ;
- pas d’objets de taille variable dans la grille ;
- un objet occupe une case ;
- certains objets peuvent être empilables si leur définition l’autorise ;
- les piles d’objets doivent rester simples et lisibles.

Avantages :

- lisibilité ;
- facilité d’usage ;
- simplicité d’implémentation ;
- compatibilité avec le drag & drop ;
- meilleure clarté dans une interface affichant plusieurs inventaires en même temps.

---

## 7. Charge, poids et force

### 7.1 Poids des objets

Chaque objet possède un poids.

Le poids total porté par un personnage dépend :

- de son inventaire personnel ;
- de son équipement ;
- de l’objet qu’il tient ou manipule si celui-ci n’est pas déjà compté ailleurs.

### 7.2 Capacité de charge

Chaque personnage possède une capacité maximale de charge.

Cette capacité dépend au minimum :

- de sa **Force**.

Des modificateurs supplémentaires pourront être ajoutés plus tard :

- traits ;
- compétences ;
- objets magiques ;
- effets temporaires ;
- race ;
- classe.

### 7.3 Dépassement de charge

Règle retenue :

> Quand un personnage dépasse sa charge maximale, il subit un **malus de déplacement** pour l’instant.

Le système ne bloque pas immédiatement toute action, mais introduit une pénalité de gameplay.

Première version recommandée :

- charge inférieure ou égale à la capacité maximale : aucun malus ;
- charge supérieure à la capacité maximale : malus de déplacement.

Plus tard, ce malus pourra être affiné par paliers.

Exemple possible :

- surcharge légère : petit malus ;
- surcharge moyenne : malus important ;
- surcharge extrême : déplacement fortement ralenti ou impossible.

---

## 8. États de possession d’un objet

Un item peut se trouver dans l’un des états suivants :

- **World** : l’objet existe dans le monde ;
- **CharacterInventory** : l’objet est dans l’inventaire personnel d’un personnage ;
- **EquipmentSlot** : l’objet est équipé sur un personnage ;
- **Cursor** : l’objet est actuellement tenu au curseur ;
- **HeldBySelectedCharacter** : l’objet est tenu ou manipulé par le personnage sélectionné ;
- **Receptacle** : l’objet est contenu dans un réceptacle ;
- **Removed** : l’objet a été retiré du monde et n’existe plus comme objet physique dans le niveau.

### 8.1 Relation entre curseur et personnage sélectionné

Le curseur représente l’objet actuellement manipulé par le joueur.

Le personnage sélectionné reste toutefois la référence logique principale :

- l’objet pris au curseur est considéré comme manipulé par le personnage sélectionné ;
- le personnage sélectionné est le récepteur par défaut ;
- l’objet peut ensuite être rangé, équipé ou transféré.

### 8.2 Règle de cohérence

À tout instant, un objet doit avoir :

- un identifiant stable ;
- un propriétaire unique ;
- un état cohérent ;
- une seule représentation active.

---

## 9. Flux fonctionnels principaux

### 9.1 Ramassage dans le monde

Lorsqu’un objet est ramassé dans le monde :

1. le joueur clique sur l’objet ;
2. l’objet quitte le monde ;
3. l’objet est attribué au personnage sélectionné ;
4. l’objet peut être placé au curseur ;
5. l’objet peut ensuite être équipé ou rangé dans l’inventaire personnel.

Exemple :

- le personnage 3 est sélectionné ;
- le joueur clique sur une épée au sol ;
- l’épée devient propriété du personnage 3 ;
- le joueur peut la mettre en main droite ou la ranger dans son inventaire.

### 9.2 Prise depuis un réceptacle

Lorsqu’un objet est retiré d’un réceptacle :

1. l’objet quitte le réceptacle ;
2. le réceptacle est mis à jour ;
3. l’objet devient propriété du personnage sélectionné ;
4. l’objet peut être tenu, équipé ou rangé.

Exemple :

- le personnage 1 est sélectionné ;
- le joueur prend une torche dans un support mural ;
- le support devient vide ;
- la torche appartient au personnage 1.

### 9.3 Équipement

Un objet peut être équipé sur le personnage sélectionné si :

- le slot est compatible ;
- l’objet est compatible avec la classe ou les règles du personnage ;
- les conditions éventuelles sont respectées.

Exemples :

- une épée peut aller en main droite ;
- un bouclier peut aller en main gauche ;
- une torche peut aller dans une main ;
- une amulette va dans le slot amulette ;
- une bague va dans un slot anneau.

### 9.4 Rangement dans l’inventaire personnel

Un objet peut être déposé dans une case libre de la grille d’inventaire personnel d’un personnage.

Si l’objet est empilable, il pourra éventuellement être fusionné avec une pile compatible.

### 9.5 Transfert entre personnages

Comme plusieurs inventaires sont visibles en même temps, il doit être possible de :

- prendre un objet dans l’inventaire du personnage A ;
- le déposer dans l’inventaire du personnage B ;
- ou l’équiper sur le personnage sélectionné.

Exemple :

- le personnage 1 est sélectionné ;
- le joueur prend une épée dans l’inventaire du personnage 4 ;
- il la dépose dans la main droite du personnage 1 ;
- l’épée quitte l’inventaire du personnage 4 et devient équipée par le personnage 1.

### 9.6 Dépôt dans un réceptacle

Un objet tenu ou présent dans un inventaire peut être placé dans un réceptacle si les règles du réceptacle le permettent.

L’objet devient alors propriété du réceptacle.

Exemple :

- le personnage sélectionné tient une torche ;
- le joueur clique sur une alcôve ;
- la torche quitte le personnage ;
- la torche devient contenue dans l’alcôve.

---

## 10. Règles d’usage du personnage sélectionné

Le personnage sélectionné est l’acteur principal de l’écran d’inventaire.

Règles retenues :

1. Le personnage sélectionné est le **récepteur par défaut**.
2. Le personnage sélectionné est celui qui **tient l’objet**.
3. Le personnage sélectionné est celui dont on affiche la fiche détaillée.
4. Le personnage sélectionné est celui dont on affiche l’équipement central.
5. Les autres personnages restent visibles pour la vue d’ensemble et les transferts.

---

## 11. Types d’objets

Le système doit à terme supporter plusieurs familles d’objets :

- armes ;
- boucliers ;
- armures ;
- casques ;
- bottes ;
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
- objets de puzzle ;
- outils ;
- recettes ;
- objets consommables.

Tous ces objets partagent une logique commune de possession, mais certains auront des comportements spécifiques.

---

## 12. Item Definition et Item Instance

Le système doit distinguer clairement deux notions.

### 12.1 Item Definition

L’Item Definition représente le modèle statique d’un objet.

Exemples :

- Item_Torch ;
- Item_IronKey ;
- Item_RubyGem ;
- Item_ShortSword ;
- Item_WoodenShield ;
- Item_HealthPotion.

Une définition d’item peut contenir :

- identifiant ;
- nom affiché ;
- description ;
- icône ;
- mesh monde ;
- mesh équipé ;
- poids ;
- type ;
- tags ;
- stackable oui/non ;
- taille de pile maximale ;
- slots compatibles ;
- comportement d’usage ;
- valeur ;
- propriétés spéciales.

### 12.2 Item Instance

L’Item Instance représente l’objet concret dans la partie.

Elle contient :

- RuntimeObjectId ;
- ItemDefinitionId ;
- OwnerType ;
- OwnerId ;
- quantité ;
- état spécifique ;
- durabilité éventuelle ;
- état allumé / éteint si applicable ;
- transform si l’objet existe dans le monde ;
- données runtime propres.

Cette séparation est indispensable pour éviter les duplications et les incohérences.

---

## 13. Torche : cas de référence

La torche est un cas de référence, car elle peut être :

- dans le monde ;
- dans un support mural ;
- dans une alcôve ;
- dans l’inventaire personnel d’un personnage ;
- tenue par le personnage sélectionné ;
- équipée dans une main ;
- allumée ;
- éteinte.

La torche permet de tester tout le modèle :

- possession unique ;
- transfert depuis un réceptacle ;
- équipement ;
- rangement ;
- dépôt ;
- persistance runtime.

---

## 14. UI et interactions

### 14.1 Interactions de base

L’interface devra supporter à terme :

- clic gauche ;
- glisser-déposer ;
- survol ;
- clic droit contextuel ;
- double-clic si utile.

### 14.2 Actions attendues

L’utilisateur doit pouvoir :

- prendre un objet dans un inventaire ;
- déposer un objet dans un autre inventaire ;
- équiper un objet ;
- retirer un objet équipé ;
- déposer un objet dans un réceptacle ;
- prendre un objet depuis le monde ;
- examiner un objet ;
- utiliser un objet ;
- transférer un objet entre personnages.

### 14.3 Vue d’ensemble

Le choix d’afficher plusieurs inventaires personnels à droite est une décision structurante.

L’écran d’inventaire doit être compris comme :

> un écran de gestion logistique du groupe, centré sur un personnage sélectionné mais offrant une vision globale des ressources portées par les six personnages.

---

## 15. Relation avec le Runtime Dungeon State

Le Runtime Dungeon State gère l’état vivant des niveaux.

Le système d’inventaire devra être complémentaire :

- les objets présents dans les niveaux relèvent du Runtime Dungeon State ;
- les objets portés par les personnages relèvent de l’état du groupe et des personnages ;
- les objets dans les réceptacles relèvent du monde tant qu’ils y sont déposés ;
- les objets équipés ou portés ne doivent plus être capturés comme objets du niveau.

À terme, une sauvegarde complète devra inclure :

- l’état runtime du donjon ;
- l’état runtime du groupe ;
- l’état des personnages ;
- les inventaires personnels ;
- les équipements ;
- l’objet au curseur si nécessaire.

---

## 16. Architecture conceptuelle recommandée

Le modèle cible suggère les éléments suivants.

### 16.1 État du groupe

Un état de groupe contient :

- liste des personnages ;
- personnage sélectionné ;
- objet au curseur ;
- formation ;
- informations globales du groupe.

### 16.2 État d’un personnage

Chaque personnage contient :

- identifiant stable ;
- nom ;
- race ;
- classe ;
- niveau ;
- caractéristiques ;
- statistiques dérivées ;
- inventaire personnel ;
- équipement ;
- charge portée ;
- capacité maximale.

### 16.3 État d’inventaire personnel

Chaque inventaire personnel contient :

- grille de slots ;
- items stockés ;
- poids total ;
- capacité maximale associée au personnage.

### 16.4 État d’équipement

Chaque équipement contient :

- main droite ;
- main gauche ;
- tête ;
- torse ;
- jambes ;
- pieds ;
- amulette ;
- anneau 1 ;
- anneau 2 ;
- éventuels slots supplémentaires.

### 16.5 État d’item

Chaque item runtime contient :

- RuntimeObjectId ;
- ItemDefinitionId ;
- OwnerType ;
- OwnerId ;
- quantité ;
- données spécifiques.

---

## 17. Ce que ce document exclut pour l’instant

Ce document ne fixe pas encore dans le détail :

- les compétences ;
- les arbres de talents ;
- les règles complètes de classes ;
- les règles complètes d’équipement ;
- la progression détaillée ;
- les recettes ;
- la carte ;
- le journal ;
- le codex ;
- le système de combat complet ;
- l’interface graphique finale pixel-perfect ;
- le SaveGame définitif.

Il fixe uniquement la **vision de l’inventaire et de la possession des objets**.

---

## 18. Décisions validées

Les décisions suivantes sont validées :

- le groupe comporte six personnages ;
- il n’y a pas d’inventaire de groupe principal ;
- chaque personnage possède son propre inventaire ;
- le personnage sélectionné est le récepteur par défaut ;
- le personnage sélectionné est celui qui tient l’objet ;
- la charge maximale dépend de la Force ;
- le dépassement de charge inflige un malus de déplacement pour l’instant ;
- l’inventaire utilise une grille simple à cases homogènes ;
- plusieurs inventaires personnels sont visibles en même temps à droite ;
- la zone centrale affiche la fiche et l’équipement du personnage sélectionné ;
- le système doit permettre les transferts, l’équipement, le rangement et la manipulation d’objets depuis cette vue globale.

---

## 19. Étapes d’implémentation recommandées

### Phase 1 — Fondations de possession d’objets

Objectif : poser le modèle sans UI finale.

À faire :

- définir les états de possession ;
- garantir une identité stable des items ;
- créer un modèle d’instance d’item ;
- relier les items au personnage sélectionné ;
- clarifier le curseur / objet tenu.

### Phase 2 — Personnages et inventaires personnels

Objectif : créer l’état d’inventaire par personnage.

À faire :

- créer six personnages runtime ;
- leur donner un inventaire personnel ;
- gérer les grilles simples ;
- calculer charge portée et capacité maximale.

### Phase 3 — Équipement

Objectif : équiper et retirer les objets.

À faire :

- créer les slots d’équipement ;
- vérifier les compatibilités ;
- gérer les transferts inventaire vers équipement ;
- gérer les retours équipement vers inventaire.

### Phase 4 — Interface d’inventaire

Objectif : réaliser l’écran d’inventaire.

À faire :

- colonne gauche des personnages ;
- panneau central du personnage sélectionné ;
- panneau droit des inventaires personnels multiples ;
- barre de défilement ;
- drag & drop ;
- tooltips.

### Phase 5 — Intégration gameplay

Objectif : relier l’inventaire au gameplay réel.

À faire :

- pickup depuis le monde ;
- pickup depuis réceptacle ;
- dépôt dans réceptacle ;
- équipement de torche ;
- effets d’objets ;
- malus de déplacement en surcharge.

### Phase 6 — Persistance

Objectif : intégrer l’inventaire au futur SaveGame.

À faire :

- sérialiser le groupe ;
- sérialiser les personnages ;
- sérialiser les inventaires ;
- sérialiser les équipements ;
- sérialiser l’objet au curseur si nécessaire.

---

## 20. Conclusion

La vision retenue est celle d’un système :

- orienté personnages ;
- sans inventaire de groupe principal ;
- avec six inventaires personnels ;
- avec une vue d’ensemble scrollable ;
- avec une logique forte de poids, de force et de charge ;
- avec un personnage sélectionné servant de référence principale ;
- avec une règle stricte de possession unique des objets.

Ce document doit servir de base aux prochaines tâches d’implémentation, afin que le système d’inventaire soit construit de manière cohérente, robuste et évolutive.
