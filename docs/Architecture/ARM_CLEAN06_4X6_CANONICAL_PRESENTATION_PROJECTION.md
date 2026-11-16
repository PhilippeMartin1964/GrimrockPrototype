# ARM-CLEAN06.4X.6 — projection canonique non-ARM de présentation

**Portée : C++ + tests en lecture seule. Aucun asset UE ni SaveGame modifié.**

Point de départ vérifié : ARM-CLEAN06.4X.5,
`c395ebb53891e611d3d16f75adb3f6585b714037`,
avec les trois contrôles Windows `SilentTask`, `LocalAuto` et
`ARM-CLEAN06.4X.5` **SUCCESS**. Les quatre vrais niveaux et la
palette portent les niveaux effectifs, sans supprimer aucun pointeur
historique. Le registre reste `DA_ItemSystem` à 34 définitions :
22 ARM et 12 non-ARM (la définition de flèches d’origine et les 11 identités migrées).

## Pourquoi cette étape est nécessaire

`AGridItemActor::InitializeFromItemDefinition` utilise encore
`UGridItemDefinitionAsset` pour la configuration de lumière,
de Niagara, de scintillement et de plusieurs réglages physiques.
En revanche, `InitializeFromItemDefinitionId` réinitialise
la lumière et les VFX. Effectuer immédiatement la bascule des
placements monde sur les seuls `ItemId` risquerait donc de faire
disparaître les flammes et effets visuels.

`UGridNonArmamentDefinitionAsset` contient déjà, depuis les jalons
06.4L à 06.4V, ses véritables données :

- `LightEmitter` (état par défaut, intensité, portée, teinte,
  fréquences et autres paramètres de scintillement) ;
- `VisualEffects` (systèmes Niagara, transformations, activation) ;
- paramètres de scintillement du pickup, matériau et couleur ;
- `ReadText` et `bReadable` ;
- politique de masse et inclinaison physique initiale.

## Modification C++

`UGridItemSubsystem::ResolveItem` copie ces données dans une
**projection transiente** `FGridResolvedItem`, directement depuis
l'**unique** définition non-ARM du registre.

La copie intervient **avant le retour anticipé de la famille
`UGridAmmoDefinitionAsset`** : les munitions ne peuvent donc pas
perdre leurs VFX ou règles physiques à cause de ce chemin spécial.

Les définitions ARM ne reçoivent aucune configuration non-ARM
inventée ; leurs propriétés visuelles restent issues du catalogue
canonique d'armement. Aucun appel à `UGridItemDefinitionAsset` ancien,
aucun fallback, aucune seconde source de vérité ni propriété SaveGame.

## Validation

Test `Grimrock.RPG.ARM.CLEAN06_4X6.CanonicalNonArmPresentationProjection`
sur les **34 véritables** définitions du registre :

- 22 familles ARM sans données non-ARM empruntées ;
- 12 familles non-ARM, dont les flèches initiales et les 11 identités migrées ;
- parité des données lumière, Niagara, scintillement, texte lisible,
  masse et inclinaison avec leurs propriétaires canoniques ;
- preuve qu'une modification d'une projection retournée ne peut pas
  modifier la définition du registre ou une prochaine résolution.

La suite `Grimrock` et la compilation Windows sont exigées sur
le SHA du commit fonctionnel. **Ce jalon ne commute pas encore**
les acteurs runtime et l'éditeur : il sécurise leur futur
chemin d'initialisation.

## Suite

Faire ensuite un chemin d'initialisation d'`AGridItemActor`
résolu à partir de `CanonicalItemId` + `ItemLevel`, avec
application de la projection visuelle et physique canonique ;
changer les placements monde et les réceptacles progressivement,
avec validation de ramassage, sauvegarde et rechargement sur
copies, puis PIE lumière/Niagara et Chaos.
