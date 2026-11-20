# ARM-CLEAN06.5G.48 — Restitution physique des réceptacles

Parent 01777ede14caa9133c489cc4d8c9a66714acfc3e. Publication après deux statuts SUCCESS.

Le service `TransferReceptacleItemToInventory` (route des réceptacles attachés) omettait ItemLevel et les données lisibles du candidat, et le rollback omettait le niveau. Il projette désormais le niveau physique, l’état réel de lumière et de lecture, le poids canonique de l’objet connu, et vérifie le niveau/capacité avant extraction. Le rollback reconstruit avec le niveau exact, maintient la provenance initiale canonique, conserve les marqueurs d’enlèvement préexistants, la lecture, la lumière, le transform et l’ordre logique. La capture d’un contenu sans acteur ne force plus sa lumière à true.

Le nouveau test sur un contenu initial N12 vérifie le refus avant mutation si le sac est plein puis une restitution réussie, identité, niveau, quantité et texte inchangés. Le chemin de secours après échec tardif reste à éprouver avec injection d’échec ; aucune garantie nouvelle contre les mutations réentrantes pendant les notifications. Ce lot ne modifie pas la sémantique EGridItemType des conditions logiques, ni les tolérances historiques des fixtures non enregistrées. Aucun asset/save joueur modifié ; validation Windows, PIE et Shipping nécessaires.
