#pragma once

#include <string>
#include <vector>

#include <glaze/glaze.hpp>

namespace Snake {
  constexpr uint32_t SECTOR_SIZE = 30;

  struct Coords {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    inline Coords& operator+=(const Coords& other) noexcept {
      x += other.x;
      y += other.y;
      z += other.z;
      return *this;
    }

    inline Coords& operator-=(const Coords& other) noexcept {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      return *this;
    }

    inline bool operator==(const Coords& other) const noexcept {
      return x == other.x && y == other.y && z == other.z;
    }
  };

  inline Coords operator+(const Coords& lhs, const Coords& rhs) noexcept {
    return Coords{
      .x = lhs.x + rhs.x,
      .y = lhs.y + rhs.y,
      .z = lhs.z + rhs.z,
    };
  }

  inline Coords operator-(const Coords& lhs, const Coords& rhs) noexcept {
    return Coords{
      .x = lhs.x - rhs.x,
      .y = lhs.y - rhs.y,
      .z = lhs.z - rhs.z,
    };
  }

  struct EnemySnake {
    std::vector<Coords> geometry;
    std::string status = "dead";
    uint32_t kills = 0;
  };

  struct PlayerSnake {
    std::string id;
    std::vector<Coords> geometry;
    Coords direction{ 0, 0, 0 };
    Coords oldDirection{ 0, 0, 0 };
    uint32_t deathCount = 0;
    std::string status = "dead";
    uint32_t reviveRemainMs = 0;
  };

  struct Food {
    Coords coords{ 0, 0, 0 };
    uint32_t points = 0;
    uint32_t type = 0;
  };

  struct SpecialFood {
    std::vector<Coords> golden;
    std::vector<Coords> suspicious;
  };

  struct GameState {
    Coords mapSize{ 0, 0, 0 };
    std::string name;
    uint32_t points = 0;
    std::vector<Coords> fences;
    std::vector<PlayerSnake> snakes;
    std::vector<EnemySnake> enemies;
    std::vector<Food> food;
    SpecialFood specialFood;
    uint32_t turn = 0;
    uint32_t tickRemainMs = 0;
    uint32_t reviveTimeoutSec = 0;
    std::vector<std::string> errors;
  };
}

template <>
struct glz::meta<Snake::Coords> {
  using T = Snake::Coords;
  static constexpr auto value = array(
    &T::x,
    &T::y,
    &T::z
  );
};

template <>
struct glz::meta<Snake::EnemySnake> {
  using T = Snake::EnemySnake;
  static constexpr auto value = object(
    "geometry", &T::geometry,
    "status", &T::status,
    "kills", &T::kills
  );
};

template <>
struct glz::meta<Snake::PlayerSnake> {
  using T = Snake::PlayerSnake;
  static constexpr auto value = object(
    "id", &T::id,
    "geometry", &T::geometry,
    "direction", &T::direction,
    "oldDirection", &T::oldDirection,
    "deathCount", &T::deathCount,
    "status", &T::status,
    "reviveRemainMs", &T::reviveRemainMs
  );
};

template <>
struct glz::meta<Snake::Food> {
  using T = Snake::Food;
  static constexpr auto value = object(
    "c", &T::coords,
    "points", &T::points,
    "type", &T::type
  );
};

template <>
struct glz::meta<Snake::SpecialFood> {
  using T = Snake::SpecialFood;
  static constexpr auto value = object(
    "golden", &T::golden,
    "suspicious", &T::suspicious
  );
};

template <>
struct glz::meta<Snake::GameState> {
  using T = Snake::GameState;
  static constexpr auto value = object(
    "mapSize", &T::mapSize,
    "name", &T::name,
    "points", &T::points,
    "fences", &T::fences,
    "snakes", &T::snakes,
    "enemies", &T::enemies,
    "food", &T::food,
    "specialFood", &T::specialFood,
    "turn", &T::turn,
    "tickRemainMs", &T::tickRemainMs,
    "reviveTimeoutSec", &T::reviveTimeoutSec,
    "errors", &T::errors
  );
};