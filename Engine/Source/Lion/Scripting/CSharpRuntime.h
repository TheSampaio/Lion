#pragma once

namespace Lion
{
	class CSharpScript;
	class Entity;

	// Optional process-wide managed gameplay host. All operations require the engine thread.
	// Native games do not initialize this host and have no .NET runtime dependency.
	class CSharpRuntime final
	{
	public:
		// Initializes Lion.Engine.dll from an absolute directory using an explicit hostfxr library.
		// Failure leaves native gameplay operational and reports the reason through error.
		static LION_API bool Initialize(const std::string& managedDirectory,
			const std::string& hostfxrPath, std::string& error);

		// Discovers public, concrete Behaviour types with parameterless constructors in an assembly.
		static LION_API bool LoadAssembly(const std::string& assemblyPath, std::string& error);

		// Releases managed instances and native lifetime tokens; the process .NET runtime stays loaded.
		static LION_API bool Shutdown(std::string& error);
		static LION_API bool IsInitialized();

		// Most recent script/host failure, retained even when the game's logging is disabled.
		static LION_API const std::string& GetLastError();

	private:
		friend CSharpScript;
		friend Entity;

		static uint64 CreateInstance(Entity& entity, const std::string& typeName, uint32& callbacks);
		static void Invoke(uint64 instance, int32 callback, float32 deltaTime = 0.0f);
		static void DestroyInstance(uint64 instance);
		static void InvalidateEntity(Entity& entity);
	};
}
