# GrimrockPrototype — Events, Commands et Links

## Objectif

Ce document définit le système de connexions logiques entre objets.

Le but est de remplacer les connexions implicites ou spécifiques par un modèle simple :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand
```

---

## Principe

Un objet source émet un événement.

Le système de liens cherche les liens correspondants.

Chaque lien exécute une commande sur un objet cible.

Exemple :

```text
Button_Normal_01.OnActivate -> Door_Stone_01.ToggleOpen
```

---

## Événements émis

Version recommandée :

```cpp
UENUM(BlueprintType)
enum class EGridObjectEvent : uint8
{
    None UMETA(DisplayName = "None"),

    OnActivate UMETA(DisplayName = "On Activate"),
    OnDeactivate UMETA(DisplayName = "On Deactivate"),
    OnToggle UMETA(DisplayName = "On Toggle"),

    OnEnter UMETA(DisplayName = "On Enter"),
    OnExit UMETA(DisplayName = "On Exit"),

    OnInsertItem UMETA(DisplayName = "On Insert Item"),
    OnRemoveItem UMETA(DisplayName = "On Remove Item"),

    OnUse UMETA(DisplayName = "On Use"),
    OnUnlock UMETA(DisplayName = "On Unlock"),
    OnTimer UMETA(DisplayName = "On Timer"),

    OnSpawn UMETA(DisplayName = "On Spawn")
};
```

---

## Commandes reçues

Version recommandée :

```cpp
UENUM(BlueprintType)
enum class EGridObjectCommand : uint8
{
    None UMETA(DisplayName = "None"),

    Activate UMETA(DisplayName = "Activate"),
    Deactivate UMETA(DisplayName = "Deactivate"),
    Toggle UMETA(DisplayName = "Toggle"),

    Open UMETA(DisplayName = "Open"),
    Close UMETA(DisplayName = "Close"),
    ToggleOpen UMETA(DisplayName = "Toggle Open"),

    Lock UMETA(DisplayName = "Lock"),
    Unlock UMETA(DisplayName = "Unlock"),

    Enable UMETA(DisplayName = "Enable"),
    Disable UMETA(DisplayName = "Disable"),

    StartTimer UMETA(DisplayName = "Start Timer"),
    StopTimer UMETA(DisplayName = "Stop Timer"),
    ResetTimer UMETA(DisplayName = "Reset Timer"),

    Teleport UMETA(DisplayName = "Teleport"),
    Spawn UMETA(DisplayName = "Spawn"),
    Destroy UMETA(DisplayName = "Destroy"),

