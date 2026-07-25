#include "Runtime/Monsters/GridMonsterActor.h"

void AGridMonsterActor::MarkDead ()
{
    CurrentHealth = 0;
    MonsterState = EGridMonsterState::Dead;
    ResetAnimationSignals ();

    if (CombatComponent)
    {
        CombatComponent->CancelAttackPresentation ();
    }

    if (DeathComponent)
    {
        DeathComponent->CommitDeath ();
    }
    else if (CollisionComponent)
    {
        CollisionComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }
}
