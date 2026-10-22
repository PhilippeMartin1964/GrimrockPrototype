# ARMEMENT01.9.6 — Définition des 18 kits N1

**22 octobre 2026 — statut : VALIDÉ TECHNIQUEMENT 3/3, 0 avertissement, 0 échec, code 0** (`TD04-20261022-122937`, filtre `Grimrock.RPG.ARMEMENT01.9.KitDesign`). **Les 18 recettes sont désormais approuvées pour l'authoring** (`ApprovedDesign`) par le jalon [ARMEMENT01.9.7](ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md), **sans être activées en runtime** ; les contraintes d'actions, slots et équipement restent à valider. La référence de nettoyage et de transition est [ARMEMENT01.9.6.1](ARMEMENT01_9_6_1_CLEANUP_CONSOLIDATION.md).

## Intention et hiérarchie des sources

Le kit N1 est **une orientation de départ**, jamais une attribution de talent,
une classe d'équipement verrouillée ou une obligation de progresser dans la branche.
Les premiers talents de chaque branche sont offerts **à partir du niveau 2**.
Le joueur pourra changer d'orientation, équiper d'autres armes et mélanger ses talents.

Sources d'autorité, dans cet ordre :
1. Classeur **existant** `ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx` :
   onglet **Classes** (priorités), **Catalogue** (classe principale / secondaires),
   **Niveaux 1-20** (profils immuables N1).
2. `DA_WeaponCatalogue` matérialisé en ARMEMENT01.9.5.2 :
   1640 profils, 82 `ClassAffinities`, six `ClassPreferences`.
   **Les affinités ne sont pas recopiées manuellement** dans les recettes :
   les tests consultent `GetClassAffinity` directement.
3. Les `DA_Class_*` de production (18 branches réelles et choix N2).
4. Les `UGridItemDefinitionAsset` créés et testés, leurs emplacements, actions,
   icônes, meshes et sauvegardes.

Manifest **de conception uniquement** :
`docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`.

Chaque ligne est une **recette de conception approuvée** pour la production d'assets (`ApprovedDesign`), mais pas un kit N1 déjà jouable.
L'item `ARMxxx` désigne un **archétype de la source Excel**, pas un
`UGridItemDefinitionAsset` déjà matérialisé dans le Content Browser.
N1 signifie **ItemLevel=1**, non une limitation à une certaine branche de talents.

## Les 18 recettes approuvées pour la production d'assets

