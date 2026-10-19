# UI-EVENTHUD01 — Flux d’événements runtime

Date : **17 octobre 2026**  
Statut : **CONTRAT ACCEPTÉ — À IMPLÉMENTER**

## 1. Objectif

Le **Flux d’événements** est un composant HUD permanent affiché en bas à droite de l’écran, immédiatement **au-dessus de la Hotbar et de la barre de navigation persistante**.

Il constitue le canal central des informations contextuelles non bloquantes du runtime et répond à la question :

> **Que vient-il de se passer ?**

Il est strictement distinct du Journal :

- le Flux est chronologique, temporaire, filtrable et orienté feedback immédiat ;
- le Journal est persistant, narratif et destiné aux quêtes, dialogues importants et informations à retenir.

Le Flux ne doit pas devenir un journal de debug.

## 2. Catégories et filtres

Le Flux possède exactement trois catégories principales :

- **Événements** ;
- **Combat** ;
- **Discussion**.

Les trois filtres sont activés par défaut et sont indépendants.

Désactiver une catégorie :

- masque les entrées correspondantes ;
- ne supprime aucune donnée ;
- n’empêche pas la collecte de nouveaux événements ;
- conserve l’ordre chronologique.

Les trois boutons sont placés **en bas du Flux** et restent visibles pendant le défilement :

```text
┌───────────────────────────────┐
│ événements anciens            │
│                               │
│ combat / détails              │
│                               │
│ discussion                    │
│ événement récent              │
├───────────────────────────────┤
│ Événements | Combat | Discussion
└───────────────────────────────┘
████████ HOTBAR / NAVIGATION ████████
```

## 3. Positionnement et hauteur

Le composant est ancré :

- à droite ;
- au bas de la zone de jeu disponible ;
- immédiatement au-dessus de la Hotbar.

Il ne doit jamais recouvrir la Hotbar.

La hauteur du Flux est réglable par le joueur à l’aide d’une poignée ou d’une zone de redimensionnement située sur son bord supérieur.

Le composant possède :

- une hauteur minimale ;
- une hauteur maximale ;
- une hauteur courante.

Le redimensionnement conserve l’ancrage inférieur.

La largeur reste fixe dans la première version.

La hauteur choisie peut être persistée comme préférence utilisateur ; elle ne fait pas partie du `SaveGame`.

## 4. Buffer et historique

Le Flux conserve un historique borné.

Valeur initiale :

**200 événements logiques maximum.**

Lorsqu’un 201e événement est ajouté :

- l’événement logique le plus ancien est supprimé automatiquement ;
- les 200 plus récents sont conservés.

Un événement de combat détaillé compte comme **une seule entrée logique**, même s’il contient plusieurs lignes.

Aucune suppression individuelle n’est prévue :

- pas de bouton poubelle ;
- pas de menu contextuel de suppression ;
- pas de commande « supprimer cet événement ».

## 5. Événements de donjon

La catégorie **Événements** reçoit les informations contextuelles perceptibles du donjon, notamment :

- serrure verrouillée ;
- tentative d’ouverture impossible ;
- porte ouverte ou fermée ;
- levier actionné ;
- bouton activé ;
- plaque de pression ;
- mécanisme ;
- piège ;
- réceptacle ;
- téléporteur ;
- passage secret découvert ;
- objet obtenu ;
- inscription ;
- panneau d’information ;
- message environnemental.

Exemples :

```text
La serrure est verrouillée.

Cette porte ne peut être ouverte depuis ce côté.

Le levier s’enclenche.

Un passage secret vient de s’ouvrir.
```

## 6. Serrures

Une interaction volontaire avec une serrure verrouillée doit produire une entrée dans le Flux.

Exemples possibles :

```text
La serrure est verrouillée.
Cette serrure nécessite une clé.
La clé utilisée ne correspond pas à cette serrure.
```

Le Flux ne doit jamais révéler une information interne inconnue du joueur, par exemple l’identifiant technique d’une clé ou d’un trigger.

## 7. Panneaux, inscriptions et migration des popups

Les messages des panneaux d’information et inscriptions doivent apparaître dans le Flux.

Exemple :

```text
Inscription

« Seul celui qui porte la marque
pourra franchir le seuil. »
```

Les popups contextuels non bloquants concernés doivent être supprimés au profit du Flux.

Le Flux devient le canal standard pour :

- informations ;
- inscriptions ;
- refus d’interaction ;
- serrures ;
- mécanismes ;
- messages environnementaux.

