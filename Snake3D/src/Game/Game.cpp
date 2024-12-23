#include "Game.h"

#include "Application.h"

namespace Snake {
  constexpr std::array<Coords, 6> DIRECTIONS = {
    Coords{ 1, 0, 0 }, Coords{ -1, 0, 0 },
    Coords{ 0, 1, 0 }, Coords{ 0, -1, 0 },
    Coords{ 0, 0, 1 }, Coords{ 0, 0, -1 }
  };

  void Game::Update(const GameState& gameState) {
    Utils::Timer timer;
    timer.Start();

    Application& app = Application::Get();

    m_Snakes.snakesData.resize(gameState.snakes.size());

    // Pre-compute obstacles for all snakes
    std::unordered_set<Coords, CoordsHash> globalObstacles;
    for (const Coords& fence : gameState.fences) {
      globalObstacles.insert(fence);
    }

    // Add enemy snake bodies to obstacles
    for (const EnemySnake& enemy : gameState.enemies) {
      if (enemy.status == "alive") {
        for (const Coords& pos : enemy.geometry) {
          globalObstacles.insert(pos);
        }
        AddSurroundingCellsAsObstacles(enemy.geometry.front(), globalObstacles, gameState.mapSize);
      }
    }

    // Process all snakes in parallel
    std::for_each(std::execution::par_unseq, gameState.snakes.begin(), gameState.snakes.end(), [this, &gameState, &globalObstacles](const PlayerSnake& snake) {
      uint64_t index = std::distance(&gameState.snakes.front(), &snake);
      ProcessSnake(snake, gameState, globalObstacles, m_Snakes.snakesData[index]);
    });

    glz::error_ctx err = glz::write_json(m_Snakes, m_Json);
    if (err) {
      CORE_ASSERT(false, "Failed to update game: Failed to write json: {}!", glz::format_error(err, m_Json));
      return;
    }

    app.SendJsonToServer(m_Json);
    timer.Stop();
    CORE_INFO("Game::Update took {} ms", timer.GetElapsedMilliSec());
  }

  void Game::ProcessSnake(const PlayerSnake& snake, const GameState& gameState, const std::unordered_set<Coords, CoordsHash>& globalObstacles, SnakeData& snakeData) {
    if (snake.status != "alive" || snake.geometry.empty()) { return; }

    snakeData.id = snake.id;

    // Create a local copy of obstacles for this snake
    std::unordered_set<Coords, CoordsHash> obstacles(globalObstacles);

    // Add other snake bodies
    for (const PlayerSnake& playerSnake : gameState.snakes) {
      if (playerSnake.status == "alive") {
        for (uint64_t j = (playerSnake.id == snake.id ? 1 : 0); j < playerSnake.geometry.size(); ++j) {
          obstacles.insert(playerSnake.geometry[j]);
        }
      }
    }

    for (const EnemySnake& enemySnake : gameState.enemies) {
      if (enemySnake.status == "alive") {
        for (const Coords& pos : enemySnake.geometry) {
          obstacles.insert(pos);
        }
        AddSurroundingCellsAsObstacles(enemySnake.geometry[0], obstacles, gameState.mapSize);
      }
    }

    snakeData.direction = FindPathToClosestFood(
      snake.geometry.front(),
      snake.direction,
      obstacles,
      gameState.food,
      gameState.specialFood,
      gameState.mapSize
    );
  }

  Coords Game::FindPathToClosestFood(const Coords& start, const Coords& currentDirection, const std::unordered_set<Coords, CoordsHash>& obstacles,
                                     const std::vector<Food>& foods, const SpecialFood& specialFoods, const Coords& mapSize) {
    if (foods.empty()) { return currentDirection; }

    std::queue<Cell> queue;
    std::unordered_set<Coords, CoordsHash> visited;
    queue.push({ start, {} });
    visited.insert(start);

    while (!queue.empty()) {
      Cell current = queue.front();
      queue.pop();

      // Check if current position contains fruit
      for (const Food& food : foods) {
        if (food.coords == current.pos) {
          if (current.path.empty()) {
            for (const Coords& dir : DIRECTIONS) {
              Coords newPos(current.pos);
              newPos += dir;
              if (IsWithinMapBounds(newPos, mapSize)) { return dir; }
            }
          }
          return current.path.front();
        }
      }

      // Try all possible directions
      for (const Coords& dir : DIRECTIONS) {
        Coords newPos(current.pos);
        newPos += dir;

        // Check boundaries
        if (IsWithinSectorBounds(newPos, current.pos) && !IsWithinMapBounds(newPos, mapSize)) { continue; }

        // Check if position is visited or contains obstacle
        if (visited.contains(newPos) || obstacles.contains(newPos)) { continue; }

        // Create new path by adding current direction
        std::vector<Coords> newPath(current.path);
        newPath.push_back(dir);

        // Add new cell to queue
        queue.emplace(newPos, std::move(newPath));
        visited.insert(std::move(newPos));
      }
    }

    return {};
  }

  void Game::AddSurroundingCellsAsObstacles(const Coords& position, std::unordered_set<Coords, CoordsHash>& obstacles, const Coords& mapSize) {
    for (const Coords& dir : DIRECTIONS) {
      Coords pos = position + dir;
      if (pos.x < 0 || pos.x > mapSize.x
          || pos.y < 0 || pos.y > mapSize.y
          || pos.z < 0 || pos.z > mapSize.z) {
        continue;
      }
      obstacles.insert(std::move(pos));
    }
  }
}