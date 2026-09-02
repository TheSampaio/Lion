#include "Engine.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <nlohmann/json.hpp>

#include <Lion/Core/Log.h>
#include <Lion/Logic/AssemblySerializer.h>
#include <Lion/Logic/Component.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Core/Vault.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Serializer.h>
#include <Lion/Math/Transform.h>
#include <Lion/Physics/BoxCollider2D.h>
#include <Lion/Physics/CircleCollider2D.h>
#include <Lion/Physics/RigidBody2D.h>
#include <Lion/Render/SpriteRenderer.h>

using Json = nlohmann::json;

namespace Lion
{
	// Concrete Serializer backed by a JSON object, bridging the engine's JSON to the abstract archive
	// that components (built-in and user-defined) see. Constructed over a mutable node for serialize,
	// or a const one for deserialize; only the matching direction is exercised in each case.
	class JsonSerializer : public Serializer
	{
	public:
		explicit JsonSerializer(Json& node) : mWrite(&node), mRead(&node) {}
		explicit JsonSerializer(const Json& node) : mRead(&node) {}

		void Write(const std::string& key, float32 value) override { (*mWrite)[key] = value; }
		void Write(const std::string& key, int32 value) override { (*mWrite)[key] = value; }
		void Write(const std::string& key, bool value) override { (*mWrite)[key] = value; }
		void Write(const std::string& key, const std::string& value) override { (*mWrite)[key] = value; }

		float32 ReadFloat(const std::string& key, float32 fallback) const override { return mRead->value(key, fallback); }
		int32 ReadInt(const std::string& key, int32 fallback) const override { return mRead->value(key, fallback); }
		bool ReadBool(const std::string& key, bool fallback) const override { return mRead->value(key, fallback); }
		std::string ReadString(const std::string& key, const std::string& fallback) const override { return mRead->value(key, fallback); }

	private:
		Json* mWrite = nullptr;
		const Json* mRead = nullptr;
	};

	static BodyType BodyTypeFromString(const std::string& value)
	{
		if (value == "Kinematic") return BodyType::Kinematic;
		if (value == "Dynamic")   return BodyType::Dynamic;
		return BodyType::Static;
	}

	// A 2D vector from a saved array, tolerant of an old three-component one: the third was a Z that a
	// 2D transform never used, so a scene written before this still reads.
	static Vector2 Vector2FromJson(const Json& array)
	{
		return Vector2(array.at(0).get<float32>(), array.at(1).get<float32>());
	}

	// The rotation as a single angle, whether it was saved as one or as the old vector whose Z carried it.
	static float32 RotationFromJson(const Json& value)
	{
		if (value.is_array())
			return value.size() >= 3 ? value.at(2).get<float32>() : 0.0f;

		return value.get<float32>();
	}

	static void WriteTransform(Json& node, const Reference<Entity>& entity)
	{
		const Reference<Transform> transform = entity->GetTransform();
		const Vector2 position = transform->GetPosition();
		const Vector2 scale = transform->GetScale();

		node["transform"]["position"] = { position.x, position.y };
		node["transform"]["rotation"] = transform->GetRotation();
		node["transform"]["scale"]    = { scale.x, scale.y };
	}

	static void WriteAssemblyPlacement(Json& node, const Reference<Entity>& entity)
	{
		const Reference<Transform> transform = entity->GetTransform();
		const Transform& source = entity->GetAssemblySourceTransform();
		const Vector2 position = transform->GetPosition() - source.GetPosition();
		const Vector2 scale = transform->GetScale();
		const Vector2 sourceScale = source.GetScale();
		constexpr float32 kMinimumScale = 0.0001f;

		node["placement"]["position"] = { position.x, position.y };
		node["placement"]["rotation"] = transform->GetRotation() - source.GetRotation();
		node["placement"]["scale"] = {
			std::fabs(sourceScale.x) > kMinimumScale ? scale.x / sourceScale.x : 1.0f,
			std::fabs(sourceScale.y) > kMinimumScale ? scale.y / sourceScale.y : 1.0f
		};
	}

	static void ApplyAssemblyPlacement(const Reference<Entity>& entity, const Json& placement)
	{
		const Transform& source = entity->GetAssemblySourceTransform();
		const Reference<Transform> target = entity->GetTransform();

		if (placement.contains("position"))
			target->SetPosition(source.GetPosition() + Vector2FromJson(placement["position"]));

		if (placement.contains("rotation"))
			target->SetRotation(source.GetRotation() + RotationFromJson(placement["rotation"]));

		if (placement.contains("scale"))
			target->SetScale(source.GetScale() * Vector2FromJson(placement["scale"]));
	}

