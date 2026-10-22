# ARMEMENT01.9.7 — Approbation des 18 recettes et traitement des affinités qualifiées

**Date : 22 octobre 2026.**

**Décision de conception : les 18 recettes N1 sont `ApprovedDesign` pour la
préparation des assets, et seulement pour celle-ci.** L'activation du wizard,
les recettes réellement attribuées par `FRPGStartingEquipmentPresetCatalog`,
les SaveGame, les objets, les effets et l'autorisation d'équiper sont
**inchangés**. La compilation UE5.5.4 et les tests des changements de ce
ticket devront être confirmés par le journal de validation local.

## Périmètre d'une approbation

Les **six** classes conservent **trois** orientations, une par branche
réellement présente dans leurs `DA_Class_*`. Tous les profils
d'armement visés sont **ItemLevel=1** dans le catalogue Excel.
L'orientation ne confère aucun talent à la création :
les premiers choix de talent ne sont disponibles qu'à partir du niveau 2.

- `ApprovedDesign` = composition de base retenue pour réaliser ses
  `UGridItemDefinitionAsset`, icônes, meshes, actions et compléments ;
- `ProductionReadiness` reste **distincte** : les assets et comportements
  non validés y restent explicitement absents ou en attente ;
- **pas** de `ProductionReady`, `RuntimeEnabled`, `TalentGranted` ni
  `EquipRestricted` impliqués par cette approbation ;
- l'onglet `Classes` et les colonnes `Catalogue!P/Q` du classeur
  `ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx`
  sont les **autorités éditoriales** pour les affinités ;
- `DA_WeaponCatalogue` matérialise les données du XLSX, il ne définit
  **aucune nouvelle affinité parallèle**.

Le registre **unique** des futures recettes est :
`docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`
(18 lignes, `ApprovalState=ApprovedDesign`).

## Décision pour chaque kit

| Classe | Branche | ID de recette | Armes N1 et emplacement | Décision / garde à respecter pour authoring |
|---|---|---|---|---|
| Guerrier | Guardian | `Kit_Warrior_Guardian` | `ARM014` MainHand, `ARM075` OffHand | Approuvé ; le bouclier n'est pas un prérequis de la posture défensive niveau 2 |
| Guerrier | Breaker | `Kit_Warrior_Breaker` | `ARM031` MainHand (2 mains) | Approuvé ; ne confère pas le talent d'attaque du Briseur |
| Guerrier | WeaponMaster | `Kit_Warrior_WeaponMaster` | `ARM015` MainHand, `ARM025` Bag | Approuvé ; polyvalence martiale sans double équipement implicite |
| Voleur | Assassin | `Kit_Rogue_Assassin` | `ARM002` MainHand, `ARM054` Bag | Approuvé ; aucun talent Assassin n'est offert au niveau 1 |
| Voleur | Shadow | `Kit_Rogue_Shadow` | `ARM001` MainHand, `ARM013` OffHand | Approuvé ; contrôle de l'action de parade/main secondaire requis |
| Voleur | Saboteur | `Kit_Rogue_Saboteur` | `ARM003` MainHand | Approuvé ; outil de désamorçage **à authorer** et usage N1 à tester |
| Rôdeur | Marksman | `Kit_Ranger_Marksman` | `ARM045` MainHand (2 mains), `ARM001` Bag | Approuvé ; alimentation en munitions et tir N1 à démontrer |
| Rôdeur | Hunter | `Kit_Ranger_Hunter` | `ARM038` MainHand (2 mains), `ARM057` Bag | Approuvé ; lance de classe secondaire, javelot principal ; jet et récupération à tester |
| Rôdeur | Scout | `Kit_Ranger_Scout` | `ARM048` MainHand (2 mains), `ARM001` Bag, torche ×1 | Approuvé ; usage du tir et de la lumière à tester, pas de Vigilance accordée |
| Mage | Evoker | `Kit_Mage_Evoker` | `ARM062` MainHand | Approuvé ; baguette de feu et action élémentaire N1 à authorer sans attribution du talent |
| Mage | Arcanist | `Kit_Mage_Arcanist` | `ARM061` MainHand, `ARM070` OffHand | Approuvé ; orbe **passif** non attaquant, effet de focalisation à implémenter/valider |
| Mage | SurfaceWeaver | `Kit_Mage_SurfaceWeaver` | `ARM067` MainHand (2 mains) | Approuvé ; catalyseur **à authorer**, aucune création de surface supposée débloquée |
| Prêtre | Restoration | `Kit_Priest_Restoration` | `ARM069` MainHand | Approuvé ; soin consommable **à authorer**, aucun bonus de soin de talent à N1 |
| Prêtre | Protection | `Kit_Priest_Protection` | `ARM020` MainHand, `ARM075` OffHand | Approuvé ; bouclier de classe secondaire, pas requis par Bénédiction N2 |
| Prêtre | Exorcism | `Kit_Priest_Exorcism` | `ARM069` MainHand, `ARM081` OffHand | Approuvé ; égide passive (5 armure physique / 3 magique au profil initial à vérifier), pas de sort d'Exorcisme anticipé |
| Alchimiste | Grenadier | `Kit_Alchemist_Grenadier` | **`ARM001` MainHand, `ARM058` Bag**, bombe ×1 | **Composition corrigée et approuvée** : dard jamais supposé utilisable en MainHand, bombe et lancer N1 à tester |
| Alchimiste | Apothecary | `Kit_Alchemist_Apothecary` | `ARM001` MainHand | Approuvé ; potion positive/composants **à authorer**, pas de `EnhancedPotion` au niveau 1 |
| Alchimiste | Transmuter | `Kit_Alchemist_Transmuter` | `ARM001` MainHand, `ARM070` OffHand | Approuvé ; orbe passif et composants de transmutation **à authorer/valider** |

