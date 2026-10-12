# UI-RPG-CONTENT-TEXT02.4 — Final item text ownership audit and residual authority cleanup

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT02.4**

## 1. Objet

Clore le chantier CONTENT-TEXT02 après la séparation entre :

- la présentation sérialisée des items / QuickItems ;
- la mécanique reconstruite par l'authoring Alchimiste ;
- les fallbacks runtime qui projettent cette même présentation sérialisée.

Aucun asset binaire n'est modifié par ce ticket.

## 2. Autorité finale

```text
DA_Item_*
├─ Item.DisplayName
├─ Item.Description
├─ Item.ReadText
├─ QuickItemCombatAction.DisplayName
├─ QuickItemCombatAction.Description
└─ Icon
    => autorité de présentation sérialisée

RPGAlchemistAuthoring.cpp
├─ ItemDefinitionId
├─ ActionId
├─ ItemType / HandUsage
├─ coûts / portée / ciblage / cooldown
├─ statuts / surfaces / armor effects
├─ tags / requirements / scaling
└─ bootstrap technique uniquement
    => autorité mécanique Editor

GridItemDefinitionAsset.cpp
└─ fallback action vide -> présentation du même DA_Item_*
    => projection runtime, pas nouvelle autorité
```

## 3. Audit des writers de DA_Item_*

La recherche globale des créations et mutations de `UGridItemDefinitionAsset`
montre un seul writer de production capable de créer/sauvegarder les onze
QuickItems Alchimiste :

```text
FRPGAlchemistAuthoring::FindOrCreateItem()
FRPGAlchemistAuthoring::ConfigureItem()
FRPGAlchemistAuthoring::SaveAuthoredAsset()
```

Les autres `NewObject<UGridItemDefinitionAsset>` trouvés sont des fixtures de
tests ou des objets transient. Les affectations de `FGridItemInstance.DisplayName`
dans le runtime copient une présentation existante et ne modifient aucun
`DA_Item_*`.

## 4. Authoring Alchimiste

Depuis TEXT02.2, un asset existant identifié par le même
`ItemDefinitionId` préserve exactement :

- `DisplayName` ;
- `Description` ;
- `ReadText` ;
- `QuickItemCombatAction.DisplayName` ;
- `QuickItemCombatAction.Description`.

Une valeur vide est elle aussi autoritaire.

Depuis TEXT02.3, un record neuf ne reçoit plus de prose joueur C++ :

```text
Item.DisplayName                  = ItemDefinitionId
Item.Description                  = AUTHOR_TEXT_REQUIRED:<ItemDefinitionId>
QuickItemCombatAction.DisplayName = ActionId
QuickItemCombatAction.Description = AUTHOR_TEXT_REQUIRED:<ActionId>
```

Les anciens noms français de Bombe incendiaire, Panacée, Flasque acide,
élixirs défensifs, etc. ont disparu de `ConfigureItem()`.

## 5. Major Transmutation

`BuildMajorTransmutationRecipeAction()` reste une frontière Editor mécanique
pour le futur crafting. Il ne possède plus de prose joueur.

Son texte bootstrap est dérivé uniquement de
`Action_Alchemist_MajorTransmutation` et du marqueur
`AUTHOR_TEXT_REQUIRED`.

Le futur système crafting devra fournir une présentation sérialisée à cette
contribution s'il l'affiche directement ; il ne doit pas réintroduire une
chaîne joueur codée en dur dans le helper mécanique.

## 6. Fallback runtime

`BuildQuickItemCombatActionDefinition()` et
`BuildInventoryCombatActionDefinition()` utilisaient deux blocs identiques
pour compléter une action sans présentation.

TEXT02.4 remplace cette duplication par un helper local unique :

```text
ApplySerializedItemPresentationFallback(Item, Definition)
```

Règles :

1. un `QuickItemCombatAction.DisplayName/Description` non vide gagne ;
2. sinon le runtime utilise `DA_Item_*.DisplayName/Description` ;
3. aucune prose C++ n'est créée ;
4. l'icône suit la même règle de fallback depuis le `DA_Item_*`.

Le HUD peut réappliquer le même fallback lors de l'enrichissement d'une action
liée, mais il copie encore uniquement le même `DA_Item_*`. Cela n'est donc
pas une seconde autorité.

## 7. Fallbacks techniques hors authoring

Des fallbacks comme :

```text
FText::FromName(ItemDefinitionId)
```

existent encore pour des situations où aucune définition résolue ou aucun nom
sérialisé n'est disponible, par exemple certains chemins de loot ou de palette.

Ils sont des identifiants techniques de secours et ne constituent pas des
writers de `DA_Item_*`. Ils ne contiennent aucune formulation joueur métier.

## 8. Tests

Le filtre :

```text
Grimrock.UI.RPG.CONTENTTEXT02
```

contient désormais quatre tests :

```text
ItemOwnershipAudit
Preservation
BootstrapOnly
FinalAudit
```

`FinalAudit` vérifie pour les onze QuickItems Alchimiste :

- chargement du `DA_Item_*` de production ;
- absence de marqueur bootstrap dans la présentation item de production ;
- fallback vers `Item.DisplayName/Description` lorsque l'action est vide ;
- priorité de la présentation explicite de l'action lorsqu'elle existe.

## 9. Hors périmètre

Les textes de `DA_Status_*` restent une autorité séparée et ne sont pas
modifiés par CONTENT-TEXT02.

Les textes UI génériques comme les raisons d'indisponibilité du HUD ne sont pas
des textes d'item et restent hors de ce chantier.

## 10. Conclusion

Après TEXT02.4, il n'existe plus de double autorité C++ / asset pour la
présentation des onze QuickItems Alchimiste.

Le C++ reconstruit la mécanique et fournit uniquement des marqueurs techniques
pour les records neufs. La présentation joueur appartient aux données
sérialisées.
