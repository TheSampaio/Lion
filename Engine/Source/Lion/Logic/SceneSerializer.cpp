#include "Engine.h"
#include "SceneSerializer.h"

#include <nlohmann/json.hpp>

#include <Lion/Core/Log.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Math/Transform.h>
#include <Lion/Physics/BoxCollider2D.h>
#include <Lion/Physics/CircleCollider2D.h>
#include <Lion/Physics/RigidBody2D.h>
#include <Lion/Render/SpriteRenderer.h>

using Json = nlohmann::json;

namespace Lion
{
	static const char8* BodyTypeToString(BodyType type)
	{
		switch (type)
		{
			case BodyType::Kinematic: return "Kinematic";
			case BodyType::Dynamic:   return "Dynamic";
			case BodyType::Static:    return "Static";
		}

		return "Static";
	}

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

		const Reference<Transform> transform = entity->GetTransform();
		const Vector position = transform->GetPosition();
		const Vector rotation = transform->GetRotation();
		const Vector scale = transform->GetScale();

		node["transform"]["position"] = { position.x, position.y, position.z };
		node["transform"]["rotation"] = { rotation.x, rotation.y, rotation.z };
		node["transform"]["scale"]    = { scale.x, scale.y, scale.z };

		// Components are written as an ordered array so the editor's display/drag order round-trips.
		Json components = Json::array();

		for (const auto& component : entity->GetComponents())
		{
			Json entry;

			if (const SpriteRenderer* renderer = dynamic_cast<const SpriteRenderer*>(component.get()))
			{
				entry["type"] = "SpriteRenderer";
				entry["texture"] = renderer->GetTexturePath();
			}
			else if (const RigidBody2D* body = dynamic_cast<const RigidBody2D*>(component.get()))
			{
				entry["type"] = "RigidBody2D";
				entry["bodyType"] = BodyTypeToString(body->GetBodyType());
				entry["fixedRotation"] = body->IsFixedRotation();
			}
			else if (const BoxCollider2D* collider = dynamic_cast<const BoxCollider2D*>(component.get()))
			{
				entry["type"] = "BoxCollider2D";
				entry["width"] = collider->GetWidth();
				entry["height"] = collider->GetHeight();
				entry["density"] = collider->GetDensity();
				entry["friction"] = collider->GetFriction();
				entry["restitution"] = collider->GetRestitution();
			}
			else if (const CircleCollider2D* collider = dynamic_cast<const CircleCollider2D*>(component.get()))
			{
				entry["type"] = "CircleCollider2D";
				entry["radius"] = collider->GetRadius();
				entry["density"] = collider->GetDensity();
				entry["friction"] = collider->GetFriction();
				entry["restitution"] = collider->GetRestitution();
			}
			else
			{
				continue;
			}

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
		std::ofstream file(filePath);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneSerializer] Failed to write scene: '{}'.", filePath));
			return false;
		}

		file << SerializeToString(scene);
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
				// RigidBody2D::EnsureBody creates the body on demand).
				for (const auto& entry : components)
				{
					const std::string type = entry.value("type", std::string());

					if (type == "SpriteRenderer")
						entity->AddComponent<SpriteRenderer>(entry.value("texture", std::string()));
					else if (type == "RigidBody2D")
						entity->AddComponent<RigidBody2D>(BodyTypeFromString(entry.value("bodyType", std::string("Static"))), entry.value("fixedRotation", false));
					else if (type == "BoxCollider2D")
						entity->AddComponent<BoxCollider2D>(
							entry.value("width", 1.0f), entry.value("height", 1.0f),
							entry.value("density", 1.0f), entry.value("friction", 0.2f), entry.value("restitution", 0.0f));
					else if (type == "CircleCollider2D")
						entity->AddComponent<CircleCollider2D>(
							entry.value("radius", 1.0f),
							entry.value("density", 1.0f), entry.value("friction", 0.2f), entry.value("restitution", 0.0f));
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

		if (!DeserializeFromString(scene, buffer.str()))
			return false;

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[SceneSerializer] Loaded scene: '{}'.", filePath));
		return true;
	}
}
