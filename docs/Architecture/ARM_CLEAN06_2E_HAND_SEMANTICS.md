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
