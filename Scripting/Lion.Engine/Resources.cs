using System.Text;
using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>Loads project data through Lion's resource resolution and centralized Vault unsealing.</summary>
public static class Resources
{
	/// <summary>Reads a resource-relative binary asset, whether sealed or plain. Intended for setup, not frame loops.</summary>
	public static byte[] ReadBytes(string path) => NativeApi.ReadResource(path);
	/// <summary>Reads a UTF-8 data asset, including custom JSON files packaged by Shipping.</summary>
	public static string ReadText(string path) => Encoding.UTF8.GetString(ReadBytes(path));
}
