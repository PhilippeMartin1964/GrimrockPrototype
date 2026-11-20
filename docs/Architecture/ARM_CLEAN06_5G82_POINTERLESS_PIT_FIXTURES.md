# ARM-CLEAN06.5G.82 — Tous les transferts inter-niveaux sans pointeur

Parent 4d6bb9a2556f5c0e2dcbf87c0d36535a33557668. Publication après double validation Windows du parent.

Convertir les trois anciennes fixtures PIT02/PIT03/PIT03.2 qui utilisent une pierre synthétique non enregistrée à la vraie Stone N0. Les attentes de poids utilisent la résolution canonique. Le fallback de cellule d'arrivée, l'ouverture en chaîne, l'animation, l'état de trappe, le retour entre niveaux et le GUID restent testés. Aucun kit/asset/SaveGame réel modifié.

La file inter-niveaux n'accepte plus aucun objet sans profil canonique, même en Automation. Elle n'écrit ni ne lit ItemDefinitionAsset et ne peut pas matérialiser un objet grâce à ce champ ancien. Les tests de compatibilité et l'audit Editor conservent ce champ v27 tant que le retrait sérialisé n'est pas prouvé. Les déplacements locaux de fixtures sans registre hors de ce périmètre ne sont pas modifiés.
