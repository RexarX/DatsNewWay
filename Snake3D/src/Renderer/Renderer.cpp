#include "Renderer.h"

#include <raymath.h>
#include <rlgl.h>
#include <rcamera.h>

namespace Snake {
  

  Renderer::Renderer(std::string_view windowName, uint32_t width, uint32_t height) {
    Init(windowName, width, height);
  }

  void Renderer::Init(std::string_view windowName, uint32_t width, uint32_t height) {
    InitWindow(width, height, windowName.data());
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowState(FLAG_VSYNC_HINT);

    InitializeMeshes();
    InitializeSkybox();

    m_Camera.position = Vector3{ 50.0f, 100.0f, 0.0f };
    m_Camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    m_Camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    m_Camera.fovy = 80.0f;
    m_Camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
  }

  void Renderer::Update(Timestep deltaTime) {
    UpdateCamera(deltaTime);
    UpdateFrustum();

    if (IsKeyPressed(KEY_F2)) {
      m_ShowGrid = !m_ShowGrid;
    }
  }

  void Renderer::Render(Timestep deltaTime, const GameState& gameState) {
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(m_Camera);

    rlEnableSmoothLines();

    DrawSkybox();

    if (m_ShowGrid) {
      DrawSectorGrid(gameState);
      DrawSimplifiedGrid(gameState.mapSize.x + 1);
    }

    // Batch render food using instancing
    for (const Food& food : gameState.food) {
      Vector3 position{
        static_cast<float>(food.coords.x),
        static_cast<float>(food.coords.y),
        static_cast<float>(food.coords.z)
      };

      if (IsPointInFrustum(position)) {
        DrawModelEx(m_SphereModel, position, { 0, 1, 0 }, 0.0f, { 1.0f, 1.0f, 1.0f }, WHITE);
      }
    }

    RenderSpecialFood(gameState.specialFood);

    RenderSnakes(gameState.enemies, gameState.snakes);

    rlDisableDepthMask();

    // Batch render fences
    for (const Coords& fence : gameState.fences) {
      Vector3 position{
        static_cast<float>(fence.x),
        static_cast<float>(fence.y),
        static_cast<float>(fence.z)
      };

      if (IsCubeInFrustum(position, 1.0f)) {
        DrawModelEx(m_CubeModel, position, { 0, 1, 0 }, 0.0f, { 1.0f, 1.0f, 1.0f }, ColorAlpha(GRAY, 0.5f));
      }
    }

    rlEnableDepthMask();
    
    EndMode3D();

    DrawHUD(gameState, deltaTime);

    EndDrawing();
  }

  void Renderer::InitializeMeshes() {
    m_CubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_CubeModel = LoadModelFromMesh(m_CubeMesh);

    m_SphereMesh = GenMeshSphere(0.5f, 8, 8);
    m_SphereModel = LoadModelFromMesh(m_SphereMesh);
  }

  void Renderer::InitializeSkybox() {
    Image cubemap = CreateCubemapImage(
      "Assets/Textures/Skybox/right.png",
      "Assets/Textures/Skybox/left.png",
      "Assets/Textures/Skybox/top.png",
      "Assets/Textures/Skybox/bottom.png",
      "Assets/Textures/Skybox/front.png",
      "Assets/Textures/Skybox/back.png"
    );

    if (!cubemap.data) {
      CORE_ERROR("Failed to create cubemap image!");
      return;
    }

    m_SkyboxMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_SkyboxModel = LoadModelFromMesh(m_SkyboxMesh);

#if defined(PLATFORM_DESKTOP)
    const int GLSL_VERSION = 330;
#else
    const int GLSL_VERSION = 100;
#endif

    m_SkyboxModel.materials[0].shader = LoadShader(
      TextFormat("Assets/Shaders/Skybox/skybox.vert", GLSL_VERSION),
      TextFormat("Assets/Shaders/Skybox/skybox.frag", GLSL_VERSION)
    );

    int c = MATERIAL_MAP_CUBEMAP;
    SetShaderValue(m_SkyboxModel.materials[0].shader,
                   GetShaderLocation(m_SkyboxModel.materials[0].shader, "environmentMap"),
                   &c,
                   SHADER_UNIFORM_INT);

    m_SkyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(cubemap, CUBEMAP_LAYOUT_AUTO_DETECT);

    UnloadImage(cubemap);
  }

