# PERF-PROFILE01 — Profilage cible UE5.5.4 avec Unreal Insights

Date : 19 octobre 2026
Etat : **implementation publiee, capture et validation locale attendues**.

## Objectif

Mesurer les couts CPU du Flux, du HUD persistant, des lectures RPG et du
blocage par surpoids dans de vraies sessions de jeu. Les jalons EVENTHUD01.18
et RPG-PERF01 ont ete valides fonctionnellement, mais leur gain en ms/FPS n'a
pas ete mesure. Aucune amelioration numerique ne doit etre annoncee avant
capture des traces.

Le script lance UnrealEditor.exe en mode standalone (-game), non en PIE.
Ne pas comparer sans precaution les temps de ce mode a ceux de PIE.

## Instrumentation : 17 scopes C++

Tous les scopes utilisent TRACE_CPUPROFILER_EVENT_SCOPE avec l'en-tete natif
ProfilingDebugging/CpuProfilerTrace.h et le canal cpu d'Unreal Insights.

| Domaine | Timers Insights |
| --- | --- |
| Flux | PERF_PROFILE01_EventFeed_AppendRow, PERF_PROFILE01_EventFeed_RebuildAllRows, PERF_PROFILE01_EventFeed_SynchronizePublication, PERF_PROFILE01_EventFeed_DeferredScroll |
| HUD | PERF_PROFILE01_HUD_NativeTick, PERF_PROFILE01_HUD_RefreshFromSources, PERF_PROFILE01_HUD_RefreshPartyPortraits, PERF_PROFILE01_HUD_SynchronizeEventFeedLayout, PERF_PROFILE01_HUD_SynchronizeActionBar, PERF_PROFILE01_HUD_RefreshActionWidgets |
| RPG | PERF_PROFILE01_RPG_ProgressionPointsRead, PERF_PROFILE01_RPG_CharacterSummary, PERF_PROFILE01_RPG_PortraitViews, PERF_PROFILE01_RPG_OverloadScan, PERF_PROFILE01_RPG_CharacterOverloaded |
| Pawn | PERF_PROFILE01_Pawn_ProgressionPoll, PERF_PROFILE01_Pawn_TryStartMove |

Ces scopes sont imbriques : par exemple RPG_CharacterOverloaded est compris
dans RPG_OverloadScan, et RPG_CharacterSummary peut etre inclus dans
RPG_PortraitViews. **Ne pas additionner leurs durees inclusives.**
Le compteur Pawn_ProgressionPoll ne s'execute qu'a la frequence reelle
du rappel (~1 Hz), pas a chaque Tick.

Aucune modification de gameplay, WBP, assets ou sauvegardes.

## Premiere validation locale : fermer Unreal Editor

Executer depuis PowerShell :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Performance.Profile01"
    .\Scripts\CapturePerfProfile01.ps1 -EngineRoot D:\UE_5.5 -Scenario Baseline -PreflightOnly

Le nouveau test Grimrock.Performance.Profile01.InstrumentationContract
verifie les 17 scopes et le lanceur. Le preflight verifie la presence
des executables et de la commande, **sans lancer Unreal**.
Ces controles ne prouvent pas encore la validite d'une trace ni les temps CPU.

## Enregistrement dans une vraie session de jeu

Le script attend la fermeture du jeu et verifie ensuite l'existence d'une
trace non vide. Exemple :

    .\Scripts\CapturePerfProfile01.ps1 -EngineRoot D:\UE_5.5 -Scenario Baseline

Chaque enregistrement produit localement :

    Saved\Profiling\PERF-PROFILE01\
      PERF-PROFILE01-Baseline-<date-heure>.utrace
      PERF-PROFILE01-Baseline-<date-heure>.json

Le JSON documente la revision Git, le scenario, les canaux, les horaires
et le chemin du fichier. Les canaux actives sont cpu,frame,bookmark,log.
Les fichiers .utrace restent locaux et ne doivent pas etre commits.

Lorsque le donjon a termine son chargement, ouvrir la console du jeu
si elle est disponible, puis saisir :

    Trace.Bookmark PERF_PROFILE01_Begin

Effectuer le scenario. A la fin saisir :

    Trace.Bookmark PERF_PROFILE01_End

Fermer normalement le jeu. Les traces incluent aussi le chargement et les
menus : **ne comparer que l'intervalle de gameplay entre les deux marqueurs**.
Les marqueurs delimitent une plage d'analyse ; ils ne demarrent ou
n'arretent pas eux-memes la capture.

