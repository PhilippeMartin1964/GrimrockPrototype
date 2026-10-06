# RPG03.9.4F2 — branche Mage Tisseur de surfaces complète

Statut : **VALIDÉ / MATÉRIALISÉ — Mage 15/15, couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Objectif

F2 authorise les cinq talents Tisseur de surfaces sur le socle générique validé par F1 :

- Imprégnation ;
- Conversion élémentaire ;
- Conduction ;
- Surface persistante ;
- Architecte du terrain.

Après F2, le Mage possède ses 15 talents conceptuels complets.

## Affinité de branche et Imprégnation

Les règles mécaniques utilisent une « affinité » pour Imprégnation, Conversion et Architecte, mais l'unique affinité existante avant F2 est le talent **Évocateur** de niveau 2. Un Tisseur pur ne peut pas l'acheter sans abandonner un palier de sa propre chaîne.

F2 résout cette incohérence sans ajouter de point de talent ni de nouveau palier : le talent logique `Talent_Mage_SurfaceWeaver_Imbuement` est matérialisé par quatre ChoiceIds exclusifs de niveau 2 :

- `Talent_Mage_SurfaceWeaver_Imbuement_Fire` ;
- `Talent_Mage_SurfaceWeaver_Imbuement_Frost` ;
- `Talent_Mage_SurfaceWeaver_Imbuement_Air` ;
- `Talent_Mage_SurfaceWeaver_Imbuement_Earth`.

Ils appartiennent à `TalentGroup_Mage_SurfaceWeaver_ImbuementAffinity`, coûtent chacun 1 point et accordent tous l'alias logique `Talent_Mage_SurfaceWeaver_Imbuement`.

Le niveau 6 exige cet alias : il n'est donc pas nécessaire d'acheter le talent Évocateur.

`Action_Mage_Imbuement` :

- SourcePolicy = Spell ;
- 1 PA / 4 mana ;
- Self/Ally R3 via le ciblage Ally existant ;
- CD1 ;
- applique `Status_ElementalImbuement` pour 2 rounds.

`Status_ElementalImbuement` est unique et possède quatre réactions conditionnées par l'affinité du **Mage source** :

- Fire -> 3 + INT mod dégâts Fire ;
- Frost -> 3 + INT mod dégâts Ice ;
- Air -> 3 + INT mod dégâts Lightning ;
- Earth -> 3 + INT mod dégâts Physical.

La réaction exige une véritable attaque d'arme, utilise l'INT du Mage identifié par le `SourceId` du Status, puis consomme uniquement l'instance de Status correspondante.

Le Status est tagué `Dispel.Magical`.

## Conversion élémentaire

`Action_Mage_ElementalConversion` :

- 2 PA / 5 mana ;
- Area1 R4 ;
- CD2 ;
- un seul ActionId avec quatre owner variants.

Table authorée :

- Fire : Oil/Poison -> Fire ;
- Frost : Water -> Ice ;
- Air : Water/Blood -> ElectrifiedWater ;
- Earth :
  - Water -> Poison ;
  - cellule neutre -> Oil.

Une conversion invalide reste un no-op.

## Conduction

`Talent_Mage_SurfaceWeaver_Conduction` authorise des modifiers +20 % conditionnés par le DamageType et l'environnement de la cible.

Compatibilités authorées :

- Fire : Oil, Poison, Burning/Elemental.Fire ;
- Ice : Water, Slow/Control.Slow ;
- Lightning : Water, Blood, ElectrifiedWater, Stunned/Control.Stun ;
- Earth Physical/Poison : School Earth et Water/Oil/Poison/Immobilized.

Les contrôles d'une attaque restent gérés par ses `StatusApplications` et leur `ArmorGate` normal après résolution des dégâts.

## Surface persistante

`Talent_Mage_SurfaceWeaver_PersistentSurface` projette sur les actions Spell du Mage :

- SurfaceDurationRoundsModifier = +2 ;
- SurfacePeriodicDamagePercentModifier = +15.

Ces modifiers sont snapshotés uniquement lorsqu'une surface est créée ou convertie par l'action du Mage. Les surfaces du décor ou d'un autre auteur ne sont pas mutées.

Le cap C6 à 6 rounds reste l'autorité runtime.

## Architecte du terrain

`Action_Mage_TerrainArchitect` :

- 4 PA / 12 mana ;
- Area2 R5 ;
- CD5 ;
- un seul ActionId avec quatre owner variants ;
- surface de base 3 rounds avant modifiers.

Surfaces :

- Fire -> Fire ;
- Frost -> Ice ;
- Air -> ElectrifiedWater ;
- Earth -> Oil.

### Décision Earth Oil/Poison

Les règles indiquent `Oil/Poison` mais la progression ne contient aucun sous-choix Earth pour Architecte du terrain.

F2 retient **Oil** comme représentation déterministe du terrain Earth créé depuis une cellule neutre, cohérente avec la conversion Earth `neutral -> Oil` de la variante `Imbuement_Earth`.

Poison reste disponible à Earth via `Water -> Poison` dans Conversion élémentaire. Aucun sous-ChoiceId Earth supplémentaire n'est inventé pour Architecte du terrain.

## Résultat Mage après F2

Authoring attendu :

- 9 actions actives :
  - 3 Évocateur ;
  - 3 Arcaniste ;
  - 3 Tisseur de surfaces ;
- 21 Choice records :
  - 8 Évocateur, l'Affinité occupant quatre variantes ;
  - 5 Arcaniste ;
  - 8 Tisseur de surfaces, Imprégnation occupant quatre variantes pour un seul palier logique.

Le Mage reste à **15 talents conceptuels** et chaque chemin complet de branche consomme toujours exactement 5 Talent Points.

## Assets

Le commandlet Mage met à jour :

- `DA_Class_Mage` ;
- `DA_Status_ElementalOverload` ;
- `DA_Status_ElementalImbuement` ;
- `DA_Status_Burning` ;
- `DA_Status_Slow`.

`DA_Status_ElementalImbuement` est le seul nouvel asset attendu.

## Validation

Depuis un working tree propre sur `master` :

```powershell
.\Scripts\AuthorRPGMage.ps1 -EngineRoot D:\UE_5.5
```

Le script compile l'Editor, lance `RPGMageAuthoring`, exécute `Grimrock.RPG.RPG03.9.4F2`, puis affiche les changements binaires.

Validation finale de référence : `Grimrock.RPG.RPG03` — 193/193, 0 warning, 0 échec, exit 0 (6 octobre 2026).
