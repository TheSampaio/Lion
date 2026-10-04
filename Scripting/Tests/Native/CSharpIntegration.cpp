#include <Lion/Lion.h>
#include <Lion/Scripting/CSharpRuntime.h>
#include <Lion/Scripting/CSharpScript.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace Lion;

namespace
{
	void Require(bool condition, const std::string& message)
	{
		if (!condition)
			throw std::runtime_error(message);
	}

	Reference<Entity> AddScript(const Reference<Scene>& scene, const std::string& type,
		const std::string& name = "Managed Entity")
	{
		Reference<Entity> entity = MakeReference<Entity>();
		entity->SetName(name);
		entity->AddComponent<CSharpScript>(type);
		scene->Add(entity);
		return entity;
	}

	class NativeMover final : public Component
	{
	public:
		void OnUpdate() override
		{
			const Reference<Transform> transform = GetTransform();
			Vector2 position = transform->GetPosition();
			position.x += GetOwner().GetScene()->GetDeltaTime();
			transform->SetPosition(position);
		}
	};

	double Measure(const Reference<Scene>& scene)
	{
		const auto started = std::chrono::steady_clock::now();
		for (int32 frame = 0; frame < 1000; ++frame)
			scene->OnUpdate(0.001f);

		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
	}
}

int main(int argc, const char* argv[])
{
	try
	{
		Require(argc == 5, "usage: CSharpIntegration <runtime-dir> <hostfxr> <examples-dll> <tests-dll>");
		std::string error;
		Require(!CSharpRuntime::Initialize("missing", argv[2], error), "Missing runtime must fail cleanly.");
		Require(CSharpRuntime::Initialize(argv[1], argv[2], error), error);
		Require(!CSharpRuntime::Initialize(argv[1], argv[2], error), "Duplicate initialization must be rejected.");
		Require(CSharpRuntime::LoadAssembly(argv[3], error), error);
		Require(CSharpRuntime::LoadAssembly(argv[4], error), error);
		Require(!CSharpRuntime::LoadAssembly(argv[4], error), "Duplicate type registration must be rejected.");

		bool wrongThreadRejected = false;
		std::thread worker([&] { std::string reason; wrongThreadRejected = !CSharpRuntime::LoadAssembly(argv[4], reason); });
		worker.join();
		Require(wrongThreadRejected, "Native hosting calls must reject worker threads.");

		Reference<Scene> scene = MakeReference<Scene>();
		Reference<Entity> probe = AddScript(scene, "Lion.Scripting.Tests.LifecycleProbe");
		Require(probe->GetTransform()->GetPosition() == Vector2(1, 2), CSharpRuntime::GetLastError());
		scene->OnUpdate(0.25f);
		Require(probe->GetTransform()->GetPosition() == Vector2(26, 2), "Managed update did not modify native Transform.");
		Require(probe->GetTransform()->GetRotation() == 4 && probe->GetTransform()->GetScale() == Vector2(2),
			"Update pass order or Transform round trip failed.");
		probe->SetEnabled(false);
		scene->OnUpdate(0.25f);
		Require(probe->GetTransform()->GetPosition() == Vector2(26, 2)
			&& probe->GetTransform()->GetRotation() == 10, "Disabled owner received updates.");
		probe->SetEnabled(true);
		Require(probe->GetTransform()->GetRotation() == 20, "Enable callback did not run.");
		probe->GetComponent<CSharpScript>()->SetEnabled(false);
		scene->OnUpdate(0.25f);
		Require(probe->GetTransform()->GetPosition() == Vector2(26, 2), "Disabled script received updates.");
		probe->GetComponent<CSharpScript>()->SetEnabled(true);
		scene->OnUpdate(0.25f, true);
		Require(probe->GetTransform()->GetPosition() == Vector2(26, 2), "Paused scene received gameplay updates.");
		probe->SetVisible(false);
		scene->OnUpdate(0.25f);
		Require(probe->GetTransform()->GetPosition() == Vector2(51, 2), "Hidden entity stopped updating.");

		Reference<Entity> fault = AddScript(scene, "Lion.Scripting.Tests.FaultProbe", "Fault Entity");
		scene->OnUpdate(0.01f);
		const std::string failure = CSharpRuntime::GetLastError();
		Require(failure.find("Intentional script failure") != std::string::npos
			&& failure.find("Fault Entity") != std::string::npos && failure.find("OnUpdate") != std::string::npos
			&& failure.find("FaultProbe") != std::string::npos, "Script failure context was lost.");
		scene->OnUpdate(0.01f);
		Require(fault->GetTransform()->GetPosition() == Vector2(1), "Faulted script kept receiving updates.");
		scene->Remove(fault);
		scene->FlushRemovals();
		Require(fault->GetTransform()->GetRotation() == 99, "Faulted script did not run cleanup.");

		Reference<Entity> victim = AddScript(scene, "Lion.Scripting.Tests.LifetimeVictim");
		scene->Remove(victim);
		scene->FlushRemovals();
		Reference<Entity> observer = AddScript(scene, "Lion.Scripting.Tests.LifetimeObserver");
		Require(observer->GetTransform()->GetRotation() == 77, "Retained native entity kept a stale managed handle valid.");
		scene->Add(victim);
		Reference<Entity> observerAfterReadd = AddScript(scene, "Lion.Scripting.Tests.LifetimeObserver");
		Require(observerAfterReadd->GetTransform()->GetRotation() == 77, "Re-adding the native owner revived an old token.");
		AddScript(scene, "Lion.Scripting.Tests.ConstructorFault");
		Require(CSharpRuntime::GetLastError().find("Intentional constructor failure") != std::string::npos,
			"Constructor exception crossed or disappeared at the native boundary.");
		scene->Clear();
		Require(probe->GetTransform()->GetRotation() == 99, "Scene clear missed managed cleanup.");
		Require(CSharpRuntime::Shutdown(error), error);

		Require(CSharpRuntime::Initialize(argv[1], argv[2], error), error);
		Require(CSharpRuntime::LoadAssembly(argv[3], error), error);
		Require(CSharpRuntime::LoadAssembly(argv[4], error), error);
		Reference<Scene> reloaded = MakeReference<Scene>();
		Reference<Entity> staleAcrossRestart = AddScript(reloaded, "Lion.Scripting.Tests.LifetimeObserver");
		Require(staleAcrossRestart->GetTransform()->GetRotation() == 77, "Runtime restart revived an invalid Entity.");
		Reference<Entity> mover = AddScript(reloaded, "Lion.Scripting.Examples.Mover");
		reloaded->OnUpdate(0.5f);
		Require(mover->GetTransform()->GetPosition() == Vector2(50, 0), "The real example failed after runtime restart.");
		reloaded->Clear();

		Reference<Scene> managedBenchmark = MakeReference<Scene>();
		Reference<Scene> nativeBenchmark = MakeReference<Scene>();
		for (int32 index = 0; index < 256; ++index)
		{
			AddScript(managedBenchmark, "Lion.Scripting.Tests.AllocationProbe");
			Reference<Entity> native = MakeReference<Entity>();
			native->AddComponent<NativeMover>();
			nativeBenchmark->Add(native);
		}
		const double nativeMilliseconds = Measure(nativeBenchmark);
		const double managedMilliseconds = Measure(managedBenchmark);
		Require(CSharpRuntime::GetLastError().empty(), CSharpRuntime::GetLastError());
		std::cout << "256 entities x 1000 scene updates: native=" << nativeMilliseconds
			<< " ms; hosted C#=" << managedMilliseconds << " ms; no managed allocations after warmup.\n";
		managedBenchmark->Clear();
		nativeBenchmark->Clear();
		Require(CSharpRuntime::Shutdown(error), error);
		std::cout << "C# native integration checks passed.\n";
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << '\n';
		return 1;
	}
}
