# CC4.1 - Nettoyage et mise en page de l'Inventaire

## 1. Objet

Cette tranche consolide CC4 sans modifier les données du personnage, l'ownership ni les règles d'Inventaire.

Elle sépare les responsabilités :

| Responsable | Travail CC4.1 |
|---|---|
| ChatGPT / Codex | Rafraîchissement C++ unique, suppression du code C++ vide, documentation |
| Utilisateur dans UE5 | Nettoyage du Graph, retrait du diagnostic visuel, correction de la mise en page et vérification de l'avertissement UMG |

Branche :

```text
codex/character-creation-cc3-startup-widget
```

---

## 2. Étape A - Récupérer et compiler

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Fermer Unreal Editor, puis compiler :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Résultat attendu : `Build succeeded`.

CC4.1 remplace le couple `RefreshInventoryState()` / `RefreshInventory()` par une seule fonction native `RefreshInventory()`.

---

## 3. Étape B - Nettoyer WBP_GridInventory

### B.1 Supprimer l'ancien override

1. Ouvrir `WBP_GridInventory`.
2. Ouvrir le Graph.
3. Rechercher `Event RefreshInventory`.
4. Supprimer le nœud événement et toute sa chaîne :
   - `RefreshSelectedCharacterDetails` ;
   - `RefreshRegisteredPartyMemberWidgets` ;
   - `RefreshRegisteredSlotWidgets`.
5. Ne pas recréer cet événement.
6. Compiler et enregistrer.

Le C++ appelle désormais ces trois méthodes depuis l'unique fonction `RefreshInventory()`. Le Blueprint ne doit plus piloter ce rafraîchissement.

### B.2 Retirer le diagnostic visuel

Dans le Designer de `WBP_GridInventory` :

1. repérer le Text Block qui affiche `SelectedCharacter: 0 Elias Guerrier Lv1` ;
2. supprimer son binding vers `GetSelectedCharacterDisplayText` ;
3. supprimer le Text Block s'il n'a aucune autre fonction ;
4. conserver `Text_CharacterName`, `Text_CharacterRace`, `Text_CharacterClass`, `Text_CharacterLevel` et `Text_CharacterExperience`.

La fiche centrale devient l'unique affichage détaillé du personnage sélectionné.

---

## 4. Étape C - Corriger les onglets du menu

Dans `WBP_GrimrockMenu`, les six onglets doivent rester accessibles dans la largeur du viewport.

Solution recommandée :

1. ajouter un `Scale Box` nommé `ScaleBox_TopTabs` à l'emplacement actuel de `HorizontalBox_TopTabs` ;
2. déplacer `HorizontalBox_TopTabs` comme enfant unique du Scale Box ;
3. régler **Stretch** sur `Scale To Fit` ;
4. régler **Stretch Direction** sur `Down Only` ;
5. régler le slot du Scale Box sur **Horizontal Alignment = Fill** ;
6. conserver les six boutons dans `HorizontalBox_TopTabs` ;
7. compiler et vérifier qu'Inventaire, Compétences, Journal, Carte, Recettes et Codex sont visibles.

Ne pas dupliquer les boutons et ne pas déplacer les onglets dans `WBP_GridInventory`.

---

## 5. Étape D - Corriger la grille d'Inventaire

Dans `WBP_GridInventory` :

1. sélectionner `InventorySlotsGridPanel` ;
2. l'envelopper dans un `Scroll Box` vertical nommé `ScrollBox_InventorySlots` ;
3. régler le Scroll Box sur **Orientation = Vertical** ;
4. régler son slot sur **Horizontal Alignment = Fill** et **Vertical Alignment = Fill** ;
5. dans **Class Defaults**, régler `InventorySlotColumnCount = 3` pour la largeur actuellement disponible ;
6. conserver `InventorySlotCountOverride = 0` afin d'utiliser les 40 slots du personnage ;
7. ne pas modifier `InventorySlotWidgetClass` ;
8. compiler et enregistrer.

Résultat attendu : trois colonnes entièrement visibles et un défilement vertical pour les lignes suivantes. Aucun slot ne doit être coupé à droite.

---

## 6. Étape E - Vérifier WBP_ItemActionMenu

Un avertissement antérieur signalait un appel à `RemoveFromParent()` sur un widget qui n'avait plus de parent UMG.

Dans le Graph qui traite `OnItemActionMenuCloseRequested` :

1. vérifier que la référence du menu est valide ;
2. appeler `Is In Viewport` sur le menu ;
3. appeler `Remove From Parent` uniquement si le résultat est vrai ;
4. remettre ensuite la référence du menu à `None` ;
5. vérifier qu'aucun second chemin ne supprime le même menu.

Cette correction est nécessaire uniquement si l'avertissement apparaît encore après compilation de CC4.1.

---

## 7. Étape F - Régression Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les onze tests CC0 à CC4. Ils doivent rester verts.

Aucun nouveau test Automation n'est ajouté : CC4.1 ne modifie pas les règles métier.

---

## 8. Étape G - Validation PIE

1. Créer Elias.
2. Ouvrir l'Inventaire.
3. Vérifier l'absence de la ligne `SelectedCharacter: ...`.
4. Vérifier les valeurs de la fiche centrale.
5. Vérifier que les six onglets sont visibles.
6. Vérifier que les trois colonnes de l'Inventaire sont entièrement visibles.
7. Ramasser une torche.
8. L'équiper en main principale.
9. Vérifier qu'elle disparaît de son slot d'Inventaire et apparaît dans l'équipement.
10. Vérifier la charge `1.0 / 80.0`.
11. Fermer le menu d'actions et vérifier l'absence d'avertissement `RemoveFromParent()`.

CC4.1 est validée lorsque les onze tests sont verts, que l'Inventaire reste fonctionnel et qu'aucun élément utile n'est coupé.
