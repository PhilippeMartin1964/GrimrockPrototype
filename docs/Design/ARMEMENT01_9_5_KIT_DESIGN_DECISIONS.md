# ARMEMENT01.9.5 — Décisions de conception Talents / Compétences / Kits

> **Statut au 22.10.2026 — DÉCISIONS PROPOSÉES REMPLACÉES.** Les statuts `Retain/Revise/RetainConditional/RetainUtility` et le fichier `ARMEMENT01_9_5_KIT_DECISIONS.csv` constituent une **revue historique des 12 kits**. Ils **ne doivent plus guider la fabrication des nouveaux assets**. La référence pour les 18 futures recettes est [ARMEMENT01.9.6](ARMEMENT01_9_6_EIGHTEEN_STARTING_KITS.md), elle-même encore `DesignProposal`, et le contrat de nettoyage est [ARMEMENT01.9.6.1](ARMEMENT01_9_6_1_CLEANUP_CONSOLIDATION.md). Ne pas supprimer le CSV : les tests historiques le consomment.

**22 octobre 2026** — Statut : **revue de design proposée, en attente de ratification ; aucune modification de gameplay**.

Ce jalon transforme l'audit technique **ARMEMENT01.9.4**, validé localement
(3/3 tests), en décisions de design **traçables et révisables**.
La matrice source est :
`docs/Design/ARMEMENT01_9_5_KIT_DECISIONS.csv`.

## Correctif d'autorité — ARMEMENT01.9.5.1 (22 octobre 2026)

**Constat et priorité :** la première revue 01.9.4/01.9.5 a utilisé les affinités de compétences
(`DA_RPGSkillPresentation`) et les talents, **mais a omis l'affinité d'ARMEMENT
déjà spécifiée dans le classeur maître**. Les conclusions de kits ci-dessous sont des
propositions provisoires ; aucune ne doit être approuvée comme conformité
d'équipement N1 avant cette correction.

**Source de vérité existante, pas une nouvelle matrice :**

`docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`

- Onglet **Classes** : six classes, 18 spécialités, armement prioritaire, exemples
  emblématiques, alternatives, attributs dominants, synergies.
- Onglet **Catalogue** : 82 archétypes, dont les champs **Classe principale** et
  **Classes secondaires** (colonnes P et Q dans la source XLSX).
- Onglet **Niveaux 1-20** : véritable profil N1 pour chaque archétype ; niveau
  d'objet fixe, distinct du niveau du personnage.
- **Autorité de conception explicite dans l'onglet Classes** : « Les préférences
  guident les talents, l'équilibrage et les drops — aucune interdiction rigide
  de classe proposée. » Il serait donc contraire à la source de transformer
  l'affinité en `CanEquip` bloquant par classe.

**Défaut d'intégration constaté :** `Scripts/ExportARMEMENT014Catalogue.ps1`
lit déjà les lignes de l'onglet Catalogue et leurs profils N1–N20, mais
l'export normalisé ne conserve que les 19 colonnes statistiques :
les colonnes P/Q sont perdues à l'import. `FGridWeaponLevelProfile`
ne contient aucune référence à la classe prioritaire ou secondaire.
La génération des kits `FRPGStartingEquipmentPresetCatalog` ne consulte
pas ces données : elle choisit dans les quelques `UGridItemDefinitionAsset`
produits, et non selon l'affinité prévue dans le classeur.

### Écarts concrets du kit N1 courant

