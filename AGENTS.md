# GrimrockPrototype - Instructions Codex

## Projet

- Projet Unreal Engine 5.5.4 : GrimrockPrototype.
- Dépôt GitHub : PhilippeMartin1964/GrimrockPrototype.
- Dossier local principal : D:\Development\GrimrockPrototype.
- Travailler sur `master` sauf instruction contraire explicite.
- Ne pas créer de branche pour les petites modifications.
- **Un commit atomique par lot de travail cohérent et vérifiable**, jamais un commit par fichier.
- **Plusieurs lots et commits successifs peuvent être réalisés dans une même intervention ChatGPT** : aucune limite artificielle par réponse, par micro-étape ou par numéro de jalon.
- Grouper dans le même lot le code, ses tests, ses corrections de prépublication et sa documentation associée.
- Si une API GitHub commite un fichier par appel, ne pas l'utiliser : créer un arbre Git unique et un commit unique pour chaque lot, ou utiliser un outil de staging atomique.
- Utiliser `docs/` en minuscule, jamais `Docs/`.

## Git — règle des lots cohérents (17.11.2026)

- Toujours commencer par `git status` lorsque le checkout local est accessible ; en GitHub distant, contrôler le SHA réel de `origin/master`, ses statuts et la concurrence avant toute écriture.
- Avant modification importante, vérifier la branche courante avec `git branch --show-current` lorsque disponible.
- Ne jamais écraser les changements utilisateur. Travailler sur `master` sans créer de branches non demandées.
- **Un lot = un objectif fonctionnel/technique précis, avec un périmètre de fichiers liés, des critères de réussite, des tests et un éventuel retour arrière identifiables.** Un lot peut couvrir plusieurs micro-jalons. Les fichiers C++, tests, scripts et documentation de cet objectif appartiennent au **même commit**.
- **Ne pas créer un commit par fichier, par correction d'include, par test, par fichier de documentation ni par simple numéro de jalon.** Ne pas découper un lot seulement pour générer davantage de commits.
- Séparer les lots lorsque leurs objectifs, risques, validations ou procédures de retour arrière sont réellement indépendants ; isoler notamment les écritures natives de `.uasset`/`.umap`, les migrations de sauvegardes et les changements de schéma lorsque leurs garanties diffèrent.
- **Un commit final atomique par lot publié** : ne pas pousser de WIP ou de commits intermédiaires de mise au point. Grouper les corrections identifiées *avant publication* dans le même commit.
- **Une réponse/intervention ChatGPT peut publier plusieurs commits de lots distincts, de manière séquentielle.** Pas de plafond rigide de commits par conversation. Après chaque lot technique, attendre les deux statuts Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur **son SHA exact** avant de publier le lot suivant.
- Si la validation publiée échoue, analyser les diagnostics puis traiter **l'ensemble du correctif de ce lot** de manière atomique ; ne pas empiler plusieurs micro-commits `Fix...`. Une révision contrôlée du seul HEAD créé par l'assistant pendant l'intervention est autorisée si son parent et l'absence de divergence sont prouvés ; une correction par commit supplémentaire cohérent est permise si réécrire `master` n'est pas sûr.
- **AUTORISATION DE RÉÉCRITURE STRICTEMENT LIMITÉE :** seulement pour un HEAD parasite ou invalide créé par ChatGPT/ses outils pendant le travail en cours, après vérification du SHA exact, du parent, des statuts et de la non-divergence ; mise à jour avec protection équivalente à `--force-with-lease`, jamais un force-push aveugle. Ne jamais réécrire un commit utilisateur ni une modification distante imprévue sans autorisation explicite.
- Après chaque publication, relire `master`, comparer au parent du lot et vérifier que le diff inclut les fichiers attendus. Vérifier la validation finale avant de marquer le lot terminé.
- **Ne pas arrêter l'intervention au seul motif qu'un lot vient de réussir.** Poursuivre les lots suivants sans nouvelle autorisation tant que les outils, le périmètre et les garanties le permettent ; s'arrêter seulement en cas de véritable blocage, de conflit distant, de risque de toucher au travail utilisateur, de validation nécessitant une action native non disponible, ou d'instruction explicite.

## Contrat architectural d'objets et d'armement (décision approuvée)

