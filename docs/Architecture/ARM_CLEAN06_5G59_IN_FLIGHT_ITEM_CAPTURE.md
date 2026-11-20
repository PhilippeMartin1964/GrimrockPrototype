# ARM-CLEAN06.5G.59 — Capture des objets lancés en vol

Parent b9e16af9535da928514fa26d4acf2c533c33558c. Publication après deux statuts SUCCESS.

La capture du monde ne persistait que les projectiles marqués ammunition. Les objets et armes lancés en main étaient donc oubliés lors d'un changement de niveau. Tous les projectiles canoniques récupérables non convertis sont désormais capturés sous leur GUID, avec validation de l'accord acteur / instance, niveau exact, lecture et lumière. Les munitions conservent leur pose de mesh antérieure ; les autres objets récupèrent la projection de pickup du registre plutôt que l'échelle de vol. Si le point de vol ne résout pas une cellule, l'objet non-ammunition utilise la cellule source valide. Après capture réussie, le changement de niveau retire les projectiles sauvegardés du niveau quitté, même si le niveau cible n'a aucune sauvegarde antérieure.

Le test InFlightItemTravel lance un véritable couteau N20, vérifie le snapshot et la lecture, voyage vers un niveau inédit et revient vers un seul pickup canonique N20. Les tests historiques sans registre gardent le périmètre précédent. Les échecs de conversion à l'impact (objet détruit malgré un dépôt refusé), les projectiles marqués conversion en cours et le rendu Chaos/PIE restent à examiner séparément. Aucun asset/save écrit. Validation Windows et PIE/Shipping nécessaires.
