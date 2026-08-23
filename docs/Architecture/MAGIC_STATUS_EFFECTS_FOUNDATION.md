# Magie et effets de statut — Fondation d’architecture

## Spellbook et sorts

MON18 fournit un pipeline complet : définition de sort, spellbook par personnage, transaction de cast, ciblage, résolution d’effet, présentation, hotbar, UI et persistance.

```text
Spell Definition
  -> Spellbook known state
  -> Action/Hotbar
  -> Cast transaction (PA + mana)
  -> Targeting
  -> Effect resolver
  -> Presentation
```

Les quatre sorts de production de référence sont Arcane Bolt, Lesser Heal, Haste et Cure Poison.

## Status Effects

MON16 fournit :

- définition data-driven ;
- collection d’effets pour groupe et monstres ;
- stacking/refresh/durée ;
- dégâts périodiques ;
- modification d’initiative ;
- contrôle ;
- présentation ;
- persistance.

Les sorts réutilisent ce moteur lorsqu’un effet durable est nécessaire : Haste applique un Status Effect et Cure Poison agit sur la collection d’effets.

## Autorités

- Spell Definition = données du sort ;
- Spellbook = sorts connus ;
- CastTransaction = validation/paiement ;
- Targeting = cible légale ;
- EffectResolver = effet gameplay ;
- StatusEffectLifecycle = état durable ;
- Presentation = retour utilisateur.

La UI et les VFX ne doivent pas contourner la transaction ou appliquer directement un effet gameplay.

## Persistance

Les spellbooks et Status Effects sont sérialisés via snapshots identifiés par `CharacterId`; les pointeurs runtime vers assets de statut sont reconstruits lors du chargement.

## Suite de production

Le framework est plus mature que le catalogue de contenu : priorité à davantage de sorts, effets, icônes, VFX et équilibrage plutôt qu’à une seconde architecture magique.