| Classe et orientation | Arme principale N1 | Arme / équipement secondaire N1 | Complément | Justification / prudence |
|---|---|---|---|---|
| **Guerrier — Gardien** | `ARM014` Épée droite (main) | `ARM075` Bouclier rond (secondaire) | — | Épée + écu de l'onglet Classes ; le bouclier **n'est pas** un prérequis du talent Posture défensive N2 |
| **Guerrier — Briseur** | `ARM031` Grande hache (deux mains) | — | — | Famille deux mains et dégâts N1 (1d6+7, 3 PA) déjà définis, **aucun** Coup puissant accordé à N1 |
| **Guerrier — Maître d'armes** | `ARM015` Épée longue (main) | `ARM025` Pic de guerre (sac) | — | Deux armes différentes pour varier le type de dégâts ; ne pas les équiper simultanément sans action spécifique |
| **Voleur — Assassin** | `ARM002` Poignard d'assassin (main) | `ARM054` Couteau de jet (sac) | — | Arme légère prioritaire Voleur, couteau de jet secondaire ; attaque sournoise N2 non accordée |
| **Voleur — Ombre** | `ARM001` Dague (main) | `ARM013` Main-gauche (secondaire) | — | Style armes légères/parade ; valider mécanique d'offhand, sans promettre une esquive liée à l'arme |
| **Voleur — Saboteur** | `ARM003` Stylet (main) | — | **Outil de désamorçage à authorer** | Ne pas inventer le déblocage d'Expert Disarm avant N2 |
| **Rôdeur — Tireur** | `ARM045` Arc court (deux mains) | `ARM001` Dague (sac) | **Munitions à vérifier** | Les deux pilotes existent déjà ; la portée et les flèches doivent fonctionner sans talent N2 |
| **Rôdeur — Chasseur** | `ARM038` Lance courte (deux mains) | `ARM057` Javelot (sac) | — | Lance classe secondaire et javelot classe principale ; tir, récupération et portée à valider |
| **Rôdeur — Éclaireur** | `ARM048` Arc recourbé (deux mains) | `ARM001` Dague (sac) | `Item_Torch` ×1 | Exploration ; la Vigilance N2 est un passif, pas une condition d'utilisation de la torche |
| **Mage — Évocateur** | `ARM062` Baguette de feu (main) | — | — | Une préférence élémentaire initiale **non exclusive** ; le talent Affinité élémentaire reste à choisir à N2 |
| **Mage — Arcaniste** | `ARM061` Baguette arcanique (main) | `ARM070` Orbe arcanique (secondaire) | — | Identité magique forte ; vérifier le support de l'orbe passif et des actions de la baguette |
| **Mage — Tisseur de surfaces** | `ARM067` Bâton élémentaire (deux mains) | — | **Catalyseur de surfaces à authorer** | Le bâton ne doit pas promettre des surfaces déverrouillées à N1 |
| **Prêtre — Restauration** | `ARM069` Sceptre sacré (main) | — | **Potion/consommable de soin à authorer** | Le sceau sacré ne confère pas le talent de soin amélioré avant N2 |
| **Prêtre — Protection** | `ARM020` Masse d'armes (main) | `ARM075` Bouclier rond (secondaire) | — | Masse et bouclier conformes au classeur ; Bénédiction N2 ne requiert pas de bouclier |
| **Prêtre — Exorcisme** | `ARM069` Sceptre sacré (main) | `ARM081` Égide sacrée (secondaire) | — | Thématique sacrée ; ne donne pas Lumière sacrée au niveau 1 |
| **Alchimiste — Grenadier** | `ARM001` Dague (main) | `ARM058` Dard (sac) | `Item_Bomb_Fire` ×1 | Dague secondaire Alchimiste utilisable sans hypothèse sur le slot du dard ; **bombe présente ≠ talent/recette Grenadier débloqué** |
| **Alchimiste — Apothicaire** | `ARM001` Dague (main) | — | **Potion positive ou composants réellement utilisables à authorer** | `EnhancedPotion` agit sur des potions utilisées après N2, pas sur une fiole vide |
| **Alchimiste — Transmutateur** | `ARM001` Dague (main) | `ARM070` Orbe arcanique (secondaire) | **Composants d'huile/transmutation à authorer** | Affinité de classe secondaire ; vérifier que l'orbe peut être porté/utilisé et que l'alchimie N1 existe |

**Important :** `ARM058` est une arme de jet à une main dans le classeur, mais son usage comme arme tenue n'est pas prouvé. **ARMEMENT01.9.7 place le dard dans le sac** et la dague `ARM001` dans la main principale ; le lancer et la récupération restent à tester.
De même, `ARM070` et `ARM081` sont des profils passifs :
aucune attaque, aucun bonus spécial ou statut ne doit leur être inventé.

Les compositions ont été choisies **sur le classeur**, en vérifiant
l'affinité par classe dans `DA_WeaponCatalogue` au moment des tests.
Les compétences demeurent un deuxième axe de revue ; elles ne remplacent
pas les préférences d'armes de l'Excel.

## Mesure de périmètre et disponibilité réelle

- **18 kits** : trois par classe, une recette par branche de talent.
- **22 archétypes distincts du catalogue** apparaissent dans les recettes,
  sur les 82 du futur catalogue d'objets. C'est la **première vague d'assets**
  à prioriser, pas l'intégralité du travail ARMEMENT01.
