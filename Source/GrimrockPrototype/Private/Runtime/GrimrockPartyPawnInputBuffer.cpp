#include "Runtime/GrimrockPartyPawn.h"

void AGrimrockPartyPawn::BufferMoveCommand(EGridEdge MoveDirection)
{
	if (!bEnableInputBuffer)
	{
		return;
	}

	BufferedCommandType = EBufferedCommandType::Move;
	BufferedMoveDirection = MoveDirection;
	bBufferedTurnRight = false;
	BufferedCommandAge = 0.f;
}

void AGrimrockPartyPawn::BufferTurnCommand(bool bTurnRight)
{
	if (!bEnableInputBuffer)
	{
		return;
	}

	BufferedCommandType = EBufferedCommandType::Turn;
	BufferedMoveDirection = EGridEdge::None;
	bBufferedTurnRight = bTurnRight;
	BufferedCommandAge = 0.f;
}


void AGrimrockPartyPawn::ClearBufferedCommand()
{
	BufferedCommandType = EBufferedCommandType::None;
	BufferedMoveDirection = EGridEdge::None;
	bBufferedTurnRight = false;
	BufferedCommandAge = 0.f;
}

bool AGrimrockPartyPawn::TryConsumeBufferedCommand()
{
	if (BufferedCommandType == EBufferedCommandType::None)
	{
		return false;
	}

	const EBufferedCommandType CommandType = BufferedCommandType;
	const EGridEdge MoveDirection = BufferedMoveDirection;
	const bool bTurnRight = bBufferedTurnRight;

	ClearBufferedCommand();

	switch (CommandType)
	{
		case EBufferedCommandType::Move:
			return TryStartMove(MoveDirection);

		case EBufferedCommandType::Turn:
			return TryStartTurn(bTurnRight);


		default:
			return false;
	}
}

bool AGrimrockPartyPawn::IsBusy() const
{
	return bIsMoving || bIsTurning;
}

