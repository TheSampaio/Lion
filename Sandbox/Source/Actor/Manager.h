#pragma once

#include <Lion/Lion.h>

class Ball;
class Brick;

// Owns the Brickout game flow: spawns the background and bricks, tracks the win/lose state,
// gives visual feedback and restarts the round on demand.
class Manager : public Lion::Entity
{
public:
	explicit Manager(Lion::Reference<Ball> ball);

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
	Lion::Reference<Ball> mBall;
	std::vector<Lion::Reference<Brick>> mBricks;
	Lion::float32 mLoseThresholdY = 0.0f;

	void SpawnBricks();
	void RemoveDestroyedBricks();
	void CheckWinLose();
	void EndGame(State state);
	void Restart();
};
