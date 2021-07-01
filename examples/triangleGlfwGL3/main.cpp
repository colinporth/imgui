// main.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
  #pragma comment(lib, "legacy_stdio_definitions")
#endif

#include "imgui.h"
#include "log/cLog.h"

// opengl3 includes,defines
#if defined(IMGUI_IMPL_OPENGL_ES2)
  #include <GLES2/gl2.h>
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  #include <GLES3/gl3.h>
#else
  #include <GL/gl3w.h>
  #define IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
  #if defined(GL_VERSION_3_1)
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
  #endif
  #if defined(GL_VERSION_3_2)
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
  #endif
  #if defined(GL_VERSION_3_3)
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
  #endif
#endif

// glfw includes,defines
#include <GLFW/glfw3.h>

#ifdef _WIN32
  #undef APIENTRY
  #define GLFW_EXPOSE_NATIVE_WIN32
  #include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

#define GLFW_HAS_WINDOW_TOPMOST  (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED  (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA    (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorContentScale
#define GLFW_HAS_VULKAN          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwCreateWindowSurface

#ifdef GLFW_RESIZE_NESW_CURSOR  // let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
  #define GLFW_HAS_NEW_CURSORS   (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
  #define GLFW_HAS_NEW_CURSORS   (0)
#endif

// using namespace
using namespace std;
using namespace fmt;
//}}}

//{{{
class cPlatform {
public:
//{{{
static bool init() {

  cLog::log (LOGINFO, format ("GLFW version {}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR));

  glfwSetErrorCallback (errorCallback);
  if (!glfwInit())
    return false;

  // Create window with graphics context
  gWindow = glfwCreateWindow (1280, 720, "openGL3 testbed", NULL, NULL);
  if (!gWindow)
    return false;

  glfwMakeContextCurrent (gWindow);
  glfwSwapInterval (1); // Enable vsync

  glfwGetFramebufferSize (gWindow, &mScreenWidth, &mScreenHeight);

  gTime = 0.0;

  // GL 3.0 + GLSL 130
  //glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  //glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);

  return true;
  }
//}}}
//{{{
static void initIo() {

  // Setup backend capabilities flags
  ImGuiIO& io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos requests (optional, rarely used)
  io.BackendPlatformName = "imgui_impl_glfw";

  // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

  io.SetClipboardTextFn = setClipboardText;
  io.GetClipboardTextFn = getClipboardText;
  io.ClipboardUserData = gWindow;
  #if defined(_WIN32)
    io.ImeWindowHandle = (void*)glfwGetWin32Window (gWindow);
  #endif

  // Create mouse cursors
  // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
  // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
  // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
  GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
  gMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
  gMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
  gMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
  gMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
  gMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  #if GLFW_HAS_NEW_CURSORS
    gMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    gMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    gMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    gMouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
  #else
    gMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    gMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    gMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    gMouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
  #endif
  glfwSetErrorCallback(prev_error_callback);

  gPrevUserCallbackMousebutton = NULL;
  gPrevUserCallbackScroll = NULL;
  gPrevUserCallbackKey = NULL;
  gPrevUserCallbackChar = NULL;
  gPrevUserCallbackMousebutton = glfwSetMouseButtonCallback (gWindow, mouseButtonCallback);
  gPrevUserCallbackScroll = glfwSetScrollCallback (gWindow, scrollCallback);
  gPrevUserCallbackKey = glfwSetKeyCallback (gWindow, keyCallback);
  gPrevUserCallbackChar = glfwSetCharCallback (gWindow, charCallback);
  }
//}}}
//{{{
static void shutdown() {

  glfwSetMouseButtonCallback (gWindow, gPrevUserCallbackMousebutton);
  glfwSetScrollCallback (gWindow, gPrevUserCallbackScroll);
  glfwSetKeyCallback (gWindow, gPrevUserCallbackKey);
  glfwSetCharCallback (gWindow, gPrevUserCallbackChar);

  for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++) {
    glfwDestroyCursor (gMouseCursors[cursor_n]);
    gMouseCursors[cursor_n] = NULL;
    }

  glfwDestroyWindow (gWindow);

  glfwTerminate();
  }
//}}}

//{{{
static bool pollEvents() {

  if (glfwWindowShouldClose (gWindow))
    return false;
  else {
    glfwPollEvents();
    return true;
    }
  }
//}}}
//{{{
static void newFrame() {

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend");

  // Setup display size (every frame to accommodate for window resizing)
  int width;
  int height;
  glfwGetWindowSize (gWindow, &width, &height);

  int displayWidth;
  int displayHeight;
  glfwGetFramebufferSize (gWindow, &displayWidth, &displayHeight);

  io.DisplaySize = ImVec2 ((float)width, (float)height);
  if ((width > 0) && (height > 0))
    io.DisplayFramebufferScale = ImVec2 ((float)displayWidth / width, (float)displayHeight / height);

  // Setup time step
  double current_time = glfwGetTime();
  io.DeltaTime = gTime > 0.0 ? (float)(current_time - gTime) : (float)(1.0f / 60.0f);
  gTime = current_time;

  updateMousePosAndButtons();
  updateMouseCursor();
  }
//}}}
//{{{
static void present() {

  // present
  int displayWidth;
  int displayHeight;
  glfwGetFramebufferSize (gWindow, &displayWidth, &displayHeight);
  glViewport (0, 0, displayWidth, displayHeight);

  glfwSwapBuffers (gWindow);
  }
//}}}

static int getScreenWidth() { return mScreenWidth; }
static int getScreenHeight() { return mScreenHeight; }

private:
  //{{{
  static void errorCallback (int error, const char* description) {
    cLog::log (LOGERROR, format ("glfw error {} {}",  error, description));
    }
  //}}}
  //{{{
  static const char* getClipboardText (void* user_data) {
    return glfwGetClipboardString ((GLFWwindow*)user_data);
    }
  //}}}
  //{{{
  static void setClipboardText (void* user_data, const char* text) {
    glfwSetClipboardString((GLFWwindow*)user_data, text);
    }
  //}}}

  //{{{
  static void mouseButtonCallback (GLFWwindow* window, int button, int action, int mods) {

    if (gPrevUserCallbackMousebutton != NULL)
      gPrevUserCallbackMousebutton (window, button, action, mods);

    if (action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(gMouseJustPressed))
      gMouseJustPressed[button] = true;
    }
  //}}}
  //{{{
  static void scrollCallback (GLFWwindow* window, double xoffset, double yoffset) {

    if (gPrevUserCallbackScroll != NULL)
      gPrevUserCallbackScroll(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
    }
  //}}}
  //{{{
  static void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (gPrevUserCallbackKey != NULL)
      gPrevUserCallbackKey(window, key, scancode, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown)) {
      if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
      if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;
      }

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    #ifdef _WIN32
      io.KeySuper = false;
    #else
      io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    #endif
    }
  //}}}
  //{{{
  static void charCallback (GLFWwindow* window, unsigned int c) {

    if (gPrevUserCallbackChar != NULL)
      gPrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
    }
  //}}}

