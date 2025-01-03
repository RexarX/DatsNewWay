#include "Application.h"

namespace Snake {
	Application::Application(std::string_view name, uint32_t windowWidth, uint32_t windowHeight,
													 uint32_t framerateLimit, uint32_t serverTickRate)
		: m_Name(name), m_WindowWidth(windowWidth), m_WindowHeight(windowHeight),
		m_FramerateLimit(framerateLimit), m_ServerTickRate(serverTickRate) {
		CORE_ASSERT(m_Instance == nullptr, "Application already exists!");
		m_Instance = this;

		Init();
	}

	Application::~Application() {
	}

	void Application::Run() {
		m_RenderThread = std::thread(&Application::RenderLoop, this);
		m_UpdateThread = std::thread(&Application::UpdateLoop, this);

		m_RenderThread.join();
		m_UpdateThread.join();
	}

	void Application::SetFramerateLimit(uint32_t limit) noexcept {
		m_FramerateLimit = limit;
		m_FramerateLimitSec = limit > 0.0 ? 1.0 / limit : 0.0;
	}

	void Application::Init() {
		if (m_Running) {
			CORE_ASSERT(false, "Application in already inilialized!");
			return;
		}
		
		m_Running = true;
	}

	void Application::UpdateLoop() {
		double lastUpdateTime = 0.0;
		double serverTickLimitSec = m_ServerTickRate == 0.0 ? 0.0 : 1.0 / m_ServerTickRate;

		Utils::Timer timer;
		timer.Start();

		while (m_Running) {
			timer.Stop();
			m_UpdateDeltaTime = timer.GetElapsedSec() - lastUpdateTime;
			if (m_ServerTickRate == 0.0 || m_UpdateDeltaTime.GetSeconds() >= serverTickLimitSec) {
				//CORE_TRACE("Update loop: {}", m_UpdateDeltaTime.GetMilliseconds());
				m_Server.Update();
				if (m_Server.GetState() == Server::State::Connected) {
					const GameState& gameState = m_Server.GetGameState();
					if (gameState.tickRemainMs < serverTickLimitSec * 1000 / 2) {
						CORE_WARN("Sleeping for {} ms!", gameState.tickRemainMs);
						std::this_thread::sleep_for(std::chrono::milliseconds(gameState.tickRemainMs));
						continue;
					}

					m_Game.Update(gameState);
					m_Server.PrintGameState();
				}

				lastUpdateTime = timer.GetElapsedSec();
			}
		}

		m_Running = false;
	}

	void Application::RenderLoop() {
		m_Renderer.Init(m_Name, m_WindowWidth, m_WindowHeight);

		double lastRenderTime = 0.0;
		double framerateLimitSec = m_FramerateLimit == 0.0 ? 0.0 : 1.0 / m_FramerateLimit;
		GameState localState;

		Utils::Timer timer;
		timer.Start();

		while (!m_Renderer.ShouldStop()) {
			timer.Stop();
			m_RenderDeltaTime = timer.GetElapsedSec() - lastRenderTime;

			if (m_FramerateLimit == 0.0 || m_RenderDeltaTime.GetSeconds() >= framerateLimitSec) {
				//CORE_TRACE("Render loop: {}", m_RenderDeltaTime.GetMilliseconds());
				{
					std::lock_guard<std::mutex> lock(m_ServerMutex);
					localState = m_Server.GetGameState();
				}
				m_Renderer.Update(m_RenderDeltaTime);
				m_Renderer.Render(m_RenderDeltaTime, localState);

				lastRenderTime = timer.GetElapsedSec();
				++m_FrameCounter;
			}
		}

		m_Running = false;
	}
}