Si la console n'est pas disponible, noter une plage temporelle stable et
la selectionner manuellement dans Insights. Autre possibilite : une capture
PIE par le panneau Trace de l'Editor, analysee separement du standalone.

## Scenarios proposes

La version, la resolution, les options graphiques, la carte et la
sauvegarde doivent rester identiques. Utiliser de preference une session
distincte par scenario. Noter **le nombre reel** d'actions/evenements.

| Nom -Scenario | Duree | Actions entre marqueurs | Elements a observer |
| --- | --- | --- | --- |
| Baseline | 60 s | Rester immobile dans le donjon, sans UI ouverte | NativeTick HUD, Game Thread |
| EventFeed | 60 s | Provoquer regulierement des messages de jeu ; noter leur nombre | AppendRow, SynchronizePublication, DeferredScroll |
| Movement | 60 s | Faire des pas dans une zone libre, puis des tentatives sous surcharge | TryStartMove, OverloadScan, CharacterOverloaded |
| Progression | 120 s | Jouer dans le donjon avec des points RPG non depenses | ProgressionPoll, ProgressionPointsRead |
| PortraitHUD | 60 s | Selectionner des personnages, ouvrir/fermer inventaire et feuille | PortraitViews, CharacterSummary, RefreshPartyPortraits |

Pour voir effectivement le rappel du Flux toutes les cinq minutes, prolonger
la session Progression a environ 6 minutes. En 120 s, les lectures
periodiques sont observables, mais le rappel a cinq minutes n'est pas garanti.

Pour Movement, noter le nombre de pas demandes et le nombre de refus.
Pour EventFeed, noter combien de messages ont effectivement ete publies.

## Analyse dans Unreal Insights

Le lanceur affiche apres fermeture une commande ouvrant UnrealInsights.exe
avec l'option -OpenTraceFile. Sinon ouvrir manuellement la trace .utrace
dans le navigateur des traces.

1. Dans Timing Insights, selectionner les marqueurs Begin -> End ou la
   plage manuellement choisie. Exclure demarrage, compilation shaders,
   chargement et transitions de carte.
2. Examiner le Game Thread et les frames CPU.
3. Dans Timers, rechercher PERF_PROFILE01_ et relever Calls, Total,
   Average, Maximum, Inclusive et Exclusive lorsque disponibles.
4. Comparer la frequence d'appel et le cout par evenement, sans sommer
   les fonctions qui s'appellent mutuellement.
5. Noter les limites de la machine et le nombre d'actions reelles.

Tableau de restitution (aucune valeur pre-remplie) :

| Scenario | Duree s | Actions | Frame Game Thread moyen ms | Timer | Calls | Average ms | Maximum ms |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| Baseline | — | — | — | PERF_PROFILE01_HUD_NativeTick | — | — | — |
| EventFeed | — | — | — | PERF_PROFILE01_EventFeed_SynchronizePublication | — | — | — |
| Movement | — | — | — | PERF_PROFILE01_RPG_OverloadScan | — | — | — |
| Progression | — | — | — | PERF_PROFILE01_Pawn_ProgressionPoll | — | — | — |
| PortraitHUD | — | — | — | PERF_PROFILE01_RPG_PortraitViews | — | — | — |

Un cout tres faible dans ces timers n'exclut pas un cout GPU ou Slate
qui n'est pas couvert par ces scopes. Ce protocole cible surtout le CPU.

## Controles gameplay de non-regression

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.PERF01"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Runtime.PartyWeight01"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01.Interaction"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.PartyHud01"

Verifier egalement en jeu que les badges, l'historique, les tooltips,
les points et le verrouillage par surcharge fonctionnent comme avant.

## Limites et livrables attendus

Le script lance une nouvelle session standalone via l'editeur Development.
Il ne recueille pas automatiquement les statistiques de Timers :
l'analyse necessite Unreal Insights et des donnees locales. La presence
du fichier utrace ne garantit pas a elle seule la validite des timers.

Un bilan de gains « avant/apres » demanderait un profil comparatif
de l'ancienne revision, dans les memes conditions. Le protocole ci-dessus
mesure en premier lieu le cout actuel et les priorites futures.

Pour clore PERF-PROFILE01, fournir le resultat TD04.2, le preflight,
les fichiers metadata .json et les tableaux ou captures des Timers
pour au moins Baseline et EventFeed. Partager les .utrace seulement
si leur taille le permet.

## Documentation Epic

- https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-insights-reference-in-unreal-engine-5
- https://dev.epicgames.com/documentation/en-us/unreal-engine/developer-guide-to-tracing-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/trace-quick-start-guide-in-unreal-engine
