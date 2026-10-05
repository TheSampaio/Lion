namespace Lion.Engine.Internal;

internal static class ResourcePath
{
	internal static void Validate(string path)
	{
		NativeApi.CheckString(path);
		if (string.IsNullOrWhiteSpace(path) || Path.IsPathRooted(path) || path.Contains('\\') || path.Contains(':')
			|| path.Split('/').Any(part => part is ".." or "." or ""))
		{
			throw new ArgumentException("Use an Assets-relative path with forward slashes and no traversal.", nameof(path));
		}
	}
}
