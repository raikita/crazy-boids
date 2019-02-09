/**
 * Author:	Andrew Robert Owens
 * Email:	arowens [at] ucalgary.ca
 * Date:	January, 2017
 * Course:	CPSC 587/687 Fundamental of Computer Animation
 * Organization: University of Calgary
 *
 * Copyright (c) 2017 - Please give credit to the author.
 *
 * File:	main.cpp
 *
 * Summary:
 *
 * This is a (very) basic program to
 * 1) load shaders from external files, and make a shader program
 * 2) make Vertex Array Object and Vertex Buffer Object for the quad
 *
 * take a look at the following sites for further readings:
 * opengl-tutorial.org -> The first triangle (New OpenGL, great start)
 * antongerdelan.net -> shaders pipeline explained
 * ogldev.atspace.co.uk -> good resource
 *
 * The actual animated stuff written by Rukiya Hassan
 *
 */

#include <iostream>
#include <cmath>
#include <chrono>
#include <limits>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include "Camera.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

//==================== GLOBAL VARIABLES ====================//
/*	Put here for simplicity. Feel free to restructure into
*	appropriate classes or abstractions.
*/

// Drawing Program
GLuint basicProgramID;

// Data needed for Quad
GLuint vaoID;
GLuint vertBufferID;
Mat4f M;

// Data needed for Line
GLuint line_vaoID;
GLuint line_vertBufferID;
Mat4f line_M;

// Only one camera so only one veiw and perspective matrix are needed.
Mat4f V;
Mat4f P;

// Only one thing is rendered at a time, so only need one MVP
// When drawing different objects, update M and MVP = M * V * P
Mat4f MVP;

// Camera and veiwing Stuff
Camera camera;
int g_moveUpDown = 0;
int g_moveLeftRight = 0;
int g_moveBackForward = 0;
int g_rotateLeftRight = 0;
int g_rotateUpDown = 0;
int g_rotateRoll = 0;
float g_rotationSpeed = 0.015625;
float g_panningSpeed = 0.25;
bool g_cursorLocked;
float g_cursorX, g_cursorY;

bool g_play = false;

int WIN_WIDTH = 800, WIN_HEIGHT = 600;
int FB_WIDTH = 800, FB_HEIGHT = 600;
float WIN_FOV = 60;
float WIN_NEAR = 0.01;
float WIN_FAR = 1000;

Vec3f place = Vec3f(0,0,0);
float pi = 3.1459265359;
int border, fov, fol;

bool followMouse = false;

//==================== FUNCTION DECLARATIONS ====================//
void displayFunc();
void resizeFunc();
void init();
void generateIDs();
void deleteIDs();
void setupVAO();
void loadQuadGeometryToGPU();
void reloadProjectionMatrix();
void loadModelViewMatrix();
void setupModelViewProjectionTransform();

void windowSetSizeFunc();
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowSetSizeFunc(GLFWwindow *window, int width, int height);
void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height);
void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void animateQuad(float t);
void moveCamera();
void reloadMVPUniform();
void reloadColorUniform(float r, float g, float b);
std::string GL_ERROR();
int main(int, char **);

//==================== FUNCTION DEFINITIONS ====================//

struct Boid {
  Vec3f position;
  Vec3f velocity;
  Vec3f heading;
  Vec3f v;
  bool predator;
  bool wall;
};

std::vector<Boid> boids;

void setupBoids(unsigned int numBoids, unsigned int numPreds) {
  float x, y, z;
  int dist = 100;
  int distbtwn = dist/2;
  for (unsigned i = 0; i < numBoids; i++) {
    Boid b;
    x = (rand()%20)-10;
    y = (rand()%20)-10;
    z = (rand()%20)-10;

    b.position = Vec3f(rand()%dist-distbtwn, rand()%dist-distbtwn, rand()%dist-distbtwn);
    b.v = Vec3f(0, 0, 0);
    b.velocity = Vec3f(x,y,z);
    b.predator = false;
    b.wall = false;

    boids.push_back(b);
  }

  for (unsigned i = 0; i < numPreds; i++) {
    Boid b;
    x = (rand()%20)-10;
    y = (rand()%20)-10;
    z = (rand()%20)-10;

    b.position = Vec3f(rand()%dist-distbtwn, rand()%dist-distbtwn, rand()%dist-distbtwn);
    b.velocity = Vec3f(x, y, z);
    b.v = Vec3f(0,0,0);
    b.predator = true;
    b.wall = false;

    boids.push_back(b);
  }

  for (signed i = -50; i < 50; i++) {
    Boid b;

    b.position = Vec3f(-border/2, i*10, -border/2);
    b.velocity = Vec3f(0, 0, 0);
    b.predator = false;
    b.wall = true;

    boids.push_back(b);
  }
}

