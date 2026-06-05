#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AStar.h"

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

float lastStepTime = 0.0f;
float stepDelay = 0.02f;
float lastKeyPressTime = 0.0f; 

unsigned int cubeVAO = 0, cubeVBO;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

unsigned int createShaderProgram() {
    const char* vShaderCode = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        out vec3 Normal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fShaderCode = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 Normal;
        
        uniform vec3 objectColor;
        uniform vec3 lightDir;
        
        void main() {
            float diff = max(dot(normalize(Normal), normalize(lightDir)), 0.0);
            vec3 lighting = (0.3 + diff * 0.7) * objectColor;
            FragColor = vec4(lighting, 1.0);
        }
    )";

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return shaderProgram;
}

void renderCube() {
    if (cubeVAO == 0) {
        float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
        };
        
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindVertexArray(cubeVAO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "A* Path-Finding OpenGL", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShaderProgram();
    
    Pathfinder pathfinder;
    pathfinder.resetGrid();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
            
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !pathfinder.isSearching && !pathfinder.pathFound) {
            pathfinder.startInstantSearchAndAnimate();
        }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !pathfinder.isResetting && currentFrame - lastKeyPressTime > 0.3f) {
            pathfinder.resetGrid();
            lastKeyPressTime = currentFrame;
        }

        if (pathfinder.isSearching && currentFrame - lastStepTime > stepDelay) {
            pathfinder.step();
            lastStepTime = currentFrame;
        }

        if (pathfinder.isResetting) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        float boardWidthWorld = std::max(1, pathfinder.mapWidth - 1) * 1.1f;
        float boardHeightWorld = std::max(1, pathfinder.mapHeight - 1) * 1.1f;
        float boardCenterX = boardWidthWorld * 0.5f;
        float boardCenterZ = boardHeightWorld * 0.5f;
        
        float maxSpan = std::max(boardWidthWorld, boardHeightWorld);
        float cameraHeight = std::max(20.0f, maxSpan * 0.85f + 8.0f);
        float cameraDepth = std::max(18.0f, maxSpan * 0.90f + 8.0f);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, std::max(100.0f, maxSpan * 6.0f));
        glm::mat4 view = glm::lookAt(
            glm::vec3(boardCenterX, cameraHeight, boardCenterZ + cameraDepth), 
            glm::vec3(boardCenterX, 0.0f, boardCenterZ),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightDir"), 0.5f, 1.0f, 0.3f);

        for (int x = 0; x < pathfinder.mapWidth; ++x) {
            for (int z = 0; z < pathfinder.mapHeight; ++z) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x * 1.1f, 0.0f, z * 1.1f));
                
                glm::vec3 color;
                float height = 1.0f;
                const Node& cell = pathfinder.grid[x][z];

                if (cell.type == WALL) {
                    color = glm::vec3(0.2f, 0.2f, 0.2f); 
                    height = 1.5f;                       
                } else if (x == pathfinder.startX && z == pathfinder.startZ) {
                    color = pathfinder.noPath ? glm::vec3(0.6f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.4f, 1.0f); 
                    height = 1.2f;
                } else if (x == pathfinder.endX && z == pathfinder.endZ) {
                    color = glm::vec3(1.0f, 0.0f, 0.0f); 
                    height = 1.2f;
                } else if (cell.state == PATH) {
                    color = glm::vec3(0.0f, 1.0f, 0.0f); 
                } else if (cell.isFinalPath) {
                    color = glm::vec3(0.2f, 0.6f, 0.4f); 
                    height = 0.5f;
                } else if (cell.state == OPEN || cell.state == CLOSED) {
                    if (pathfinder.noPath) {
                        color = glm::vec3(0.7f, 0.2f, 0.2f);
                    } else {
                        color = (cell.state == OPEN) ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.5f, 0.0f);
                    }
                } else {
                    color = glm::vec3(0.8f, 0.8f, 0.8f); 
                    height = 0.2f;                       
                }

                model = glm::scale(model, glm::vec3(1.0f, height, 1.0f));
                
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));
                
                renderCube();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}