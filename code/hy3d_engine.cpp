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

static mesh ReserveMeshMemory(memory_arena *arena, i32 nVertices, i32 nIndices)
{
    mesh result;
    result.nVertices = nVertices;
    result.nIndices = nIndices;
    result.vertices = ReserveArrayMemory(arena, nVertices, vertex);
    result.indices = ReserveArrayMemory(arena, nIndices, triangle_index);
    return result;
}

static mesh ReserveOBJMeshMemory(memory_arena *arena, i32 nVertices)
{
    mesh result;
    result.nVertices = nVertices;
    result.vertices = ReserveArrayMemory(arena, nVertices, vertex);
    return result;
}

#include <string>
#include <fstream>
#include <vector>

static inline void SplitData(const std::string &in, std::vector<std::string> &out, std::string token)
{
    out.clear();
    std::string temp;
    for (int i = 0; i < int(in.size()); i++)
    {
        std::string test = in.substr(i, token.size());
        if (test == token)
        {
            if (!temp.empty())
            {
                out.push_back(temp);
                temp.clear();
                i += (int)token.size() - 1;
            }
            else
            {
                out.push_back("");
            }
        }
        else if (i + token.size() >= in.size())
        {
            temp += in.substr(i, token.size());
            out.push_back(temp);
            break;
        }
        else
        {
            temp += in[i];
        }
    }
}

static bool LoadOBJ(std::string filename, memory_arena *arena, object *object, loaded_bitmap *texture, vec3 position, vec3 material)
{
    if (filename.substr(filename.size() - 4, 4) != ".obj")
        return false;

    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    object->texture = texture;

    std::string tag;
    std::string data;
    std::string line;

    u32 nVertices = 0;
    object->hasNormals = false;

    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            size_t tagStart = line.find_first_not_of(" \t");
            size_t tagEnd = line.find_first_of(" \t", tagStart);
            if (tagStart != std::string::npos && tagEnd != std::string::npos)
                tag = line.substr(tagStart, tagEnd - tagStart);
            else if (tagStart != std::string::npos)
                tag = line.substr(tagStart);
        }

        if (tag == "f")
            nVertices += 3;
        if (tag == "vn")
            object->hasNormals = true;
    }
    object->vertices = ReserveArrayMemory(arena, nVertices, vertex);
    object->nVertices = nVertices;
    file.clear();
    file.seekg(0);

    std::vector<vec3> positions;
    std::vector<vec2> texCoords;
    std::vector<vec3> normals;
    positions.reserve(nVertices / 3);
    texCoords.reserve(nVertices / 3);
    normals.reserve(nVertices / 3);

    i32 i = 0;
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            size_t tagStart = line.find_first_not_of(" \t");
            size_t tagEnd = line.find_first_of(" \t", tagStart);
            if (tagStart != std::string::npos && tagEnd != std::string::npos)
                tag = line.substr(tagStart, tagEnd - tagStart);
            else if (tagStart != std::string::npos)
                tag = line.substr(tagStart);

            size_t dataStart = line.find_first_not_of(" \t", tagEnd);
            size_t dataEnd = line.find_last_not_of(" \t");
            if (dataStart != std::string::npos && dataEnd != std::string::npos)
                data = line.substr(dataStart, dataEnd - dataStart + 1);
            else if (dataStart != std::string::npos)
                data = line.substr(dataStart);
        }

        if (tag == "v")
        {
            std::vector<std::string> dataSplit;
            SplitData(data, dataSplit, " ");

            vec3 v;
            v.x = std::stof(dataSplit[0]);
            v.y = std::stof(dataSplit[1]);
            v.z = std::stof(dataSplit[2]);
            positions.push_back(v);
        }
        else if (tag == "vt")
        {
            std::vector<std::string> dataSplit;
            SplitData(data, dataSplit, " ");

            vec2 v;
            v.x = std::stof(dataSplit[0]);
            v.y = std::stof(dataSplit[1]);
            texCoords.push_back(v);
        }
        else if (tag == "vn")
        {
            std::vector<std::string> dataSplit;
            SplitData(data, dataSplit, " ");

            vec3 v;
            v.x = std::stof(dataSplit[0]);
            v.y = std::stof(dataSplit[1]);
            v.z = std::stof(dataSplit[2]);
            normals.push_back(v);
        }
        else if (tag == "f")
        {
            std::vector<std::string> dataSplit; // p/t/n
            SplitData(data, dataSplit, " ");

            // Contains 3 indices to the vertices that make a triangle
            // Cases:
            // P
            // P/TC
            // P/TC/N
            // P//N
            std::vector<std::string> faceVert;

            for (std::string faceVertString : dataSplit)
            {
                SplitData(faceVertString, faceVert, "/");

                vertex v = {};

                // We always have the position index
                i32 index = std::stoi(faceVert[0]) - 1;
                v.pos = positions[index];

                // Position/Texture Coordinates
                if (faceVert.size() == 2)
                {
                    index = std::stoi(faceVert[1]) - 1;
                    v.texCoord = texCoords[index];
                }
                else if (faceVert.size() == 3)
                {
                    // Position/Texture Coordinate/Normal
                    if (faceVert[1] != "")
                    {
                        index = std::stoi(faceVert[1]) - 1;
                        v.texCoord = texCoords[index];

                        index = std::stoi(faceVert[2]) - 1;
                        v.normal = normals[index];
                    }
                    // Position//Normal
                    else
                    {
                        index = std::stoi(faceVert[2]) - 1;
                        v.normal = normals[index];
                    }
                }
                object->vertices[i] = v;
                i++;
            }
        }
    }
    file.close();
    object->pos = position;
    object->mat = material;
    return true;
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

    InitializeMemoryArena(&state->memoryArena,
                          (u8 *)memory->permanentMemory + sizeof(engine_state),
                          memory->permanentMemorySize - sizeof(engine_state));

    state->curObject = &state->monkey;
    LoadBitmap(&state->bunnyTexture, memory->DEBUGReadFile, "bunny_tex.bmp");
    LoadBitmap(&state->cruiserTexture, memory->DEBUGReadFile, "cruiser.bmp");
    LoadBitmap(&state->f16Tex, memory->DEBUGReadFile, "F16s.bmp");
    LoadBitmap(&state->background, memory->DEBUGReadFile, "city_bg_purple.bmp");

    LoadOBJ("bunny.obj", &state->memoryArena, &state->bunny, 0, {0.0f, -0.1f, 1.0f}, {0.9f, 0.85f, 0.9f});
    LoadOBJ("suzanne.obj", &state->memoryArena, &state->monkey, 0, {0.0f, 0.0f, 5.0f}, {0.9f, 0.75f, 0.45f});
    LoadOBJ("gourad.obj", &state->memoryArena, &state->gourad, 0, {0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 1.0f});
    LoadOBJ("bunny_tex.obj", &state->memoryArena, &state->bunnyTextured, &state->bunnyTexture, {0.0f, 0.0f, 5.0f}, {1.0f, 1.0f, 1.0f});
    LoadOBJ("cruiser.obj", &state->memoryArena, &state->cruiser, &state->cruiserTexture, {0.0f, 0.0f, 5.0f}, {1.0f, 1.0f, 1.0f});
    LoadOBJ("f16.obj", &state->memoryArena, &state->f16, &state->cruiserTexture, {0.0f, 0.0f, 5.0f}, {1.0f, 1.0f, 1.0f});

    state->orientation = {};

    state->diffuse.intensity = {1.0f, 1.0f, 1.0f};
    state->diffuse.direction = {0.0f, 0.0f, 1.0f};
    state->ambient = {0.2f, 0.15f, 0.25f};

    memory->isInitialized = true;
    e->frameStart = std::chrono::steady_clock::now();
}

