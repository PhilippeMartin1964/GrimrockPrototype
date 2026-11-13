# ARM-CLEAN04 — Vue de travail simplifiée pour les armes

> **ARM-CLEAN06 — NOUVELLE AUTORITÉ (13.11.2026).** [Modèle cible approuvé](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) · [Migration](ARM_CLEAN06_MIGRATION_PLAN.md). **Audit ou jalon de l'ancien modèle ; ne constitue plus l'architecture cible.** **Cible non encore implémentée ; aucun legacy dans le runtime final.**


**12 novembre 2026 — Code éditeur sur `master`, compilation UE5.5.4 et recette visuelle à réaliser localement.**

## Principe

La classe `UGridItemDefinitionAsset` contient 51 propriétés de premier niveau, parce qu'elle est partagée avec les livres, torches, objets lançables, potions et équipements. Il serait dangereux de supprimer ces propriétés pour rendre l'éditeur moins chargé.

Le module `GrimrockPrototypeEditor` enregistre une personnalisation `IDetailCustomization` **uniquement** lorsque `bUseWeaponCatalogue = true` :

- section « Arme — résumé (lecture seule) » : identifiant ARMXXX, niveau par défaut, formule de dégâts et protections des niveaux 1 et 20, PA et portée ;
- rappel explicite des **sources à éditer** : catalogue pour les statistiques, profil de présentation de l'item pour les sons et VFX actuels ;
- catégories d'identité, de catalogue, de présentation d'attaque et de Niagara remontées en priorité ;
- combat avancé, résistances et propriétés rares moins mis en avant ; lecture et QuickItem inutilisés masqués.
- aucune propriété d'item supprimée ni modifiée, aucune conversion automatique de `.uasset`.

**Limite volontaire :** il s'agit d'un **premier écran de diagnostic et de hiérarchisation**, pas encore d'un éditeur interactif unique de croissance N1–N20. Les champs des `CombatActions` restent sérialisés pour préserver l'intégrité des objets en attendant leur migration structurée. Ils ne constituent plus l'autorité des dégâts réels des armes cataloguées.

## Vérification UE locale

1. Fermer Unreal Editor ; compiler `GrimrockPrototypeEditor` (UE5.5.4) avec `Scripts/ValidateUE.ps1`.
2. Ouvrir `DA_Weapon_StraightSword_01` (ARM014). La section « Arme — résumé » doit être visible en tête ; N1 et N20 doivent correspondre au catalogue.
3. Ouvrir `DA_Weapon_RoundShield_01` ou un bouclier catalogué : afficher « passif », aucune attaque imaginaire, mais les deux protections.
4. Ouvrir un item générique `bUseWeaponCatalogue=false` : sa fiche ancienne reste inchangée.
5. Vérifier `AttackAudio`, `ImpactHitAudio`, `ImpactMissAudio` dans la catégorie de présentation ; écouter en PIE sans dégâts sur le monstre de test.
6. Vérifier qu'aucun `.uasset` n'est modifié du seul fait de l'ouverture de sa fiche.

La spécification `docs/Design/ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md` demeure un projet distinct : aucun effet sonore/niveau n'est inventé par cette personnalisation. Le catalogue compact sera adopté après la vérification des 1 640 profils prévue dans `ARM-CLEAN03`.
