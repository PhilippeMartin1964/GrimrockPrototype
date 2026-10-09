# Magie et effets de statut — Fondation d'architecture

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Les snapshots Spellbook/Status séparés par CharacterId décrits dans
> d'anciens jalons ont été supprimés pendant TD07.3. L'état durable vit
> directement dans \`FGridCharacterInventoryState\`.

## 1. Spellbook

MON18 fournit :

\`\`\`text
Spell Definition
    -> KnownSpellIds du personnage
    -> catalogue / hotbar
    -> cast transaction
    -> targeting
    -> effect resolver
    -> presentation
\`\`\`

\`KnownSpellIds\` est l'autorité durable du Spellbook par personnage.
\`UGridPartySpellbookComponent\` est une façade/runtime projection, pas un
snapshot persistant parallèle.

## 2. Cast transaction

La transaction valide et paie les ressources nécessaires (PA, mana et autres
conditions applicables) avant d'appliquer l'effet.

Un Widget ou un VFX ne doit jamais contourner cette transaction.

## 3. Status Effects

MON16 fournit :

- définition data-driven ;
- collection runtime ;
- stacking / refresh ;
- durée en tours/rounds ;
- dégâts périodiques ;
- contrôle ;
- initiative/modificateurs ;
- présentation ;
- persistance personnage et monstre.

Pour un personnage :

\`\`\`text
FGridCharacterInventoryState::StatusEffects
    = autorité durable
\`\`\`

Les références de définition à l'intérieur des effets sont des caches
transients réhydratés depuis leur identité.

Les monstres persistés utilisent leurs snapshots d'effet stables dans le
DungeonRuntimeState.

## 4. Actions magiques et Talents

Les actions magiques restent des actions du catalogue de combat. Un Talent peut
accorder/modifier une action mais ne crée pas un second moteur de magie.

Le read model Talent lit les vraies primitives de combat et de Status Effects
pour produire les sections EFFETS / UTILISATION.

## 5. UI

Spellbook et Skills/Talents suivent le même
\`UGridPartyInventoryComponent::SelectedCharacterIndex\`.

La UI :

- affiche les sorts connus ;
- affiche coûts et ciblage projetés ;
- permet l'affectation à la barre d'actions ;
- n'applique aucun effet gameplay directement.

## 6. Save

Save courant : **v24 exact-match**.

Il n'existe plus de :

\`\`\`text
CharacterSpellbookStates
CharacterStatusEffectStates
\`\`\`

Les données vivent directement dans le personnage durable.

## 7. Règle de production

Le framework Magic/Status est déjà structurellement complet. Ajouter des sorts,
effets, VFX, icônes et équilibrages doit normalement enrichir les assets et
catalogues existants, pas introduire une deuxième architecture.
