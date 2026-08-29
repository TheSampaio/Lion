#include "Engine.h"
#include "Input.h"

#include <Lion/Core/Vault.h>
#include <Lion/Core/Window.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace Lion
{
    Input* Input::sInstance = nullptr;
	bool Input::sControlKeys[GLFW_KEY_LAST + 1] = { false };
	std::vector<InputAction> Input::sActions;
	std::unordered_map<std::string, float32> Input::sActionStrengths;
	std::unordered_map<std::string, float32> Input::sPreviousActionStrengths;

	namespace
	{
		const char8* DeviceName(InputDevice device)
		{
			switch (device)
			{
				case InputDevice::Keyboard:      return "keyboard";
				case InputDevice::MouseButton:   return "mouse_button";
				case InputDevice::GamepadButton: return "gamepad_button";
				case InputDevice::GamepadAxis:   return "gamepad_axis";
			}

			return "keyboard";
		}

		InputDevice ParseDevice(const std::string& name)
		{
			if (name == "mouse_button")   return InputDevice::MouseButton;
			if (name == "gamepad_button") return InputDevice::GamepadButton;
			if (name == "gamepad_axis")   return InputDevice::GamepadAxis;
			return InputDevice::Keyboard;
		}
	}

    void Input::New()
    {
        sInstance = new Input();
    }

    void Input::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    bool Input::GetKeyPress(KeyCode keyCode)
    {
        return Window::IsKeyPressed(static_cast<int32>(keyCode));
    }

    bool Input::GetKeyRelease(KeyCode keyCode)
    {
        return Window::IsKeyReleased(static_cast<int32>(keyCode));
    }

    bool Input::GetKeyTap(KeyCode keyCode)
    {
        const int32 keyCodeId = static_cast<int32>(keyCode);

		if (keyCodeId < 0 || keyCodeId > GLFW_KEY_LAST)
			return false;

        if (GetKeyPress(keyCode))
            sControlKeys[keyCodeId] = true;

        if (GetKeyRelease(keyCode) && sControlKeys[keyCodeId])
        {
            sControlKeys[keyCodeId] = false;
            return true;
        }

        return false;
    }

	bool Input::GetMouseButtonPress(int32 button)
	{
		return Window::IsMouseButtonPressed(button);
	}

	bool Input::IsGamepadConnected(int32 gamepad)
	{
		return Window::IsGamepadConnected(gamepad);
	}

	std::string Input::GetGamepadName(int32 gamepad)
	{
		return Window::GetGamepadName(gamepad);
	}

	bool Input::GetGamepadButtonPress(GamepadButton button, int32 gamepad)
	{
		return Window::IsGamepadButtonPressed(gamepad, static_cast<int32>(button));
	}

	float32 Input::GetGamepadAxis(GamepadAxis axis, int32 gamepad)
	{
		return Window::GetGamepadAxis(gamepad, static_cast<int32>(axis));
	}

	float32 Input::EvaluateAction(const InputAction& action)
	{
		float32 strength = 0.0f;

		for (const InputBinding& binding : action.bindings)
		{
			float32 value = 0.0f;

			switch (binding.device)
			{
				case InputDevice::Keyboard:
					value = Window::IsKeyPressed(binding.code) ? 1.0f : 0.0f;
					break;

				case InputDevice::MouseButton:
					value = Window::IsMouseButtonPressed(binding.code) ? 1.0f : 0.0f;
					break;

				case InputDevice::GamepadButton:
				{
					const int32 first = binding.gamepad < 0 ? 0 : binding.gamepad;
					const int32 last = binding.gamepad < 0 ? 15 : binding.gamepad;

					for (int32 gamepad = first; gamepad <= last && value == 0.0f; ++gamepad)
						value = Window::IsGamepadButtonPressed(gamepad, binding.code) ? 1.0f : 0.0f;
					break;
				}

				case InputDevice::GamepadAxis:
				{
					const int32 first = binding.gamepad < 0 ? 0 : binding.gamepad;
					const int32 last = binding.gamepad < 0 ? 15 : binding.gamepad;

					for (int32 gamepad = first; gamepad <= last; ++gamepad)
					{
						if (!Window::IsGamepadConnected(gamepad))
							continue;

						float32 axis = Window::GetGamepadAxis(gamepad, binding.code);

						// GLFW exposes triggers as -1..1 while sticks are centered at zero. Project actions
						// speak the useful 0..1 range for both kinds.
						if (binding.code == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER
							|| binding.code == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER)
							axis = (axis + 1.0f) * 0.5f;

						value = std::max(value, std::max(0.0f, axis * binding.scale));
					}
					break;
				}
			}

			if (binding.device != InputDevice::GamepadAxis)
				value = std::max(0.0f, value * binding.scale);

			const float32 deadzone = std::clamp(action.deadzone, 0.0f, 0.99f);
			value = value <= deadzone ? 0.0f : (value - deadzone) / (1.0f - deadzone);
			strength = std::max(strength, std::clamp(value, 0.0f, 1.0f));
		}

		return strength;
	}

	void Input::Update()
	{
		sPreviousActionStrengths = sActionStrengths;
		sActionStrengths.clear();

		for (const InputAction& action : sActions)
			sActionStrengths[action.name] = EvaluateAction(action);
	}

	float32 Input::GetActionStrength(const std::string& action)
	{
		const auto found = sActionStrengths.find(action);
		return found == sActionStrengths.end() ? 0.0f : found->second;
	}

	bool Input::GetActionPress(const std::string& action)
	{
		return GetActionStrength(action) > 0.0f;
	}

	bool Input::GetActionTap(const std::string& action)
	{
		const auto previous = sPreviousActionStrengths.find(action);
		return GetActionStrength(action) > 0.0f
			&& (previous == sPreviousActionStrengths.end() || previous->second <= 0.0f);
	}

	const std::vector<InputAction>& Input::GetActionMap()
	{
		return sActions;
	}

	void Input::SetActionMap(const std::vector<InputAction>& actions)
	{
		sActions = actions;
		sActionStrengths.clear();
		sPreviousActionStrengths.clear();
	}

	bool Input::LoadActionMap(const std::string& path)
	{
		std::ifstream file(path);

		if (!file.is_open())
			return false;

		std::stringstream buffer;
		buffer << file.rdbuf();

		try
		{
			const nlohmann::json root = nlohmann::json::parse(Vault::Unseal(buffer.str()));
			std::vector<InputAction> actions;

			for (const auto& value : root.value("actions", nlohmann::json::array()))
			{
				InputAction action;
				action.name = value.value("name", std::string());
				action.deadzone = value.value("deadzone", 0.2f);

				if (action.name.empty())
					continue;

				for (const auto& item : value.value("bindings", nlohmann::json::array()))
					action.bindings.push_back({
						ParseDevice(item.value("device", std::string("keyboard"))),
						item.value("code", 0),
						item.value("scale", 1.0f),
						item.value("gamepad", -1)
					});

				actions.push_back(std::move(action));
			}

			SetActionMap(actions);
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	bool Input::SaveActionMap(const std::string& path, const std::vector<InputAction>& actions)
	{
		nlohmann::json root;
		root["actions"] = nlohmann::json::array();

		for (const InputAction& action : actions)
		{
			nlohmann::json value;
			value["name"] = action.name;
			value["deadzone"] = action.deadzone;
			value["bindings"] = nlohmann::json::array();

			for (const InputBinding& binding : action.bindings)
				value["bindings"].push_back({
					{ "device", DeviceName(binding.device) },
					{ "code", binding.code },
					{ "scale", binding.scale },
					{ "gamepad", binding.gamepad }
				});

			root["actions"].push_back(std::move(value));
		}

		std::error_code error;
		const std::filesystem::path filePath(path);
		std::filesystem::create_directories(filePath.parent_path(), error);
		std::ofstream file(filePath, std::ios::trunc);

		if (!file.is_open())
			return false;

		file << root.dump(2);
		return file.good();
	}
}