- Trois archétypes sont déjà matérialisés comme pilotes :
  `ARM001` dague, `ARM045` arc court, `ARM075` bouclier rond.
  Les **19 autres** doivent être vérifiés/authorés avant de déclarer les
  recettes immédiatement jouables (pas de création automatique de 19 assets ici).
- Deux supports non armement existants sont explicitement référencés :
  `Item_Torch` et `Item_Bomb_Fire`.
- Cinq kits nécessitent encore un **complément fonctionnel** sans
  `ItemDefinitionId` inventé : Saboteur, Tisseur de surfaces,
  Restauration, Apothicaire, Transmutateur.
- Les autres risques : arcs et munitions, jet/récupération, armes lourdes,
  slots passifs, focaliseurs, actions et effets, progression, consommables.
- La **quantité d'objets par kit n'est pas fixée à deux** : le service
  transactionnel devra pouvoir en distribuer davantage selon le kit final.

### Identités et rétrocompatibilité

Les nouveaux identifiants de conception sont **distincts** :
`Kit_<ClassId>_<BranchId>`, par exemple
`Kit_Warrior_Guardian` ou `Kit_Mage_SurfaceWeaver`.

Les 12 anciens `StartingEquipmentKitId` restent inchangés dans
`FRPGStartingEquipmentPresetCatalog` et les SaveGame existants. Lors
de l'intégration finale, il faudra décider **explicitement** :
compatibilité héritée, affichage des seuls 18 kits aux nouvelles parties,
mappage/migration éventuel et comportement des sauvegardes anciennes.

Les identifiants proposés ne sont **pas** injectés aujourd'hui dans
les DataAssets de classes, ni visibles dans le wizard. Leurs futurs noms,
résumés et textes de conseil seront localisés via
`Localization/Source/UI.csv → ST_UI`, **sans texte français en dur dans le C++ runtime**.

## Vérification automatisée (sans modifier le gameplay)

Trois tests sous `Grimrock.RPG.ARMEMENT01.9.KitDesign` :
1. `EighteenRealTalentBranches` : 6 × 3, IDs distincts,
   une branche réelle de talent N2 par recette ;
2. `RealN1WeaponsAndAdvisoryAffinity` : archetypes et profils N1 réels
   dans `DA_WeaponCatalogue`, arme principale au moins `Primary`
   ou `Secondary`, tenue 1/2 mains et cohérence offhand,
   **22 archétypes distincts** ;
3. `NoLegacyKitOrSaveMutation` : les nouveaux IDs
   `Kit_*` n'ont encore aucune recette runtime et
   les supports à authorer ne sont pas distribués prématurément.

Lancer après synchronisation de `master` :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
  -EngineRoot D:\UE_5.5 `
  -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.KitDesign"
```

**Résultat local confirmé par le journal du 22.10.2026 : 3/3 tests, 0 avertissement, 0 échec, exit code 0.** La commande est conservée pour les futures régressions.
Aucun `.uasset` n'est généré ni modifié par ce jalon.
Ne pas lancer de commandlet d'authoring d'armes à ce stade.

## Jalons suivants — recommandés

1. **ARMEMENT01.9.7 — VALIDATION DE DESIGN** : 18 compositions approuvées pour l'authoring, lecture C++ conservatrice des affinités Excel qualifiées ; voir [rapport 01.9.7](ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md).
2. **ARMEMENT01.10 — Production des 22 archétypes N1 prioritaires** :
   des `UGridItemDefinitionAsset` vraiment utilisables, par lots atomiques ;
   référencer le profil N1 du même `DA_WeaponCatalogue`.
3. **ARMEMENT01.11–.12 — Assets visuels, slots, actions et tests** :
   construire les icônes/meshes et interactions en jeu, avec vérification
   particulière des focaliseurs et accessoires passifs.
4. **ARMEMENT01.13 — Activation des 18 kits dans le wizard** :
   uniquement après complétude des assets et revue des SaveGame.

Il ne faut **ni forcer le choix d'une branche**, ni ajouter une quatrième
branche, ni masquer les 12 anciens kits avant d'avoir une stratégie de
compatibilité validée.