  void Renderer::DrawSkybox() {
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    Matrix matView = GetCameraMatrix(m_Camera);
    matView.m12 = 0;
    matView.m13 = 0;
    matView.m14 = 0;

    rlPushMatrix();
    DrawModel(m_SkyboxModel, Vector3Zero(), 1.0f, WHITE);
    rlPopMatrix();

    rlEnableBackfaceCulling();
    rlEnableDepthMask();
  }

  void Renderer::DrawSimplifiedGrid(uint32_t size) {
    constexpr uint32_t gridStep = 5;
    constexpr Color gridColor{ 40, 40, 40, 255 };

    for (uint32_t i = 0; i <= size; i += gridStep) {
      DrawLine3D(
        { static_cast<float>(i), 0.0f, 0.0f },
        { static_cast<float>(i), 0.0f, static_cast<float>(size) },
        gridColor
      );

      DrawLine3D(
        { 0.0f, 0.0f, static_cast<float>(i) },
        { static_cast<float>(size), 0.0f, static_cast<float>(i) },
        gridColor
      );
    }
  }

  void Renderer::DrawSectorGrid(const GameState& gameState) {
    constexpr Color SECTOR_COLOR{ 30, 30, 30, 100 };
    constexpr Color BOUNDING_BOX_COLOR{ 255, 0, 0, 200 };
    
    float mapSizeX = static_cast<float>(gameState.mapSize.x);
    float mapSizeY = static_cast<float>(gameState.mapSize.y);
    float mapSizeZ = static_cast<float>(gameState.mapSize.z);

    for (float x = 0; x < mapSizeX; x += SECTOR_SIZE) {
      for (float y = 0; y < mapSizeY; y += SECTOR_SIZE) {
        for (float z = 0; z < mapSizeZ; z += SECTOR_SIZE) {
          Vector3 position{ x + SECTOR_SIZE / 2.0f, y + SECTOR_SIZE / 2.0f, z + SECTOR_SIZE / 2.0f };
          if (IsCubeInFrustum(position, static_cast<float>(SECTOR_SIZE))) {
            DrawCubeWires(position, static_cast<float>(SECTOR_SIZE), static_cast<float>(SECTOR_SIZE), static_cast<float>(SECTOR_SIZE),
                          SECTOR_COLOR);
          }
        }
      }
    }

    Vector3 mapCenter{ mapSizeX / 2, mapSizeY / 2, mapSizeZ / 2 };
    DrawCubeWires(mapCenter, mapSizeX, mapSizeY, mapSizeZ, BOUNDING_BOX_COLOR);
  }

  void Renderer::RenderSpecialFood(const SpecialFood& specialFood) {
    for (const Coords& golden : specialFood.golden) {
      Vector3 position{
        static_cast<float>(golden.x),
        static_cast<float>(golden.y),
        static_cast<float>(golden.z)
      };

      if (IsPointInFrustum(position)) {
        DrawModelEx(m_SphereModel, position, { 0, 1, 0 }, 0.0f, { 1.0f, 1.0f, 1.0f }, GOLD);
      }
    }

    for (const Coords& suspicious : specialFood.suspicious) {
      Vector3 position{
        static_cast<float>(suspicious.x),
        static_cast<float>(suspicious.y),
        static_cast<float>(suspicious.z)
      };

      if (IsPointInFrustum(position)) {
        DrawModelEx(m_SphereModel, position, { 0, 1, 0 }, 0.0f, { 1.0f, 1.0f, 1.0f }, GREEN);
      }
    }
  }

