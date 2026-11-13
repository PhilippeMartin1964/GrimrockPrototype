# ARM-CLEAN06.2E — Recensement semantique natif des mains et familles

## Situation apres 06.2D

Les 82 archetypes et leurs 20 niveaux sont numeriquement compatibles
avec la cible (06.2C), les variantes audio et Niagara possedent un
modele sans perte (06.2B), et les donnees des six classes et 82 affinites
sont conservees sans reduction (06.2D).

Le catalogue ancien serialize Family et HandUsage comme chaines
informatives provenant du XLSX. Le comportement d'equipement reel
est determine par UGridItemDefinitionAsset::GetEffectiveHandUsage()
et CanEquipToSlot(). Ces autorites sont distinctes.

La baseline contient 24 definitions concretes et 82 archetypes ARM.
Les 24 definitions ne permettent pas a elles seules de determiner
les mains des 82 archetypes. Un profil passif sans degats ne
devient pas automatiquement un bouclier; un focus reste possible.

## Outil Unreal Editor en lecture seule

Le commandlet UGridArmClean06HandSemanticsCommandlet examine :

- les 82 enregistrements sources, leurs valeurs Family et HandUsage
  intactes ainsi que leur activite offensive ;
- les 24 vrais DA_Weapon_*, ItemDefinitionId, ItemType,
  WeaponArchetypeId, bUseWeaponCatalogue, usage de main effectif
  et permissions MainHand / OffHand ;
- la jointure de chaque definition avec son profil ARM N1 ;
- les politiques prouvables TwoHanded, EitherHand, MainHand,
  OffHand, ou la valeur explicite Unresolved ;
- les boucliers passifs et armes offensives demontres par
  ItemType et DamageDice ; les Focus restent indeterminables
  sans information de type certaine ;
- les divergences entre plusieurs definitions du meme archetype
  et les archetypes encore sans definition concrete.

Le texte informatif HandUsage du catalogue ne cree jamais de
restriction d'equipement et aucun raccourci actif n'est invente.

## Execution autonome

Apres build et Automation Grimrock reussis, l'agent execute une seule
fois Scripts/PreflightARM_CLEAN06_2_HandSemantics.ps1 sur le SHA du
commit ayant pour sujet exact :
chore(ARM-CLEAN06.2E): inventory UE weapon hand semantics read-only

Le script exige un worktree detache, propre et synchronise sur
ce SHA, demarre UnrealEditor-Cmd avec -NullRHI et ecrit un rapport
JSON complet UNIQUEMENT dans le dossier prive de validation.
Une issue GitHub publie les compteurs sans noms de fichiers,
champs textuels d'origine ni autres valeurs brutes.

Le statut GitHub dedie est UE5.5.4/ARM-CLEAN06.2E.
Success signifie audit execute en lecture seule, PAS conversion
du contenu ni autorisation d'effacer les anciens champs.

Aucun .uasset, .umap, audio Sword / RatGiant, SaveGame,
lecteur runtime ni presentation WBP n'est modifie.

## Suite

Il reste a trancher les types de Focus, les regles de mains
des archetypes sans definition concrete, les liaisons d'action,
reactions, munitions, visuels et objets non-ARM. Les assets
canoniques seront authorés seulement avec preuve de parite
sur le vrai contenu UE ; la bascule des lecteurs 06.3,
le cutover 06.4 et la suppression 06.5 ne sont pas autorises
sur la base de ce seul audit. Pas de validation PIE necessaire.


## ARM-CLEAN06.2E.1 — Réparation de l'inventaire des identités

Sur le SHA `39e4526d`, le build UE et la suite Automation `Grimrock`
étaient verts, mais le commandlet additionnel s'est arrêté à l'étape
`COMMANDLET`. La cause détaillée n'était pas identifiable dans les
diagnostics publics : elle ne doit donc pas être présentée comme certaine.

La version 06.2E.1 apporte deux protections :

1. Un `ItemDefinitionId` vide ou répété parmi les 24 définitions est
   désormais **une anomalie inventoriée**, pas une raison d'interrompre
   l'examen avant la fin du dossier. Les 24 lignes sont conservées,
   sans renommer, fusionner, corriger ni ignorer aucune variante
   (notamment les ThrowingKnife).
2. Le script Windows distingue par codes **strictement filtrés**
   les causes possibles : argument, catalogue, archétype, nombre
   d'assets, sortie JSON, rapport absent ou erreur native. Seule une
   catégorie prédéfinie peut paraître dans le statut GitHub ; les
   données métiers, chemins, logs bruts et noms d'assets restent privés.

Le rapport natif comporte les compteurs
`unique_nonempty_item_ids`, `missing_item_ids` et
`duplicate_item_ids`. Leur somme doit valoir 24. Un ID absent ou
dupliqué **bloque toujours la migration binaire**, même si l'inventaire
en lecture seule peut être validé. Les joints de catalogue et les
règles de mains encore incertaines restent non résolus.

La reprise automatique est attachée au sujet exact du commit
`fix(ARM-CLEAN06.2E.1): preserve conflicting IDs and diagnose native census`.
L'agent exécute toujours l'audit uniquement **après** réussite de la
compilation et de tous les tests `Grimrock`, dans un worktree détaché
et propre. Une réussite exige aussi la création du JSON natif,
sa validation structurelle, la propreté Git après le commandlet et
la publication d'une issue résumée. Le statut spécifique
`UE5.5.4/ARM-CLEAN06.2E` reste distinct de `UE5.5.4/LocalAuto`.

**Aucune conversion des assets, suppression d'UPROPERTY, bascule
runtime, opération SaveGame ni modification Sword/RatGiant.**
Une recette PIE ne serait pas significative à ce stade.