    ShowText UMETA(DisplayName = "Show Text"),
    PlayAnimation UMETA(DisplayName = "Play Animation"),
    PlaySound UMETA(DisplayName = "Play Sound")
};
```

Note d’état 2026-05-19 : les commandes `Lock`, `Unlock`, `Spawn`, `Despawn`, `Teleport` et `ShowMessage` sont déclarées côté C++, mais leur exécution runtime reste partielle et dépend du type de cible. Les cibles qui ne supportent pas encore une commande doivent la refuser avec un log clair plutôt que modifier le gameplay silencieusement.

---

## Structure des liens

Structure cible :

```cpp
USTRUCT(BlueprintType)
struct FGridObjectLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    FName SourceObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    EGridObjectEvent SourceEvent = EGridObjectEvent::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    FName TargetObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    EGridObjectCommand TargetCommand = EGridObjectCommand::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    float Delay = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Link")
    bool bOneShot = false;
};
```

Selon l’existant, cette structure peut être ajoutée dans `GridTypes.h` ou dans un fichier dédié.

---

## Exemples de liens

### Bouton vers porte

```text
Button_Normal_01.OnActivate -> Door_Stone_01.ToggleOpen
```

### Bouton secret vers porte secrète

```text
Button_Secret_01.OnActivate -> Door_Secret_01.Open
```

### Levier vers téléporteur

```text
Lever_01.OnToggle -> Teleporter_Rune_01.Toggle
```

### Support de torche vers porte

```text
Receptacle_TorchHolder_01.OnInsertItem -> Door_Stone_01.Open
Receptacle_TorchHolder_01.OnRemoveItem -> Door_Stone_01.Close
```

### Fente à pièce vers porte secrète

```text
Receptacle_CoinSlot_01.OnInsertItem -> Door_Secret_01.Open
```

### Timer

```text
Button_Normal_01.OnActivate -> Timer_Default_01.StartTimer
Timer_Default_01.OnTimer -> Door_Stone_01.Close
```

### WallInscription

```text
Readable_WallInscription_01.OnUse -> Message_01.ShowText
```

---

## Règles runtime

1. Les objets ne doivent pas se connaître directement.
2. Les objets émettent uniquement des événements.
3. Le système central de liens reçoit l’événement.
4. Le système central exécute les commandes sur les cibles.
5. Les délais sont gérés par le système central.
6. Les liens `bOneShot` sont désactivés après leur première exécution.
7. Les objets cibles ignorent les commandes qu’ils ne supportent pas, mais doivent produire un log de debug utile.

---

## API runtime recommandée

Dans un composant central, probablement `UGridActivationComponent` :

```cpp
void EmitEvent(FName SourceObjectId, EGridObjectEvent Event);

void ExecuteCommand(FName TargetObjectId, EGridObjectCommand Command);
```

Version évoluée possible :

```cpp
void EmitEvent(
    FName SourceObjectId,
    EGridObjectEvent Event,
    const FGridEventPayload& Payload
);

void ExecuteCommand(
    FName TargetObjectId,
    EGridObjectCommand Command,
    const FGridCommandPayload& Payload
);
```

Mais il vaut mieux commencer simple.

---

## Comportement attendu par objet

### Bouton

Émet :

```text
OnActivate
```

Reçoit éventuellement :

```text
Enable
Disable
```

### Levier

Émet :

```text
OnActivate
OnDeactivate
OnToggle
```

Reçoit :

```text
Enable
Disable
Toggle
Activate
Deactivate
```

### Plaque de pression

Émet :

```text
OnActivate
OnDeactivate
OnToggle
```

Reçoit :

```text
Enable
Disable
```

### Réceptacle

Émet :

```text
OnInsertItem
OnRemoveItem
```

Reçoit :

```text
Enable
Disable
```

### Porte

Reçoit :

```text
Open
Close
ToggleOpen
Lock
Unlock
Enable
Disable
```

### Porte secrète

Reçoit :

```text
Open
Close
ToggleOpen
```

### Timer

Émet :

```text
OnTimer
```

Reçoit :

```text
StartTimer
StopTimer
ResetTimer
```

### Téléporteur

Reçoit :

```text
Enable
Disable
Toggle
Teleport
```

Peut utiliser un `OnEnter` interne lorsque le joueur marche dessus.

---

## Règles éditeur

L’inspecteur d’objet doit afficher :

```text
ObjectId
ArchetypeId
Category
ActorClass
InitialState
EmittedEvents
AcceptedCommands
Outgoing Links
Incoming Links
```

Création d’un lien :

```text
Source Object: Button_Normal_01
Source Event: OnActivate
Target Object: Door_Stone_01
Target Command: ToggleOpen
Delay: 0.0
One Shot: false
```

---

## Debug recommandé

Ajouter un affichage runtime ou des logs pour :

```text
nombre d’objets indexés
nombre de liens
événement émis
liens trouvés
commande exécutée
commande ignorée
objet cible introuvable
lien one-shot désactivé
```

Exemple de log :

```text
[GridActivation] Event Button_01.OnActivate
[GridActivation] -> Door_Stone_01.ToggleOpen Delay=0.00 OneShot=false
[GridDoor] Door_Stone_01 ToggleOpen accepted
```

