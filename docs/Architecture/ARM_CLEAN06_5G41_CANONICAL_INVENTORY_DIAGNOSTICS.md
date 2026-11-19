# ARM-CLEAN06.5G.41 — Lectures du cache isolées et diagnostics canoniques

Parent bbe81ff1d02c41eddab48901743e0f671f30b04b. Publication après deux statuts SUCCESS.

- `FindItemDefinition` n'accède au cache qu'en Automation active ; les appelants natifs provisoires obtiennent nullptr autrement. Le reset du cache en réhydratation canonique est réservé aux fixtures.
- Diagnostics des définitions et de compatibilité : liste d'identités triées du seul ItemSystem, résolutions N0 ou bornes N1/N20, permissions de main ARM et slots non-ARM, lumière canonique. Un registre absent ou une résolution invalide est signalé explicitement, sans ancienne définition. Le diagnostic d'instance existant est inchangé ; pas de nouvelle règle de gameplay. Le test 3AC couvre toutes les identités, l'unicité, les poids, les permissions du bouclier et des munitions, et l'absence du registre. Le format des journaux de définitions devient CanonicalDefinitions/Kind ; les diagnostics d'ownership et de personnage restent inchangés.
- `RuntimeItemDefinitionsById` demeure UPROPERTY Transient pour retenir les objets des fixtures pendant GC. La suppression des appelants historiques et des tests qui en dépendent reste à faire ; ne pas présenter ce lot comme zéro legacy. Pas de changement d'asset, de sauvegarde ou de texte joueur.

Validation : UE5.5.4/SilentTask, LocalAuto et ultérieurement configuration sans Automation.