	// Serializes one entity (name, transform and its ordered components) into a JSON node. Scene entries
	// for Assembly instances stay compact: the definition lives in the asset while the scene owns placement
	// and the instance-level visibility controlled by the Hierarchy eye.
	static Json EntityToJson(const Reference<Entity>& entity, bool linkedReference)
	{
		Json node;

		if (linkedReference && entity->IsAssemblyInstance())
		{
			node["assembly"] = entity->GetAssemblyPath();
			node["visible"] = entity->IsVisible();
			WriteAssemblyPlacement(node, entity);
			return node;
		}

		node["name"] = entity->GetName();

		if (entity->IsFolder())
			node["folder"] = true;

		// Only what departs from the default is written: an entity is enabled and visible unless it says
		// otherwise, and a scene file that repeats the obvious is a scene file nobody can read.
		if (!entity->IsEnabled())
			node["enabled"] = false;

		if (!entity->IsVisible())
			node["visible"] = false;

		WriteTransform(node, entity);

		// Components are written as an ordered array so the editor's display/drag order round-trips.
		// Each one names its registered type and serializes its own fields, so user-defined components
		// persist exactly like the built-in ones. Unregistered types carry no name and are skipped.
		Json components = Json::array();

		for (const auto& component : entity->GetComponents())
		{
			const std::string& type = component->GetTypeName();

			if (type.empty())
				continue;

			Json entry;
			entry["type"] = type;

			JsonSerializer serializer(entry);
			component->Serialize(serializer);

			components.push_back(entry);
		}

		node["components"] = components;
		return node;
	}

	// Rebuilds one regular entity or expands one linked Assembly root into its complete hierarchy.
	static std::vector<Reference<Entity>> EntityTreeFromJson(const Json& node, const std::string& resourceRoot);

	static bool IsAssemblyDescendant(const Entity* entity)
	{
		for (const Entity* parent = entity ? entity->GetParent() : nullptr; parent; parent = parent->GetParent())
			if (parent->IsAssemblyInstance())
				return true;

		return false;
	}

	std::string SceneSerializer::SerializeToString(const Reference<Scene>& scene)
	{
		Json root;

		const glm::vec2 gravity = scene->GetGravity();
		root["gravity"] = { gravity.x, gravity.y };
		root["entities"] = Json::array();

		// Parents are stored as an index into this array, so the hierarchy round-trips.
		std::unordered_map<const Entity*, int32> indices;
		int32 index = 0;

		for (const auto& entity : scene->GetEntities())
			if (!IsAssemblyDescendant(entity.get()))
				indices.emplace(entity.get(), index++);

		for (const auto& entity : scene->GetEntities())
		{
			// A linked root represents the complete Assembly. Its authored descendants live in the asset and
			// must not be duplicated into the containing scene as accidental overrides.
			if (IsAssemblyDescendant(entity.get()))
				continue;

			Json node = EntityToJson(entity, true);

			const auto parent = indices.find(entity->GetParent());
			node["parent"] = (parent != indices.end()) ? parent->second : -1;

			root["entities"].push_back(node);
		}

		return root.dump(2);
	}

	std::string SceneSerializer::SerializeEntityToString(const Reference<Entity>& entity)
	{
		return entity ? EntityToJson(entity, true).dump(2) : std::string();
	}

	std::string SceneSerializer::SerializeEntityDefinitionToString(const Reference<Entity>& entity)
	{
		return entity ? EntityToJson(entity, false).dump(2) : std::string();
	}

