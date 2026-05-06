# Movie Cube Project Walkthrough

This document explains the current project structure and the code that was split out of the original large `Cube.cpp` file. Use it as a learning guide while reading the source.

## What Changed

Before the refactor, `src/Cube.cpp` contained almost everything: cube rendering, camera movement, shader source strings, shader compilation, video frame loading, texture uploading, and audio playback.

Now the project is separated by responsibility:

```text
src/
|-- Cube.cpp
|-- Camera.cpp
|-- ShaderProgram.cpp
|-- Video.cpp
|-- headers/
|   |-- Camera.h
|   |-- ShaderProgram.h
|   |-- Video.h
|-- shaders/
|   |-- video_cube.vert
|   |-- video_cube.frag
|-- main.cpp
```

The main idea is simple: each file should own one type of work.

- `Cube.cpp` owns the window, OpenGL scene loop, cube mesh, and drawing.
- `Camera.cpp` owns camera movement and mouse look.
- `Video.cpp` owns loading `.ppm` frames, updating OpenGL textures, fallback video colors, and audio playback.
- `ShaderProgram.cpp` owns reading, compiling, and linking shader files.
- `src/headers/` owns declarations shared between `.cpp` files.
- `src/shaders/` owns GLSL shader code.

## Runtime Flow

The program starts in `src/main.cpp`:

```cpp
int main()
{
    return RunCubeScene();
}
```

`RunCubeScene()` is implemented in `src/Cube.cpp`. It creates an `Environment`, checks if OpenGL setup succeeded, then runs the render loop.

The high-level flow is:

1. `main()` calls `RunCubeScene()`.
2. `RunCubeScene()` creates `Environment`.
3. `Environment` initializes GLFW, creates the window, initializes GLEW, loads shaders, and enables depth testing.
4. `Environment::Run()` starts audio and creates a `Cube`.
5. Each frame:
   - input is processed,
   - video textures are updated,
   - the scene is rendered,
   - GLFW swaps buffers and polls events.

## `src/Cube.cpp`

This file still contains the main application logic, but it is much smaller now.

### Constants

```cpp
constexpr unsigned int SCR_WIDTH = 1000;
constexpr unsigned int SCR_HEIGHT = 600;
constexpr float FIXED_CUBE_SCALE = 2.4f;
const std::string AUDIO_VIDEO_PATH = "...";
```

- `SCR_WIDTH` and `SCR_HEIGHT` set the window size.
- `FIXED_CUBE_SCALE` controls how large the cube appears.
- `AUDIO_VIDEO_PATH` tells `AudioPlayer` which video file to play audio from.

### `class Cube`

The `Cube` class owns the OpenGL cube geometry and the video textures used on the cube faces.

#### `Cube::Cube(const glm::vec3& pos)`

Creates the cube at a position.

Important work inside:

- Defines `cubeVertices`, which contains 36 vertices.
- Each vertex has 5 floats:
  - 3 floats for position: `x, y, z`
  - 2 floats for texture coordinate: `u, v`
- Creates a VAO and VBO with `glGenVertexArrays` and `glGenBuffers`.
- Uploads vertex data to the GPU with `glBufferData`.
- Tells OpenGL how to read vertex positions and texture coordinates with `glVertexAttribPointer`.

The VAO remembers the vertex layout. The VBO stores the actual vertex data.

#### `Cube::~Cube()`

Deletes the OpenGL VAO and VBO when the cube is destroyed.

This matters because OpenGL objects live on the GPU. If we create them, we should also delete them.

#### `Cube::Update(float time)`

Calls:

```cpp
videoTextures.Update(time);
```

This updates the textures on the cube faces. The `time` value is used to choose which video frame should be shown.

#### `Cube::Draw(GLuint shaderProgram) const`

Draws the cube.

Important work inside:

- Builds a `model` matrix with position and scale.
- Sends the `model` matrix to the shader uniform named `model`.
- Loops over all 6 faces of the cube.
- For each face:
  - sends a small brightness value,
  - binds that face's video texture,
  - draws 6 vertices with `glDrawArrays`.

Why 6 vertices per face? Each cube face is made from 2 triangles, and each triangle has 3 vertices.

### `class Environment`

`Environment` owns the window, OpenGL setup, camera, audio player, shader program, and main loop.

