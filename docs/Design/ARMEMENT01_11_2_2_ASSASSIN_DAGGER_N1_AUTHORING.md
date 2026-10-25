# ARMEMENT01.11.2.2 - ARM002 (Poignard d'assassin), authoring N1

Statut : code C++ et outillage; compilation Unreal locale et test de cette etape encore a confirmer.
Etat confirme par l'utilisateur : ARMEMENT01.11.2.1 a enregistre ARM002->VIS_AssassinDagger_01;
1/1 sous Grimrock.RPG.ARMEMENT01.11.2, TD04-20261025-075614, zero avertissement;
2/2 sous Grimrock.RPG.ARMEMENT01.11.1, TD04-20261025-075641, zero avertissement;
CSV : ARM002 true/true/true, ItemCount=0, Status=MissingItem.

## Identites et autorites

- Archetype ARM002 ; package DA_Weapon_AssassinDagger_01 ; ItemDefinitionId Weapon_AssassinDagger_01.
- ActionId Attack_AssassinDagger_01. Profil combat, nom, poids, PA, portee, degats et scaling proviennent de DA_WeaponCatalogue niveau 1.
- Rogue/Assassin est le kit approuve ; MainHand explicite ; arme de melee a une main.
- Visuel existant VIS_AssassinDagger_01, dans DA_WeaponVisualCatalogue deja enregistre, avec vraie icone et vrai mesh.
- Ne pas inventer un bonus de critique depuis l'ombre : cette particularite n'est pas implementee par cet authoring.

## Realisation

- GridAssassinDaggerN1ItemAuthoring : sans -Apply, preflight strict en lecture seule, verdict READY/BLOCKED/PRESENT.
- Avec -Apply -Archetype=ARM002 : refuse tout chemin ou archetype/item-id deja occupe ; cree un seul DataAsset.
- Aucune reecriture des catalogues visuel/RPG ; aucune modification de l'infobulle, de l'inventaire, des kits existants et des .umap.
- Script de conduite : Scripts/AuthorARMEMENT011122AssassinDaggerN1.ps1. Les diagnostics sont obligatoires, pas de succes silencieux.
- Deux tests Grimrock.RPG.ARMEMENT01.11.2.2 : verif source+visuel, puis contrat de l'item s'il existe.
- L'absence d'item reste informative avant -Apply ; elle ne demontre pas une arme jouable.
- Ni ce code ni le script ne versionnent des assets utilisateurs sur GitHub.

## Commandes, Unreal Editor ferme

    cd D:\Development\GrimrockPrototype
    git branch --show-current
    git status --short --untracked-files=all
    git pull --ff-only origin master

    .\Scripts\AuthorARMEMENT011122AssassinDaggerN1.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM002

Ne lancer -Apply QUE lorsque le preflight signale READY et que les assets graphiques locaux ont ete inspectes :

    .\Scripts\AuthorARMEMENT011122AssassinDaggerN1.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM002 -Apply

Puis :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.11.1"
    Get-Content .\Saved\ARMEMENT01\ARMEMENT01_11_1_MeleeReadiness.csv
    git status --short --untracked-files=all

Attendu apres application, non encore valide : ItemCount=1, ItemValid=true, MainHandAttack=true,
VisualMapped=true ; statut NeedsSpecialBehaviorReview ou autre conforme a la grille d'audit.

## Recette finale et Git

Verifier en PIE le drop/pickup, l'icone, le rendu et la taille, l'equipement MainHand,
l'attaque N1, les PA, la portee, les degats, et la sauvegarde/restauration.
Le DataAsset genere seul ne valide pas les mecanismes speciaux de l'Assassin.
Preserver les modifications locales WBP_GridCombatTooltip.uasset et L_MainMenu.umap.
Conserver les six assets visuels ARM002, le DA_WeaponVisualCatalogue modifie et le nouveau DataAsset,
puis effectuer un controle visuel et Git LFS avant un commit binaire dedie, sans git add . ni force-push.
