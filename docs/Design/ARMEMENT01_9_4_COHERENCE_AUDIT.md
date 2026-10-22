# ARMEMENT01.9.4 — Audit de cohérence Talents / Compétences / Kits

Date : 22 octobre 2026 — statut : **outil d'audit préparé ; compilation et exécution UE5.5.4 à confirmer**.

## Objectif et autorité

Observer la **cohérence** de l'équipement de départ, sans inventer de conditions de
port d'arme, sans attribuer des talents au niveau 1, sans modifier les six DataAssets
de classe, les 25 Skills, l'inventaire, le wizard ou la sauvegarde.

Autorités de données **lues**, jamais modifiées :
- `DA_Class_Warrior/Rogue/Ranger/Mage/Priest/Alchemist`, comprenant leurs vrais
  `ProgressionChoices[*].TalentBranchId` et `MinimumLevel` ;
- les douze kits réellement créés par `FRPGStartingEquipmentPresetCatalog`
  sur des **copies temporaires** de ces classes, sauf si les kits sont authorés dans
  le DataAsset (ceux-ci priment) ;
- `UGridItemDefinitionAsset` pour chaque objet de kit ;
- `DA_RPGSkillPresentation` et
  `URPGSkillPresentationAsset::ResolveClassAffinity` pour les **25 compétences**.

**Constat technique** : `FRPGStartingEquipmentService::TryBuildKit` vérifie
les définitions, quantités, niveaux, emplacements, conflit d'armes à deux mains
et capacité d'inventaire. Il **ne consulte pas** les Skills ni les talents
pour classer un kit. L'audit ne change pas ce comportement.

Le premier point de choix de talent est prévu **au niveau 2** via
`FRPGClassProgressionAuthoring::ConfigureCanonicalTalentGrants`, puis aux
niveaux pairs jusqu'au niveau 20. Le personnage de départ est niveau 1.
Une suggestion de branche n'est donc **pas** un talent possédé.

## Matrice des douze kits — hypothèses de revue, pas des règles

| Classe | Kit de départ | Proposition de rapprochement | Compétence à confronter à la vraie affinité | Lecture et lacune |
|---|---|---|---|---|
| Guerrier | Défenseur | Guardian | Skill_Shield | Bouclier pertinent ; vérifier les actions de bouclier et prérequis |
| Guerrier | Escarmoucheur | **À définir** | Skill_Throwing | Dague + couteau de lancer ; aucune branche spécialisée au lancer explicitement documentée |
| Voleur | Ombre | Shadow | Skill_Stealth | Le nom suggère Ombre, mais la dague et le couteau de lancer ne démontrent pas cette synergie |
| Voleur | Éclaireur | **À définir** | Skill_Perception | Torche et dague utilitaires, aucune branche Rogue « Scout » |
| Rôdeur | Archer | Marksman | Skill_RangedWeapons | Arc court directement compatible avec l'orientation tir ; vérifier l'approvisionnement en projectiles |
| Rôdeur | Pisteur | Scout | Skill_Survival | Torche + dague, cohérence d'exploration encore faible |
| Mage | Érudit | Arcanist | Skill_Arcana | La gemme bleue n'est pas validée comme focaliseur magique : **kit provisoire** |
| Mage | Vagabond | **À définir** | Skill_Arcana | Dague et torche : aucune identité magique attestée |
| Prêtre | Pèlerin | **À définir** | Skill_Religion | Dague et torche : manque de signe religieux ou matériel de soin dédié |
| Prêtre | Protecteur | Protection | Skill_Shield | Bouclier pertinent ; vérifier l'affinité de Shield pour Priest |
| Alchimiste | Apothicaire | Apothecary | Skill_Alchemy | Fiole vide pertinente mais pas de potion immédiatement disponible |
| Alchimiste | Artificier | Grenadier | Skill_Alchemy | Bombe de feu pertinente ; vérifier actions/prérequis au niveau 1 |

**Ces huit branchements sont des propositions révisables, pas une correspondance
automatique talent→kit.** Quatre kits restent volontairement sans rapprochement.
La compétence indiquée est une **question à examiner**, pas une affinité inventée.
La vraie affinité classe/Skill est obtenue à l'exécution depuis le catalogue.

## Contrôle en lecture seule sous Unreal

Nouveaux éléments :
- `FGridStartingKitCoherenceAudit::BuildProductionReport` : lecture seule et
  génération déterministe de trois CSV ;
- `UGridStartingKitCoherenceAuditCommandlet` : écrit sous
  `Saved/ARMEMENT/ARMEMENT01.9.4/`, jamais dans `Content/` ni les packages UE ;
- `Grimrock.RPG.ARMEMENT01.9.CoherenceAudit` : **trois tests** (couverture réelle,
  distinction proposition/autorité, déterminisme et absence de mutation).

