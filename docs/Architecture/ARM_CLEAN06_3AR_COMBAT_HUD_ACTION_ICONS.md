# ARM-CLEAN06.3AR — Icônes d'actions du HUD, priorité canonique

**Parent :** 06.3AQ `9791bac` ; validation automatique complète verte.

## Constat et correction

Le catalogue d'actions de combat (06.3AL/06.3AP) projette une
icône d'**ACTION** depuis `DA_ItemSystem.WeaponVisualCatalogue.ActionIcons`.
Mais `UGridCombatHudWidget::EnrichHotbarActionPresentation`
écrasait parfois cette icône à partir de l'ancien
`UGridItemDefinitionAsset.Icon` ou du `WeaponVisualCatalogue`
historique : les icônes d'**OBJET** contaminaient la barre d'actions.
L'alias PrimaryAttack n'est pas exempt de ce second passage.

Le lecteur de présentation
`FGridCombatHudCanonicalActionReadModel` :
- vérifie l'appartenance au registre et distingue le
  `SourcePolicy::Equipment` des spells/quick items ;
- traite un ItemId enregistré sans fallback vers `DA_Weapon_*`,
  **y compris quand le GUID est périmé ou ItemLevel=N0 invalide** ;
- valide le slot réel, la propriété physique et l'action liée à
  l'instance pour les attaques individuelles ;
- préserve le libellé spécifique à l'action s'il est déjà renseigné ;
  emploie le nom/description du registre seulement pour un champ vide ;
- projette exclusivement l'icône du catalogue visuel global ;
- gère l'identifiant spécial `Attack_DualWield` comme un composite
  déjà autorisé par la politique de deux armes, sans inventer une
  `ItemActionBinding` manquante dans le registre ;
- laisse le chemin historique aux ItemId inconnus et aux
  quick items non équipés ; aucun effet sur l'icône d'inventaire.

L'enrichissement HUD conserve l'ancien traitement des raisons de
désactivation (clés `Combat.Action.Unavailable.*`), les raccourcis
et les coûts d'action ; seul le choix des métadonnées visuelles
d'une action d'équipement change.

## Tests et recettes

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatHudActionIconProvenance`
avec le véritable registre vérifie ARM014 N1/N20, un GUID périmé,
une action inventée, un item manquant, une arme N0 invalide, le
composite dual wield, un bouclier passif, et les identités inconnues
ou actions QuickItem.

Les tests de 06.3AL/06.3AP restent couverts par la suite `Grimrock`.

### Checklist localisation UI/WBP (début et fin)

Les chaînes visibles existantes (`Action.DisplayName`,
`Action.Description`, `Combat.Action.Unavailable.*`) ne sont pas
modifiées ; aucune chaîne créée ou supprimée.

| Étape | État |
|---|---|
| `UI.csv` | inchangé |
| `ST_UI` | inchangé ; aucune nouvelle clé |
| `WBP` | inchangé ; présentation WBP existante |
| `Game/fr/en` | inchangé ; aucune nouvelle traduction |
| PIE/GPU | à contrôler visuellement, non attesté par `-NullRHI` |

Pas de modification binaire `.uasset`, `.umap` ou SaveGame.