#### `Environment::Environment()`

Initializes the app.

Important work inside:

- Calls `glfwInit()`.
- Requests an OpenGL 3.3 core profile context.
- Creates the window with `glfwCreateWindow`.
- Sets callbacks for window resize and mouse movement.
- Captures the mouse cursor for first-person camera control.
- Initializes GLEW.
- Enables depth testing with `glEnable(GL_DEPTH_TEST)`.
- Loads shader files with `CreateShaderProgramFromFiles`.
- Sets the shader uniform `videoTexture` to texture unit `0`.

If any critical step fails, `ready` stays `false`.

#### `Environment::~Environment()`

Deletes the shader program and calls `glfwTerminate()`.

This cleans up OpenGL and GLFW resources.

#### `Environment::IsReady() const`

Returns whether setup succeeded.

`RunCubeScene()` uses this before starting the loop.

#### `Environment::Run()`

Runs the actual app loop.

Important work inside:

- Starts audio playback.
- Creates a `Cube`.
- Calculates `deltaTime` each frame.
- Processes input.
- Updates cube video textures.
- Renders the cube.
- Swaps the front/back buffers.
- Polls keyboard, mouse, and window events.

`deltaTime` is the time between frames. It makes camera movement speed consistent even if FPS changes.

#### `Environment::FramebufferSizeCallback(...)`

Updates the OpenGL viewport when the window is resized.

Without this, OpenGL might keep rendering using the old window size.

#### `Environment::MouseCallback(...)`

Handles mouse movement.

Important work inside:

- Gets the `Environment` pointer stored in the GLFW window.
- On the first mouse event, stores the initial mouse position.
- Calculates mouse offset from the previous position.
- Sends that offset to `camera.ProcessMouseMovement`.

The mouse offset is what turns the camera.

#### `Environment::ProcessInput(float deltaTime)`

Handles keyboard input.

- `Esc` closes the window.
- Movement keys are passed to `camera.ProcessKeyboard`.

#### `Environment::Render(const Cube& cube) const`

Draws one frame.

Important work inside:

- Clears the screen color and depth buffer.
- Uses the shader program.
- Builds the camera `view` matrix.
- Builds the perspective `projection` matrix.
- Sends both matrices to the shader.
- Calls `cube.Draw(shaderProgram)`.

The render matrices are the core of 3D drawing:

- `model`: where the object is.
- `view`: where the camera is.
- `projection`: how 3D space is projected onto the screen.

#### `RunCubeScene()`

Creates `Environment` and starts it if setup succeeded.

This function exists so `main.cpp` can stay tiny.

## `src/headers/Camera.h` And `src/Camera.cpp`

The `Camera` class stores camera position, direction, speed, and mouse sensitivity.

### Camera Data

```cpp
glm::vec3 Position;
glm::vec3 Front;
glm::vec3 Up;
float Yaw;
float Pitch;
float MovementSpeed;
float MouseSensitivity;
```

- `Position` is where the camera is in the world.
- `Front` is the direction the camera is looking.
- `Up` is the upward direction.
- `Yaw` is left/right rotation.
- `Pitch` is up/down rotation.
- `MovementSpeed` controls keyboard movement speed.
- `MouseSensitivity` controls mouse look speed.

### `Camera::GetViewMatrix() const`

Returns:

```cpp
glm::lookAt(Position, Position + Front, Up);
```

This creates the camera view matrix. It tells OpenGL how the world should look from the camera position.

### `Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime)`

Reads keyboard state from GLFW and moves the camera.

Keys:

- `W`: forward
- `S`: backward
- `A`: left
- `D`: right
- `Space`: up
- `Left Shift`: down

It multiplies movement by `deltaTime`, so movement speed does not depend on frame rate.

### `Camera::ProcessMouseMovement(float xoffset, float yoffset)`

Uses mouse movement to rotate the camera.

Important work inside:

- Multiplies offsets by `MouseSensitivity`.
- Adds horizontal movement to `Yaw`.
- Adds vertical movement to `Pitch`.
- Clamps `Pitch` between `-89` and `89` degrees.
- Calls `UpdateVectors()`.

The pitch clamp prevents the camera from flipping upside down.

### `Camera::UpdateVectors()`

Converts `Yaw` and `Pitch` angles into a direction vector.