	std::string SceneSerializer::SerializeEntityTreeDefinitionToString(const Reference<Entity>& rootEntity)
	{
		if (!rootEntity)
			return {};

		std::vector<Reference<Entity>> entities;
		const Reference<Scene> scene = rootEntity->GetScene();

		const auto collect = [&](const auto& self, const Reference<Entity>& entity) -> void
		{
			entities.push_back(entity);

			if (!scene)
				return;

			for (Entity* child : entity->GetChildren())
			{
				const auto found = std::find_if(scene->GetEntities().begin(), scene->GetEntities().end(),
					[child](const Reference<Entity>& candidate) { return candidate.get() == child; });

				if (found != scene->GetEntities().end())
					self(self, *found);
			}
		};

		collect(collect, rootEntity);

		Json definition;
		definition["root"] = 0;
		definition["entities"] = Json::array();

		std::unordered_map<const Entity*, int32> indices;
		for (int32 index = 0; index < static_cast<int32>(entities.size()); ++index)
			indices.emplace(entities[index].get(), index);

		for (const auto& entity : entities)
		{
			Json node = EntityToJson(entity, false);
			const auto parent = indices.find(entity->GetParent());
			node["parent"] = parent != indices.end() ? parent->second : -1;
			definition["entities"].push_back(std::move(node));
		}

		return definition.dump(2);
	}

