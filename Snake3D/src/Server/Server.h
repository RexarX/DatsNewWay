#pragma once

#include "pch.h"

#include <cpr/cpr.h>

namespace Snake {
  class Server {
  public:
    struct GameRound {
      std::string name;
      std::string startTime;
      std::string endTime;
    };

    struct Error {
      std::string error;
      int32_t errCode = 0;
      std::string currentTime;
      std::vector<GameRound> nextRounds;
    };

    enum class State {
      Connected, Disconnected,
      WaitingForNextGame
    };

    Server() = default;
    Server(const Server&) = delete;
    ~Server() { Disconnect(); }

    void Connect(std::string_view url, std::string_view token);
    void Disconnect();

    void Update();
    void Send(std::string_view json);

    void PrintGameState();

    inline State GetState() const noexcept { return m_State; }

    inline const std::string& GetUrl() const noexcept { return m_Url; }
    inline const std::string& GetToken() const noexcept { return m_Token; }

    inline const GameState& GetGameState() const noexcept { return m_GameState; }

  private:
    State m_State = State::Disconnected;
    Error m_LastError;

    std::string m_Url;
    std::string m_Token;

    GameState m_GameState;

    cpr::Session m_Session;
  };
}

template <>
struct glz::meta<Snake::Server::GameRound> {
  using T = Snake::Server::GameRound;
  static constexpr auto value = object(
    "name", &T::name,
    "startTime", &T::startTime,
    "endTime", &T::endTime
  );
};

template <>
struct glz::meta<Snake::Server::Error> {
  using T = Snake::Server::Error;
  static constexpr auto value = object(
    "error", &T::error,
    "errCode", &T::errCode,
    "currentTime", &T::currentTime,
    "nextRounds", &T::nextRounds
  );
};