# ARM-CLEAN06.2W — Physique des lancers des armes ARM canoniques

Les armes ARM de type `ThrowingOneHand` du manifeste possèdent
quatre paramètres de lancer physique distincts : `ThrowSpeed`,
`ThrowArc`, `ThrowLifeSeconds` et `ThrowImpactDropOffset`.
Le quatrième manquait dans `FGridCanonicalProjectilePolicy`.
Le modèle le conserve désormais et le restitue dans
`FGridCanonicalResolvedWeapon` au niveau N1–N20.

Le projecteur Editor-only
`GridArmClean06ProjectilePolicyProjection` part exclusivement
des 22 `ItemPath` actifs du manifeste. Il ne projette
la physique qu'en présence de `bCombatThrowWeapon` ET du
`BuildKind=ThrowingOneHand` explicite. Il refuse toute
contradiction d'identité, vitesse/durée invalide, arc négatif,
décalage d'impact négatif, ou état non prouvé.
Les autres armes conservent leur politique désactivée.

Deux tests Automation vérifient la copie exacte des quatre
valeurs dans les profils et le résultat transient niveau 20,
les packages originaux non modifiés et le refus de valeurs
physiques invalides, sans autoriser de projectile fantôme.

**Reste nécessaire avant toute migration binaire** : copie
sans perte des paramètres purement visuels du projectile
(orientation du mesh, échelle, axe `ProjectileMeshForwardAxis`,
`Stable/Tumble/Spin`, vitesse et axe de rotation), et
parité de la consommation de projectile/ramassage dans le combat
et les SaveGame. Les données de présentation doivent appartenir
au catalogue visuel ; elles ne sont pas un coût ou des dégâts.

Aucun asset .uasset, niveau, UI ou système historique
n'est modifié. Validation UE5.5.4/Automation `Grimrock`
requise sur le SHA exact.
