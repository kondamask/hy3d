#include "hy3d_vector.h"

struct mat3
{
    static mat3 Identity()
    {
        return {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f};
    }
    static mat3 Scale(float factor)
    {
        return {
            factor, 0.0f, 0.0f,
            0.0f, factor, 0.0f,
            0.0f, 0.0f, factor};
    }

    float cell[3][3]; // NOTE: [row][column]
};

mat3 operator*(mat3 a, mat3 b)
{
    mat3 result;
    for (size_t j = 0; j < 3; j++)
    {
        for (size_t k = 0; k < 3; k++)
        {
            float sum = 0.0f;
            for (size_t i = 0; i < 3; i++)
            {
                sum += a.cell[j][i] * b.cell[i][k];
            }
            result.cell[j][k] = sum;
        }
    }
    return result;
}

mat3 operator*=(mat3 &a, mat3 b)
{
    a = a * b;
    return a;
}

mat3 operator*(mat3 a, float b)
{
    for (size_t j = 0; j < 3; j++)
    {
        for (size_t i = 0; i < 3; i++)
        {
            a.cell[i][j] *= b;
        }
    }
    return a;
}

mat3 operator*(float b, mat3 a)
{
    return a * b;
}

mat3 operator*=(mat3 &a, float b)
{
    a = a * b;
    return a;
}

vec3 operator*(vec3 v, mat3 m)
{
    return {
        v.x * m.cell[0][0] + v.y * m.cell[1][0] + v.z * m.cell[2][0],
        v.x * m.cell[0][1] + v.y * m.cell[1][1] + v.z * m.cell[2][1],
        v.x * m.cell[0][2] + v.y * m.cell[1][2] + v.z * m.cell[2][2]};
}

vec3 operator*=(vec3 &v, mat3 m)
{
    v = v * m;
    return v;
}