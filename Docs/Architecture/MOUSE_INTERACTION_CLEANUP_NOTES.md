# Notes de nettoyage des interactions souris

## Corrections appliquées

- Le multi-trace qui ignorait les obstacles et les composants refusés a été remplacé par un impact `Visibility` unique.
- Le dépôt d'un item ne cible plus automatiquement le réceptacle du bord regardé : un impact direct est obligatoire.
- La chaîne de porte utilise maintenant `CanPartyInteractWithEdgeObject()` comme les autres interactions de bord.

## Limites connues

- Aucune solution de repli à la souris vers une cellule ou un bord n'est implémentée. La solution de repli vers l'avant existe seulement dans l'action clavier historique, désactivée par défaut.
- Le curseur de la chaîne de porte est `Take`, alors que `Pull` serait sémantiquement plus cohérent. Ce changement visuel doit être validé avec le widget Blueprint.
- Le curseur système de repli ne représente pas tous les états de `EGridInteractionCursor`.
- Les objets génériques lisibles avec `Edge=None` méritent un test fonctionnel dédié : leur interaction emprunte le chemin réservé aux bords.
- Les meshes de niveau et de porte reposent en partie sur les réglages de collision de leurs ressources Static Mesh. Un audit dans l'éditeur reste nécessaire pour garantir que chaque obstacle visuel bloque `ECC_Visibility`.
- Le survol recalcule et transmet l'état du curseur à chaque tick. Une mise en cache serait possible, mais n'est pas nécessaire pour le socle actuel.

## Évolutions hors périmètre

- Un service central de résolution des priorités de la souris.
- Une sélection explicite de cellule ou de bord lorsque le rayon ne touche aucun acteur.
- Des indications d'interaction ou une aide contextuelle.
- Une refonte des collisions ou des canaux de trace.
- Une automatisation de tests fonctionnels Unreal pour les occlusions et sous-composants.
