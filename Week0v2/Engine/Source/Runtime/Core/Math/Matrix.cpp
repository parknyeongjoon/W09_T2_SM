#include "Matrix.h"
#include "Rotator.h"
#include "Quat.h"

const FMatrix FMatrix::Identity = { {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
} };

// 행렬 덧셈
FMatrix FMatrix::operator+(const FMatrix& Other) const {
    FMatrix Result;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] + Other.M[i][j];
    return Result;
}

// 행렬 뺄셈
FMatrix FMatrix::operator-(const FMatrix& Other) const {
    FMatrix Result;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] - Other.M[i][j];
    return Result;
}

// 행렬 곱셈
FMatrix FMatrix::operator*(const FMatrix& Other) const {
    FMatrix Result = {};
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            for (size_t k = 0; k < 4; k++)
                Result.M[i][j] += M[i][k] * Other.M[k][j];
    return Result;
}

// 스칼라 곱셈
FMatrix FMatrix::operator*(float Scalar) const {
    FMatrix Result;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] * Scalar;
    return Result;
}

// 스칼라 나눗셈
FMatrix FMatrix::operator/(float Scalar) const {
    FMatrix Result;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            Result.M[i][j] = M[i][j] / Scalar;
    return Result;
}

float* FMatrix::operator[](int row) {
    return M[row];
}

const float* FMatrix::operator[](int row) const
{
    return M[row];
}

// 전치 행렬
FMatrix FMatrix::Transpose(const FMatrix& Mat) {
    FMatrix Result;
    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            Result.M[i][j] = Mat.M[j][i];
    return Result;
}

// 행렬식 계산 (라플라스 전개, 4x4 행렬)
float FMatrix::Determinant(const FMatrix& Mat) {
    float det = 0.0f;
    for (size_t i = 0; i < 4; i++) {
        float subMat[3][3];
        for (size_t j = 1; j < 4; j++) {
            size_t colIndex = 0;
            for (size_t k = 0; k < 4; k++) {
                if (k == i) continue;
                subMat[j - 1][colIndex] = Mat.M[j][k];
                colIndex++;
            }
        }
        float minorDet =
            subMat[0][0] * (subMat[1][1] * subMat[2][2] - subMat[1][2] * subMat[2][1]) -
            subMat[0][1] * (subMat[1][0] * subMat[2][2] - subMat[1][2] * subMat[2][0]) +
            subMat[0][2] * (subMat[1][0] * subMat[2][1] - subMat[1][1] * subMat[2][0]);
        det += (i % 2 == 0 ? 1 : -1) * Mat.M[0][i] * minorDet;
    }
    return det;
}

// 역행렬 (가우스-조던 소거법)
FMatrix FMatrix::Inverse(const FMatrix& Mat) {
    float det = Determinant(Mat);
    if (fabs(det) < 1e-6) {
        return Identity;
    }

    FMatrix Inv;
    float invDet = 1.0f / det;

    // 여인수 행렬 계산 후 전치하여 역행렬 계산
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            float subMat[3][3];
            size_t subRow = 0;
            for (size_t r = 0; r < 4; r++) {
                if (r == i) continue;
                size_t subCol = 0;
                for (size_t c = 0; c < 4; c++) {
                    if (c == j) continue;
                    subMat[subRow][subCol] = Mat.M[r][c];
                    subCol++;
                }
                subRow++;
            }
            float minorDet =
                subMat[0][0] * (subMat[1][1] * subMat[2][2] - subMat[1][2] * subMat[2][1]) -
                subMat[0][1] * (subMat[1][0] * subMat[2][2] - subMat[1][2] * subMat[2][0]) +
                subMat[0][2] * (subMat[1][0] * subMat[2][1] - subMat[1][1] * subMat[2][0]);

            Inv.M[j][i] = ((i + j) % 2 == 0 ? 1 : -1) * minorDet * invDet;
        }
    }
    return Inv;
}

