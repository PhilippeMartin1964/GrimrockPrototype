# UI-RPG-DESC01.14.1 — Projection C++ structurée du read-model Talent

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS — source, assets de production et Automation validés le 8 octobre 2026**

## Objet

Première moitié de DESC01.14. Aucun UMG ni DataAsset binaire n'est modifié.

Implémenté :

- enum explicite `ERPGTalentPresentationType` ;
- authoring C++ des six classes pour les 90 nœuds ;
- champs structurés TYPE / STATUT / PRINCIPE / EFFETS / UTILISATION / ACQUISITION ;
- statut d'acquisition distinct de toute disponibilité d'action ;
- cooldowns affichés en **rounds** dans le nouveau read-model ;
- cible Ally normalisée « soi-même ou un allié vivant » ;
- présentation conceptuelle des nœuds groupés lue depuis `DA_RPGTalentPresentation` lorsqu'elle existe ;
- test source de la matrice TYPE 90/90 ;
- maintien temporaire des anciens champs du read-model uniquement pour que le WBP actuel reste compatible jusqu'à DESC01.15.

## Frontière binaire

Le 8 octobre 2026, l'utilisateur a exécuté `Scripts/AuthorUIRPGTalentTree.ps1`.
Le build Development Editor a réussi, exactement les six `DA_Class_*` ont été
modifiés, `Grimrock.UI.RPG01.ProductionAssets` a réussi 5/5 et
`Grimrock.RPG.RPG03` a réussi 193/193, sans warning ni échec.

DESC01.14.2 supprime maintenant le fallback d'inférence : le read-model projette
directement `Choice.PresentationType`. Un nouveau test de production exige que
les six assets matérialisés portent explicitement la matrice TYPE 90/90.

Validation finale reçue le 8 octobre 2026 :

```text
Grimrock.UI.RPG01.ProductionAssets   6/6
Grimrock.UI.RPG.DESC01             14/14
Grimrock.RPG.RPG03                193/193
warnings                              0
échecs                                0
```

Les six `DA_Class_*` ont été commités dans
`a42c62c0 UI-RPG-DESC01.14.2 materialize explicit Talent types`.
DESC01.14 est clos.

## Dettes gameplay — calendrier obligatoire

Les dettes ne sont pas corrigées dans la projection UI.

Après DESC01.14 et avant DESC01.16 QA six classes :

- **D01** Second souffle : **CLOS** — garde-fou générique confirmé par test spécifique ;
- **D02** Désamorçage expert / Maître des serrures : **DIFFÉRÉ LOCK/TRAP** — primitives SafeFailure déjà conformes ;
- **D03** Sabotage monde : **DIFFÉRÉ ACTIONS HORS COMBAT** — cible/difficulté/événement existent déjà ;
- **D05** Repousser les morts-vivants : **CLOS** — 4 dégâts fixes validés 21/21 ;
- **D06** Réaction en chaîne : **CLOS** — 9/9 RPG03.9.6A + 7/7 RPG03.9.6B1 ;
- **D07** Transmutation majeure : **CLOS** — durée finale fixe 4 rounds validée 10/10 + 4/4 + 7/7.

**D04** est une dette de présentation bestiaire et doit être réglée avant DESC01.16.

**D08** reste un arbitrage gameplay à prendre avant de clôturer Transmutation majeure :
raccorder réellement le +50 % de réaction ou supprimer ce bonus mort.

**D09** est le chantier Crafting futur ; il n'empêche pas la refonte du panneau Talent.

Aucune dette ci-dessus ne peut être maquillée dans le read-model.
