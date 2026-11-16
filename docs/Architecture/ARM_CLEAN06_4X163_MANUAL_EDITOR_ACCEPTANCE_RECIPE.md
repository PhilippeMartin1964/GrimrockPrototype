# ARM-CLEAN06.4X.16.3 — Recette ciblée d'édition des réceptacles

Jalon fonctionnel validé : `2952e852e9e4ee633fc3f42f66fbcfd5fe6bf1cb`.
Deux contrôles UE5.5.4 SUCCESS. **Cette recette
manuelle ne remplace pas les tests automatisés.**

Sur une **copie** du DataAsset de niveau, avec
un Grid Editor dont le `LevelAsset` pointe vers
cette copie :

1. Sélectionner un réceptacle d'essai. Activer
   `Override Acceptance / Capacity`, vérifier que
   sa liste d'objets acceptés et sa capacité
   correspondent à la définition héritée.
2. Dans la liste acceptée, choisir un objet
   connu de `DA_ItemSystem` (ex. une clé ou
   des flèches). Vérifier l'identifiant canonique
   et le comportement accept/reject en PIE.
3. Tester un objet non autorisé ; vérifier
   le refus et l'absence de consommation.
   Tester retrait et réinsertion de l'objet
   autorisé, puis les liens d'énigme associés.
4. Ajouter un contenu initial non-ARM (flèches)
   puis un ARM connu (épée droite), en contrôlant
   `CanonicalItemId`, quantité et niveaux
   physiques : non-ARM N0, ARM N1–N20.
5. Régler l'ARM à N20 et modifier seulement
   d'autres propriétés : N20 doit rester
   inchangé. Changer ensuite l'objet initial :
   le nouveau `CanonicalItemId` doit remplacer
   l'ancien et le niveau doit être recalculé.
6. Démarrer PIE sur la copie et vérifier
   l'apparition, l'affichage et le retrait
   du contenu ; aucun warning de niveau N0
   ARM ni de source contradictoire.

**Ne pas enregistrer ces essais dans les
DataAssets des vrais niveaux.** Ne pas toucher
aux `.sav` personnels. Signaler tout défaut
avec capture Selected Object et nom du
DataAsset de test. La migration réelle des
règles de définition reste un autre chantier.
