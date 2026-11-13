# ARM-CLEAN06.3F — Vérifier la configuration réelle de démarrage

Le test persistant `RealNativeRegistryAndActions` confirme
désormais explicitement que la propriété `Config` du vrai
`UGridItemSubsystem` reçoit, depuis `DefaultGame.ini`,
**le chemin exact du seul `DA_ItemSystem` natif**.
Il ne se contente plus de charger ce DataAsset par un chemin codé
en dur pour le test.

La propriété est inspectée par réflexion Unreal
(`FSoftObjectProperty`), sans la rendre publique et sans
dupliquer une référence persistante. Les autres vérifications
des 22 armes N1–N20, 19 attaques, 3 passifs, 5 présentations,
munitions et GUID restent inchangées.

Aucune modification des assets, du HUD, de la logique de
combat ni des sauvegardes. Ce prévol est destiné à sécuriser
la validation PIE ciblée, pas à remplacer celle-ci.
