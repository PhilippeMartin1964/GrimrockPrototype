# GrimrockPrototype - Instructions Codex

## Projet

- Projet Unreal Engine 5.5.4 : GrimrockPrototype.
- Dépôt GitHub : PhilippeMartin1964/GrimrockPrototype.
- Dossier local principal : D:\Development\GrimrockPrototype.
- Travailler sur `master` sauf instruction contraire explicite.
- Ne pas créer de branche pour les petites modifications.
- Une tâche / une réponse ChatGPT = un seul commit Git au maximum.
- Ne jamais créer un commit par fichier.
- Si les outils GitHub disponibles risquent de créer plusieurs commits, ne pas les utiliser.
- Utiliser `docs/` en minuscule, jamais `Docs/`.

## Git

- Toujours commencer par `git status`.
- Avant modification importante, vérifier la branche courante avec `git branch --show-current`.
- Ne pas écraser les changements utilisateur.
- RÈGLE ABSOLUE : une étape fonctionnelle = exactement un commit final maximum sur `master`.
- Aucun commit intermédiaire ne doit être poussé pour une étape en cours.
- Toutes les modifications de code, tests et documentation appartenant à une même étape doivent être regroupées dans ce commit final.
- Les corrections découvertes avant livraison doivent être intégrées dans ce même commit final ; ne jamais publier de commit séparé `Fix...`, `Fix includes...`, `Cleanup...` ou équivalent pour la même étape.
- Avant toute publication, vérifier que l'écart entre le commit de départ de l'étape et le HEAD publié contient exactement un commit, sauf demande explicite contraire de l'utilisateur.
- Si plusieurs commits intermédiaires ont été créés localement ou techniquement, les regrouper avant toute mise à jour de `origin/master`.
- AUTORISATION PERMANENTE : si ChatGPT ou un outil qu'il pilote crée accidentellement sur `master` un commit parasite, vide, intermédiaire ou techniquement indésirable, ChatGPT est autorisé à réécrire immédiatement `master` par force-push/reset vers le dernier état vérifié puis à republier l'étape sous forme atomique, sans demander une nouvelle confirmation.
- Cette autorisation de réécriture ne couvre que les commits créés par ChatGPT ou ses outils pendant le travail en cours. Ne jamais supprimer ni réécrire un commit utilisateur ou un changement distant imprévu sans instruction explicite.
- Avant toute réécriture automatique, vérifier le parent et la divergence distante ; après la réécriture, relire `master` et vérifier que le commit parasite n'est plus atteignable depuis la branche.

## Unreal Engine

- Version cible : Unreal Engine 5.5.4.
- Modifier les `.uasset` et `.umap` seulement si nécessaire.
- Pour les assets Unreal, préférer les modifications via Unreal Editor.
- Après modification C++, compiler avec Visual Studio 2026 ou UnrealBuildTool si disponible.
- En UE5.5.4, conserver la toolchain v143 / MSVC 14.44.35207 installée sous Visual Studio 2026. Le libellé UBT `Visual Studio 2022` est attendu pour cette famille de compilateur ; vérifier le chemin physique `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207`.
- Pour le formatage, utiliser clang-format 19.1.5 standalone ; ne pas basculer implicitement sur le clang-format 22.x fourni par Visual Studio 2026.
- Ne pas versionner `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`.

## Style de travail

- Expliquer clairement les fichiers modifiés.
- Faire des changements minimaux.
- Ne pas mélanger correction C++, documentation et assets binaires dans un même changement sauf nécessité.

## RÈGLE PERMANENTE — C++ = données / WBP = interface (CHECKLIST À CHAQUE JALON)

