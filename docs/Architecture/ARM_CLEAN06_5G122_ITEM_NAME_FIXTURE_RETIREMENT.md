# ARM-CLEAN06.5G.122 — Tests tooltips, VFX, combat et hotbar

Parent `b5d2c99910b123744cec2dd103ad9a1cfaa8ff3d`. Les factories de `FGridItemInstance` dans cinq fichiers de tests cessent d'écrire une copie de `DisplayName`. Les GUID, quantités, emplacements d'équipement et les états de lumière sont conservés. Les tests d'aperçu d'équipement, de tooltip d'arme, VFX de pile, équipement MON11 et raccourcis TD06.2 continuent à exercer les définitions explicitement mises à leur disposition, sans dépendance aux noms dupliqués. Aucun changement de runtime, d'assets, ni de sauvegardes.
