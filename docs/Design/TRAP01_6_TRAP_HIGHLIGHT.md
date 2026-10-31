# TRAP01.6 — Surbrillance et états visuels des pièges

## Principe

L'état persistant du piège reste l'autorité du gameplay. Sa présentation est une
projection de cet état et des propriétés de sa définition
`UGridWorldObjectDefinitionAsset`. Le système existant est conservé : mesh runtime,
Overlay Material, CustomDepth et stencil. Aucun nouvel acteur d'effet, matériau
imposé ou système de sélection n'est ajouté.

| État persistant | Modèle | Effet de danger | Interaction souris |
| --- | --- | --- | --- |
| Hidden | Invisible, non créé si absent | Aucun | Aucune |
| Detected | Visible | Selon la définition | Conservée |
| Disarmed | Visible | Aucun | Statut consultable |
| Disabled | Visible | Aucun | Statut consultable |
| Triggered, `bOneShot = true` | Visible | Aucun | Statut consultable |
| Triggered, `bOneShot = false` | Visible | Selon la définition, jusqu'à désamorçage/désactivation | Conservée |

Le mesh révélé conserve sa collision souris `QueryOnly` et sa réponse bloquante
à `Visibility`. Les options graphiques ne changent ni cette collision, ni les
dégâts, ni les conditions de désamorçage. Un piège dangereux peut donc être affiché
sans surbrillance tout en conservant son comportement.

## Réglages dans Unreal Editor

Ouvrir le DataAsset **World Object Definition** du piège, puis ses propriétés
`Dungeon Trap`, catégorie `Reveal`. Il ne s'agit pas du DataAsset de palette ou
de placement. Les noms ci-dessous sont les identifiants C++ ; l'éditeur affiche
habituellement leurs versions espacées, sans le préfixe `b` des booléens.

| Propriété dans `DungeonTrap` | Défaut | Effet |
| --- | --- | --- |
| `bEnableDangerHighlight` | `true` | Autorise Overlay et CustomDepth seulement tant que le piège est dangereux. Décochée : retire les deux effets. |
| `RevealedOverlayMaterial` | Aucun | Material ou Material Instance de danger, partagé avec le mécanisme existant. |
| `bUseDangerCustomDepth` | `true` | Autorise l'écriture CustomDepth/stencil, indépendamment de la présence d'un Overlay. |
| `DangerStencilValue` | `252` | Valeur de 3 à 255, utilisée uniquement si CustomDepth est actif. |

Les options sont visibles lorsque `bIsDungeonTrap` est activé ; les sous-options
sont masquées lorsque leur activation parente est décochée. Les anciennes
définitions conservent le comportement de révélation par défaut et leur référence
Overlay existante. Aucune migration d'asset n'est nécessaire.

Chaque définition possède ses propres réglages. Pour différencier deux pièges,
utiliser des définitions distinctes et, si nécessaire, des Material Instances
distinctes. Deux définitions référençant la même instance partagent ses couleurs.
Modifier ces assets avant de relancer le PIE pour valider le résultat ; il n'y a
pas de surveillance automatique des modifications d'assets pendant une partie.

### Couleur, intensité et opacité

Réutiliser en priorité le matériau Overlay déjà assigné. Si son parent expose
les paramètres nécessaires, créer une **Material Instance** depuis le Content
Browser (clic droit sur le matériau, **Create Material Instance**), puis :

1. Ouvrir l'instance et activer les cases de remplacement des paramètres du parent.
2. Régler couleur, intensité et opacité ; leurs noms dépendent du matériau parent.
3. Assigner l'instance à `DungeonTrap.RevealedOverlayMaterial` dans la définition.
4. Enregistrer et tester sur le mesh réel du piège en PIE.

Si le parent ne possède pas ces paramètres, les ajouter **dans Unreal Editor**.
Un parent Overlay simple peut utiliser le domaine `Surface`, le mode de mélange
`Translucent` et le modèle `Unlit` : un Vector Parameter `DangerColor` multiplié
par un Scalar Parameter `DangerIntensity` alimente `Emissive Color`, et un Scalar
Parameter `DangerOpacity` alimente `Opacity` (plage conseillée 0 à 1). Ces noms
sont une convention d'authoring, pas des paramètres lus ou forcés par le C++.
Créer ensuite une instance de ce parent. Adapter les valeurs au rendu du niveau.

La couleur n'est plus décrite comme nécessairement rouge. Le C++ ne crée pas
d'instance dynamique pour la production et ne remplace jamais les paramètres
de l'instance choisie. Aucun `.uasset` graphique n'est créé ou modifié par ce jalon.

### Absence de matériau et stencil

- Sans Overlay : le modèle conserve ses matériaux de base. CustomDepth peut
  toujours servir à un postprocess existant, si son option est activée.
- Sans Overlay et sans CustomDepth : aucun effet supplémentaire ; modèle et
  interaction restent présents après révélation.
- CustomDepth seul ne garantit pas une couleur visible : il fournit un masque
  que le matériau postprocess doit exploiter. Changer le stencil demande donc
  d'adapter le filtre du postprocess associé, s'il en existe un.

