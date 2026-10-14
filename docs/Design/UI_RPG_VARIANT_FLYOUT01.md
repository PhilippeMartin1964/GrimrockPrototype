# UI-RPG-VARIANT-FLYOUT01 — Contrat UX des Talents à variantes

Date : 14 octobre 2026

## Décision

Un Talent conceptuel à variantes reste un seul nœud dans l'arbre.

Avant acquisition, un clic sur ce nœud ouvre une popup ancrée. La popup contient uniquement les icônes des variantes concrètes. Aucun texte, coût, statut, tooltip ou bouton d'acquisition n'est porté par la popup.

Un clic sur une icône concrète :

1. sélectionne son ChoiceId pour consultation uniquement ;
2. ferme la popup ;
3. projette la variante concrète dans le panneau Talent normal.

Le panneau droit se comporte alors exactement comme pour un Talent simple. ACQUÉRIR / CONFIRMER reste l'unique flux d'acquisition.

## Identité visuelle dans l'arbre

Avant acquisition, le nœud utilise l'icône générique du TalentNodeId.

Après acquisition, FGridTalentNodeView::SelectedChoiceId est autoritaire et l'icône du nœud est remplacée par celle du ChoiceId acquis.

Un nœud à variante déjà acquis ne rouvre jamais la popup. Un clic affiche directement le détail de la variante acquise.

Aucun changement libre n'est autorisé. Le remplacement d'une variante acquise est réservé à un futur système de déspécialisation.

## Consultation des variantes verrouillées

La popup est une surface de consultation et non une autorité d'acquisition. Une variante verrouillée reste consultable. Son état concret et la transaction déterminent si ACQUÉRIR est disponible.

## Autorité visuelle

URPGClassVisualAsset porte deux mappings :

- TalentNodeVisuals : TalentNodeId conceptuel -> icône ;
- TalentVariantVisuals : ChoiceId concret -> icône.

Aucun parsing de nom d'asset ou logique spécifique à une classe n'est autorisé.

## Layout popup

Le composant est générique 2..N :

- 2 variantes : 2 x 1 ;
- 3-4 variantes : 2 colonnes ;
- 5+ variantes : 3 colonnes.

Chaque entrée est uniquement une icône. Taille Designer visée : 100 x 100.

## Matérialisation UMG

Le flyout n'utilise plus `UMenuAnchor`.

Le projet possède un curseur custom `WBP_GridMouseCursor` ajouté au viewport à
Z=9999. Le contenu différé de `SMenuAnchor` peut être peint au-dessus de cette
couche, ce qui rend le curseur invisible sur la popup. Le flyout est donc ajouté
directement au viewport à Z=9000 : au-dessus de l'écran Skills, mais sous le
curseur Grimrock.

`WBP_RPGTalentNode` conserve `VariantFlyoutWidgetClass =
WBP_RPGTalentVariantFlyout`. Le `MenuAnchor_Variants` éventuellement présent
dans l'asset n'est plus consommé par le C++ et doit être retiré lors du prochain
nettoyage UMG, en replaçant `Button_TalentNode` directement sous
`SB_NodeRoot`.

`WBP_RPGTalentVariantFlyout` :
- parent `UGridTalentVariantFlyoutWidget` ;
- un cadre visuel contenant `UniformGridPanel Grid_Variants` ;
- `VariantIconWidgetClass = WBP_RPGTalentVariantIcon`.

`WBP_RPGTalentVariantIcon` :
- parent `UGridTalentVariantIconWidget` ;
- SizeBox 100 x 100 ;
- `Button_Variant` ;
- `Image_VariantIcon` ;
- aucun TextBlock.

Le positionnement est calculé depuis la géométrie du nœud cliqué puis converti
en coordonnées viewport. La popup tente d'abord le côté droit du nœud, bascule
à gauche si nécessaire et reste verticalement dans le viewport. Un clic qui
revient dans la hiérarchie Skills ferme le flyout ; un clic sur une variante
ferme le flyout puis projette le ChoiceId dans le panneau droit.

## Nettoyage du panneau droit

Le choix de variante ne relève plus de WBP_RPGTalentDetail.

À retirer du Designer :
- HB_AcquireConfirm ;
- Text_AcquirePrompt ;
- Text_AcquireFeedback.

Le feedback de transaction est exclusivement porté par le toast.

Après matérialisation du flyout, l'ancienne section de sélection de variantes dans le détail pourra être supprimée avec son presenter legacy.

À supprimer de DA_RPGTalentDetailTextSet :
- Talent.Acquisition.Prompt.ConfirmVariant ;
- Talent.Acquisition.Prompt.ChooseVariant.

Talent.Acquisition.Exclusivity.SingleVariant reste utile dans les conditions d'acquisition.
