RÈGLE GIT ABSOLUE :
Une réponse ChatGPT = au maximum un commit Git.
Si plusieurs fichiers doivent être modifiés, l’assistant doit impérativement produire un seul commit atomique.
Interdiction d’utiliser une action GitHub qui crée un commit par fichier.
Si l’outil disponible ne permet pas un commit unique, l’assistant doit s’arrêter et fournir un patch ou un prompt Codex au lieu de modifier GitHub directement.
Aucune branche ne doit être créée.
Tout doit rester sur master.