FMatrix FMatrix::CreateRotationMatrix(float roll, float pitch, float yaw)
{
    float radRoll = roll * (3.14159265359f / 180.0f);
    float radPitch = pitch * (3.14159265359f / 180.0f);
    float radYaw = yaw * (3.14159265359f / 180.0f);

    float cosRoll = cos(radRoll), sinRoll = sin(radRoll);
    float cosPitch = cos(radPitch), sinPitch = sin(radPitch);
    float cosYaw = cos(radYaw), sinYaw = sin(radYaw);

    // Z축 (Yaw) 회전
    FMatrix rotationZ = { {
        { cosYaw, -sinYaw, 0, 0 },
        { sinYaw, cosYaw, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    } };

    // Y축 (Pitch) 회전
    FMatrix rotationY = { {
        { cosPitch, 0, sinPitch, 0 },
        { 0, 1, 0, 0 },
        { -sinPitch, 0, cosPitch, 0 },
        { 0, 0, 0, 1 }
    } };

    // X축 (Roll) 회전
    FMatrix rotationX = { {
        { 1, 0, 0, 0 },
        { 0, cosRoll, -sinRoll, 0 },
        { 0, sinRoll, cosRoll, 0 },
        { 0, 0, 0, 1 }
    } };

    // DirectX 표준 순서: Z(Yaw) → Y(Pitch) → X(Roll)  
    return rotationX * rotationY * rotationZ;  // 이렇게 하면  오른쪽 부터 적용됨
}


// 스케일 행렬 생성
FMatrix FMatrix::CreateScaleMatrix(float scaleX, float scaleY, float scaleZ)
{
    return { {
        { scaleX, 0, 0, 0 },
        { 0, scaleY, 0, 0 },
        { 0, 0, scaleZ, 0 },
        { 0, 0, 0, 1 }
    } };
}

FMatrix FMatrix::CreateTranslationMatrix(const FVector& position)
{
    FMatrix translationMatrix = FMatrix::Identity;
    translationMatrix.M[3][0] = position.X;
    translationMatrix.M[3][1] = position.Y;
    translationMatrix.M[3][2] = position.Z;
    return translationMatrix;
}

FMatrix FMatrix::GetScaleMatrix(const FVector& InScale)
{
    return CreateScaleMatrix(InScale.X, InScale.Y, InScale.Z);
}

FMatrix FMatrix::GetTranslationMatrix(const FVector& position)
{
    return CreateTranslationMatrix(position);
}

FMatrix FMatrix::GetRotationMatrix(const FRotator& InRotation)
{
    return CreateRotationMatrix(InRotation.Roll, InRotation.Pitch, InRotation.Yaw);
}

FMatrix FMatrix::GetRotationMatrix(const FQuat& InRotation)
{
    // 쿼터니언 요소 추출
    const float QuatX = InRotation.x, QuatY = InRotation.y, QuatZ = InRotation.z, w = InRotation.w;

    // 중간 계산값
    const float xx = QuatX * QuatX;
    float yy = QuatY * QuatY;
    float zz = QuatZ * QuatZ;
    
    const float xy = QuatX * QuatY;
    float xz = QuatX * QuatZ;
    float yz = QuatY * QuatZ;
    const float wx = w * QuatX;
    const float wy = w * QuatY;
    const float wz = w * QuatZ;

    // 회전 행렬 구성
    FMatrix Result;

    Result.M[0][0] = 1.0f - 2.0f * (yy + zz);
    Result.M[0][1] = 2.0f * (xy - wz);
    Result.M[0][2] = 2.0f * (xz + wy);
    Result.M[0][3] = 0.0f;

    Result.M[1][0] = 2.0f * (xy + wz);
    Result.M[1][1] = 1.0f - 2.0f * (xx + zz);
    Result.M[1][2] = 2.0f * (yz - wx);
    Result.M[1][3] = 0.0f;

    Result.M[2][0] = 2.0f * (xz - wy);
    Result.M[2][1] = 2.0f * (yz + wx);
    Result.M[2][2] = 1.0f - 2.0f * (xx + yy);
    Result.M[2][3] = 0.0f;

    Result.M[3][0] = 0.0f;
    Result.M[3][1] = 0.0f;
    Result.M[3][2] = 0.0f;
    Result.M[3][3] = 1.0f; // 4x4 행렬이므로 마지막 값은 1

    return Result;
}

FVector FMatrix::TransformVector(const FVector& v, const FMatrix& m)
{
    FVector result;

    // 4x4 행렬을 사용하여 벡터 변환 (W = 0으로 가정, 방향 벡터)
    result.X = v.X * m.M[0][0] + v.Y * m.M[1][0] + v.Z * m.M[2][0] + 0.0f * m.M[3][0];
    result.Y = v.X * m.M[0][1] + v.Y * m.M[1][1] + v.Z * m.M[2][1] + 0.0f * m.M[3][1];
    result.Z = v.X * m.M[0][2] + v.Y * m.M[1][2] + v.Z * m.M[2][2] + 0.0f * m.M[3][2];


    return result;
}

// FVector4를 변환하는 함수
FVector4 FMatrix::TransformVector(const FVector4& v, const FMatrix& m)
{
    FVector4 result;
    result.x = v.x * m.M[0][0] + v.y * m.M[1][0] + v.z * m.M[2][0] + v.w * m.M[3][0];
    result.y = v.x * m.M[0][1] + v.y * m.M[1][1] + v.z * m.M[2][1] + v.w * m.M[3][1];
    result.z = v.x * m.M[0][2] + v.y * m.M[1][2] + v.z * m.M[2][2] + v.w * m.M[3][2];
    result.w = v.x * m.M[0][3] + v.y * m.M[1][3] + v.z * m.M[2][3] + v.w * m.M[3][3];
    return result;
}


