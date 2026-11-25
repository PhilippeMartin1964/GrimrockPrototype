# GrimrockPrototype

Prototype de dungeon crawler sur grille. Le projet associe exploration, mécanismes, combat tactique, progression RPG et un éditeur de donjons intégré à Unreal.

## Démarrage

Utiliser Unreal Engine **5.5.4** et Visual Studio **2026** avec le toolset v143 **MSVC 14.44.35207**. Le module `GrimrockLua` utilise le sous-module `ThirdParty/Lua54`. Les assets binaires utilisent Git LFS. Consulter le [guide d’environnement](docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md).

```powershell
git submodule update --init -- ThirdParty/Lua54
git lfs pull
.\Scripts\ValidateUE.ps1 -EngineRoot 'D:\UE_5.5' -BuildShipping -AutomationFilter Grimrock
```

La [référence des scripts](docs/SCRIPTS_REFERENCE.md) couvre aussi les tests ciblés, l’authoring et le packaging. La [validation Windows](docs/Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md) compile et teste chaque SHA de `master` dans un worktree dédié.

## Reprendre le projet

- [Documentation](docs/README.md) et [instructions de travail](AGENTS.md).
- [Architecture courante](docs/Architecture/PROJECT_SYNTHESIS.md) et [modèle des objets](docs/Architecture/DataModel.md).
- [Roadmap fonctionnelle](docs/Design/PROJECT_COMPLETION_ROADMAP.md) et [clôtures de consolidation](docs/Technical/Consolidation.md).
