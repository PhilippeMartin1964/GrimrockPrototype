# GrimrockPrototype — Roadmap fonctionnelle

La [baseline de consolidation](../Technical/Consolidation.md) fixe les clôtures techniques jusqu’à CLEANUP06. CLEANUP07 actualise la documentation. **CLEANUP08, recette générale, reste ouvert.** Ce document conserve l’ordre déjà établi et ne sélectionne pas de nouveau chantier produit.

## Fonctionnalités livrées

| Domaine | État et preuve |
| --- | --- |
| Exploration, objets du monde, portes, serrures, réceptacles, fosses | Implémentés. Les parcours retenus ont été validés dans la recette Shipping D.16. |
| Inventaire, équipement, combat et actions rapides | Implémentés. La recette D.16 valide les parcours combinés, dont attaques, parade, blocage, combat à distance, pierres et huile. Le [modèle objets](../Architecture/DataModel.md) précise le contenu effectivement matérialisé. |
| MON13–MON20 et progression RPG | Jalons clos dans leur périmètre. Le recrutement, les classes, compétences, talents, magie et statuts disposent des contrats et validations consignés dans leurs jalons. La clôture ne vaut pas production complète de campagne. |
| MON21.1–.3 | Contrat de campagne, définition et état des quêtes, raccordement Event → Command validés. |
| MON21.6 et MAP-THEME01 | Carte texturée, exploration persistante, multi-dalles et étages validés en Automation et en PIE. La convention visuelle X miroir reste un point documenté, la convention runtime `East=X+` demeure. |
| New Game, Continue et SaveGame | Parcours D.16 validés manuellement. SaveGame **v27 exact-match**, sept sauvegardes éprouvées sur copies. |

## Fonctionnalités partielles et travaux déjà définis

- **MON21.4 — Persistance des quêtes.** Suspension volontaire. MON21.4.2 a livré le champ durable `CampaignQuestState` et le contrat atomique de snapshot. Après décision de reprise, continuer à MON21.4.3 pour l’enregistrement à l’échelle du donjon, puis MON21.4.4 pour relier la capture et la restauration au Pawn. [Contrat détaillé](MON21_4_QUEST_PERSISTENCE.md)
- **MON21.5 / .7 / .8.** Journal lu depuis `UGridQuestSubsystem`, découverte et projection de Codex, puis recette croisée des systèmes, persistance, UI et PIE. [Contrat de campagne](MON21_1_QUESTS_JOURNAL_MAP_CODEX_ARCHITECTURE_AUDIT.md)
- **MON22.** Tranche jouable de 45–90 minutes avec début et fin identifiés. Elle combine exploration et énigmes, Event → Command, Logic et Lua, objets et équipement, recrutement et RPG, magie, monstres et combats, quêtes, journal, carte, codex et sauvegarde.
- **Horizon MON23–MON30.** Conteneurs et pièges de serrure et artisanat, audio/VFX/atmosphère, menus/options/accessibilité, performances produit, packaging/installateur, éditeur autonome, publication de donjons, campagne complète. Cet horizon reste planifié.

## Anomalies et limites connues

- Présentation visuelle du ciblage déjà reportée hors ARM-CLEAN06. La recette du gameplay ne clôt pas ce défaut visuel.
- Profilage CLEANUP05 limité au poste testé en Development D3D12 1080p. L’attente de rendu observée pendant la phase RPG/flux reste sans cause projet attribuée. Les mesures ne constituent pas une certification graphique Shipping.
- Quelques accents des diagnostics sont altérés par l’encodage hérité du bootstrap Windows. Les statuts et la sélection du SHA restent vérifiés.

## Décisions produit attendues

Confirmer l’ordre de reprise fonctionnelle après CLEANUP08. L’extension des gammes d’équipement, leurs variantes et prérequis reste conceptuelle. Les 82 archétypes du catalogue mécanique ne signifient pas 82 armes jouables. Les règles futures de lancer et leur intégration aux Skills demandent un contrat explicite si prévues.
