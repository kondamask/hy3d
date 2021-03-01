#include "hy3d_engine.h"
#include "hy3d_renderer.cpp"
#include <intrin.h>

static void LoadBitmap(loaded_bitmap *bmp, debug_read_file *ReadFile, char *filename)
{
    debug_read_file_result file = ReadFile(filename);
    if (file.size != 0 && file.content)
    {
        bitmap_header *header = (bitmap_header *)file.content;
        u8 *address = (u8 *)file.content + header->bitmapOffset;
        u32 *pixels = (u32 *)address;
        bmp->pixels = pixels;
        bmp->height = (i16)header->height;
        bmp->width = (i16)header->width;

        if (header->compression == 3)
        {
            u32 alphaMask = ~(header->redMask | header->greenMask | header->blueMask);
            u32 redShift, greenShift, blueShift, alphaShift;
            _BitScanForward((unsigned long *)&redShift, header->redMask);
            _BitScanForward((unsigned long *)&greenShift, header->greenMask);
            _BitScanForward((unsigned long *)&blueShift, header->blueMask);
            _BitScanForward((unsigned long *)&alphaShift, alphaMask);
            if (alphaShift == 24 && redShift == 16 && greenShift == 8 && blueShift == 0)
                return;
            u32 *dest = pixels;
            for (i32 y = 0; y < header->height; y++)
            {
                for (i32 x = 0; x < header->width; x++)
                {
                    u32 c = *dest;
                    *dest++ = ((((c >> alphaShift) & 0xFF) << 24) |
                               (((c >> redShift) & 0xFF) << 16) |
                               (((c >> greenShift) & 0xFF) << 8) |
                               (((c >> blueShift) & 0xFF) << 0));
                }
            }
        }
    }
}

static void Initialize(hy3d_engine *e, engine_state *state, engine_memory *memory)
{
    e->input = {};

    e->space.left = -1.0f;
    e->space.right = 1.0f;
    e->space.top = 1.0f;
    e->space.bottom = -1.0f;
    e->space.width = e->space.right - e->space.left;
    e->space.height = e->space.top - e->space.bottom;
    e->screenTransformer.xFactor = e->pixelBuffer.width / e->space.width;
    e->screenTransformer.yFactor = e->pixelBuffer.width / e->space.height;

    state->peepoCube.orientation = {0.0f, 0.0f, 0.0f};
    state->peepoCubeZ = 2.0f;
    state->peepoCubeDrawOutline = false;

    state->crateCube.orientation = {0.3f, -0.4f, 0.0f};
    state->crateCubeZ = 2.0f;

    LoadBitmap(&state->background, memory->DEBUGReadFile, "city_bg_purple.bmp");
    state->background.opacity = 1.0f;
    LoadBitmap(&state->logo, memory->DEBUGReadFile, "hy3d_gimp.bmp");
    state->logo.opacity = 0.3f;
    state->logoVelX = 100.0f;
    state->logoVelY = 80.0f;
    LoadBitmap(&state->peepoTexture, memory->DEBUGReadFile, "peepo.bmp");
    state->peepoTexture.opacity = 1.0f;
    LoadBitmap(&state->crateTexture, memory->DEBUGReadFile, "crate.bmp");
    state->crateTexture.opacity = 1.0f;

    memory->isInitialized = true;
    e->frameStart = std::chrono::steady_clock::now();
}