- **Référence de conception obligatoire :** `docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md` ; édition : `docs/Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md` ; conversion : `docs/Architecture/ARM_CLEAN06_MIGRATION_PLAN.md`.
- **Architecture cible APPROUVÉE mais NON ENCORE IMPLÉMENTÉE.** Les rapports ARM-CLEAN05 décrivent l'existant ; l'ancien pipeline ARMEMENT/INDUS ne fixe plus les nouveaux schémas.
- **Zéro legacy dans le runtime final :** aucune référence de catalogue dans `DA_Weapon_*`, pas de copies N1/poids/nom/icône/mesh ni de fallback ; conversion ponctuelle UE Editor obligatoire avant suppression d'anciennes UPROPERTY, .uasset, niveaux ou sauvegardes.

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

## RÈGLE PERMANENTE — C++ comportemental / WBP Designer présentation

- **Contrat autoritaire :** `docs/UI_UMG_PRESENTATION_CONTRACT.md`. Cette section **annule** l'interprétation antérieure qui imposait toute logique de formatage et de liste dans l'Event Graph.
- **C++ gère le comportement** : calculs, collecte de données, messages localisés via `FUILocalizationText::Get` + `FText::Format`, remplissage via `SetText` des widgets `BindWidget` déjà créés dans le Designer, `SetVisibility` sur des sections existantes, `CreateWidget` **d'un WBP complet** et `AddChild` dans un conteneur WBP.
- **WBP Designer gère l'apparence** : types de conteneurs, hiérarchie, TextBlocks, polices, tailles, couleurs, marges, colonnes, alignements, icônes, animations. **Aucun `WidgetTree->ConstructWidget<UTextBlock>`, GridPanel de cellules, SetFont/SetPadding/positionnement d'enfant en C++ pour une UI authorée.**
- **Event Graph seulement si justifié**, sans recopier les boucles de rendu, `Format Text` et messages déjà gérés nativement. Refuser un Graph complexe lorsqu'un simple `SetText` natif sur TextBlock du Designer suffit.
- **Listes dynamiques** : le C++ crée des **instances de WBP de ligne déjà conçus visuellement** et les ajoute à un conteneur WBP. Les dimensions et styles de la ligne restent modifiables exclusivement dans son Designer.
- **Tests** : validation de la logique/formatage en C++, et captures PIE pour styles/layout. Le repli est obligatoire quand un WBP n'est pas prêt.

## Procédure obligatoire — Texte joueur dans les WBP (CHECKLIST À CHAQUE JALON)

- **DÉCLENCHEUR PERMANENT :** dès qu'une tâche ajoute, change, supprime ou présente du texte visible dans un WBP, une infobulle, le HUD ou une interface native, **ouvrir et appliquer** `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` **avant tout authoring UMG**. Inclure ce contrôle dans la définition de « terminé » du ticket et dans le compte rendu.
- Inventorier les textes par provenance : clé `ST_UI` (texte statique partagé), `FText` DataAsset (contenu métier), `FUILocalizationText` / `LOCTEXT` (texte dynamique), `FText` local WBP justifié, ou notation mécanique. Réutiliser les clés existantes ; ne créer de nouvelles lignes que dans `Localization/Source/UI.csv` lorsque nécessaire.
- **INTERDICTION ABSOLUE :** ne jamais dire « les clés sont prêtes pour l'UMG » parce que `UI.csv` a été committé. Le CSV n'actualise pas `ST_UI.uasset` : fermer Unreal Editor, exécuter `-run=UIStringTableAuthoring` dans Unreal, vérifier le code 0 et **ouvrir `ST_UI` pour constater chaque nouvelle clé avant de demander de créer les TextBlock**. Si le générateur n'est pas exécutable dans l'environnement disponible, déclarer le blocage et fournir les commandes de matérialisation locale.
- Pour les labels statiques avec clé, lier la propriété `Text` du Designer directement à `ST_UI + Key`. Pas de copie du français et pas de double `SetText()` C++ sur le même champ. Vérifier les textes dynamiques, le pipeline de traduction `Game` (fr/en), les tests de localisation et le rendu PIE.
- Les `.uasset` (WBP et String Table) sont des assets UE, jamais produits artificiellement. Inspecter les changements Git et versionner la projection validée avec le jalon concerné ; sinon laisser explicitement « matérialisation ST_UI non faite » et **ne pas clore**.
- **TO-DO récurrent non supprimable :** au début **et à la fin** de chaque ticket UI/WBP, relire les cases de la checklist de `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` et préciser explicitement le statut `UI.csv → ST_UI → WBP → Game/fr/en`.

## Publication automatique par lots