  void Renderer::RenderSnakes(const std::vector<EnemySnake>& enemies, const std::vector<PlayerSnake>& players) {
    for (const EnemySnake& enemy : enemies) {
      if (enemy.status == "alive") {
        DrawSnakeOptimized(enemy.geometry, RED, false);
      }
    }

    for (const PlayerSnake& snake : players) {
      if (snake.status == "alive") {
        DrawSnakeOptimized(snake.geometry, BLUE, true);
      }
    }
  }

  void Renderer::DrawSnakeOptimized(const std::vector<Coords>& geometry, Color color, bool isPlayer) {
    if (geometry.empty()) { return; }

    Vector3 position{
      static_cast<float>(geometry.front().x),
      static_cast<float>(geometry.front().y),
      static_cast<float>(geometry.front().z)
    };

    // Draw head
    if (IsCubeInFrustum(position, 1.0f)) {
      DrawModelEx(m_CubeModel, position, { 0, 1, 0 }, 0.0f, { 1.0f, 1.0f, 1.0f }, color);
    }

    // Draw body segments with smaller scale and alpha
    Vector3 scale = { 0.8f, 0.8f, 0.8f };
    Color bodyColor = ColorAlpha(color, 0.8f);

    for (uint64_t i = 1; i < geometry.size(); ++i) {
      position = {
        static_cast<float>(geometry[i].x),
        static_cast<float>(geometry[i].y),
        static_cast<float>(geometry[i].z)
      };

      if (IsCubeInFrustum(position, 1.0f)) {
        DrawModelEx(m_CubeModel, position, { 0, 1, 0 }, 0.0f, scale, bodyColor);
      }
    }
  }

  void Renderer::DrawHUD(const GameState& gameState, Timestep deltaTime) {
    static char buffer[128];

    snprintf(buffer, sizeof(buffer), "Points: %d\nTurn: %d\nTime Remaining: %dms\nFPS: %d",
             gameState.points, gameState.turn, gameState.tickRemainMs, static_cast<uint32_t>(deltaTime.GetFramerate()));

    DrawText(buffer, 10, 10, 20, WHITE);
    DrawText("ESC - Exit | F2 - Show/Hide grid\nWASD - Move | Mouse - Rotate | Scroll - FOV", 10, GetScreenHeight() - 50, 20, WHITE);
  }

