# ARMEMENT-INSPECTION02.5 — Contrat d'authoring du WBP joueur

## Préconditions et portes

- 02.5A `f7773143`, 02.5B.1 `2f573988`, route 02.5B.3 `d778ec76` : les deux statuts Windows success (Editor + Shipping + Grimrock).
- Le présent lot complète les faits non-ARM (bonus et résistances), la réserve magique sous forme restante/maximale et les actions réellement résolues, sans déclencher d'action. Les actions détaillées reprennent leur DisplayName et coûts nominaux ; les compactes exposent leur catégorie et coût d'attaque connu seulement, pas de nom technique ni permission d'exécuter. Les prérequis détaillés, réactions et effets conditionnels ne sont pas encore projetés ; ils restent un approfondissement après la recette de base.
- Le CSV est préparé, mais ST_UI et traduction Game ne sont PAS matérialisés. Aucun nouveau libellé dynamique n'est lu par C++ avant matérialisation. Il faut valider le SHA de ce lot et franchir la porte ST_UI avant de créer le WBP. Ne jamais fabriquer un .uasset hors UE.

## Étape native 1 — localisation

Dans le checkout propre, synchronisé sur master, fermer UE. Utiliser le commandlet existant :

~~~powershell
cd D:\Development\GrimrockPrototype
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) { throw "Echec materialisation ST_UI : $LASTEXITCODE" }
~~~

Vérifier code 0, log de matérialisation et les 12 clés suivantes dans `/Game/Localization/StringTables/ST_UI` ouvert dans UE. Le tableau contient les traductions EN proposées à importer dans Game.po, non encore appliquées. Ne commencer le Designer qu'après preuve.

| Clé | FR | EN |
|---|---|---|
| Item.Examination.Title | Examiner | Examine |
| Item.Examination.PreviewUnavailable | Aperçu 3D indisponible | 3D preview unavailable |
| Item.Examination.View.Front | Face | Front |
| Item.Examination.View.Side | Profil | Side |
| Item.Examination.View.Back | Dos | Back |
| Item.Examination.View.Reset | Recentrer | Reset view |
| Item.Examination.ControlsHint | Glisser gauche : tourner · glisser droit ou milieu : déplacer · molette : zoom | Left drag: rotate · right or middle drag: pan · wheel: zoom |
| Item.Examination.Weight | Poids unitaire (kg) | Unit weight (kg) |
| Item.Examination.Quantity | Quantité | Quantity |
| Item.Examination.Stats | Caractéristiques | Item attributes |
| Item.Examination.Actions | Actions associées | Bound actions |
| Item.Examination.Close | Fermer | Close |

Réutiliser `UI.Section.Description` pour le titre de description et `Navigation.Back` si nécessaire. Les noms/descriptions d'objet viennent du registre ; les valeurs dynamiques et niveaux sont alimentés en C++.

## Étape native 2 — Designer

Créer **un seul** WBP : `/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemExamination` (parent **GridItemExaminationWidget**). Class Defaults : Is Focusable, StatRowWidgetClass = `WBP_ItemTooltipNativeLine` existant (vérifier ses deux TextBlocks Text_Label et Text_Value). Aucun Event Graph nécessaire. Conserver les éléments suivants exactement nommés et Is Variable pour les liaisons natives :

~~~text
Overlay_Root
  Border_ModalBackdrop
  Border_ExaminationFrame
    VerticalBox_Content
      HorizontalBox_Header
        Text_Title
        Text_ItemName
        Border_LevelBadge
          Text_ItemLevel
        Button_Close
          Text_Close
      HorizontalBox_Main
        VerticalBox_Preview
          SizeBox_Preview
            Overlay_Viewport
              Border_Black
              Image_RenderTarget
              Image_FallbackIcon
              Text_PreviewUnavailable
          HorizontalBox_ViewControls
            Button_Front
              Text_Front
            Button_Profile
              Text_Profile
            Button_Back
              Text_Back
            Button_Reset
              Text_Reset
          Text_ControlsHint
        ScrollBox_Details
          VerticalBox_Details
            Text_DescriptionTitle
            Text_Description
            HorizontalBox_Weight
              Text_WeightLabel
              Text_Weight
            HorizontalBox_Quantity
              Text_QuantityLabel
              Text_Quantity
            Section_Stats
              Text_StatsTitle
              VerticalBox_Stats
            Section_Actions
              Text_ActionsTitle
              VerticalBox_Actions
~~~

