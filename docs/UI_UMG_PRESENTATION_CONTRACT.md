# Contrat permanent — Frontière entre C++ et WBP/UMG

**Décision de conception : 24 octobre 2026 — applicable à toutes les nouvelles interfaces et aux refontes.**  
**Exemple de référence : ARMEMENT01.10.3.3 — Combat Tooltip.**

## 1. Une seule responsabilité par couche

| C++ (autorité métier, état, lecture) | WBP / UMG (présentation, interactions visuelles) |
|---|---|
| Calcul de combat, jets, dégâts, absorption, PV et effets | Libellés, intitulés de sections, textes explicatifs |
| Valeurs brutes : nombres, booléens, énumérations, identifiants | Mise en forme des nombres / textes et références à ST_UI |
| Snapshots historiques des attaques et protections | Format Text / Format Number / conversion de valeurs en texte |
| Modèles de lecture USTRUCT(BlueprintType), TArray de lignes, indicateurs de visibilité | Conteneurs, alignements, colonnes, tailles, marges, couleurs, polices, icônes |
| Sélection de l'entrée / des données à afficher dans l'interface | Construction des lignes par Widget Blueprint réutilisable et événements Blueprint |
| Règles de filtrage métier (par exemple Absorbed > 0) | Afficher/cacher les titres, tableaux, espacements et messages selon les flags |
| Annoncer « données mises à jour » et livrer un modèle immuable au WBP | Actualiser les éléments graphiques et retourner un statut de présentation |

**INTERDIT pour une nouvelle UI ou refonte :** créer ses colonnes, en-têtes, TextBlocks, cases, fonts, padding, slots ou couleurs via C++ ; parcourir WidgetTree pour fixer leur géométrie ; assembler des chaînes françaises ou des formules de présentation dans une classe d'interface C++ ; dupliquer la localisation des textes statiques entre C++ et le Designer.

**AUTORISÉ :** exposer une donnée numérique déjà calculée, FText de contenu authoré dans un DataAsset, identifiant d'une source, état de chargement, flag de visibilité, liste ordonnée de résultats ; déclencher un seul événement de rafraîchissement Blueprint. Une projection **numérique** métier (par exemple résultat d'un dé virtuel issu d'une plage) reste C++ ; sa notation « 1d20 (16) » est une décision d'affichage du WBP.

**Exceptions ciblées et documentées :** journal textuel historique, export, console, erreurs, log, outils de debug, accessibilité ou fallback sans WBP peuvent générer du texte en C++. Ne pas réécrire ces domaines sans jalon propre. La présente règle **n'impose pas de refactor rétroactif aveugle** de tous les widgets existants.

## 2. Patrons autorisés

1. Déclarer un **F...ViewData** en USTRUCT(BlueprintType), avec propriétés BlueprintReadOnly **sans propriété de layout**. Préparer les nombres, identifiants, booléens et les tableaux C++ une seule fois.
2. Une classe native de widget peut uniquement transmettre ce modèle via événement Blueprint, sans créer de widgets. **Aucun ClearChildren/AddChild/SetFont/SetPadding/ConstructWidget dans cette classe**.
3. La classe WBP applique **ST_UI** aux labels statiques et les patterns de texte dynamiques ; elle choisit le layout et la visibilité.
4. Pour les listes dynamiques, créer un **WBP de ligne réutilisable** : le WBP parent itère les lignes, crée une instance du WBP de ligne et la met dans le VerticalBox. Les quatre TextBlock / colonnes sont définis **dans le Designer de la ligne**, jamais à la volée en C++.
5. Pour l'alignement, régler des **largeurs/poids de colonnes cohérents dans le Designer**, avec police Alegreya Sans ; réutiliser le même composant de ligne ou les mêmes mesures déclarées côté WBP.
6. Éviter les bindings évalués chaque frame si des données immuables existent. Préférer l'événement d'initialisation / rafraîchissement qui remplit l'interface **une fois par nouvelle donnée**.
7. Garder une **présentation de repli** si le nouveau WBP n'a pas encore été authoré. Ne jamais afficher une infobulle vide pendant une migration.

## 3. Localisation obligatoire

Appliquer **en plus** [le workflow ST_UI complet](UI_WBP_LOCALIZATION_WORKFLOW.md).

- Texte statique WBP → clé déjà matérialisée dans ST_UI → TextBlock Text référencé.
- Texte dynamique WBP → **pattern ST_UI localisé** + valeurs numériques du read-model → nœuds Blueprint Format Text / ToText.
- Nom d'objet → FText issu du DataAsset ; pas de recopie dans UI.csv.
- Texte d'un historique textuel sans WBP → exception justifiée, localisé par les mécanismes existants.

**Aucune nouvelle clé ne peut être utilisée avant matérialisation et inspection du ST_UI.uasset réel.** Les modifications de CSV ne suffisent pas.

## 4. Checklist obligatoire avant commit UI

- [ ] Calcul métier exclusivement en C++ ; read-model USTRUCT/BlueprintReadOnly clairement délimité.
- [ ] Pas de logique de création/layout/formatage de texte dans la classe C++ du WBP.
- [ ] Tous les textes statiques et patterns liés à ST_UI, ou contenus métier FText source.
- [ ] Liste dynamique rendue par un WBP de ligne indépendant et maintenable.
- [ ] Visibilité des sections pilotée par des flags issus du modèle.
- [ ] Préservation d'un fallback fonctionnel tant que le WBP n'est pas authoré.
- [ ] Tests C++ sur les **valeurs et le filtrage**, tests PIE/captures sur le rendu réel.
- [ ] WBP enregistré **dans son chemin /Game exact**, jamais édité en binaire hors Unreal.

**Un test C++ sur le read-model ne certifie pas l'alignement ou les traductions du WBP.** Ne déclarer la migration complète qu'après tests manuels en PIE et capture.
