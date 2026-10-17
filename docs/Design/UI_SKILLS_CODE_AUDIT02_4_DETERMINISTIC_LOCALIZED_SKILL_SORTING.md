# UI-SKILLS-CODE-AUDIT02.4 — Deterministic Localized Skill Sorting

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ / CLOS**

## 1. Défauts corrigés

Le tri des Skills comportait deux dépendances fragiles :

~~~text
libellés joueurs -> FString::Compare(..., IgnoreCase)
affinité         -> static_cast<uint8>(EGridSkillClassAffinity)
~~~

Le premier chemin ignorait la collation internationale d'Unreal. Le second
faisait de l'ordre numérique actuel de l'enum une règle de présentation
implicite.

## 2. Collation localisée

Les noms de Skill et les libellés d'attribut passent désormais par :

~~~text
FTextComparison::CompareToCaseIgnored(...)
~~~

Les textes restent résolus sous forme FText par les helpers UI existants. La
conversion en chaîne n'est utilisée qu'à l'interface de FTextComparison.

En cas d'égalité du nom affiché, SkillId reste le dernier tie-break technique
déterministe. Il n'est jamais présenté au joueur.

## 3. Ordre d'affinité explicite

Le tri Affinity ne dépend plus de l'ordinal de EGridSkillClassAffinity.

L'ordre sémantique ascendant est codé explicitement :

~~~text
Recommended -> 0
Useful      -> 1
Neutral     -> 2
Discouraged -> 3
~~~

Le sens descendant inverse uniquement cette priorité primaire. À affinité
égale, le nom localisé reste ascendant dans les deux directions, comme pour les
tris Rank et Attribute.

## 4. Comparateur centralisé

La logique n'est plus recalculée séparément par le helper et par la lambda de
tri. Le chemin unique est :

~~~text
CompareLocalizedText(...)
CompareSkillNames(...)
GetAffinitySortPriority(...)
ComparePrimaryColumn(...)
CompareSkillEntries(...)
SortSkillEntries(...)
~~~

Cela évite les divergences futures entre tri ascendant, descendant, affinité et
collation.

## 5. Tests

Le test Name existant vérifie une collation française avec caractère accentué :

~~~text
Évasion < Zoologie
~~~

Un nouveau test est ajouté :

~~~text
Grimrock.UI.Skills02.Sort.Affinity
~~~

Il vérifie l'ordre sémantique ascendant/descendant et le tie-break alphabétique
localisé entre Skills de même affinité.

La suite Grimrock.UI.Skills02 passe de 26 à **27 tests** si aucune autre suite
n'est ajoutée parallèlement.

## 6. Non-objectifs

02.4 ne modifie pas :

~~~text
matrice d'affinité
ResolveClassAffinity(...)
rangs / économie Skill
undo de session
sélection
UMG
.uasset
ST_UI
ordre canonique des assets
~~~

Le tri reste purement présentation et ne réordonne aucune donnée persistée.

## 7. Validation locale

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis, si un diagnostic ciblé est nécessaire :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02.Sort"
~~~

Validation locale reçue :

~~~text
Grimrock.UI.Skills02  27/27 PASS
warnings               0
failures               0
~~~

État final : **UI-SKILLS-CODE-AUDIT02.4 VALIDÉ / CLOS**.