Section_Stats et Section_Actions sont des VerticalBox ; Border_LevelBadge un Border ; Image_* des Image ; Button_* des Button ; Text_* des TextBlock. VerticalBox_Stats et VerticalBox_Actions sont vides au Designer. Tous les noms déclarés dans GridItemExaminationWidget.h sont obligatoires en pratique (HasDesignerContract). Les autres noms sont libres. Les TextBlock statiques réfèrent directement aux clés du tableau ; pas de liaison de texte dans le Graph. Les valeurs Text_ItemName, Text_ItemLevel, Text_Description, Text_Weight, Text_Quantity restent dynamiques. Les styles/marges/polices restent au Designer.

Proposition de disposition : cadre centré, fenêtre couvrant environ 75–85 % de l'écran, viewport carré, noir pur, image blanche opaque (pas de tint noir), brush sans asset fixe. Root plein écran : backdrop hit-testable, contenu visible/self-hit-test invisible selon besoin ; les images peuvent être hit-test invisible (le parent gère la souris). Le fond et le cadre sont distincts. Ne pas étirer le RT carré. Pas de nouveau matériau UI avant preuve de nécessité : affectation directe du RT existante, rendu/alpha/gamma à vérifier.

## Étape native 3 — raccordement et recette

Dans `/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPartyPawn`, Class Defaults > Inventory | UI > Item Examination Widget Class = WBP_ItemExamination. Sauvegarder, compiler. Ne pas remplacer les deux WBP d'inventaire et ne pas retirer PresentItemExamination avant validation du nouveau chemin. Vérifier PIE : clic droit sac et main équipée, épée N5, bouclier réserve, clé/pierre/livre N0, fermeture Escape/X, rotation/zoom/pan, aucune commande de mouvement/attaque, pas de perte du curseur, fond noir et lumière PBR, retour au même inventaire, action Lire inchangée. Tester changement carte et combat, fallback sans mesh, plusieurs ouvertures/fermetures. Puis Shipping, cultures fr/en et cook. Ne pas déclarer recette achevée sans ces preuves.

Le rendu et les effets animés dans GamePreview nécessitent une vérification GPU ; l'agent NullRHI ne peut pas les établir. Aucun changement de gameplay, sauvegarde ou progression N1–N20.


## Diagnostic première ouverture

La capture du 28.11 confirme les douze clés ST_UI présentes localement. Le propriétaire a choisi le dossier Inventory. En cas de texte bleu Print String, l'ancien événement PresentItemExamination est encore appelé comme repli ; ne pas modifier le Graph avant diagnostic. Filtrer Output Log sur `[ItemExamination]` : contexte, classe pawn non assignée, source physique, CreateWidget, BindWidget/StatRowWidgetClass, ligne Text_Label/Text_Value ou présentation/viewport. Le succès donne `Opened Class=...`. Le repli est conservé jusqu'à validation de la route native. La hiérarchie doit placer ScrollBox_Details directement sous HorizontalBox_Main et Text_ControlsHint sous VerticalBox_Preview (voisin de HorizontalBox_ViewControls). Ces corrections de disposition sont distinctes du diagnostic d'ouverture. Aucun asset natif modifié par ce lot.


## Recette PIE — premier affichage et correction éclairage

Le propriétaire a confirmé l'ouverture après affectation de StatRowWidgetClass à WBP_ItemTooltipNativeLine. Les statistiques et actions sont visibles ; l'épée longue N1 est presque noire sur fond noir. Hypothèse : scène GamePreview vide sans environnement spéculaire, sources directionnelles faibles. Le runtime utilise désormais DaylightAmbientCubemap (référence native, sans mesh de ciel), met à jour les captures de scène à la première demande et utilise deux sources directionnelles suivant la caméra (12/6 lux nominaux), exposition manuelle +1. Aucun changement de mesh, matériau, WBP ou lumière du donjon. Test structurel : disponibilité de la cubemap ; la preuve de rendu et du cook effectif reste distincte de la compilation Shipping/NullRHI. Si le rendu reste trop sombre, vérifier l'exposition et le chemin de capture avant tout changement PBR. La capture indique également une surface rectangulaire et un badge blanc : réglages Designer à traiter après éclairage.


### Éclairage V2 — échec visuel du premier correctif

