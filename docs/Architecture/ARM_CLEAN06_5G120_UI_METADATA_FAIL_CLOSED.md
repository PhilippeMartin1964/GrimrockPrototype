# ARM-CLEAN06.5G.120 — Métadonnées UI sans copies d'instance

Parent `b7a037804e131c20e26820964e6a80f609d55d89`. `FGridInventoryItemMetadataReadModel` ne lit plus `FGridItemInstance::Weight` ni `DisplayName`, y compris dans les mondes de test sans `ItemSystem`. En production, les identités canoniques continuent à fournir leur nom, description et masse, et les identités invalides ou inconnues restent sans fallback. Dans les fixtures Automation isolées, seules les métadonnées d'une définition ancienne explicitement donnée et de même identité peuvent être projetées. Sinon, nom technique, description vide et masse zéro.

Les tests 3K et 3O gardent les niveaux N1/N20, refus N0, définition contradictoire et projections de piles, et ajoutent des assertions de refus en absence de définition. Les membres C++ non réfléchis seront supprimés après audit complet. Aucun changement des assets, des sauvegardes ni du chemin UI canonique.
