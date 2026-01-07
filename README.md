# 🏎️ OpenGL-RacingCar 

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C.svg)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3%2B-green.svg)

> A high-performance 3D car racing simulation built from scratch using C++, OpenGL, and GLSL.

---

## 📖 Table of Contents
- [Overview](#-overview)
- [Features](#-features)
- [Tech Stack](#-tech-stack)
- [Installation & Build](#-installation--build)
- [Controls](#-controls)
- [Technical Details](#-technical-details)
- [Screenshots](#-screenshots)

---

## 📝 Overview

**OpenGL-RacingCar** is a custom graphics engine and racing game developed to demonstrate advanced rendering techniques and physics simulations. Unlike games built on Unity or Unreal, this project handles all matrix transformations, shader compilation, and render loops manually.

The objective was to create a functional racing experience while implementing a core programmable rendering pipeline.

---

## ✨ Features

### 🎨 Graphics Engine
* **Programmable Shaders:** Custom Vertex and Fragment shaders handling transformation and coloring.
* **Lighting System:** Implements the Blinn-Phong lighting model for realistic ambient, diffuse, and specular reflection.
* **Texture Mapping:** UV mapped textures for track asphalt, terrain, and vehicle skins.
* **Skybox:** Cubemap implementation for immersive backgrounds.

### 🚗 Gameplay & Physics
* **Vehicle Physics:** Acceleration, braking, and friction mechanics.
* **Collision Detection:** AABB (Axis-Aligned Bounding Box) logic for track boundaries.
* **Camera System:** Toggleable views (Third-person chase, First-person, Top-down).
* **Lap System:** Checkpoint-based lap timing and counter.

---

## 🛠 Tech Stack

* **Language:** C++17
* **Graphics API:** OpenGL 3.3 Core Profile
* **Window Manager:** [GLFW](https://www.glfw.org/)
* **Math:** [GLM](https://github.com/g-truc/glm)
* **Asset Loading:** [Assimp](https://github.com/assimp/assimp) (Models), [stb_image](https://github.com/nothings/stb) (Textures)

---

## ⚙ Installation & Build

### Prerequisites
* CMake (3.10 or higher)
* C++ Compiler (GCC, Clang, or MSVC)
* OpenGL Drivers

### Build Instructions (Linux/Mac)
```bash
# 1. Clone the repository
git clone [https://github.com/your-username/your-repo-name.git](https://github.com/your-username/your-repo-name.git)
cd your-repo-name

# 2. Create build directory
mkdir build && cd build

# 3. Compile
cmake ..
make

# 4. Run
./RacingGame
