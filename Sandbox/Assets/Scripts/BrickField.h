#pragma once

#include <Lion/Lion.h>

class Ball;

// Runs the round: lays out the bricks, decides when it is won or lost, and starts it over.
//
// It owns none of the entities it makes — the scene does. It finds the ball the same way anything
// here finds anything: by asking the scene for the trait, never for the object.
class BrickField : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;

private:
	enum class State
	{
		Playing,
		Won,
		Lost,
	};

	State mState = State::Playing;
	Ball* mBall = nullptr;
	Lion::float32 mLoseThresholdY = 0.0f;

	void SpawnBricks();
	void CheckWinLose();
	void EndRound(State state);
	void Restart();
};