- Cette section et « Git — règle des lots cohérents » remplacent la contrainte historique « une réponse = un commit » et « un micro-jalon = exactement un commit ».
- Pour **chaque lot** : définir le périmètre lié → modifier code/tests/documentation ensemble → contrôler le diff → créer et pousser **un commit atomique final** sur `origin/master` → vérifier les deux statuts de validation sur le SHA publié.
- Dès que les deux statuts requis sont `success`, **enchaîner le prochain lot utile sans interrompre la réponse pour demander « Go »**. Une intervention peut contenir plusieurs commits de lots indépendants.
- Un échec de compilation, d'Automation, de préflight ou de statut Git suspend le lot suivant jusqu'à analyse/correction du lot en échec. Ne jamais déclarer un lot validé sur `pending`, sur un statut absent, ni sur le statut d'un ancien SHA.
- Ne jamais publier un commit par fichier ou un WIP. Si le connecteur GitHub n'a pas de commit atomique multi-fichiers, utiliser une opération Git tree/commit/ref atomique ou conserver un patch ; ne pas multiplier les commits d'API.
- Ne pas demander une confirmation supplémentaire avant des commits/push ordinaires. Une migration d'assets native ou de sauvegardes conserve **tous ses garde-fous spécifiques** et une intervention locale si indispensable.
- Si le push est impossible, conserver le patch/commit préparé et donner précisément le blocage, sans prétendre l'avoir publié.

## Validation locale automatique obligatoire après publication

- **RÈGLE PERMANENTE :** tout jalon modifiant le C++, un test, un asset Unreal ou le runtime doit passer par le dispositif déjà installé `VALIDATION-AUTO01`. Ne jamais demander par réflexe à l'utilisateur de lancer `ValidateUE.ps1`, `git pull`, Visual Studio ou le Session Frontend.
- **Chaîne autoritaire :** push atomique de `master` → tâche planifiée Windows `GrimrockPrototype-UEValidation` (environ toutes les 3 minutes) → worktree dédié `D:\Development\GrimrockPrototype_UEValidation` → compilation UE 5.5.4 + suite Automation `Grimrock` → statut GitHub `UE5.5.4/LocalAuto`.
- **Après chaque push**, relever le SHA exact publié et consulter son statut de commit GitHub, par exemple `GET /repos/PhilippeMartin1964/GrimrockPrototype/commits/<SHA>/status` ou via le connecteur GitHub. Ne pas consulter par erreur le statut d'un parent ou d'un autre commit.
- **Statut `success` :** indiquer la validation technique réussie pour ce SHA. **`pending` ou statut absent :** écrire clairement « validation Windows en cours / non encore publiée », jamais « validé ». **`failure` :** analyser le diagnostic GitHub, en particulier l'issue publique `[UEValidation]` associée au SHA ; chercher la cause dans le code et corriger de manière autonome lorsque les outils le permettent.
- Une issue publique ne contient volontairement que des diagnostics filtrés. Ne pas exiger de l'utilisateur les journaux locaux par défaut ; ne les demander qu'en dernier recours si la cause ne peut pas être trouvée à distance.
- Ne pas prétendre lancer Unreal Engine sur le PC utilisateur depuis une session ChatGPT qui n'a pas accès au poste Windows : **c'est l'agent planifié, déjà opérationnel, qui exécute les commandes locales**. L'assistant est responsable du suivi des statuts GitHub pendant son intervention, pas de prétendre avoir exécuté le build localement.
- Le rendu GPU/PIE et la qualité artistique ne sont pas prouvés par `-NullRHI`. Les distinguer explicitement de la compilation et de la suite Automation ; demander une intervention visuelle seulement si aucune capture automatisée exploitable n'est disponible.
- Documentation de référence : `docs/Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md`, `Scripts/UEValidationAgent.ps1`, `Scripts/InstallUEValidationAgent.ps1`.
- **Compte rendu de fin de jalon obligatoire :** SHA publié, statut exact `UE5.5.4/LocalAuto`, résultat ou blocage, tests concernés, reste à vérifier visuellement. Aucun lancement manuel récurrent à transférer à l'utilisateur.

## VALIDATION-AUTO01.11 — tâche Windows réellement silencieuse

- L'action de la tâche planifiée doit être `wscript.exe //B //Nologo`,
  pas un lancement direct de `powershell.exe`.
- L'agent migre automatiquement son ancienne action par
  `Scripts/UEValidationSilentTask.ps1`, sans réclamer `git pull`
  ni une réinstallation de la tâche à l'utilisateur.
- Contrôler **deux statuts distincts** sur le SHA : `UE5.5.4/SilentTask`
  (action silencieuse installée et vérifiée) et `UE5.5.4/LocalAuto`
  (compilation + suite `Grimrock`). Ne pas les confondre.
