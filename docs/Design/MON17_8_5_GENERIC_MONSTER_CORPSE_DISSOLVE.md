# MON17.8.5 — Generic Monster Corpse Dissolve

Statut : **C++ / CONTRAT / TESTS AJOUTÉS — compilation UE5.5.4, authoring matériaux et validation PIE requises**

## 1. Objectif

Ajouter une disparition progressive du cadavre après l'animation de mort, sans créer de logique spécifique au `GoblinThrower` et sans modifier le gameplay de mort.

Pipeline cible :

```text
mort gameplay immédiate
    -> DeathMontage
    -> pose finale maintenue
    -> corpse hold
    -> dissolve visuel
    -> SkeletalMesh hidden
    -> Actor mort conservé
```

Le GoblinThrower est le premier cas d'authoring, mais l'implémentation est commune à tous les monstres utilisant :

```text
AGridMonsterActor
UGridMonsterDefinitionAsset
UGridMonsterDeathComponent
```

## 2. Principes non négociables

La dissolution est **presentation-only**.

Elle ne doit jamais :

- déclencher la mort ;
- déterminer les dégâts ;
- attendre avant de libérer l'occupation ;
- modifier le loot ;
- modifier l'XP ;
- réémettre `MonsterDied` ;
- détruire l'Actor runtime ;
- modifier la cellule logique du monstre ;
- devenir une autorité de persistance.

Le cadavre peut disparaître visuellement, mais l'Actor mort reste disponible pour l'état runtime / SaveGame.

## 3. Nouveau contrat data-driven

`UGridMonsterDefinitionAsset` expose désormais :

```text
bEnableDeathDissolve
DeathDissolveDelay
DeathDissolveDuration
DeathDissolveParameterName
```

Valeurs par défaut :

```text
bEnableDeathDissolve       = false
DeathDissolveDelay         = 2.0 s
DeathDissolveDuration      = 1.5 s
DeathDissolveParameterName = DissolveAmount
```

Le système est volontairement **désactivé par défaut** afin qu'aucun monstre existant ne change de comportement lors de l'introduction du contrat.

### Validation

Le DataAsset rejette :

```text
DeathDissolveDelay < 0
DeathDissolveDuration <= 0
valeurs non finies
bEnableDeathDissolve=true + DeathDissolveParameterName=None
```

## 4. Intégration dans UGridMonsterDeathComponent

Le composant existant reste propriétaire du cycle de mort.

MON17.8.5 n'ajoute aucun nouveau composant et aucun Tick permanent.

Le composant s'abonne au delegate générique existant :

```text
AGridMonsterActor::OnMonsterDied
```

Lorsque la mort logique est diffusée et que la dissolution est activée, il programme :

```text
StartDelay = PresentationDuration + DeathDissolveDelay
```

avec :

```text
PresentationDuration = DeathExpectedDuration si DeathMontage est assigné
PresentationDuration = 0 sinon
```

Pour le GoblinThrower MON17.8.4 :

```text
DeathExpectedDuration = 3.6333333 s
```

Ainsi, avec les valeurs initiales proposées :

```text
t=0.000      mort + début de chute
t=3.633      pose finale atteinte
+2.000       corpse hold
t=5.633      début dissolve
+1.500       dissolve
~t=7.133     SkeletalMesh caché
```

Ces temps sont data-driven et pourront être ajustés après PIE.

## 5. Dynamic Material Instances

Au début du dissolve uniquement :

1. le composant mémorise les matériaux actuellement affectés au Skeletal Mesh ;
2. il crée un `UMaterialInstanceDynamic` pour chaque slot ;
3. il initialise le paramètre :

```text
DissolveAmount = 0.0
```

4. pendant la seule phase de dissolution, un timer temporaire à environ 30 Hz fait progresser :

```text
DissolveAmount 0.0 -> 1.0
```

5. à `1.0`, le timer est arrêté et le Skeletal Mesh est caché.

L'Actor n'est pas détruit.

Le composant expose pour diagnostic :

```text
bDeathDissolveActive
DeathDissolveAlpha
```

## 6. Aucun Tick permanent

`UGridMonsterDeathComponent` conserve :

```text
PrimaryComponentTick.bCanEverTick = false
```

La mise à jour visuelle n'existe que pendant `DeathDissolveDuration` via `FTimerManager`.

Cela évite de faire ticker en permanence tous les monstres / cadavres du donjon.

## 7. Reset de présentation

Le composant expose :

```text
ResetDeathDissolvePresentation(
    bRestoreOriginalMaterials,
    bRestoreVisibility)
```

Cette API :

- arrête les timers de dissolution ;
- peut restaurer les matériaux d'origine ;
- remet l'alpha transitoire à zéro ;
- peut rendre le mesh visible à nouveau.

