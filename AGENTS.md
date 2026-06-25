# GrimrockPrototype - Instructions Codex

## Projet

- Projet Unreal Engine 5.5.4 : GrimrockPrototype.
- Dépôt GitHub : PhilippeMartin1964/GrimrockPrototype.
- Dossier local principal : D:\Development\GrimrockPrototype.
- Travailler sur `master` sauf instruction contraire explicite.
- Ne pas créer de branche pour les petites modifications.
- Utiliser `docs/` en minuscule, jamais `Docs/`.

## Git

- Toujours commencer par `git status`.
- Avant modification importante, vérifier la branche courante avec `git branch --show-current`.
- Ne pas faire de `git push --force` sans instruction explicite.
- Ne pas écraser les changements utilisateur.

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