# ARM-CLEAN06.2Z — Prévisualiser des clés de présentation sans inventer d'ID

## Problème bloquant

Le prévol 06.2Y.1 conserve les 19 attaques offensives réelles des
22 ARM de production. Plusieurs `FGridCombatActionDefinition` ont
un `ActionId` stable, mais pas de `PresentationProfileId`
authoré. Les convertir par une clé nouvelle non vérifiée pourrait
fusionner des audio/Niagara différents ou changer le résultat du HUD.

## Règle candidate, NON appliquée

`GridArmClean06PresentationKeyPreview::Build` est un outil
**Editor-only strictement en lecture**. Pour chaque attaque
principale issue du manifeste, il privilégie la liaison
`ActionId/PresentationId` déjà authorée. Si elle manque,
il n'émet une **proposition** que si :

1. Le `ActionId` est présent dans le manifeste et
   dans le vrai `DA_ActionCatalogue` canonique provisoire.
2. La présentation locale de l'arme est valide et peut être
   projetée sans perdre les variants audio et Niagara.
3. L'action n'a pas de `PresentationProfileId` explicite
   qui entrerait en contradiction avec ce choix.
4. Le même identifiant ne collisionne avec aucun profil déjà
   présent dans le catalogue ni avec une autre proposition.

La proposition utilise **exactement le `ActionId` déjà authoré**
comme clé candidate de présentation. Ce n'est pas un renommage
arbitraire ni une clé générée à partir du nom de l'objet.
**Elle n'est pas authorée dans le registre ni dans les assets** :
c'est un plan de migration examiné en mémoire, jamais un
nouveau `ItemActionBinding` effectif.

Les attaques personnalisées non projetées, les profils locaux
absents, les IDs explicites sans liaison et les collisions
sont **comptés et laissés intacts**, sans fallback.
Les trois ARM passifs conservent zéro action offensive.

## Tests et critères de sortie

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.AuthoredActionPresentationKeyProposals`
classe les 19 attaques des vrais 22 DataAssets, construit les
propositions avec les sources réelles, prouve que leurs
`PresentationId` sont des `ActionId` existants et uniques,
compare les compteurs avant/après et refuse toute écriture
sur le catalogue de présentations. Une seconde exécution
doit produire le même résultat.

Ces propositions ne suffisent **jamais** à autoriser
06.3–06.5 : l'authoring explicite de la politique et la
parité d'exécution / SaveGame resteront indispensables.
Aucun `.uasset`, `.umap`, WBP, niveau, gameplay ou
SaveGame n'est modifié. Validation UE5.5.4/Automation
`Grimrock` sur le SHA exact.
