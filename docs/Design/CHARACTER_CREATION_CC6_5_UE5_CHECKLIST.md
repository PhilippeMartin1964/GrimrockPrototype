# CC6.5 - Polissage visuel de la création et de l'inventaire

## 1. Objet

CC6.5 améliore le rendu visuel sans changer les règles de création de personnage.

Cette tranche ajoute :

- une couleur d'accent par classe ;
- un cadre ou liseré optionnel dans l'inventaire ;
- une icône de classe réduite dans la liste du groupe ;
- une préparation propre pour les futurs états visuels.

Cette tranche ne remplace pas encore le portrait par un personnage plein pied équipé. Ce sujet doit rester une tranche dédiée, car il demandera un système de composition corps + équipement.

---

## 2. Résumé technique

Ajouts C++ :

- `AvailableClassVisuals` dans `UGridInventoryWidget` ;
- `Border_CharacterClassAccent` dans `UGridInventoryWidget` ;
- application automatique de `URPGClassVisualAsset::AccentColor` dans l'inventaire ;
- `Image_ClassIcon` et `Border_ClassAccent` optionnels dans `UGridPartyMemberWidget` ;
- `SetAvailableClassVisuals()` dans `UGridPartyMemberWidget` pour alimenter la liste réduite du groupe.

Principe :

- `ClassIcon` reste l'icône principale de classe ;
- `AccentColor` devient la couleur d'identité visuelle de la classe ;
- les widgets sont optionnels grâce à `BindWidgetOptional` ;
- aucun Graph Blueprint ne doit calculer la classe à partir du texte affiché.

---

## 3. Étape A - Récupérer et compiler

Fermer Unreal Editor, puis :

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Compiler :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Résultat attendu : `Build succeeded`.

---

## 4. Étape B - Régler les couleurs d'accent des classes

Ouvrir les 6 DataAssets `DA_ClassVisual_*` créés en CC6.4 et renseigner `AccentColor`.

Valeurs recommandées :

| DataAsset | Classe | AccentColor recommandé |
|---|---|---|
| `DA_ClassVisual_Warrior` | Guerrier | rouge acier `#B94337` |
| `DA_ClassVisual_Rogue` | Voleur | ambre sombre `#C28A2E` |
| `DA_ClassVisual_Ranger` | Rôdeur | vert forêt `#3F8F4B` |
| `DA_ClassVisual_Mage` | Mage | bleu arcanique `#3E70C8` |
| `DA_ClassVisual_Priest` | Prêtre | or pâle `#D7C67A` |
| `DA_ClassVisual_Alchemist` | Alchimiste | vert-de-gris `#3CA69A` |

Conseils :

- garder une saturation modérée ;
- éviter les couleurs trop lumineuses qui attirent plus l'œil que le portrait ;
- conserver une cohérence avec l'icône de classe ;
- sauvegarder chaque DataAsset après modification.

---

## 5. Étape C - Configurer `WBP_GridInventory`

### C.1 Renseigner `AvailableClassVisuals`

Dans `WBP_GridInventory` :

1. Ouvrir **Class Defaults**.
2. Dans **Details**, chercher :

```text
AvailableClassVisuals
```

3. Ajouter les 6 DataAssets :

```text
DA_ClassVisual_Warrior
DA_ClassVisual_Rogue
DA_ClassVisual_Ranger
DA_ClassVisual_Mage
DA_ClassVisual_Priest
DA_ClassVisual_Alchemist
```

Cette liste permet à l'inventaire de retrouver `AccentColor` à partir du `ClassId` du personnage sélectionné.

### C.2 Ajouter le cadre d'accent

Dans la fiche centrale personnage, autour du portrait ou de l'icône de classe, ajouter un widget `Border` nommé exactement :

```text
Border_CharacterClassAccent
```

Réglages recommandés :

| Propriété | Valeur |
|---|---|
| Is Variable | oui |
| Visibility initiale | `Collapsed` |
| Brush Color | blanc ou transparent par défaut |
| Placement | autour du portrait, derrière l'icône, ou comme bande latérale |
| Thickness / Padding | discret, par exemple `2` à `4` px |

