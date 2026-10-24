RÈGLE GIT ABSOLUE :
Une réponse ChatGPT = au maximum un commit Git.
Si plusieurs fichiers doivent être modifiés, l’assistant doit impérativement produire un seul commit atomique.
Interdiction d’utiliser une action GitHub qui crée un commit par fichier.
Si l’outil disponible ne permet pas un commit unique, l’assistant doit s’arrêter et fournir un patch ou un prompt Codex au lieu de modifier GitHub directement.
Aucune branche ne doit être créée.
Tout doit rester sur master.

RÈGLE PERMANENTE LOCALISATION WBP :
Tout texte UI/WBP nouveau ou modifié déclenche la checklist `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.
Les nouvelles clés `Localization/Source/UI.csv` doivent être matérialisées par Unreal (`-run=UIStringTableAuthoring`) dans `ST_UI.uasset`, puis vérifiées réellement dans l'éditeur AVANT de demander l'authoring des TextBlock WBP.
Ne pas confondre CSV committé, String Table générée, WBP raccordé et traduction Game fr/en.
Si la matérialisation ou la vérification de ST_UI manque, ne pas annoncer le jalon comme terminé.
