#include "gl_local.h"

#include "../client/keys.h"

#include <GLFW/glfw3.h>

GLFWwindow *glfw_window;

extern unsigned sys_msg_time;
extern void AppActivate(bool fActive, bool minimize);

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  int index = button - GLFW_MOUSE_BUTTON_1;
  if(action == GLFW_PRESS)
    Key_Event(K_MOUSE1 + index, true, sys_msg_time);
  else if(action == GLFW_RELEASE)
    Key_Event(K_MOUSE1 + index, false, sys_msg_time);
}

static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  if(yoffset > 0) {
    Key_Event(K_MWHEELUP, true, sys_msg_time);
    Key_Event(K_MWHEELUP, false, sys_msg_time);
  } else if(yoffset < 0) {
    Key_Event(K_MWHEELDOWN, true, sys_msg_time);
    Key_Event(K_MWHEELDOWN, false, sys_msg_time);
  }
}

static void window_iconify_callback(GLFWwindow *window, int iconified) { AppActivate(true, iconified); }

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  static const int key_map[GLFW_KEY_LAST + 1] = {
      [GLFW_KEY_SPACE] = ' ',              //
      [GLFW_KEY_APOSTROPHE] = '\'',        //  /* ' */
      [GLFW_KEY_COMMA] = ',',              //  /* , */
      [GLFW_KEY_MINUS] = '-',              //  /* - */
      [GLFW_KEY_PERIOD] = '.',             //  /* . */
      [GLFW_KEY_SLASH] = '/',              //  /* / */
      [GLFW_KEY_0] = '0',                  //
      [GLFW_KEY_1] = '1',                  //
      [GLFW_KEY_2] = '2',                  //
      [GLFW_KEY_3] = '3',                  //
      [GLFW_KEY_4] = '4',                  //
      [GLFW_KEY_5] = '5',                  //
      [GLFW_KEY_6] = '6',                  //
      [GLFW_KEY_7] = '7',                  //
      [GLFW_KEY_8] = '8',                  //
      [GLFW_KEY_9] = '9',                  //
      [GLFW_KEY_SEMICOLON] = ';',          //  /* ; */
      [GLFW_KEY_EQUAL] = '=',              //  /* = */
      [GLFW_KEY_A] = 'a',                  //
      [GLFW_KEY_B] = 'b',                  //
      [GLFW_KEY_C] = 'c',                  //
      [GLFW_KEY_D] = 'd',                  //
      [GLFW_KEY_E] = 'e',                  //
      [GLFW_KEY_F] = 'f',                  //
      [GLFW_KEY_G] = 'g',                  //
      [GLFW_KEY_H] = 'h',                  //
      [GLFW_KEY_I] = 'i',                  //
      [GLFW_KEY_J] = 'j',                  //
      [GLFW_KEY_K] = 'k',                  //
      [GLFW_KEY_L] = 'l',                  //
      [GLFW_KEY_M] = 'm',                  //
      [GLFW_KEY_N] = 'n',                  //
      [GLFW_KEY_O] = 'o',                  //
      [GLFW_KEY_P] = 'p',                  //
      [GLFW_KEY_Q] = 'q',                  //
      [GLFW_KEY_R] = 'r',                  //
      [GLFW_KEY_S] = 's',                  //
      [GLFW_KEY_T] = 't',                  //
      [GLFW_KEY_U] = 'u',                  //
      [GLFW_KEY_V] = 'v',                  //
      [GLFW_KEY_W] = 'w',                  //
      [GLFW_KEY_X] = 'x',                  //
      [GLFW_KEY_Y] = 'y',                  //
      [GLFW_KEY_Z] = 'z',                  //
      [GLFW_KEY_LEFT_BRACKET] = '[',       //  /* [ */
      [GLFW_KEY_BACKSLASH] = '\\',         //  /* \ */
      [GLFW_KEY_RIGHT_BRACKET] = ']',      //  /* ] */
      [GLFW_KEY_GRAVE_ACCENT] = '`',       //  /* ` */
      [GLFW_KEY_WORLD_1] = -1,             // /* non-US #1 */
      [GLFW_KEY_WORLD_2] = -1,             // /* non-US #2 */
      [GLFW_KEY_ESCAPE] = K_ESCAPE,        //
      [GLFW_KEY_ENTER] = K_ENTER,          //
      [GLFW_KEY_TAB] = K_TAB,              //
      [GLFW_KEY_BACKSPACE] = K_BACKSPACE,  //
      [GLFW_KEY_INSERT] = K_INS,           //
      [GLFW_KEY_DELETE] = K_DEL,           //
      [GLFW_KEY_RIGHT] = K_RIGHTARROW,     //
      [GLFW_KEY_LEFT] = K_LEFTARROW,       //
      [GLFW_KEY_DOWN] = K_DOWNARROW,       //
      [GLFW_KEY_UP] = K_UPARROW,           //
      [GLFW_KEY_PAGE_UP] = K_PGUP,         //
      [GLFW_KEY_PAGE_DOWN] = K_PGDN,       //
      [GLFW_KEY_HOME] = K_HOME,            //
      [GLFW_KEY_END] = K_END,              //
      [GLFW_KEY_CAPS_LOCK] = -1,           //
      [GLFW_KEY_SCROLL_LOCK] = -1,         //
      [GLFW_KEY_NUM_LOCK] = -1,            //
      [GLFW_KEY_PRINT_SCREEN] = -1,        //
      [GLFW_KEY_PAUSE] = K_PAUSE,          //
      [GLFW_KEY_F1] = K_F1,                //
      [GLFW_KEY_F2] = K_F2,                //
      [GLFW_KEY_F3] = K_F3,                //
      [GLFW_KEY_F4] = K_F4,                //
      [GLFW_KEY_F5] = K_F5,                //
      [GLFW_KEY_F6] = K_F6,                //
      [GLFW_KEY_F7] = K_F7,                //
      [GLFW_KEY_F8] = K_F8,                //
      [GLFW_KEY_F9] = K_F9,                //
      [GLFW_KEY_F10] = K_F10,              //
      [GLFW_KEY_F11] = K_F11,              //
      [GLFW_KEY_F12] = K_F12,              //
      [GLFW_KEY_F13] = -1,                 //
      [GLFW_KEY_F14] = -1,                 //
      [GLFW_KEY_F15] = -1,                 //
      [GLFW_KEY_F16] = -1,                 //
      [GLFW_KEY_F17] = -1,                 //
      [GLFW_KEY_F18] = -1,                 //
      [GLFW_KEY_F19] = -1,                 //
      [GLFW_KEY_F20] = -1,                 //
      [GLFW_KEY_F21] = -1,                 //
      [GLFW_KEY_F22] = -1,                 //
      [GLFW_KEY_F23] = -1,                 //
      [GLFW_KEY_F24] = -1,                 //
      [GLFW_KEY_F25] = -1,                 //
      [GLFW_KEY_KP_0] = K_KP_INS,          //
      [GLFW_KEY_KP_1] = K_KP_END,          //
      [GLFW_KEY_KP_2] = K_KP_DOWNARROW,    //
      [GLFW_KEY_KP_3] = K_KP_PGDN,         //
      [GLFW_KEY_KP_4] = K_KP_LEFTARROW,    //
      [GLFW_KEY_KP_5] = K_KP_5,            //
      [GLFW_KEY_KP_6] = K_KP_RIGHTARROW,   //
      [GLFW_KEY_KP_7] = K_KP_HOME,         //
      [GLFW_KEY_KP_8] = K_KP_UPARROW,      //
      [GLFW_KEY_KP_9] = K_KP_PGUP,         //
      [GLFW_KEY_KP_DECIMAL] = K_KP_DEL,    //
      [GLFW_KEY_KP_DIVIDE] = K_KP_SLASH,   //
      [GLFW_KEY_KP_MULTIPLY] = -1,         //
      [GLFW_KEY_KP_SUBTRACT] = K_KP_MINUS, //
      [GLFW_KEY_KP_ADD] = K_KP_PLUS,       //
      [GLFW_KEY_KP_ENTER] = K_KP_ENTER,    //
      [GLFW_KEY_KP_EQUAL] = -1,            //
      [GLFW_KEY_LEFT_SHIFT] = K_SHIFT,     //
      [GLFW_KEY_LEFT_CONTROL] = -1,        //
      [GLFW_KEY_LEFT_ALT] = -1,            //
      [GLFW_KEY_LEFT_SUPER] = -1,          //
      [GLFW_KEY_RIGHT_SHIFT] = K_SHIFT,    //
      [GLFW_KEY_RIGHT_CONTROL] = -1,       //
      [GLFW_KEY_RIGHT_ALT] = -1,           //
      [GLFW_KEY_RIGHT_SUPER] = -1,         //
      [GLFW_KEY_MENU] = -1,                //
  };
  if(action == GLFW_REPEAT || key == GLFW_KEY_UNKNOWN)
    return;
  key = key_map[key];
  if(key > 0)
    Key_Event(key, action == GLFW_PRESS, uv_now(&global_uv_loop));
}