Les véritables interfaces modales ou fenêtres de confirmation restent hors contrat.

## 8. Visibilité et observabilité

Le Flux ne doit afficher que ce que le joueur peut légitimement connaître.

Une notion de visibilité doit être prévue, conceptuellement :

```cpp
enum class EGridEventVisibility : uint8
{
    Hidden,
    Observable,
    Always
};
```

### Hidden

Événement logique interne, non visible.

Exemple : un trigger secret s’active hors de toute perception du joueur.

### Observable

L’événement est affichable lorsque son effet est perceptible par le joueur.

### Always

Message explicitement destiné au joueur, par exemple un panneau d’information.

Cette règle doit empêcher les spoilers liés aux triggers invisibles, scripts internes, portes secrètes non découvertes ou mécanismes distants.

## 9. Combat : principe général

La catégorie **Combat** doit conserver un compte rendu détaillé du calcul réel, mais le Flux permanent doit rester synthétique.

La présentation normale d'une attaque tient à un en-tête puis au maximum deux lignes de résolution :

```text
Daniel attaque Rat géant — COUP CRITIQUE
Attaque : 1d20 (20) = 20; Défense : 10 + 1 = 11
Dégâts : 1d3 (2) × 200 % = 4; PV Rat géant : 8 → 4
```

Le détail exhaustif du snapshot est disponible dans une **info-bulle au survol** de l'entrée Combat. Ainsi le joueur peut comprendre le calcul sans que chaque attaque occupe la majorité du Flux.

Cette info-bulle réutilise **la même classe authored `WBP_GridNavigationTooltip` que `WBP_GridPersistentHud`**. Son fond, son brush, son padding et sa typographie restent donc pilotés par un seul asset UMG partagé.

Le HUD ne calcule aucune règle de combat : le système de combat produit un snapshot résolu et structuré ; le Flux ne fait que projeter et formater ces valeurs.

## 10. Jets d’attaque et de défense

La résolution doit présenter explicitement les jets de dés.

Le jet d'attaque actuel est affiché comme un vrai `1d20`, avec son résultat naturel entre parenthèses puis les modificateurs réellement appliqués.

Exemple synthétique :

```text
Attaque : 1d20 (14) + 3 + 2 = 19; Défense : 10 + 4 = 14
```

L'info-bulle conserve les libellés détaillés des composantes (`Précision`, `Bonus d'attaque`, `Esquive`, seuil critique, etc.).

## 11. Calcul détaillé des dégâts

Les dégâts doivent également être entièrement traçables et présentés comme une résolution de dés.

Le resolver actuel utilise une plage uniforme inclusive `MinDamage..MaxDamage`. Cette plage est mathématiquement équivalente à :

```text
1d(MaxDamage - MinDamage + 1) + (MinDamage - 1)
```

La présentation du Flux utilise donc cette notation de **dé virtuel**, même lorsque le nombre de faces n'existe pas comme dé physique :

```text
1..3  → 1d3
2..8  → 1d7 + 1
3..10 → 1d8 + 2
```

Exemple de résolution :

```text
Dégâts : (1d8 (6) + 2) × 200 % = 16; PV Rat géant : 20 → 4
```

Le décalage `+(MinDamage-1)` exprime seulement la normalisation exacte de la plage actuelle. Il ne doit pas être présenté comme provenant d'une recette, d'une pierre à affûter, d'un enchantement ou d'une caractéristique tant que cette provenance n'est pas explicitement fournie par le système de combat.

L'info-bulle conserve la décomposition complète : jet, bonus et malus, coefficients, critique, multiplicateurs, armure, résistance, dégâts bruts, dégâts finaux et transition de PV.

Pour plusieurs dés futurs, le modèle conserve les résultats individuels :

```text
2d6 (4 + 6) = 10
```
Le modèle doit pouvoir représenter, lorsque les règles de combat l’exigent :

- dés lancés ;
- résultats individuels ;
- bonus et malus ;
- statistiques utilisées ;
- seuils ;
- attaque totale ;
- défense totale ;
- critiques ;
- armure ;
- résistances ;
- vulnérabilités ;
- dégâts bruts ;
- réductions ;
- dégâts finaux ;
- effets secondaires.

## 12. Discussion

La catégorie **Discussion** contient les paroles réellement échangées.

Exemple :

```text
Gardien
« Aucun mortel n’est autorisé à franchir cette porte. »