| Archétype documenté | Classe principale XLSX | Classes secondaires XLSX | Conséquence pour les kits |
|---|---|---|---|
| `ARM001` Dague | Voleur | Rôdeur, Alchimiste | Appropriée pour les deux kits Voleur ; secondaire pour Rôdeur/Alchimiste ; **pas l'arme privilégiée du Guerrier ou du Prêtre** ; simplement arme de secours pour le Mage selon l'onglet Classes |
| `ARM045` Arc court | Rôdeur | Voleur, Alchimiste | Valide pour le Rôdeur Archer, dont le kit actuel utilise déjà le véritable profil N1 |
| `ARM054` Couteau de jet | Rôdeur | Voleur, Alchimiste | **Non privilégié pour le Guerrier Escarmoucheur**, contrairement au choix actuel ; utilisable comme complément du Voleur |
| `ARM075` Bouclier rond | Guerrier | Prêtre | Défenseur et Protecteur utilisent le bouclier en cohérence avec leur classe, même si leurs armes principales actuelles sont moins pertinentes |
| `ARM061` Baguette arcanique | Mage | Prêtre, Alchimiste | Exemple de premier armement N1 à authorer pour le Mage, au lieu de qualifier une gemme sans comportement |
| `ARM069` Sceptre sacré | Prêtre | Mage | Exemple d'armement N1 de Prêtre à authorer ; **aucun pouvoir particulier ne peut être présumé sans implémentation** |

Les autres choix de kit utilitaires (torche, fiole, bombe, gemme)
ne figurent **pas** dans cette classification d'armes à 82 archétypes ;
ils requièrent leurs propres vérifications d'usage, sans affinité inventée.

### Hiérarchie d'évaluation corrigée

1. **XLSX onglet Classes + Catalogue** → pertinence d'armement **par classe**
   (primaire / secondaire / non spécifié), **sans verrou d'équipement**.
2. **XLSX onglet Niveaux 1-20** → profil immuable du **véritable objet N1**
   à la création, sous réserve que l'asset UE existe et soit utilisable.
3. **`DA_RPGSkillPresentation`** → affinité avec les **compétences** ; axe
   séparé de l'affinité arme/classe.
4. **`DA_Class_*` et mécanique RPG02** → talents réels, premier choix au
   niveau 2 ; orientation future, jamais talent déjà acquis au départ.
5. **Gameplay/production UE** → compatibilité du slot, actions et visuels,
   capacité, sauvegarde, disponibilité réelle de l'objet.

### Correctif technique nécessaire après cette rectification documentaire

- Étendre le pipeline d'**export depuis le XLSX existant** afin de conserver
  le mapping `ArchetypeId → classe principale / classes secondaires` et la
  lecture de l'onglet Classes. Le résultat doit être **dérivé du XLSX**, pas
  réécrit dans un deuxième tableau manuel.
- Rendre les affinités accessibles à un **read model C++ de recommandation**
  pour le New Game ; ne pas créer une règle de `CanEquip` par classe.
- Évaluer les 12 kits à partir de leurs vrais `WeaponArchetypeId` et
  `ItemLevel=1` ; signaler explicitement **asset N1 non authoré** plutôt
  que remplacer silencieusement par la dague pilote.
- Authorer prioritairement les objets N1 manquants en respectant
  leurs statistiques **déjà fixées dans le XLSX** ; ajuster ensuite les
  recettes de kit et leurs tests transactionnels.
- Conserver la localisation via `Localization/Source/UI.csv → ST_UI`,
  sans libellés d'armes ou de classes français codés en dur côté runtime.

**Statut de cette section : correction documentaire de la source de vérité.
Aucune donnée runtime n'est encore importée pour les affinités, aucun
`.uasset` ni kit de départ n'est modifié ici.**

## Principe directeur

Une classe propose des **orientations de départ**, pas des talents acquis.
Les premiers choix de talent interviennent au **niveau 2**. Les kits restent
disponibles au **niveau 1** et ne modifient ni la progression, ni les
pré-requis des compétences, ni l'accès aux items.

Le mot **« lien »** signifie ici : orientation conseillée, jamais octroi du
talent, restriction d'inventaire, pénalité ou droit d'utiliser une arme.

Sources vérifiées :
- `StartingKitCoherence.csv` (12 kits, objets/slots réels) ;
- `ClassTalentBranches.csv` (18 branches et choix réels niveau 2) ;
- `ClassSkillAffinities.csv` (150 affinités) ;
- `docs/Rules/RPG_Talents_Mechanics_v0_1.md` (mécaniques et contraintes des talents) ;
- `FRPGStartingEquipmentPresetCatalog` (recettes initiales actuelles) ;
- `FRPGStartingEquipmentService` (validation transactionnelle, sans affinité).

