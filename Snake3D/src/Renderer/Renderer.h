#pragma once

#include "pch.h"

#include <raylib.h>

namespace Snake {
  class Renderer {
  public:
    Renderer() = default;
    Renderer(std::string_view windowName, uint32_t width, uint32_t height);
    ~Renderer() {
      UnloadSkybox();
      CloseWindow();
    }

    void Init(std::string_view windowName, uint32_t width, uint32_t height);
    void Update(Timestep deltaTime);
    void Render(Timestep deltaTime, const GameState& gameState);

    inline bool ShouldStop() const noexcept { return WindowShouldClose(); }

  private:
    void InitializeMeshes();
    void InitializeSkybox();

    void UnloadSkybox();

    void UpdateCamera(Timestep deltaTime);
    void UpdateFrustum();

    void DrawSkybox();
    void DrawSimplifiedGrid(uint32_t size);
    void DrawSectorGrid(const GameState& gameState);
    void RenderSpecialFood(const SpecialFood& specialFood);
    void RenderSnakes(const std::vector<EnemySnake>& enemies, const std::vector<PlayerSnake>& players);
    void DrawSnakeOptimized(const std::vector<Coords>& geometry, Color color, bool isPlayer);
    void DrawHUD(const GameState& gameState, Timestep deltaTime);

    
    bool IsPointInFrustum(const Vector3& point) const noexcept;
    bool IsCubeInFrustum(const Vector3& position, float size) const noexcept;

    static Image CreateCubemapImage(const char* rightPath, const char* leftPath, const char* topPath,
                                    const char* bottomPath, const char* frontPath, const char* backPath);

    static void SaveCubemapImage(const char* outputPath, const char* rightPath, const char* leftPath,
                                 const char* topPath, const char* bottomPath, const char* frontPath,
                                 const char* backPath);

  private:
    struct Plane {
      Vector3 normal{ 0, 0, 0 };
      float distance = 0.0f;
    };

    struct Frustum {
      Plane planes[6];  // Left, Right, Top, Bottom, Near, Far
    };

    Camera3D m_Camera;
    float m_CameraAngleY = 0.0f;
    float m_CameraAngleX = 0.0f;
    float m_MouseSpeed = 0.1f;
    float m_MoveSpeed = 10.0f;

    Mesh m_CubeMesh;
    Model m_CubeModel;
    Mesh m_SphereMesh;
    Model m_SphereModel;
    bool m_ShowGrid = false;

    Mesh m_SkyboxMesh;
    Model m_SkyboxModel;

    Frustum m_Frustum;
  };
}