/*
 * Copyright (C) 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#if __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/OpenGL.h>
  #include <GLUT/glut.h>
#else
  #include <GL/glew.h>
  #include <GL/gl.h>
  #include <GL/glut.h>
#endif

#if not defined(__APPLE__) && not defined(_WIN32)
  #include <GL/glx.h>
#endif

#include <mutex>

#include <ignition/common/Console.hh>
#include <ignition/common/Filesystem.hh>

#include <ignition/rendering/Camera.hh>
#include <ignition/rendering/Image.hh>
#include <ignition/rendering/RayQuery.hh>
#include <ignition/rendering/RenderPass.hh>
#include <ignition/rendering/Scene.hh>
#include <ignition/rendering/OrbitViewController.hh>

#include "example_config.hh"
#include "GlutWindow.hh"

#define KEY_ESC 27
#define KEY_TAB  9

const std::string RESOURCE_PATH =
    ignition::common::joinPaths(std::string(PROJECT_BINARY_PATH), "media");

//////////////////////////////////////////////////
unsigned int imgw = 0;
unsigned int imgh = 0;

std::vector<ir::CameraPtr> g_cameras;
ir::CameraPtr g_camera;
ir::CameraPtr g_currCamera;
unsigned int g_cameraIndex = 0;
ir::ImagePtr g_image;

bool g_initContext = false;

#if __APPLE__
  CGLContextObj g_context;
  CGLContextObj g_glutContext;
#elif _WIN32
#else
  GLXContext g_context;
  Display *g_display;
  GLXDrawable g_drawable;
  GLXContext g_glutContext;
  Display *g_glutDisplay;
  GLXDrawable g_glutDrawable;
#endif

// view control variables
ir::RayQueryPtr g_rayQuery;
ir::OrbitViewController g_viewControl;
ir::RayQueryResult g_target;
struct mouseButton
{
  int button = 0;
  int state = GLUT_UP;
  int x = 0;
  int y = 0;
  int motionX = 0;
  int motionY = 0;
  int dragX = 0;
  int dragY = 0;
  int scroll = 0;
  bool buttonDirty = false;
  bool motionDirty = false;
};
struct mouseButton g_mouse;
std::mutex g_mouseMutex;

// pbr variables
std::mutex g_pbrMutex;
int g_pbrUpdate = 0;

// pbr variables
std::mutex g_lightMutex;
int g_lightUpdate = 0;



//////////////////////////////////////////////////
void mouseCB(int _button, int _state, int _x, int _y)
{
  // ignore unknown mouse button numbers
  if (_button >= 5)
    return;

  std::lock_guard<std::mutex> lock(g_mouseMutex);
  g_mouse.button = _button;
  g_mouse.state = _state;
  g_mouse.x = _x;
  g_mouse.y = _y;
  g_mouse.motionX = _x;
  g_mouse.motionY = _y;
  g_mouse.buttonDirty = true;
}

//////////////////////////////////////////////////
void motionCB(int _x, int _y)
{
  std::lock_guard<std::mutex> lock(g_mouseMutex);
  int deltaX = _x - g_mouse.motionX;
  int deltaY = _y - g_mouse.motionY;
  g_mouse.motionX = _x;
  g_mouse.motionY = _y;

  if (g_mouse.motionDirty)
  {
    g_mouse.dragX += deltaX;
    g_mouse.dragY += deltaY;
  }
  else
  {
    g_mouse.dragX = deltaX;
    g_mouse.dragY = deltaY;
  }
  g_mouse.motionDirty = true;
}

//////////////////////////////////////////////////
void handleMouse()
{
  std::lock_guard<std::mutex> lock(g_mouseMutex);
  // only ogre supports ray query for now so use
  // ogre camera located at camera index = 0.
  ir::CameraPtr rayCamera = g_cameras[0];
  if (!g_rayQuery)
  {
    g_rayQuery = rayCamera->Scene()->CreateRayQuery();
    if (!g_rayQuery)
    {
      ignerr << "Failed to create Ray Query" << std::endl;
      return;
    }
  }
  if (g_mouse.buttonDirty)
  {
    g_mouse.buttonDirty = false;

    // test mouse picking
    if (g_mouse.button == GLUT_LEFT_BUTTON && g_mouse.state == GLUT_DOWN)
    {
      // Get visual using Selection Buffer from Camera
      ir::VisualPtr visual;
      ignition::math::Vector2i mousePos(g_mouse.x, g_mouse.y);
      visual = rayCamera->VisualAt(mousePos);
      if (visual)
      {
        std::cout << "Selected visual at position: ";
        std::cout << g_mouse.x << " " << g_mouse.y << ": ";
        std::cout << visual->Name() << "\n";
      }
      else
      {
        std::cout << "No visual found at position: ";
        std::cout << g_mouse.x << " " << g_mouse.y << std::endl;
      }
    }

    // camera orbit
    double nx =
        2.0 * g_mouse.x / static_cast<double>(rayCamera->ImageWidth()) - 1.0;
    double ny = 1.0 -
        2.0 * g_mouse.y / static_cast<double>(rayCamera->ImageHeight());
    g_rayQuery->SetFromCamera(rayCamera, ignition::math::Vector2d(nx, ny));
    g_target  = g_rayQuery->ClosestPoint();
    if (!g_target)
    {
      // set point to be 10m away if no intersection found
      g_target.point = g_rayQuery->Origin() + g_rayQuery->Direction() * 10;
      return;
    }

    // mouse wheel scroll zoom
    if ((g_mouse.button == 3 || g_mouse.button == 4) &&
        g_mouse.state == GLUT_UP)
    {
      double scroll = (g_mouse.button == 3) ? -1.0 : 1.0;
      double distance = rayCamera->WorldPosition().Distance(
          g_target.point);
      int factor = 1;
      double amount = -(scroll * factor) * (distance / 5.0);
      for (ir::CameraPtr camera : g_cameras)
      {
        g_viewControl.SetCamera(camera);
        g_viewControl.SetTarget(g_target.point);
        g_viewControl.Zoom(amount);
      }
    }
  }

  if (g_mouse.motionDirty)
  {
    g_mouse.motionDirty = false;
    auto drag = ignition::math::Vector2d(g_mouse.dragX, g_mouse.dragY);

    // left mouse button pan
    if (g_mouse.button == GLUT_LEFT_BUTTON && g_mouse.state == GLUT_DOWN)
    {
      for (ir::CameraPtr camera : g_cameras)
      {
        g_viewControl.SetCamera(camera);
        g_viewControl.SetTarget(g_target.point);
        g_viewControl.Pan(drag);
      }
    }
    else if (g_mouse.button == GLUT_MIDDLE_BUTTON && g_mouse.state == GLUT_DOWN)
    {
      for (ir::CameraPtr camera : g_cameras)
      {
        g_viewControl.SetCamera(camera);
        g_viewControl.SetTarget(g_target.point);
        g_viewControl.Orbit(drag);
      }
    }
    // right mouse button zoom
    else if (g_mouse.button == GLUT_RIGHT_BUTTON && g_mouse.state == GLUT_DOWN)
    {
      double hfov = rayCamera->HFOV().Radian();
      double vfov = 2.0f * atan(tan(hfov / 2.0f) /
          rayCamera->AspectRatio());
      double distance = rayCamera->WorldPosition().Distance(
          g_target.point);
      double amount = ((-g_mouse.dragY /
          static_cast<double>(rayCamera->ImageHeight()))
          * distance * tan(vfov/2.0) * 6.0);
      for (ir::CameraPtr camera : g_cameras)
      {
        g_viewControl.SetCamera(camera);
        g_viewControl.SetTarget(g_target.point);
        g_viewControl.Zoom(amount);
      }
    }
  }
}

//////////////////////////////////////////////////
void updateLight()
{
  std::lock_guard<std::mutex> lock(g_lightMutex);

  static double t = 1;
  if (g_lightUpdate == 0)
  {
    return;
  }
  else if (g_lightUpdate == 1)
  {
    t = 1;
    g_lightUpdate = 0;
  }
  else if (g_lightUpdate == 2)
  {
    t += 0.005;
  }

  for (auto cam : g_cameras)
  {
    ir::ScenePtr scene = cam->Scene();
//    ir::LightPtr light = scene->LightByName("point");
//    light->SetLocalPosition(cos(t) * 3, sin(t) * 3, sin(t) * 0.5 + 2);
    auto light =
        std::dynamic_pointer_cast<ir::DirectionalLight>(scene->LightByName("dir"));
    light->SetDirection(cos(t) * 0.5, sin(t) * 0.5, -1);

  }
}

//////////////////////////////////////////////////
void updatePbr()
{
  std::lock_guard<std::mutex> lock(g_pbrMutex);
  if (g_pbrUpdate == 0)
    return;

  std::vector<std::string> meshes;
  meshes.push_back("drill");
  meshes.push_back("extinguisher");
  meshes.push_back("rescue_randy");
  meshes.push_back("valve");

  // create PBR material
  std::string environmentMap =
      ignition::common::joinPaths(RESOURCE_PATH, "fort_point.dds");
  for (auto mesh : meshes)
  {
    for (auto cam : g_cameras)
    {
      ir::ScenePtr scene = cam->Scene();
      ir::MaterialPtr matPBR = scene->Material(mesh);
      std::string textureMap = ignition::common::joinPaths(RESOURCE_PATH, mesh,
          mesh + "_albedo.png");
      std::string normalMap = ignition::common::joinPaths(RESOURCE_PATH, mesh,
          mesh + "_normal.png");
      std::string roughnessMap = ignition::common::joinPaths(RESOURCE_PATH, mesh,
          mesh + "_roughness.png");
      std::string metalnessMap = ignition::common::joinPaths(RESOURCE_PATH, mesh,
         mesh + "_metalness.png");


      if (g_pbrUpdate == 1)
      {
        if (matPBR->HasNormalMap())
          matPBR->ClearNormalMap();
        else
          matPBR->SetNormalMap(normalMap);
      }
      else if (g_pbrUpdate == 2)
      {
        if (matPBR->HasRoughnessMap())
          matPBR->ClearRoughnessMap();
        else
          matPBR->SetRoughnessMap(roughnessMap);
      }
      else if (g_pbrUpdate == 3)
      {
        if (matPBR->HasMetalnessMap())
          matPBR->ClearMetalnessMap();
        else
          matPBR->SetMetalnessMap(metalnessMap);
      }
      else if (g_pbrUpdate == 4)
      {
        if (matPBR->HasEnvironmentMap())
          matPBR->ClearEnvironmentMap();
        else if (mesh != "rescue_randy")
          matPBR->SetEnvironmentMap(environmentMap);
      }
      else if (g_pbrUpdate == 5)
      {
        matPBR->SetNormalMap(normalMap);
        matPBR->SetRoughnessMap(roughnessMap);
        matPBR->SetMetalnessMap(metalnessMap);
        if (mesh != "rescue_randy")
          matPBR->SetEnvironmentMap(environmentMap);
      }
      else if (g_pbrUpdate == 6)
      {
        matPBR->ClearNormalMap();
        matPBR->ClearRoughnessMap();
        matPBR->ClearMetalnessMap();
        matPBR->ClearEnvironmentMap();
      }

      ir::VisualPtr meshPBR = scene->VisualByName(mesh);
      meshPBR->SetMaterial(matPBR, false);
    }
  }
  g_pbrUpdate = 0;
}

//////////////////////////////////////////////////
void displayCB()
{
#if __APPLE__
  CGLSetCurrentContext(g_context);
#elif _WIN32
#else
  if (g_display)
  {
    glXMakeCurrent(g_display, g_drawable, g_context);
  }
#endif

  g_cameras[g_cameraIndex]->Capture(*g_image);
  handleMouse();

  updateLight();

  updatePbr();

#if __APPLE__
  CGLSetCurrentContext(g_glutContext);
#elif _WIN32
#else
  glXMakeCurrent(g_glutDisplay, g_glutDrawable, g_glutContext);
#endif

  unsigned char *data = g_image->Data<unsigned char>();

  glClearColor(0.5, 0.5, 0.5, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPixelZoom(1, -1);
  glRasterPos2f(-1, 1);
  glDrawPixels(imgw, imgh, GL_RGB, GL_UNSIGNED_BYTE, data);

  glutSwapBuffers();

  // Uncomment to print FPS
  // static ignition::common::Time previous;
  // auto now = ignition::common::Time::SystemTime();
  // std::cerr << (now - previous).Double() << std::endl;
  // previous = now;
}

//////////////////////////////////////////////////
void idleCB()
{
  glutPostRedisplay();
}

//////////////////////////////////////////////////
void keyboardCB(unsigned char _key, int, int)
{
  if (_key == KEY_ESC || _key == 'q' || _key == 'Q')
  {
    exit(0);
  }
  else if (_key == KEY_TAB)
  {
    g_cameraIndex = (g_cameraIndex + 1) % g_cameras.size();
  }
  else if (_key == 'p')
  {
    // toggle all render passes
    for (ir::CameraPtr camera : g_cameras)
    {
      for (unsigned int i = 0; i < camera->RenderPassCount(); ++i)
      {
        ir::RenderPassPtr pass = camera->RenderPassByIndex(i);
        pass->SetEnabled(!pass->IsEnabled());
      }
    }
  }
  else if (_key == 'l')
  {
    std::lock_guard<std::mutex> lock(g_lightMutex);
    if (g_lightUpdate <= 1)
      g_lightUpdate = 2;
    else
      g_lightUpdate = 1;
  }

  std::lock_guard<std::mutex> lock(g_pbrMutex);
  std::string k;
  k += _key;
  int key = -1;
  try
  {
    key = std::stoi(k);
  } catch (...)
  {
    return;
  }
  if (key >= 1 && key <= 6)
    g_pbrUpdate = key;
/*  else if (_key == '2')
    g_pbrUpdate = 2;
  else if (_key == '3')
    g_pbrUpdate = 3;
  else if (_key == '4')
    g_pbrUpdate = 4;
  else if (_key == '5')
    g_pbrUpdate = 5;
*/
  std::cerr <<  _key << std::endl;
}

