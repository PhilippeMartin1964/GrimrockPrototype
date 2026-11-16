# ARM-CLEAN06.4X.13 — Parité visible de la palette (porte de bascule)

**Parent X.12 validé :** `4f6ed4c4d6ebff6827a0ec8c97692fd59034d556` ;
`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS.

Les 33 entrées directes de la palette possèdent déjà un `CanonicalItemId`
et un `CanonicalDefaultItemLevel` physiques. X.11 vérifie leur validité,
X.12 transmet identité et niveau aux nouvelles instances placées.
Les tuiles Slate et la recherche affichent encore des champs provenant
de `DefaultItemDefinition`.

Trois sondes Editor-only indépendantes comparent les valeurs réellement
rendues aujourd'hui aux projections du seul `DA_ItemSystem` :
- `ProbeA_LabelParity` : titre effectif, y compris fallback par ItemId ;
- `ProbeB_DescriptionParity` : texte utilisé dans les recherches ;
- `ProbeC_IconParity` : chemin soft de l'icône des tuiles.

**Une différence fait échouer la sonde concernée.** Il ne faut
ni modifier des libellés, ni substituer une icône historique, ni
déclarer la parité par hypothèse. Les tests ne changent aucun asset ;
leur verdict doit être vérifié sur le SHA final. En cas d'échec,
qualifier et documenter les divergences réelles **avant** de
basculer les lecteurs Slate. Aucune valeur de remplacement inventée.

Aucun `.uasset`, `.sav`, `.umap`, WBP, Blueprint ni état
runtime n'est modifié ; UE5.5.4 Build et suite complète `Grimrock`
sont requis. Même une stricte égalité textuelle et de chemin
d'icône ne remplace pas une recette visuelle dans l'éditeur.

**Suite conditionnelle :** passage des 33 tuiles, filtres et
validations au registre canonique si les trois sondes sont SUCCESS ;
sinon résoudre chaque famille de divergence avant cutover.