**Modification d'une seule composition** depuis ARMEMENT01.9.6 :
`Kit_Alchemist_Grenadier` remplace `ARM058` en `MainHand` par
`ARM001` en `MainHand`, et place `ARM058` en `Bag`.
La bombe `Item_Bomb_Fire` ×1 est conservée. Le total des **22
archétypes distincts** utilisés par les 18 kits reste inchangé.

La liste de 18 recettes est approuvée **pour définir ce qu'il faut fabriquer**.
Les cinq compléments fonctionnels sans ID final restent :
désamorçage, catalyseur de surfaces, soin, potion/composants de l'Apothicaire
et composants de transmutation. Ils ne doivent **pas** être remplacés par
des effets fictifs ou des consommables sans usage.

## Résolution conservatrice des ambiguïtés Excel

### Problème source — non inventé

Le XLSX contient des champs `Classes secondaires` qualifiés par exemple
`Prêtre (marteaux)` sur les profils d'armes lourdes
`ARM027` à `ARM037` (dont des grandes épées, une grande hache, une
faux et de véritables marteaux).

Dans ARMEMENT01.9.5.2, le `SecondarySourceLabel` conserve
fidèlement le texte original. En revanche, `GetClassAffinity` expose
`Secondary` pour `Priest` sur ces lignes sans distinguer les
conditions entre parenthèses.

**Il serait incorrect d'affirmer qu'une grande épée ou une grande hache
est recommandée au Prêtre au motif « marteaux ».**
Inversement, il serait incorrect de déduire automatiquement du seul
texte qu'une `Masse lourde` satisfait toujours « marteaux ».
Le sens précis des exceptions doit rester celui que le concepteur
clarifiera **dans le XLSX**, sans inventer une matrice concurrente.

### Décision technique effective dans ce ticket

Nouvelle API **consultative** :

`UGridWeaponCatalogueAsset::GetKitRecommendationAffinity(ArchetypeId, ClassId)`

