#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// Cross-platform SDL includes
#ifdef _WIN32
    #include <SDL.h>
    #include <SDL_ttf.h>
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
#endif


#define WIDTH 900
#define MAX_OBJECTS 100
#define HEIGHT 600
#define M_PI 3.14159265358979323846
#define GRAVITY -9.81f
#define FLOOR_HEIGHT 0.0f

//globals

bool useOrtho = false;
float orthoScale = 100.0f;
float mouseSensitivity = 0.1f; // FPS mouse look sensitivity (degrees per pixel)
static int LINE_THICKNESS = 3;  // grubsze linie w pikselach (ekran)

// Global clip planes for perspective
static const float NEAR_PLANE = 0.2f;
static const float FAR_PLANE  = 500.0f;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3d struct
typedef struct { float x,y,z; } Vertex;
// rorate matrix
typedef struct{ float matrix[4][4]; }Matrix4x4;
typedef enum {
  OBJECT_TYPE_PLAYER,
  OBJECT_TYPE_DYNAMIC,
  OBJECT_TYPE_STATIC
}ObjectType;

//physics object
typedef struct PhysicsObject {
  Vertex position;
  Vertex velocity;
  Vertex force;
  ObjectType type;
  float height;
  float mass;
}PhysicsObject;
//Camera;
typedef struct {
  PhysicsObject physics;
  float yaw, pitch; // rotation angles (in degrees): yaw (left/right), pitch (up/down)
  float fov;
}Camera;
PhysicsObject object[100];
int objCount = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init camera
Camera camera = {
  .physics = {
    .position = {5.0f, 0.0f, -10.0f}, // wysokość jak postać w FPS (~1.7m wzrostu)
    .velocity = {0.0f, 0.0f, 0.0f},
    .force = {0.0f, 0.0f, 0.0f},
    .mass = 1.0f,
    .type = OBJECT_TYPE_PLAYER,
    .height = 1.7f,
  },
  .yaw = 0.0f,
  .pitch = 0.0f,
  .fov = 90.f,
};

Vertex vertexCube[] = {
//    x      y      z
  {-1.0f, -1.0f, -1.0f},// lewy-dolny-tył
  { 1.0f, -1.0f, -1.0f},// prawy-dolny-tył
  { 1.0f, 1.0f, -1.0f}, // prawy-góry-tył
  {-1.0f, 1.0f, -1.0f},// lewy-górny-tył
  {-1.0f, -1.0f, 1.0f},//lewy-dolny-przód
  { 1.0f, -1.0f, 1.0f},// prawy-dolny-przód
  { 1.0f, 1.0f, 1.0f}, // prawy-góry-przód
  {-1.0f, 1.0f, 1.0f},// lewy-górny-przód
};

// removed unused zoom; FOV controls (W/E) adjust camera.fov directly

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Vertex rotatePoint(Vertex p, float angleX, float angleY);
void checkFloorCollision(PhysicsObject* obj);
SDL_Point projectOrtho(Vertex v, Camera cam, float scale );
SDL_Point project(Vertex v, Camera cam);
SDL_Point projectDynamic(Vertex v, Camera cam);
void renderText(SDL_Renderer* renderer, const char* text, Vertex position,Camera cam, TTF_Font* font, SDL_Color color);
void drawline(SDL_Renderer* renderer, Vertex v1, Vertex v2, Camera cam);
void drawThickLine(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2, int thickness);
void drawAxesWithLabels(SDL_Renderer* renderer, Camera cam, TTF_Font * font);
void drawFloor(SDL_Renderer* renderer, Camera cam, int size, float spacing);
// Helpers for clipping and camera-space projection
Vertex toCameraSpace(Vertex v, Camera cam);
bool clipLineToNearFar(Vertex* a, Vertex* b, float nearP, float farP);
SDL_Point projectCameraSpace(Vertex r, Camera cam);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void updatePhysics(PhysicsObject* obj, float deltaTime) 
{

  Vertex acceleration = {
    obj->force.x / obj->mass,
    obj->force.y / obj->mass,
    obj->force.z / obj->mass
  };
  obj->velocity.x += acceleration.x * deltaTime;
  obj->velocity.y += acceleration.y * deltaTime;
  obj->velocity.z += acceleration.z * deltaTime;

  obj->position.x += obj->velocity.x * deltaTime;
  obj->position.y += obj->velocity.y * deltaTime;
  obj->position.z += obj->velocity.z * deltaTime;

  obj->force.x = 0.0f;
  obj->force.y = 0.0f;
  obj->force.z = 0.0f;
  obj->force.y += GRAVITY * obj->mass;
  checkFloorCollision(obj);

}
SDL_Point projectOrtho(Vertex v, Camera cam, float scale ){
  Vertex relative = {
    v.x -cam.physics.position.x,
    v.y -cam.physics.position.y,
    v.z -cam.physics.position.z
  };
  Vertex r = rotatePoint(relative, cam.pitch, cam.yaw);

  SDL_Point p;
  p.x = (int)(r.x * scale + WIDTH /2);
  p.y = (int)(-r.y * scale + HEIGHT/2);
  return p;
}
SDL_Point project(Vertex v, Camera cam){

  Vertex relative = {
    v.x - cam.physics.position.x,
    v.y - cam.physics.position.y,
    v.z - cam.physics.position.z,
  };

  Vertex r = rotatePoint(relative, cam.pitch, cam.yaw);
  SDL_Point p = { -1000, -1000 };
  if (r.z <= NEAR_PLANE || r.z >= FAR_PLANE) return p;

  float aspect = (float)WIDTH / HEIGHT;
  float fovRad = cam.fov *(M_PI/180.0f);
  float tanHalf = tanf(fovRad * 0.5f);

  float x_ndc =  r.x / (r.z * tanHalf * aspect);
  float y_ndc = -r.y / (r.z * tanHalf);    // minus = ekran Y w dół

  // 4) NDC -> screen
  p.x = (int)((x_ndc + 1.0f) * 0.5f * WIDTH);
  p.y = (int)((y_ndc + 1.0f) * 0.5f * HEIGHT);
  return p;
}

