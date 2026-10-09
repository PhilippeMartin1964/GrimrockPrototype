# GrimrockPrototype — Environnement de développement

Date de référence : **9 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**

## 1. Environnement autoritaire

Le dépôt est développé et validé avec :

```text
Unreal Engine 5.5.4
Windows
Visual Studio Community 2026 18.10.3
C++
MSVC v143 / 14.44.35207
Windows SDK 10.0.28000.0
clang-format 19.1.5 standalone
Git
```

La racine Unreal n'est jamais codée en dur dans le dépôt. Les scripts reçoivent :

```powershell
-EngineRoot D:\UE_5.5
```

ou la variable d'environnement :

```text
UE_ROOT
```

## 2. Validation locale

Editor + Automation :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "<filtre>"
```

Shipping :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Dépendances projet/plugins :

```powershell
.\Scripts\CheckProjectDependencies.ps1 -EngineRoot D:\UE_5.5
```

## 3. Politique des plugins

### Plugins requis

Tout plugin **requis** par le projet doit être :

1. disponible dans UE5.5.4, ou
2. versionné dans le dépôt si sa licence et son mode de distribution l'autorisent, ou
3. décrit explicitement avec une procédure reproductible d'installation.

Un plugin first-party futur placé sous `Plugins/` doit pouvoir être versionné. Pour cette raison, le dépôt n'ignore plus globalement tout le dossier `/Plugins/`.

### Meshy

Meshy est classé comme :

```text
outil de développement optionnel
usage : production/import ponctuel d'assets
dépendance runtime : aucune
dépendance build : aucune
versionnement : non
installation obligatoire : non
```

Le `.uproject` conserve volontairement une référence explicite :

```json
{
    "Name": "meshy",
    "Enabled": false,
    "Optional": true
}
```

Ainsi :

- un clone propre doit fonctionner sans Meshy ;
- Meshy n'est pas chargé par défaut ;
- l'absence du plugin n'est pas une erreur de projet ;
- sa copie locale peut rester sous `Plugins/meshy/`, dossier ignoré par Git.

## 4. Ajouter Meshy ponctuellement sur une nouvelle machine

Si un développeur doit utiliser Meshy pour produire ou convertir un asset :

1. obtenir une version de Meshy compatible avec UE5.5.4 depuis sa source légitime ;
2. l'installer localement sous :

```text
<Repo>/Plugins/meshy/
```

3. l'activer uniquement pour la session/tranche de production concernée ;
4. produire/importer les assets nécessaires ;
5. vérifier que les assets livrés ne conservent aucune dépendance dure vers des classes, modules ou assets Meshy ;
6. désactiver Meshy avant validation finale ;
7. restaurer le descripteur projet avant commit si l'Editor l'a modifié :

```powershell
git restore GrimrockPrototype.uproject
```

8. ne jamais ajouter `Plugins/meshy/` au commit.

Si Meshy devient un jour nécessaire au runtime, au build ou à l'ouverture d'assets livrés, cette politique n'est plus valable : il faudra rouvrir la dette de dépendance et définir sa version, sa licence et son mode de distribution.

## 5. Vérification d'un clone ou d'un nouvel environnement

Ordre recommandé :

```powershell
git clone <repository>
cd GrimrockPrototype

.\Scripts\CheckProjectDependencies.ps1 -EngineRoot D:\UE_5.5
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.StartupFlow.STARTUPFLOW01"
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Le premier script vérifie que chaque plugin **activé** du `.uproject` possède réellement un descripteur `.uplugin` dans le projet ou l'installation UE.

## 6. Toolchain Visual Studio 2026 / UE5.5.4

L'environnement validé le **9 octobre 2026** utilise :

```text
IDE                         : Visual Studio Community 2026 18.10.3
Installation                : C:\Program Files\Microsoft Visual Studio\18\Community
Toolset C++                  : MSVC v143 / 14.44.35207
Version compilateur vue UBT : 14.44.35229
Windows SDK                 : 10.0.28000.0
```

