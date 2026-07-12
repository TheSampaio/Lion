#include "Engine.h"
#include "SceneSerializer.h"

#include <nlohmann/json.hpp>

#include <Lion/Core/Log.h>
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

	static Vector VectorFromJson(const Json& array)
	{
		return Vector(array.at(0).get<float32>(), array.at(1).get<float32>(), array.at(2).get<float32>());
	}

	// Serializes one entity (name, transform and its ordered components) into a JSON node.
	static Json EntityToJson(const Reference<Entity>& entity)
	{
		Json node;
		node["name"] = entity->GetName();

		if (entity->IsFolder())
			node["folder"] = true;

		// Only what departs from the default is written: an entity is enabled and visible unless it says
		// otherwise, and a scene file that repeats the obvious is a scene file nobody can read.
		if (!entity->IsEnabled())
			node["enabled"] = false;

		if (!entity->IsVisible())
			node["visible"] = false;

		const Reference<Transform> transform = entity->GetTransform();
		const Vector position = transform->GetPosition();
		const Vector rotation = transform->GetRotation();
		const Vector scale = transform->GetScale();

		node["transform"]["position"] = { position.x, position.y, position.z };
		node["transform"]["rotation"] = { rotation.x, rotation.y, rotation.z };
		node["transform"]["scale"]    = { scale.x, scale.y, scale.z };

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

	// Rebuilds one entity from a JSON node (does not add it to a scene).
	static Reference<Entity> EntityFromJson(const Json& node);

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
			indices.emplace(entity.get(), index++);

		for (const auto& entity : scene->GetEntities())
		{
			Json node = EntityToJson(entity);

			const auto parent = indices.find(entity->GetParent());
			node["parent"] = (parent != indices.end()) ? parent->second : -1;

			root["entities"].push_back(node);
		}

		return root.dump(2);
	}

	std::string SceneSerializer::SerializeEntityToString(const Reference<Entity>& entity)
	{
		return entity ? EntityToJson(entity).dump(2) : std::string();
	}

	bool SceneSerializer::Serialize(const Reference<Scene>& scene, const std::string& filePath)
	{
		std::ofstream file(filePath, std::ios::binary);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneSerializer] Failed to write scene: '{}'.", filePath));
			return false;
		}

		// A scene leaves the editor sealed, and it is JSON again by the time anything reads it — Vault::Unseal
		// takes plaintext back unchanged, so a scene written by hand still opens. Sealed is what a .lscene is,
		// not what shipping does to one, which is why nothing has to remember which kind it is holding.
		file << Vault::Seal(SerializeToString(scene));
		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[SceneSerializer] Saved scene: '{}'.", filePath));
		return true;
	}

	bool SceneSerializer::DeserializeFromString(const Reference<Scene>& scene, const std::string& text)
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
		std::vector<Reference<Entity>> entities;
		entities.reserve(root["entities"].size());

		for (const auto& node : root["entities"])
			entities.push_back(EntityFromJson(node));

		size_t index = 0;
		for (const auto& node : root["entities"])
		{
			const int32 parent = node.value("parent", -1);

			if (parent >= 0 && parent < static_cast<int32>(entities.size()) && parent != static_cast<int32>(index))
				entities[index]->SetParent(entities[parent].get(), false);  // Transforms are already local.

			index++;
		}

		for (const auto& entity : entities)
			scene->Add(entity);

		return true;
	}

	static Reference<Entity> EntityFromJson(const Json& node)
	{
		auto entity = MakeReference<Entity>();
		entity->SetName(node.value("name", std::string("Entity")));
		entity->SetFolder(node.value("folder", false));
		entity->SetEnabled(node.value("enabled", true));
		entity->SetVisible(node.value("visible", true));

		if (node.contains("transform"))
		{
			const Json& transform = node["transform"];
			const Reference<Transform> target = entity->GetTransform();

			if (transform.contains("position")) target->SetPosition(VectorFromJson(transform["position"]));
			if (transform.contains("rotation")) target->SetRotation(VectorFromJson(transform["rotation"]));
			if (transform.contains("scale"))    target->SetScale(VectorFromJson(transform["scale"]));
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

		return entity;
	}

	Reference<Entity> SceneSerializer::DeserializeEntityFromString(const Reference<Scene>& scene, const std::string& text)
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

		Reference<Entity> entity = EntityFromJson(node);
		scene->Add(entity);
		return entity;
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
		if (!DeserializeFromString(scene, Vault::Unseal(buffer.str())))
			return false;

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[SceneSerializer] Loaded scene: '{}'.", filePath));
		return true;
	}
}
