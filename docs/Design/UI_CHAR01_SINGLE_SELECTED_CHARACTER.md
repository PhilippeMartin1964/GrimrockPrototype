# UI-CHAR01 — Single Selected Character Authority

Date : **20 septembre 2026**  
Statut : **AUTOMATION VALIDÉE — 20 septembre 2026 ; passe UMG/PIE visuelle encore à réaliser**

## Objectif

UI-CHAR01 consolide la règle décidée pour la nouvelle interface :

```text
un personnage sélectionné
    -> une feuille personnage
    -> un paper doll / équipement
    -> un inventaire
```

Le projet possédait déjà la bonne autorité runtime : `UGridPartyInventoryComponent::SelectedCharacterIndex`. Le ticket ne crée donc aucun nouvel état de sélection.

## Existant conservé

- `SetSelectedCharacterIndex()` valide le changement ;
- `FGridInventoryCharacterSummary::bIsSelected` projette l'état vers l'UI ;
- `UGridInventoryWidget::SelectCharacter()` change la sélection puis rafraîchit le workspace ;
- `UGridPartyMemberWidget::OnPartyMemberClicked` remonte le clic ;
- feuille, équipement et inventaire lisent déjà `GetSelectedCharacterIndex()`.

## Six sélecteurs canoniques

`UGridInventoryWidget` expose désormais six bindings optionnels :

```text
PartyMember_1 -> index 0
PartyMember_2 -> index 1
PartyMember_3 -> index 2
PartyMember_4 -> index 3
PartyMember_5 -> index 4
PartyMember_6 -> index 5
```

Lorsqu'ils existent dans `WBP_GridInventory`, ils sont enregistrés automatiquement au `NativeConstruct()`.

Le Graph Blueprint n'a donc plus besoin d'appeler manuellement `RegisterPartyMemberWidget()` pour ces six portraits. Les membres absents restent `Collapsed` via `RefreshRegisteredPartyMemberWidgets()`.

## Visuel de portrait

`UGridPartyMemberWidget` expose maintenant :

```text
Image_Portrait
Border_Selected
```

`Image_Portrait` affiche directement `FGridInventoryCharacterSummary::Portrait`.

`Border_Selected` est un overlay décoratif :

- visible lorsque `bIsSelected == true` ;
- collapsed sinon ;
- il ne doit pas être le parent qui contient tout le portrait.

Aucune couleur n'est imposée par le C++. Le style final reste possédé par UMG.

## Contrat UMG cible

Dans `WBP_GridInventory` :

```text
HorizontalBox_PartySelector
├── PartyMember_1
├── PartyMember_2
├── PartyMember_3
├── PartyMember_4
├── PartyMember_5
└── PartyMember_6
```

Chaque instance est un `WBP_PartyMember` dérivé de `UGridPartyMemberWidget`.

Dans `WBP_PartyMember` :

```text
Overlay_Root
├── Image_Portrait
├── éléments nom/classe/état existants si conservés
└── Border_Selected
```

Le clic visible appelle toujours `HandleClicked()`. Il ne modifie jamais directement un index dans le Blueprint.

## Chemin de sélection

```text
WBP_PartyMember
-> UGridPartyMemberWidget::HandleClicked()
-> OnPartyMemberClicked(CharacterIndex)
-> UGridInventoryWidget::HandleRegisteredPartyMemberClicked()
-> SelectCharacter(CharacterIndex)
-> UGridPartyInventoryComponent::SetSelectedCharacterIndex()
-> RefreshInventory()
```

Le rafraîchissement synchronise ensemble :

- les six portraits ;
- la feuille personnage ;
- le paper doll ;
- les slots d'équipement ;
- la grille d'inventaire ;
- le poids/charge affiché.

## Invariants

1. Un seul `SelectedCharacterIndex`.
2. Aucun index parallèle dans un widget de page.
3. Les portraits ne possèdent aucune donnée gameplay.
4. La feuille et le sac lisent le même personnage sélectionné.
5. Un portrait absent est masqué.
6. Le contour de sélection est uniquement une projection visuelle.
7. Aucun `.uasset` n'est modifié à l'aveugle.

## Automation

Filtre :

```text
Grimrock.UI.Character01
```

Tests :

```text
Grimrock.UI.Character01.SelectionAuthority
Grimrock.UI.Character01.SelectionVisual
```

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Character01"
```

Après la passe UMG :

1. ouvrir l'Inventaire ;
2. cliquer successivement les portraits disponibles ;
3. vérifier qu'un seul portrait est sélectionné ;
4. vérifier que feuille, équipement, sac et poids changent ensemble ;
5. vérifier qu'un emplacement absent reste masqué ;
6. fermer/réouvrir l'Inventaire : la sélection runtime est conservée ;
7. vérifier qu'aucune logique de sélection n'existe dans le Graph Blueprint.


## Validation reçue

Validation locale du 20 septembre 2026 :

```text
Filter                 : Grimrock.UI.Character01
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le contrat C++ UI-CHAR01 est validé. La passe UMG/PIE reste nécessaire pour la validation visuelle complète.
