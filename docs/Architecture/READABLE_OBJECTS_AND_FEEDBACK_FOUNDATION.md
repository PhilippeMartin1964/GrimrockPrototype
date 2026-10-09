# Objets lisibles et retours d'interaction

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Les anciens noms \`FGridLevelObjectData::OverrideReadableText\` /
> \`UGridLevelAsset::Objects\` ne sont plus le schéma courant.

## 1. World Object readable

Une \`UGridWorldObjectDefinitionAsset\` peut activer :

\`\`\`text
bIsReadable
ReadableText
bShowReadableOnlyOnce
\`\`\`

Un placement \`FGridWorldObjectInstance\` peut fournir :

\`\`\`text
ReadableTextOverride
\`\`\`

Un override vide signifie que le texte de la définition reste applicable.

\`Notes\` est de l'authoring et ne devient jamais automatiquement du texte
joueur. \`LogicId\` est une identité/alias et n'est pas un libellé.

## 2. Loose Items lisibles

\`FGridLooseItemInstance\` peut utiliser les données lisibles dédiées :

\`\`\`text
ReadableContentAsset
ReadableContentId
ReadTitleOverride
ReadTextOverride
\`\`\`

Le contenu joueur doit venir de ces autorités explicites, pas de Notes/IDs.

## 3. Runtime

\`AGridGenericObjectActor\` porte le comportement générique d'un World Object
readable lorsque la définition l'autorise.

\`CanInteract()\` refuse une lecture sans texte effectif ou déjà consommée
lorsque \`bShowReadableOnlyOnce\` s'applique.

Les règles de côté/portée restent celles de l'interaction monde.

## 4. Message lisible

\`UReadableMessageWidget\` est une surface de présentation. Le runtime choisit
le texte puis ouvre/ferme le widget.

Le message lisible actif possède une priorité spéciale dans le routage souris :
le clic suivant le ferme et ne déclenche pas d'interaction derrière lui.

Mouvement/rotation peuvent également le fermer.

## 5. Feedback court

Le feedback de refus/confirmation court doit rester distinct du message lisible
long :

- il ne crée pas une nouvelle autorité métier ;
- il ne remplace pas les logs d'une configuration invalide ;
- il ne doit pas être déclenché par le simple hover.

## 6. Curseur

Le curseur annonce l'affordance résolue par le controller/acteur.
\`SetGridInteractionCursor()\` centralise le feedback curseur custom.

Le curseur ne garantit pas qu'une mutation réussira si l'état change entre
hover et clic ; la validation métier est répétée lors de l'action.

## 7. Validation Editor

La validation doit détecter les configurations clairement incohérentes :
définition readable sans contenu attendu, override impossible, référence de
contenu invalide, etc., sans muter silencieusement le LevelAsset.

## 8. Invariants

1. Notes/LogicId ne sont jamais des textes joueur implicites.
2. Définition + override local constituent l'authoring World Object.
3. ReadableContent dédié constitue l'authoring des items lisibles.
4. Message long et feedback court restent distincts.
5. Hover = zéro mutation.
6. Le runtime reste autoritaire sur la possibilité réelle d'interaction.
