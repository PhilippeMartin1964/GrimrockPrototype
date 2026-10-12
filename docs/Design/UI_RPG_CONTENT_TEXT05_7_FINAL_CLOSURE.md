# UI-RPG-CONTENT-TEXT05.7 — Final closure

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT05.7**

## 1. Objet

Fermer la campagne `UI-RPG-CONTENT-TEXT05` après les migrations Skills et
Spells, en résolvant le dernier reliquat identifié par TEXT05.1 :
`Attack_Unarmed`.

## 2. Décision Attack_Unarmed

`Attack_Unarmed` est une primitive runtime universelle intégrée au système de
combat :

- elle existe pour tous les personnages ;
- elle n'a pas de cycle d'authoring indépendant ;
- elle ne possède aucune mécanique variable par contenu ;
- elle est créée par le catalogue comme fallback/option universelle ;
- son icône est déjà une présentation runtime dédiée du TurnManager.

Créer un DataAsset seulement pour :

```text
À mains nues
Attaque physique effectuée sans arme.
```

introduirait une nouvelle famille d'assets et une nouvelle autorité pour deux
chaînes sans bénéfice de gameplay ou de structure.

La décision finale est donc :

```text
Attack_Unarmed.ActionId + mécanique
    = C++ runtime primitive

Attack_Unarmed DisplayName + Description
    = LOCTEXT localisé
    = vocabulaire système explicite
    = pas du contenu RPG authored
```

Cette exception est du même ordre que les raisons d'indisponibilité du
catalogue (`CombatInactive`, `InsufficientMana`, etc.).

## 3. Frontière finale CONTENT-TEXT

Les familles authored sont désormais :

```text
DA_Class_*
    -> Classes / Choices / CombatActions

DA_RPGTalentPresentation
    -> branches / nœuds conceptuels

DA_RPGTalentDetailTextSet
    -> vocabulaire Talent générique

DA_Item_*
    -> items / QuickItem presentation

DA_Status_*
    -> Status Effects

DA_MONCAT_*
    -> catégories de monstres

DA_Skill_*
    -> Skills

DA_Spell_*
    -> Spells + Presentation
```

Le C++ conserve :

```text
stable IDs
mécanique
validation
read models
résolution runtime
fallbacks techniques
vocabulaire système localisé
```

## 4. Résultat TEXT05

```text
P1 Skills          CLOSED
P2 Spells          CLOSED
P3 Attack_Unarmed  CLOSED
Final audit        CLOSED
```

### Skills

- 25 `DA_Skill_*` autoritaires pour le `DisplayName` ;
- aucun `DisplayName` canonique dans `FRPGCanonicalSkillDefinition` ;
- bootstrap neuf = `SkillId`.

### Spells

- quatre `DA_Spell_*` matérialisés ;
- `FGridProductionSpellLibrary` est une façade de résolution ;
- aucune prose Spell authored en C++ ;
- aucun coût/effet Spell authored en C++ ;
- infrastructure temporaire de matérialisation retirée.

### Attack_Unarmed

- mécanique inchangée ;
- deux `LOCTEXT` explicitement documentés comme vocabulaire système ;
- aucun DataAsset artificiel ajouté.

## 5. FinalAudit

Le test :

```text
Grimrock.UI.RPG.CONTENTTEXT05.FinalAudit
```

vérifie notamment :

- absence de `DisplayName` authored dans l'authoring Skills ;
- absence de prose/mécanique authored dans la façade Spell ;
- résolution Spell par `UGridSpellDefinitionAsset` ;
- présence exacte des deux chaînes système `Attack_Unarmed` ;
- commentaire source explicite de l'exception ;
- stabilité de l'identité/mécanique `Attack_Unarmed` ;
- suppression de l'authoring/commandlet/runner Spell temporaires.

## 6. Ce que cet audit ne cherche pas à supprimer

TEXT05 ne vise pas à retirer tous les textes localisés du C++.

Les éléments suivants restent légitimes hors contenu RPG authored :

- messages d'erreur ;
- raisons d'indisponibilité ;
- labels d'outils Editor ;
- feedback système ;
- fallback diagnostic ;
- vocabulaire UI générique ;
- primitives runtime sans cycle d'authoring autonome.

Les déplacer systématiquement dans des DataAssets augmenterait le nombre
d'autorités et le coût de maintenance sans améliorer l'architecture.

## 7. Conclusion

`UI-RPG-CONTENT-TEXT05` est terminé architecturalement.

Après validation locale du filtre final :

```text
UI-RPG-CONTENT-TEXT05 — CLOSED
```

Aucun ticket `CONTENT-TEXT06` n'est nécessaire à partir de cet audit.
