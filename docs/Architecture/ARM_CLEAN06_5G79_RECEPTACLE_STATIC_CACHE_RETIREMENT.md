# ARM-CLEAN06.5G.79 — Réceptacle sans cache statique

Parent 90f9435fa9a99215e77ecf47da04f6bb82c1cf88. Préflight 5G.77 et 5G.78 validés avant publication.

Conformément au contrat normatif, FGridContainedReceptacleItem ne porte plus Weight ou DisplayName. Ces champs runtime n'étaient pas SaveGame et leur nom temporaire n'était déjà pas capturé dans FGridRuntimeItemState. Un transfert depuis un curseur v27 portant un cache obsolète renvoie désormais le nom canonique et la masse canonique dès l'acceptation ; à la récupération, les projections transitoires FGridItemInstance encore nécessaires au schéma v27 sont reconstruites à partir du registre, jamais depuis une copie contenue. Les états lumière, identité, niveau, quantité, position et ReadableContent/ReadTitleOverride/ReadTextOverride restent inchangés. Le rollback ne réintroduit pas de cache statique.

Le test 0183 InsertRestoreTake injecte un poids et un nom contradictoires, vérifie la masse et le nom projetés, le poids total et la conservation des véritables textes narratifs après capture, restauration et récupération. Aucun WBP, texte joueur, niveau, package ou fichier de sauvegarde modifié. Les champs encore persistés dans FGridItemInstance sont un périmètre distinct, soumis à preuve SaveGame.
