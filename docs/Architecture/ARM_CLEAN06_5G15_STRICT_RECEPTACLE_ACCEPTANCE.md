# ARM-CLEAN06.5G.15 — Acceptation des réceptacles par ItemId

Les deux règles natives (support de torche / gardien) ont déjà été converties et X.48 contrôle zéro source historique. Ce lot retire les chemins runtime et d'authoring de repli, avant le retrait physique de la propriété.

- La résolution d'une règle exige un registre configuré et un ItemId inscrit, sans déduire de niveau. Un ancien pointeur même concordant est refusé ; bAcceptAnyItem explicite reste indépendant.
- Le Slate d'instance utilise une saisie d'ItemId validée, comme le contenu initial existant. Un brouillon vide n'accepte rien et un ID inconnu n'écrase pas le précédent. Les copies de règles bloquent toute règle inconnue, vide ou historique, sans copie partielle. Le champ canonique des définitions partagées devient éditable, le pointeur visible en lecture seule pendant le retrait.
- X.161 et X.162 vérifient identité stricte, absence de registre, invalidités, copie atomique et override ; GEUI09 fournit un ID explicite. Le véritable script Lua du gardien reste testé avec refus d'un objet différent, consommation de deux gemmes, troisième conservée, matériaux et ouverture physique. La pile de trois gemmes (capacité synthétique de 10 déjà utilisée par la fixture) migre dans une copie transitoire du registre ; aucun changement des données de production. L'objet incompatible de test est Stone, inscrit au registre.
- Aucune modification d'asset, de WBP, de SaveGame ou de texte joueur. La présentation de l'inspecteur Editor est à contrôler visuellement avec la recette finale. Exiger les deux statuts Windows avant le retrait du schéma AcceptedItems.ItemDefinition.