L'audit des 12 compétences référencées donne : **10 Recommended**,
**1 Useful** (Prêtre / Shield) et **1 Neutral** (Guerrier / Throwing).
Il s'agit de compétences *associées manuellement par l'audit* aux kits,
**non** de compétences automatiquement déduites des objets.

### Sens exact des statuts dans la matrice

| Statut | Nombre | Décision proposée |
|---|---:|---|
| `Retain` | 1 | Kit satisfaisant pour son rôle actuel ; vérifications pratiques restantes |
| `RetainUtility` | 1 | Kit d'exploration pertinent sans lien obligatoire à une branche |
| `RetainConditional` | 5 | Kit utilisable ; son association à la branche doit être présentée comme une **orientation** et non une synergie démontrée |
| `Revise` | 5 | L'équipement actuel n'illustre pas assez la mécanique visée ; remplacement ciblé à concevoir |

**Toutes les lignes portent `ApprovalState=PendingApproval`.**
« Retain » ne signifie donc **pas encore** que l'utilisateur a
définitivement approuvé le contenu des kits.

## Revue détaillée des 12 kits

### Guerrier — Défenseur (`RetainConditional`)

- Contenu actuel : dague `Weapon_Dagger_01` en main principale,
  bouclier rond `Weapon_RoundShield_01` en main secondaire.
- Branche proposée : `Guardian`. Niveau 2 :
  `Talent_Warrior_Guardian_DefensiveStance`, action de **posture défensive**.
  Elle applique `Status_Guarded` et **ne requiert pas de bouclier**.
  En revanche, `Talent_Warrior_Guardian_ShieldBash` au niveau 6 exige
  effectivement un bouclier équipé.
- `Skill_Shield` est **Recommended** ; cela appuie le bouclier,
  mais pas nécessairement la dague, `Skill_LightWeapons` étant
  **Neutral** pour Guerrier.
- Proposition : **garder le kit à titre provisoire**. Étudier une arme
  typique du Guerrier avant de remplacer la dague. Ne jamais présenter
  le bouclier comme requis pour `DefensiveStance`.

### Guerrier — Escarmoucheur (`Revise`)

- Dague + couteau de lancer ; `Skill_Throwing` = **Neutral**.
- Aucune des trois branches actuelles (Guardian, Breaker, WeaponMaster)
  n'identifie un parcours initial spécifique au lancer.
- Proposition : **revoir le rôle**. Soit « Combattant mobile » compatible
  WeaponMaster avec une arme mieux adaptée, soit kit utilitaire assumé.
  Ne pas créer artificiellement une quatrième branche. Ne pas
  prétendre que `Throwing` est recommandé au Guerrier.

### Voleur — Ombre (`RetainConditional`)

- Dague + couteau de lancer ; `Skill_Stealth` = **Recommended**.
- Branche `Shadow`, niveau 2 `Talent_Rogue_Shadow_Dodge` :
  action personnelle donnant `Status_Evasive` ; **aucune dague requise**.
  La disparition n'arrive que plus tard dans cette branche.
- Proposition : **conserver** pour un style mobile, mais vérifier si les
  objets et le tutoriel rendent perceptible la furtivité.
  `Dodge` ne doit pas être présenté comme débloqué grâce à la dague.

### Voleur — Éclaireur (`RetainUtility`)

- Dague + torche ; `Skill_Perception` = **Recommended**.
- Aucune branche Rogue `Scout` (Assassin / Shadow / Saboteur uniquement).
- Proposition : **conserver comme kit d'exploration transversal** ;
  ne pas forcer un `TalentBranchId`. Éventuellement introduire plus
  tard un outil de détection/désamorçage si réellement disponible.

