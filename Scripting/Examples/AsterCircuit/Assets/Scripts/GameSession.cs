using Lion.Engine;

namespace AsterCircuit;

internal static class GameSession
{
	internal static int Cleared, Weapon, Lives = 3, CurrentStage;
	internal static float Checkpoint = 192;
	private static bool _loaded;
	private static string SavePath => Path.Combine(Environment.GetEnvironmentVariable("ASTER_SAVE_DIRECTORY")
		?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "LionGames", "AsterCircuit"), "Progress.lnsave");
	internal static void Load()
	{
		if (_loaded) return;
		_loaded = true;
		try
		{
			if (File.Exists(SavePath))
			{
				string[] fields = File.ReadAllText(SavePath).Split(':');
				if (fields.Length == 3 && fields[0] == "ASTER1" && int.TryParse(fields[1], out int mask)
					&& mask is >= 0 and <= 7 && fields[2] == (mask ^ 0xA571).ToString()) Cleared = mask;
			}
		}
		catch (IOException exception) { Log.Warning("Could not read Aster progress: " + exception.Message); }
		catch (UnauthorizedAccessException exception) { Log.Warning("Could not read Aster progress: " + exception.Message); }
	}
	internal static void Save()
	{
		try
		{
			Directory.CreateDirectory(Path.GetDirectoryName(SavePath)!);
			File.WriteAllText(SavePath + ".tmp", $"ASTER1:{Cleared}:{Cleared ^ 0xA571}");
			File.Move(SavePath + ".tmp", SavePath, true);
		}
		catch (IOException exception) { Log.Warning("Could not save Aster progress: " + exception.Message); }
		catch (UnauthorizedAccessException exception) { Log.Warning("Could not save Aster progress: " + exception.Message); }
	}
	internal static void Start(int stage)
	{
		CurrentStage = stage; Lives = 3; Checkpoint = 192;
	}
	internal static string StagePath(int stage) => "Scenes/" + Art.Tracks[stage] + ".lnscene";
	internal static void CycleWeapon()
	{
		for (int index = 0; index < 4; index++)
		{
			Weapon = (Weapon + 1) % 4;
			if (Weapon == 0 || (Cleared & (1 << (Weapon - 1))) != 0) return;
		}
	}
}
