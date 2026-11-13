# ARM-CLEAN06.2AK — Bloqueurs d'actions issus de l'Automation réelle

## Objet et périmètre

Le manifeste compte **22** armes de production : **19** attaques,
**3** objets passifs. Le jalon 06.2AJ a ajouté 19 clés de migration
explicites sans modifier les assets ou le gameplay.

Pour éviter une nouvelle couche C++, nous exploitons le rapport
**Automation UE5.5.4 existant**, test
`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionCatalogues22`.
Le test écrit déjà des compteurs avec son `AddInfo`.

Le déclencheur ponctuel du validateur Windows, sous
`UE5.5.4/ARM-CLEAN06.2AK`, ne fait que lire les rapports
privés de son propre SHA, extraire ces sept compteurs et
publier une **description filtrée** dans le statut GitHub :

- `standard` : attaques standard prouvées ;
- `custom` : attaques principales non standard encore à convertir ;
- `extra` : autres actions de la source non converties ;
- `bound` : liens ActionId/PresentationId complets ;
- `noKey` : ID de présentation encore absent ;
- `noFX` : profil de présentation local absent ;
- `collisions` : doublons de PresentationId.

Une donnée absente ou incohérente fait échouer **ce statut
diagnostique uniquement**, sans masquer un échec de validation
générale. Le rapport n'expose ni noms d'assets ni chemins privés.
Aucun nouveau test C++, commandlet, résolveur, lecture des deux
anciens assets hors manifeste, ni modification binaire n'est ajouté.

L'agent conserve ses statuts `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto`. La destination reste une suppression
définitive de l'outillage provisoire après 06.4–06.5.

Cette étape mesure précisément les blocages avant l'authoring
réel des quatre catalogues. Elle **n'autorise pas** le cutover.
