using Lion.Engine;

namespace StarCollector;

public sealed class MenuController : Behaviour
{
	private InputAction _confirm;
	private InputAction _quit;
	private bool _wasConfirm = true;

	public override void OnAwake()
	{
		_confirm = Input.Action("Confirm");
		_quit = Input.Action("Quit");
	}

	public override void OnUpdate(float deltaTime)
	{
		if (_quit.Strength > 0) { Application.RequestQuit(); return; }
		bool confirm = _confirm.Strength > 0;
		if (confirm && !_wasConfirm) { Entity.Scene.Load("Scenes/Game.lnscene"); }
		_wasConfirm = confirm;
	}
}
