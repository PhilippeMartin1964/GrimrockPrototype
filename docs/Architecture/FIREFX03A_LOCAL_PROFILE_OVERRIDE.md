# FIREFX03A–03D — Historique, mécanisme retiré

Ce document décrit un ancien contournement conçu lorsque
`DA_GroundFire` et `Effects/Fire/Local` étaient exclus de Git.

**Depuis FIREFX05, ce mécanisme n'existe plus dans le runtime.**
`Saved/Config/GrimrockLocalVfx.ini` est ignoré par le code.
Ne créez pas ce fichier et ne videz pas la propriété
`FireVfxProfile` du Blueprint pour l'utiliser.

Le contrat à appliquer est :
- profils/Blueprints Grimrock versionnés ;
- référence directe `BP_GridLevelRuntimeActor → DA_GroundFire` ;
- seul `Content/F_FreeFlameFx_Pack/` exclu pour ses fichiers tiers.

Voir `docs/Architecture/FIREFX05_VERSIONED_ASSETS.md`
et `docs/Architecture/SURFACE_AND_CATALYST_VFX_PROFILES.md`.