void displayFunc() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(basicProgramID);

  // ===== DRAW QUAD ====== //
  MVP = P * V * M;
  reloadMVPUniform();
  reloadColorUniform(1, 0, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(vaoID);
  // Draw Quads, start at vertex 0, draw 4 of them (for a quad)
  glDrawArrays(GL_TRIANGLES, 0, 9*boids.size());

  // ==== DRAW LINE ===== //
  MVP = P * V * line_M;
  reloadMVPUniform();

  reloadColorUniform(0, 1, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(line_vaoID);
  // Draw lines
  glDrawArrays(GL_LINES, 0, 2*boids.size());

}

float length(Vec3f A, Vec3f B) {
  float x = A.x() - B.x();
  float y = A.y() - B.y();
  float z = A.z() - B.z();

  return sqrt(x*x + y*y + z*z);
}

float vecToScal(Vec3f a) {
  float x = a.x();
  float y = a.y();
  float z = a.z();

  return sqrt(x*x + y*y + z*z);
}

void boundaries(Vec3f p, int i) {
  float turn = 0.05;

  if (p.x() >= border)
    boids[i].velocity.x() += -turn;
  else if (p.x() < -border)
    boids[i].velocity.x() += turn;

  if (p.y() >= border)
    boids[i].velocity.y()  += -turn;
  else if (p.y() < -border)
    boids[i].velocity.y() += turn;

  if (p.z() >= border)
    boids[i].velocity.z() += -turn;
  else if (p.z() < -border)
    boids[i].velocity.z() += turn;

}

bool viewRange(int i, int j) {
  float phi = fov/2 * pi/180;

  Vec3f qmp = (boids[j].position - boids[i].position)/length(boids[j].position, boids[i].position);
  float hqp = vecToScal(boids[i].heading/vecToScal(boids[i].heading))*vecToScal(qmp)*cos(phi);


  if (hqp > cos(phi)) {
    //printf("seen\n");
    return true;
  }

  return false;
}


// make them be pulled into centre by a "force" when exit boundaries
void animateQuad(float t) {
  Vec3f avgVelocity, avgPos, avoVector, turnVector, mostDense, avgHeading;
  Vec3f direct;
  float distance = 0;
  float preyMaxSpeed = 1;
  float predMaxSpeed = 3;
  float speed = 0;
  int numNeighbours = 0;
  float avo = 15;
  //float fol = 100;
  bool seen = false;

  for (unsigned i = 0; i < boids.size(); i++) {
    avgPos = avgVelocity = avoVector = mostDense = Vec3f(0,0,0);

    for (unsigned j = 0; j < boids.size(); j++) {
      if (j != i) {
        mostDense += boids[j].position;
        distance = length(boids[j].position, boids[i].position);
        seen = viewRange(i, j);
        // if close enough, follow
        if (distance < fol && distance > avo && seen && !boids[j].predator && !boids[j].wall) {
          numNeighbours++;
          // calculate average position
          avgPos += boids[j].position;
          // follow, velocity matching
          avgVelocity += boids[j].velocity;
        }
        // avoid predator, predator is 10 time scarier than colliding w/ prey
        if (distance < avo * 10 && boids[j].predator && !boids[j].wall) {
          avoVector -= (boids[j].position - boids[i].position) / 25;
        }
        if (distance < avo && boids[j].wall) {
          avoVector -= (boids[j].position - boids[i].position)/10 ;
        }
        // avoid colliding into prey
        if (seen && distance < avo && !boids[j].predator && !boids[j].wall) {
          avoVector -= (boids[j].position - boids[i].position) / 50;
        }
      }
    } // end loop for j

    // have predators follow prey
    mostDense = ((mostDense/(boids.size()-1))-boids[i].position) / 200;

    // following mouse behaviour
    if (followMouse)
      direct = (place - boids[i].position) / 1000;  // directed by mouse movement
    else
      direct = Vec3f(0,0,0);

    // found another behaviour
    if (numNeighbours > 0) {
      avgPos = ((avgPos/numNeighbours)-boids[i].position) / 150;
      avgVelocity = ((avgVelocity/numNeighbours)-boids[i].velocity) / 8;

      avgHeading = (avgHeading/numNeighbours)-boids[i].heading;

      numNeighbours = 0;  // reset for next boid
    }
    boids[i].velocity += avgPos + avgVelocity + avoVector + direct;
    if (boids[i].predator)
      boids[i].velocity += avoVector + avgPos;
    // stay within boundaries
    boundaries(boids[i].position, i);

    // limit speed
    speed = vecToScal(boids[i].velocity);
    if (speed > preyMaxSpeed && !boids[i].predator && !boids[i].wall)
      boids[i].velocity = ((boids[i].velocity / speed) * preyMaxSpeed);
    else if (speed > predMaxSpeed && boids[i].predator && !boids[i].wall)
      boids[i].velocity = ((boids[i].velocity / speed) * predMaxSpeed);

    boids[i].v = boids[i].v + boids[i].velocity;
    // update movement, but not the walls
    if (!boids[i].wall)
      boids[i].position += boids[i].velocity;
  } // end loop for i
}

void loadQuadGeometryToGPU() {
  // Just basic layout of floats, for a quad
  // 3 floats per vertex, 4 vertices
  float width = 2;
  std::vector<Vec3f> verts;
/*
  verts.push_back(Vec3f(0*width+x, 1*width+y, 0*width+z));
  verts.push_back(Vec3f(1*width+x, 1*width+y, 0*width+z));
  verts.push_back(Vec3f(1*width+x, 0*width+y, 0*width+z));

  verts.push_back(Vec3f(0*width+x, 1*width+y, 0*width+z));
  verts.push_back(Vec3f(1*width+x, 0*width+y, 0*width+z));
  verts.push_back(Vec3f(0*width+x, 0*width+y, 0*width+z));
*/

  for (unsigned i = 0; i < boids.size(); i++) {
    if (boids[i].wall)
      width = 15;
    else if (boids[i].predator)
      width = 7;
    else
      width = 2;
    float x = boids[i].position.x();
    float y = boids[i].position.y();
    float z = boids[i].position.z();

    verts.push_back(Vec3f(0.5*width+x, 1.5*width+y, 0*width+z));
    verts.push_back(Vec3f(0.5*width+x, 0.5*width+y, 0.5*width+z));
    verts.push_back(Vec3f(0*width+x, 0*width+y, 0*width+z));

    verts.push_back(Vec3f(0*width+x, 0*width+y, 0*width+z));
    verts.push_back(Vec3f(0.5*width+x, 0.5*width+y, 0.5*width+z));
    verts.push_back(Vec3f(1*width+x, 0*width+y, 0*width+z));

    verts.push_back(Vec3f(1*width+x, 0*width+y, 0*width+z));
    verts.push_back(Vec3f(0.5*width+x, 0.5*width+y, 0.5*width+z));
    verts.push_back(Vec3f(0.5*width+x, 1.5*width+y, 0*width+z));
  }


  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * 9 * boids.size(), // byte size of Vec3f, 4 of them
               verts.data(),      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void loadLineGeometryToGPU() {
  float length = 5;
  // Just basic layout of floats, for a quad
  // 3 floats per vertex, 4 vertices
  Vec3f h;
  std::vector<Vec3f> verts;

  for (unsigned i = 0; i < boids.size(); i++) {

    float x = boids[i].position.x();
    float y = boids[i].position.y();
    float z = boids[i].position.z();
    h = (boids[i].velocity / vecToScal(boids[i].velocity))*length;
    boids[i].heading = h;

    if (!boids[i].wall) {
      verts.push_back(Vec3f(x,y,z));
      verts.push_back(Vec3f(h.x()+x,h.y()+y,h.z()+z));
    }
    else {
      verts.push_back(Vec3f(0,0,0));
      verts.push_back(Vec3f(0,0,0));
    }


  }

  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * 2 * boids.size(), // byte size of Vec3f, 4 of them
               verts.data(),      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer

}

void setupVAO() {
  glBindVertexArray(vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(line_vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(0); // reset to default
}

void reloadProjectionMatrix() {
  // Perspective Only

  // field of view angle 60 degrees
  // window aspect ratio
  // near Z plane > 0
  // far Z plane

  P = PerspectiveProjection(WIN_FOV, // FOV
                            static_cast<float>(WIN_WIDTH) /
                                WIN_HEIGHT, // Aspect
                            WIN_NEAR,       // near plane
                            WIN_FAR);       // far plane depth
}

void loadModelViewMatrix() {
  M = IdentityMatrix();
  line_M = IdentityMatrix();
  // view doesn't change, but if it did you would use this
  V = camera.lookatMatrix();
}

void reloadViewMatrix() { V = camera.lookatMatrix(); }

void setupModelViewProjectionTransform() {
  MVP = P * V * M; // transforms vertices from right to left (odd huh?)
}

void reloadMVPUniform() {
  GLint id = glGetUniformLocation(basicProgramID, "MVP");

  glUseProgram(basicProgramID);
  glUniformMatrix4fv(id,        // ID
                     1,         // only 1 matrix
                     GL_TRUE,   // transpose matrix, Mat4f is row major
                     MVP.data() // pointer to data in Mat4f
                     );
}

void reloadColorUniform(float r, float g, float b) {
  GLint id = glGetUniformLocation(basicProgramID, "inputColor");

  glUseProgram(basicProgramID);
  glUniform3f(id, // ID in basic_vs.glsl
              r, g, b);
}

void generateIDs() {
  // shader ID from OpenGL
  std::string vsSource = loadShaderStringfromFile("./shaders/basic_vs.glsl");
  std::string fsSource = loadShaderStringfromFile("./shaders/basic_fs.glsl");
  basicProgramID = CreateShaderProgram(vsSource, fsSource);

  // VAO and buffer IDs given from OpenGL
  glGenVertexArrays(1, &vaoID);
  glGenBuffers(1, &vertBufferID);
  glGenVertexArrays(1, &line_vaoID);
  glGenBuffers(1, &line_vertBufferID);
}

void deleteIDs() {
  glDeleteProgram(basicProgramID);

  glDeleteVertexArrays(1, &vaoID);
  glDeleteBuffers(1, &vertBufferID);
  glDeleteVertexArrays(1, &line_vaoID);
  glDeleteBuffers(1, &line_vertBufferID);
}

void init() {
  glEnable(GL_DEPTH_TEST);
  glPointSize(50);

  camera = Camera(Vec3f{0, 0, 500}, Vec3f{0, 0, -1}, Vec3f{0, 1, 0});


  int numBoids, numPrey;
  int input[5];
  int i = 0;
  string line;
  ifstream infile("parameters.txt");
  while(getline(infile,line) && i < 5) {
    input[i] = atoi(line.c_str());
    cout << input[i] << endl;
    i++;
  }
  infile.close();

  //int d = atoi(line.c_str());
  //cout << d << endl;
  numBoids = input[0];
  numPrey = input[1];
  fov = input[2];
  border = input[3];
  fol = input[4];

  setupBoids(numBoids, numPrey);

  // SETUP SHADERS, BUFFERS, VAOs

  generateIDs();
  setupVAO();
  loadQuadGeometryToGPU();
  loadLineGeometryToGPU();

  loadModelViewMatrix();
  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

int main(int argc, char **argv) {
  GLFWwindow *window;

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
      glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "CPSC 587 A4", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowSizeCallback(window, windowSetSizeFunc);
  glfwSetFramebufferSizeCallback(window, windowSetFramebufferSizeFunc);
  glfwSetKeyCallback(window, windowKeyFunc);
  glfwSetCursorPosCallback(window, windowMouseMotionFunc);
  glfwSetMouseButtonCallback(window, windowMouseButtonFunc);

  glfwGetFramebufferSize(window, &WIN_WIDTH, &WIN_HEIGHT);

  // Initialize glad
  if (!gladLoadGL()) {
    std::cerr << "Failed to initialise GLAD" << std::endl;
    return -1;
  }

  std::cout << "GL Version: :" << glGetString(GL_VERSION) << std::endl;
  std::cout << GL_ERROR() << std::endl;

  init(); // our own initialize stuff func

  float t = 0;
  float dt = 0.01;
  double xpos, ypos;

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         !glfwWindowShouldClose(window)) {


    if (g_play) {
      glfwGetCursorPos(window, &xpos, &ypos);
      t += dt;
      place.x() = xpos - WIN_WIDTH/2;
      place.y() = WIN_HEIGHT/2 - ypos;
      //printf("x = %f, y = %f\n", place.x(), place.y());
      animateQuad(t);
      loadQuadGeometryToGPU();
      loadLineGeometryToGPU();
    }

    displayFunc();
    moveCamera();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up after loop
  deleteIDs();

  return 0;
}

//==================== CALLBACK FUNCTIONS ====================//


void windowSetSizeFunc(GLFWwindow *window, int width, int height) {
  WIN_WIDTH = width;
  WIN_HEIGHT = height;

  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height) {
  FB_WIDTH = width;
  FB_HEIGHT = height;

  glViewport(0, 0, FB_WIDTH, FB_HEIGHT);
}

void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      g_cursorLocked = GL_TRUE;
    } else {
      g_cursorLocked = GL_FALSE;
    }
  }
}

void windowMouseMotionFunc(GLFWwindow *window, double x, double y) {
  if (g_cursorLocked) {
    float deltaX = (x - g_cursorX) * 0.01;
    float deltaY = (y - g_cursorY) * 0.01;
    camera.rotateAroundFocus(deltaX, deltaY);

    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }

  g_cursorX = x;
  g_cursorY = y;
}

void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods) {
  bool set = action != GLFW_RELEASE && GLFW_REPEAT;
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, GL_TRUE);
    break;
  case GLFW_KEY_W:
    g_moveBackForward = set ? 1 : 0;
    break;
  case GLFW_KEY_S:
    g_moveBackForward = set ? -1 : 0;
    break;
  case GLFW_KEY_A:
    g_moveLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_D:
    g_moveLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_Q:
    g_moveUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_E:
    g_moveUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_UP:
    g_rotateUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_DOWN:
    g_rotateUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_LEFT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? -1 : 0;
    else
      g_rotateLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_RIGHT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? 1 : 0;
    else
      g_rotateLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_SPACE:
    g_play = set ? !g_play : g_play;
    break;
  case GLFW_KEY_F:
    followMouse = set ? !followMouse : followMouse;
    break;
  case GLFW_KEY_LEFT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 0.5;
    } else {
      g_panningSpeed *= 0.5;
    }
    break;
  case GLFW_KEY_RIGHT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 1.5;
    } else {
      g_panningSpeed *= 1.5;
    }
    break;
  default:
    break;
  }
}

