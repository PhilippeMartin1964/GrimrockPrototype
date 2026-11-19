# ARM-CLEAN06.5G.39 — Enregistrement historique réservé aux fixtures Automation

Parent 175ba1043faab4c7e93e9dc7089fb2454af44231. Publication uniquement après SUCCESS de SilentTask et LocalAuto du parent.

La recherche de code GitHub sur RegisterItemDefinition a retourné 73 fichiers, dont 64 fichiers de tests, les deux fichiers de déclaration/implémentation et la documentation. Aucun appelant C++ de production identifié. 5G.37/38 couvrent les appels Blueprint directs et le retrait de la réflexion. L’index GitHub n’est pas une preuve exhaustive des appels dynamiques.

Encadrer déclaration et définition de RegisterItemDefinition par WITH_DEV_AUTOMATION_TESTS, à l’identique du helper historique RehydrateOwnedItemDefinitions. Conserver intégralement le comportement et les assertions des fixtures. Les autres lecteurs et le cache restent présents : TryBuildKit peut encore fournir StartingDefinitions aux chemins historiques, les diagnostics et FindItemDefinition lisent encore la carte. Le retrait du cache n’est pas revendiqué ici.

Validation : UE5.5.4/SilentTask et UE5.5.4/LocalAuto au SHA publié ; suite Grimrock sans régression. La compilation sans WITH_DEV_AUTOMATION_TESTS (Shipping) devra être vérifiée lors du gate final. Aucun asset ni SaveGame modifié.
