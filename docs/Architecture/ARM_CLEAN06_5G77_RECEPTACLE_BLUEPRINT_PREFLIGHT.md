# ARM-CLEAN06.5G.77 — Préflight Blueprint des réceptacles

Parent 254dadf2e72cca777239a21f502096138952cc78. Publication après validation Windows du parent.

Après 5G.72, le pointeur historique de FGridContainedReceptacleItem n'est plus utilisé comme autorité de catégorie et TryInsertItem conserve un paramètre historique ignoré. Le retrait réfléchi exige une preuve native préventive. MIG09 charge chaque Blueprint du périmètre /Game, inspecte ses appels et vérifie Variables / types de pins de graphes pour le struct contenu, y compris valeurs de map et pins split. L'audit est conservateur : même une utilisation légitime des autres champs demandera examen avant modification du schéma. Il n'est pas une preuve exhaustive d'absence de chaînes dynamiques, bindings de propriété non exposés dans ces graphes, instances placées dans les maps ou données de sauvegarde ; ces contrôles restent séparés avant retrait. Pour les Blueprint de réceptacle, vérifier aussi que le CDO n'a aucune ligne de contenu runtime authorée.

Aucune suppression de propriété, de fonction ou de package, aucune modification de Blueprint, aucune sauvegarde ouverte. La suite Automation fournit le verdict natif ; les usages exacts sont diagnostiqués dans les journaux privés si nécessaire.