//==================== OPENGL HELPER FUNCTIONS ====================//

void moveCamera() {
  Vec3f dir;

  if (g_moveBackForward) {
    dir += Vec3f(0, 0, g_moveBackForward * g_panningSpeed);
  }
  if (g_moveLeftRight) {
    dir += Vec3f(g_moveLeftRight * g_panningSpeed, 0, 0);
  }
  if (g_moveUpDown) {
    dir += Vec3f(0, g_moveUpDown * g_panningSpeed, 0);
  }

  if (g_rotateUpDown) {
    camera.rotateUpDown(g_rotateUpDown * g_rotationSpeed);
  }
  if (g_rotateLeftRight) {
    camera.rotateLeftRight(g_rotateLeftRight * g_rotationSpeed);
  }
  if (g_rotateRoll) {
    camera.rotateRoll(g_rotateRoll * g_rotationSpeed);
  }

  if (g_moveUpDown || g_moveLeftRight || g_moveBackForward ||
      g_rotateLeftRight || g_rotateUpDown || g_rotateRoll) {
    camera.move(dir);
    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }
}

std::string GL_ERROR() {
  GLenum code = glGetError();

  switch (code) {
  case GL_NO_ERROR:
    return "GL_NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  default:
    return "Non Valid Error Code";
  }
}
