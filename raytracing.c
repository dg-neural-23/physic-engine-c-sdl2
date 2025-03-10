#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <math.h>


#define WIDTH 900
#define HEIGHT 600
#define M_PI 3.14159265358979323846
#define GRAVITY -9.81f
#define FLOOR_HEIGHT 0.0f

//globals

bool useOrtho = false;
float orthoScale = 100.0f;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//3d struct
typedef struct { float x,y,z; } Vertex;
// rorate matrix
typedef struct{ float matrix[4][4]; }Matrix4x4;
//physics object
typedef struct PhysicsObject {
  Vertex position;
  Vertex velocity;
  Vertex force;
  float mass;
}PhysicsObject;
//Camera;
typedef struct {
  PhysicsObject physics;
  float yaw, pitch; // rotation angles (in degrees): yaw (left/right), pitch (up/down)
  float fov;
}Camera;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init camera
Camera camera = {
  .physics = {
    .position = {0.0f, 10.0f, -5.0f},
    .velocity = {0.0f, 0.0f, 0.0f},
    .force = {0.0f, 0.0f, 0.0f},
    .mass = 1.0f
  },
  .yaw = 0.0f,
  .pitch = 0.0f,
  .fov = 60.f,
};

// Vertex vertexCube[] = {
// //    x      y      z
//   {-1.0f, -1.0f, -1.0f},// lewy-dolny-tył
//   { 1.0f, -1.0f, -1.0f},// prawy-dolny-tył
//   { 1.0f, 1.0f, -1.0f}, // prawy-góry-tył
//   {-1.0f, 1.0f, -1.0f},// lewy-górny-tył
//   {-1.0f, -1.0f, 1.0f},//lewy-dolny-przód
//   { 1.0f, -1.0f, 1.0f},// prawy-dolny-przód
//   { 1.0f, 1.0f, 1.0f}, // prawy-góry-przód
//   {-1.0f, 1.0f, 1.0f},// lewy-górny-przód
// };