  void Renderer::UpdateFrustum() {
    Matrix matView = GetCameraMatrix(m_Camera);
    Matrix matProj = GetCameraProjectionMatrix(&m_Camera, static_cast<float>(GetScreenWidth()) / GetScreenHeight());
    Matrix matViewProj = MatrixMultiply(matView, matProj);

    // Left plane
    m_Frustum.planes[0].normal.x = matViewProj.m3 + matViewProj.m0;
    m_Frustum.planes[0].normal.y = matViewProj.m7 + matViewProj.m4;
    m_Frustum.planes[0].normal.z = matViewProj.m11 + matViewProj.m8;
    m_Frustum.planes[0].distance = matViewProj.m15 + matViewProj.m12;

    // Right plane
    m_Frustum.planes[1].normal.x = matViewProj.m3 - matViewProj.m0;
    m_Frustum.planes[1].normal.y = matViewProj.m7 - matViewProj.m4;
    m_Frustum.planes[1].normal.z = matViewProj.m11 - matViewProj.m8;
    m_Frustum.planes[1].distance = matViewProj.m15 - matViewProj.m12;

    // Top plane
    m_Frustum.planes[2].normal.x = matViewProj.m3 - matViewProj.m1;
    m_Frustum.planes[2].normal.y = matViewProj.m7 - matViewProj.m5;
    m_Frustum.planes[2].normal.z = matViewProj.m11 - matViewProj.m9;
    m_Frustum.planes[2].distance = matViewProj.m15 - matViewProj.m13;

    // Bottom plane
    m_Frustum.planes[3].normal.x = matViewProj.m3 + matViewProj.m1;
    m_Frustum.planes[3].normal.y = matViewProj.m7 + matViewProj.m5;
    m_Frustum.planes[3].normal.z = matViewProj.m11 + matViewProj.m9;
    m_Frustum.planes[3].distance = matViewProj.m15 + matViewProj.m13;

    // Near plane
    m_Frustum.planes[4].normal.x = matViewProj.m3 + matViewProj.m2;
    m_Frustum.planes[4].normal.y = matViewProj.m7 + matViewProj.m6;
    m_Frustum.planes[4].normal.z = matViewProj.m11 + matViewProj.m10;
    m_Frustum.planes[4].distance = matViewProj.m15 + matViewProj.m14;

    // Far plane
    m_Frustum.planes[5].normal.x = matViewProj.m3 - matViewProj.m2;
    m_Frustum.planes[5].normal.y = matViewProj.m7 - matViewProj.m6;
    m_Frustum.planes[5].normal.z = matViewProj.m11 - matViewProj.m10;
    m_Frustum.planes[5].distance = matViewProj.m15 - matViewProj.m14;

    // Normalize all planes
    for (uint32_t i = 0; i < 6; ++i) {
      float length = std::sqrt(
        m_Frustum.planes[i].normal.x * m_Frustum.planes[i].normal.x
        + m_Frustum.planes[i].normal.y * m_Frustum.planes[i].normal.y
        + m_Frustum.planes[i].normal.z * m_Frustum.planes[i].normal.z
      );

      m_Frustum.planes[i].normal.x /= length;
      m_Frustum.planes[i].normal.y /= length;
      m_Frustum.planes[i].normal.z /= length;
      m_Frustum.planes[i].distance /= length;
    }
  }

  bool Renderer::IsPointInFrustum(const Vector3& point) const noexcept {
    for (uint32_t i = 0; i < 6; ++i) {
      if (m_Frustum.planes[i].normal.x * point.x + m_Frustum.planes[i].normal.y * point.y
          + m_Frustum.planes[i].normal.z * point.z + m_Frustum.planes[i].distance <= 0) {
        return false;
      }
    }
    return true;
  }

  bool Renderer::IsCubeInFrustum(const Vector3& position, float size) const noexcept {
    float radius = size * 0.5f;
    for (uint32_t i = 0; i < 6; ++i) {
      const Plane& plane = m_Frustum.planes[i];
      float d = plane.normal.x * position.x + plane.normal.y * position.y + plane.normal.z * position.z;
      float r = radius * (std::fabs(plane.normal.x) + std::fabs(plane.normal.y) + std::fabs(plane.normal.z));
      if (d + r + plane.distance <= 0) { return false; }
    }
    return true;
  }

  Image Renderer::CreateCubemapImage(const char* rightPath, const char* leftPath, const char* topPath, const char* bottomPath,
                                     const char* frontPath, const char* backPath) {
    Image right = LoadImage(rightPath);
    Image left = LoadImage(leftPath);
    Image top = LoadImage(topPath);
    Image bottom = LoadImage(bottomPath);
    Image front = LoadImage(frontPath);
    Image back = LoadImage(backPath);

    // Verify all images were loaded
    if (!right.data && !left.data && !top.data && !bottom.data && !front.data && !back.data) {
      CORE_ERROR("Failed to load one or more cubemap faces!");
      return Image{ 0 };
    }

    // Verify all images are square and the same size
    if (right.width != right.height || right.width != left.width || right.width != top.width ||
        right.width != bottom.width || right.width != front.width || right.width != back.width) {
      ImageResize(&right, 1024, 1024);
      ImageResize(&left, 1024, 1024);
      ImageResize(&top, 1024, 1024);
      ImageResize(&bottom, 1024, 1024);
      ImageResize(&front, 1024, 1024);
      ImageResize(&back, 1024, 1024);
    }

    uint32_t faceSize = right.width;

    // Create image in cross layout (vertical cross)
    // Layout:
    //       [T]
    //    [L][F][R][B]
    //       [B]
    Image cubemap = GenImageColor(faceSize * 4, faceSize * 3, BLANK);

    ImageDraw(&cubemap, top, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ (float)faceSize, 0, (float)faceSize, (float)faceSize }, WHITE);

