# FIREFX01 — Audio des surfaces de feu, sans dépendance au pack commercial

- FGridFireVfxSettings expose `FireCrackleLoopSound`, `MaxFireCrackleSources` (défaut 4) et `FireCrackleVolume` (défaut 0.65).
- Sélectionner un son **réellement bouclant**, spatial et avec atténuation configurée dans l'asset ou MetaSound.
- Les cases de feu adjacentes par leurs côtés sont regroupées en zones ; chaque zone reçoit au plus une source audio, à son barycentre.
- Les sources sont conservées entre deux reconstructions visuelles si l'ancre de zone persiste, afin de ne pas relancer sans cesse la boucle.
- La disparition des cellules, le changement de niveau ou EndPlay arrêtent/détruisent leurs sources.
- Le son peut être présent même si `GroundFireSystem` n'est pas affecté, pour dissocier le comportement audio de l'installation locale FireFX.
- Aucune référence `/Game/FireFX` ni audio privé n'est codée ou committée.
- Un commit de `BP_GridLevelRuntimeActor` contenant une référence au profil local ignoré par Git **reste interdit**.

Tests : `Grimrock.RPG.FIREFX01.FireAudioClusters` ; compilation/suite Unreal automatiques.
La restitution sonore du feu, sa spatialisation effective et l'équilibre du mixage exigent un contrôle PIE.

Ce jalon concerne le **crépitement persistant**. L'embrasement bref avec « wouf » et `NS_FFX_FireBurst` est pris en charge séparément par FIREFX04.