	bool SceneSerializer::Serialize(const Reference<Scene>& scene, const std::string& filePath)
	{
		std::ofstream file(filePath, std::ios::binary);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneSerializer] Failed to write scene: '{}'.", filePath));
			return false;
		}

		// A scene leaves the editor sealed — XOR against the key, base64, broken into lines — and it is JSON
		// again by the time anything reads it: Vault::Unseal takes plaintext back unchanged, so a scene
		// written by hand still opens, and nothing has to remember which kind it is holding.
		file << Vault::Seal(SerializeToString(scene));
		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[SceneSerializer] Saved scene: '{}'.", filePath));
		return true;
	}

	bool SceneSerializer::DeserializeFromString(const Reference<Scene>& scene, const std::string& text)
	{
		return DeserializeFromString(scene, text, {});
	}

	bool SceneSerializer::DeserializeFromString(const Reference<Scene>& scene, const std::string& text,
		const std::string& resourceRoot)
	{
		Json root;

		try
		{
			root = Json::parse(text);
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneSerializer] Invalid scene JSON: {}", exception.what()));
			return false;
		}

		scene->Clear();

		if (root.contains("gravity") && root["gravity"].is_array() && root["gravity"].size() == 2)
			scene->SetGravity(glm::vec2(root["gravity"][0].get<float32>(), root["gravity"][1].get<float32>()));

		if (!root.contains("entities"))
			return true;

		// Build every entity first, then link parents, and only then add them to the scene: Awake
		// creates physics bodies from the world transform, which needs the hierarchy in place.
		std::vector<Reference<Entity>> roots;
		std::vector<Reference<Entity>> entities;
		roots.reserve(root["entities"].size());

		for (const auto& node : root["entities"])
		{
			std::vector<Reference<Entity>> tree = EntityTreeFromJson(node, resourceRoot);

			if (tree.empty())
				tree.push_back(MakeReference<Entity>());

			roots.push_back(tree.front());
			entities.insert(entities.end(), tree.begin(), tree.end());
		}

		size_t index = 0;
		for (const auto& node : root["entities"])
		{
			const int32 parent = node.value("parent", -1);

			if (parent >= 0 && parent < static_cast<int32>(roots.size()) && parent != static_cast<int32>(index))
				roots[index]->SetParent(roots[parent].get(), false);  // Transforms are already local.

			index++;
		}

		for (const auto& entity : entities)
			scene->Add(entity);

		return true;
	}

	static void PopulateEntityFromJson(const Reference<Entity>& entity, const Json& node, bool preserveTransform)
	{
		while (!entity->GetComponents().empty())
			entity->RemoveComponent(entity->GetComponents().back().get());

		entity->SetName(node.value("name", std::string("Entity")));
		entity->SetFolder(node.value("folder", false));
		entity->SetEnabled(node.value("enabled", true));
		entity->SetVisible(node.value("visible", true));
		entity->SetAssemblyPath({});

		if (!preserveTransform && node.contains("transform"))
		{
			const Json& transform = node["transform"];
			const Reference<Transform> target = entity->GetTransform();

			if (transform.contains("position")) target->SetPosition(Vector2FromJson(transform["position"]));
			if (transform.contains("rotation")) target->SetRotation(RotationFromJson(transform["rotation"]));
			if (transform.contains("scale"))    target->SetScale(Vector2FromJson(transform["scale"]));
		}

		if (node.contains("components"))
		{
			const Json& components = node["components"];

			if (components.is_array())
			{
				// Ordered format: add components in their saved order (safe in any order because
				// RigidBody2D::EnsureBody creates the body on demand). Each is created from its
				// registered type name and restores its own fields, so user components load the same way.
				for (const auto& entry : components)
				{
					const std::string type = entry.value("type", std::string());

					if (Component* component = entity->AddComponentByName(type))
					{
						JsonSerializer serializer(entry);
						component->Deserialize(serializer);
					}
				}
			}
			else
			{
				// Legacy keyed-object format (older saved scenes). RigidBody first for awake order.
				if (components.contains("RigidBody2D"))
				{
					const Json& body = components["RigidBody2D"];
					entity->AddComponent<RigidBody2D>(BodyTypeFromString(body.value("type", std::string("Static"))), body.value("fixedRotation", false));
				}

				if (components.contains("BoxCollider2D"))
				{
					const Json& collider = components["BoxCollider2D"];
					entity->AddComponent<BoxCollider2D>(
						collider.value("width", 1.0f), collider.value("height", 1.0f),
						collider.value("density", 1.0f), collider.value("friction", 0.2f), collider.value("restitution", 0.0f));
				}

				if (components.contains("CircleCollider2D"))
				{
					const Json& collider = components["CircleCollider2D"];
					entity->AddComponent<CircleCollider2D>(
						collider.value("radius", 1.0f),
						collider.value("density", 1.0f), collider.value("friction", 0.2f), collider.value("restitution", 0.0f));
				}

				if (components.contains("SpriteRenderer"))
				{
					const Json& renderer = components["SpriteRenderer"];
					entity->AddComponent<SpriteRenderer>(renderer.value("texture", std::string()));
				}
			}
		}

	}

	static std::vector<Reference<Entity>> EntityTreeFromJson(const Json& node, const std::string& resourceRoot)
	{
		if (node.contains("assembly"))
		{
			const std::string path = node.value("assembly", std::string());
			std::vector<Reference<Entity>> tree = AssemblySerializer::DeserializeTree(path, resourceRoot);

			if (tree.empty())
			{
				tree.push_back(MakeReference<Entity>());
				tree.front()->SetName(std::filesystem::path(path).stem().string() + " (Missing Assembly)");
			}

			Reference<Entity> instance = tree.front();
			instance->SetAssemblyPath(path);
			instance->SetVisible(node.value("visible", instance->IsVisible()));

			if (node.contains("placement"))
			{
				ApplyAssemblyPlacement(instance, node["placement"]);
			}
			else if (node.contains("transform"))
			{
				// Scene files written before relative Assembly placement stored the final local Transform.
				// Read that shape exactly once; the next save migrates it to a placement delta.
				const Json& transform = node["transform"];
				const Reference<Transform> target = instance->GetTransform();

				if (transform.contains("position")) target->SetPosition(Vector2FromJson(transform["position"]));
				if (transform.contains("rotation")) target->SetRotation(RotationFromJson(transform["rotation"]));
				if (transform.contains("scale"))    target->SetScale(Vector2FromJson(transform["scale"]));
			}

			return tree;
		}

		Reference<Entity> entity = MakeReference<Entity>();
		PopulateEntityFromJson(entity, node, false);
		return { entity };
	}

	Reference<Entity> SceneSerializer::DeserializeEntityFromString(const Reference<Scene>& scene, const std::string& text)
	{
		return DeserializeEntityFromString(scene, text, {});
	}

	Reference<Entity> SceneSerializer::DeserializeEntityFromString(const Reference<Scene>& scene,
		const std::string& text, const std::string& resourceRoot)
	{
		Json node;

		try
		{
			node = Json::parse(text);
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneSerializer] Invalid entity JSON: {}", exception.what()));
			return nullptr;
		}

		std::vector<Reference<Entity>> entities = EntityTreeFromJson(node, resourceRoot);

		for (const auto& entity : entities)
			scene->Add(entity);

		return entities.empty() ? nullptr : entities.front();
	}

	Reference<Entity> SceneSerializer::DeserializeEntityDefinitionFromString(const std::string& text)
	{
		Json node;

		try
		{
			node = Json::parse(text);
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error,
				LION_FORMAT_TEXT("[SceneSerializer] Invalid Assembly entity JSON: {}", exception.what()));
			return nullptr;
		}

		Reference<Entity> entity = MakeReference<Entity>();
		PopulateEntityFromJson(entity, node, false);
		return entity;
	}

	bool SceneSerializer::DeserializeEntityDefinitionInto(const Reference<Entity>& entity,
		const std::string& text, bool preserveTransform)
	{
		if (!entity)
			return false;

		Json node;

		try
		{
			node = Json::parse(text);
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error,
				LION_FORMAT_TEXT("[SceneSerializer] Invalid Assembly entity JSON: {}", exception.what()));
			return false;
		}

		PopulateEntityFromJson(entity, node, preserveTransform);

		if (entity->mScene)
			entity->Awake();

		return true;
	}

	std::vector<Reference<Entity>> SceneSerializer::DeserializeEntityTreeDefinitionFromString(
		const std::string& text)
	{
		Json definition;

		try
		{
			definition = Json::parse(text);
		}
		catch (const std::exception& exception)
		{
			Log::Console(LogLevel::Error,
				LION_FORMAT_TEXT("[SceneSerializer] Invalid Assembly hierarchy JSON: {}", exception.what()));
			return {};
		}

		// Definitions written before Assemblies gained children were a single entity node. Reading that
		// shape as a one-element hierarchy keeps existing projects forward-compatible.
		if (!definition.contains("entities"))
		{
			Reference<Entity> entity = MakeReference<Entity>();
			PopulateEntityFromJson(entity, definition, false);
			return { entity };
		}

		const Json& nodes = definition["entities"];

		if (!nodes.is_array() || nodes.empty())
			return {};

		std::vector<Reference<Entity>> entities;
		entities.reserve(nodes.size());

		for (const auto& node : nodes)
		{
			Reference<Entity> entity = MakeReference<Entity>();
			PopulateEntityFromJson(entity, node, false);
			entities.push_back(std::move(entity));
		}

		for (size_t index = 0; index < nodes.size(); ++index)
		{
			const int32 parent = nodes[index].value("parent", -1);

			if (parent >= 0 && parent < static_cast<int32>(entities.size())
				&& parent != static_cast<int32>(index))
				entities[index]->SetParent(entities[parent].get(), false);
		}

		return entities;
	}

	bool SceneSerializer::DeserializeEntityTreeDefinitionInto(const Reference<Scene>& scene,
		const Reference<Entity>& rootEntity, const std::string& text, bool preserveRootTransform)
	{
		if (!scene || !rootEntity)
			return false;

		std::vector<Reference<Entity>> definition = DeserializeEntityTreeDefinitionFromString(text);

		if (definition.empty())
			return false;

		// Linked instances have no overrides. Replacing the authored subtree is therefore exact: children
		// removed from the source disappear, new ones appear, and reordered/reparented ones follow it.
		const std::vector<Entity*> oldChildren = rootEntity->GetChildren();
		for (Entity* child : oldChildren)
			scene->Remove(child);
		scene->FlushRemovals();

		const std::string rootNode = SerializeEntityDefinitionToString(definition.front());
		if (!DeserializeEntityDefinitionInto(rootEntity, rootNode, preserveRootTransform))
			return false;

		std::unordered_map<const Entity*, Entity*> replacements;
		replacements.emplace(definition.front().get(), rootEntity.get());

		for (size_t index = 1; index < definition.size(); ++index)
		{
			Entity* parent = definition[index]->GetParent();
			const auto replacement = replacements.find(parent);

			if (replacement != replacements.end())
				definition[index]->SetParent(replacement->second, false);

			replacements.emplace(definition[index].get(), definition[index].get());
			scene->Add(definition[index]);
		}

		return true;
	}

	bool SceneSerializer::Deserialize(const Reference<Scene>& scene, const std::string& filePath)
	{
		std::ifstream file(filePath);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Warning, LION_FORMAT_TEXT("[SceneSerializer] Failed to open scene: '{}'.", filePath));
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		// A scene a project keeps is plain JSON; a scene a game ships is sealed. This does not have to know
		// which it opened — the content says so itself, and unsealing something plain gives it back.
		std::string resourceRoot;
		std::filesystem::path ancestor = std::filesystem::absolute(filePath).parent_path();

		while (!ancestor.empty())
		{
			if (ancestor.filename() == "Assets")
			{
				resourceRoot = ancestor.generic_string();
				break;
			}

			const std::filesystem::path parent = ancestor.parent_path();

			if (parent == ancestor)
				break;

			ancestor = parent;
		}

		if (!DeserializeFromString(scene, Vault::Unseal(buffer.str()), resourceRoot))
			return false;

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[SceneSerializer] Loaded scene: '{}'.", filePath));
		return true;
	}
}
