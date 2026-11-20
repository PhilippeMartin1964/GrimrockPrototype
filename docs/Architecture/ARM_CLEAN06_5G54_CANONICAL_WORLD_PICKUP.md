# ARM-CLEAN06.5G.54 — Ramassage physique canonique

Parent 0e67ab86c3b401c747a5badca54ea75b4d90652b. Publication après deux statuts SUCCESS.

Les deux fonctions de ramassage au sol partageaient une logique dupliquée susceptible de créer un GUID et de corriger une quantité invalide. En production, l'instance construite exige maintenant l'accord runtime entry / acteur (GUID, ID, niveau), une résolution canonique exacte et une quantité autorisée avant insertion dans le sac. Le poids vient du registre et les propriétés de lecture, lumière et transform sont conservées. Le chemin de retrait après insertion réussie reste inchangé, ainsi que la portée et les règles de cellule. Les fixtures historiques sans GameInstance gardent temporairement leur comportement ; aucun nouveau fallback de production.

Le scénario 3BD Pickup vérifie le refus d'un GUID contradictoire par clic, puis d'un ID contradictoire par cellule, l'absence d'octroi, la rétention de l'acteur et le succès après correction. Le routage via les fosses, les autres lanceurs et la suppression du cache historique restent à traiter. Aucun asset/save écrit. PIE/Shipping encore requis.
