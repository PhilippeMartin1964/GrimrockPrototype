# Documentation GrimrockPrototype

Les références ci-dessous décrivent le projet après la [recette intégrée et la clôture de CLEANUP08](Technical/Consolidation.md#cleanup08--recette-intégrée-et-clôture). Les rapports datés décrivent leur révision. Une ancienne validation ou procédure de migration ne remplace pas les contrats courants.

| Besoin | Référence |
| --- | --- |
| Comprendre le projet | [Synthèse](Architecture/PROJECT_SYNTHESIS.md), [index d’architecture](Architecture/ARCHITECTURE_INDEX.md) et [carte des domaines](Architecture/Maps/GRIMROCK_PROJECT_MAP.md) |
| Développer un objet ou une arme | [Modèle relationnel](Architecture/DataModel.md), [invariants](Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) et [édition](Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md) |
| Concevoir et examiner un objet en 3D | [Fiche maîtresse](Design/Armement/ARMEMENT_DESIGN01_FICHE_MAITRESSE.md), [cahier des charges](Design/Armement/ARMEMENT_INSPECTION01_SPECIFICATION.md), [addendum approuvé](Design/Armement/ARMEMENT_INSPECTION01A_ADDENDUM.md), [plan d’implémentation](Design/Armement/ARMEMENT_INSPECTION02_PLAN_IMPLEMENTATION.md) et [socle commun](Design/Armement/ARMEMENT_INSPECTION02_1_SOCLE_COMMUN.md) |
| Développer une interface | [Architecture UI](Design/UI_ARCHITECTURE_CURRENT.md), [contrat UMG](UI_UMG_PRESENTATION_CONTRACT.md), [workflow obligatoire des textes](UI_WBP_LOCALIZATION_WORKFLOW.md) |
| Préparer le poste, compiler ou profiler | [Environnement](Design/DEVELOPMENT_ENVIRONMENT_SETUP.md), [scripts](SCRIPTS_REFERENCE.md), [agent Windows](Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md), [Unreal Insights](Design/PERF_PROFILE01_UNREAL_INSIGHTS_CAPTURE.md) |
| Choisir un jalon | [Roadmap](Design/PROJECT_COMPLETION_ROADMAP.md), [dette technique](Architecture/TECHNICAL_DEBT_REGISTER.md), [consolidation](Technical/Consolidation.md) |
| Recette et règles | [Smoke test donjon](Tests/TEST_DUNGEON_PASS_CHECKLIST.md), [réceptacles](Tests/RECEPTACLE_RUNTIME_TESTS.md), [interaction grille](Runtime_EdgeInteractionRules.md) et [ramassage](Runtime_ItemPickupRules.md) |
| Art et licences | [Pipeline matériaux](00_README_Art_Materials_Textures.md), [meshes Blender](05_Static_Mesh_Blender_UE5_5_4_Pipeline.md), [licences des polices](FONT_LICENCES.md) et [ArtBook](ArtBook/) |
| Retrouver une décision ou une preuve | [Journal des décisions](Design/99_DECISIONS_LOG.md) et [synthèses historiques](History/README.md) |

`Architecture/` décrit les contrats, `Design/` les règles de gameplay et projets futurs, `Technical/` les validations techniques, `Tests/` les recettes et `History/` les synthèses datées. Les chemins existants des guides techniques sont conservés. `Rules/` reste destiné aux règles transversales et `UI/` à ses recettes spécialisées. Les ressources sources et visuelles restent dans leurs répertoires.

Chaque domaine a une référence identifiée. Les [instructions permanentes](../AGENTS.md) régissent les opérations Git, UE et UI. Pour les résultats courants, consulter le SHA et les preuves dans la consolidation.
