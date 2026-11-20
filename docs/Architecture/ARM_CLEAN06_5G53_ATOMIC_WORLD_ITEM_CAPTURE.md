# ARM-CLEAN06.5G.53 — Capture atomique des objets du monde

Parent 7f3847d9e3894e61e26dddd75afdb114a14181e4. Publication après deux statuts SUCCESS.

Les objets libres actifs et projectiles de munitions non convertis sont capturés dans une map provisoire avant toute réinitialisation du snapshot. Le chemin de production lit l'ID du runtime entry, vérifie identité GUID/ID/niveau de l'acteur, refuse GUID dupliqués, quantité invalide et identité hors registre via ValidateLevel ; la même validation assemble réceptacles et PendingInboundItems. Les munitions actives gardent exactement une unité et ne sérialisent pas le pointeur historique. Conservation du snapshot antérieur si refus. Le rafraîchissement des coordonnées physiques reste un calcul runtime préalable. Les fixtures historiques sans GameInstance gardent temporairement leur ancien chemin, à retirer après migration. Les acteurs déjà détruits restent traités comme des objets retirés, les projectiles déjà convertis ne sont pas capturés deux fois.

Le test existant 3BD Capture vérifie GUID puis ID physique contradictoires, conservation du snapshot et recapture positive après correction. Les doublons entre inventaire et niveaux distincts, capture des monstres et suppression native des anciens assets ne sont pas déclarés résolus. Aucun asset/save écrit. Validation Windows et PIE/Shipping requises.
