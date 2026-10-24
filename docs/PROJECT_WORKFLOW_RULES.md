RÈGLE GIT ABSOLUE :
Une réponse ChatGPT = au maximum un commit Git.
Si plusieurs fichiers doivent être modifiés, l’assistant doit impérativement produire un seul commit atomique.
Interdiction d’utiliser une action GitHub qui crée un commit par fichier.
Si l’outil disponible ne permet pas un commit unique, l’assistant doit s’arrêter et fournir un patch ou un prompt Codex au lieu de modifier GitHub directement.
Aucune branche ne doit être créée.
Tout doit rester sur master.

RÈGLE PERMANENTE C++ / WBP :
Lire `docs/UI_UMG_PRESENTATION_CONTRACT.md` pour toute nouvelle UI / refonte :
C++ = modèle de données métier (valeurs numériques, états, flags, listes ordonnées), sans construction de TextBlocks, sans gestion de mise en page, sans formatage d'un texte visible.
WBP = mise en page, construction des lignes via Widget Blueprint, styles, textes et formats localisés, décisions visuelles de masquage.
Les anciens journaux textuels et fallbacks sont des exceptions documentées, non une raison de créer de nouveaux layouts C++.
Prévoir un repli si le WBP n'a pas encore implémenté son renderer.

RÈGLE PERMANENTE LOCALISATION WBP :
Tout texte UI/WBP nouveau ou modifié déclenche la checklist `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.
Les nouvelles clés `Localization/Source/UI.csv` doivent être matérialisées par Unreal (`-run=UIStringTableAuthoring`) dans `ST_UI.uasset`, puis vérifiées réellement dans l'éditeur AVANT de demander l'authoring des TextBlock WBP.
Ne pas confondre CSV committé, String Table générée, WBP raccordé et traduction Game fr/en.
Si la matérialisation ou la vérification de ST_UI manque, ne pas annoncer le jalon comme terminé.