### Rôdeur — Archer (`Retain`)

- Arc court à deux mains + dague au sac ;
  `Skill_RangedWeapons` = **Recommended**.
- `Talent_Ranger_Marksman_PreciseShot` au niveau 2 requiert
  **une arme à distance** et augmente sa portée. L'arc court correspond
  matériellement à la contrainte.
- Proposition : conserver ce kit comme **référence de cohérence** ;
  vérifier séparément munitions, projectile/retrait et comportement
  en début de partie. Le talent reste indisponible avant le niveau 2.

### Rôdeur — Pisteur (`RetainConditional`)

- Dague + torche ; `Skill_Survival` = **Recommended**.
- Branche `Scout` : `Talent_Ranger_Scout_Vigilance` niveau 2
  est un **passif** ; il ne requiert pas une torche.
- Proposition : conserver la vocation survie/exploration, mais
  évaluer un équipement de pistage réel (si des outils existent).
  Le talent n'est pas acquis au niveau 1.

### Mage — Érudit (`Revise`)

- Dague et `Gem_Blue`, toutes deux dans le sac ;
  `Skill_Arcana` = **Recommended**.
- `Talent_Mage_Arcanist_ArcaneShield` niveau 2 est un **sort** :
  restauration d'armure magique avec coût en mana. **Aucun prérequis
  de gemme bleue ou de focaliseur n'est prévu par la mécanique.**
- Proposition : remplacer/confirmer la gemme par un vrai composant
  magique ou un focaliseur **avec comportement validé**, ou conserver
  la gemme uniquement comme ressource sans lui attribuer de pouvoir
  non implémenté. Réexaminer l'arme de départ du Mage.

### Mage — Vagabond (`Revise`)

- Dague + torche dans le sac ; `Skill_Arcana` = **Recommended**
  mais les objets ne matérialisent pas cette affinité.
- Aucune branche associée justifiée.
- Proposition : réorienter le kit vers une voie de magie identifiable,
  ou l'assumer comme survie/exploration sans promettre un talent.
  Prévoir au moins une ressource ou capacité magique **utilisable**
  avant de déclarer le kit spécifique au Mage.

### Prêtre — Pèlerin (`Revise`)

- Dague + torche, `Skill_Religion` = **Recommended** mais sans objet
  de soin ou religieux distinctif.
- Aucune branche associée justifiée. Les branches réellement
  existantes sont Restoration / Protection / Exorcism.
- Proposition : objet symbolique ou équipement de soin **avec
  comportement de jeu réel** ; vérifier si une compétence ou un sort
  de soin est disponible au niveau 1 avant de proposer un consommable.

### Prêtre — Protecteur (`RetainConditional`)

- Dague + bouclier ; `Skill_Shield` = **Useful**, pas Recommended.
- `Talent_Priest_Protection_Blessing` au niveau 2 est **un sort de
  bénédiction** avec coût en mana, **ne requérant pas de bouclier**.
  Le bouclier renforce une thématique défensive mais n'est pas
  le prérequis de la branche.
- Proposition : conserver comme variante défensive, avec
  mention de son affinité Useful et sans fausse contrainte de port.

### Alchimiste — Apothicaire (`Revise`)

- Dague + fiole vide ; `Skill_Alchemy` = **Recommended**.
- `Talent_Alchemist_Apothecary_EnhancedPotion` niveau 2 est
  **passif**, améliorant les potions positives effectivement
  **utilisées** ; il n'améliore ni une fiole vide, ni une bombe.
- Proposition : ne plus présenter la fiole seule comme équipement
  suffisant. Tester la disponibilité d'une potion positive et
  de composants/recettes accessibles au niveau 1 ; sinon prévoir
  la création d'un objet approprié, sans débloquer `EnhancedPotion`
  prématurément.

### Alchimiste — Artificier (`RetainConditional`)

- Dague + bombe de feu ; `Skill_Alchemy` = **Recommended**.
- `Talent_Alchemist_Grenadier_FireBomb` au niveau 2 :
  **débloque une recette et une action** exigeant un item consommable.
