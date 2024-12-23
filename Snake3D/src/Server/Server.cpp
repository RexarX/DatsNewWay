#include "Server.h"

#include "Game/GameObjects.h"

constexpr const char* MOVE_ENDPOINT = "player/move";

namespace Snake {
  void Server::Connect(std::string_view url, std::string_view token) {
    if (url.empty()) {
      CORE_ASSERT(false, "Failed to connect to the server: url is empty!");
      return;
    }

    if (token.empty()) {
      CORE_ASSERT(false, "Failed to connect to the server: token is empty!");
      return;
    }

    if (url == m_Url && token == m_Token && m_State == State::Connected) { return; }

    m_Url = url;
    m_Token = token;

    m_Session.SetUrl(std::format("{}/{}", url, MOVE_ENDPOINT));
    m_Session.SetHeader({
      { "X-Auth-Token", m_Token},
      { "Content-Type", "application/json" }
    });

    m_State = State::Connected;
  }

  void Server::Disconnect() {
    m_State = State::Disconnected;
  }

  void Server::Update() {
    if (m_State == State::Disconnected) {
      CORE_ASSERT(false, "Failed to update server: server is not connected!");
      return;
    }

    cpr::Response response = m_Session.Post();
    if (response.error) {
      CORE_ASSERT(false, "Failed to update server: {}!", response.error.message);
      return;
    }

    glz::error_ctx err = glz::read_json(m_GameState, response.text);
    if (!err) {
      m_State = State::Connected;
      return;
    }

    CORE_ERROR("Error while updating server: Failed to parse server response: {}!", glz::format_error(err, response.text));

    err = glz::read_json(m_LastError, response.text);
    if (err) {
      if (m_LastError.errCode == 23) { // No active game error
        m_State = State::WaitingForNextGame;
        if (!m_LastError.nextRounds.empty()) {
          const GameRound& nextGame = m_LastError.nextRounds[0];
          CORE_INFO("No active game. Next game '{}' starts at {}", nextGame.name, nextGame.startTime);
        }
      }
      return;
    }

    CORE_ASSERT(false, "Failed to update server: Failed to parse server response: {}!", glz::format_error(err, response.text));
  }

  void Server::Send(std::string_view json) {
    if (m_State == State::Disconnected) {
      CORE_ASSERT(false, "Failed to post to the server: server is not connected!");
      return;
    }

    if (m_State == State::WaitingForNextGame) { return; }

    if (json.empty()) {
      CORE_ASSERT(false, "Failed to post to the server: json is empty!");
      return;
    }

    CORE_INFO("Sending json: {}", json);
    m_Session.SetBody(cpr::Body(json));
    cpr::Response response = m_Session.Post();
    if (response.error) {
      CORE_ASSERT(false, "Failed to post to the server: {}!", response.error.message);
      return;
    }
  }

  void Server::PrintGameState() {
    CORE_INFO("Game state:\nMap size: ({}, {}, {})\nName: {}\nPoints: {}\nTurn: {}\nTick remain ms: {}\nRevive timeout: {} seconds",
              m_GameState.mapSize.x, m_GameState.mapSize.y, m_GameState.mapSize.z,
              m_GameState.name, m_GameState.points, m_GameState.turn, m_GameState.reviveTimeoutSec, m_GameState.tickRemainMs);
  }
}