void renderText(SDL_Renderer* renderer, const char* text, Vertex position,Camera cam, TTF_Font* font, SDL_Color color) {
  SDL_Point screenPos = project(position,cam);
  if (screenPos.x >= 0 && screenPos.x < WIDTH && screenPos.y >= 0 && screenPos.y < HEIGHT){
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    if(textSurface) {
      SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
      if(textTexture) {
        //przesun tekst aby byl wycentrowany wzgledem punktu
        SDL_Rect dstRect = {
          screenPos.x - textSurface->w /2,
          screenPos.y - textSurface->h /2,
          textSurface->w,
          textSurface->h,
        };
        SDL_RenderCopy(renderer,textTexture,NULL, &dstRect);
        SDL_DestroyTexture(textTexture);
      }
      SDL_FreeSurface(textSurface);

    }
  }
}
SDL_Point projectDynamic(Vertex v, Camera cam) {

  if(useOrtho)
    return projectOrtho(v,cam,orthoScale);
  else 
    return project(v,cam);
}
void drawline(SDL_Renderer* renderer, Vertex v1, Vertex v2, Camera cam)  
{
  if (useOrtho) {
    SDL_Point p1 = projectOrtho(v1, cam, orthoScale);
    SDL_Point p2 = projectOrtho(v2, cam, orthoScale);
    drawThickLine(renderer, p1, p2, LINE_THICKNESS);
    return;
  }

  // Perspective path with near/far clipping in camera space
  Vertex r1 = toCameraSpace(v1, cam);
  Vertex r2 = toCameraSpace(v2, cam);
  if (!clipLineToNearFar(&r1, &r2, NEAR_PLANE, FAR_PLANE)) {
    return; // fully outside
  }

  SDL_Point p1 = projectCameraSpace(r1, cam);
  SDL_Point p2 = projectCameraSpace(r2, cam);
  drawThickLine(renderer, p1, p2, LINE_THICKNESS);
}
void drawAxesWithLabels(SDL_Renderer* renderer, Camera cam, TTF_Font * font) {
  Vertex origin = {0.0f,0.0f, 0.0f};
  Vertex xAxis = {10.0f,0.0f,0.0f};
  Vertex yAxis = {0.0f, 10.0f, 0.0f};
  Vertex zAxis = {0.0f,0.0f,10.0f};
  //draw main axes
  SDL_SetRenderDrawColor(renderer, 255,0,0,255);
  drawline(renderer, origin, xAxis, cam);

  SDL_SetRenderDrawColor(renderer,0,255,0,255);
  drawline(renderer, origin, yAxis,cam);
  
  SDL_SetRenderDrawColor(renderer, 0,0,255,255);
  drawline(renderer,origin,zAxis,cam);
  char label[10];
  SDL_Color colorX = {255,0,0,255};
  SDL_Color colorY = {0,255,0,255};
  SDL_Color colorZ = {0,0,255,255};
  
  for(int i=1; i <=10; i++) {
    Vertex tickTop = {i, 0.2f, 0};
    Vertex tickBottom = {i, -0.2f,0};
    SDL_SetRenderDrawColor(renderer,255,0,0,255);
    drawline(renderer, tickTop, tickBottom, cam);
    sprintf(label,"%d",i);
    Vertex textPos = {i, -0.5f, 0};
    renderText(renderer,label, textPos, cam,font,colorX);
  }

  for(int i=1; i <=10; i++) {
    Vertex tickTop = {0.2f, i, 0};
    Vertex tickBottom = {-0.2f, i, 0};
    SDL_SetRenderDrawColor(renderer,0,255,0,255);
    drawline(renderer, tickTop, tickBottom, cam);
    sprintf(label,"%d",i);
    Vertex textPos = {0, i, -0.5f};
    renderText(renderer,label, textPos, cam,font,colorY);
  }

  for(int i=1; i <=10; i++) {
    Vertex tickTop = {0.2f, 0, i};
    Vertex tickBottom = {-0.2f, 0, i};
    SDL_SetRenderDrawColor(renderer,0,0,255,255);
    drawline(renderer, tickTop, tickBottom, cam);
    sprintf(label,"%d",i);
    Vertex textPos = {0, -0.5f, i};
    renderText(renderer,label, textPos, cam,font,colorZ);
  }
  renderText(renderer, "X", (Vertex){10.5f,0,0}, cam,font, colorX);
  renderText(renderer, "Y", (Vertex){0,10.5f,0}, cam,font, colorY);
  renderText(renderer, "Z", (Vertex){0,0,10.5f}, cam,font, colorZ);
}
Vertex rotatePoint(Vertex p, float angleX, float angleY) {
  //stopnie→radiany
  float radX = angleX * (M_PI / 180.0f);
  float radY = angleY * (M_PI / 180.0f);

  Vertex temp;
  temp.x = p.x * cosf(radY) + p.z* sinf(radY);
  temp.y = p.y;
  temp.z = -p.x * sinf(radY) + p.z* cosf(radY);

  Vertex result;
  result.x = temp.x;
  result.y = temp.y * cosf(radX) - temp.z * sinf(radX);
  result.z = temp.y * sinf(radX) + temp.z * cosf(radX);
  return result;
}