static void Update(hy3d_engine *e, engine_state *state)
{
    std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();
    std::chrono::duration<f32> frameTime = frameEnd - e->frameStart;
    f32 dt = frameTime.count();
    e->frameStart = frameEnd;

    if (e->input.keyboard.isPressed[ONE])
        state->curObject = &state->bunny;
    if (e->input.keyboard.isPressed[TWO])
        state->curObject = &state->monkey;
    if (e->input.keyboard.isPressed[THREE])
        state->curObject = &state->gourad;
    if (e->input.keyboard.isPressed[FOUR])
        state->curObject = &state->bunnyTextured;
    if (e->input.keyboard.isPressed[FIVE])
        state->curObject = &state->cruiser;
    if (e->input.keyboard.isPressed[SIX])
        state->curObject = &state->f16;

    // Cube Control
    f32 speed = 2.5f * dt;
    if (e->input.keyboard.isPressed[UP])
        state->curObject->orientation.thetaX += speed;
    if (e->input.keyboard.isPressed[DOWN])
        state->curObject->orientation.thetaX -= speed;
    if (e->input.keyboard.isPressed[LEFT])
        state->curObject->orientation.thetaY += speed;
    if (e->input.keyboard.isPressed[RIGHT])
        state->curObject->orientation.thetaY -= speed;
    if (e->input.keyboard.isPressed[Q])
        state->curObject->orientation.thetaZ += speed;
    if (e->input.keyboard.isPressed[W])
        state->curObject->orientation.thetaZ -= speed;
    if (e->input.keyboard.isPressed[R])
    {
        state->curObject->orientation.thetaX = 0.0f;
        state->curObject->orientation.thetaY = 0.0f;
        state->curObject->orientation.thetaZ = 0.0f;
    }

    f32 offsetZ = 1.0f * dt;
    if (e->input.keyboard.isPressed[Z])
        state->curObject->pos.z -= offsetZ;
    if (e->input.keyboard.isPressed[X])
        state->curObject->pos.z += offsetZ;

    if (e->input.keyboard.isPressed[C])
        state->diffuse.direction.x -= speed;
    if (e->input.keyboard.isPressed[V])
        state->diffuse.direction.x += speed;
    if (e->input.keyboard.isPressed[SHIFT])
    {
        state->diffuse.direction.x = 0;
        state->diffuse.direction.y = 0;
        state->diffuse.direction.z *= -1.0f;
    }
}

static void Render(hy3d_engine *e, engine_state *state)
{
    DrawBitmap(&state->background, 0, 0, &e->pixelBuffer);

    DrawObject(state->curObject, state->diffuse, state->ambient, shade_type::GOURAUD,
               &e->pixelBuffer, &e->screenTransformer);
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