- Point à vérifier impérativement : le don d'une `Item_Bomb_Fire`
  au niveau 1 ne doit pas sous-entendre que l'action `FireBomb` ou
  sa recette sont disponibles **avant** le talent.
- Proposition : conserver sous réserve de test en runtime
  (`NewGame`, action disponible, consommation de l'objet, recharge
  des stocks, sauvegarde).

## Plan ordonné pour les équipements manquants

| Priorité | Travail suivant | Condition d'acceptation |
|---|---|---|
| P0 | **Vérifier niveau 1** : arc court + munitions, bombe de feu et consommation, fiole vide et interaction | Pas d'objet offert mais inutilisable *sans explication* ; aucun talent prématuré |
| P1 | Mage Érudit / Vagabond : objets magiques réellement opérationnels | Liés à une mécanique existante, asset et visuel disponibles |
| P1 | Prêtre Pèlerin : objet sacré ou consommable de soin autorisé par les règles | Effet et obtention démontrés, aucune compétence fictive |
| P1 | Apothicaire : potion utilisable ou composants/recette accessibles | Bénéfice de `EnhancedPotion` vérifié seulement après niveau 2 |
| P1 | Guerrier Escarmoucheur : équipement ou rôle de rechange cohérent | Pas de fausse affinité Throwing, pas de quatrième branche inventée |
| P2 | Guerrier Défenseur / Voleur Ombre / Rôdeur Pisteur / Prêtre Protecteur | Meilleur alignement contenu/identité, sans imposer talent ni restriction |
| P2 | Révision des visuels, icônes et noms localisés `ST_UI` après approbation | Écran Classe + Résumé cohérents et sauvegarde intacte |

## Architecture et garde-fous

- La matrice `ARMEMENT01_9_5_KIT_DECISIONS.csv` appartient à la **documentation de conception**, pas à `Content/` ni au runtime.
- Aucune ligne ne doit devenir un prérequis dans
  `FRPGStartingEquipmentService`, `URPGClassAsset`,
  `UGridPartyInventoryComponent` ou le wizard.
- Tous les statuts restent `PendingApproval` tant que les décisions
  de game design ne sont pas ratifiées.
- Les recommandations futures dans l'interface joueur devront être
  des `FText` provenant de `Localization/Source/UI.csv` / `ST_UI` ;
  **aucun libellé français nouveau en dur dans le C++ runtime**.
- Le système d'affinité du catalogue `DA_RPGSkillPresentation`
  reste la source de vérité, sans copier les affinités en dur dans
  les DataAssets des kits.
- Un changement des kits ou des branches existantes fera échouer
  la validation de la matrice jusqu'à ce que la revue soit actualisée.

## Validation ciblée UE5.5.4

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
  -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.DesignReview"
```

Trois tests attendus :
1. `TwelveKitsAndDisposition` : comparaison des douze kits et affinités
   au rapport productif de ARMEMENT01.9.4 ;
2. `RealLevel2TalentChoices` : chaque choix de talent est réellement
   authoré dans la bonne classe/branche et est accessible au niveau 2 ;
3. `NoPrematureApproval` : aucune recommandation de design n'est
   accidentellement présentée comme approuvée ou comme un verrou gameplay.

**Aucune campagne globale Grimrock demandée** ; ne déclarer ces
trois tests réussis qu'à réception du journal TD04.2 utilisateur.

## Suite : ARMEMENT01.9.6

La prochaine étape devrait être **validation fonctionnelle des objets
réellement utilisables au niveau 1** et plan d'authoring des équipements
manquants, en commençant par l'arc, les munitions, l'utilisation de la
bombe et la disponibilité des consommables de l'Apothicaire.

La mise à jour des douze kits de production et la présentation des conseils
dans le wizard sont **hors périmètre de 01.9.5**, car elles doivent suivre
l'approbation des orientations et l'existence des nouveaux objets.
