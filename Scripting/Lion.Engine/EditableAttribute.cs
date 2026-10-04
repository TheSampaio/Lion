namespace Lion.Engine;

/// <summary>Exposes a mutable instance field to Mane and native scene serialization.</summary>
/// <remarks>Supported field types are float, int, bool, string and System.Numerics.Vector2.
/// The exact field name is its persistent key: renaming it changes the saved contract.
/// Private and inherited fields are supported; properties, readonly/static fields and other types
/// are not. Constructors may initialize defaults but must not access an Entity or perform gameplay:
/// the editor constructs a temporary default instance without dispatching lifecycle callbacks.</remarks>
[AttributeUsage(AttributeTargets.Field, Inherited = true)]
public sealed class EditableAttribute : Attribute;