This is the math that turns mouse movement into the `Front` vector:

```cpp
front.x = cos(yaw) * cos(pitch);
front.y = sin(pitch);
front.z = sin(yaw) * cos(pitch);
```

Then the result is normalized so it has length `1`.

## `src/headers/ShaderProgram.h` And `src/ShaderProgram.cpp`

This module handles shader loading and compilation.

Shaders used to be string constants inside `Cube.cpp`. Now they are real files in `src/shaders`.

### `ReadFile(const std::filesystem::path& path)`

Private helper function.

Reads an entire text file into a `std::string`.

Used for:

- `src/shaders/video_cube.vert`
- `src/shaders/video_cube.frag`

If the file cannot be opened, it prints an error and returns an empty string.

### `CompileShader(GLenum type, const std::string& source)`

Private helper function.

Compiles one shader.

Important work inside:

- Creates a shader with `glCreateShader`.
- Gives GLSL source code to OpenGL with `glShaderSource`.
- Compiles it with `glCompileShader`.
- Checks compile status.
- Prints shader compile errors if compilation fails.

`type` is usually either:

- `GL_VERTEX_SHADER`
- `GL_FRAGMENT_SHADER`

### `CreateShaderProgramFromFiles(...)`

Public function declared in `ShaderProgram.h`.

Important work inside:

- Reads the vertex shader file.
- Reads the fragment shader file.
- Compiles both shaders.
- Creates a shader program.
- Attaches the shaders to the program.
- Links the program.
- Checks link status.
- Deletes the separate shader objects after linking.
- Returns the final shader program ID.

OpenGL uses the linked program when drawing the cube.

## `src/shaders/video_cube.vert`

This is the vertex shader.

Its job is to position vertices on the screen and pass texture coordinates to the fragment shader.

Inputs:

```glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
```

- `aPos` is the vertex position from the VBO.
- `aTexCoord` is the texture coordinate from the VBO.

Uniforms:

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
```

These matrices transform the vertex from object space to screen space.

Main line:

```glsl
gl_Position = projection * view * model * vec4(aPos, 1.0);
```

This applies the transformations in this order:

1. `model`: object to world
2. `view`: world to camera
3. `projection`: camera to screen

## `src/shaders/video_cube.frag`

This is the fragment shader.

Its job is to decide the final color of each pixel.

Inputs:

```glsl
in vec2 TexCoord;
uniform sampler2D videoTexture;
uniform float brightness;
```

- `TexCoord` tells which part of the texture to sample.
- `videoTexture` is the current face texture.
- `brightness` slightly changes brightness per cube face.

Main line:

```glsl
FragColor = vec4(color * brightness, 1.0);
```

This samples the video texture, applies brightness, and outputs the pixel color.

## `src/headers/Video.h` And `src/Video.cpp`

This module owns video frame loading, texture updating, fallback textures, and audio playback.

### `FACE_COUNT`

```cpp
constexpr int FACE_COUNT = 6;
```

A cube has 6 faces, so the app keeps 6 textures.

### `struct VideoFrame`

Stores one loaded image frame.

Fields:

- `width`: image width
- `height`: image height
- `pixels`: RGB byte data

Each pixel has 3 bytes: red, green, blue.

### `ReadToken(std::ifstream& file)`

Private helper function.

Reads a token from a PPM file and skips comment lines that start with `#`.

PPM files have a small text header before the binary pixel data. This function helps read that header.

### `LoadPpmFrame(const std::filesystem::path& path, VideoFrame& frame)`

Loads one `.ppm` image file into a `VideoFrame`.

Important work inside:

- Opens the file in binary mode.
- Checks that the magic value is `P6`.
- Reads width, height, and max color value.
- Requires max color value to be `255`.
- Reads RGB bytes into `frame.pixels`.
- Stores width and height.
- Returns `true` when loading succeeds.

The app uses `.ppm` because it is simple to parse compared to formats like `.png` or `.jpg`.

### `FaceVideoTextures::FaceVideoTextures()`

Constructor for the video texture manager.

Important work inside:

- Calls `LoadFrameLists()`.
- Creates 6 OpenGL textures.
- Sets texture wrapping and filtering.
- Generates a fallback frame for each face.
- Uploads fallback frames to the GPU.

