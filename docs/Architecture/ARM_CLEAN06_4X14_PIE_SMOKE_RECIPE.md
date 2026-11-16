# ARM-CLEAN06.4X.14 — Recette visuelle UE Editor et PIE

**Base validée par Automation :**
`d9595b6308374da5f7fb53d7cab5fa279e9cfe60`,
`SilentTask=SUCCESS` et `LocalAuto=SUCCESS`.
Le test `-NullRHI` ne vérifie pas le rendu Slate, UMG,
Niagara, collisions Chaos ni les interactions réelles.

## Préparation

Utiliser UE **5.5.4** sur une **copie de niveau de test**, sans
enregistrer les vrais `DA_GridLevel_*` migrés.
Sauvegarder toute modification temporaire hors des assets
canoniques du projet. Ne pas toucher aux fichiers `.sav`.
La palette est `DA_ObjectPalette_Default` et le registre est
`DA_ItemSystem` (34 définitions).

## Palette Slate

1. Ouvrir le Grid Editor ; vérifier la présence des entrées de
   catégories `Weapon` et `Items`, avec icônes carrées,
   titres lisibles et absence de tuiles vides inattendues.
2. Chercher une arme par nom et `ItemId`, puis un non-ARM
   par nom et mot de description : résultats cohérents.
3. Tester filtres de catégories, Favoris et Récents :
   sélection, texte et icône identiques entre vues.
4. Placer une arme ARM depuis la palette : lire dans
   l'instance transitoire `CanonicalItemId`, `ItemLevel`
   physique N1–N20 et GUID. Modifier uniquement les notes :
   le niveau personnalisé ne doit pas être remis au défaut.
5. Placer un non-ARM et contrôler son niveau **N0 réel**,
   GUID, quantité et icône. Le sélectionner puis l'éditer.
6. Vérifier qu'une entrée canonique invalide n'est pas
   discrètement affichée à partir du vieux
   `DefaultItemDefinition`.

## Présentation et interaction runtime PIE

1. Lancer un donjon de test avec objet libre non-ARM
   et réceptacle à contenu initial : vérifier le mesh,
   la lumière, les effets Niagara et le scintillement.
2. Retirer, remettre et saisir le contenu : contrôler
   quantité, interactions souris et absence d'acteur dupliqué.
3. Tester le réceptacle `PhysicalAtHit` avec/sans
   physique et gravité, sans traversée inattendue.
4. Activer une énigme liée à un réceptacle ; constater
   que les signaux logiques et règles d'acceptation
   historiques n'ont pas changé.
5. Pour un item ARM, vérifier l'affichage et la lumière
   historiques, en attendant son cutover dédié.

## Résultat attendu et limites

Consigner **PASS/FAIL**, nom de l'objet, action et capture
pour tout défaut constaté. Aucun test de cette recette
ne convertit des sauvegardes personnelles ni n'autorise
la suppression du legacy. Une recette PIE réussie ne
remplace pas les tests Save/Continue et Cook/Shipping,
qui restent des jalons distincts.