static void Update(hy3d_engine *e, engine_state *state)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e->frameStart;
    f32 dt = frameTime.count();
    e->frameStart = frameEnd;


    // Cube Control
    f32 speed = 2.5f * dt;
    if (e->input.keyboard.isPressed[UP])
    {
        state->peepoCube.orientation.thetaX += speed;
    }
    if (e->input.keyboard.isPressed[DOWN])
    {
        state->peepoCube.orientation.thetaX -= speed;
    }
    if (e->input.keyboard.isPressed[LEFT])
    {
        state->peepoCube.orientation.thetaY += speed;
    }
    if (e->input.keyboard.isPressed[RIGHT])
    {
        state->peepoCube.orientation.thetaY -= speed;
    }
    if (e->input.keyboard.isPressed[Q])
        state->peepoCube.orientation.thetaZ += speed;
    if (e->input.keyboard.isPressed[W])
        state->peepoCube.orientation.thetaZ -= speed;

    // Logo Control
    if (state->logo.posX < 0 || (state->logo.posX + state->logo.width) > e->pixelBuffer.width)
        state->logoVelX = -state->logoVelX;
    if (state->logo.posY < 0 || (state->logo.posY + state->logo.height) > e->pixelBuffer.height)
        state->logoVelY = -state->logoVelY;
    state->logo.posX += state->logoVelX * dt;
    state->logo.posY += state->logoVelY * dt;

    state->logo.opacity += (f32)e->input.mouse.WheelDelta() * 0.0005f;
    if (state->logo.opacity > 1.0f)
        state->logo.opacity = 1.0f;
    if (state->logo.opacity < 0.0f)
        state->logo.opacity = 0.0f;

    if (e->input.keyboard.isPressed[R])
    {
        state->peepoCube.orientation.thetaX = 0.0f;
        state->peepoCube.orientation.thetaY = 0.0f;
        state->peepoCube.orientation.thetaZ = 0.0f;
    }
    f32 offsetZ = 1.0f * dt;
    if (e->input.keyboard.isPressed[Z])
        state->peepoCubeZ -= offsetZ;
    if (e->input.keyboard.isPressed[X])
        state->peepoCubeZ += offsetZ;
    state->peepoCubeDrawOutline = e->input.keyboard.isPressed[CTRL];
}

static void Render(hy3d_engine *e, engine_state *state)
{
    DrawBitmap(&state->background, 0, 0, &e->pixelBuffer);
    DrawBitmap(&state->logo, (i32)(state->logo.posX), (i32)(state->logo.posY), &e->pixelBuffer);

    //state->crateCube = MakeCubeSkinned(1.0f, state->crateCube.orientation);
    //mat3 rotation = RotateX(state->crateCube.orientation.thetaX) *
    //                RotateY(state->crateCube.orientation.thetaY) *
    //                RotateZ(state->crateCube.orientation.thetaZ);
    //vec3 translation = {0.0f, 0.0f, state->crateCubeZ};
    //DrawObjectTextured(state->crateCube.vertices, state->crateCube.nVertices,
    //                   state->crateCube.indices, state->crateCube.nIndices,
    //                   rotation, translation, &state->crateTexture,
    //                   &e->pixelBuffer, &e->screenTransformer);

    state->peepoCube = MakeCubeSkinned(1.0f, state->peepoCube.orientation);
    state->peepoCubeAxis = MakeAxis3D({-0.0f, -0.0f, -0.0f}, 1.0f, state->peepoCube.orientation);
    mat3 rotation = RotateX(state->peepoCube.orientation.thetaX) *
               RotateY(state->peepoCube.orientation.thetaY) *
               RotateZ(state->peepoCube.orientation.thetaZ);
    vec3 translation = {0.0f, 0.0f, state->peepoCubeZ};
    DrawObjectTextured(state->peepoCube.vertices, state->peepoCube.nVertices,
                       state->peepoCube.indices, state->peepoCube.nIndices,
                       rotation, translation, &state->peepoTexture,
                       &e->pixelBuffer, &e->screenTransformer);
    DrawAxis3D(state->peepoCubeAxis.vertices, state->peepoCubeAxis.nVertices,
               state->peepoCubeAxis.lines, state->peepoCubeAxis.nLinesVertices, state->peepoCubeAxis.colors,
               rotation, translation, &e->pixelBuffer, &e->screenTransformer);
    if (state->peepoCubeDrawOutline)
    {
        DrawObjectOutline(state->peepoCube.vertices, state->peepoCube.nVertices,
                          state->peepoCube.lines, state->peepoCube.nLineIndices, {150, 150, 150},
                          rotation, translation, &e->pixelBuffer, &e->screenTransformer);
    }
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender)
{
    engine_state *state = (engine_state *)memory->permanentMemory;

    if (!memory->isInitialized)
        Initialize(&e, state, memory);

    ClearZBuffer(&e.pixelBuffer);
    Update(&e, state);
    Render(&e, state);
}
