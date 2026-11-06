using Lion.Engine.Internal;

namespace Lion.Engine;

/// <summary>The native mixer routing buses.</summary>
public enum AudioBus
{
	/// <summary>Final output level.</summary>
	Master,
	/// <summary>Sound effects.</summary>
	SFX,
	/// <summary>Background music.</summary>
	Music
}

/// <summary>A guarded view of the native audio mixer. Voices require explicit Stop or a scene-owned AudioPlayer.</summary>
public sealed class AudioMixer
{
	private readonly Scene _scene;
	internal AudioMixer(Scene scene) => _scene = scene;
	/// <summary>Whether the native output device is available.</summary>
	public bool IsAvailable { get { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_scene.Handle, 16, v); return v[0] != 0; } }
	/// <summary>Plays a resource-relative clip and returns a voice token, or zero on failure.</summary>
	public ulong Play(string path, float volume = 1, float pitch = 1, bool loop = false, AudioBus bus = AudioBus.SFX)
	{
		Span<float> v = stackalloc float[10]; v[0] = volume; v[1] = pitch; v[2] = loop ? 1 : 0; v[3] = (int)bus;
		return NativeApi.SceneCommand(_scene.Handle, 11, v, path: path);
	}
	/// <summary>Stops a native voice.</summary>
	public void Stop(ulong voice) { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_scene.Handle, 12, v, voice); }
	/// <summary>Tests whether the native voice is still playing.</summary>
	public bool IsPlaying(ulong voice) { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_scene.Handle, 13, v, voice); return v[0] != 0; }
	/// <summary>Changes a playing voice's level.</summary>
	public void SetVolume(ulong voice, float volume) { Span<float> v = stackalloc float[10]; v[0] = volume; NativeApi.SceneCommand(_scene.Handle, 14, v, voice); }
	/// <summary>Changes a playing voice's pitch multiplier.</summary>
	public void SetPitch(ulong voice, float pitch) { Span<float> v = stackalloc float[10]; v[0] = pitch; NativeApi.SceneCommand(_scene.Handle, 15, v, voice); }
	/// <summary>Sets a mixer bus level.</summary>
	public void SetBusVolume(AudioBus bus, float volume) { Span<float> v = stackalloc float[10]; v[0] = (int)bus; v[1] = volume; NativeApi.SceneCommand(_scene.Handle, 10, v); }
	/// <summary>Reads a mixer bus level.</summary>
	public float GetBusVolume(AudioBus bus) { Span<float> v = stackalloc float[10]; v[0] = (int)bus; NativeApi.SceneCommand(_scene.Handle, 18, v); return v[0]; }
	/// <summary>Stops all native voices.</summary>
	public void StopAll() { Span<float> v = stackalloc float[10]; NativeApi.SceneCommand(_scene.Handle, 17, v); }
}
