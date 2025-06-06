#pragma once

namespace Lion
{
	class Layer;

	class Stack
	{
	public:
		Stack() = default;
		~Stack();

		LION_API void PushLayer(Layer* layer);
		LION_API void PushOverlay(Layer* overlay);

		LION_API void PopLayer(Layer* layer);
		LION_API void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<Layer*>::const_iterator end()	const { return m_Layers.end(); }
		std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<Layer*> m_Layers;
		uint32 m_LayerInsertIndex = 0;
	};
}