rserr_t GLimp_SetMode(int *pwidth, int *pheight, int mode, bool fullscreen) {
  int width, height;

  if(glfw_window) {
    glfwTerminate();
    if(!GLimp_Init(NULL, NULL))
      return rserr_unknown;
  }

  if(!ri.Vid_GetModeInfo(&width, &height, mode)) {
    ri.Con_Printf(PRINT_ALL, " invalid mode\n");
    return rserr_invalid_mode;
  }

  GLFWmonitor *monitor = NULL;

  //   if(fullscreen) {
  //     monitor = glfwGetPrimaryMonitor();
  //   }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
  // glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);

  glfw_window = glfwCreateWindow(width, height, "Quake 2", monitor, NULL);

  if(!glfw_window) {
    if(fullscreen)
      return rserr_invalid_fullscreen;
    return rserr_unknown;
  }

  glfwSetScrollCallback(glfw_window, scroll_callback);
  glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
  glfwSetKeyCallback(glfw_window, key_callback);
  glfwSetWindowIconifyCallback(glfw_window, window_iconify_callback);

  /* Make the window's context current */
  glfwMakeContextCurrent(glfw_window);

  ri.Vid_NewWindow(width, height);
  *pwidth = width;
  *pheight = height;

  AppActivate(true, false);

  return rserr_ok;
}

int GLimp_Init(void *a, void *b) {
  if(!glfwInit())
    return false;
  return true;
}

void GLimp_Shutdown(void) {}

void GLimp_BeginFrame(float ignore) {}

void GLimp_EndFrame(void) { glfwSwapBuffers(glfw_window); }

void GLimp_AppActivate(bool active) {
  if(active) {
    glfwRestoreWindow(glfw_window);
  } else {
    if(vid_fullscreen->value)
      glfwIconifyWindow(glfw_window);
  }
}