Elle prépare MON17.8.6, qui traitera explicitement le contrat Save/Restore des monstres morts.

MON17.8.5 ne change pas encore la politique de restauration : le comportement MON9 existant reste inchangé jusqu'à MON17.8.6.

## 8. Matériaux — authoring UE5.5.4 requis

Le runtime suppose seulement l'existence éventuelle d'un paramètre scalaire :

```text
DissolveAmount
0.0 = visible
1.0 = entièrement dissous
```

Les matériaux GoblinThrower concernés sont actuellement :

```text
M_Goblin_Bomber
M_Cloth_Bomber
M_Hair_Bomber
```

Les `.uasset` étant binaires / Git LFS, leur graphe Material et leur Blend Mode ne doivent pas être devinés depuis GitHub.

Avant authoring du dissolve, ouvrir les trois matériaux dans UE5.5.4 et relever pour chacun :

```text
Blend Mode
Shading Model
Opacity / Opacity Mask actuellement utilisés ou non
Masked / Translucent éventuel
paramètres existants pertinents
```

La solution Material sera choisie ensuite à partir de cet état réel.

Important : le C++ fonctionne même si le paramètre n'existe pas ; dans ce cas aucune transition visuelle ne sera perceptible, mais le mesh sera tout de même caché à la fin. La validation PIE doit donc confirmer que les trois matériaux réagissent bien au paramètre.

## 9. Configuration initiale GoblinThrower proposée

Après authoring matériau :

```text
DA_MON_GoblinThrower

bEnableDeathDissolve       = true
DeathDissolveDelay         = 2.0
DeathDissolveDuration      = 1.5
DeathDissolveParameterName = DissolveAmount
```

Ces valeurs sont un point de départ, pas encore une validation esthétique.

Le rythme final doit rester compatible avec un dungeon crawler posé, pas un jeu d'action.

## 10. Tests automatisés

Le fichier existant :

```text
Source/GrimrockPrototype/Private/Tests/
    GridMonsterMON178DeathPresentationTests.cpp
```

est étendu avec :

```text
Grimrock.Monsters.MON17.8.DeathDissolveDefinitionContract
Grimrock.Monsters.MON17.8.DeathDissolveApiContract
```

### DeathDissolveDefinitionContract

Vérifie :

- dissolve désactivé par défaut ;
- valeurs par défaut ;
- nom `DissolveAmount` ;
- compatibilité des anciennes définitions ;
- validité quand le dissolve est activé avec les defaults ;
- rejet d'un nom de paramètre vide ;
- rejet d'un délai négatif ;
- rejet d'une durée nulle.

### DeathDissolveApiContract

Vérifie la présence de :

```text
bEnableDeathDissolve
DeathDissolveDelay
DeathDissolveDuration
DeathDissolveParameterName
bDeathDissolveActive
DeathDissolveAlpha
ResetDeathDissolvePresentation
```

et protège l'invariant :

```text
PrimaryComponentTick.bCanEverTick == false
```

## 11. Validation demandée après pull / compilation

Exécuter :

```text
Grimrock.Monsters.MON17.8
```

Le filtre doit maintenant contenir **6 tests** :

```text
AnimationStateBridgeContract
BestiaryPresentationBridge
DeathDefinitionContract
DeathPresentationApiContract
DeathDissolveDefinitionContract
DeathDissolveApiContract
```

Puis régressions :

```text
Grimrock.Monsters.MON8
Grimrock.Monsters.MON10
Grimrock.Monsters.MON17.3.3
```

Aucun de ces résultats n'est déclaré réussi tant que le log UE5.5.4 local n'a pas été fourni.

## 12. Validation PIE MON17.8.5 à venir

Après authoring des matériaux et activation dans `DA_MON_GoblinThrower` :

```text
1. mort et DeathMontage inchangés ;
2. pose finale tenue pendant DeathDissolveDelay ;
3. dissolve progressif visible sur corps / vêtements / cheveux ;
4. aucune partie du Gobelin ne reste opaque par oubli de matériau ;
5. mesh caché à la fin ;
6. Actor mort non détruit ;
7. loot toujours visible et récupérable ;
8. victoire / encounter inchangés ;
9. aucune nouvelle occupation de cellule ;
10. RatGiant inchangé tant que bEnableDeathDissolve=false.
```

## 13. Frontière avec MON17.8.6

MON17.8.6 traitera la persistance :

```text
mort restaurée depuis SaveGame
    -> aucun DeathMontage
    -> aucun DeathAudio / DeathVFX
    -> aucune dissolution rejouée
    -> SkeletalMesh hidden immédiatement
```

Aucune progression de dissolve n'est ajoutée au SaveGame : un monstre mort restauré sera considéré comme ayant déjà terminé toute sa présentation de mort.
