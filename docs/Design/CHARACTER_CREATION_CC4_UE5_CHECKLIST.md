# CC4 - Intégration du personnage dans l'Inventaire

## 1. Objet

Cette checklist sépare les changements déjà réalisés en C++ des opérations visuelles à effectuer dans Unreal Engine 5.5.4.

CC4 affiche le personnage créé par CC3 dans les widgets d'Inventaire existants. Elle ne modifie ni l'ownership des objets, ni les slots générés, ni le glisser-déposer.

Branche :

```text
codex/character-creation-cc3-startup-widget
```

---

## 2. Répartition des responsabilités

| Responsable | Travail CC4 |
|---|---|
| ChatGPT / Codex | Résumé complet du personnage, noms localisés, rafraîchissement natif, bindings UMG optionnels, test Automation et documentation |
| Utilisateur dans UE5 | Compiler, ajouter les champs visuels dans deux Widget Blueprints, vérifier le rafraîchissement et exécuter les tests |
| Hors CC4 | Sauvegarde, progression, choix d'autres races ou classes et refonte graphique de l'Inventaire |

Aucun calcul de caractéristique ne doit être ajouté dans un Graph Blueprint.

---

## 3. Étape A - Récupérer et compiler

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

---

## 4. Ce que le C++ fournit

`FGridInventoryCharacterSummary` expose désormais :

- nom, identifiants et noms localisés de race et de classe ;
- niveau et expérience ;
- six caractéristiques ;
- PV et mana actuels et maximums ;
- portrait ;
- charge actuelle, charge maximale et état de surcharge ;
- état de sélection et occupation de l'Inventaire.

`UGridPartyInventoryComponent` reste l'unique source de ces valeurs.

`UGridInventoryWidget::RefreshSelectedCharacterDetails()` alimente la fiche centrale. La méthode native `RefreshInventory()` appelle désormais :

1. `RefreshSelectedCharacterDetails()` ;
2. `RefreshRegisteredPartyMemberWidgets()` ;
3. `RefreshRegisteredSlotWidgets()`.

---

## 5. Étape B - Mettre à jour WBP_PartyMember

Ouvrir le Widget Blueprint existant `WBP_PartyMember`.

Ajouter ou renommer trois **Text Block** avec les noms exacts suivants :

| Nom exact | Contenu natif |
|---|---|
| `Text_Name` | Nom du personnage |
| `Text_ClassLevel` | Classe localisée et niveau |
| `Text_Weight` | Charge actuelle et maximale |

Pour chacun :

- cocher **Is Variable** ;
- laisser le texte de conception libre ;
- retirer les bindings Blueprint placés sur la propriété **Text**.

Ces trois widgets utilisent `BindWidgetOptional`. Leur absence ne bloque pas la compilation, mais la valeur correspondante ne sera pas visible.

L'événement Blueprint `RefreshMemberVisual` peut rester utilisé pour la couleur, la bordure ou l'état sélectionné. Il ne doit plus calculer ni affecter le nom, la classe, le niveau ou la charge.

Résultat attendu pour Elias :

```text
Elias
Guerrier - Niv. 1
Charge 0.0 / 80.0
```

---

## 6. Étape C - Construire la fiche centrale

Ouvrir `WBP_GridInventory`. Dans la zone centrale du personnage sélectionné, ajouter les widgets suivants.

| Nom exact | Type |
|---|---|
| `Image_CharacterPortrait` | Image |
| `Text_CharacterName` | Text Block |
| `Text_CharacterRace` | Text Block |
| `Text_CharacterClass` | Text Block |
| `Text_CharacterLevel` | Text Block |
| `Text_CharacterExperience` | Text Block |
| `Text_CharacterStrength` | Text Block |
| `Text_CharacterDexterity` | Text Block |
| `Text_CharacterConstitution` | Text Block |
| `Text_CharacterIntelligence` | Text Block |
| `Text_CharacterWisdom` | Text Block |
| `Text_CharacterCharisma` | Text Block |
| `Text_CharacterHealth` | Text Block |
| `Text_CharacterMana` | Text Block |
| `Text_CharacterCarryWeight` | Text Block |

Pour chacun, cocher **Is Variable** et ne créer aucun binding Blueprint sur **Text** ou **Brush**.

Les libellés statiques tels que « Force », « PV » ou « Expérience » peuvent être des Text Blocks distincts avec n'importe quel nom.

Le portrait est automatiquement masqué lorsque `DefaultPortrait` n'a pas été défini dans `WBP_CharacterCreation`.

---

## 7. Étape D - Simplifier RefreshInventory

Dans le Graph de `WBP_GridInventory`, rechercher `Event RefreshInventory`.

Solution recommandée :

1. supprimer l'override Blueprint s'il ne fait qu'appeler les anciennes méthodes de rafraîchissement ;
2. laisser l'implémentation native exécuter les trois rafraîchissements.

Si le Graph contient encore une opération visuelle nécessaire, conserver l'événement et ajouter **Call to Parent Function**. Ne pas rappeler ensuite manuellement les trois méthodes natives.

Compiler et enregistrer `WBP_GridInventory` et `WBP_PartyMember`.

---

## 8. Étape E - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les onze tests :

- quatre tests CC0 ;
- trois tests CC1 ;
- trois tests CC2 ;
- `Grimrock.CharacterCreation.CC4.InventorySummary`.

Les onze tests doivent être verts.

---

## 9. Étape F - Validation en PIE

1. Lancer un nouveau PIE.
2. Créer `Elias` avec **Entrée** ou le bouton.
3. Ouvrir l'Inventaire.
4. Vérifier la colonne du groupe : `Elias`, `Guerrier - Niv. 1`.
5. Vérifier la fiche centrale :
   - race `Humain` ;
   - classe `Guerrier` ;
   - niveau `1` et expérience `0` ;
   - caractéristiques `16 / 12 / 14 / 10 / 10 / 10` ;
   - PV `20 / 20` ;
   - mana `0 / 0` ;
   - charge `0.0 / 80.0`.
6. Ramasser un objet et vérifier que la charge se rafraîchit.
7. Équiper puis déplacer cet objet pour vérifier que les slots, le curseur et l'ownership fonctionnent toujours.
8. Fermer et rouvrir l'Inventaire : les mêmes valeurs doivent réapparaître.

En l'absence de CC5, le personnage est recréé à chaque nouveau lancement du PIE.

---

## 10. Critère de validation

CC4 est validée lorsque les onze tests sont verts, que les valeurs affichées correspondent à la création et que les interactions d'Inventaire existantes ne régressent pas.
