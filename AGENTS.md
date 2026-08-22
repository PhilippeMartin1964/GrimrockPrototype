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

## Unreal Engine

- Version cible : Unreal Engine 5.5.4.
- Modifier les `.uasset` et `.umap` seulement si nécessaire.
- Pour les assets Unreal, préférer les modifications via Unreal Editor.
- Après modification C++, compiler avec Visual Studio 2022 ou UnrealBuildTool si disponible.
- Ne pas versionner `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`.

## Style de travail

- Expliquer clairement les fichiers modifiés.
- Faire des changements minimaux.
- Ne pas mélanger correction C++, documentation et assets binaires dans un même changement sauf nécessité.

## Publication automatique

- Cette section remplace toute instruction antérieure contradictoire.
- Toute demande de développement, correction ou documentation implique :
  1. préparer et vérifier l'ensemble des changements de l'étape sans publication intermédiaire ;
  2. créer un seul commit atomique final au maximum pour l'étape ;
  3. pousser automatiquement ce commit unique sur `origin/master`.
- Ne jamais utiliser un workflow qui crée un commit par fichier ou un commit par correction intermédiaire.
- Ne jamais demander une confirmation supplémentaire avant un commit ou un push ordinaire.
- S’arrêter uniquement en cas de divergence distante, de changements utilisateur imprévus, de force-push, de rebase/reset destructif ou de blocage technique réel.
- Si l’environnement interdit le push, ne pas demander à nouveau l’autorisation : conserver le commit local et fournir directement la commande exacte à exécuter.
