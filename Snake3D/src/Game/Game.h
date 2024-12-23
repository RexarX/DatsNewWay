#pragma once

#include "pch.h"

#include <raylib.h>

struct CoordsHash {
  inline size_t operator()(const Snake::Coords& v) const {
    return std::hash<int32_t>()(v.x) ^ (std::hash<int32_t>()(v.y) << 1) ^ (std::hash<int32_t>()(v.z) << 2);
  }
};

namespace Snake {
  class Game {
  public:
    Game() = default;
    ~Game() = default;

    void Update(const GameState& gameState);

  private:
    struct SnakeData {
      std::string id;
      Coords direction{ 0, 0, 0 };
    };

    static void ProcessSnake(const PlayerSnake& snake, const GameState& gameState, const std::unordered_set<Coords, CoordsHash>& globalObstacles, SnakeData& snakeData);

    static Coords FindPathToClosestFood(const Coords& start, const Coords& currentDirection, const std::unordered_set<Coords, CoordsHash>& obstacles,
                                        const std::vector<Food>& foods, const SpecialFood& specialFoods, const Coords& mapSize);

    static void AddSurroundingCellsAsObstacles(const Coords& position, std::unordered_set<Coords, CoordsHash>& obstacles, const Coords& mapSize);

    static inline bool IsWithinMapBounds(const Coords& pos, const Coords& mapSize) {
      return pos.x >= 0 && pos.x <= mapSize.x && pos.y >= 0 && pos.y <= mapSize.y && pos.z >= 0 && pos.z <= mapSize.z;
    }

    static inline Coords GetSectorCoords(const Coords& pos) {
      return Coords{ static_cast<int32_t>(pos.x / SECTOR_SIZE), static_cast<int32_t>(pos.y / SECTOR_SIZE), static_cast<int32_t>(pos.z / SECTOR_SIZE) };
    }

    static inline bool IsWithinSectorBounds(const Coords& pos1, const Coords& pos2) {
      Coords sector1 = GetSectorCoords(pos1);
      Coords sector2 = GetSectorCoords(pos2);
      return std::abs(sector1.x - sector2.x) <= 1 && std::abs(sector1.y - sector2.y) <= 1 && std::abs(sector1.z - sector2.z) <= 1;
    }

  private:
    struct Cell {
      Coords pos{ 0, 0, 0 };
      std::vector<Coords> path;
    };

    struct WeightedCell {
      Cell cell;
      float score = 0.0f;

      inline bool operator<(const WeightedCell& other) const {
        return score < other.score;
      }
    };

    struct Snakes {
      std::vector<SnakeData> snakesData;
    };

    Snakes m_Snakes;
    std::string m_Json;

    friend struct glz::meta<SnakeData>;
    friend struct glz::meta<Snakes>;
  };
}

template <>
struct glz::meta<Snake::Game::Snakes> {
  using T = Snake::Game::Snakes;
  static constexpr auto value = object(
    "snakes", &T::snakesData
  );
};

template <>
struct glz::meta<Snake::Game::SnakeData> {
  using T = Snake::Game::SnakeData;
  static constexpr auto value = object(
    "id", &T::id,
    "direction", &T::direction
  );
};