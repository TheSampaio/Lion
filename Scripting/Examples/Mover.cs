using System.Numerics;
using Lion.Engine;

namespace Lion.Scripting.Examples;

/// <summary>Moves its native owner 100 pixels per second along local X.</summary>
public sealed class Mover : Behaviour
{
	public override void OnUpdate(float deltaTime)
	{
		var state = Transform.State;
		state.Position += new Vector2(100.0f * deltaTime, 0.0f);
		Transform.State = state;
	}
}