Le composant v143 / 14.44 doit rester installé dans Visual Studio 2026. UE5.5.4 ne connaît pas nativement la génération Visual Studio 2026 et conserve le nom de famille `VisualStudio2022` pour cette toolchain. C'est pourquoi UBT peut afficher :

```text
Using Visual Studio 2022 14.44.35229 toolchain
(C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207)
```

Ce libellé est **normal**. Le chemin physique est l'autorité pour déterminer l'installation réellement utilisée. Après désinstallation de Visual Studio 2022, `vswhere` ne retourne plus que Visual Studio 2026 et les builds Editor continuent d'utiliser le chemin `Visual Studio\18\Community`.

La solution générée par UE5.5.4 peut également afficher `UE5 (Visual Studio 2022)` et `GrimrockPrototype (Visual Studio 2022)` dans Visual Studio 2026. Ne pas retargeter manuellement les projets uniquement pour supprimer ce libellé.

### Configuration UBT utilisateur

La configuration validée conserve volontairement :

```xml
<WindowsPlatform>
  <Compiler>VisualStudio2022</Compiler>
  <CompilerVersion>14.44.35207</CompilerVersion>
</WindowsPlatform>
```

dans :

```text
%APPDATA%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml
```

`VisualStudio2022` désigne ici la famille de compilateur comprise par UE5.5.4. Ne pas remplacer cette valeur par `VisualStudio2026` sans migration de version Unreal et nouvelle validation complète.

UE5.5.4 avertit encore que cette version MSVC n'est pas sa version préférée et mentionne `14.38.33130`. La migration n'a pas tenté de supprimer ce warning par un changement arbitraire de compilateur.

### Validations réalisées après migration

La configuration VS2026/v143 a été validée avec :

- ouverture de `GrimrockPrototype.sln` dans Visual Studio 2026 sans migration de solution ;
- build complet depuis Visual Studio 2026 ;
- build `GrimrockPrototypeEditor Win64 Development` via `Scripts\ValidateUE.ps1` ;
- Automation `Grimrock.StartupFlow.STARTUPFLOW01` : **1 succès, 0 warning, 0 échec** ;
- `RunUAT BuildCookRun` Shipping : build, cook, stage, package et archive **réussis** ;
- MSBuild utilisé par UAT : `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`;
- désinstallation de Visual Studio 2022 Community, puis nouveau build Editor réussi avec uniquement les toolchains VS2026 détectées.

Décision : conserver UE5.5.4 + v143/14.44 tant que les harness Editor et Shipping restent verts. Réouvrir ce point lors d'une migration Unreal, d'une incompatibilité concrète ou de l'introduction d'une CI Windows reproductible.

## 7. Clang-format

Grimrock C++ Style v1 reste figé sur :

```text
clang-format 19.1.5
C:\Program Files\LLVM\bin\clang-format.exe
```

Depuis TOOLCHAIN01 (`bfd4e1d7`), `Scripts\CheckCppFormat.ps1` et `Scripts\FormatCpp.ps1` ne dépendent plus de Visual Studio. Leur ordre de résolution est :

1. `-ClangFormatPath` explicite ;
2. `C:\Program Files\LLVM\bin\clang-format.exe` ;
3. `clang-format` dans le `PATH`.

La version est toujours contrôlée strictement et doit être **19.1.5**. Visual Studio 2026 installe actuellement un clang-format 22.1.3 sur la machine validée ; il ne doit pas remplacer silencieusement la baseline STYLE01.

Contrôle :

```powershell
.\Scripts\CheckCppFormat.ps1
```

Au moment de la migration, le contrôle parcourt **867 fichiers first-party** et signale **457 fichiers non conformes**. Le même résultat a été obtenu avec le clang-format 19.1.5 historique de VS2022 et le standalone 19.1.5 ; cette dérive préexistante est donc distincte de la migration d'IDE. Ne pas lancer un reformatage massif dans un ticket de toolchain.

