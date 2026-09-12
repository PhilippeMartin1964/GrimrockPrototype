# LUA-UX04 — Lua Syntax Highlighting

Statut : **implémenté, validation UE5.5.4 utilisateur requise**  
Date : **12 septembre 2026**  
Baseline : `059b390b905202882411df1040ba57e1c4c2580d`

## Objectif

LUA-UX04 ajoute une coloration syntaxique au champ Source de la fenêtre **GRIMROCK LUA — SCRIPTS** sans modifier le langage, le texte sauvegardé ni le compilateur LUA-COMP01.

La séparation reste stricte :

```text
Source Lua
   |
   +-- affichage -> FGridLuaSyntaxHighlighter
   |
   +-- compilation -> FGridLuaAuthoringCompiler
```

Le highlighter est donc **présentation uniquement**. Il ne valide, ne transforme et ne réécrit jamais la source.

## Implémentation

Nouveau marshaller Slate :

```text
FGridLuaSyntaxHighlighter
    -> FSyntaxHighlighterTextLayoutMarshaller
    -> FSyntaxTokenizer
    -> FSlateTextRun
```

Le `SMultiLineEditableTextBox` existant conserve son rôle d'éditeur de texte, avec :

```cpp
.Marshaller(FGridLuaSyntaxHighlighter::Create())
```

Le texte reste éditable normalement et `OnTextChanged` continue d'alimenter `DraftSource` sans balises ni données de présentation.

## Catégories visuelles

La palette est volontairement sobre :

```text
Lua keyword       violet/bleu     function, if, then, elseif, end, return, local...
Lua literal       bleu clair      true, false, nil
Number            cyan            0..9 dans les littéraux numériques usuels
String            vert            "GuardianDoor", "Open", 'text', [[text]]
Comment           vert/gris       -- commentaire, --[[ commentaire ]]
persistent        or              persistent
Grimrock API      cyan/bleu       grid.command, grid.visual.set_material, grid.vars.*, grid.log
Normal            gris clair      identifiants ordinaires
```

Tous les runs utilisent la fonte Slate `Mono` afin de rendre le code plus lisible.

## Robustesse de parsing

Le marshaller maintient un état de présentation pour :

```text
-- commentaire sur une ligne
--[[ commentaire multiligne ]]
"chaîne double"
'chaîne simple'
[[chaîne multiligne]]
```

Les guillemets échappés `\"` et `\'` ne ferment pas prématurément une chaîne.

Les mots-clés ne sont colorés que lorsqu'ils constituent un token autonome : un identifiant tel que `endif_value` ne colore pas artificiellement `end` ou `if`.

Les chiffres intégrés dans un identifiant, par exemple `Door2`, restent neutres. Les chiffres de valeurs comme `>= 2` sont colorés comme nombres.

## Grimrock API

Les noms complets sont tokenisés comme une seule unité visuelle :

```lua
grid.command("GuardianDoor", "Open")
grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
```

Cela permet de distinguer immédiatement :

```text
grid.command                -> API moteur
"GuardianDoor"              -> argument Lua / LogicId
"Open"                      -> argument Lua / Command
```

LUA-UX04 ne vérifie pas si ces arguments sont valides. Cette responsabilité reste exclusivement celle de LUA-COMP01.

## Non-objectifs

LUA-UX04 n'ajoute pas encore :

- soulignement des erreurs de compilation ;
- gutter / numéros de ligne ;
- navigation automatique vers `line:column` ;
- auto-complétion ;
- thèmes configurables ;
- refactoring ou formatage automatique.

Ces fonctions pourront être ajoutées séparément si elles deviennent utiles.

## Tests

Nouveau filtre :

```text
Grimrock.LUAUX04
```

Le test vérifie :

- création du marshaller live ;
- classification des mots-clés Lua ;
- classification de `true/false/nil` ;
- classification des nombres ;
- classification de `persistent` ;
- classification de `grid.command` et `grid.visual.set_material` ;
- neutralité d'un identifiant ordinaire.

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.LUAUX04"
```

Puis non-régression Lua :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19"
```