Elias
« Nous ne cherchons pas le combat. »
```

Les choix proposés au joueur ne sont pas enregistrés avant leur sélection.

Une réponse effectivement prononcée par le groupe peut être ajoutée au Flux.

Le Journal décidera séparément quels dialogues méritent une conservation narrative permanente.

## 13. Code couleur sémantique

Le code couleur est conservé mais volontairement simple.

Il concerne uniquement des éléments natifs UMG tels que :

- texte ;
- mots ou fragments de texte ;
- titres ;
- nombres ;
- séparateurs ;
- `Border` ;
- fonds simples.

**Aucune icône sémantique n’est requise dans UI-EVENTHUD01.**

Exemples de familles :

| Nature | Traitement envisagé |
|---|---|
| Information | neutre |
| Interaction impossible | ambre |
| Serrure / verrou | bronze |
| Mécanisme | cuivre |
| Secret découvert | doré |
| Danger / piège | rouge |
| Dégâts physiques | rouge / cuivre |
| Feu | orange / rouge |
| Glace | bleu clair |
| Foudre | bleu / violet |
| Poison | vert |
| Soin | vert clair |
| Magie arcanique | violet |
| Buff | positif |
| Debuff | négatif |
| Discussion | bleu / gris / ivoire |

La palette exacte sera définie lors de l’intégration UMG.

La couleur ne doit jamais être l’unique porteur d’information.

## 14. Fragments de texte sémantiques

Le modèle doit anticiper la possibilité de colorer seulement certains fragments d’une entrée.

Exemple :

```text
Elias inflige 11 dégâts de FEU au Squelette.
```

Le nombre `11` et le mot `FEU` peuvent recevoir des traitements différents.

Autre exemple :

```text
La serrure nécessite une CLÉ DE CUIVRE.
```

Le terme `CLÉ DE CUIVRE` peut devenir un fragment sémantique distinct.

Le contrat ne doit donc pas enfermer toutes les entrées dans un `FText` monolithique impossible à enrichir.

## 15. Futur système de tooltips sémantiques

Le système doit anticiper, sans l’implémenter dans la V1, la possibilité d’afficher une info-bulle lorsqu’un mot coloré est survolé.

Exemple :

```text
Résistance au FEU : -25 %
              ↑
```

Au survol de `FEU`, une info-bulle pourra expliquer le type de dégâts et son rôle.

Ce principe doit être conçu ultérieurement comme un mécanisme UI générique réutilisable dans :

- Flux d’événements ;
- Character Sheet ;
- Inventory ;
- Skills ;
- Combat ;
- tooltips d’objets ;
- Journal ;
- Codex ;
- Crafting ;
- sorts.

Cette extension fera l’objet d’un jalon séparé, de type **UI-SEMANTIC-TEXT01**.

## 16. Classification de données

Le Flux doit distinguer la catégorie de filtrage de la nature sémantique.

Conceptuellement :

```cpp
enum class EGridHudEventCategory : uint8
{
    Dungeon,
    Combat,
    Dialogue
};
```

et :

```cpp
enum class EGridHudEventSemantic : uint8
{
    Default,
    Information,
    Interaction,
    Locked,
    Mechanism,
    Secret,
    Danger,
    PhysicalDamage,
    Fire,
    Ice,
    Lightning,
    Poison,
    Arcane,
    Healing,
    Buff,
    Debuff,
    Dialogue
};
```

La liste sémantique exacte pourra évoluer.

Une entrée doit conserver au minimum les informations conceptuelles suivantes :

```cpp
EGridHudEventCategory Category;
EGridHudEventSemantic Semantic;
EGridEventVisibility Visibility;
FName EventCode;
FText Header;
```

ainsi que les données structurées nécessaires à sa présentation.

## 17. Données de combat structurées

Le combat doit exposer une structure de résultat détaillée plutôt qu’une chaîne finale opaque.

Conceptuellement :

```text
FGridCombatEvent
├── Attacker
├── Defender
├── Result
├── AttackRoll
│   ├── DiceExpression
│   ├── IndividualRolls
│   ├── Modifiers
│   └── FinalValue
├── Defense
│   ├── Base
│   ├── Modifiers
│   └── FinalValue
└── Damage
    ├── DiceExpression
    ├── IndividualRolls
    ├── Modifiers
    ├── RawDamage
    ├── Mitigations
    └── FinalDamage
