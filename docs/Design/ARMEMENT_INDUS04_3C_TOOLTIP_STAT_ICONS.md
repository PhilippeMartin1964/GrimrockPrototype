# ARMEMENT-INDUS04.3C - Icones des statistiques du tooltip natif

**Statut : correctif de compatibilite C++ publie ; recette locale et affectations UMG a effectuer.**

## Contrat

Le widget `WBP_ItemTooltipNativeLine` (parent C++ `UGridItemTooltipWeaponLineWidget`) utilise deja `StatIconTextures`, une Map **Name -> Texture2D** configuree dans ses **Class Defaults**. `Image_StatIcon` et `SizeBox_StatIcon` existent ; `PresentIcon` conserve le SizeBox et met seulement `Image_StatIcon` en `Hidden` quand la texture n'est pas valide. Il n'y a pas de logique a recreer dans le Graph.

La presentation des comparaisons, la taille d'icone choisie librement dans le Designer, les fontes, le cadre, les couleurs, la hauteur du tooltip et ses futures descriptions narratives **ne sont pas modifies**. Chaque ligne reutilise exactement la meme classe native, qu'elle provienne du bloc arme ou d'une simulation avant/apres.

### Compatibilite introduite en 04.3C

- Les StatId `CurrentPhysicalArmor` (armure **restante effective** dans la comparaison globale) et `WeaponPhysicalArmor` (armure physique de profil) peuvent reutiliser la cle historique `Armor` si aucune texture valide n'est affectee a leur cle exacte.
- Une texture explicite pour `CurrentPhysicalArmor` ou `WeaponPhysicalArmor` a priorite sur la cle `Armor`.
- Les autres identifiants restent inchanges, notamment les resistances.
- Pas d'ajout automatique d'icones pour des StatId d'attaque sans visuel approuve.

## Affectations dans Unreal Editor

Ouvrir **Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipNativeLine**. Selectionner *Class Defaults* puis la Map hereditee **Inventory | Tooltip | Designer / Stat Icon Textures** (le panneau peut afficher le nom humanise). Ajouter ou verifier :

| Cle `Name` exacte | Asset `Texture2D` existant |
|---|---|
| `Strength` | `T_Attribute_Strength` |
| `Dexterity` | `T_Attribute_Dexterity` |
| `Constitution` | `T_Attribute_Constitution` |
| `Intelligence` | `T_Attribute_Intelligence` |
| `Wisdom` | `T_Attribute_Wisdom` |
| `Charisma` | `T_Attribute_Charisma` |
| `MaxHealth` | `T_PV` |
| `MaxMana` | `T_Mana` |
| `CarryWeight` | `T_Weight` |
| `FireResistance` | `T_Resistance_Fire` |
| `IceResistance` | `T_Resistance_Ice` |
| `LightningResistance` | `T_Resistance_Lightning` |
| `PoisonResistance` | `T_Resistance_Poison` |
| `HolyResistance` | `T_Resistance_Holy` |
| `NecroticResistance` | `T_Resistance_Necrotic` |
| `ArcaneResistance` | `T_Resistance_Arcane` |
| `Armor` | Choisir une icone d'armure physique dans UE, si disponible |
| `PhysicalResistance` | Choisir une icone de resistance physique dans UE, si disponible |
| `WeaponAP` | `T_Icon_PA` (optionnel, deja present dans le projet) |

Les attributs se trouvent dans `Content/GrimrockPrototype/UI/Portraits/Attributes/`. Les PV, mana, poids, resistances et PA se trouvent dans `Content/GrimrockPrototype/Blueprints/UI/Icons/`.

Ne pas assigner une icone de portee ou de precision arbitraire : `WeaponDice`, `WeaponMinDamage`, `WeaponMaxDamage`, `WeaponAccuracy`, `WeaponRange`, `WeaponDamageType`, `WeaponScaling` peuvent rester sans texture ; la place de l'icone reste reservee a la taille choisie dans le Designer. Ne pas creer un nouveau WBP, ni modifier `WBP_ItemTooltipStatLine` historique.

## Alignement horizontal des icones (04.3C.3)

**Regle : toute ligne reserve l'emplacement de l'icone, avec ou sans texture, a la taille librement reglee dans le Designer.**

La capture du 26 octobre montre le conteneur `SizeBox_1`, alors que le C++ lie facultativement `SizeBox_StatIcon` : ce binding ne peut pas agir sur un widget qui n'a pas le meme nom. Le correctif ne depend plus de ce binding : `Image_StatIcon` est lie correctement et passe en `Hidden` quand il n'a pas de texture. Contrairement a `Collapsed`, **`Hidden` preserve la place dans le layout**, sans dessiner l'image.

Dans `WBP_ItemTooltipNativeLine`, conserver la taille actuelle de **32x32** ou choisir librement **24x24**, ou une autre taille ulterieurement. C++ n'impose pas de largeur ni de hauteur. Aucun nouveau Graph ni renommage de `SizeBox_1` n'est necessaire. Dans `HorizontalBox_Line`, `SizeBox_1` conserve son slot **Size = Auto**, et les dimensions de `Image_StatIcon` et du SizeBox restent entierement gerees par les proprietes Designer.

`Hidden` reserve naturellement la **largeur et la hauteur** de l'Image definies dans le Designer. Ce comportement est volontaire pour aligner tous les libelles ; la gestion future de la hauteur globale du tooltip appartient au jalon narratif.

Verification PIE : `Precision` (sans icone), `Cout en PA` (avec icone) et `Portee (cases)` (sans icone) doivent commencer exactement au meme X. Les lignes sans icone ne montrent pas de rectangle blanc.

## Test et recette

1. Avec Unreal ferme, pull `master` puis executer `ValidateUE.ps1` sur `Grimrock.UI.EquipmentTooltip043C` : **1/1** attendu.
2. Relancer `Grimrock.UI.WeaponTooltip04` : **9/9** ; `Grimrock.UI.EquipmentTooltip043B` : **4/4** ; `Grimrock.UI.Item01` : **5/5**.
3. Ouvrir Unreal, renseigner la Map dans `WBP_ItemTooltipNativeLine`, compiler et sauvegarder cet unique WBP.
4. En PIE : equiper Dague + Bouclier, survoler Grande Hache puis Pic de guerre, verifier le bon icone sur les lignes d'armure restante ainsi que les resistances et deltas. Le deltas Pic contre Dague restent **+3/+3**, sans nouvelle modification de calcul.
5. Verifier les lignes **sans** icone : `Image_StatIcon` passe en `Hidden` mais conserve sa place dans le layout. Les dimensions sont celles du Designer et non du C++ ; aucun rectangle blanc ne doit apparaitre.

### Versionnement local

Le fichier `Content/Localization/StringTables/ST_UI.uasset` est deja modifie par le dernier smoke et attend son commit binaire **distinct**. `WBP_ItemTooltipNativeComparison.uasset` contient la correction manuelle des en-tetes, et attend lui aussi un commit distinct. Ne les restaurer ni ne les ecraser. Le present commit GitHub ne touche pas aux fichiers binaires.

Enregistrer `WBP_ItemTooltipNativeLine.uasset` via Unreal puis publier **separement** les trois changements locaux (en-tetes, `ST_UI`, icones), apres validation. Une verification Git `status --short` precede chaque commit. Aucun succes de compilation locale ou PIE pour 04.3C ne doit etre affirme avant reception des logs et captures.
