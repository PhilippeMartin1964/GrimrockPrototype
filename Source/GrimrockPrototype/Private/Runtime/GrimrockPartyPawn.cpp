// Fill out your copyright notice in the Description page of Project Settings.


#include "Runtime/GrimrockPartyPawn.h"

// Sets default values
AGrimrockPartyPawn::AGrimrockPartyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGrimrockPartyPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGrimrockPartyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AGrimrockPartyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

