# ARM-CLEAN06.2AA — Liaisons transitoires de présentations par ActionId connu

## But

Construire réellement dans un **candidat Unreal en mémoire** les
seules propositions de 06.2Z, sans toucher les fichiers historiques
ni présumer une autorisation de conversion binaire.

Le service Editor-only
`GridArmClean06ProvisionalPresentationAssembly::Apply`
appelle le projecteur read-only de 06.2Z pour les **22 ARM
du manifeste** et trie les profils dont la présentation
historique est valide, mais sans `PresentationProfileId`.

Chaque liaison admissible utilise le `ActionId` original
déjà authoré comme `PresentationId` **provisoire**.
Le candidat reçoit alors :
- un profil de présentation copié sans perte depuis le
  DataAsset réel dans `DA_ActionPresentationCatalogue` transitoire ;
- une liaison `ActionId/PresentationId` transitoire dans
  l'archétype ARM correspondant, sans changement de MainHand/OffHand.

La nouvelle méthode prépare les deux catalogues en copies locales,
contrôle chaque `ActionId`, `ArchetypeId`, unicité et collision,
valide le catalogue des présentations, puis installe les copies
ensemble dans le registre transitoire seulement. Toute
incohérence empêche le transfert.

## Séparation obligatoire des autorités

Le compteur `Baseline.Core.Actions.BoundActions` demeure celui
des **liaisons historiquement authorées**. Le nombre
`AppliedProvisionalBindings` est conservé séparément :
une liaison proposée, même exacte, **ne devient pas une
autorisation de gameplay ou de migration** par ce commit.
La méthode `Baseline.ReadyForBinaryCutover()` conserve donc
son refus. Aucun ancien `DA_Weapon_*` n'est altéré.

Les profils locaux manquants, les réactions, les actions
personnalisées et les munitions sont évalués dans leurs
catalogues respectifs. Les clés préexistantes ne sont jamais
écrasées, les doublons ne sont pas créés, et le second passage
est idempotent.

## Preuve native

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProvisionalPresentationBindingAssembly`
compose les 23 définitions transitoires validées en 06.2Y,
stabilise les propositions 06.2Z, vérifie les références
présentation/action/archétype, la validité globale des
présentations, les compteurs avant/après, la répétabilité
et la propreté des packages source.

Le cutover vers 06.3–06.5 nécessitera toujours un authoring
explicite dans l'Editor UE5.5.4, et des preuves de parité
de gameplay/SaveGame. Les anciennes définitions, WBP,
niveaux et ressources audio/Niagara demeurent inchangés.
