# ARMEMENT01.5 — Instances fixes et profils de catalogue au combat

Date : 20 octobre 2026
Statut : C++ publié ; compilation et tests UE5.5.4 locaux non encore confirmés.

## Références et périmètre

ARMEMENT01.4 validé : DA_WeaponCatalogue contient 82 archétypes × 20 niveaux
= 1 640 profils, dont 260 non offensifs. Campagne globale de référence
1 465 réussis, 0 avertissement, 0 échec.

ARMEMENT01.5 relie les nouvelles instances d'objets au catalogue,
sans toucher au moteur multi-dés ARMEMENT01.3 ni migrer automatiquement
les définitions déjà existantes.

## Autorité des données et sauvegarde

FGridItemInstance.ItemLevel : nouvelle UPROPERTY sérialisable.
La valeur 0 signifie Legacy ; 1..20 est le niveau fixe du butin.
Ce niveau appartient à l'instance, indépendamment du niveau de
personnage. Il traverse l'inventaire, l'équipement, le curseur
et les snapshots FGridPartyInventoryState / SaveGame déjà existants.

UGridItemDefinitionAsset : opt-in explicite avec
bUseWeaponCatalogue, WeaponArchetypeId, DefaultWeaponItemLevel
et WeaponCatalogue (référence soft vers le catalogue).

ApplyItemDefinitionToInstance fixe le niveau par défaut seulement
si ItemLevel=0. Un niveau 1..20 déjà porté est préservé, y compris
après un transfert ou un chargement ; le poids est tiré du profil.
Les armes liées au catalogue ne peuvent pas être empilables.

TryGetWeaponCatalogueProfile vérifie la clé (archétype, niveau).
ResolveEquippedOffensiveProfile construit les dégâts réels depuis
cette clé. Il préserve l'identité de l'attaque, ses coûts et les
bonus distincts, mais remplace la définition de dégâts, l'attribut
et la portée par les valeurs du profil. K reste une seule fois
dans DamageDice.FlatBonus, sans second tirage de dés.

Deux circuits sont raccordés :
- attaque normale depuis MainHand/OffHand ;
- action/talent utilisant l'arme équipée et contribution du
  catalogue d'actions.

Un profil passif ne produit aucune attaque, et une référence
manquante n'entraîne jamais de repli silencieux vers les dégâts Legacy.

Les objets déjà existants restent bUseWeaponCatalogue=false et
continuent à utiliser leurs anciens combats et ItemLevel=0.
L'activation sur un DataAsset d'objet de production est manuelle.

## Configuration d'une définition d'arme

1. Cocher bUseWeaponCatalogue.
2. Définir WeaponArchetypeId (ARM001..ARM082).
3. Indiquer DefaultWeaponItemLevel (1..20).
4. Assigner le DataAsset DA_WeaponCatalogue.
5. Conserver au moins une action offensive valide dans CombatActions
   (identité, coût en PA, présentation et source Equipment).
6. L'objet ne doit pas être stackable.

Le catalogue ne remplace pas CombatActions : il ne fournit que le
profil d'équipement correspondant au niveau fixe de l'instance.

Les armes jetées depuis l'inventaire comme QuickItems sont exclues
de la liaison multi-niveaux automatique, car la sélection de
l'instance source doit être résolue avant la projection de l'attaque.

## Tests

Filtre : Grimrock.RPG.ARMEMENT01.5

Six tests couvrent le catalogue de production, le niveau fixé
à la création, les snapshots et métadonnées sérialisées,
la projection de dés pour plusieurs niveaux, les boucliers passifs,
le refus d'une référence invalide, l'inventaire vers l'équipement
et l'absence de régression Legacy.

Validation locale :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.5"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Résultat global attendu si aucune régression : 1 471 tests réussis,
0 avertissement, 0 échec. Il reste à le confirmer par les logs TD04.2.

## Hors périmètre

Rareté, affixes, malus de sur-niveau, forge, runes, création
d'objets de butin à niveau aléatoire, migration automatique des
DataAssets d'armes, travail UMG et fiches d'inspection.
