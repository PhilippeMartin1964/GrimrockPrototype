# ARM-CLEAN06.2Y — Prévol intégré des 22 armes, flèches et projectiles

## Candidat transitoire unique

Le point d'entrée Editor-only
`GridArmClean06CompleteProductionPreview::Build`
compose dans un même `UGridItemSystemAsset` **transient** :

- les **22** définitions ARM de production et leur
  `DA_WeaponVisualCatalogue` (06.2P) ;
- les attaques et présentations dont la projection
  complète est démontrée, sans inventer d'identifiants
  `ActionId/PresentationId` (06.2Q) ;
- les **82** archétypes N1–N20, soit **1 640** profils
  numériques et **260** profils passifs, les affinités
  et les règles de mains prouvées pour 22 objets (06.2S) ;
- le **vrai** `DA_Item_Arrows_01`, sous son identifiant
  original, et les liaisons d'arcs réellement authorées
  (06.2V) ;
- les politiques de lancer physiques **et** les
  sept réglages visuels des trois ARM `ThrowingOneHand`,
  à leurs deux propriétaires respectifs (06.2W/06.2X).

Le candidat comporte ainsi **23 définitions** :
22 armes/boucliers/focaliseurs et la munition réelle.
Les 60 ARM non concrétisés restent de simples progressions,
non des données de gameplay imaginées.

## Barrière de migration

`ReadyForBinaryCutover()` refuse le cutover tant qu'il
existe des actions de production sans liaison ou non
projetées, des règles de munitions et de lancer non prouvées,
des identités incompatibles, ou une erreur
`UGridItemSystemAsset::Validate`.

Les droits d'attaque MainHand/OffHand observés
sont conservés (19 et deux), **même lorsque** leurs
`ItemActionBinding` ne sont pas encore materialisés :
le validateur 06.2T signale l'incomplétude au lieu de
supprimer les commandes ou de fabriquer une présentation.

Le catalogue numérique reste **structurellement invalide**
tant qu'un ARM actif possède une source MainHand/OffHand
sans liaison d'action authorée. Le prévol accepte uniquement
cette erreur de validation connue, en exigeant une erreur par
arme offensive non liée ; tout autre défaut structurel
interrompt la construction. Le registre complet reste
invalide et `ReadyForBinaryCutover()` demeure faux.

Les trois politiques de vol et les trois présentations
de projectile sont comparées aux vrais DataAssets,
et leurs compteurs non convertis ne sont annulés
qu'après vérification des champs. Aucun fallback
vers l'ancien runtime n'est introduit dans le
nouveau registre.

## Automation et limites

Le test
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.CompleteProductionPreview23`
charge le seul manifeste officiel, vérifie 23 définitions,
82/1 640/260, les liaisons de flèches, les trois
projectiles numériques/visuels, l'absence d'écriture,
le refus d'une double projection et l'état du cutover.

Ce prévol ne remplace ni les DataAssets existants,
ni la sauvegarde, ni les lecteurs gameplay.
Les actions personnalisées, les IDs de présentation
absents et les propriétés de réactions/défenses
devront recevoir une conversion explicite avant
une migration binaire sous Unreal Editor 5.5.4.
Les outils transitoires disparaîtront après le
cutover définitif, conformément au contrat
ARM-CLEAN06. La validation Windows `SilentTask`
et `LocalAuto` doit réussir sur le SHA publié.