```

Le nom exact des structures sera fixé lors de l’implémentation.

## 18. Service central

Le service central proposé est :

`UGridEventFeedSubsystem`

de type :

`UWorldSubsystem`

Responsabilités :

- recevoir les événements ;
- attribuer leur ordre chronologique ;
- appliquer la politique générale de visibilité ;
- conserver le buffer ;
- appliquer la limite de 200 événements ;
- notifier la présentation.

Il ne doit pas gérer :

- le calcul du combat ;
- le fonctionnement des portes ou serrures ;
- les règles des sorts ;
- la logique des dialogues ;
- les règles métier des producteurs.

Aucun acteur runtime ne doit référencer directement le widget.

## 19. Architecture cible

```text
Porte / Serrure
Levier / Bouton
Plaque / Piège
Panneau / Inscription
Téléporteur
Sort
Combat
Dialogue
Script
        │
        │ PublishEvent()
        ▼
┌─────────────────────────────┐
│ UGridEventFeedSubsystem     │
│                             │
│ ordre chronologique         │
│ visibilité                  │
│ buffer maximum : 200        │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ WBP_GridEventFeed           │
│                             │
│ affichage                   │
│ couleurs                    │
│ scroll                      │
│ hauteur réglable            │
│                             │
│ ─────────────────────────── │
│ Événements Combat Discussion│
└─────────────────────────────┘
             ↑
       au-dessus de
        la Hotbar
