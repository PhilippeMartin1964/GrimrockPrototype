# ARM-CLEAN06.2Q — Assemblage transitoire des actions et présentations ARM

L'assemblage de ARM-CLEAN06.2P contient les 22 fiches
concrètes et leurs visuels, sans action ni catalogue numérique.

Ce jalon ajoute, toujours exclusivement en mémoire, les
`DA_ActionCatalogue` et `DA_ActionPresentationCatalogue`
**candidats**. Le générateur de production historique
est reproduit et comparé champ par champ dans
`GridArmClean06ActionProjection` (ARM-CLEAN06.2H) :
seules les attaques standard complètement équivalentes
sont ajoutées au catalogue canonique.

Les attaques personnalisées et les actions locales
secondaires restent intactes dans les assets sources.
Une action sans `PresentationProfileId` authoré,
ou sans présentation locale valide, n'acquiert
aucune liaison arbitraire. Les éventuels conflits
entre identités de présentation sont conservés
comme une question explicite de migration ;
aucune fusion automatique des sons/Niagara.

Les couples `ActionId/PresentationId` prouvés
sont regroupés dans une map **transitoire**
`BindingsByArchetype` et ne sont pas persistés
sur un autre asset tant que le `WeaponCatalogue`
canonique N1–N20 n'est pas constitué avec ses
mains, passifs, réactions et munitions démontrés.

Un test Automation combine réellement les
projections de ARM-CLEAN06.2P, 06.2H et 06.2G,
sur les 22 seules armes du manifeste. Il vérifie
19 armes offensives, trois passives, l'absence
de création d'attaques/présentations fictives,
les erreurs explicites, la propreté des packages
sources et le rejet du `DA_ItemSystem` incomplet.

Aucun lecteur runtime historique n'est modifié.
Aucun `.uasset`, `.umap`, WBP, Sword, RatGiant,
ThrowingKnife, Shuriken ou SaveGame n'est écrit.
Le modèle ne sera matérialisé qu'après une
validation complète sur le SHA Windows exact.
