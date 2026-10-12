# UI-RPG-CONTENT-TEXT04.4 — Final monster category text ownership audit and closure

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT04.4**

## 1. Objet

Clore CONTENT-TEXT04 après séparation de l'identité mécanique des catégories
de monstres et de leur présentation joueur.

Aucun `.uasset` ni `.umap` n'est modifié par ce ticket.

## 2. Autorité finale

```text
DA_MONCAT_*
├─ CategoryId
├─ DisplayName
└─ Description
    => autorité de présentation sérialisée

UGridMonsterDefinitionAsset
├─ CategoryId
└─ CategoryDefinition -> DA_MONCAT_*
    => identité/raccordement bestiaire

FRPGRangerAuthoring
├─ découvre les CategoryId réellement présents
├─ crée/raccorde DA_MONCAT_<CategoryId>
├─ préserve DisplayName sérialisé si CategoryId stable
└─ bootstrap neuf : DisplayName = CategoryId
    => mécanique Editor, aucune prose joueur

GridSkillsPageService
└─ ResolveByCategoryId(CategoryId)
   -> DisplayName sérialisé
   -> texte vide si absent
    => projection runtime, aucune synthèse joueur
```

## 3. Production actuelle

Le bestiaire de production raccordé à cette autorité contient actuellement :

```text
DA_MONCAT_Goblin
DA_MONCAT_Vermin
```

Les deux assets sont structurellement valides et possèdent un
`DisplayName` sérialisé différent de leur bootstrap technique `CategoryId`.

`Description` reste facultative.

## 4. Suppression de la double autorité

TEXT04.1 avait identifié :

```text
ProductionCategorySpecs[]
Goblin -> Gobelins
Vermin -> Vermine
```

TEXT04.2 a d'abord protégé le `DisplayName` sérialisé lors de la
rematérialisation.

TEXT04.3 a ensuite supprimé :

- `FProductionMonsterCategorySpec` ;
- `ProductionCategorySpecs[]` ;
- `FindProductionCategorySpec()` ;
- les littéraux joueur `Gobelins` et `Vermine` du writer de production.

Le C++ Ranger ne connaît donc plus le vocabulaire joueur des catégories.

## 5. Catégories nouvelles

Une nouvelle catégorie découverte dans un monstre de production ne demande plus
de modification du registre C++.

Exemple :

```text
Monster.CategoryId = Undead
        ↓
DA_MONCAT_Undead créé/raccordé
        ↓
DisplayName = Undead   // bootstrap technique seulement
```

Le vrai libellé joueur doit ensuite appartenir au `DA_MONCAT_*`.

## 6. Projection Ranger

`CollectProductionFavoredEnemyCategories()` ne possède aucun nom de catégorie.

Il exige que chaque monstre de production possède une `CategoryDefinition`
valide et cohérente, puis projette directement :

```text
Category.CategoryId
Category.DisplayName
```

Les variantes `Ennemi juré` et leurs lignes de contexte consomment ainsi la
même autorité bestiaire.

## 7. Runtime et absence de fallback joueur

`UGridMonsterCategoryAsset::ResolveByCategoryId()` retourne uniquement un
asset canonique valide dont le `CategoryId` correspond.

`GridSkillsPageService::MonsterCategoryDisplayName()` applique :

```text
DA_MONCAT résolu
    -> DisplayName

DA_MONCAT absent
    -> FText::GetEmpty()
```

Il n'existe pas de fallback `HumanizeId(CategoryId)` ou de transformation
équivalente dans ce chemin.

Cette règle évite qu'un identifiant technique devienne silencieusement un
libellé joueur.

## 8. CategoryId mécaniques sans asset de présentation

Des systèmes peuvent déjà référencer des `CategoryId` purement mécaniques
avant que ces catégories n'entrent dans le bestiaire de production.

Exemple actuel : le Priest utilise `Undead` et `Demon` dans des filtres de
combat.

Ces identifiants :

- sont légitimes comme données mécaniques ;
- ne constituent pas des writers de `DA_MONCAT_*` ;
- n'autorisent pas le C++ à inventer leur libellé joueur.

Tant qu'aucun `DA_MONCAT_Undead` / `DA_MONCAT_Demon` canonique n'existe, le
read-model n'affiche pas de libellé de catégorie pour ces références.

Quand ces catégories devront être présentées au joueur, la couverture devra
être ajoutée par des Data Assets sérialisés, pas par une table C++.

## 9. Tests de clôture

Le filtre :

```text
Grimrock.UI.RPG.CONTENTTEXT04
```

contient désormais :

```text
MonsterCategoryOwnershipAudit
Preservation
BootstrapOnly
FinalAudit
```

`FinalAudit` vérifie :

- les deux `DA_MONCAT_*` de production actuels ;
- leur validité et leur `CategoryId` canonique ;
- l'absence du bootstrap technique dans leur `DisplayName` final ;
- la projection Ranger depuis le même `DisplayName` sérialisé ;
- l'absence de registre de présentation dans l'authoring Ranger ;
- l'absence de writer `FText::FromString` pour MONCAT ;
- le bootstrap technique `FText::FromName(CategoryId)` ;
- le fallback vide du read-model ;
- l'absence de `HumanizeId(CategoryId)`.

## 10. Conclusion

Après TEXT04.4 :

```text
CategoryId      = identité gameplay
DA_MONCAT_*     = présentation joueur
Ranger C++      = découverte / raccordement / bootstrap technique
Runtime UI      = lecture DA_MONCAT ou absence de libellé
```

CONTENT-TEXT04 peut être clos après validation locale.