static float zoom = 200.0f; 

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Vertex rotatePoint(Vertex p, float angleX, float angleY);
void checkFloorCollision(PhysicsObject* obj);
SDL_Point projectOrtho(Vertex v, Camera cam, float scale );
SDL_Point project(Vertex v, Camera cam);
SDL_Point projectDynamic(Vertex v, Camera cam);
void renderText(SDL_Renderer* renderer, const char* text, Vertex position,Camera cam, TTF_Font* font, SDL_Color color);
void drawline(SDL_Renderer* renderer, Vertex v1, Vertex v2, Camera cam);
void drawAxesWithLabels(SDL_Renderer* renderer, Camera cam, TTF_Font * font);
void drawFloor(SDL_Renderer* renderer, Camera cam, int size, float spacing);

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

  Vertex rotated = rotatePoint(relative, cam.pitch, cam.yaw);

  float aspect = (float)WIDTH / HEIGHT;
  float fovRad = cam.fov *(M_PI/180.0f);
  float tanHalfFOV = tanf(fovRad * 0.5f); 
  SDL_Point p;
  if(rotated.z > 0.1f) {
    p.x =(int)((rotated.x / (rotated.z * tanHalfFOV *aspect)) * (WIDTH / 2)+ WIDTH/2);
    p.y =(int)((-rotated.y / (rotated.z * tanHalfFOV)) * (HEIGHT/2) + HEIGHT /2);
  }else {
    p.x = -1000;
    p.y = -1000;
  }
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
  SDL_Point p1 = projectDynamic(v1,cam);
  SDL_Point p2 = projectDynamic(v2,cam);
  SDL_RenderDrawLine(renderer, p1.x, p1.y, p2.x, p2.y);
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
    Vertex textPos = {-0.5f,i,0};
    renderText(renderer,label, textPos, cam,font,colorX);
  }

  for(int i=1; i <=10; i++) {
    Vertex tickTop = {i, 0.2f, 0};
    Vertex tickBottom = {i, -0.2f,0};
    SDL_SetRenderDrawColor(renderer,0,255,0,255);
    drawline(renderer, tickTop, tickBottom, cam);
    sprintf(label,"%d",i);
    Vertex textPos = {-0.5f,i,0};
    renderText(renderer,label, textPos, cam,font,colorY);
  }

  for(int i=1; i <=10; i++) {
    Vertex tickTop = {i, 0.2f, 0};
    Vertex tickBottom = {i, -0.2f,0};
    SDL_SetRenderDrawColor(renderer,0,0,255,255);
    drawline(renderer, tickTop, tickBottom, cam);
    sprintf(label,"%d",i);
    Vertex textPos = {-0.5f,i,0};
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
void drawFloor(SDL_Renderer* renderer, Camera cam, int size, float spacing) {
  // Jaśniejszy kolor podłogi dla lepszej widoczności
  SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);

  // Rysowanie linii równoległych do osi Z (biegnących wzdłuż osi X)
  for (int i = -size; i <= size; i++) {
    Vertex start = {-size * spacing, FLOOR_HEIGHT, i * spacing};
    Vertex end = {size * spacing, FLOOR_HEIGHT, i * spacing};
    drawline(renderer, start, end, cam);
  }
  
  // Rysowanie linii równoległych do osi X (biegnących wzdłuż osi Z)
  for (int i = -size; i <= size; i++) {
    Vertex start = {i * spacing, FLOOR_HEIGHT, -size * spacing};
    Vertex end = {i * spacing, FLOOR_HEIGHT, size * spacing};
    drawline(renderer, start, end, cam);
  }
}
void checkFloorCollision(PhysicsObject* obj){

  if (obj->position.y < FLOOR_HEIGHT) {
      obj->position.y = FLOOR_HEIGHT;
    if(obj->velocity.y <0) {
      obj-> velocity.y = -obj->velocity.y * 0.5f;
      if(fabsf(obj->velocity.y) < 0.1f) {
        obj->velocity.y = 0.0f;
      }

      obj->velocity.x *= 0.9f;
      obj->velocity.z *= 0.9f;
    }
  }
}
int main()
{

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
   SDL_Window* window = SDL_CreateWindow("Raytracing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH,HEIGHT, SDL_WINDOW_OPENGL);
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
   TTF_Font* font = TTF_OpenFont("/usr/share/fonts/TTF/Hack-Regular.ttf",12);
   if(!font){
    printf("błąd ładowania czcionki: %s\n", TTF_GetError());
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
   }
   // zmienne - kąty
   float rotationX = 0.0f;
   float rotationY = 0.0f;

   int quit = EXIT_SUCCESS;
   SDL_Event event;
   Uint32 lastTime = SDL_GetTicks64();
   float deltaTime = 0.0f;

   while(!quit) {
    //obsluga deltatime
    
    Uint32 currentTime = SDL_GetTicks();
    deltaTime = (currentTime - lastTime) / 1000.0f; // Konwersja na sekundy
    lastTime = currentTime;
    
      while (SDL_PollEvent(&event))
      {
        if(event.type == SDL_QUIT) 
          quit = 1;
        if (event.type == SDL_KEYDOWN) 
        {
          float moveSpeed = 5.0f;
          float rotationSpeed = 2.0f;

            switch(event.key.keysym.sym) 
            {
              case SDLK_w:  // Do przodu
                camera.physics.position.z += moveSpeed;
                break;
              case SDLK_s:  // Do tyłu
                camera.physics.position.z -= moveSpeed;
                break;
              case SDLK_a:  // W lewo
                camera.physics.position.x -= moveSpeed;
                break;
              case SDLK_d:  // W prawo
                camera.physics.position.x += moveSpeed;
                break;
              case SDLK_UP:  // Obrót w górę
                camera.pitch -= rotationSpeed;
                break;
              case SDLK_DOWN:  // Obrót w dół
                camera.pitch += rotationSpeed;
                break;
              case SDLK_LEFT:  // Obrót w lewo
                camera.yaw -= rotationSpeed;
                break;
              case SDLK_RIGHT:  // Obrót w prawo
                camera.yaw += rotationSpeed;
                break;
              case SDLK_o: // Przełączanie projekcji
                useOrtho = !useOrtho;
                break;
              case SDLK_e: // Zoom +
                zoom += 100.0f;
                break;
              case SDLK_q: // Zoom -
                zoom -= 100.0f;
                break;
            }
        } 
        if (event.type == SDL_MOUSEMOTION)   
        {
          if( event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
            {
              rotationY += event.motion.xrel * 1.0f; 
              rotationX += event.motion.yrel * 1.1f;
            }
        
        }
      }


      updatePhysics(&camera.physics,deltaTime);

      //czyszczenie ekranu + czarne tło
      SDL_SetRenderDrawColor(renderer, 0,0,0,255);
      SDL_RenderClear(renderer);
      
      drawFloor(renderer,camera,10,1.0f);

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
