using System.Numerics;
using Lion.Engine;

namespace AsterCircuit;

public sealed class FrontEnd : Behaviour
{
	[Editable] private int _screen = 0;
	private readonly List<Button> _buttons = [];
	private InputAction _confirm, _back, _next, _previous;
	private int _selection;
	private float _time;
	private SpriteRenderer? _hero;
	private AudioPlayer? _music;
	private AudioMixer? _audio;
	private float _reportTime;
	private readonly string? _report = Environment.GetEnvironmentVariable("ASTER_TEST_REPORT");

	public override void OnAwake()
	{
		GameSession.Load();
		if (!Application.IsEditor)
		{
			Window.Title = "Aster Circuit";
			Window.BackgroundColor = new(0.025f, 0.04f, 0.09f);
			Window.ShowFrameStats = false;
			Window.SetResizable(true);
		}
		_confirm = Input.Action("Confirm"); _back = Input.Action("Back"); _next = Input.Action("Next"); _previous = Input.Action("Previous");
		var scene = Entity.Scene;
		_audio = scene.Audio;
		_music = Art.Music(scene, "Menu");
		Art.Rectangle(scene, "Panel", Vector2.Zero, new(1280, 720), new(0.025f, 0.04f, 0.09f), -100);
		for (int i = 0; i < 27; i++)
		{
			var tile = Art.Sprite(scene, "Signal", new(-624 + i * 48, -322), 5, 5, new(48), -10);
			tile.SetColor(new(0.2f, 0.4f, 0.5f));
		}
		Art.Text(scene, "Brand", "ASTER CIRCUIT", new(0, 265), 52, Art.Amber);
		Art.Text(scene, "Subtitle", "THREE SIGNALS. ONE COURIER.", new(0, 212), 18, Art.Mint);
		if (_screen == 1) BuildSelect(scene);
		else if (_screen == 2) BuildEnding(scene);
		else if (_screen == 3) BuildGameOver(scene);
		else BuildMenu(scene);
		RefreshSelection();
	}
	private void BuildMenu(Scene scene)
	{
		_hero = Art.Sprite(scene, "Courier", new(0, 78), 0, 0, new(150), 10);
		AddButton(scene, "START / CONTINUE", new(0, -60), 400);
		AddButton(scene, "NEW CAMPAIGN", new(0, -122), 400);
		AddButton(scene, "QUIT", new(0, -184), 400);
		Art.Text(scene, "Controls", "ARROWS / WASD: MOVE   Z / SPACE: JUMP   X / J: FIRE\nC: WEAPON   P: PAUSE   ENTER: CONFIRM   ESC: BACK", new(0, -256), 14, Art.Ice);
	}
	private void BuildSelect(Scene scene)
	{
		for (int stage = 0; stage < 3; stage++)
		{
			float x = (stage - 1) * 400;
			Art.Rectangle(scene, "Card", new(x, 20), new(356, 294), new(0.06f, 0.08f, 0.14f));
			Art.Sprite(scene, "Boss Portrait", new(x, 76), stage + 2, 0, new(160), 90);
			Art.Text(scene, "Boss Name", Art.BossNames[stage], new(x, -26), 19, Art.Colors[stage]);
			AddButton(scene, Art.StageNames[stage], new(x, -91), 348);
			Art.Text(scene, "Clear Status", (GameSession.Cleared & (1 << stage)) != 0 ? "SIGNAL RESTORED" : "SIGNAL LOST", new(x, -153), 15, Art.Colors[stage]);
		}
		Art.Text(scene, "Selection Help", "LEFT / RIGHT TO SELECT   ENTER TO DEPLOY\nESC TO RETURN    DEFEAT A WARDEN TO UNLOCK ITS WEAPON", new(0, -241), 16, Art.Ice);
	}
	private void BuildEnding(Scene scene)
	{
		Art.Sprite(scene, "Courier", new(0, 75), 0, 7, new(168), 90);
		Art.Text(scene, "Ending", "THE ARRAY IS ALIVE AGAIN.\nALL THREE SIGNALS RESTORED.", new(0, -53), 24, Art.Mint);
		AddButton(scene, "RETURN TO TITLE", new(0, -165), 440);
		Art.Text(scene, "Credits", "ORIGINAL ART + MUSIC   GAMEPLAY IN C SHARP   POWERED BY LION", new(0, -253), 14, Art.Ice);
	}
	private void BuildGameOver(Scene scene)
	{
		Art.Text(scene, "Game Over", "COURIER OFFLINE", new(0, 70), 32, Art.Amber);
		Art.Text(scene, "Retry Help", "YOUR RESTORED SIGNALS ARE SAVED.", new(0, 0), 20, Art.Ice);
		AddButton(scene, "RETRY STAGE", new(0, -97), 400);
		AddButton(scene, "STAGE SELECT", new(0, -163), 400);
	}
	private void AddButton(Scene scene, string label, Vector2 position, float width)
	{
		var text = Art.Text(scene, "Action " + label, label, position, 22);
		var button = text.Entity.AddComponent<Button>(value => value.SetField("Size", new Vector3(width, 48, 0)));
		_buttons.Add(button);
	}
	private void RefreshSelection()
	{
		for (int i = 0; i < _buttons.Count; i++)
		{
			_buttons[i].IsSelected = i == _selection;
			_buttons[i].Entity.GetComponent<TextRenderer>()!.SetField("Color", i == _selection ? Art.Amber : Art.Ice);
		}
	}
	public override void OnUpdate(float deltaTime)
	{
		_time += deltaTime;
		if (_hero != null) Art.Frame(_hero, 0, (int)(_time * 5) % 3, new(150));
		int movement = (_next.WasPressed ? 1 : 0) - (_previous.WasPressed ? 1 : 0);
		if (movement != 0) { _selection = (_selection + movement + _buttons.Count) % _buttons.Count; RefreshSelection(); }
		for (int i = 0; i < _buttons.Count; i++) if (_buttons[i].WasClicked) { _selection = i; Activate(); return; }
		if (_confirm.WasPressed) { Activate(); return; }
		if (_back.WasPressed) { if (_screen == 0) Application.RequestQuit(); else Entity.Scene.Load("Scenes/Menu.lnscene"); }
		_reportTime += deltaTime;
		if (_report != null && _reportTime > 0.1f)
		{
			_reportTime = 0;
			Telemetry.Write(_report, new { Screen = _screen == 1 ? "Select" : _screen == 2 ? "Ending" : _screen == 3 ? "GameOver" : "Menu", Selection = _selection, GameSession.Cleared });
		}
	}
	private void Activate()
	{
		_audio?.Play("Sounds/Pickup.wav", 0.4f);
		var scene = Entity.Scene;
		if (_screen == 1) { GameSession.Start(_selection); scene.Load(GameSession.StagePath(_selection)); }
		else if (_screen == 2) scene.Load("Scenes/Menu.lnscene");
		else if (_screen == 3)
		{
			if (_selection == 0) { GameSession.Start(GameSession.CurrentStage); scene.Load(GameSession.StagePath(GameSession.CurrentStage)); }
			else scene.Load("Scenes/Select.lnscene");
		}
		else if (_selection == 2) Application.RequestQuit();
		else
		{
			if (_selection == 1) { GameSession.Cleared = 0; GameSession.Weapon = 0; GameSession.Save(); }
			scene.Load("Scenes/Select.lnscene");
		}
	}
	public override void OnDestroy() { _music?.Stop(); }
}
