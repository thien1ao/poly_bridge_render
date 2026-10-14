#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "../headers/box.hpp"
#include "../headers/renderer.hpp"
#include "../headers/sphere.hpp"
#include "../headers/window.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <exception>
#include <iostream>

int main()
{
  glfwSetErrorCallback([](int code, const char* message) {
    std::cerr << "GLFW error " << code << ": " << message << '\n';
  });

  try {
    GLFWwindow* window = createWindow();

    {
      Renderer renderer;
      renderer.init(window);

      // scene sizes (world units), the bridge will later go between the pillar tops
      const float pillarWidth    = 1.2f;
      const float pillarHeight   = 3.0f;
      const float pillarDistance = 6.0f; // between pillar centers
      const float ballRadius     = 0.3f;

      const glm::vec3 pillarColor{ 0.6f, 0.6f, 0.6f };
      const glm::vec3 ballGreen  { 0.1f, 0.6f, 0.2f };
      const glm::vec3 ballWhite  { 1.0f, 1.0f, 1.0f };
      const uint32_t  ballSquares = 4; // 4 rows of squares, 8 around

      uint32_t sphereMesh = renderer.uploadMesh(createSphere(48, 24));
      uint32_t pillarMesh = renderer.uploadMesh(createBox(pillarWidth, pillarHeight, pillarWidth));

      const glm::vec3 firstPillarPos { -pillarDistance * 0.5f, 0.0f, 0.0f };
      const glm::vec3 secondPillarPos{  pillarDistance * 0.5f, 0.0f, 0.0f };

      renderer.addObject(pillarMesh, glm::translate(glm::mat4(1.0f), firstPillarPos), pillarColor);
      renderer.addObject(pillarMesh, glm::translate(glm::mat4(1.0f), secondPillarPos), pillarColor);

      // sphere mesh has radius 1: scale it down and put it on top of the first pillar
      glm::mat4 ball = glm::translate(glm::mat4(1.0f),
                                      firstPillarPos + glm::vec3(0.0f, pillarHeight + ballRadius, 0.0f));
      ball = glm::scale(ball, glm::vec3(ballRadius));
      renderer.addObject(sphereMesh, ball, ballGreen, ballWhite, ballSquares);

      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.drawFrame();
      }
    } // renderer is destroyed before the window

    glfwDestroyWindow(window);
    glfwTerminate();
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    glfwTerminate();
    return 1;
  }

  return 0;
}
