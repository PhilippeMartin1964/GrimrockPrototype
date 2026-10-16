# UI-TEXT-CENTRAL01.2D2 — Physical Talent legacy purge

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## Architecture finale

~~~text
UI vocabulary
    UI.csv -> ST_UI -> FUILocalizationText

Recipe content
    Recipe_* -> DA_RPGRecipeCatalog -> FGridRecipeDisplayView
~~~

Le read-model d'acquisition transporte directement `RecipeId + DisplayName`.
Il ne fabrique plus de clé intermédiaire de localisation pour les recettes.

## Suppression physique

Le type/asset historique Talent, son resolver de compatibilité, les deux
commandlets de migration UI et le commandlet Recipe one-shot D1 sont retirés.
Le test d'intégrité de l'ancien asset disparaît avec son autorité.

Les documents historiques 01.2A/01.2B/01.2C restent conservés comme traces de
migration, mais ne définissent plus l'architecture courante.

## Stop-condition

~~~text
ancien asset Talent absent
ancien type C++ absent
ancien resolver Talent absent
GrantedRecipeTextIds absent
préfixe de compatibilité recette absent du Source/
~~~

Le test `Game.UI.TextCentral.CENTRAL01.TalentLegacyPurge` impose cette
stop-condition. `TalentStringTableRuntime` valide directement ST_UI et les
six formats nommés sans bridge intermédiaire.