// Convert world-space vertex to camera-space (translate by camera position, then rotate by yaw/pitch)
Vertex toCameraSpace(Vertex v, Camera cam) {
  Vertex relative = {
    v.x - cam.physics.position.x,
    v.y - cam.physics.position.y,
    v.z - cam.physics.position.z,
  };
  return rotatePoint(relative, cam.pitch, cam.yaw);
}

// Clip line segment in camera space to [NEAR_PLANE, FAR_PLANE] in z
bool clipLineToNearFar(Vertex* a, Vertex* b, float nearP, float farP) {
  // Trivial reject
  if (a->z <= nearP && b->z <= nearP) return false;
  if (a->z >= farP  && b->z >= farP)  return false;

  float dz = b->z - a->z;
  // Clip to near plane
  if (a->z < nearP && b->z > a->z) {
    float t = (nearP - a->z) / dz;
    a->x = a->x + t * (b->x - a->x);
    a->y = a->y + t * (b->y - a->y);
    a->z = nearP;
  } else if (b->z < nearP && a->z > b->z) {
    float t = (nearP - a->z) / dz;
    b->x = a->x + t * (b->x - a->x);
    b->y = a->y + t * (b->y - a->y);
    b->z = nearP;
  }

  // Recompute dz after potential near clipping
  dz = b->z - a->z;

  // Clip to far plane
  if (a->z > farP && b->z < a->z) {
    float t = (farP - a->z) / dz;
    a->x = a->x + t * (b->x - a->x);
    a->y = a->y + t * (b->y - a->y);
    a->z = farP;
  } else if (b->z > farP && a->z < b->z) {
    float t = (farP - a->z) / dz;
    b->x = a->x + t * (b->x - a->x);
    b->y = a->y + t * (b->y - a->y);
    b->z = farP;
  }

  // After clipping, ensure the segment is still valid
  if (a->z <= nearP && b->z <= nearP) return false;
  if (a->z >= farP  && b->z >= farP)  return false;
  return true;
}

// Project a camera-space point to screen
SDL_Point projectCameraSpace(Vertex r, Camera cam) {
  SDL_Point p = { -1000, -1000 };
  if (r.z <= 0.0f) return p; // behind camera after clipping shouldn't happen

  float aspect = (float)WIDTH / HEIGHT;
  float fovRad = cam.fov * (M_PI / 180.0f);
  float tanHalf = tanf(fovRad * 0.5f);

  float x_ndc =  r.x / (r.z * tanHalf * aspect);
  float y_ndc = -r.y / (r.z * tanHalf);

  p.x = (int)((x_ndc + 1.0f) * 0.5f * WIDTH);
  p.y = (int)((y_ndc + 1.0f) * 0.5f * HEIGHT);
  return p;
}

