> [!WARNING]
> This library is still early in development and subject to change frequently. Use at your own risk!

# CGE

CGE (aka Siege): A simple C++ Game Engine.

This project is a port of my **Rust Game Engine** to C++.

Supports Windows, Linux, and MacOS, compiled with Clang + CMake.

---

## Example Image

<p align="center">
  </br>
  <img alt="CGE-Example" src="https://github.com/SuperLuigiLinked/CGE/assets/65352263/4a251db0-8e54-4270-bce9-aa7a70259244">
</p>

## Example Code

```
    render.clear();

    render.backcolor = cge::RGBA::splat(0.0f, 0.0f, 0.25f);

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = { -1.0f, -1.0f },
                .st = { 0xFFFF0000 },
            },
            cge::Vertex {
                .xyzw = { 0.5f, 0.0f },
                .st = { 0x00FF0000 },
            },
            cge::Vertex {
                .xyzw = { -1.0f, 1.0f },
                .st = { 0xFFFF0000 },
            }
        }
    );

    render.triangle(
        std::array
        {
            cge::Vertex {
                .xyzw = { 1.0f, -1.0f },
                .st = { 0xFF00FF00 },
            },
            cge::Vertex {
                .xyzw = { -0.5f, 0.0f },
                .st = { 0x0000FF00 },
            },
            cge::Vertex {
                .xyzw = { 1.0f, 1.0f },
                .st = { 0xFF00FF00 },
            }
        }
    );
```

---
