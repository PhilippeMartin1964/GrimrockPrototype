# ARM-CLEAN06.2U — Prévol unique des 22 ARM et quatre catalogues

Ce jalon **assemble réellement en mémoire** les résultats de 06.2P,
06.2Q et 06.2S sous un même `UGridItemSystemAsset`, sans
`SavePackage` : 22 définitions concrètes, visuels et icônes d'actions,
82 profils numériques compactés, un catalogue d'actions et un de
présentations contenant **uniquement les profils démontrés**.

La projection des droits d'attaque MainHand (19) et OffHand (2) est
effectuée depuis les vrais `DA_Weapon_*` du seul manifeste. Les droits
`AttackSourceSlots` sont conservés **même si** l'action ne dispose
pas encore d'un couple `ActionId/PresentationId`. Le garde 06.2T
fait échouer la validation dans ce cas : aucun bouton d'attaque ne
disparaît silencieusement ni ne devient une action fantôme.

Un diagnostic structuré distingue les sources d'attaque sans
liaison, les munitions dont `AmmoKindId` n'est pas raccordé,
les règles de lancer non migrées, les identités incompatibles et
les actions personnalisées restant dans leurs sources. La
méthode `ReadyForBinaryCutover()` exige zéro écart et un
`UGridItemSystemAsset::Validate` réussi ; elle n'autorise
aucun transfert actuellement.

Le test Automation
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.FullProductionCandidatePreflight`
utilise les véritables assets du jeu, prouve la composition,
contrôle les nombres, la préservation des mains et les
packages restés propres, puis refuse un nouvel assemblage
sur la même destination.

Cela **n'est pas** une migration de .uasset ni une activation
du système canonique. Les 60 archétypes sans définition
concrète restent numériques et sans sémantique de main
inférée. La suite est la création des références canoniques
manquantes via l'Editor Unreal, puis leur parité gameplay
et Save/Continue avant le cutover. Aucune fiche hors
manifeste n'est examinée ou modifiée.
