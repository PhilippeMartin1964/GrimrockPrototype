# ARM-CLEAN06.2AM.1 — Exécuter le vrai prévol de l'authoring sous Automation

Le test existant `CompleteProductionPreview23` invoque maintenant
`UGridArmClean06CanonicalStageCommandlet::Main(TEXT(""))`
après sa vérification des 23 objets et des quatre catalogues.

Sans `-Apply`, le commandlet prépare le plan des 28 packages et
refuse toute destination existante ; il **ne crée ni ne sauvegarde
aucun package**. Le test confirme en outre la propreté des
deux catalogues sources. Tous les tests habituels `Grimrock`
doivent encore réussir.

C'est la preuve Native UE5.5.4 du chemin réel du futur authoring,
pas une autorisation d'écriture automatique. L'écriture des
28 `.uasset` reste séparée et ne peut être exécutée qu'après
validation de ce SHA et avec un dossier utilisateur propre.
L'agent de CI ne transmet jamais `-Apply`.
