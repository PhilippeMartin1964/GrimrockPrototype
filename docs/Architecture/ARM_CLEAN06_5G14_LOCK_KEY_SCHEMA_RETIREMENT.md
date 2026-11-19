# ARM-CLEAN06.5G.14 — Retrait physique du schéma historique des serrures

Précondition : 5G.13 validé sur 93f17cff75dbdb39b423ef04d2384b7ca0947c07, plus preuve native X.48 (serrures cuivre/fer et overrides dormants déjà convertis).
- Suppression de AcceptedKeyItems dans FGridLockBehaviorParams et FGridLockAcceptedKeyInstanceRuleValues, sans alias ni valeur de repli.
- L'override ne copie plus que les IDs explicites. L'inspecteur Editor affiche seulement ces IDs, y compris la liste vide volontaire. La ligne historique des assets et le helper de conversion de clés sont retirés.
- X.29, GEUI09 et X.50 vérifient la résolution, l'édition et l'override vide ; X.165 conserve les scénarios d'ordre, doublons, IDs de script, absence de registre et inventaire des quatre niveaux et des définitions. Le test historique de conversion de pointeurs est remplacé par le contrat d'authoring utilisable.
- X.48 ajoute deux assertions par réflexion de l'absence des propriétés ; X.51/X.59/X.81/X.1661/TD07 contrôlent les deux assets exacts, les trois owners dormants et les autres règles de puzzle sans autoriser un ancien état des clés. Les audits de références AssetRegistry et le test de non-modification des packages restent.
- Aucun asset, Blueprint, WBP, SaveGame ni texte joueur modifié. Le changement de présentation concerne seulement le Slate de l'inspecteur Editor. Contrôle visuel inspecteur et recette finale 06.6 toujours distincts.

Attendre les deux statuts Windows sur le SHA publié avant le prochain lot.

## Correction du lot avant validation

Issue #202 : BUILD_FAILED (C2039 et erreurs en cascade) dans TD0737 CurrentAssetNormalization, référence résiduelle au champ supprimé dans le lambda de contrôle des serrures. Le contrôle exact des deux IDs est conservé, le test du champ retiré est couvert par réflexion dans X.48/X.81. Révision atomique du seul lot 5G.14, parent inchangé.