  //{{{
  static void updateMousePosAndButtons() {

    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE (io.MouseDown); i++) {
      // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
      io.MouseDown[i] = gMouseJustPressed[i] || glfwGetMouseButton (gWindow, i) != 0;
      gMouseJustPressed[i] = false;
      }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    const bool focused = glfwGetWindowAttrib (gWindow, GLFW_FOCUSED) != 0;
    if (focused) {
      if (io.WantSetMousePos) {
        glfwSetCursorPos (gWindow, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        }
      else {
        double mouse_x, mouse_y;
        glfwGetCursorPos (gWindow, &mouse_x, &mouse_y);
        io.MousePos = ImVec2 ((float)mouse_x, (float)mouse_y);
        }
      }
    }
  //}}}
  //{{{
  static void updateMouseCursor() {

    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) ||
        glfwGetInputMode (gWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
      return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
      // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
      glfwSetInputMode (gWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else {
      // Show OS mouse cursor
      // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
      glfwSetCursor (gWindow, gMouseCursors[imgui_cursor] ? gMouseCursors[imgui_cursor] : gMouseCursors[ImGuiMouseCursor_Arrow]);
      glfwSetInputMode (gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
    }
  //}}}

  inline static GLFWwindow* gWindow = NULL;    // Main window
  inline static double gTime = 0.0;
  inline static bool gMouseJustPressed[ImGuiMouseButton_COUNT] = {};
  inline static GLFWcursor* gMouseCursors[ImGuiMouseCursor_COUNT] = {};

  inline static GLFWmousebuttonfun gPrevUserCallbackMousebutton = NULL;
  inline static GLFWscrollfun gPrevUserCallbackScroll = NULL;
  inline static GLFWkeyfun gPrevUserCallbackKey = NULL;
  inline static GLFWcharfun gPrevUserCallbackChar = NULL;

  inline static int mScreenWidth = 0;
  inline static int mScreenHeight = 0;
  };
//}}}
//{{{
class cGraphics {
public:
  //{{{
  bool init (int width, int height) {

    if (gl3wInit())
      return false;

    glViewport (0, 0, width, height);

    // Query for GL version (e.g. 320 for GL 3.2)
    const char* glVersion = (const char*)glGetString (GL_VERSION);
    #if defined(IMGUI_IMPL_OPENGL_ES2)
      mGlVersionNum = 200; // GLES 2
      cLog::log (LOGINFO, "forcing openGLES version 2");
    #else
      GLint major = 0;
      glGetIntegerv (GL_MAJOR_VERSION, &major);
      GLint minor = 0;
      glGetIntegerv (GL_MINOR_VERSION, &minor);
      if (major == 0 && minor == 0)
        sscanf (glVersion, "%d.%d", &major, &minor);
      mGlVersionNum = (GLuint)(major * 100 + minor * 10);
    #endif

    cLog::log (LOGINFO, format ("OpenGL version {} parsed as {}", glVersion, mGlVersionNum));
    string glslVersion = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
    cLog::log (LOGINFO, format ("GLSL version {}", glslVersion));
    string glRenderer = (const char*)glGetString (GL_RENDERER);
    cLog::log (LOGINFO, format ("Renderer - {}", glRenderer));
    string glVendor = (const char*)glGetString (GL_VENDOR);
    cLog::log (LOGINFO, format ("Vendor - {}", glVendor));

    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl";

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
      if (mGlVersionNum >= 320) {
        // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        cLog::log (LOGINFO, "- openGL > 3.2 has vtxOffset");
        }
    #endif

    gHasClipOrigin = mGlVersionNum >= 450;
    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
      GLint num_extensions = 0;
      glGetIntegerv (GL_NUM_EXTENSIONS, &num_extensions);
      for (GLint i = 0; i < num_extensions; i++) {
        auto extension = (const char*)glGetStringi (GL_EXTENSIONS, i);
        if ((extension != NULL) && strcmp (extension, "GL_ARB_clip_control") == 0) {
          gHasClipOrigin = true;
          cLog::log (LOGINFO, "- openGL has GL_ARB_clip_control clipOrigin");
          }
        }
    #endif

    // glsl versionString, number
    #if defined(IMGUI_IMPL_OPENGL_ES2)
      mGlslVersionString = "#version 100\n";
      mGlslVersionNum = 100;
    #elif defined(IMGUI_IMPL_OPENGL_ES3)
      mGlslVersionString = "#version 300 es\n";
      mGlslVersionNum = 300;
    #else
      mGlslVersionString = "#version 130\n";
      mGlslVersionNum = 130;
    #endif
    cLog::log (LOGINFO, format ("GLSL using {}", mGlslVersionNum));

    // Make an arbitrary GL call (we don't actually need the result)
    GLint current_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &current_texture);

    return true;
    }
  //}}}
  //{{{
  void shutdown() {
    destroyDeviceObjects();
    }
  //}}}

  //{{{
  void clear() {
    glClearColor (0.45f, 0.55f, 0.6f, 1.f);
    glClear (GL_COLOR_BUFFER_BIT);
    }
  //}}}
  //{{{
  void newFrame() {

    if (!gShaderHandle)
      createDeviceObjects();
    }
  //}}}
  //{{{
  void draw (ImDrawData* draw_data) {
  //  Render function.
  // implementation is overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
  // This is in order to be able to run within an OpenGL engine that doesn't do so.

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
      return;

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
    glActiveTexture (GL_TEXTURE0);

    GLuint last_program;
    glGetIntegerv (GL_CURRENT_PROGRAM, (GLint*)&last_program);

    GLuint last_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, (GLint*)&last_texture);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
      GLuint last_sampler;
      if (mGlVersionNum >= 330)
        glGetIntegerv (GL_SAMPLER_BINDING, (GLint*)&last_sampler);
      else {
        last_sampler = 0;
        }
    #endif

    GLuint last_array_buffer;
    glGetIntegerv (GL_ARRAY_BUFFER_BINDING, (GLint*)&last_array_buffer);

    #ifndef IMGUI_IMPL_OPENGL_ES2
      GLuint last_vertex_array_object;
      glGetIntegerv (GL_VERTEX_ARRAY_BINDING, (GLint*)&last_vertex_array_object);
    #endif

    #ifdef GL_POLYGON_MODE
      GLint last_polygon_mode[2];
      glGetIntegerv (GL_POLYGON_MODE, last_polygon_mode);
    #endif

    GLint last_viewport[4];
    glGetIntegerv (GL_VIEWPORT, last_viewport);

    GLint last_scissor_box[4];
    glGetIntegerv (GL_SCISSOR_BOX, last_scissor_box);

    GLenum last_blend_src_rgb;
    glGetIntegerv (GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);

    GLenum last_blend_dst_rgb;
    glGetIntegerv (GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);

    GLenum last_blend_src_alpha;
    glGetIntegerv (GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);

    GLenum last_blend_dst_alpha;
    glGetIntegerv (GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);

    GLenum last_blend_equation_rgb;
    glGetIntegerv (GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);

    GLenum last_blend_equation_alpha;
    glGetIntegerv (GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);

    GLboolean last_enable_blend = glIsEnabled (GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled (GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled (GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled (GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled (GL_SCISSOR_TEST);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
      GLboolean last_enable_primitive_restart = (mGlVersionNum >= 310) ?
                                                  glIsEnabled (GL_PRIMITIVE_RESTART) : GL_FALSE;
    #endif

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to
    // - VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound
    // - but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
    #ifndef IMGUI_IMPL_OPENGL_ES2
      glGenVertexArrays (1, &vertex_array_object);
    #endif

    setupRenderState (draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];

      // Upload vertex/index buffers
      glBufferData (GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert),
                    (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
      glBufferData (GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx),
                    (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback != NULL) {
          // User callback, registered via ImDrawList::AddCallback()
          // ImDrawCallback_ResetRenderState is a special callback value to request reset render state
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            setupRenderState (draw_data, fb_width, fb_height, vertex_array_object);
          else
            pcmd->UserCallback (cmd_list, pcmd);
          }
        else {
          // Project scissor/clipping rectangles into framebuffer space
          ImVec4 clip_rect;
          clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
          clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
          clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
          clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

          if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
            // Apply scissor/clipping rectangle
            glScissor ((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

            // Bind texture, Draw
            glBindTexture (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
            #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
              if (mGlVersionNum >= 320)
                glDrawElementsBaseVertex (GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                          sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                          (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
              else
            #endif
                glDrawElements (GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
            }
          }
        }
      }

    // Destroy the temporary VAO
    #ifndef IMGUI_IMPL_OPENGL_ES2
      glDeleteVertexArrays (1, &vertex_array_object);
    #endif

    // Restore modified GL state
    glUseProgram (last_program);
    glBindTexture (GL_TEXTURE_2D, last_texture);
    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
      if (mGlVersionNum >= 330)
        glBindSampler (0, last_sampler);
    #endif
    glActiveTexture (last_active_texture);

    #ifndef IMGUI_IMPL_OPENGL_ES2
      glBindVertexArray (last_vertex_array_object);
    #endif

    glBindBuffer (GL_ARRAY_BUFFER, last_array_buffer);
    glBlendEquationSeparate (last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate (last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend)
      glEnable (GL_BLEND);
    else
      glDisable (GL_BLEND);

    if (last_enable_cull_face)
      glEnable (GL_CULL_FACE);
    else
      glDisable (GL_CULL_FACE);

    if (last_enable_depth_test)
      glEnable (GL_DEPTH_TEST);
    else
      glDisable (GL_DEPTH_TEST);

    if (last_enable_stencil_test)
      glEnable (GL_STENCIL_TEST);
    else
      glDisable (GL_STENCIL_TEST);

    if (last_enable_scissor_test)
      glEnable (GL_SCISSOR_TEST);
    else
      glDisable (GL_SCISSOR_TEST);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
      if (mGlVersionNum >= 310) {
        if (last_enable_primitive_restart)
          glEnable (GL_PRIMITIVE_RESTART);
        else
          glDisable (GL_PRIMITIVE_RESTART);
        }
    #endif

    #ifdef GL_POLYGON_MODE
      glPolygonMode (GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
    #endif

    glViewport (last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor (last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    }
  //}}}

private:
  //{{{
  bool checkShader (GLuint handle, const char* desc) {

    GLint status = 0, log_length = 0;
    glGetShaderiv (handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv (handle, GL_INFO_LOG_LENGTH, &log_length);

    if ((GLboolean)status == GL_FALSE)
      cLog::log (LOGERROR, format ("checkShader failed to compile {}", desc));

    if (log_length > 1) {
      ImVector<char> buf;
      buf.resize ((int)(log_length + 1));
      glGetShaderInfoLog (handle, log_length, NULL, (GLchar*)buf.begin());
      cLog::log (LOGERROR, buf.begin());
      }

    return (GLboolean)status == GL_TRUE;
    }
  //}}}
  //{{{
  bool checkProgram (GLuint handle, const char* desc) {

    GLint status = 0;
    GLint log_length = 0;
    glGetProgramiv (handle, GL_LINK_STATUS, &status);
    glGetProgramiv (handle, GL_INFO_LOG_LENGTH, &log_length);

    if ((GLboolean)status == GL_FALSE)
      cLog::log (LOGERROR, format ("CreateDeviceObjects failed to link {} {}", desc, mGlslVersionString));

    if (log_length > 1) {
      ImVector<char> buf;
      buf.resize ((int)(log_length + 1));
      glGetProgramInfoLog (handle, log_length, NULL, (GLchar*)buf.begin());
      cLog::log (LOGERROR, buf.begin());
      }

    return (GLboolean)status == GL_TRUE;
    }
  //}}}

  //{{{
  bool createFontsTexture() {

    // Build texture atlas
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small)
    // because it is more likely to be compatible with user's existing shaders
    // If your ImTextureId represent a higher-level concept than just a GL texture id,
    // consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures (1, &gFontTexture);
    glBindTexture (GL_TEXTURE_2D, gFontTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    #ifdef GL_UNPACK_ROW_LENGTH
      glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    #endif
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->SetTexID ((ImTextureID)(intptr_t)gFontTexture);

    // Restore state
    glBindTexture (GL_TEXTURE_2D, last_texture);

    return true;
    }
  //}}}
  //{{{
  void destroyFontsTexture() {

    if (gFontTexture) {
      ImGuiIO& io = ImGui::GetIO();
      glDeleteTextures (1, &gFontTexture);
      io.Fonts->SetTexID (0);
      gFontTexture = 0;
      }
    }
  //}}}

  //{{{
  bool createDeviceObjects() {

    // Backup GL state
    GLint last_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &last_texture);

    GLint last_array_buffer;
    glGetIntegerv (GL_ARRAY_BUFFER_BINDING, &last_array_buffer);

    #ifndef IMGUI_IMPL_OPENGL_ES2
      GLint last_vertex_array;
      glGetIntegerv (GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    #endif

    // Parse GLSL version string
    mGlslVersionNum = 130;
    sscanf (mGlslVersionString.c_str(), "#version %d", &mGlslVersionNum);

    // Select shaders matching our GLSL versions
    const GLchar* vertex_shader = NULL;
    const GLchar* fragment_shader = NULL;
    if (mGlslVersionNum < 130) {
      cLog::log (LOGINFO, "ImGui using glsl 120");
      vertex_shader = vertex_shader_glsl_120;
      fragment_shader = fragment_shader_glsl_120;
      }
    else if (mGlslVersionNum == 300) {
      cLog::log (LOGINFO, "GLSL ImGui using 300_es");
      vertex_shader = vertex_shader_glsl_300_es;
      fragment_shader = fragment_shader_glsl_300_es;
      }
    else if (mGlslVersionNum >= 410) {
      cLog::log (LOGINFO, "GLSL ImGui using 410_core");
      vertex_shader = vertex_shader_glsl_410_core;
      fragment_shader = fragment_shader_glsl_410_core;
      }
    else {
      cLog::log (LOGINFO, "GLSL ImGui using 130");
      vertex_shader = vertex_shader_glsl_130;
      fragment_shader = fragment_shader_glsl_130;
      }

    // Create shaders
    const GLchar* vertex_shader_with_version[2] = { mGlslVersionString.c_str(), vertex_shader };
    gVertHandle = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (gVertHandle, 2, vertex_shader_with_version, NULL);
    glCompileShader (gVertHandle);
    checkShader (gVertHandle, "vertex shader");

    const GLchar* fragment_shader_with_version[2] = { mGlslVersionString.c_str(), fragment_shader };
    gFragHandle = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (gFragHandle, 2, fragment_shader_with_version, NULL);
    glCompileShader (gFragHandle);
    checkShader (gFragHandle, "fragment shader");

    gShaderHandle = glCreateProgram();
    glAttachShader (gShaderHandle, gVertHandle);
    glAttachShader (gShaderHandle, gFragHandle);
    glLinkProgram (gShaderHandle);
    checkProgram (gShaderHandle, "shader program");

    gAttribLocationTex = glGetUniformLocation (gShaderHandle, "Texture");
    gAttribLocationProjMtx = glGetUniformLocation (gShaderHandle, "ProjMtx");
    gAttribLocationVtxPos = (GLuint)glGetAttribLocation (gShaderHandle, "Position");
    gAttribLocationVtxUV = (GLuint)glGetAttribLocation (gShaderHandle, "UV");
    gAttribLocationVtxColor = (GLuint)glGetAttribLocation (gShaderHandle, "Color");

    // Create buffers
    glGenBuffers (1, &gVboHandle);
    glGenBuffers (1, &gElementsHandle);

    createFontsTexture();

    // Restore modified GL state
    glBindTexture (GL_TEXTURE_2D, last_texture);
    glBindBuffer (GL_ARRAY_BUFFER, last_array_buffer);

    #ifndef IMGUI_IMPL_OPENGL_ES2
      glBindVertexArray (last_vertex_array);
    #endif

    return true;
    }
  //}}}
  //{{{
  void destroyDeviceObjects() {

    if (gVboHandle) {
      glDeleteBuffers (1, &gVboHandle);
      gVboHandle = 0;
      }

    if (gElementsHandle) {
      glDeleteBuffers (1, &gElementsHandle);
      gElementsHandle = 0;
      }
    if (gShaderHandle && gVertHandle)
      glDetachShader (gShaderHandle, gVertHandle);

    if (gShaderHandle && gFragHandle)
      glDetachShader (gShaderHandle, gFragHandle);

    if (gVertHandle) {
      glDeleteShader (gVertHandle);
      gVertHandle = 0;
      }

    if (gFragHandle) {
      glDeleteShader (gFragHandle);
      gFragHandle = 0;
      }

    if (gShaderHandle) {
      glDeleteProgram (gShaderHandle);
      gShaderHandle = 0;
      }

    destroyFontsTexture();
    }
  //}}}

  //{{{
  void setupRenderState (ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object) {

    // Set renderState - alpha-blending enabled, scissor enabled, polygon fill, no face culling, no depth testing
    glEnable (GL_BLEND);
    glEnable (GL_SCISSOR_TEST);
    glBlendEquation (GL_FUNC_ADD);
    glBlendFuncSeparate (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_STENCIL_TEST);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
      if (mGlVersionNum >= 310)
        glDisable (GL_PRIMITIVE_RESTART);
    #endif

    #ifdef GL_POLYGON_MODE
      glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    #endif

      // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
    #if defined(GL_CLIP_ORIGIN)
      bool clip_origin_lower_left = true;
      if (gHasClipOrigin) {
        GLenum current_clip_origin = 0;
        glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&current_clip_origin);
        if (current_clip_origin == GL_UPPER_LEFT)
          clip_origin_lower_left = false;
        }
    #endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glViewport (0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    #if defined(GL_CLIP_ORIGIN)
      if (!clip_origin_lower_left) {
        //{{{  Swap top and bottom if origin is upper left
        float tmp = T;
        T = B;
        B = tmp;
        }
        //}}}
    #endif

    const float ortho_projection[4][4] = { { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
                                           { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
                                           { 0.0f,         0.0f,        -1.0f,   0.0f },
                                           { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
                                         };

    glUseProgram (gShaderHandle);
    glUniform1i (gAttribLocationTex, 0);
    glUniformMatrix4fv (gAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
      if (mGlVersionNum >= 330)
        // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
        glBindSampler (0, 0);
    #endif

    (void)vertex_array_object;
    #ifndef IMGUI_IMPL_OPENGL_ES2
      glBindVertexArray (vertex_array_object);
    #endif

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glBindBuffer (GL_ARRAY_BUFFER, gVboHandle);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, gElementsHandle);
    glEnableVertexAttribArray (gAttribLocationVtxPos);
    glEnableVertexAttribArray (gAttribLocationVtxUV);
    glEnableVertexAttribArray (gAttribLocationVtxColor);
    glVertexAttribPointer (gAttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE,
                           sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer (gAttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE,
                           sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer (gAttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                           sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
    }
  //}}}

  //{{{  shaders 130
  const GLchar* vertex_shader_glsl_130 =
    "uniform mat4 ProjMtx;"
    "in vec2 Position;"
    "in vec2 UV;"
    "in vec4 Color;"
    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"
    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }"
    "\n";

  const GLchar* fragment_shader_glsl_130 =
    "uniform sampler2D Texture;"
    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"
    "out vec4 Out_Color;"
    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }"
    "\n";
  //}}}
  //{{{  shaders 120
  const GLchar* vertex_shader_glsl_120 =
    "uniform mat4 ProjMtx;"
    "attribute vec2 Position;"
    "attribute vec2 UV;"
    "attribute vec4 Color;"
    "varying vec2 Frag_UV;"
    "varying vec4 Frag_Color;"
    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }"
    "\n";

  const GLchar* fragment_shader_glsl_120 =
    "#ifdef GL_ES\n"
    "  precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D Texture;"
    "varying vec2 Frag_UV;"
    "varying vec4 Frag_Color;"
    "void main() {"
    "  gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);"
    "  }"
    "\n";
  //}}}
  //{{{  shaders 300_es
  const GLchar* vertex_shader_glsl_300_es =
    "precision mediump float;"
    "layout (location = 0) in vec2 Position;"
    "layout (location = 1) in vec2 UV;"
    "layout (location = 2) in vec4 Color;"
    "uniform mat4 ProjMtx;"
    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"
    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }"
    "\n";

  const GLchar* fragment_shader_glsl_300_es =
    "precision mediump float;"
    "uniform sampler2D Texture;"
    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"
    "layout (location = 0) out vec4 Out_Color;"
    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }"
    "\n";
  //}}}
  //{{{  shaders 410_core
  const GLchar* vertex_shader_glsl_410_core =
    "layout (location = 0) in vec2 Position;"
    "layout (location = 1) in vec2 UV;"
    "layout (location = 2) in vec4 Color;"
    "uniform mat4 ProjMtx;"
    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"
    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }"
    "\n";

  const GLchar* fragment_shader_glsl_410_core =
    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"
    "uniform sampler2D Texture;"
    "layout (location = 0) out vec4 Out_Color;"
    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }"
    "\n";
  //}}}

  string mGlVersionString;
  GLuint mGlVersionNum = 0;
  string mGlslVersionString;
  GLuint mGlslVersionNum = 0;

  GLuint gFontTexture = 0;
  GLuint gShaderHandle = 0;
  GLuint gVertHandle = 0;
  GLuint gFragHandle = 0;

  GLint gAttribLocationTex = 0;
  GLint gAttribLocationProjMtx = 0;
  GLuint gAttribLocationVtxPos = 0;
  GLuint gAttribLocationVtxUV = 0;
  GLuint gAttribLocationVtxColor = 0; // Vertex attributes location

  unsigned int gVboHandle = 0;
  unsigned int gElementsHandle = 0;

  bool gHasClipOrigin = false;
  };
//}}}
//{{{
class cShader {
public:
  cShader() {}
  //{{{
  void init (const string& vertexCode, const string& fragmentCode) {

    mVertexCode = vertexCode;
    mFragmentCode = fragmentCode;

    compile();
    link();
    }
  //}}}

  template<typename T> void setUniform (const string& name, T val);
  template<typename T> void setUniform (const string& name, T val1, T val2);
  template<typename T> void setUniform (const string& name, T val1, T val2, T val3);

  template<> void setUniform<bool>(const string& name, bool val) {
    glUniform1i (glGetUniformLocation (mId, name.c_str()), val);
    }

  template<> void setUniform<int>(const string& name, int val) {
    glUniform1i (glGetUniformLocation (mId, name.c_str()), val);
    }

  template<> void setUniform<float*>(const string& name, float* val) {
    glUniformMatrix4fv (glGetUniformLocation (mId, name.c_str()), 1, GL_FALSE, val);
    }

  template<> void setUniform<float>(const string& name, float val) {
    glUniform1f (glGetUniformLocation (mId, name.c_str()), val);
    }

  template<> void setUniform<float>(const string& name, float val1, float val2) {
    glUniform2f (glGetUniformLocation (mId, name.c_str()), val1, val2);
    }

  template<> void setUniform<float>(const string& name, float val1, float val2, float val3) {
    glUniform3f (glGetUniformLocation (mId, name.c_str()), val1, val2, val3);
    }

  //{{{
  void use() {
    glUseProgram (mId);
    }
  //}}}

private:
  //{{{
  void checkCompileError() {

    int success;
    char infoLog[1024];
    glGetShaderiv (mVertexId, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog (mVertexId, 1024, NULL, infoLog);
      cLog::log (LOGERROR, "Error compiling Vertex Shader " + string(infoLog));
      }

    glGetShaderiv (mFragmentId, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog (mFragmentId, 1024, NULL, infoLog);
      cLog::log (LOGERROR, "Error compiling Fragment Shader " + string(infoLog));
      }
    }
  //}}}
  //{{{
  void checkLinkingError() {

    int success;
    char infoLog[1024];
    glGetProgramiv (mId, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog (mId, 1024, NULL, infoLog);
      cLog::log (LOGERROR, "Error Linking Shader Program " + string (infoLog));
      }
    }
  //}}}

  //{{{
  void compile() {

    auto vertexCode = mVertexCode.c_str();
    mVertexId = glCreateShader (GL_VERTEX_SHADER);
    glShaderSource (mVertexId, 1, &vertexCode, NULL);
    glCompileShader (mVertexId);

    auto fragmentCode = mFragmentCode.c_str();
    mFragmentId = glCreateShader (GL_FRAGMENT_SHADER);
    glShaderSource (mFragmentId, 1, &fragmentCode, NULL);
    glCompileShader (mFragmentId);
    checkCompileError();
    }
  //}}}
  //{{{
  void link() {

    mId = glCreateProgram();
    glAttachShader (mId, mVertexId);
    glAttachShader (mId, mFragmentId);

    glLinkProgram (mId);
    checkLinkingError();

    glDeleteShader (mVertexId);
    glDeleteShader (mFragmentId);
    }
  //}}}

  unsigned mId;
  unsigned mVertexId;
  unsigned mFragmentId;

  string mVertexCode;
  string mFragmentCode;
  };
//}}}
//{{{
class cFileManager {
public:
  cFileManager() {}
  ~cFileManager() {}

  static string read (const string& filename) {
    ifstream file;
    file.exceptions (ifstream::failbit | ifstream::badbit);
    stringstream file_stream;

    try {
      file.open (filename.c_str());
      file_stream << file.rdbuf();
      file.close();
      }
    catch (ifstream::failure e) {
      cLog::log (LOGERROR, "Error reading Shader File");
      }

    return file_stream.str();
    }
  };
//}}}
//{{{
class cTriangle {
public:
  cTriangle() {
    glGenVertexArrays (1, &mVao);
    glBindVertexArray (mVao);
    cLog::log (LOGINFO, format ("{} vao:{}", kTag, mVao));

    glGenBuffers (1, &mVbo);
    glBindBuffer (GL_ARRAY_BUFFER, mVbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    cLog::log (LOGINFO, format ("{} vbo:{}", kTag, mVbo));

    glGenBuffers (1, &mEbo);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mEbo);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);
    cLog::log (LOGINFO, format ("{} ebo:{}", kTag, mEbo));

    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray (0);

    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray (1);

    mShader.init (kVertShader, kFragShader);
    //glBindBuffer (GL_ARRAY_BUFFER, 0);
    //glBindVertexArray (0);
    //shader.init (cFileManager::read ("vert.txt"), cFileManager::read ("frag.txt"));
    }

  void setRotate (float angle)    { mShader.setUniform ("angle", angle); }
  void setOffset (float offset[]) { mShader.setUniform ("offset", offset[0], offset[1]); }
  void setColor (float color[])   { mShader.setUniform ("color", color[0], color[1], color[2]); }

  void draw() {
    mShader.use();
    glBindVertexArray (mVao);
    glDrawElements (GL_TRIANGLES, sizeof(kIndices), GL_UNSIGNED_INT, 0);
    //glBindVertexArray (0);
    }

private:
  inline static const string kTag ="traiangle";
  //{{{  vertices
  const float kVertices[18] = { 0.0f,   0.25f, 0.0f,  1.0f, 0.0f, 0.0f, // vertex 1 red
                                0.25f, -0.25f, 0.0f,  0.0f, 1.0f, 0.0f, // vertex 2 green
                               -0.25f, -0.25f, 0.0f,  0.0f, 0.0f, 1.0f, // vertex 3 blue
                              };
  const unsigned kIndices[3] = { 0, 1, 2 };
  //}}}
  //{{{  shaders
  string kVertShader =
    "#version 330 core\n"
    "uniform float angle;"
    "uniform vec2 offset;"
    "layout (location = 0) in vec3 pos;"
    "layout (location = 1) in vec3 color;"
    "out vec3 vertexColor;\n"
    "void main() {"
    "  vec2 rotated_pos;"
    "  rotated_pos.x = offset.x + (pos.x * cos(angle)) - (pos.y * sin(angle));"
    "  rotated_pos.y = offset.y + (pos.x * sin(angle)) + (pos.y * cos(angle));"
    "  gl_Position = vec4 (rotated_pos.x, rotated_pos.y, pos.z, 1.0);"
    "  vertexColor = color;"
    "  }"
    "\n";

  string kFragShader =
    "#version 330 core\n"
    "uniform vec3 color;"
    "in vec3 vertexColor;"
    "out vec4 FragColor;"
    "void main() {"
    "  FragColor = vec4 (color * vertexColor, 1.0);"
    "  }"
    "\n";
  //}}}

  unsigned mVao;
  unsigned mVbo;
  unsigned mEbo;
  cShader mShader;
  };
//}}}
//{{{
class cRectangle {
public:
  cRectangle() {
    glGenVertexArrays (1, &mVao);
    glBindVertexArray (mVao);
    cLog::log (LOGINFO, format ("{} vao:{}", kTag, mVao));

    glGenBuffers (1, &mVbo);
    glBindBuffer (GL_ARRAY_BUFFER, mVbo);
    glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    cLog::log (LOGINFO, format ("{} vbo:{}", kTag, mVbo));

    glGenBuffers (1, &mEbo);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mEbo);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);
    cLog::log (LOGINFO, format ("{} ebo:{}", kTag, mEbo));

    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray (0);

    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray (1);

    mShader.init (kVertShader, kFragShader);
    //glBindBuffer (GL_ARRAY_BUFFER, 0);
    //glBindVertexArray (0);
    //shader.init (cFileManager::read ("vert.txt"), cFileManager::read ("frag.txt"));
    }

  void setRotate (float angle)    { mShader.setUniform ("angle", angle); }
  void setOffset (float offset[]) { mShader.setUniform ("offset", offset[0], offset[1]); }
  void setColor (float color[])   { mShader.setUniform ("color", color[0], color[1], color[2]); }

  void draw() {
    mShader.use();
    glBindVertexArray (mVao);
    glDrawElements (GL_TRIANGLES, sizeof(kIndices), GL_UNSIGNED_INT, 0);
    //glBindVertexArray (0);
    }


//GLubyte indices[] = {0,1,2, // first triangle (bottom left - top left - top right)
//                     0,2,3}; // second triangle (bottom left - top right - bottom right)
//glVertexPointer(3, GL_FLOAT, 0, vertices);
//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);

private:
  inline static const string kTag ="rectangle";
  //{{{  vertices
  const float kVertices[24] = { -1.f, -1.f, 0.0f,  1.f, 0.f, 0.f, // bl vertex 1 red
                                -1.f,  1.f, 0.0f,  0.f, 1.f, 0.f, // br vertex 2 green
                                 1.f,  1.f, 0.0f,  0.f, 0.f, 1.f, // tr vertex 3 blue
                                 1.f, -1.f, 0.0f,  1.f, 1.f, 1.f, // tl vertex 4 white
                              };
  const unsigned kIndices[6] = { 0,1,2, 0,2,3 };  // 3-2      2    3-2
                                                  // | |     /|    |/
                                                  // 0-1    0-1    0
  //}}}
  //{{{  shaders
  string kVertShader =
    "#version 330 core\n"
    "uniform float angle;"
    "uniform vec2 offset;"
    "layout (location = 0) in vec3 pos;"
    "layout (location = 1) in vec3 color;"
    "out vec3 vertexColor;\n"
    "void main() {"
    "  vec2 rotated_pos;"
    "  rotated_pos.x = offset.x + (pos.x * cos(angle)) - (pos.y * sin(angle));"
    "  rotated_pos.y = offset.y + (pos.x * sin(angle)) + (pos.y * cos(angle));"
    "  gl_Position = vec4 (rotated_pos.x, rotated_pos.y, pos.z, 1.0);"
    "  vertexColor = color;"
    "  }"
    "\n";

  string kFragShader =
    "#version 330 core\n"
    "uniform vec3 color;"
    "in vec3 vertexColor;"
    "out vec4 FragColor;"
    "void main() {"
    "  FragColor = vec4 (color * vertexColor, 1.0);"
    "  }"
    "\n";
  //}}}

  unsigned mVao;
  unsigned mVbo;
  unsigned mEbo;
  cShader mShader;
  };
//}}}

constexpr float kPi = 3.14159265f;
float angle = 0.0;
float offset[] = {0.0, 0.0};
float color[4] = { 1.0f,1.0f,1.0f,1.0f };

//{{{
void addLogo() {

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  static ImVec4 col1 = ImVec4 (68.0f / 255.0f, 83.0f / 255.0f, 89.0f / 255.0f, 1.0f);
  static ImVec4 col2 = ImVec4 (40.0f / 255.0f, 60.0f / 255.0f, 80.0f / 255.0f, 1.0f);
  static ImVec4 col3 = ImVec4 (50.0f / 255.0f, 65.0f / 255.0f, 82.0f / 255.0f, 1.0f);
  static ImVec4 col4 = ImVec4 (20.0f / 255.0f, 40.0f / 255.0f, 60.0f / 255.0f, 1.0f);

  const ImVec2 pos = ImGui::GetCursorScreenPos();
  float x = pos.x + 4.0f;
  float y = pos.y + 4.0f;
  float sz = 300.0f;

  draw_list->AddQuadFilled (ImVec2 (x, y + 0.25f * sz), ImVec2 (x + 0.5f * sz, y + 0.5f * sz),
                            ImVec2 (x + sz, y + 0.25f * sz), ImVec2 (x + 0.5f * sz, y),
                            ImColor (col1));
  draw_list->AddQuadFilled (ImVec2 (x, y + 0.25f * sz), ImVec2 (x + 0.5f * sz, y + 0.5f * sz),
                            ImVec2 (x + 0.5f * sz, y + 1.0f * sz), ImVec2 (x, y + 0.75f * sz),
                            ImColor (col2));
  draw_list->AddQuadFilled (ImVec2 (x + 0.5f * sz, y + 0.5f * sz), ImVec2 (x + sz, y + 0.25f * sz),
                            ImVec2 (x + sz, y + 0.75f * sz), ImVec2 (x + 0.5f * sz, y + 1.0f * sz),
                            ImColor (col3));
  draw_list->AddLine (ImVec2 (x + 0.75f * sz, y + 0.375f * sz), ImVec2 (x + 0.75f * sz, y + 0.875f * sz),
                      ImColor (col4));
  draw_list->AddBezierCurve (ImVec2 (x + 0.72f * sz, y + 0.24f * sz), ImVec2 (x + 0.68f * sz, y + 0.15f * sz),
                             ImVec2 (x + 0.48f * sz, y + 0.13f * sz), ImVec2 (x + 0.39f * sz, y + 0.17f * sz),
                             ImColor (col4), 10, 18);
  draw_list->AddBezierCurve (ImVec2 (x + 0.39f * sz, y + 0.17f * sz), ImVec2 (x + 0.2f * sz, y + 0.25f * sz),
                             ImVec2 (x + 0.3f * sz, y + 0.35f * sz), ImVec2 (x + 0.49f * sz, y + 0.38f * sz),
                             ImColor (col4), 10, 18);
  }
//}}}

int main (int, char **) {

  cLog::init (LOGINFO);
  cLog::log (LOGNOTICE, "triangle openGL");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  //ImGuiIO& io = ImGui::GetIO();

  cPlatform platform;
  if (!platform.init()) {
    //{{{  error return
    cLog::log (LOGERROR, "Platform init failed");
    return 1;
    }
    //}}}
  platform.initIo();

  cGraphics graphics;
  if (!graphics.init (platform.getScreenWidth(), platform.getScreenHeight())) {
    //{{{  error return
    cLog::log (LOGERROR, "Graphics init failed");
    return 1;
    }
    //}}}

  //cTriangle shape;
  cRectangle shape;

  while (platform.pollEvents()) {
    graphics.clear();
    graphics.newFrame();
    platform.newFrame();
    ImGui::NewFrame();

    shape.draw();

    // gui
    ImGui::Begin ("Triangle");
    ImGui::SliderFloat2 ("offset", offset, -1.0, 1.0);
    shape.setOffset (offset);
    ImGui::SliderFloat ("angle", &angle, 0, 2.0f * kPi);
    shape.setRotate (angle);
    ImGui::ColorEdit3 ("color", color);
    shape.setColor (color);
    ImGui::End();

    // logo
    ImGui::Begin ("logo");
    addLogo();
    ImGui::End();

    // imgui
    ImGui::Render();
    graphics.draw (ImGui::GetDrawData());

    platform.present();
    }

  // shutdown
  graphics.shutdown();
  platform.shutdown();
  ImGui::DestroyContext();

  return 0;
  }
