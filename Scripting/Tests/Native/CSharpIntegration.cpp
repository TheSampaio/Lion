#include <Lion/Lion.h>
#include <Lion/Scripting/CSharpRuntime.h>
#include <Lion/Scripting/CSharpScript.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Vault.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace Lion;

namespace
{
	class AuthoringEdit final : public Reflector
	{
	public:
		void Field(const char8* name, float32& value) override { if (std::string(name) == "_speed") value = 62.5f; }
		void Field(const char8* name, int32& value) override { if (std::string(name) == "_count") value = 7; }
		void Field(const char8*, bool& value) override { value = false; }
		void Field(const char8*, std::string& value) override { value = "Edited café"; }
		void Field(const char8*, Vector& value) override { value = Vector(5, 6, 0); }
		void FieldAsset(const char8*, std::string&) override {}
	};

	class NestedEdit final : public Reflector
	{
	public:
		explicit NestedEdit(Component& target) : mTarget(target) {}
		void Field(const char8*, float32& value) override
		{
			mTarget.Reflect(mEdit);
			value = 91;
		}
		void Field(const char8*, int32&) override {}
		void Field(const char8*, bool&) override {}
		void Field(const char8*, std::string&) override {}
		void Field(const char8*, Vector&) override {}
		void FieldAsset(const char8*, std::string&) override {}
	private:
		Component& mTarget;
		AuthoringEdit mEdit;
	};

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
		Require(argc == 6, "usage: CSharpIntegration <runtime-dir> <hostfxr> <examples-dll> <tests-dll> <invalid-dll>");
		std::string error;
		Require(!CSharpRuntime::Initialize("missing", argv[2], error), "Missing runtime must fail cleanly.");
		Require(CSharpRuntime::Initialize(argv[1], argv[2], error), error);
		Require(!CSharpRuntime::Initialize(argv[1], argv[2], error), "Duplicate initialization must be rejected.");
		Require(ComponentRegistry::RegisterNamed("Lion.Scripting.Examples.Mover", []() -> Scope<Component> { return MakeScope<NativeMover>(); }), "Collision fixture registration failed.");
		Require(!CSharpRuntime::LoadAssembly(argv[3], error), "Managed discovery replaced a native component.");
		ComponentRegistry::UnregisterNamed("Lion.Scripting.Examples.Mover");
		Require(CSharpRuntime::LoadAssembly(argv[3], error), error);
		Require(CSharpRuntime::LoadAssembly(argv[4], error), error);
		Require(!CSharpRuntime::LoadAssembly(argv[5], error) && error.find("Editable field") != std::string::npos,
			"Unsupported editable fields must fail discovery with a diagnostic.");
		Require(!ComponentRegistry::Contains("Lion.Scripting.Invalid.ValidBeforeFailure"), "Failed discovery partially registered types.");
		Require(!CSharpRuntime::LoadAssembly(argv[4], error), "Duplicate type registration must be rejected.");

		bool wrongThreadRejected = false;
		std::thread worker([&] { std::string reason; wrongThreadRejected = !CSharpRuntime::LoadAssembly(argv[4], reason); });
		worker.join();
		Require(wrongThreadRejected, "Native hosting calls must reject worker threads.");

