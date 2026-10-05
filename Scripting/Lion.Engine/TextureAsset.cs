using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>A typed texture resource identity, independent of machine and runtime addresses.</summary>
/// <remarks>This does not load or own GPU memory. Assignment to SpriteRenderer uses Lion's native
/// shared asset cache. Missing resources follow the native loader's diagnostic policy.</remarks>
public readonly struct TextureAsset
{
	private readonly string? _path;
	private TextureAsset(string path) => _path = path;

	/// <summary>Creates an identity from an Assets-relative path using forward slashes.</summary>
	/// <param name="path">For example Sprites/Player.png; absolute paths and traversal are rejected.</param>
	/// <returns>A texture identity without loading the resource.</returns>
	public static TextureAsset FromPath(string path) { ResourcePath.Validate(path); return new TextureAsset(path); }

	/// <summary>The resource-relative persistent identity; empty for the default value.</summary>
	public string Path => _path ?? "";
}
