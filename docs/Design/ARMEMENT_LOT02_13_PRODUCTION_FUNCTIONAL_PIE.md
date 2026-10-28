# ARMEMENT-LOT02.13 — Validation fonctionnelle et recette PIE

Au 28 octobre 2026, le jalon LOT02.12 a fourni 11/11 DataAssets COMPLETE,
66/66 ressources et le commit Git LFS bd8b3879. La suite industrielle 7/7
a passe sans avertissement. Les onze entrees restent Planned.

## Matrice automatisée sur vrais DataAssets

Le test Grimrock.RPG.ARMEMENT.LOT02.ProductionFunctionalMatrix
charge les onze DataAssets, vérifie leurs identités, leurs références
mesh et icône, les modes de production, les actions et les coûts N1.
Il projette les vingt niveaux de chaque arme dans le modèle de tooltip
(220 profils) et exécute un aller-retour réel du composant d'inventaire
entre sac et emplacement équipé, avec contrôle de l'identité physique.
Il vérifie la génération d'action QuickItem pour les trois projectiles
ARM054, ARM057 et ARM058 et la consommation nominale d'une unité.

Ce test est en contexte Editor : il ne remplace PAS une recette PIE,
ne déclenche pas d'attaque réelle dans un monde, ne lance pas de projectile
physique et ne valide pas une sauvegarde sur disque.

## Commandes locales

Fermer Unreal Editor, puis depuis la racine du projet :

```powershell
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
Import-Csv .\Saved\ARMEMENT-LOT02\LOT02_13_ProductionFunctionalMatrix.csv | Format-Table -AutoSize
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.UI.WeaponTooltip04'
git status --short
```

## Recette PIE sur carte de test isolée

Placer les objets depuis les definitions de la palette, sans modifier
les maps de production. Pour chacun : representation au sol, ramassage,
icône carrée, tooltip N1, équipement, action N1, dépôt/reprise,
sauvegarde/rechargement et vérification de l'identité/du niveau de l'objet.

| Famille | ARM | Conditions à observer en jeu |
| --- | --- | --- |
| Mêlée deux mains | ARM038 | Main secondaire réservée et unique objet physique; portée N1 |
| Arc | ARM048 | Flèches consommées; refus sans flèches; portée et LOS |
| Armes de jet | ARM054, ARM057, ARM058 | Lancer depuis arme et sac; consommation 1, collision, récupération unique |
| Magie offensive | ARM061, ARM062, ARM067, ARM069 | Action N1, PA, portée; aucun soin/brûlure/surface gratuit |
| Focaliseur | ARM070 | OffHand, bonus magique et absorption persistante, aucune attaque |
| Bouclier | ARM081 | OffHand, bonus physique nominal, réserve et recharge non doublées |

## Points bloquants ou à différer

ARM048 : le BuildItem du commandlet générique ne renseigne actuellement
pas RequiredAmmunitionItemDefinitionId. La matrice indique donc
BLOCKED_MISSING_ARROWS_LINK lorsque la valeur est absente ;
un test automatisé vert ne doit pas masquer cette dette fonctionnelle.
Il faudra corriger le pipeline puis migrer prudemment le DataAsset déjà produit.

ARM054 : ancien DA_Weapon_ThrowingKnife toujours utilisé par
Warrior_Skirmisher et Rogue_Shadow et protégé par le test
LegacyThrowingItemsPreserved. Aucune suppression avant migration des
références C++, Blueprint et SaveGame puis non-régression.

Les effets de poison, soin, brûlure ou surface seulement proposés en design
ne sont pas autorisés automatiquement par les nouveaux modèles.
Le passage Planned vers Active et l'activation des kits restent différés.

Critères de clôture : build UE, tests LOT02/Industrial/tooltip,
rapport CSV de 11 lignes et 220 profils, PIE documentée par famille,
absence de blocage de munition et approbation d'activation des kits.

Aucun XLSX local, document Meshy non suivi, niveau ou asset binaire
n'est modifié par ce commit.