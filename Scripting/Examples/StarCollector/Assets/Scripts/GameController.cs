using System.Numerics;
using Lion.Engine;

namespace StarCollector;

public sealed class GameController : Behaviour
{
	[Editable] public float RoundSeconds = 20;
	private readonly Entity[] _coins = new Entity[3];
	private Entity _player;
	private TextRenderer _status = null!;
	private InputAction _quit;
	private float _remaining;
	private int _score;
	private int _shownSeconds = -1;
	private int _shownScore = -1;

	public override void OnAwake()
	{
		var scene = Entity.Scene;
		_player = scene.FindEntity("Player") ?? throw new InvalidOperationException("Missing Player.");
		for (int index = 0; index < _coins.Length; index++)
		{
			_coins[index] = scene.FindEntity($"Coin {index + 1}") ?? throw new InvalidOperationException("Missing collectible.");
		}
		_status = Entity.GetComponent<TextRenderer>() ?? throw new InvalidOperationException("Game Controller requires a Text Renderer.");
		_quit = Input.Action("Quit");
		_remaining = Math.Max(RoundSeconds, 1);
	}

	public override void OnUpdate(float deltaTime)
	{
		if (_quit.Strength > 0) { Application.RequestQuit(); return; }
		_remaining -= deltaTime;
		var position = _player.Transform.Position;
		foreach (var coin in _coins)
		{
			if (coin.IsValid && Vector2.DistanceSquared(position, coin.Transform.Position) <= 42 * 42)
			{
				coin.Destroy();
				_score++;
			}
		}
		int seconds = Math.Max((int)MathF.Ceiling(_remaining), 0);
		if (seconds != _shownSeconds || _score != _shownScore)
		{
			_status.Text = $"COLLECTED {_score}/3   TIME {seconds}";
			_shownSeconds = seconds;
			_shownScore = _score;
		}
		if (_score == _coins.Length) { Entity.Scene.Load("Scenes/Win.lnscene"); }
		else if (_remaining <= 0) { Entity.Scene.Load("Scenes/Lose.lnscene"); }
	}
}
