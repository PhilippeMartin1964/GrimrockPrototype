# ARM-CLEAN06.4K — Audit de propriété des 50 champs historiques non-ARM

**Parent validé :** 06.4J `d1548ca7ad69162dcba12bc8f8570b6d6004135c`,
`SilentTask`, `LocalAuto` et `ARM-CLEAN06.4J` SUCCESS ;
rapport public https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/143.

## Enjeu démontré

11 `ItemId` non-armement distincts, 40 occurrences dans les
niveaux/réceptacles/palette, 11 projections *en mémoire* conformes
sur les champs communs et **50 occurrences de propriétés
historiques actives non encore transférées**. Une seule famille
consommable est candidate, aucune pièce d'équipement hors ARM,
neuf objets restent génériques et un objet demeure non classé.

Ces valeurs **ne suffisent pas** pour créer les assets cible :
la base canonique ne contient que cinq attributs, les dérivés
non-armement portent uniquement les visuels et les extensions
propres aux consommables/équipements/munitions. Le runtime
continue d'exécuter des politiques *legacy* de lecture,
lumière, lancer et physique lorsque l'item n'est pas encore
enregistré dans `DA_ItemSystem`.

## Contrôle exact dans Unreal Editor

Le nouveau commandlet
`UGridArmClean064KNonArmFieldOwnershipCommandlet` recharge,
en lecture seule, les 11 anciennes définitions et les
23 définitions du `DA_ItemSystem` réel.

Pour chaque identité, il vérifie :

1. le SHA/provenance de 06.4I et 06.4J, les IDs et les 40
   occurrences ; la définition source demeure distincte
   de celles déjà enregistrées ;
2. **toutes** les valeurs `ExportText` des `FProperty` de
   l'ancien asset contre l'export intégral 06.4I ;
3. les champs jugés non recouverts par 06.4J :
   existent bien sur la classe actuelle, sont **non par défaut**
   (`FProperty::Identical_InContainer` vs CDO), sans doublon ;
4. une catégorie d'autorité **indicative**, une revue
   sémantique obligatoire et le blocage de conversion ;
5. l'égalité totale des 50 occurrences de champs, les 11 IDs,
   40 placements et l'absence de mutation `Dirty`.

| Famille de propriété historique | Autorité à définir ou comparer | État |
|---|---|---|
| `ReadText` | Texte lisible (instance ou asset lisible) + capacité de lecture | Revue requise |
| `LightEmitter` | Règle de lumière non-ARM + état mutable d'instance | Contrat incomplet |
| `VisualEffects`, `WorldSparkle*` | Présentation persistante non-ARM (pas les FX d'attaque) | Contrat incomplet |
| `bUseItemWeightAsWorldPhysicsMass`, tilt | Masse de ramassage / Chaos | Revue physique |
| `Throw*`, orientation, axe projectile | Lancer **générique non-ARM** : valeurs physiques et rendu | Contrat incomplet |
| `QuickItemCombatAction`, `CombatActions` | `DA_ActionCatalogue`, liaisons d'usage et vrai exécuteur | Parité requise |
| Main, équipement, bonus, résistances | Définition non-ARM spécialisée si justifiée | Parité requise |
| Empilement hors consommable | Politique explicite de stack non-ARM | Contrat à définir |
| Références anciennes de catalogues d'armes | Dette inattendue sur non-ARM | Interdiction de déduction |
| Autres champs | Propriétaire inconnu | Bloquant |

Les intitulés du tableau sont des **domaines de diagnostic**,
pas des propriétés déjà déplacées. Le commandlet reconnaît
les propriétés par **nom exact** et marque tout autre champ
`UNCLASSIFIED`, sans créer d'autorité de repli.

En particulier, le `DA_WeaponCatalogue.ProjectilePolicies`
canonique est actuellement réservé aux **archétypes d'armes**.
Il serait incorrect d'y transférer les lancers génériques de
torches, composants ou objets d'énigme. Le modèle
`FGridCanonicalItemInteractionReadModel` montre également
que `ReadText` et `UseActions` ne garantissent pas,
sans preuve, la même interaction que l'ancien système.

## Preuves, publication et validation

`Scripts/AuditARM_CLEAN06_4K_NonArmFieldOwners.ps1`
lit uniquement les reçus **privés** 06.4I et 06.4J
et les compare à leurs empreintes SHA-256, puis rehash
les **84 originaux** et **84 sauvegardes privées**
du jalon 06.4G.

Après le Build UE et la suite Automation `Grimrock`,
le sujet Git exact
`chore(ARM-CLEAN06.4K): classify 50 live nonarm legacy field owners without writes`
active `-run=GridArmClean064KNonArmFieldOwnership`.

Le dossier Windows privé contient `ownership-private.json`
(valeurs/champs exacts, source, famille, revue de chaque
champ) et `field-owner-gate-private.json` (provenance,
empreintes, compteurs et verrou fermé).
Le ticket public `[ARM-CLEAN06.4K]` ne contient **que les
comptes agrégés par catégorie**, jamais les noms d'objets,
IDs, champs métier, valeurs ou chemins sensibles.

**Succès 06.4K = inventaire réalisé et exhaustif, pas parité
sémantique ni autorisation d'authoring.** Toutes les
listes d'écriture des `.uasset`, `.umap`, `.sav`
restent vides ; le runtime, l'UMG, les niveaux et les
SaveGames restent inchangés. `-NullRHI` ne remplace
pas une séance PIE/Chaos ou un Cook.

## Suite conditionnée par les chiffres réels

Examiner dans 06.4L les domaines réellement présents,
en priorité ceux qui nécessitent des classes/données
non-ARM nouvelles, décider une **autorité unique** pour
chacun et la prouver via un moteur transitoire à comparaison
de valeurs et tests de comportement. Seulement ensuite :
authoring dans UE Editor avec backup, liste fermée,
rollback, mise à jour niveaux/palettes et conversion des
versions SaveGame existantes.

Aucun texte WBP n'est touché :
`UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
