RÈGLE GIT — LOTS COHÉRENTS (MISE À JOUR DU 17.11.2026)

La règle antérieure « une réponse ChatGPT = un seul commit » est ABROGÉE.
Référence autoritaire et détaillée : `AGENTS.md`, sections « Git »,
« Publication automatique par lots » et « Validation locale automatique ».

- Un lot cohérent, testable et réversible = un commit atomique final.
  Inclure dans ce commit tous les fichiers de code, tests, scripts et
  documentation qui concourent au même résultat.
- Ne JAMAIS créer un commit par fichier, par include corrigé ou par
  micro-jalon ; ne pas non plus fusionner artificiellement deux travaux
  indépendants qui nécessitent des validations ou retours arrière distincts.
- Une seule intervention ChatGPT peut mener plusieurs lots et plusieurs
  commits SUCCESSIFS sur `master`. Aucun plafond de commits par réponse.
- Chaque commit est poussé sur `master` sans branche supplémentaire.
  Contrôler le HEAD distant et l'absence de divergence.
- Après un commit technique, attendre la validation Windows réelle sur
  son SHA exact : `UE5.5.4/SilentTask=success` ET
  `UE5.5.4/LocalAuto=success` avant de commencer le commit suivant.
  En cas d'échec, corriger d'abord le lot concerné ; ne pas cumuler
  plusieurs changements non validés.
- Les validations natives obligatoires pour `.uasset`, `.umap`,
  PIE/Chaos ou SaveGame ne sont pas contournées par cette politique.
- Une fois un lot validé, continuer les lots suivants en autonomie ;
  ne s'arrêter que pour un blocage réel ou une intervention humaine
  indispensable. Pas de nouvel accord utilisateur pour chaque commit.

RÈGLE PERMANENTE C++ / WBP (RECTIFIÉE LE 24.10.2026) :
Lire `docs/UI_UMG_PRESENTATION_CONTRACT.md` pour toute nouvelle UI / refonte.
C++ = logique métier ET comportement de présentation : choix de contenus, formatage dynamique localisé via FUILocalizationText/FText::Format, SetText sur TextBlocks déjà authorés, visibilité des sections, CreateWidget d'un WBP complet de ligne et AddChild.
WBP Designer = arbre visuel, police, taille, bordures, alignements, marges, couleur et colonnes éditables.
Ne jamais construire des TextBlocks ou leur layout individuel en C++. Ne pas déplacer systématiquement le code dans les Event Graphs : éviter de recréer toute une logique en Blueprint. Graphs seulement pour les comportements qui s'y prêtent.
ARMEMENT01.10.3.3.9 : les anciens Text_Attack/Text_Defense, la projection AttackDice et PresentCombatTooltip sont supprimés du C++. Supprimer d'abord l'override et les variables/widget obsolètes **dans Unreal Editor**, compiler/sauvegarder les WBP puis installer le C++ nettoyé. Conserver uniquement le fallback textuel GÉNÉRIQUE de l'Event Feed si le WBP structuré manque, pas les anciennes lignes graphiques.

RÈGLE PERMANENTE LOCALISATION WBP :
Tout texte UI/WBP nouveau ou modifié déclenche la checklist `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.
Les nouvelles clés `Localization/Source/UI.csv` doivent être matérialisées par Unreal (`-run=UIStringTableAuthoring`) dans `ST_UI.uasset`, puis vérifiées réellement dans l'éditeur AVANT de demander l'authoring des TextBlock WBP.
Ne pas confondre CSV committé, String Table générée, WBP raccordé et traduction Game fr/en.
Si la matérialisation ou la vérification de ST_UI manque, ne pas annoncer le jalon comme terminé.