```

## 20. Scroll

Les événements restent dans l’ordre chronologique :

- anciens en haut ;
- récents en bas.

À chaque nouvelle entrée publiée dans une catégorie visible, le Flux revient automatiquement en bas pour afficher immédiatement l’événement le plus récent, y compris si le joueur avait remonté l’historique.

Le défilement à la roulette reste libre entre deux nouveaux événements. Le changement de filtre conserve autant que possible la position de consultation courante ; une entrée dans une catégorie masquée ne déclenche pas de retour automatique en bas.

## 21. Interaction souris

Le Flux ne crée aucun nouveau mode souris.

Le contenu purement visuel ne doit pas intercepter les clics destinés au monde.

Les éléments interactifs de la V1 sont uniquement :

- les trois filtres ;
- la poignée de hauteur ;
- le scroll.

Les futurs fragments de texte avec tooltip sémantique seront intégrés au système d’interaction UI commun lorsqu’ils seront implémentés.

## 22. Persistance

Le contenu du Flux n’est pas enregistré dans le `SaveGame`.

Peuvent être conservés comme préférences utilisateur :

- hauteur du panneau ;
- état des filtres ;
- éventuellement, à terme, la limite du buffer si elle devient réglable par le joueur.

## 23. Hors contrat V1

Ne font pas partie de UI-EVENTHUD01 V1 :

- suppression individuelle d’événements ;
- icônes sémantiques ;
- Journal ;
- quêtes ;
- Codex ;
- recherche textuelle ;
- bookmarks ;
- archivage permanent ;
- calcul métier dans UMG ;
- système complet de tooltip sur mots colorés.

Le support futur des fragments sémantiques doit néanmoins être anticipé dans le modèle de données.

## 24. Critères d’acceptation

UI-EVENTHUD01 sera considéré valide lorsque :

1. le Flux est affiché en bas à droite ;
2. il est entièrement situé au-dessus de la Hotbar ;
3. les boutons Événements / Combat / Discussion sont placés en bas du Flux ;
4. les trois filtres sont indépendants ;
5. masquer une catégorie ne détruit aucune entrée ;
6. la hauteur est réglable ;
7. le buffer est limité initialement à 200 événements ;
8. aucune suppression individuelle n’est proposée ;
9. les événements les plus anciens sont éliminés automatiquement au dépassement ;
10. cliquer sur une serrure verrouillée produit un message ;
11. les panneaux et inscriptions publient dans le Flux ;
12. les anciens popups contextuels concernés sont supprimés ;
13. les événements secrets internes non perceptibles restent invisibles ;
14. les dialogues PNJ sont intégrés ;
15. les réponses effectivement prononcées par le groupe sont intégrées ;
16. le combat expose les vrais jets de dés ;
17. le calcul attaque / défense / dégâts est entièrement traçable ;
18. UMG ne recalcule aucun résultat métier ;
19. la couleur s’applique uniquement à du texte et à des widgets UMG simples ;
20. aucune icône sémantique n’est requise ;
21. le modèle permet de distinguer de futurs fragments de texte sémantiques ;
22. l’architecture permet l’ajout ultérieur de tooltips sur ces fragments ;
23. la roulette permet de consulter l’historique, et toute nouvelle entrée visible replace le Flux en bas ;
24. le Flux n’interfère pas avec les commandes du jeu ;
25. aucun acteur métier ne dépend directement de `WBP_GridEventFeed`;
26. aucune entrée du Flux n’est automatiquement copiée dans le Journal.
27. une action et ses conséquences synchrones perceptibles occupent une seule entrée logique ;
28. le séparateur `→` relie la cause à sa première conséquence et `·` sépare les conséquences supplémentaires ;
29. une conséquence non visible mais audible utilise une formulation sensorielle sans révéler son identité cachée ;
30. une conséquence ni visible ni audible n'est pas publiée dans le Flux.

## 25. Roadmap d’implémentation

| Jalon | Contenu | Résultat attendu |
|---|---|---|
| **EVENTHUD01.1 — Core Event Feed** | Catégories, sémantique, visibilité, structure d’entrée, `UGridEventFeedSubsystem`, buffer FIFO 200 | Le runtime peut publier et conserver des événements sans UI |
| **EVENTHUD01.2 — HUD Shell** | `WBP_GridEventFeed`, ancrage bas-droite, placement au-dessus Hotbar, scroll, hauteur min/max | Panneau fonctionnel vide |
| **EVENTHUD01.3 — Filters & Resize** | Trois boutons en bas, filtrage indépendant, poignée de redimensionnement, autoscroll | Interaction UI de base complète |
| **EVENTHUD01.4 — Dungeon Messages** | Portes, serrures, leviers, boutons, plaques, réceptacles, téléporteurs, pièges | Premiers vrais événements runtime |
| **EVENTHUD01.5 — Popup Migration** | Panneaux, inscriptions, erreurs d’interaction et suppression des anciens popups concernés | Un seul canal de feedback contextuel |
| **EVENTHUD01.6 — Semantic Colors** | Règles de couleurs, texte et `Border` uniquement, sans icônes | Sémantique visuelle simple et cohérente |
| **EVENTHUD01.7 — Combat Trace Model** | Données structurées : jets, dés individuels, modificateurs, attaque, défense, dégâts, mitigation | Le moteur produit une trace complète |
| **EVENTHUD01.8 — Combat Rendering** | Affichage des vrais jets et du calcul complet | Combat totalement explicable dans le Flux |
| **EVENTHUD01.9 — Dialogue Feed** | Paroles PNJ et réponses réellement prononcées | Troisième catégorie alimentée |
| **EVENTHUD01.10 — Validation** | Buffer 200, filtres, resize, souris, secrets, scroll, changement de niveau, stress test | Jalon maître validé |
| **EVENTHUD01.11 — Gameplay Timestamp** | Horloge de partie persistante et préfixe bicolore | Chronologie lisible dans les trois catégories |
| **EVENTHUD01.11P — Gameplay/World Time Polish** | Format `[J X, HH:MM:SS]`, secondes et `WorldTimeScale = 6.0` | Horloge monde compacte et accélérée |
| **EVENTHUD01.12 — Event Consequence Grouping** | Cause + conséquences immédiates, perception spatiale et anti-spoiler | Une chaîne logique = une entrée player-facing |

## 26. Extension ultérieure

Le système de tooltips sur mots colorés sera traité séparément, sous un jalon dédié de type :

**UI-SEMANTIC-TEXT01 — Semantic Rich Text & Shared Tooltips**

Objectif :

```text
mot coloré → donnée sémantique → survol → tooltip explicatif
```

Ce mécanisme devra être commun aux différentes interfaces du jeu et ne pas être spécifique au seul Flux d’événements.

## 27. Décision

Le présent contrat a été **accepté le 17 octobre 2026**.

Il constitue la référence fonctionnelle pour l’implémentation de **UI-EVENTHUD01**.

Les améliorations éventuelles seront décidées après utilisation en jeu, sans modifier rétroactivement les objectifs validés de la V1.


## 28. EVENTHUD01.11 — Gameplay Timestamp

Chaque événement accepté capture un timestamp immuable de **temps monde** dérivé du gameplay effectif :

```text
[J 0, 02:34:00] Daniel attaque Rat géant — TOUCHÉ
```

Le timestamp :

- utilise l'horloge gameplay persistante du `UGrimrockGameInstance` ;
- ne dépend pas de `UWorld::GetTimeSeconds()` comme valeur absolue par niveau ;
- progresse à partir du temps de monde actif, donc respecte la pause ;
- applique un `WorldTimeScale` authored de **6.0** par défaut, sans modifier la `GlobalTimeDilation` du gameplay ;
- ne compte pas le menu principal ni les chargements ;
- est remis à zéro pour New Game ;
- est restauré après Load Game ;
- est sauvegardé dans `UGrimrockPartySaveGame::GameplayElapsedSeconds` ;
- ouvre le schéma exact-match **v27** ;
- ne rend jamais l'historique du Flux persistant.

La présentation est bicolore : `TimestampColor` est indépendant de la couleur sémantique du message et reste éditable dans `WBP_GridEventFeed`.


## 29. EVENTHUD01.11P — Gameplay/World Time Polish

Le format final du Flux est :

```text
[J 0, 00:02:36] Daniel attaque Rat géant — TOUCHÉ
```

L'horloge logique applique par défaut :

```text
WorldTimeScale = 6.0
10 s gameplay = 60 s monde
10 min gameplay = 1 h monde
4 h gameplay = 1 jour monde
```

Le facteur est éditable dans les Class Defaults du `UGrimrockGameInstance`.
Il n'utilise pas `GlobalTimeDilation` et n'accélère donc ni animations, ni
portes, ni déplacements, ni combat. Le champ SaveGame v27 existant conserve
les **secondes monde accumulées** ; aucun bump de schéma supplémentaire n'est
nécessaire.


## 30. EVENTHUD01.12 — Event Consequence Grouping

### 30.1 Une chaîne logique, une entrée

Une action player-facing et ses conséquences **synchrones et immédiates** sont
publiées comme une seule `FGridHudEventEntry`, avec un seul `SequenceId` et
un seul timestamp :

```text
[J 0, 00:02:36] Le levier est activé → Une porte s'ouvre au fond du couloir.
```

Plusieurs conséquences perceptibles utilisent le séparateur `·` :

```text
[J 0, 00:02:36] Le levier est activé → Une porte s'ouvre · Un téléporteur s'active.
```

Le widget peut naturellement replier le texte si sa largeur est insuffisante ;
ce wrapping visuel ne crée jamais une deuxième entrée logique.

Une conséquence réellement **asynchrone** — animation différée, timer, nouvelle
condition, nouvel événement runtime — reste une nouvelle entrée avec son propre
timestamp.

### 30.2 Perception spatiale

Le Flux ne décrit pas la connaissance omnisciente du moteur. Une conséquence
cell-bound est projetée selon la perception du groupe :

1. **Visible proche** : formulation spatiale proche, par exemple
   `Une porte devant vous s'ouvre.`
