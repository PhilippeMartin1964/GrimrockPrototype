# ARM-CLEAN06.5G.61 — Autorité canonique du poids

Parent 4aba6cc4b9797ae0d0fd65aa2541f5aad0635ec5. Publication après deux statuts SUCCESS.

Le lecteur commun refuse une masse copiée sur un ItemId absent du registre ; seule une fixture Automation active sans registre conserve ce calcul. La masse physique des objets au sol partage le contrôle d'identité, niveau et quantité du ramassage. Le poids des contenus de réceptacle ne clamp plus de quantité invalide. Le diagnostic de lancer n'autorise plus une ancienne masse. Les formules poids × quantité restent identiques pour des données valides ; les tests 3I et 3N distinguent le refus de production et le test numérique historique isolé. La policy Chaos bUseItemWeightAsWorldPhysicsMass de l'ancien acteur, les propriétés sérialisées et les catégories des énigmes doivent être retirées séparément sur preuve native. Aucune modification d'asset ou SaveGame. Validation Windows et PIE/Shipping nécessaires.

## Correctif de validation 5G.61.1

Le rapport Windows [#207](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/207) signale `CanonicalInventoryDiagnosticWeight`. Le test 3AC attendait encore une masse copiée pour un ID inconnu dans un registre configuré. Le diagnostic d'instance et de définition doit afficher INVALID dans ce cas ; seules les fixtures isolées sans registre conservent leur format numérique. Les assertions N1/N20/N0 et les poids valides restent inchangés. Nouvelle double validation requise.
