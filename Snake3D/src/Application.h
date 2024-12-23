#pragma once

#include "Game/Game.h"

#include "Renderer/Renderer.h"

#include "Server/Server.h"

namespace Snake {
	class Application {
	public:
		Application(std::string_view name, uint32_t windowWidth = 1280, uint32_t windowHeight = 720,
								uint32_t framerateLimit = 0, uint32_t serverTickRate = 1);

		Application(const Application&) = delete;
		~Application();

		void Run();

		void ConnectToServer(std::string_view url, std::string_view token) {
			m_Server.Connect(url, token);
		}

		void SendJsonToServer(std::string_view json) {
			m_Server.Send(json);
		}

		void SetFramerateLimit(uint32_t limit) noexcept;

		inline const std::string& GetName() const noexcept { return m_Name; }

		inline const Server& GetServer() const noexcept { return m_Server; }

		inline Timestep GetDeltaTime() const noexcept { return m_RenderDeltaTime; }
		inline uint32_t GetFramerateLimit() const noexcept { return m_FramerateLimit; }
		inline uint64_t GetFrameNumber() const noexcept { return m_FrameCounter; }

		inline uint32_t GetServerTickRate() const noexcept { return m_ServerTickRate; }

		static inline Application& Get() noexcept { return *m_Instance; }
	
	private:
		void Init();

		void UpdateLoop();
		void RenderLoop();

	private:
		static inline Application* m_Instance = nullptr;

		std::string m_Name;
		uint32_t m_WindowWidth = 0;
		uint32_t m_WindowHeight = 0;

		Game m_Game;
		Renderer m_Renderer;
		Server m_Server;

		std::mutex m_ServerMutex;

		bool m_Running = false;
		std::thread m_UpdateThread;
		std::thread m_RenderThread;

		Timestep m_RenderDeltaTime = 0.0;
		uint32_t m_FramerateLimit = 0.0;
		double m_FramerateLimitSec = 0.0;
		uint64_t m_FrameCounter = 0;

		Timestep m_UpdateDeltaTime = 0.0;
		uint32_t m_ServerTickRate = 0;
		double m_ServerTickLimitSec = 0.0;
	};
}