This means the cube can still show animated colors even if no `.ppm` frames are found.

### `FaceVideoTextures::~FaceVideoTextures()`

Deletes the 6 OpenGL textures.

This frees GPU texture memory.

### `FaceVideoTextures::Update(float time)`

Updates all cube face textures for the current time.

For each face:

- Tries to load the correct video frame for that time.
- If loading succeeds, uploads it.
- If loading fails, generates a fallback frame and uploads that instead.

### `FaceVideoTextures::Bind(int face) const`

Binds one face texture to OpenGL texture unit `0`.

`Cube::Draw()` calls this before drawing each face.

### `FaceVideoTextures::LoadFrameLists()`

Scans folders under:

```text
assets/video_faces/
```

Expected folders:

```text
face0
face1
face2
face3
face4
face5
```

For each folder, it collects `.ppm` files and sorts them by filename.

If only `face0` has frames, the app can reuse face 0 frames for the other faces.

### `FaceVideoTextures::LoadFrameForTime(int face, float time, VideoFrame& frame) const`

Chooses which frame should be shown at a specific time.

Important line:

```cpp
const std::size_t frameIndex = static_cast<std::size_t>(time * fps) % paths.size();
```

This turns time into a frame number:

- `time * fps` means "how many frames should have passed".
- `% paths.size()` loops back to the start when the video reaches the end.

Then it calls `LoadPpmFrame`.

### `FaceVideoTextures::GenerateFallbackFrame(...) const`

Creates a generated RGB image when no video frame is available.

It fills a 256x256 image with animated color waves and a small play-triangle shape.

This is not real video. It is a visual fallback so the cube does not appear blank.

### `FaceVideoTextures::UploadFrame(int face, const VideoFrame& frame)`

Uploads a `VideoFrame` to an OpenGL texture.

If the frame size changed:

```cpp
glTexImage2D(...)
```

This creates or recreates the full texture.

If the size is the same:

```cpp
glTexSubImage2D(...)
```

This only replaces the pixels. It is better for regular video updates because the texture already exists.

### `AudioPlayer::Start(const std::string& videoPath)`

Starts audio playback on Windows.

Important work inside:

- Checks if audio already started.
- Checks if the video file exists.
- Builds an `ffplay` command.
- Starts `ffplay` hidden with `CreateProcessA`.
- Stores process handles so playback can be stopped later.

This uses `ffplay` instead of decoding audio in C++.

### `AudioPlayer::Stop()`

Stops the `ffplay` process if it is running.

It terminates the process and closes Windows handles.

### `AudioPlayer::~AudioPlayer()`

Calls `Stop()`.

This makes sure audio stops when the app closes.

## Header Files

Header files in `src/headers` contain declarations. Source files contain implementations.

For example:

- `Camera.h` says what a `Camera` can do.
- `Camera.cpp` says how those functions work.

This is common C++ organization. It lets different `.cpp` files use a class without copying the full implementation everywhere.

## Why The Refactor Helps

The project is easier to learn and change now because each topic has a home.

- Want to change camera speed? Open `Camera.h`.
- Want to change mouse math? Open `Camera.cpp`.
- Want to change shader color? Open `src/shaders/video_cube.frag`.
- Want to change matrix transforms? Open `Cube.cpp`.
- Want to change video frame loading? Open `Video.cpp`.
- Want to change shader loading? Open `ShaderProgram.cpp`.

This also makes build errors easier to understand because each file has a smaller purpose.

## Suggested Learning Order

1. Start with `src/main.cpp`.
2. Read `RunCubeScene()` in `src/Cube.cpp`.
3. Read `Environment::Environment()` to understand setup.
4. Read `Environment::Run()` to understand the loop.
5. Read `Cube::Draw()` to understand drawing.
6. Read `Camera.cpp` to understand movement.
7. Read `ShaderProgram.cpp` to understand shader loading.
8. Read `Video.cpp` to understand texture updates.
9. Read the GLSL files in `src/shaders`.

## Build Command

Use:

```bash
cpp-starter-cli build
```

The important compile idea is that all `.cpp` files are compiled together:

```text
src/Camera.cpp
src/Cube.cpp
src/ShaderProgram.cpp
src/Video.cpp
src/main.cpp
```

That is why moving code into new `.cpp` files works: the build includes them all.
