# RPG03.9.6B2 — Alchimiste : Transmutateur

Date : **6 octobre 2026**  
Dépendances : `RPG03.9.6A` validé 8/8, `RPG03.9.6B1` validé 7/7  
Statut : **VALIDÉ — Alchimiste 15/15 couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Résultat

L'Alchimiste atteint **15/15 talents conceptuels** :

- Grenadier 5/5 ;
- Apothicaire 5/5 ;
- Transmutateur 5/5.

Aucun `.uasset` n'est modifié.

## Transmutateur

- **Huile glissante** : `Item_Flask_Oil`, AP2 R4 Area1, Oil 4 rounds, coût de traversée +1.
- **Flasque acide** : `Item_Flask_Acid`, Hostile R4 AP2 CD1, PhysicalArmor `6 + 2×Alchemy`, jamais de spill HP, `Status_Corroded` 2 rounds.
- **Nuage corrosif** : `Item_Flask_CorrosiveCloud`, AP3 R4 Area1 CD2, impact `4 + Alchemy` Poison, PoisonCloud 3 rounds à 2 Poison/round, Poison 2 Turns sous ArmorGate magique.
- **Catalyseur** : action de classe AP1 R4 CD2, `SurfaceInteraction=AnyCanonical`.
- **Transmutation majeure** : AP4 R4 Area2 CD5, coût source 1, quatre profils de recette Fire/Ice/Poison/Oil. **D07** impose désormais une durée finale exacte de 4 rounds sur toute conversion réussie, y compris une surface préexistante. Le Choice apporte toujours +50 % aux dégâts de réaction de surface de cette ActionId (D08 reste séparé).

## Frontière recette

Le projet ne possède toujours pas de moteur de recettes.

B2 ne crée donc pas quatre faux `Item_Catalyst_Rare`. Il expose :

```text
BuildMajorTransmutationRecipeAction(OutputSurfaceType)
```

qui produit la contribution combat canonique pour une seule recette sélectionnée. Le futur système Crafting choisira exactement une sortie par utilisation tout en consommant l'unique `Item_Catalyst_Rare`.

Les quatre RequirementIds de recette sont déjà accordés par le talent :

```text
Recipe_MajorTransmutation_Fire
Recipe_MajorTransmutation_Ice
Recipe_MajorTransmutation_Poison
Recipe_MajorTransmutation_Oil
```

## Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6B2"
```


## Correctif D07 — durée finale fixe

Le profil générique de conversion possède désormais :

```text
bUseFixedFinalDuration
FixedFinalDurationRounds
```

Par défaut, rien ne change : une conversion existante conserve sa durée puis
applique `SurfaceDurationRoundsModifier`.

Transmutation majeure active explicitement :

```text
bUseFixedFinalDuration = true
FixedFinalDurationRounds = 4
```

La durée finale vaut donc exactement 4 rounds, qu'une cellule soit vide ou
qu'elle contienne déjà une surface. Le modificateur de durée de la source n'est
pas appliqué dans ce mode.


## Validation D07

Le correctif de durée finale fixe a été validé le 8 octobre 2026 :

```text
RPG03.9.6A  10/10
RPG03.9.4F1  4/4
RPG03.9.6B2  7/7
warnings      0
échecs        0
```

D07 est clos.