L'audit du code et de la configuration trouve `r.CustomDepth=3` dans
`Config/DefaultEngine.ini`. `GridEditorPreviewObjectActor` utilise **0** pour
l'absence de marquage, **1** pour le survol et **2** pour la sélection, y compris
sur les composants mobiles. Ces usages restent inchangés. Le réglage des pièges
exclut 0/1/2 ; le runtime borne également toute valeur hors plage à 3..255,
sans invalider le contrat de gameplay. **252 reste un défaut de compatibilité,
pas une valeur imposée.** L'audit ne constitue pas une inspection des graphes de
tous les matériaux/Blueprints personnalisés : coordonner les autres masques
éventuellement authorés dans les assets lors du choix d'une valeur.

## Application, nettoyage et restauration

`RefreshDungeonTrapRevealVisual` calcule séparément révélation et danger. À chaque
appel, il assigne l'Overlay voulu **ou `nullptr`**, active **ou désactive**
CustomDepth, et écrit le stencil voulu **ou zéro**. Ainsi, changer l'état ou
retirer un matériau nettoie effectivement l'ancien effet.

Le succès de `TryDisarmTriggerTrap` et `TryRecordDungeonTrapTriggered` appellent
déjà ce rafraîchissement. Une modification explicite des ensembles persistants,
notamment `DisabledTriggerTrapIds`, doit être suivie du rafraîchissement ou de la
restauration normale. Ce jalon n'ajoute aucune règle de désactivation.

`RestoreDungeonTrapRevealVisuals`, utilisé par la restauration et la reconstruction
du niveau, traite aussi les pièges Hidden : il nettoie/cache un ancien visuel sans
en créer un nouveau. Les objets qui ne sont pas des pièges restent exclus.
Les états Disarmed/Disabled/Triggered sont restaurés sans réintroduire un effet
de danger inapproprié. Aucune donnée graphique supplémentaire n'est sauvegardée.

## Validation

Les assertions préexistantes sont conservées. `TRAP01.4.BestEligibleMemberAndDexterity`
vérifie désormais qu'un désamorçage réussi retire immédiatement Overlay,
CustomDepth et stencil, conserve le modèle/collision et permet toujours
`Execute_CanInteract` pour consulter le statut.

`TRAP01.6.HighlightTransitionsAndRestore` couvre les cinq états avec et sans
Material Instance, en usage unique et réarmable, en partant d'un effet déjà actif.
Il contrôle la reconstruction après sérialisation/rechargement par les API UE
`SaveGameToMemory` / `LoadGameFromMemory`, l'appel réel de
déclenchement, les options indépendantes, le retrait d'un matériau et les bornes
du stencil. Les matériaux de test sont transitoires et ne produisent aucun asset.

Les tests sans RHI contrôlent l'état des composants ; ils ne valident pas
l'apparence du matériau ou du postprocess. Contrôles PIE à effectuer :

1. Détecter les pièges sol/mur/plafond et comparer deux définitions avec des
   instances/couleurs différentes.
2. Changer couleur, intensité, opacité dans leurs instances, relancer le PIE et
   constater le résultat sans compilation C++.
3. Désamorcer, désactiver, déclencher en usage unique puis réarmable ; vérifier
   modèle, effet de danger, curseur et consultation du statut.
4. Sauvegarder/continuer et reconstruire le niveau dans ces états.
5. Retirer l'Overlay, désactiver CustomDepth, désactiver toute surbrillance et
   vérifier les trois configurations ; vérifier aussi les sélections de l'éditeur.

Aucun texte joueur ni WBP n'est modifié. Le pipeline
`UI.csv → ST_UI → WBP → Game/fr/en` est hors périmètre de ce jalon.
TRAP01.7 n'est pas engagé.

### Résultats techniques du 31 octobre 2026

- Compilation réelle `GrimrockPrototypeEditor Win64 Development` sous UE 5.5.4 :
  réussie, code 0, avec génération UHT des nouvelles propriétés.
- Toolchain v143 conservée sous Visual Studio 2026 :
  `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207`.
  UBT affiche la version compilateur 14.44.35229 et son avertissement habituel
  de version non préférée ; aucun changement de toolchain.
- `Grimrock.TRAP01` : **10 succès**, zéro échec, zéro avertissement
  d'automatisation. Inclut TRAP01.2, TRAP01.3, TRAP01.4 et TRAP01.6.
  Rapport local : `Saved/Automation/TRAP016/TD04-20261031-121337/index.json`.
- Régressions `Grimrock.WorldObjects.MOVINGPARTS01` (dont stencil de sélection),
  `Grimrock.RPG.ARMEMENT01.16.TrapInteraction` et
  `Grimrock.Runtime.Objects.GenericAudioContract` : **3 succès**, zéro échec,
  zéro avertissement d'automatisation.
  Rapport local : `Saved/Automation/TRAP016Regression/TD04-20261031-121403/index.json`.
- Nouveau fichier de test formaté avec clang-format standalone **19.1.5** ;
  `git diff --check` sans défaut. Les assets préexistants de l'utilisateur et
  les rapports sous `Saved/` ne font pas partie du commit.
