#include "hy3d_engine.h"
#include "hy3d_renderer.cpp"
#include <intrin.h>

static void LoadBitmap(loaded_bitmap *bmp, debug_read_file *ReadFile, char *filename)
{
    debug_read_file_result file = ReadFile(filename);
    if (file.size != 0 && file.content)
    {
        bmp->opacity = 1.0f;
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

static void InitializeMemoryArena(memory_arena *arena, u8 *base, size_t size)
{
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}

#define ReserveStructMemory(arena, type) (type *)ReserveMemory(arena, sizeof(type))
#define ReserveArrayMemory(arena, count, type) (type *)ReserveMemory(arena, (count) * sizeof(type))
static void *ReserveMemory(memory_arena *arena, size_t size)
{
    ASSERT(arena->used + size <= arena->size)
    void *result = arena->base + arena->used;
    arena->used += size;
    return result;
}

static mesh ReserveMeshMemory(memory_arena *arena, i32 nVertices, i32 triangleIndices)
{
    mesh result;
    result.nVertices = nVertices;
    result.nTriangleIndices = triangleIndices;
    result.vertices = ReserveArrayMemory(arena, nVertices, vertex);
    result.triangleIndices = ReserveArrayMemory(arena, triangleIndices, triangle_index);
    return result;
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

    LoadBitmap(&state->background, memory->DEBUGReadFile, "city_bg_purple.bmp");
    state->background.opacity = 1.0f;

    InitializeMemoryArena(&state->memoryArena,
                          (u8 *)memory->permanentMemory + sizeof(engine_state),
                          memory->permanentMemorySize - sizeof(engine_state));

    // Make cube
    state->cubeMeshUnfolded = ReserveMeshMemory(&state->memoryArena, 14, 36);
    LoadUnfoldedCubeMesh(&state->cubeMeshUnfolded, 1.0f);
    state->cubePeepo = MakeCubeUnfolded(&state->cubeMeshUnfolded, {}, {0.0f, 0.0f, 2.0f}, 1.0f);
    LoadBitmap(&state->peepoTexture, memory->DEBUGReadFile, "peepo.bmp");

    // Make plane
    i32 divisions = 30;
    i32 nVertices = (divisions + 1) * (divisions + 1);
    i32 nTriangleIndices = (divisions * divisions * 2) * 3;
    f32 side = 0.8f;
    state->planeMesh = ReserveMeshMemory(&state->memoryArena, nVertices, nTriangleIndices);
    LoadSquarePlaneMesh(&state->planeMesh, side, divisions);
    state->plane = MakeSquarePlane(&state->planeMesh, {}, {0.0f, 0.0f, 2.0f}, side, divisions);
    LoadBitmap(&state->planeTexture, memory->DEBUGReadFile, "hy3d_plane.bmp");
    state->planeWave.Initialize(0.05f, 11.0f, 8.0f);

    state->lightDir = {0.5, 0.0, 1.0f};

    memory->isInitialized = true;
    e->frameStart = std::chrono::steady_clock::now();
}

static void Update(hy3d_engine *e, engine_state *state)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e->frameStart;
    f32 dt = frameTime.count();
    e->frameStart = frameEnd;

    state->planeWave.time += dt;

    // Cube Control
    f32 speed = 2.5f * dt;
    if (e->input.keyboard.isPressed[UP])
    {
        state->cubePeepo.orientation.thetaX += speed;
    }
    if (e->input.keyboard.isPressed[DOWN])
    {
        state->cubePeepo.orientation.thetaX -= speed;
    }
    if (e->input.keyboard.isPressed[LEFT])
    {
        state->cubePeepo.orientation.thetaY += speed;
    }
    if (e->input.keyboard.isPressed[RIGHT])
    {
        state->cubePeepo.orientation.thetaY -= speed;
    }
    if (e->input.keyboard.isPressed[Q])
        state->cubePeepo.orientation.thetaZ += speed;
    if (e->input.keyboard.isPressed[W])
        state->cubePeepo.orientation.thetaZ -= speed;
    if (e->input.keyboard.isPressed[R])
    {
        state->cubePeepo.orientation.thetaX = 0.0f;
        state->cubePeepo.orientation.thetaY = 0.0f;
        state->cubePeepo.orientation.thetaZ = 0.0f;
    }
    f32 offsetZ = 1.0f * dt;
    if (e->input.keyboard.isPressed[Z])
    {
        state->cubePeepo.pos.z -= offsetZ;
        state->plane.pos.z -= offsetZ;
    }
    if (e->input.keyboard.isPressed[X])
    {
        state->cubePeepo.pos.z += offsetZ;
        state->plane.pos.z += offsetZ;
    }


    if (e->input.keyboard.isPressed[C])
    {
        state->lightDir.x -= speed;
    }
    if (e->input.keyboard.isPressed[V])
    {
        state->lightDir.x += speed;
    }
}

static void Render(hy3d_engine *e, engine_state *state)
{
    DrawBitmap(&state->background, 0, 0, &e->pixelBuffer);

    mat3 rotation;

    rotation = RotateX(state->cubePeepo.orientation.thetaX) *
               RotateY(state->cubePeepo.orientation.thetaY) *
               RotateZ(state->cubePeepo.orientation.thetaZ);
    DrawObjectTextured(
        *state->plane.mesh, rotation, state->plane.pos, &state->planeTexture,
        VertexShaderWave, &state->planeWave, state->lightDir, &e->pixelBuffer, &e->screenTransformer);
    //DrawObjectTextured(
    //    *state->cubePeepo.mesh, rotation, state->cubePeepo.pos, &state->peepoTexture,
    //    0, 0, state->lightDir, &e->pixelBuffer, &e->screenTransformer);
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