// Rysuje linię o zadanej grubości w przestrzeni ekranu
void drawThickLine(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2, int thickness) {
  if (thickness <= 1) {
    SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
    return;
  }
  float dx = (float)(p2.x - p1.x);
  float dy = (float)(p2.y - p1.y);
  float len = sqrtf(dx*dx + dy*dy);
  if (len < 1e-3f) {
    // Bardzo krótka linia -> narysuj kilka punktów wokół
    int half = thickness / 2;
    for (int oy = -half; oy <= half; ++oy) {
      for (int ox = -half; ox <= half; ++ox) {
        SDL_RenderDrawPoint(renderer, p1.x + ox, p1.y + oy);
      }
    }
    return;
  }
  // wektor normalny (prostopadły), znormalizowany
  float nx = -dy / len;
  float ny =  dx / len;
  int half = thickness / 2;
  for (int i = -half; i <= half; ++i) {
    int ox = (int)roundf(nx * i);
    int oy = (int)roundf(ny * i);
    SDL_RenderDrawLine(renderer, p1.x + ox, p1.y + oy, p2.x + ox, p2.y + oy);
  }
}
void drawFloor(SDL_Renderer* renderer, Camera cam, int size, float spacing) {
  // Jaśniejszy kolor podłogi dla lepszej widoczności
  SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);


  for (int i = -size; i <= size; i++) {
    Vertex start = {-size * spacing, FLOOR_HEIGHT, i * spacing};
    Vertex end = {size * spacing, FLOOR_HEIGHT, i * spacing};
    drawline(renderer, start, end, cam);
  }
  for (int i = -size; i <= size; i++) {
    Vertex start = {i * spacing, FLOOR_HEIGHT, -size * spacing};
    Vertex end = {i * spacing, FLOOR_HEIGHT, size * spacing};
    drawline(renderer, start, end, cam);
  }


  /*
  for (int i = -size; i <= size; i++) {
    Vertex start = {i * spacing, FLOOR_HEIGHT, -size * spacing};
    Vertex end = {i * spacing, FLOOR_HEIGHT, size * spacing};
    drawline(renderer, start, end, cam);
  }

  */
}
  void checkFloorCollision(PhysicsObject* obj){
  
    if(obj->type == OBJECT_TYPE_PLAYER ) { 
  float minEyeHight = FLOOR_HEIGHT + obj-> height;

  if (obj->position.y < minEyeHight) {
      obj->position.y = minEyeHight;
      obj->velocity.y = 0.0f;
  }
}
  else if(obj->type == OBJECT_TYPE_DYNAMIC) {
    if(obj->position.y < FLOOR_HEIGHT) {
      obj->position.y = FLOOR_HEIGHT;
      if(obj->velocity.y < 0) {
        obj-> velocity.y = -obj->velocity.y * 0.5f;
        if(fabsf(obj->velocity.y) < 0.1f) {
          obj->velocity.y = 0.0f;
      }
        obj->velocity.x *= 0.9f;
        obj->velocity.z *= 0.9f;
        }
      }
    }
  }
