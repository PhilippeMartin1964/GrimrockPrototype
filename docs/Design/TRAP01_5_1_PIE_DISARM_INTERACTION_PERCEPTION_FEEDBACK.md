# TRAP01.5.1 — Correctif PIE : cliquer pour désamorcer et comprendre la Perception

31 octobre 2026 — C++ et tests publiés ; **validation UE5.5.4 locale en attente**.

## Diagnostic du premier essai PIE

Résultats déjà confirmés par l'utilisateur : un piège au sol peut être détecté,
son mesh révélé apparaît, les dégâts fonctionnent et un piège à usage unique
ne réattaque pas en traversant une nouvelle fois sa case.

Problèmes observés :
1. Le mesh révélé utilise par défaut `NoCollision` ; le clic monde ne
   peut pas le sélectionner via `ECC_Visibility`.
2. `AGridGenericObjectActor::CanInteract` ne prend historiquement en compte
   que les objets lisibles ; un piège non lisible reste inerte au clic.
3. La seule voie de désamorçage était un **clic droit sur le kit dans le sac**
   puis « Désamorcer le piège », près d'un piège **détecté et armé**.
4. Pour un piège `bOneShot=true` déjà déclenché, la sélection de
   désamorçage est refusée car la menace est consommée. L'interface
   n'expliquait pas cet état au joueur.
5. La détection passive est un **jet d20** basé sur la Perception
   (rang + Sagesse + progression), la distance et les conditions
   de lumière. Les jets sont mémorisés par GUID et distance. Le jeu
   annonçait une réussite, sans détailler pourquoi un autre déplacement
   ne produisait aucune découverte.

## Comportement désormais prévu

### Piège détecté et encore armé

- Son **acteur visuel générique** devient détectable par le rayon souris
  `ECC_Visibility` en **QueryOnly** et ne bloque jamais le mouvement.
  Aucun collider n'est créé pour un piège encore caché.
- Son `GetInteractionCursor` annonce `Use` ; son libellé est
  « Examiner le mécanisme ».
- Un clic gauche sur la géométrie révélée appelle
  `AGridLevelRuntimeActor::TryInteractWithRevealedDungeonTrap`.
  Ce traitement conserve le contrôle serveur C++ du GUID, la portée,
  l'orientation, l'état, le combat et les outils.
- **Le kit reste dans l'inventaire**. Le runtime sélectionne un membre
  vivant porteur du kit `Item_DisarmTools_01`, puis l'existant
  `TryDisarmTriggerTrap` choisit le meilleur personnage formé,
  sous Dextérité, et résout un seul d20.
- L'action historique du **clic droit sur les outils** est conservée :
  les deux chemins utilisent la même résolution de désamorçage
  et la même garde de sauvegarde/XP.

### Piège déjà déclenché, désarmé ou désactivé

Cliquer sur le mesh fait apparaître un message de statut précis.
Pour un piège `bOneShot` déjà déclenché :

« Ce piège s'est déjà déclenché : il est inactif et ne nécessite aucun désamorçage. »

Un piège déjà désarmé affiche « Ce piège est déjà désamorcé. » ;
un mécanisme désactivé, « Ce mécanisme est déjà désactivé. »

Aucune nouvelle blessure, aucun jet de dé et aucun gain d'XP à ce stade.

### Prérequis manquants

Le flux explique désormais « Un personnage vivant doit avoir appris
la compétence Pièges et posséder ses outils » ou « Aucun membre du groupe
ne possède une trousse de désamorçage utilisable ».

Pour une cible trop éloignée (ou masquée par une paroi), il invite
à s'approcher et à faire face au mécanisme.

## Retour de Perception, sans dévoiler les pièges cachés

Une réussite réelle annonce désormais la localisation ET le calcul :

« Elias repère un piège au sol, à 2 cases, devant vous
(Perception : d20 12, total 17, difficulté 14). »

Un **échec isolé** ne doit pas publier « jet raté contre ce piège » :
cela trahirait l'existence d'un piège invisible.

**Contrat actuel (TRAP-INSTANCE01) :** tous les scans passifs sans nouvelle
découverte sont silencieux, qu'un piège soit absent, hors de portée, masqué
par un mur ou simplement non repéré. Le journal ne confirme jamais qu'une
zone est sûre. Une **réussite réelle** conserve son annonce explicative.
Le réglage historique `bShowPassivePerceptionFeedback` et sa phrase neutre
« aucun danger repéré » ont été supprimés, à la demande de l'utilisateur.
Une éventuelle commande manuelle « Examiner les alentours » est un
jalon distinct, non implémenté ici.

## Tests automatiques / conservation des invariants

La suite `Grimrock.TRAP01.2` vérifie désormais aussi :
- **absence de notification** après un scan bloqué, sans fuite d'information ;
- conservation du message positif lorsqu'un piège est réellement découvert ;
- acteur générique révélé interactif, collision QueryOnly ;
- `Visibility=Block`, curseur `Use`, réponse au clic ;
- piège à usage unique déjà déclenché : clic informatif.

La suite `Grimrock.TRAP01.4` vérifie :
- clic direct sur un second piège détecté et encore armé ;
- désamorçage et XP via les services C++ existants ;
- répétition du clic sans nouveau gain d'XP.

Régression complémentaire : `Grimrock.RPG.ARMEMENT01.16.Trap`,
`Grimrock.TRAP01.3`, `Grimrock.UI.EventHUD01.XP01`.

Exécution locale (compilation incluse) :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TRAP01.2"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.TRAP01.4"
```

### Recette PIE

1. Placer un **nouveau** piège à pointes à 1 case devant le groupe.
2. Se déplacer jusqu'à ce qu'il soit **détecté**, sans entrer dans sa case.
   Utiliser un voleur sachant Pièges et possédant le kit.
3. Une fois son modèle rouge visible, pointer le modèle et faire
   **clic gauche**. Le curseur doit annoncer une interaction.
4. Vérifier l'événement de jet / désamorçage et la récompense d'XP
   (en cas de réussite).
5. Entrer ensuite dans sa case : aucune blessure si le piège
   a été désamorcé.
6. Sur une autre instance non désamorcée, se laisser blesser.
   Le second clic doit expliquer « déjà déclenché ».
7. Vérifier que les pièges inconnus ne sont jamais révélés par
   un simple survol et qu'un scan sans découverte ne révèle
   ni GUID ni position.

## Limites

La surbrillance rouge actuelle (stencil 252 / overlay de matériau facultatif)
reste une présentation provisoire. Son amélioration esthétique pourra
être traitée avec la création du matériau `M_Trap_RevealOverlay_Red`,
sans mélanger rendu et autorité du piège.

Le clic fonctionne pour les acteurs visuels dérivés de
`AGridGenericObjectActor` (les trois définitions TRAP01.5).
Les définitions qui utilisent uniquement `AGridRuntimeObjectActor`
ne sont pas automatiquement interactives.

Aucun fichier `.uasset`, placement de `L_Dungeon` ou changement
de l'algorithme de dégâts n'est inclus dans ce correctif.
