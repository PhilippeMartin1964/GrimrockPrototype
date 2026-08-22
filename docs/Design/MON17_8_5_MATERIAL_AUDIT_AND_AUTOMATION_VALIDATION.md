# MON17.8.5 — Material Audit & Automation Validation

Statut : **AUTOMATION VALIDÉE 6/6 — AUTHORING MATÉRIAUX / PIE DISSOLVE À FAIRE**

## 1. Validation automatisée UE5.5.4

Filtre exécuté :

```text
Grimrock.Monsters.MON17.8
```

Résultat fourni localement sous UE5.5.4 :

```text
AnimationStateBridgeContract       Success
BestiaryPresentationBridge         Success
DeathDefinitionContract            Success
DeathDissolveApiContract           Success
DeathDissolveDefinitionContract    Success
DeathPresentationApiContract       Success
```

Soit :

```text
6 / 6 Success
0 échec
```

Cette exécution valide les contrats C++ MON17.8 actuellement présents, notamment le contrat data-driven de dissolution, l'API du `UGridMonsterDeathComponent` et l'absence de Tick permanent.

Elle ne remplace pas la validation visuelle PIE du dissolve, qui dépend encore de l'authoring des matériaux.

## 2. Audit réel des matériaux GoblinThrower

Les trois matériaux du GoblinThrower ont été inspectés directement dans UE5.5.4.

### M_Goblin_Bomber

```text
Blend Mode       = Opaque
Shading Model    = Default Lit
Opacity          = non utilisé
Opacity Mask     = non utilisé
```

Conséquence : pour permettre un dissolve par clipping tout en conservant un pipeline simple et cohérent avec les autres matériaux, ce matériau devra passer en `Masked` et recevoir le masque de dissolution sur `Opacity Mask`.

### M_Cloth_Bomber

```text
Blend Mode       = Masked
Shading Model    = Default Lit
Opacity          = non utilisé
Opacity Mask     = utilisé
```

Conséquence : conserver le masque existant et le multiplier par le masque de dissolution.

### M_Hair_Bomber

```text
Blend Mode       = Masked
Shading Model    = Hair
Opacity          = non utilisé
Opacity Mask     = utilisé
```

Conséquence : conserver le masque cheveux existant et le multiplier par le masque de dissolution. Le Shading Model `Hair` ne doit pas être modifié.

## 3. Contrat matériau commun

Le C++ MON17.8.5 pilote exclusivement le paramètre scalaire :

```text
DissolveAmount
0.0 = entièrement visible
1.0 = entièrement dissous
```

Le même sens doit être conservé dans les trois matériaux.

Le masque logique attendu est :

```text
DissolveMask = Noise >= DissolveAmount ? 1 : 0
```

Puis :

```text
M_Goblin_Bomber:
OpacityMask = DissolveMask

M_Cloth_Bomber:
OpacityMask = ExistingOpacityMask * DissolveMask

M_Hair_Bomber:
OpacityMask = ExistingOpacityMask * DissolveMask
```

Le bruit doit être spatialement stable : il ne doit pas dépendre du temps. Le corpse dissolve est une révélation progressive d'un motif fixe, pas une texture animée.

## 4. Règles de non-régression matériaux

L'authoring UE doit préserver :

- Base Color / Normal / Roughness / Metallic actuels ;
- le Shading Model `Default Lit` du corps et du tissu ;
- le Shading Model `Hair` des cheveux ;
- le masque alpha existant du tissu ;
- le masque alpha existant des cheveux ;
- le comportement normal lorsque `DissolveAmount = 0`.

Pour `M_Goblin_Bomber`, le seul changement structurel nécessaire est `Opaque -> Masked` afin de rendre `Opacity Mask` disponible.

## 5. Configuration GoblinThrower à tester ensuite

Après authoring des trois matériaux :

```text
DA_MON_GoblinThrower
bEnableDeathDissolve       = true
DeathDissolveDelay         = 2.0
DeathDissolveDuration      = 1.5
DeathDissolveParameterName = DissolveAmount
```

Avec la mort validée MON17.8.4 :

```text
DeathExpectedDuration = 3.6333333 s
```

la chronologie initiale attendue reste :

```text
t=0.000  début de mort
t=3.633  pose finale
+t=2.000 corpse hold
t=5.633  début dissolve
+t=1.500 dissolve
t≈7.133  SkeletalMesh caché
```

## 6. Validation PIE suivante

Après authoring :

```text
1. le DeathMontage reste inchangé ;
2. la pose finale reste visible pendant le délai ;
3. corps, tissu et cheveux se dissolvent ensemble ;
4. aucun slot matériau ne reste opaque ;
5. le mesh est caché à la fin ;
6. l'Actor mort reste présent ;
7. loot / XP / victoire / encounter restent inchangés ;
8. RatGiant reste inchangé tant que son dissolve est désactivé.
```
