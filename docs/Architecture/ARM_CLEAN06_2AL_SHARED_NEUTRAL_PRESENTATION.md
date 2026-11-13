# ARM-CLEAN06.2AL — Un seul profil neutre pour les 15 attaques non configurées

## Preuve native 06.2AK

Le statut Windows du SHA `7add15e5` fournit les compteurs **réels**
du test Automation sur les seules **22 définitions du manifeste** :
19 actions standard, zéro action personnalisée, zéro action locale
supplémentaire, quatre présentations effectives liées, 15 absences
de profil, zéro clé manquante et zéro collision.

## Comportement historique : une valeur, pas un effet inventé

`UGridPlayerAttackPresentationComponent::ResolveProfile` renvoie
explicitement `FGridPlayerAttackPresentationProfile()` quand
`HasValidPlayerAttackPresentation()` est faux. La classe par défaut
est valide : `MotionStyle=None`, pas d'animation d'arme, pas de
sons ni de systèmes Niagara, timings et feedback de base conservés.

Pour les sources dont `bProvidesAttackPresentation=false`,
le convertisseur projette cette valeur **exacte** vers une unique
entrée `PRES_AttackNoFX` dans le catalogue transitoire.
Les 15 `ItemActionBinding` y pointent par identifiant ; il n'y
a plus de consultation du défaut historique au runtime cible.
L'ID et le profil sont des données canoniques explicites,
**pas un fallback de compatibilité**.

Les quatre sources avec présentation valide continuent à
utiliser leur vrai `PlayerAttackPresentationProfile` et leur
identifiant source ou la clé explicite du manifeste. Un opt-in
présent mais invalide reste refusé : aucune réparation silencieuse
et aucun effet audio/Niagara inventé.

## Barrières et tests

- 19 liens obligatoires, 15 liens neutres, un seul preset neutre ;
- aucune des trois entrées passives ne possède d'attaque ;
- chaque `PresentationId` pointe vers un profil existant ;
- parité complète du défaut via `EqualPresentation` sur
  mouvements, délais, audio et Niagara ;
- `CompleteProductionPreview23` exige validation globale de
  quatre catalogues, 82 ARM, 1 640 profils et 260 passifs ;
- `ReadyForBinaryCutover()` signifie **prévol des données pour
  authoring binaire surveillé**, pas conversion réelle des
  fichiers ni activation des lecteurs 06.3–06.5.

Le code introduit une **réutilisation**, pas 15 copies.
Les deux fichiers hors manifeste restent hors périmètre.
`.uasset`, `.umap`, WBP, SaveGame et runtime historique
restent inchangés. L'ensemble doit passer
`SilentTask` et `LocalAuto` sur le SHA exact, puis un
authoring Unreal Editor sécurisé, suivi de PIE au moment opportun.