Valeurs :
- `Primary` : classe principale sans réserve ;
- `Secondary` : classe secondaire citée **sans qualificatif propre** ;
- `Unspecified` : aucune préférence de classe connue ;
- `NeedsReview` : secondarité **qualifiée** ou désaccord avec le texte
  source ; non approuvée automatiquement pour un kit.

Cette API analyse **chaque fragment** de `SecondarySourceLabel`
séparé par « / ». Un `Prêtre (marteaux) / Rôdeur` rend donc
**`Priest = NeedsReview` et `Ranger = Secondary`**, au lieu de
bloquer la seconde classe à cause du qualificatif de la première.
Les fragments incohérents sont eux aussi `NeedsReview` (défaut sûr).

L'API historique `GetClassAffinity` continue volontairement à
retourner ses trois valeurs antérieures pour préserver sa compatibilité.
**Tout outil futur approuvant automatiquement un kit DOIT consulter
`GetKitRecommendationAffinity`, pas la fonction historique.**
Aucune fonction `CanEquip`, règle de combat, donnée de sauvegarde,
affinité d'objet ou package `.uasset` n'est modifié.

### Résultat pour les 18 kits approuvés

Les armes primaires et secondaires de **ces 18 compositions** sont
vérifiées en test à l'aide de `GetKitRecommendationAffinity`.
Aucune des paires `(ArchetypeId, ClassId)` utilisées dans les
recettes approuvées ne doit retourner `NeedsReview` ou `Unspecified`.
Les armes lourdes Guerrier restent primaires même si leur
`SecondarySourceLabel` comporte `Prêtre (marteaux)` :
la réserve ne concerne **que le Prêtre**.

**Correction éditoriale différée** : le XLSX reste inchangé. Lors de
sa prochaine révision, clarifier explicitement les lignes concernées
(p. ex. choisir quelles armes le Prêtre peut considérer secondaires,
sans créer d'interdiction). Relancer ensuite l'export existant,
l'import idempotent des affinités et les tests de cette API.
Cette question de la cohérence des libellés du XLSX **n'empêche pas**
les 18 recettes actuelles, car aucun kit Prêtre ne référence une arme
lourde soumise à cette réserve.

## Vérification et preuve à produire localement

Tests nouveaux :

```text
Grimrock.RPG.ARMEMENT01.9.KitApproval.QualifiedPriestAffinities
Grimrock.RPG.ARMEMENT01.9.KitApproval.IndependentSourceQualifiers
Grimrock.RPG.ARMEMENT01.9.KitApproval.NoGameplayOrCatalogueMutation
```

Les trois tests existants sous
`Grimrock.RPG.ARMEMENT01.9.KitDesign` sont adaptés à
`ApprovedDesign`, à la nouvelle composition du Grenadier et à la
fonction conservatrice, pour la **totalité des 18 recettes**.

Depuis `master` après compilation Development Editor :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
  -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.Kit"

```

**Attendu : 6/6 tests, zéro avertissement** (3 KitDesign +
3 KitApproval). La nouvelle campagne de tests doit être
confirmée par le **journal local utilisateur** : les 3/3 réussites
du jalon précédent ne couvrent pas les modifications de ce ticket.

## Après approbation : ordre d'exécution

1. **ARMEMENT01.10** : authorer en lots atomiques les **22 archétypes
   N1** réellement nécessaires (trois pilotes déjà existants, dix-neuf
   à vérifier et produire), en respectant les profils Excel.
2. Créer et valider en runtime les **cinq types de compléments
   fonctionnels** signalés ci-dessus et contrôler munitions,
   consommables, slots passifs et actions des focaliseurs.
3. Produire/valider icônes et meshes partagés, sans créer
   1640 modèles 3D par erreur.
4. **Seulement ensuite**, activer les 18 kits dans le wizard avec
   textes `UI.csv → ST_UI`, tests de génération, sélections
   invalides et compatibilité des anciennes sauvegardes.

La documentation des 12 kits legacy et les tests des jalons 01.9.2–5
sont préservés. Aucun changement dans les données live avant la
complétude de la production.