Après `git pull --ff-only origin master`, lancer uniquement :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 `
  -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.CoherenceAudit"

& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=GridStartingKitCoherenceAudit -unattended -nop4 -nosplash -nullrhi
```

Sortie attendue (soumise aux tests locaux, **non attestée ici**) :

```text
Saved/ARMEMENT/ARMEMENT01.9.4/
  StartingKitCoherence.csv   # 12 kits, vrai contenu, branche hypothétique, Skill et affinité réelle
  ClassTalentBranches.csv    # 18 branches issues des DataAssets, prérequis niveau minimum
  ClassSkillAffinities.csv   # 150 couples (6 classes × 25 compétences)
```

`ReadOnlyAudit OK Classes=6 Kits=12 Branches=18 Skills=25 AffinityPairs=150
KitsWithoutProposedBranch=4` est le signal attendu du commandlet.
Il faut vérifier le **journal et les fichiers générés** avant de déclarer
l'audit définitivement validé. Les rapports sont des sorties de revue, pas
des données de gameplay et **ne doivent pas être committés comme assets**.

## Ce qui reste à décider après la revue

1. **ARMEMENT01.9.5 — validation de la matrice** : approuver ou rejeter
   les huit suggestions ; ne pas obliger un kit à correspondre à une branche
   exclusive tant que le personnage n'a pas choisi ses talents.
2. **Kits génériques** : déterminer pour Escarmoucheur, Éclaireur,
   Vagabond et Pèlerin si le kit sert une orientation sans talent,
   ou si un équipement distinct manque au catalogue.
3. **Disponibilité réelle** : focaliseur du mage, matériel de prêtre,
   consommables utiles d'alchimie, munitions et outils d'exploration ;
   créer les définitions/visuels manquants avant de durcir les contrôles.
4. **Présentation facultative** : afficher une **affinité conseillée** et une
   éventuelle lacune dans le wizard, depuis un read model localisé
   `ST_UI`. Pas de sixième étape ni de nouveau SaveGame.
5. **Aucun verrou arbitraire** : une compétence « déconseillée » ou une
   branche suggérée ne doivent pas empêcher de choisir un kit de classe.

## État restant du chantier général ARMEMENT01

### Développé et couvert par des validations antérieures

- ARMEMENT01.2–.3 : modèle de dégâts et vrais jets multi-dés.
- ARMEMENT01.4 : catalogue statistique 82 × 20 = 1 640 profils (et pas
  1 640 modèles 3D).
- ARMEMENT01.5 : `ItemLevel` fixe sur l'instance, lié au catalogue.
- ARMEMENT01.6 : résolveurs et présentation visuelle avec
  **trois objets pilotes** (dague, arc court, bouclier rond).
- ARMEMENT01.7 : objets ramassables et règle d'arme à deux mains.
- ARMEMENT01.8.1–.4 : niveau fixe pour placement, spawns, réceptacles,
  butin de monstres ; tests ciblés déjà remis par l'utilisateur.
- ARMEMENT01.9.1–.3 : kits transactionnels, six classes, choix dans wizard,
  reprise SaveGame et localisation. La dernière revue visuelle de Résumé
  a été fournie par l'utilisateur le 22.10.2026.

### Restant ou non clôturé — à planifier sans présumer d'une implémentation

| Priorité | Travail | Nature |
|---|---|---|
| P0 | 01.9.4 : faire exécuter le commandlet, lire les rapports, confirmer les résultats | Audit / validation |
| P0 | 01.9.5 : approuver la cohérence talent/compétence/kit et corriger les 4 lacunes | Décisions de design |
| P1 | Étendre les **collectibles réels et leurs visuels** au-delà des 3 pilotes ; prioriser les équipements requis par les six classes | Assets / authoring |
| P1 | Revoir l'UX des kits : icônes, conseils contextualisés, localisation exhaustive, revue de sauvegarde complète | Gameplay UI / tests |
| P2 | Décider et, si approuvé, implémenter les **pénalités de maniement d'un objet de niveau supérieur** (`ARM-004`) | Proposition v0.3 non approuvée |
| P2 | Statuts de **rareté / affixes**, forges, huiles, runes et améliorations sans changer `ItemLevel` (`ARM-005/006`) | Design et implémentations distinctes |
| P2 | Couverture fonctionnelle/visuelle, balancing et tests de non-régression pour les **82 archétypes**, pas seulement les pilotes | Campagne de finition |

Les propositions `ARM-004/005/006` du contrat v0.3
(`docs/Design/ARMEMENT01_CONTRAT_ARMEMENT_V0_3.md`) ne doivent
**pas** être qualifiées d'implémentations validées : ce sont des travaux
ultérieurs à trancher, distincts des systèmes de forge/alchimie existants
dans d'autres modules du jeu.
