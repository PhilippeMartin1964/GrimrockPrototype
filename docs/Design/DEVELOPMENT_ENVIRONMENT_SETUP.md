# GrimrockPrototype — Environnement de développement

Date de référence : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**

## 1. Environnement autoritaire

Le dépôt est développé et validé avec :

```text
Unreal Engine 5.5.4
Windows
Visual Studio 2022
C++
clang-format 19.1.5
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
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_8"
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Le premier script vérifie que chaque plugin **activé** du `.uproject` possède réellement un descripteur `.uplugin` dans le projet ou l'installation UE.

## 6. Toolchain Visual Studio

Le projet ne pince actuellement pas une version MSVC précise dans les Target.cs.

Sur l'environnement validé en août 2026, UBT a utilisé :

```text
Visual Studio 2022
MSVC 14.44.35227
Windows SDK 10.0.26100.0
```

UE5.5.4 affiche actuellement un warning indiquant que cette version MSVC n'est pas sa version préférée et mentionne `14.38.33130` comme version préférée. Malgré ce warning, les builds Editor et les validations Automation ont réussi.

Décision TD07.1 :

- **ne pas forcer un downgrade ou un pinning préventif tant que le build UE5.5.4 et Shipping sont verts** ;
- documenter la toolchain réellement utilisée ;
- considérer tout changement de toolchain comme un point à revalider avec les deux harness ;
- rouvrir ce point si UBT transforme le warning en incompatibilité ou si deux environnements produisent des résultats divergents.

## 7. Clang-format

La baseline de formatage utilise :

```text
clang-format 19.1.5
```

Contrôle :

```powershell
.\Scripts\CheckCppFormat.ps1
```

La dérive globale historique de formatage reste une dette distincte ; TD07.1 ne lance aucun reformatage massif.