int main(int argc, char *argv[])
{
  // Suppress unused parameter warnings
  (void)argc;
  (void)argv;

  if(SDL_Init(0) < 0) {
    printf("Błąd inicjalizacji SDL: %s\n", SDL_GetError());
    return 1;
  }
  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    printf("Błąd przy inicjalizacji Subsystemu Video!");
    SDL_Quit();
    return 1;
  }
  //tworzenie okna SDL
   SDL_Window* window = SDL_CreateWindow("Engine3d", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH,HEIGHT, SDL_WINDOW_OPENGL);
   if (!window) {
    printf("Błąd tworzenia okna: %s\n", SDL_GetError());
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
    return 1;
   }
   //tworzenie rerendera
   SDL_Renderer* renderer = SDL_CreateRenderer( window, -1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
   if(!renderer) {
    fprintf(stderr, "Nie udało się utworzyć rendera: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
   }
   if(TTF_Init() <0) {
    printf("błąd inicjalizacji SDL_TTF: %s\n", TTF_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
   }
   // Enable FPS-style relative mouse mode (captures cursor and provides relative motion)
   if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
     fprintf(stderr, "Nie udało się włączyć trybu względnego myszy: %s\n", SDL_GetError());
   }
   
   // Cross-platform font loading
   TTF_Font* font = NULL;
   const char* fontPaths[] = {
       "./fonts/Hack-Regular.ttf",                    // Local font (cross-platform)
       "/usr/share/fonts/TTF/Hack-Regular.ttf",       // Linux (Arch/Manjaro)
       "/usr/share/fonts/truetype/hack/Hack-Regular.ttf", // Linux (Ubuntu/Debian)
       "C:/Windows/Fonts/arial.ttf",                  // Windows fallback
       "C:/Windows/Fonts/consola.ttf",                // Windows Consolas
       NULL
   };
   
   for (int i = 0; fontPaths[i] != NULL; i++) {
       font = TTF_OpenFont(fontPaths[i], 12);
       if (font) {
           printf("Loaded font: %s\n", fontPaths[i]);
           break;
       }
   }
   
   if(!font){
    printf("błąd ładowania czcionki: %s\n", TTF_GetError());
    printf("Próbowano ścieżek:\n");
    for (int i = 0; fontPaths[i] != NULL; i++) {
        printf("  - %s\n", fontPaths[i]);
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
   }
  // removed unused local rotation variables

   int quit = EXIT_SUCCESS;
   SDL_Event event;
   Uint64 lastTime = SDL_GetTicks64();
   float deltaTime = 0.0f;

   while(!quit) {
    //obsluga deltatime
    
    Uint64 currentTime = SDL_GetTicks64();
    deltaTime = (currentTime - lastTime) / 1000.0f; // Konwersja na sekundy
    lastTime = currentTime;
    
      while (SDL_PollEvent(&event))
      {
        if(event.type == SDL_QUIT) 
          quit = 1;
        if (event.type == SDL_KEYDOWN) 
        {
          float moveSpeed = 1.0f;
          float rotationSpeed = 2.0f;

            switch(event.key.keysym.sym) 
            {
              case SDLK_z:
                camera.physics.position.z += moveSpeed;
                break;
              case SDLK_a:
                camera.physics.position.z -= moveSpeed;
                break;
              case SDLK_c:  // Do przodu
                camera.physics.position.y += moveSpeed;
                break;
              case SDLK_d:  // Do tyłu
                camera.physics.position.y -= moveSpeed;
                break;
              case SDLK_x:  // W lewo
                camera.physics.position.x -= moveSpeed;
                break;
              case SDLK_s:  // W prawo
                camera.physics.position.x += moveSpeed;
                break;
              case SDLK_v:  // Obrót w górę
                camera.pitch -= rotationSpeed;
                break;
              case SDLK_f:  // Obrót w dół
                camera.pitch += rotationSpeed;
                break;
              case SDLK_b:  // Obrót w lewo
                camera.yaw -= rotationSpeed;
                break;
              case SDLK_g:  // Obrót w prawo
                camera.yaw += rotationSpeed;
                break;
              case SDLK_q: // Przełączanie projekcji
                useOrtho = !useOrtho;
                break;
              case SDLK_w: // FOV + (mniejsze zbliżenie)
                camera.fov += 5.0f;
                if (camera.fov > 120.0f) camera.fov = 120.0f;
                break;
              case SDLK_e: // FOV - (większe zbliżenie)
                camera.fov -= 5.0f;
                if (camera.fov < 30.0f) camera.fov = 30.0f;
                break;
            }
        } 
        if (event.type == SDL_MOUSEMOTION)   
        {
          // FPS-style mouse look when in relative mouse mode
          if (SDL_GetRelativeMouseMode()) {
            camera.yaw   += event.motion.xrel * mouseSensitivity;
            camera.pitch -= event.motion.yrel * mouseSensitivity; // move up -> look up

            // Clamp pitch to avoid flipping (gimbal singularity)
            if (camera.pitch > 89.0f)  camera.pitch = 89.0f;
            if (camera.pitch < -89.0f) camera.pitch = -89.0f;

            // Normalize yaw to [-180, 180] range to keep numbers bounded
            if (camera.yaw > 180.0f)  camera.yaw -= 360.0f;
            if (camera.yaw < -180.0f) camera.yaw += 360.0f;
          }
        }
      }


      updatePhysics(&camera.physics,deltaTime);

      for(int i = 0; i < objCount; i++){
        updatePhysics(&object[i], deltaTime);
      }

      //czyszczenie ekranu + czarne tło
      SDL_SetRenderDrawColor(renderer, 15,15,15,255);
      SDL_RenderClear(renderer);
      
      drawFloor(renderer,camera,20,1.0f);

      drawAxesWithLabels(renderer,camera,font);


  

      SDL_RenderPresent(renderer);
      SDL_Delay(16);

   }

   TTF_Quit();
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   SDL_Quit();

   return EXIT_SUCCESS;

}
