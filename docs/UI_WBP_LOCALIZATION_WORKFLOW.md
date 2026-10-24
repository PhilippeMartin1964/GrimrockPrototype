# Procédure obligatoire — Localisation des textes WBP (UE 5.5.4)

**Politique permanente — 24 octobre 2026.** Déclencheur : création, modification ou suppression de texte visible dans un WBP, tooltip, HUD ou widget natif.

**STOP impératif : ne jamais demander de construire un WBP référençant une nouvelle clé ST_UI tant que cette clé n'a pas été matérialisée dans le vrai asset Unreal ST_UI.uasset ET vérifiée dans le Content Browser.** Une ligne ajoutée au CSV Git n'est PAS automatiquement disponible dans le WBP.

Références de conception : [UI-LOCALIZATION01.3](Design/UI_LOCALIZATION01_3_UI_STRING_TABLE_CSV_AUTHORING_CONTRACT.md), [UI-LOCALIZATION01.4A](Design/UI_LOCALIZATION01_4A_STRING_TABLE_CSV_IMPORTER.md), [UI-TEXT-DIRECT01](Design/UI_TEXT_DIRECT01_DIRECT_ST_UI_BINDINGS.md), [UI-LOCALIZATION01.6A](Design/UI_LOCALIZATION01_6A_UNREAL_LOCALIZATION_TARGET.md).

## 1. Les cinq étapes qu'il ne faut plus confondre

| Étape | Fichier / action | Rôle |
|---|---|---|
| 1. **Source** | `Localization/Source/UI.csv` | Clés + texte français + commentaire, versionnés en Git |
| 2. **Matérialisation** | `UnrealEditor-Cmd.exe -run=UIStringTableAuthoring` | Valide le CSV, produit l'asset Unreal |
| 3. **String Table** | `Content/Localization/StringTables/ST_UI.uasset` = `/Game/Localization/StringTables/ST_UI` | Clés réellement sélectionnables dans le Designer |
| 4. **WBP** | `/Game/GrimrockPrototype/Blueprints/UI/.../WBP_...` | Référence statique `ST_UI + Key`, ou valeur dynamique du C++ |
| 5. **Traductions** | Target `Game`, `Config/Localization/Game_*.ini`, `Content/Localization/Game/` | Gather, PO anglais, import, compilation locres, test fr/en |

Le succès des étapes **1** ou **2** ne prouve pas **3–5**. Ne jamais écrire un faux `.uasset` ou modifier une String Table binaire hors des outils Unreal.

## 2. Choisir l'autorité du texte AVANT de coder

- **Label/titre/bouton statique partagé** : vérifier les clés existantes dans `UI.csv` ; si nécessaire, ajouter une clé stable. Le WBP lie directement sa propriété `Text` à `ST_UI` dans le Designer. **Ne pas créer un relais C++ `SetText()` pour recopier ce même libellé.**
- **Texte dynamique ou formaté** : la valeur vient de C++ / read-model ; wording via `FUILocalizationText::Get(Key)` et `FText::Format`, ou `LOCTEXT` si sa collecte Unreal est vérifiée. Le champ dynamique du WBP n'a pas de binding concurrent.
- **Nom ou description d'objet, talent, sort, bouclier** : préserver le `FText` du DataAsset du domaine. Pas de doublon de contenu authoré dans `ST_UI`.
- **Notation mécanique** : calculée par C++ (`+1`, `1/5`, pourcentages).
- **Texte réellement local à un écran** : peut rester un `FText` localisable dans le WBP si le choix est intentionnel et documenté (contrat UI-TEXT-DIRECT01) ; contrôler quand même sa culture cible.

**Une seule autorité par propriété FText.** En UE5.5, `UTextBlock::SetText()` remplace le contenu de la propriété et peut effacer son binding. Interdit de cumuler référence statique `ST_UI` du Designer et `SetText()` qui réécrit la même valeur.

Les clés nouvelles doivent être uniques, stables et respecter l'interdiction du token `Grimrock` dans les identifiants de localisation (UI-LOCALIZATION01.3).

## 3. Procédure de bout en bout — ordre de travail obligatoire

### A. Définir les textes et les chemins

1. Fournir **le dossier exact du WBP dans `/Game`** et son chemin disque sous `Content/`, sa classe C++ parente, son nom et la hiérarchie complète des widgets UMG.
2. Inventorier tous les TextBlock : label, valeur, origine du `FText`, clé ou DataAsset. Statique et dynamique ne sont pas interchangeables.
3. Rechercher la clé dans `UI.csv`. Ajouter uniquement les clés centrales manquantes au format `Key,SourceString,Comment` (CSV UTF-8).
4. Vérifier le texte source et les doublons, par exemple :

   ~~~powershell
   cd D:\Development\GrimrockPrototype
   Import-Csv .\Localization\Source\UI.csv | Where-Object Key -eq 'Combat.Tooltip.Title' | Format-Table Key,SourceString -AutoSize
   ~~~

5. Pour un `LOCTEXT` C++ ou texte local WBP, documenter sa collecte/traduction séparément : la commande ST_UI ne traite pas toute la localisation du jeu.

### B. Matérialiser la vraie String Table AVANT l'UMG

1. Vérifier `git status --short`, préserver les changements locaux et mettre le checkout à jour si nécessaire avec `git pull --ff-only origin master`.
2. **Fermer Unreal Editor et UnrealEditor-Cmd** ; compiler l'éditeur si le code vient de changer.
3. Dans PowerShell, à la racine du projet, exécuter :

   ~~~powershell
   cd D:\Development\GrimrockPrototype
   & "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
   if ($LASTEXITCODE -ne 0) { throw "Echec materialisation ST_UI : $LASTEXITCODE" }
   ~~~

