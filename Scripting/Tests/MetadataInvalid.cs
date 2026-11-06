using Lion.Engine;

namespace Lion.Scripting.Invalid;

public sealed class ValidBeforeFailure : Behaviour;

public sealed class UnsupportedField : Behaviour
{
	[Editable] public DateTime Date;
}
