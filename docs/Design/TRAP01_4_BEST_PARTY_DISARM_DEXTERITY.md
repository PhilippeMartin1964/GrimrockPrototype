# TRAP01.4 — Désamorçage par le meilleur personnage éligible

31 octobre 2026 — Implémentation C++ publiée, compilation et tests locaux UE 5.5.4 à réaliser.
Prérequis validés localement : TRAP01.1 (2/2), TRAP01.2 (2/2), TRAP01.3 (2/2), ARMEMENT01.16.Trap (3/3).

## Contrat validé

- Le piège reste caché jusqu'à la perception ou au déclenchement (TRAP01.1/2). Pas d'attaque sans action explicite sur un piège détecté.
- L'action reste indisponible en combat et exige l'outil `Item_DisarmTools_01` avec le tag `Tool.Disarm` dans le sac sur lequel le joueur a ouvert le menu.
- Le personnage qui **effectue** le désamorçage peut être différent du propriétaire du sac sur lequel le joueur a cliqué. Il doit lui-même être vivant, être éligible à `Skill_Traps` et **posséder ses propres outils**, sans transfert ou téléportation implicite.
- `Skill_Traps` est gouverné par la **Dextérité**. Le rang de compétence et les modificateurs de progression de classe restent pris en compte. L'apprentissage préalable reste obligatoire (`bAllowUntrainedChecks=false`).
- L'autorité de choix et de jet demeure `FRPGSkillCheckService::TryResolveBestPartySkillCheck`. Le runtime fournit un état du groupe copié où les personnages sans outils ne sont pas candidats. La copie n'altère jamais les véritables points de vie ni le sac.
- Le gagnant est le personnage ayant le meilleur bonus statique de compétence effectif ; les égalités utilisent l'ordre stable des membres du groupe conformément au résolveur RPG partagé.
- Une seule tentative de désamorçage est résolue et un seul d20 est tiré ; l'échec laisse le piège armé, la réussite le neutralise durablement et émet `Disarmed` une seule fois.
- Aucun outil n'est consommé. Les requêtes refusées ne consomment ni dé ni action, et ne modifient pas les états de pièges.
- Le flux d'événements annonce le nom réel de l'intervenant, par exemple « Elias désamorce le piège. » ou « Elias échoue à désamorcer le piège. »

## Compatibilité du DataAsset de production

Le projet contient actuellement `Content/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.uasset`, qui était historiquement configuré sur **Intelligence**.

Le catalogue canonique éditorial `FRPGSkillCatalogAuthoring` est modifié vers **Dexterity**. Toutefois, un commit de code via GitHub ne re-sérialise pas le binaire `.uasset` dans Unreal Editor.

**Mesure transitoire explicite :** dans `TryDisarmTriggerTrap`, le runtime du désamorçage duplique l'asset de compétence fourni et règle la Dextérité sur cette seule copie transitoire. Le `DA_Skill_Traps` global n'est pas modifié à l'exécution. Le jet et le choix du personnage utilisent donc bien la Dextérité dès TRAP01.4, même avant la remise à jour du binaire.

**Action locale à prévoir :** ouvrir `DA_Skill_Traps` dans UE5.5.4, régler `Governing Attribute = Dexterity`, vérifier `Allow Untrained Checks = false`, enregistrer le DataAsset, puis inclure ce fichier binaire dans un commit local distinct et validé. Éviter de régénérer aveuglément les 25 autres skills pour ne pas créer de bruit dans les assets. Le UI de compétence peut encore afficher Intelligence tant que ce changement local n'est pas effectué.

## Interfaces modifiées

- `AGridLevelRuntimeActor::TryDisarmTriggerTrap` conserve ses paramètres existants. L'index d'origine désigne désormais le **propriétaire des outils cliqués**, et non forcément le désamorceur. Un paramètre de sortie facultatif `int32* OutDisarmerCharacterIndex` est ajouté à la fin, sans casser les appels historiques.
- `UGridInventoryWidget` transmet la sortie et construit un texte localisé avec le personnage réellement choisi.
- Les modèles de données des niveaux, sauvegardes, pièges, armes, surfaces et états restent inchangés.

## Validation

Commande PowerShell (compilation incluse) :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TRAP01.4"
```

Tests :

- `Grimrock.TRAP01.4.BestEligibleMemberAndDexterity` : 4 membres, outils possédés personnellement, compétence apprise, choix correct malgré l'Intelligence, pas de jet si refus, réussite/échec et persistance.
- `Grimrock.TRAP01.4.CanonicalSkillDexterity` (module Editor) : `Skill_Traps` du catalogue canonique utilise la Dextérité et exige toujours une formation ; la transformation d'une copie legacy préserve le contrat.

Puis, sans recompiler : filtres `Grimrock.RPG.ARMEMENT01.16.Trap`, `Grimrock.TRAP01.3`, `Grimrock.TRAP01.2`, `Grimrock.TRAP01.1`.

## Limites conservées

Le système graphique de sélection sur le mesh révélé et les boutons distants seront détaillés dans TRAP01.5/6, sans permettre de désamorcer les pièges encore cachés.
Le menu contextuel apparaît tant que l'outil et un piège détecté sont présents ; en l'absence de spécialiste éligible, l'exécution sera rejetée avec une explication. Désactiver préventivement le bouton du menu lorsqu'il n'existe aucun spécialiste sera une amélioration de présentation ultérieure.
Les jets non concluants peuvent être retentés pour l'instant, sans modifier ce contrat. Une règle de risque/détérioration à l'échec nécessiterait une spécification distincte.