- **APPLIQUER** `docs/UI_UMG_PRESENTATION_CONTRACT.md` pour toute nouvelle interface et refonte WBP. Le C++ doit exposer un `USTRUCT(BlueprintType)` / read-model de nombres, états, noms authorés, flags et listes ; le **WBP** seul possède mise en forme des textes, colonnes, polices, couleurs, espacements, visibilité et créations de lignes.
- **INTERDIT dans les classes C++ de présentation des nouveaux WBP :** `WidgetTree->ConstructWidget`, `AddChildToGrid`, `SetColumnFill`, `SetFont`, `SetPadding`, réglages de layout ou `FText::Format` pour composer des libellés d'interface. Ces opérations appartiennent au Designer / Event Graph. Les générateurs de logs ou de texte de fallback ne sont pas refondus sans ticket explicite.
- Pour les listes, fournir `TArray` + booléens métier et authorer un **WBP de ligne réutilisable** ; pas de table reconstruite cellule par cellule en C++. Prévoir un fallback si le Blueprint n'est pas encore prêt.
- **TESTS :** valider C++ sur les valeurs et flags ; le layout doit être validé dans l'éditeur par le joueur et par capture PIE. Une compilation ne prouve jamais la qualité du WBP.

## Procédure obligatoire — Texte joueur dans les WBP (CHECKLIST À CHAQUE JALON)

- **DÉCLENCHEUR PERMANENT :** dès qu'une tâche ajoute, change, supprime ou présente du texte visible dans un WBP, une infobulle, le HUD ou une interface native, **ouvrir et appliquer** `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` **avant tout authoring UMG**. Inclure ce contrôle dans la définition de « terminé » du ticket et dans le compte rendu.
- Inventorier les textes par provenance : clé `ST_UI` (texte statique partagé), `FText` DataAsset (contenu métier), `FUILocalizationText` / `LOCTEXT` (texte dynamique), `FText` local WBP justifié, ou notation mécanique. Réutiliser les clés existantes ; ne créer de nouvelles lignes que dans `Localization/Source/UI.csv` lorsque nécessaire.
- **INTERDICTION ABSOLUE :** ne jamais dire « les clés sont prêtes pour l'UMG » parce que `UI.csv` a été committé. Le CSV n'actualise pas `ST_UI.uasset` : fermer Unreal Editor, exécuter `-run=UIStringTableAuthoring` dans Unreal, vérifier le code 0 et **ouvrir `ST_UI` pour constater chaque nouvelle clé avant de demander de créer les TextBlock**. Si le générateur n'est pas exécutable dans l'environnement disponible, déclarer le blocage et fournir les commandes de matérialisation locale.
- Pour les labels statiques avec clé, lier la propriété `Text` du Designer directement à `ST_UI + Key`. Pas de copie du français et pas de double `SetText()` C++ sur le même champ. Vérifier les textes dynamiques, le pipeline de traduction `Game` (fr/en), les tests de localisation et le rendu PIE.
- Les `.uasset` (WBP et String Table) sont des assets UE, jamais produits artificiellement. Inspecter les changements Git et versionner la projection validée avec le jalon concerné ; sinon laisser explicitement « matérialisation ST_UI non faite » et **ne pas clore**.
- **TO-DO récurrent non supprimable :** au début **et à la fin** de chaque ticket UI/WBP, relire les cases de la checklist de `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` et préciser explicitement le statut `UI.csv → ST_UI → WBP → Game/fr/en`.

## Publication automatique

- Cette section remplace toute instruction antérieure contradictoire.
- Toute demande de développement, correction ou documentation implique :
  1. préparer et vérifier l'ensemble des changements de l'étape sans publication intermédiaire ;
  2. créer un seul commit atomique final au maximum pour l'étape ;
  3. pousser automatiquement ce commit unique sur `origin/master`.
- Ne jamais utiliser un workflow qui crée un commit par fichier ou un commit par correction intermédiaire.
- Ne jamais demander une confirmation supplémentaire avant un commit ou un push ordinaire.
- Ne pas demander de confirmation pour un force-push destiné exclusivement à retirer des commits accidentels créés par ChatGPT ou ses outils pendant l'étape en cours ; appliquer alors les garde-fous de la section Git.
- S'arrêter uniquement en cas de divergence distante ou de changements utilisateur imprévus, de réécriture qui toucherait un commit utilisateur, ou de blocage technique réel.
- Si l'environnement interdit le push, ne pas demander à nouveau l'autorisation : conserver le commit local et fournir directement la commande exacte à exécuter.