2. **Visible lointaine** : formulation non identifiante, par exemple
   `Une porte s'ouvre au fond du couloir.`
3. **Non visible mais audible** :
   `Vous entendez une porte s'ouvrir au loin.`
4. **Ni visible ni audible** : aucune conséquence player-facing.

La vue réutilise la géométrie canonique de grille
`FGridMonsterPerception::HasDirectionalLineOfSight()` et
`AGridLevelRuntimeActor::CanMove()`. L'ouïe réutilise
`FGridMonsterPerception::CanHearThroughGrid()` et
`AGridLevelRuntimeActor::CanSoundTraverse()`.

La portée initiale EVENTHUD01.12 est volontairement conservative :
**6 cellules pour la vue et 6 cellules pour l'ouïe**. Ces valeurs sont des
paramètres de projection et pourront être déplacées vers une politique
data-driven si le tuning du donjon l'exige.

### 30.3 Identité et anti-spoiler

Le déterminant défini (`la porte`, `le mécanisme`) est réservé aux situations
où l'identité est réellement établie par le contexte player-facing. Une
conséquence distante emploie par défaut un déterminant indéfini.

Une porte secrète non identifiée ne doit jamais produire :

```text
Une porte secrète s'ouvre.
```

Si son mouvement est visible, le Flux décrit seulement ce qui est perçu :

```text
Une partie du mur se met en mouvement.
```

Si elle est seulement audible :

```text
Vous entendez un mécanisme au loin.
```

Si elle n'est ni visible ni audible, elle reste silencieuse dans le Flux.

### 30.4 Architecture

Le regroupement appartient à `UGridEventFeedSubsystem`, pas au widget.
Les producteurs ouvrent un groupe au début de l'action racine, exécutent leurs
liens synchrones, puis ferment le groupe. Les publications Dungeon survenues
pendant cette fenêtre deviennent des conséquences de l'entrée racine.

Le widget continue donc à recevoir une `FGridHudEventEntry` normale et ne
reconstruit aucune causalité a posteriori.