    ImageDraw(&cubemap, left, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ 0, (float)faceSize, (float)faceSize, (float)faceSize }, WHITE);

    ImageDraw(&cubemap, front, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ (float)faceSize, (float)faceSize, (float)faceSize, (float)faceSize }, WHITE);

    ImageDraw(&cubemap, right, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ (float)faceSize * 2, (float)faceSize, (float)faceSize, (float)faceSize }, WHITE);

    ImageDraw(&cubemap, back, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ (float)faceSize * 3, (float)faceSize, (float)faceSize, (float)faceSize }, WHITE);

    ImageDraw(&cubemap, bottom, Rectangle{ 0, 0, (float)faceSize, (float)faceSize },
      Rectangle{ (float)faceSize, (float)faceSize * 2, (float)faceSize, (float)faceSize }, WHITE);

    UnloadImage(right);
    UnloadImage(left);
    UnloadImage(top);
    UnloadImage(bottom);
    UnloadImage(front);
    UnloadImage(back);

    return cubemap;
  }

  void Renderer::SaveCubemapImage(const char* outputPath, const char* rightPath, const char* leftPath, const char* topPath,
                                  const char* bottomPath, const char* frontPath, const char* backPath) {
    Image cubemap = CreateCubemapImage(rightPath, leftPath, topPath, bottomPath, frontPath, backPath);
    if (cubemap.data) {
      ExportImage(cubemap, outputPath);
      CORE_INFO("Cubemap saved successfully to: {}!", outputPath);
      UnloadImage(cubemap);
    }
  }

  void Renderer::UnloadSkybox() {
    UnloadTexture(m_SkyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
    UnloadModel(m_SkyboxModel);
  }

  void Renderer::UpdateCamera(Timestep deltaTime) {
    Vector2 mouseDelta = GetMouseDelta();

    m_CameraAngleY += (mouseDelta.x * -m_MouseSpeed * DEG2RAD);
    m_CameraAngleX += (mouseDelta.y * -m_MouseSpeed * DEG2RAD);

    m_CameraAngleX = Clamp(m_CameraAngleX, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f);

    Vector3 forward = Vector3Normalize(Vector3Subtract(m_Camera.target, m_Camera.position));
    Vector3 right = Vector3CrossProduct(forward, m_Camera.up);

    if (IsKeyDown(KEY_W)) { m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(forward, m_MoveSpeed * deltaTime)); }
    if (IsKeyDown(KEY_S)) { m_Camera.position = Vector3Subtract(m_Camera.position, Vector3Scale(forward, m_MoveSpeed * deltaTime)); }
    if (IsKeyDown(KEY_D)) { m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(right, m_MoveSpeed * deltaTime)); }
    if (IsKeyDown(KEY_A)) { m_Camera.position = Vector3Subtract(m_Camera.position, Vector3Scale(right, m_MoveSpeed * deltaTime)); }

    float wheelMove = GetMouseWheelMove();
    m_Camera.fovy = Clamp(m_Camera.fovy - wheelMove * 5.0f, 20.0f, 120.0f);

    float cameraDistance = Vector3Length(Vector3Subtract(m_Camera.target, m_Camera.position));
    Vector3 newForward = {
      std::cos(m_CameraAngleX) * std::sin(m_CameraAngleY),
      std::sin(m_CameraAngleX),
      std::cos(m_CameraAngleX) * std::cos(m_CameraAngleY)
    };

    m_Camera.target = Vector3Add(m_Camera.position, Vector3Scale(newForward, cameraDistance));
  }
}