Le propriétaire ne constate aucune amélioration avec le SHA 3278c29c, dont les deux validations Windows sont success. DefaultEngine.ini active Lumen GI et réflexions globalement. Hypothèse à vérifier : une capture ponctuelle d'un monde GamePreview vide ne bénéficie pas du même pipeline que l'atelier. V2 retire la configuration de SkyLight capturé et la remplace par AmbientCubemap directement dans le post-process, désactive GI et réflexions Lumen localement pour cette caméra seulement, conserve les deux sources et l'exposition. Aucun réglage global ni asset joueur modifié. Un test NullRHI vérifie la configuration exacte, pas sa luminosité. Un seul readback par présentation en Development (absent Shipping, jamais dans la boucle de rotation) écrit `[ItemExamination] StudioCaptureV2` avec RGB et alpha ; si le résultat reste noir, cette mesure distingue la capture de la composition UMG et confirme le chargement du nouveau binaire. Ne pas augmenter de nouveau les intensités sans preuve GPU. Supprimer ce diagnostic temporaire après recette.


### Éclairage V3 — essai avec état de rendu persistant

Au SHA `486d1de4`, V3 conserve un état de rendu entre les captures, amorce deux captures et ajoute une source frontale. Le test `Game.UI.Items.Inspection.StudioLightingGPU` est introduit pour mesurer l'épée longue N1 sur le vrai RHI. Le passage suivant invalide l'hypothèse d'un défaut lié à l'état de rendu persistant.

### ARMEMENT-INSPECTION02.5B.6 — Studio et composition GPU

Sur DX12 et Radeon RX 9070 XT, V3 rend 1 048 576 pixels noirs, alpha 255, au premier examen de l'épée longue N1. Les sorties couleur de base et normales restent noires même en changeant le format du Render Target et en forçant une recréation de l'état de rendu pendant la compilation. UE 5.5.4 écarte la primitive dans `UStaticMeshComponent::CreateSceneProxy` tant que `IsCompiling()` est vrai. L'atelier dessine sa scène continuellement et récupère naturellement le maillage après compilation. L'examen ponctuel doit attendre les ressources visuelles, y compris la préparation des PSO du composant, puis capturer à nouveau. Le widget programme une reprise sur le monde propriétaire et l'annule à la fermeture.

Un second défaut se trouve dans le WBP local. `Image_RenderTarget.ColorAndOpacity` vaut initialement `(0,026 ; 0,026 ; 0,026 ; 1)` en linéaire. Cette teinte atténue la texture même lorsque ses pixels sont éclairés. Dans le Designer, régler **Color and Opacity** de **Image_RenderTarget** sur blanc opaque `(1, 1, 1, 1)` et conserver le tint du brush blanc. Utiliser une zone d'image carrée pour respecter les proportions. Le C++ fournit la texture dynamique au widget.

Le monde `GamePreview` charge le HDR `DaylightAmbientCubemap` dans un Sky Light mobile filtré, comme le fait `FAdvancedPreviewScene` avec l'environnement de son profil. Ombres et reflets utilisent les matériaux PBR de l'objet. Deux sources directionnelles disposées autour de la caméra complètent l'environnement. L'exposition manuelle, la suppression du bloom et le plan proche de 0,1 cm sont propres à la capture. Aucun décor visible ne remplit le fond. `SCS_FinalToneCurveHDR` fournit le signal linéaire après courbe de tonalité à une cible `RTF_RGBA8_SRGB`. Slate échantillonne cette texture et applique la correction d'affichage. Le test exige un alpha à 255 sur toute l'image et des coins noirs. La capture est effectuée à la demande, sans historique temporel.

Pour rejouer la vérification sur GPU, exécuter `Scripts/ValidateUE.ps1 -EngineRoot D:/UE_5.5 -AutomationFilter Game.UI.Items.Inspection.StudioLightingGPU -UseRHI`. Le test conserve les seuils de lisibilité de V3, attend la première image effective et vérifie l'ouverture, la rotation, le zoom, la face opposée et le déplacement.

La recette DX12 du 28 novembre 2026 ouvre l'épée longue N1 depuis l'action d'inventaire dans `L_Dungeon` en PIE. Sur les pixels utiles, la moyenne du canal RVB maximal est de 89,12/255 dans la cible et de 89,64/255 dans le viewport après rééchantillonnage. Le noir mesuré est `(0, 0, 0)`. Les captures de l'examen, de la source et du zoom, ainsi que l'original du WBP, sont conservés localement dans `Saved/Automation/InspectionStudio/`. La propriété du WBP a été corrigée dans Unreal sur le fichier local. Ce WBP et l'affectation du pawn restent des changements locaux.
