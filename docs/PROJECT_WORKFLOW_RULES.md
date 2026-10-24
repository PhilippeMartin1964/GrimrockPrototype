RÈGLE GIT ABSOLUE :
Une réponse ChatGPT = au maximum un commit Git.
Si plusieurs fichiers doivent être modifiés, l’assistant doit impérativement produire un seul commit atomique.
Interdiction d’utiliser une action GitHub qui crée un commit par fichier.
Si l’outil disponible ne permet pas un commit unique, l’assistant doit s’arrêter et fournir un patch ou un prompt Codex au lieu de modifier GitHub directement.
Aucune branche ne doit être créée.
Tout doit rester sur master.

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