4. Chercher le message réel :

   ~~~text
   [UI-LOCALIZATION01.4A] ST_UI materialization succeeded. Changed=true Source=...
   ~~~

   `Changed=false` est également correct **si les clés recherchées sont déjà présentes**. Code de sortie **0** obligatoire.

5. Confirmer le fichier binaire et regarder les modifications Git, **sans les écraser** :

   ~~~powershell
   Test-Path .\Content\Localization\StringTables\ST_UI.uasset
   git status --short -- .\Content\Localization\StringTables\ST_UI.uasset
   ~~~

6. **Rouvrir UE**, ouvrir `/Game/Localization/StringTables/ST_UI` et rechercher chacune des nouvelles clés. Une clé présente dans `UI.csv` mais absente de `ST_UI` signifie **STOP : le WBP ne doit PAS être commencé**. Diagnostiquer le log commandlet, le projet ouvert et le CSV, sans ajouter manuellement un doublon dans le Designer.

**GATE OBLIGATOIRE : preuve de la présence réelle des clés dans ST_UI.**

### C. Construire et raccorder le WBP

1. Créer le WBP dans son dossier exact et choisir le **bon parent C++** ; vérifier les noms/types `BindWidget` / `Is Variable`.
2. Pour chaque label statique : `Details → Content → Text → Referenced Text / String Table → ST_UI → Key`. Ne pas recopier le français.
3. Les champs de données dynamiques reçoivent un `FText` fourni par C++ / read-model. Ne pas leur donner un binding Blueprint concurrent.
4. Compiler/enregistrer le WBP ; tester en PIE, avec capture des valeurs et des infobulles.

### D. Gather / traduire / compiler (target Game)

**Une clé affichée en français dans ST_UI n'est PAS une preuve de traduction anglaise.**

1. Le target Unreal `Game` utilise `fr` comme langue native et `en` comme langue cible : voir `Config/Localization/Game_*.ini`.
2. **Gather** les textes à traduire, y compris les nouvelles clés ; vérifier leur présence dans les données collectées.
3. **Export** PO, traduire réellement et relire les nouvelles entrées du `Game.po` anglais.
4. **Import** PO, **Compile** les `Game.locres`, tester les deux cultures dans Unreal.
5. Ne pas présumer que le target historiquement configuré pour la String Table collecte tous les nouveaux `LOCTEXT` C++ : vérifier cette couverture séparément avant tout statut « EN complet ».

Le smoke technique existant est :

~~~powershell
.\Scripts\Localization\RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5
~~~

Ce script synchronise ST_UI et teste gather, export, un **échantillon** de traductions anglaises, import, compile et exécution. **Il ne traduit PAS toutes les nouvelles chaînes en anglais.** Il restaure par défaut les sorties transitoires suivies dans Git mais peut mettre à jour `ST_UI.uasset` ; ne pas confondre smoke réussi et localisation livrée.

### E. Valider puis versionner les bons fichiers

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Game.UI.Localization.LOCALIZATION01"
git status --short
~~~

Ajouter les tests du domaine WBP (par exemple `Grimrock.UI.EventHUD01`), les contrôles en PIE/fr/en, puis inspecter :
- `Localization/Source/UI.csv` (source),
- `Content/Localization/StringTables/ST_UI.uasset` (projection réelle),
- les WBP `.uasset` modifiés dans UE,
- les artefacts Game PO/locres/manifest lorsque la traduction de production est concernée.

Respecter **un seul commit atomique par étape sur master** (`AGENTS.md`). Si ChatGPT ne peut pas matérialiser le binaire dans l'environnement du développeur, le signaler comme **non terminé**, exécuter localement les commandes, vérifier l'asset et ne pas prétendre à une livraison complète. Le commit du CSV seul **ne clôt pas** le travail.

## 4. Checklist à reporter dans CHAQUE ticket impliquant un texte WBP

- [ ] WBP localisé : dossier logique `/Game`, chemin disque, parent C++, hiérarchie, types.
- [ ] Tous les textes inventoriés et classés par autorité (statique, dynamique, DataAsset, local WBP, mécanique).
- [ ] Clés existantes réutilisées et clés nouvelles ajoutées sans doublon dans `UI.csv`.
- [ ] **`UIStringTableAuthoring` exécuté, code de sortie 0 ; `Changed=true/false` vérifié.**
- [ ] **TOUTES les nouvelles clés recherchées dans le vrai asset `ST_UI` ouvert dans Unreal AVANT l'authoring UMG.**
- [ ] WBP lié à la String Table, sans texte français recopié ni `SetText` redondant.
- [ ] Modèles dynamiques et DataAssets sans nouvelle autorité de texte concurrente.
- [ ] Gather et traduction Game fr/en vérifiés, ou « EN à faire » indiqué expressément.
- [ ] Tests locaux et rendu PIE validés par log/capture.
- [ ] CSV, ST_UI, WBP et ressources linguistiques inspectés et versionnés.
- [ ] **Aucun statut VALIDÉ tant que les étapes requises ne sont pas démontrées.**

## 5. Incident de référence ARMEMENT01.10.3.3

Les 14 clés `Combat.Tooltip.*`, notamment `Combat.Tooltip.Title`, ont été publiées dans `UI.csv` au commit `47f4d077`. Le binaire `ST_UI.uasset` n'avait pas encore été régénéré : la clé était donc **absente du Designer**, malgré sa présence dans Git. Ce workflow rend obligatoire la matérialisation/inspection de `ST_UI` **avant** de recommencer la construction de `WBP_GridCombatTooltip`. Ce document **ne matérialise pas lui-même** le binaire.
