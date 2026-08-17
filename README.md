# Smart Village Scenery 🌾

A real-time animated 2D village scenery built with **C++ and OpenGL (GLUT)**. The scene features a fully interactive day/night cycle, dynamic weather, live traffic, and an emergency ambulance simulation — all rendered from scratch using primitive OpenGL shapes.

![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%20%2F%20GLUT-green)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

## Features

- 🌞🌙 **Day / Night cycle** — dynamic sky gradient, sun, moon, and twinkling stars
- 🌧️ **Rain & lightning** — thousands of animated raindrops with ground splash effects and random lightning flashes
- 🚑 **Emergency ambulance mode** — ambulance leaves the hospital, flashes sirens, and crosses traffic while signals turn red
- 🚦 **Live traffic simulation** — cars, buses, and a school bus that respect traffic signals and keep a safe following distance
- 🚶 **Pedestrians** — animated people crossing at the zebra crossing, with automatic umbrellas during rain
- 🏘️ **Village environment** — houses with smoke-emitting chimneys, hospital, pharmacy, windmill, trees, flowers, lamp posts, and more
- 🌊 **River scene** — a rowing boat, a fisherman with a jumping fish, and a flock of birds flying overhead
- 🎮 **Fully interactive** — toggle day/night, rain, emergency mode, and boat movement in real time via keyboard

## Controls

| Key | Action |
|-----|--------|
| `D` | Switch to Day mode |
| `N` | Switch to Night mode |
| `R` | Toggle Rain |
| `E` | Toggle Emergency (Ambulance) mode |
| `B` | Toggle Boat movement |
| `ESC` | Exit the program |

## Tech Stack

- **Language:** C++
- **Graphics API:** OpenGL (via `GL/glut.h`)
- **Windowing/Input:** GLUT / freeglut
- **Platform:** Windows (`windows.h` dependency)

## Prerequisites

- A C++ compiler (MinGW / MSVC)
- [freeglut](http://freeglut.sourceforge.net/) development libraries installed and linked

## Build & Run

### Using MinGW (g++)

```bash
g++ main.cpp -o SmartVillage.exe -lfreeglut -lopengl32 -lglu32
./SmartVillage.exe
```

### Using Visual Studio

1. Create a new **Empty C++ Project**.
2. Add `project_final_sunday_monday_close.cpp` to the project.
3. Install `freeglut` via NuGet or link the `glut32.lib` / `freeglut.lib` manually.
4. Add the include and library directories for GLUT/OpenGL in project settings.
5. Build and run.

## Project Structure

```
main.cpp   # Single-file source (all rendering + logic)
```

## How It Works (Overview)

- `display()` renders every visible object in layered order (sky → environment → buildings → road → vehicles → people → river → weather).
- `update()` runs on a 16ms timer, advancing all positions, animation states, and timers, then triggers a redraw.
- `keyboard()` handles key presses to switch modes (day/night/rain/emergency/boat).
- All 2D transformations (translation, rotation, scaling/reflection) are done via `glTranslatef`, `glRotatef`, and `glScalef` under an orthographic `gluOrtho2D` projection.

## License

This project was created for academic purposes as part of a Computer Graphics course.

## Author

Md Sakibur Rahman