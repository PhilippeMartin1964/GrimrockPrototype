# ARM-CLEAN06.3AV — Projectile physique ARM spécialisé sans DA_Weapon historique

**Parent :** ARM-CLEAN06.3AU (`629dddf`).

## Diagnostic vérifié

`RehydrateOwnedItemDefinitions` admet les identités canoniques sans
`UGridItemDefinitionAsset`, mais la chaîne de lancer exigeait encore
ce pointeur à plusieurs étapes :

1. `FGridCanonicalThrowBallisticsReadModel` vérifiait d'abord l'ancien asset
   et son opt-in `bUseWeaponCatalogue`, **avant** `DA_ItemSystem`.
2. `FGridPhysicalThrowStrengthReadModel` lisait la masse canonique, mais
   exigeait toujours un ancien asset pour le seuil de Force et l'accès à
   la vitesse. Le test 06.3U n'exerçait pas le chemin sans définition.
3. Le contrôleur et le curseur souris refusaient l'objet si la définition
   historique était absente ; le menu affichait à tort « définition
   indisponible », même pour une identité enregistrée.
4. Les cinq chemins du Pawn et le spawner de projectile exigeaient
   `Definition->Weight`, `IsPhysicallyThrowable` ou
   `IsCombatThrowable`.
5. `AGridThrownItemActor::InitializeThrownItem` ne savait pas démarrer
   depuis le seul `ItemId` ; l'atterrissage et le transit par une fosse
   étaient bloqués par une absence d'ancien asset.

## Adaptation : uniquement projectiles spécialisés déjà authorés

Les ARM dont le `DA_ItemSystem` valide l'`ItemId` à **N1–N20**
et possède `Weapon.Projectile.bEnabled` utilisent désormais directement :

- `InitialSpeed` et `Arc` depuis `ProjectilePolicy` à niveau réel ;
- `WeightKg` depuis l'item canonique, jamais le cache SaveGame ni
  `DA_Weapon_*.Weight` ;
- la politique de main (`HandPolicy` et `CanEquipInHand`) réelle ;
- le seuil physique **inchangé** de `0,25 kg × Force` et le coefficient
  existant `sqrt(Force/10)` avec pénalité de poids ;
- `ProjectileMesh`, transformation de vol, rotation, durée de vie et
  décalage d'impact depuis la définition et le catalogue visuels ;
- un objet lancé et récupérable identifié par `ItemId`, `ItemLevel`,
  `RuntimeObjectId`, sans exiger de pointeur `DA_Weapon_*`.

Le contrôle de disponibilité depuis le menu contextuel, le curseur,
le contrôleur et le Pawn s'appuie sur la même règle de Force et de
niveau. Un identifiant enregistré mais invalide, notamment un ARM
N0 persistant ou N21, ne peut pas utiliser une vitesse N1 ancienne.

Le contrôle `bCombatThrowWeapon` n'est contourné que pour une arme
dont le **propre profil canonique** autorise un projectile spécialisé,
sur les entrées de combat déjà sélectionnées par le système d'actions.
Aucune attaque ni raccourci fictif n'est créé.

La création du projectile admet ce profil sans ancien asset.
`AGridThrownItemActor` initialise alors l'identité depuis `ItemId`,
applique la présentation canonique et produit au contact un world item.
La chute en fosse et le pending item transportent cette identité sans
pointeur historique. L'autorité des GUID, niveau et emplacements est
conservée, sans nouveau champ SaveGame.

## Portée **volontairement limitée**

- **Lancers physiques ordinaires** d'un ARM sans profil de projectile
  spécialisé : leur `ThrowSpeed/ThrowArc` ne sont pas authorés dans
  `DA_ItemSystem`. Une ancienne définition strictement correspondante
  reste provisoirement nécessaire ; en son absence le lancer est refusé.
  **Aucune vitesse 1800 arbitraire n'est inventée.**
- **Flèches N0 via arc/arbalète** : leur chemin de combat distinct
  `bRangedAmmunitionProjectile` conserve la vérification de l'ancien
  asset, le temps de convertir son mécanisme propre.
- **Objets hors registre** : compatibilité physique et lancers existants
  conservés lorsqu'un asset historique correspondant est disponible.
- **Autres interactions** (Consommer/Lire/Hotbar, lumière, tooltips
  complets) : certaines restent historiques. La conversion 06.4 et
  la suppression définitive 06.5 ne sont pas encore autorisées.

## Tests et limites

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy` :

- vrai `DA_ItemSystem` et niveaux N1/N20 des trois ARM de jet
  `Weapon_ThrowingKnife_01`, `Weapon_Javelin_01`,
  `Weapon_Dart_01` ;
- vitesse/arc, masse, Force et permission sans ancien asset, y compris
  face à des champs historiques volontairement contradictoires ;
- monde UE transitoire avec cellule praticable ; vérification de création
  réelle d'un `AGridThrownItemActor` **sans** définition historique,
  mesh de vol canonique, dépôt au sol sous le même GUID et mesh récupérable ;
- ARM invalides N0 et N21 strictement rejetés avant spawn ; source
  inconnue sans ancien asset rejetée ; échelle legacy intacte ;
- suite complète `Grimrock` et contrôles Windows sur le SHA final.

Les tests `-NullRHI` **ne prouvent ni la trajectoire GPU en PIE,
ni le résultat Chaos et les animations sur le poste utilisateur,
ni un test réel multi-niveau en présence de fosses**. Prévoir une
recette PIE différenciée au cutover 06.6.

## Localisation : contrôle en entrée et en sortie

`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en` :
**inchangé**. Aucun texte, clé, WBP, String Table ou locres ajouté.
Les messages de lancer existants continuent à être obtenus depuis
`FUILocalizationText::Get` ; aucun `SetText()` ni arbre UMG n'est
modifié. Aucune nouvelle matérialisation `ST_UI` requise.

**Ce jalon n'est pas le cutover binaire des assets ni une preuve
du retrait de l'ensemble du legacy runtime.**