//////////////////////////////////////////////////
void initCamera(ir::CameraPtr _camera)
{
  g_camera = _camera;
  imgw = g_camera->ImageWidth();
  imgh = g_camera->ImageHeight();
  ir::Image image = g_camera->CreateImage();
  g_image = std::make_shared<ir::Image>(image);
  g_camera->Capture(*g_image);
}

//////////////////////////////////////////////////
void initContext()
{
  glutInitDisplayMode(GLUT_DOUBLE);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(imgw, imgh);
  glutCreateWindow("Ogre2 Demo");
  glutDisplayFunc(displayCB);
  glutIdleFunc(idleCB);
  glutKeyboardFunc(keyboardCB);

  glutMouseFunc(mouseCB);
  glutMotionFunc(motionCB);
}

//////////////////////////////////////////////////
void printUsage()
{
  std::cout << "===============================" << std::endl;
  std::cout << "  TAB - Switch render engines  " << std::endl;
  std::cout << "  ESC - Exit                   " << std::endl;
  std::cout << "  P   - Toggle render pass     " << std::endl;
  std::cout << "===============================" << std::endl;
}

//////////////////////////////////////////////////
void run(std::vector<ir::CameraPtr> _cameras)
{
  if (_cameras.empty())
  {
    ignerr << "No cameras found. Scene will not be rendered" << std::endl;
    return;
  }

#if __APPLE__
  g_context = CGLGetCurrentContext();
#elif _WIN32
#else
  g_context = glXGetCurrentContext();
  g_display = glXGetCurrentDisplay();
  g_drawable = glXGetCurrentDrawable();
#endif

  g_cameras = _cameras;
  initCamera(_cameras[0]);
  initContext();
  printUsage();

#if __APPLE__
  g_glutContext = CGLGetCurrentContext();
#elif _WIN32
#else
  g_glutDisplay = glXGetCurrentDisplay();
  g_glutDrawable = glXGetCurrentDrawable();
  g_glutContext = glXGetCurrentContext();
#endif

  glutMainLoop();
}


