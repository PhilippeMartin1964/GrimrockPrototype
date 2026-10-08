# UI-RPG-DESC01.14.1 — Projection C++ structurée du read-model Talent

Date : **8 octobre 2026**  
État : **SOURCE IMPLEMENTÉ — validation locale utilisateur requise**

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

Les six `DA_Class_*` de production ont été matérialisés avant l'existence de
`PresentationType`. DESC01.14.1 garde donc un fallback de migration pour ces
assets **uniquement jusqu'à DESC01.14.2**.

DESC01.14.2 doit :

1. exécuter `Scripts/AuthorUIRPGTalentTree.ps1` ;
2. matérialiser exactement les six `DA_Class_*` ;
3. vérifier la matrice TYPE sur les assets de production ;
4. supprimer le fallback d'inférence ;
5. valider `Grimrock.UI.RPG.DESC01` et les régressions RPG03.

## Dettes gameplay — calendrier obligatoire

Les dettes ne sont pas corrigées dans la projection UI.

Après DESC01.14 et avant DESC01.16 QA six classes :

- **D01** Second souffle : garde-fou PV maximum ;
- **D02** Désamorçage expert / Maître des serrures : conséquences des échecs sûrs ;
- **D03** Sabotage : caller monde ;
- **D05** Repousser les morts-vivants : 4 dégâts fixes ;
- **D06** Réaction en chaîne : raccord bombes -> SurfaceReaction ;
- **D07** Transmutation majeure : durée finale 4 rounds.

**D04** est une dette de présentation bestiaire et doit être réglée avant DESC01.16.

**D08** reste un arbitrage gameplay à prendre avant de clôturer Transmutation majeure :
raccorder réellement le +50 % de réaction ou supprimer ce bonus mort.

**D09** est le chantier Crafting futur ; il n'empêche pas la refonte du panneau Talent.

Aucune dette ci-dessus ne peut être maquillée dans le read-model.