Le C++ applique automatiquement la couleur `AccentColor` de la classe sélectionnée.

### C.3 Vérifier l'icône de classe

Conserver l'image ajoutée en CC6.4 :

```text
Image_CharacterClassIcon
```

Le C++ continue de l'alimenter automatiquement. Si `AvailableClassVisuals` est renseigné dans `WBP_GridInventory`, l'icône peut être relue depuis `DA_ClassVisual_*`; sinon le système conserve le fallback persisté en CC6.4.

---

## 6. Étape D - Polir la liste du groupe

Dans le widget utilisé pour les membres du groupe, basé sur `UGridPartyMemberWidget`, ajouter si souhaité :

```text
Image_ClassIcon
Border_ClassAccent
```

Réglages recommandés :

| Widget | Type | Rôle |
|---|---|---|
| `Image_ClassIcon` | Image | petite icône de classe du membre |
| `Border_ClassAccent` | Border | liseré ou fond coloré par classe |

Réglages conseillés pour `Image_ClassIcon` :

| Propriété | Valeur |
|---|---|
| Is Variable | oui |
| Taille | `24x24` à `32x32` |
| Visibility initiale | `Collapsed` |

Réglages conseillés pour `Border_ClassAccent` :

| Propriété | Valeur |
|---|---|
| Is Variable | oui |
| Brush Color | blanc ou transparent par défaut |
| Placement | bord gauche, fond léger, ou cadre discret |
| Visibility initiale | `Collapsed` |

Important : chaque instance de ce widget doit connaître les `DA_ClassVisual_*`. Deux chemins sont possibles :

- renseigner `AvailableClassVisuals` directement dans le Blueprint du membre du groupe ;
- ou appeler `SetAvailableClassVisuals()` depuis le Blueprint parent si vous préférez centraliser la configuration.

---

## 7. Étape E - Graph Blueprint

Ne pas ajouter de logique Blueprint pour :

- déduire une classe depuis un texte ;
- choisir une couleur en fonction du nom affiché ;
- changer manuellement la texture de `Image_CharacterClassIcon` dans l'inventaire ;
- recopier des données de création vers l'inventaire.

Le Blueprint doit seulement :

- placer les widgets ;
- renseigner les DataAssets dans `AvailableClassVisuals` ;
- éventuellement appeler `SetAvailableClassVisuals()` pour les widgets réduits du groupe.

---

## 8. Étape F - Validation PIE

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Créer un personnage Mage.
4. Ouvrir l'inventaire.
5. Vérifier que `Image_CharacterClassIcon` affiche l'icône Mage.
6. Vérifier que `Border_CharacterClassAccent` prend la couleur bleue du Mage.
7. Recommencer avec Guerrier ou Rôdeur.
8. Vérifier que la couleur change selon la classe.
9. Si la liste du groupe a été polie, vérifier que `Image_ClassIcon` et `Border_ClassAccent` s'affichent aussi sur le membre.

Si l'accent ne s'affiche pas :

- vérifier que `AvailableClassVisuals` est renseigné dans `WBP_GridInventory` ;
- vérifier que `ClassId` du `DA_ClassVisual_*` correspond exactement à la classe ;
- vérifier que `Border_CharacterClassAccent` est nommé exactement ainsi ;
- vérifier que `Is Variable` est coché ;
- vérifier que le Border n'est pas masqué par un autre widget ;
- vérifier que la couleur `AccentColor` n'est pas transparente.

---

## 9. Critère de validation

CC6.5 est validée lorsque :

- la compilation C++ réussit ;
- les 6 `DA_ClassVisual_*` ont une couleur d'accent lisible ;
- `WBP_GridInventory` affiche l'icône de classe et l'accent de classe ;
- la liste du groupe peut afficher une version réduite si les widgets optionnels sont ajoutés ;
- aucune règle de création ou d'inventaire n'a changé ;
- le portrait plein pied équipé reste identifié comme une future tranche séparée.
