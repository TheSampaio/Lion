// Original scores, reproducible without samples, soundfonts, network services or copyrighted melodies.
using System.Text;

if (args.Length != 1) throw new ArgumentException("Usage: ScoreComposer <sound-output-directory>");
string destination = Path.GetFullPath(args[0]);
Directory.CreateDirectory(destination);
int[] forge = [64, 67, 71, 76, 74, 71, 67, 62, 64, 71, 69, 67, 66, 62, 59, 62, 67, 71, 74, 79, 76, 74, 71, 69, 67, 66, 64, 62, 59, 62, 66, 71];
int[] frost = [73, 80, 78, 76, 73, 71, 68, 71, 76, 78, 80, 83, 80, 78, 76, 71, 73, 76, 78, 80, 85, 83, 80, 78, 76, 73, 71, 68, 66, 68, 71, 73];
int[] storm = [69, 72, 76, 79, 81, 79, 76, 72, 74, 77, 81, 84, 83, 81, 77, 74, 76, 79, 83, 86, 84, 83, 79, 76, 77, 76, 74, 72, 71, 74, 76, 79];
Score("Forge", 144, forge, [40, 43, 38, 47], 16, 0.25);
Score("Frost", 112, frost, [42, 45, 40, 47], 16, 0.5);
Score("Storm", 168, storm, [45, 48, 50, 52], 16, 0.125);
Score("Menu", 104, [72, 76, 79, 83, 81, 79, 76, 74, 72, 74, 76, 79, 84, 83, 79, 76], [48, 45, 41, 43], 8, 0.5);
Score("Boss", 176, [52, 64, 55, 67, 59, 71, 58, 70, 53, 65, 57, 69, 60, 72, 59, 71], [40, 41, 45, 47], 8, 0.125);
Effect("Shot", 0.11, 1100, 380, false);
Effect("Hit", 0.18, 180, 45, true);
Effect("Jump", 0.16, 180, 670, false);
Effect("Pickup", 0.36, 520, 1320, false);
Effect("Clear", 0.9, 440, 880, false);
Console.WriteLine($"Composed five original scores and five effects in {destination}");

void Score(string name, double bpm, int[] melody, int[] roots, int bars, double duty)
{
	const int rate = 22050;
	double beat = 60 / bpm, duration = bars * 4 * beat;
	short[] samples = new short[(int)Math.Round(duration * rate)];
	uint noise = 0xCA71A5u;
	int[] chord = [0, 7, 12, 7];
	for (int i = 0; i < samples.Length; i++)
	{
		double time = (double)i / rate;
		int step = (int)(time / (beat * 0.5));
		double noteTime = time % (beat * 0.5), gate = Math.Min(noteTime * 200, 1) * Math.Clamp((beat * 0.46 - noteTime) * 80, 0, 1);
		int bar = (int)(time / (beat * 4)), note = melody[step % melody.Length] + (bar >= bars / 2 ? 12 : 0);
		double lead = Pulse(time * Hertz(note), duty) * gate * 0.12;
		int bassNote = roots[bar % roots.Length] + ((step & 3) == 2 ? 12 : 0);
		double bass = (4 * Math.Abs((time * Hertz(bassNote) % 1) - 0.5) - 1) * 0.13;
		int harmony = roots[bar % roots.Length] + 24 + chord[step & 3];
		double arp = Pulse(time * Hertz(harmony), 0.5) * gate * 0.045;
		noise ^= noise << 13; noise ^= noise >> 17; noise ^= noise << 5;
		double drumTime = time % beat, eighth = time % (beat * 0.5);
		double kick = Math.Sin(2 * Math.PI * (85 * drumTime - 60 * drumTime * drumTime)) * Math.Exp(-drumTime * 35) * 0.15;
		double snare = ((noise & 1) == 0 ? -1 : 1) * Math.Exp(-eighth * 95) * (((int)(time / beat) & 1) != 0 ? 0.065 : 0.025);
		// Endpoint ramp prevents a click; melodies/tempo are built from full-bar loops.
		double envelope = Math.Min(time * 120, 1) * Math.Min((duration - time) * 120, 1);
		samples[i] = (short)(Math.Clamp((lead + bass + arp + kick + snare) * envelope, -0.85, 0.85) * short.MaxValue);
	}
	Wave(name, samples, rate);
}

void Effect(string name, double duration, double start, double end, bool noisy)
{
	const int rate = 22050;
	short[] samples = new short[(int)(duration * rate)];
	uint noise = 0xC1AC017u;
	double phase = 0;
	for (int i = 0; i < samples.Length; i++)
	{
		double t = (double)i / rate, blend = t / duration;
		phase += (start + (end - start) * blend) / rate;
		noise ^= noise << 13; noise ^= noise >> 17; noise ^= noise << 5;
		double signal = noisy ? ((noise & 1) == 0 ? -1 : 1) : Pulse(phase, 0.25);
		samples[i] = (short)(signal * Math.Sin(Math.PI * blend) * 0.23 * short.MaxValue);
	}
	Wave(name, samples, rate);
}

void Wave(string name, short[] samples, int rate)
{
	using var writer = new BinaryWriter(File.Create(Path.Combine(destination, name + ".wav")), Encoding.ASCII);
	writer.Write("RIFF"u8); writer.Write(36 + samples.Length * 2); writer.Write("WAVEfmt "u8);
	writer.Write(16); writer.Write((short)1); writer.Write((short)1); writer.Write(rate); writer.Write(rate * 2);
	writer.Write((short)2); writer.Write((short)16); writer.Write("data"u8); writer.Write(samples.Length * 2);
	foreach (short sample in samples) writer.Write(sample);
}

static double Hertz(int midi) => 440 * Math.Pow(2, (midi - 69) / 12.0);
static double Pulse(double phase, double duty) => phase % 1 < duty ? 1 : -1;