		Reference<Scene> bindingScene = MakeReference<Scene>();
		// Probe comes first: authored Awake must still be able to resolve later entities.
		Require(SceneSerializer::DeserializeFromString(bindingScene, R"({"entities":[
			{"name":"Binding Probe","components":[{"type":"Lion.Scripting.Tests.BindingsProbe"}]},
			{"name":"Caption","components":[{"type":"TextRenderer"},{"type":"SpriteRenderer"}]}]})"),
			"Could not deserialize the authored binding scene.");
		Reference<Entity> bindingProbe = bindingScene->FindEntity("Binding Probe");
		Reference<Entity> caption = bindingScene->FindEntity("Caption café");
		Require(caption != nullptr, "Authored Awake ran before the complete scene was attached.");
		bindingScene->OnUpdate(0.01f);
		bindingScene->OnUpdate(0.01f);
		Require(bindingProbe->GetTransform()->GetPosition().x == 2, CSharpRuntime::GetLastError());
		Require(caption->GetComponent<TextRenderer>()->GetText() == "C# café ✓", "Typed text binding lost UTF-8.");
		caption->RemoveComponent<SpriteRenderer>();
		bindingScene->OnUpdate(0.01f);
		Require(bindingProbe->GetTransform()->GetPosition().x == 3, CSharpRuntime::GetLastError());
		bindingScene->Clear();
		Reference<Entity> bindingObserver = AddScript(bindingScene, "Lion.Scripting.Tests.BindingLifetimeObserver");
		Require(bindingObserver->GetTransform()->GetPosition().x == 78, CSharpRuntime::GetLastError());
		bindingScene->Clear();

		Require(CSharpRuntime::SetGameplayActive(false), "Could not enter authoring mode.");
		Reference<Scene> authored = MakeReference<Scene>();
		Reference<Entity> authoredOwner = MakeReference<Entity>();
		Component* authoredScript = authoredOwner->AddComponentByName("Lion.Scripting.Tests.AuthoringProbe");
		Require(authoredScript != nullptr && authoredScript->GetTypeName() == "Lion.Scripting.Tests.AuthoringProbe", "Script identity was not registered.");
		authoredOwner->AddComponentByName("Lion.Scripting.Examples.Mover");
		authored->Add(authoredOwner);
		authored->OnUpdate(0.5f);
		Require(authoredOwner->GetTransform()->GetPosition() == Vector2(0), "C# gameplay ran during authoring.");
		const std::string defaults = SceneSerializer::SerializeToString(authored);
		Require(defaults.find("_speed") != std::string::npos && defaults.find("12.0") != std::string::npos
			&& defaults.find("Offset.x") != std::string::npos && defaults.find("Offset.z") == std::string::npos,
			"Editable defaults or 2D serialization metadata were lost.");
		AuthoringEdit edit;
		authoredScript->Reflect(edit);
		const std::string snapshot = SceneSerializer::SerializeToString(authored);
		Require(snapshot.find("62.5") != std::string::npos && snapshot.find("Lion.Scripting.Examples.Mover") != std::string::npos,
			"Editing fields or saving two adapter identities failed.");
		Require(SceneSerializer::DeserializeFromString(authored, snapshot), "Authoring round trip failed.");
		Require(CSharpRuntime::SetGameplayActive(true), "Could not enter runtime mode.");
		Require(SceneSerializer::DeserializeFromString(authored, snapshot), "Play reconstruction failed.");
		const Reference<Entity> playedOwner = authored->GetEntities().front();
		Require(playedOwner->GetTransform()->GetPosition().x == 62.5f
			&& playedOwner->GetTransform()->GetRotation() == 7
			&& playedOwner->GetTransform()->GetScale() == Vector2(5, 6), "Authored fields were not applied before Awake.");
		authored->OnUpdate(0.5f);
		Require(playedOwner->GetTransform()->GetPosition().x == 112.5f, "Boolean field or multiple-script updates failed.");
		NestedEdit nested(*playedOwner->GetComponents().front());
		playedOwner->GetComponents().front()->Reflect(nested);
		authored->OnUpdate(0.5f);
		Require(playedOwner->GetTransform()->GetPosition().x == 141, "Reentrant Inspector reflection invalidated field storage or lost an edit.");
		authored->Clear();
		CSharpRuntime::SetGameplayActive(false);
		Require(SceneSerializer::DeserializeFromString(authored, snapshot), "Stop reconstruction failed.");
		Require(authored->GetEntities().front()->GetTransform()->GetPosition() == Vector2(0), "Stop ran managed Awake instead of restoring authoring.");
		authored->Clear();
		std::string incompatible = snapshot;
		const std::string savedField = "\"_speed\": 62.5";
		const auto fieldPosition = incompatible.find(savedField);
		Require(fieldPosition != std::string::npos, "Missing saved field fixture.");
		incompatible.replace(fieldPosition, savedField.size(), "\"_speed\": \"invalid\"");
		Require(SceneSerializer::DeserializeFromString(authored, incompatible), "Incompatible managed fields crashed scene reconstruction.");
		CSharpRuntime::SetGameplayActive(true);
		const std::string recovered = SceneSerializer::SerializeToString(authored);
		Require(SceneSerializer::DeserializeFromString(authored, recovered), "Recovered defaults did not load.");
		Require(authored->GetEntities().front()->GetTransform()->GetPosition().x == 12,
			"Incompatible field data did not restore managed defaults.");
		authored->Clear();
		CSharpRuntime::SetGameplayActive(true);

		Reference<Scene> scene = MakeReference<Scene>();
		const auto renderScene = MakeReference<Scene>();
		const auto renderProbe = AddScript(renderScene, "Lion.Scripting.Tests.RenderProbe");
		renderScene->OnRender();
		Require(renderProbe->GetTransform()->GetPosition().x == 1, "Managed native render callback did not run.");
		renderProbe->SetVisible(false);
		renderScene->OnRender();
		Require(renderProbe->GetTransform()->GetPosition().x == 1, "Hidden owner received managed rendering.");
		renderScene->Clear();
		const auto resourceFixture = std::filesystem::absolute(argv[0]).parent_path() / "Resources";
		std::filesystem::create_directories(resourceFixture);
		const std::string resourceText = "Resource caf\xC3\xA9 \xE2\x9C\x93\n";
		std::ofstream(resourceFixture / "Plain.txt", std::ios::binary) << resourceText;
		std::ofstream(resourceFixture / "Sealed.txt", std::ios::binary) << Vault::Seal(resourceText);
		std::ofstream(resourceFixture / "Empty.txt", std::ios::binary);
		SetResourceOverrideDirectory(resourceFixture.string());
		const auto gameplayScene = MakeReference<Scene>();
		const auto gameplay = AddScript(gameplayScene, "Lion.Scripting.Tests.GameplayProbe");
		Require(gameplay->GetTransform()->GetScale().y == 91, "Gameplay API setup failed: " + CSharpRuntime::GetLastError());
		for (int32 index = 0; index < 1000; index++) gameplayScene->OnUpdate(0.016f);
		Require(gameplay->GetTransform()->GetScale() == Vector2(1000, 92), "Gameplay physics/lifetime contract failed: " + CSharpRuntime::GetLastError());
		gameplayScene->OnUpdate(0.016f, true);
		Require(gameplay->GetTransform()->GetScale().x == 1001, "Paused managed updates did not opt in.");
		gameplayScene->Clear();
		SetResourceOverrideDirectory("");
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
		Require(!ComponentRegistry::Contains("Lion.Scripting.Tests.AuthoringProbe"), "Shutdown retained managed registry factories.");

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
