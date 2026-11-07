# FIREFX04 — Embrasement transitoire après l'impact du catalyseur

## Contrat

- Le gameplay C6/RPG03.6 est résolu une seule fois lors de la confirmation de l'action.
- `AGridLevelRuntimeActor::BeginCatalystImpactVfx` joue **un seul** événement visuel et sonore si au moins une case réagit en `OilIgnition` ou `PoisonBurst`.
- Les cas `Fizzle` et `IceMelt` n'émettent pas de son d'embrasement.
- Les références `FireIgnitionBurstVfx` et `FireIgnitionSound` sont facultatives : aucun chemin `/Game/FireFX` n'est codé dans le C++.
- Choisir le vrai système FireBurst présent sous `Content/F_FreeFlameFx_Pack/` dans le Content Browser. La référence souple dans `BP_GridLevelRuntimeActor` est **versionnée**, pas le Niagara du pack.
- Le son « wouf » doit être assigné à `FireIgnitionSound` dans le Blueprint, avec une atténuation spatiale configurée. Les sons créés pour Grimrock sont versionnés.
- `FireIgnitionBurstScale` commence à 1 et peut être ajusté après PIE avec le pack réel.
- Le crépitement persistant est une autre responsabilité : il accompagnera les cellules de feu dans FIREFX01 et sera nettoyé avec elles.

## Vérifications demandées à l'agent Windows

Compiler UE5.5.4 et exécuter la suite `Grimrock` sur le commit.
Aucun asset du pack tiers n'est versionné ; les références souples doivent être conservées et rester inspectables sans charger le pack.
La présence d'un Niagara `NS_FFX_FireBurst`, sa taille, son orientation, le rendu et la synchronisation audible nécessitent ensuite une recette PIE/GPU locale.

## Séquence attendue

1. Une flasque de catalyseur quitte le groupe et suit sa trajectoire balistique.
2. Son bris intervient à l'impact.
3. Si une réaction C6 produit un embrasement, un unique « wouf » et `FireBurst` optionnels se déclenchent.
4. Le feu persistant sur la surface est géré par le profil `FireVfxProfile`, indépendamment du burst.
5. Aucun burst de feu ne doit être joué lorsque le catalyseur n'enflamme rien.

La validation `-NullRHI` ne prouve pas l'apparence de l'effet Niagara ni la qualité audio.
