using System.Text.Json;

namespace AsterCircuit;

internal static class Telemetry
{
	internal static void Write<T>(string path, T snapshot)
	{
		try { File.WriteAllText(path, JsonSerializer.Serialize(snapshot)); }
		catch (IOException)
		{
			// An external test reader may briefly hold the file. Optional observation must never fault gameplay.
		}
	}
}
