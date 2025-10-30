# 🎮 3D Physics Engine with SDL2

![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL2-blue?style=flat)
![License](https://img.shields.io/badge/license-MIT-green)

Zaawansowany silnik graficzny 3D napisany w C z wykorzystaniem SDL2, oferujący zarówno projekcję perspektywiczną jak i ortograficzną, system fizyki oraz kontrolę w stylu FPS.

---

## 📋 Spis Treści

- [Funkcje](#-funkcje)
- [Wymagania](#-wymagania)
- [Instalacja](#-instalacja)
- [Kompilacja](#-kompilacja)
- [Sterowanie](#-sterowanie)
- [Architektura](#-architektura)
- [Szczegóły Techniczne](#-szczegóły-techniczne)
- [Roadmap](#-roadmap)
- [Licencja](#-licencja)

---

## ✨ Funkcje

### Rendering 3D
- ✅ **Dual Projection System**
  - Projekcja perspektywiczna z dynamicznym FOV (30° - 120°)
  - Projekcja ortograficzna z regulowaną skalą
  - Płynne przełączanie między trybami (klawisz `Q`)

- ✅ **Advanced Camera System**
  - Kamera FPS z kontrolą myszy i klawiaturą
  - Rotacja yaw/pitch z zabezpieczeniem przed gimbal lock
  - Smooth mouse look z regulowaną czułością
  - Dynamiczny FOV (Field of View)

- ✅ **Rendering Pipeline**
  - Near/Far plane clipping (0.02 - 500 jednostek)
  - Przestrzeń kamery z transformacjami 3D
  - Grubo-liniowy rendering (3px) dla lepszej widoczności
  - Optymalizacja odrzucania obiektów poza frustum

### System Fizyki
- ✅ **Physics Engine**
  - Symulacja grawitacji (-9.81 m/s²)
  - Kolizje z podłogą z odbiciami
  - Tłumienie prędkości (friction)
  - Delta-time based physics dla płynnej animacji
  - System sił i mas dla obiektów fizycznych

### Wizualizacja
- ✅ **3D Grid & Axes**
  - Siatka podłogi 20x20 jednostek
  - Kolorowe osie współrzędnych (RGB = XYZ)
  - Znaczniki co 1 jednostkę z etykietami tekstowymi
  - Dynamiczne renderowanie tekstu TTF

- ✅ **UI Elements**
  - Renderowanie tekstu w przestrzeni 3D
  - Etykiety osi z kolorem RGB
  - Adaptacyjne skalowanie UI

---

## 🔧 Wymagania

### Zależności Systemowe
```bash
# Arch Linux
sudo pacman -S sdl2 sdl2_ttf gcc make pkg-config

# Ubuntu/Debian
sudo apt install libsdl2-dev libsdl2-ttf-dev gcc make pkg-config

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel gcc make pkgconfig
```

### Minimalne Wymagania
- **Kompilator:** GCC 7.0+
- **SDL2:** ≥ 2.0.0
- **SDL2_ttf:** ≥ 2.0.0
- **System:** Linux (testowane na Arch Linux KDE)
- **Czcionka:** `/usr/share/fonts/TTF/Hack-Regular.ttf` (lub inna czcionka TTF)

---

## 📥 Instalacja

```bash
# Klonowanie repozytorium
git clone https://github.com/dg-neural-23/physic-engine-c-sml2.git
cd physic-engine-c-sml2

# Instalacja zależności (Arch Linux)
sudo pacman -S sdl2 sdl2_ttf
```

---

## 🔨 Kompilacja

```bash
# Kompilacja projektu
make

# Uruchomienie
./raytracing

# Czyszczenie plików binarnych
make clean
```

### Konfiguracja Makefile
```makefile
CC = gcc
CFLAGS = -Wall -g $(shell pkg-config --cflags SDL2_ttf)
LIBS = -lSDL2 -lm $(shell pkg-config --libs SDL2_ttf)
```

---

## 🎮 Sterowanie

### 🖱️ Mysz
| Akcja | Funkcja |
|-------|---------|
| **Ruch myszy** | Rozglądanie się (FPS look) |
| **Czułość** | 0.1° na pixel |

### ⌨️ Klawiatura

#### Ruch Kamery (ZXCDSV)
| Klawisz | Kierunek |
|---------|----------|
| **Z** | Do przodu (+Z) |
| **A** | Do tyłu (-Z) |
| **X** | W lewo (-X) |
| **S** | W prawo (+X) |
| **C** | Do góry (+Y) |
| **D** | Do dołu (-Y) |

#### Rotacja Manualna (VFBG)
| Klawisz | Rotacja |
|---------|---------|
| **V** | Pitch up (w górę) |
| **F** | Pitch down (w dół) |
| **B** | Yaw left (w lewo) |
| **G** | Yaw right (w prawo) |

#### Kontrola Widoku
| Klawisz | Funkcja |
|---------|---------|
| **Q** | Przełącznik Perspektywa ↔ Ortho |
| **W** | FOV + (wider view, 120° max) |
| **E** | FOV - (zoom in, 30° min) |

### 🎯 Parametry Ruchu
- **Prędkość ruchu:** 1.0 jednostka/klatkę
- **Prędkość rotacji:** 2.0°/klatkę
- **Czułość myszy:** 0.1°/pixel

---

## 🏗️ Architektura

### Struktura Danych

```c
typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    Vertex position;    // Pozycja w świecie
    Vertex velocity;    // Prędkość [m/s]
    Vertex force;       // Siła [N]
    float mass;         // Masa [kg]
} PhysicsObject;

typedef struct {
    PhysicsObject physics;
    float yaw;          // Rotacja Y [-180°, 180°]
    float pitch;        // Rotacja X [-89°, 89°]
    float fov;          // Field of View [30°, 120°]
} Camera;
```

### Pipeline Renderowania

```
World Space
    ↓
[Translate to Camera]
    ↓
Camera Space
    ↓
[Rotate by Yaw/Pitch]
    ↓
View Space
    ↓
[Clip to Near/Far]
    ↓
[Perspective Divide]
    ↓
NDC (Normalized Device Coords)
    ↓
[Viewport Transform]
    ↓
Screen Space
```

### Moduły Główne

1. **Physics Engine** (`updatePhysics`)
   - Integracja Eulera
   - F = ma
   - Kolizje z podłogą

2. **Projection System**
   - `projectOrtho()` - Projekcja ortograficzna
   - `project()` - Projekcja perspektywiczna
   - `toCameraSpace()` - Transformacja do przestrzeni kamery
   - `clipLineToNearFar()` - Clipping frustum

3. **Rendering**
   - `drawThickLine()` - Rysowanie linii 3px
   - `drawFloor()` - Siatka podłogi
   - `drawAxesWithLabels()` - Osie 3D z etykietami
   - `renderText()` - Tekst w przestrzeni 3D

---

## 🔬 Szczegóły Techniczne

### Projekcja Perspektywiczna

```c
// FOV-based perspective projection
float aspect = WIDTH / HEIGHT;
float fovRad = camera.fov * (PI / 180.0f);
float tanHalf = tan(fovRad * 0.5f);

float x_ndc = r.x / (r.z * tanHalf * aspect);
float y_ndc = -r.y / (r.z * tanHalf);

screen_x = (x_ndc + 1.0) * 0.5 * WIDTH;
screen_y = (y_ndc + 1.0) * 0.5 * HEIGHT;
```

### Parametry Fizyki

| Parametr | Wartość |
|----------|---------|
| **Grawitacja** | -9.81 m/s² |
| **Wysokość podłogi** | 0.0m |
| **Współczynnik odbicia** | 0.5 |
| **Tarcie** | 0.9 |
| **Threshold zatrzymania** | 0.1 m/s |

### Clipping Planes

| Plane | Wartość |
|-------|---------|
| **Near Plane** | 0.02 jednostki |
| **Far Plane** | 500.0 jednostek |

### Parametry Kamery

| Parametr | Default | Range |
|----------|---------|-------|
| **Position** | (5, 1.7, -10) | Unlimited |
| **FOV** | 90° | 30° - 120° |
| **Pitch** | 0° | -89° do 89° |
| **Yaw** | 0° | -180° do 180° |

---

## 🚀 Roadmap

### Wersja 1.1 (W toku)
- [ ] Renderowanie sześcianów 3D
- [ ] System materiałów i kolorów
- [ ] Zaawansowane oświetlenie (ambient, diffuse)

### Wersja 1.2 (Planowane)
- [ ] Ray tracing dla cieni
- [ ] Tekstury i UV mapping
- [ ] Import modeli OBJ

### Wersja 2.0 (Przyszłość)
- [ ] Skybox
- [ ] Particle system
- [ ] Collision detection między obiektami
- [ ] Post-processing effects

---

## 📊 Performance

### Benchmarki (AMD Ryzen 5600X, RTX 3060)
- **FPS:** 60 (VSync enabled)
- **Frame time:** ~16ms
- **Resolution:** 900x600
- **Draw calls:** ~400-500 linii/klatkę

### Optymalizacje
- ✅ Frustum culling
- ✅ VSync dla stabilnego framerate
- ✅ Delta-time physics
- ✅ Thick line batching

---

## 🐛 Znane Problemy

- [ ] Brak obsługi wielu obiektów
- [ ] Czcionka zakodowana na sztywno (`/usr/share/fonts/TTF/Hack-Regular.ttf`)
- [ ] Brak menu konfiguracji
- [ ] Gimbal lock przy pitch = ±90°

---

## 🤝 Contributing

Pull requesty mile widziane! Dla większych zmian, proszę najpierw otworzyć issue.

```bash
# Fork projektu
git checkout -b feature/AmazingFeature
git commit -m 'Add some AmazingFeature'
git push origin feature/AmazingFeature
# Otwórz Pull Request
```

---

## 📝 Licencja

MIT License - zobacz plik `LICENSE` dla szczegółów.

---

## 👨‍💻 Autor

**dg-neural-23**
- GitHub: [@dg-neural-23](https://github.com/dg-neural-23)
- Projekt: [physic-engine-c-sml2](https://github.com/dg-neural-23/physic-engine-c-sml2)

---

## 🙏 Podziękowania

- **SDL2** - Simple DirectMedia Layer
- **SDL2_ttf** - TrueType Font rendering
- **Hack Font** - Domyślna czcionka projektu
- Społeczność **Arch Linux KDE**

---

<div align="center">

**⭐ Jeśli podoba Ci się ten projekt, zostaw gwiazdkę! ⭐**

Made with ❤️ using C and SDL2

</div>
