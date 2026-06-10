# Notes de nettoyage des interactions souris

## Corrections appliquees

- Le multi-trace qui sautait les obstacles et composants refuses a ete remplace par un hit `Visibility` unique.
- Le depot d'un item ne cible plus automatiquement le receptacle du bord regarde : un hit direct est obligatoire.
- La chaine de porte utilise maintenant `CanPartyInteractWithEdgeObject()` comme les autres interactions de bord.

## Limites connues

- Aucun fallback souris cellule/bord n'est implemente. Le fallback avant existe seulement dans l'action clavier historique, desactivee par defaut.
- Le curseur de la chaine de porte est `Take`, alors que `Pull` serait semantiquement plus coherent. Ce changement visuel doit etre valide avec le widget Blueprint.
- Le fallback de curseur systeme ne represente pas tous les etats de `EGridInteractionCursor`.
- Les objets generiques lisibles avec `Edge=None` meritent un test fonctionnel dedie : leur interaction appelle le chemin de bord.
- Les meshes de niveau et de porte reposent en partie sur les reglages de collision de leurs Static Mesh assets. Un audit dans l'editeur reste necessaire pour garantir que chaque obstacle visuel bloque `ECC_Visibility`.
- Le survol recalcule et transmet l'etat du curseur a chaque tick. Une mise en cache serait possible, mais n'est pas necessaire au socle actuel.

## Evolutions hors perimetre

- Un service central de resolution des priorites souris.
- Une selection explicite cellule/bord lorsque le rayon ne touche aucun acteur.
- Des prompts d'interaction ou une aide contextuelle.
- Une refonte des collisions ou des canaux de trace.
- Une automatisation de tests fonctionnels Unreal pour les occlusions et sous-composants.

