# ARM-CLEAN06.5H.38 — Bouclier canonique sous dégâts de surface

## Objet

Le test `Grimrock.RPG.SHIELD02.4A` ne charge plus `DA_Weapon_RoundShield_01` pour les deux scénarios d'ARM075 N1. Le monde de test initialise un `UGameInstance` et son `UGridItemSubsystem` **uniquement** lorsque le scénario porte effectivement un bouclier de production. Il résout `Weapon_RoundShield_01` au niveau N1 depuis le registre natif et exige 5 points d'armure physique. Aucun pointeur historique n'est enregistré dans l'inventaire.

Les assertions d'origine demeurent : dégâts physiques absorbés par l'équipement avant la réserve de classe et les HP, dégâts de feu sur la réserve magique, conservation du GUID et concordance du modèle UI.

## Séparation des fixtures

Les scénarios sans bouclier restent des tests historiques synthétiques sans GameInstance : `SHIELD024A_TestMagicFocus` est toujours créé en mémoire, puis enregistré pour contrôler l'absorption de dégâts magiques et le cas sans armure. Cette fixture ne doit pas être confondue avec une définition de production à migrer.

## Validation et périmètre

Aucun code gameplay, `.uasset`, `.umap`, `.sav`, surface, Niagara ni réglage de ciblage n'est modifié. Ne supprimer aucun des 53 packages historiques H.23.

Le prochain commit reste bloqué tant que `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` ne sont pas tous deux SUCCESS sur le SHA H.38, avec compilations Editor/Shipping et automation `Grimrock.RPG.SHIELD02.4A`. Audit natif des sept sauvegardes toujours obligatoire avant toute suppression.
