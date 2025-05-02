#include "FLoaderOBJ.h"
#include "UObject/ObjectFactory.h"
#include "Components/Material/Material.h"
#include "Components/Mesh/StaticMesh.h"

bool FLoaderOBJ::ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo)
{
    std::ifstream OBJ(ObjFilePath.ToWideString());
    if (!OBJ)
    {
        return false;
    }

    OutObjInfo.PathName = ObjFilePath.ToWideString().substr(0, ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
    OutObjInfo.ObjectName = ObjFilePath.ToWideString().substr(ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
    // ObjectName은 wstring 타입이므로, 이를 string으로 변환 (간단한 ASCII 변환의 경우)
    std::wstring wideName = OutObjInfo.ObjectName;
    std::string fileName(wideName.begin(), wideName.end());

    // 마지막 '.'을 찾아 확장자를 제거
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos) {
        OutObjInfo.DisplayName = fileName.substr(0, dotPos);
    } else {
        OutObjInfo.DisplayName = fileName;
    }
    
    std::string Line;

    while (std::getline(OBJ, Line))
    {
        if (Line.empty() || Line[0] == '#')
            continue;
        
        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        if (Token == "mtllib")
        {
            LineStream >> Line;
            OutObjInfo.MatName = Line;
            continue;
        }

        if (Token == "usemtl")
        {
            LineStream >> Line;
            FString MatName(Line);

            if (!OutObjInfo.MaterialSubsets.IsEmpty())
            {
                FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
                LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
            }
            
            FMaterialSubset MaterialSubset;
            MaterialSubset.MaterialName = MatName;
            MaterialSubset.IndexStart = OutObjInfo.VertexIndices.Num();
            MaterialSubset.IndexCount = 0;
            OutObjInfo.MaterialSubsets.Add(MaterialSubset);
        }

        if (Token == "g" || Token == "o")
        {
            LineStream >> Line;
            OutObjInfo.GroupName.Add(Line);
            OutObjInfo.NumOfGroup++;
        }

        if (Token == "v") // Vertex
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutObjInfo.Vertices.Add(FVector(x,y,z));
            continue;
        }

        if (Token == "vn") // Normal
        {
            float nx, ny, nz;
            LineStream >> nx >> ny >> nz;
            OutObjInfo.Normals.Add(FVector(nx,ny,nz));
            continue;
        }

        if (Token == "vt") // Texture
        {
            float u, v;
            LineStream >> u >> v;
            OutObjInfo.UVs.Add(FVector2D(u, v));
            continue;
        }

        if (Token == "f")
        {
            TArray<uint32> faceVertexIndices;  // 이번 페이스의 정점 인덱스
            TArray<uint32> faceNormalIndices;  // 이번 페이스의 법선 인덱스
            TArray<uint32> faceTextureIndices; // 이번 페이스의 텍스처 인덱스
            
            while (LineStream >> Token)
            {
                std::istringstream tokenStream(Token);
                std::string part;
                TArray<std::string> facePieces;

                // '/'로 분리하여 v/vt/vn 파싱
                while (std::getline(tokenStream, part, '/'))
                {
                    facePieces.Add(part);
                }

                // OBJ 인덱스는 1부터 시작하므로 -1로 변환
                uint32 vertexIndex = facePieces[0].empty() ? 0 : std::stoi(facePieces[0]) - 1;
                uint32 textureIndex = (facePieces.Num() > 1 && !facePieces[1].empty()) ? std::stoi(facePieces[1]) - 1 : UINT32_MAX;
                uint32 normalIndex = (facePieces.Num() > 2 && !facePieces[2].empty()) ? std::stoi(facePieces[2]) - 1 : UINT32_MAX;

                faceVertexIndices.Add(vertexIndex);
                faceTextureIndices.Add(textureIndex);
                faceNormalIndices.Add(normalIndex);
            }

            if (faceVertexIndices.Num() == 4) // 쿼드
            {
                // 첫 번째 삼각형: 0-1-2
                OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[1]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);

                OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[1]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);

                OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[1]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);

                // 두 번째 삼각형: 0-2-3
                OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[3]);

                OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[3]);

                OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[3]);
            }
            else if (faceVertexIndices.Num() == 3) // 삼각형
            {
                OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[1]);
                OutObjInfo.VertexIndices.Add(faceVertexIndices[2]);

                OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[1]);
                OutObjInfo.TextureIndices.Add(faceTextureIndices[2]);

                OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[1]);
                OutObjInfo.NormalIndices.Add(faceNormalIndices[2]);
            }
            // // 삼각형화 (삼각형 팬 방식)
            // for (int j = 1; j + 1 < faceVertexIndices.Num(); j++)
            // {
            //     OutObjInfo.VertexIndices.Add(faceVertexIndices[0]);
            //     OutObjInfo.VertexIndices.Add(faceVertexIndices[j]);
            //     OutObjInfo.VertexIndices.Add(faceVertexIndices[j + 1]);
            //
            //     OutObjInfo.TextureIndices.Add(faceTextureIndices[0]);
            //     OutObjInfo.TextureIndices.Add(faceTextureIndices[j]);
            //     OutObjInfo.TextureIndices.Add(faceTextureIndices[j + 1]);
            //
            //     OutObjInfo.NormalIndices.Add(faceNormalIndices[0]);
            //     OutObjInfo.NormalIndices.Add(faceNormalIndices[j]);
            //     OutObjInfo.NormalIndices.Add(faceNormalIndices[j + 1]);
            // }
        }
    }

    if (!OutObjInfo.MaterialSubsets.IsEmpty())
    {
        FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
        LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
    }
    
    return true;
}

bool FLoaderOBJ::ParseMaterial(FObjInfo& OutObjInfo, OBJ::FStaticMeshRenderData& OutFStaticMesh)
{
    // Subset
    OutFStaticMesh.MaterialSubsets = OutObjInfo.MaterialSubsets;
    
    std::ifstream MtlFile(OutObjInfo.PathName + OutObjInfo.MatName.ToWideString());
    if (!MtlFile.is_open())
    {
        return false;
    }

    std::string Line;
    int32 MaterialIndex = -1;
    
    while (std::getline(MtlFile, Line))
    {
        if (Line.empty() || Line[0] == '#')
            continue;
        
        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        // Create new material if token is 'newmtl'
        if (Token == "newmtl")
        {
            LineStream >> Line;
            MaterialIndex++;

            FObjMaterialInfo Material = {};
            Material.MTLName = Line;
            OutFStaticMesh.Materials.Add(Material);
        }

        if (Token == "Kd")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].Diffuse = FVector(x, y, z);
        }

        if (Token == "Ks")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].Specular = FVector(x, y, z);
        }

        if (Token == "Ka")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].Ambient = FVector(x, y, z);
        }

        if (Token == "Ke")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].Emissive = FVector(x, y, z);
        }

        if (Token == "Ns")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].SpecularScalar = x;
        }

        if (Token == "Ni")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].DensityScalar = x;
        }

        if (Token == "d" || Token == "Tr")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].TransparencyScalar = x;
            OutFStaticMesh.Materials[MaterialIndex].bTransparent = true;
        }

        if (Token == "illum")
        {
            uint32 x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].IlluminanceModel = x;
        }

        if (Token == "map_Kd")
        {
            LineStream >> Line;
            
            OutFStaticMesh.Materials[MaterialIndex].DiffuseTextureName = Line;

            FWString TexturePath = OutObjInfo.PathName + OutFStaticMesh.Materials[MaterialIndex].DiffuseTextureName.ToWideString();
            OutFStaticMesh.Materials[MaterialIndex].DiffuseTexturePath = TexturePath;
            OutFStaticMesh.Materials[MaterialIndex].bHasTexture = true;

            CreateTextureFromFile(OutFStaticMesh.Materials[MaterialIndex].DiffuseTexturePath);
        }

        if (Token == "map_Bump")
        {
            //나중에 Bump쓰면 파일 이름 접미사가 _Height이냐 _Normal이냐에 따라서 Bump와 normalMap이 나뉜다. fbx파일 도입되면 나누고 지금은 Bump안하기 때문에 이렇게만
            LineStream >> Line;

            if (Line[0] == '-')
            {
                float NS;
                LineStream >> NS;
                OutFStaticMesh.Materials[MaterialIndex].NormalScale = NS;
                LineStream >> Line;
            }

            OutFStaticMesh.Materials[MaterialIndex].NormalTextureName = Line;
            
            FWString TexturePath = OutObjInfo.PathName + OutFStaticMesh.Materials[MaterialIndex].NormalTextureName.ToWideString();
            OutFStaticMesh.Materials[MaterialIndex].NormalTexturePath = TexturePath;
            OutFStaticMesh.Materials[MaterialIndex].bHasTexture = true;

            CreateTextureFromFile(OutFStaticMesh.Materials[MaterialIndex].NormalTexturePath);
        }
    }
    
    return true;
}

bool FLoaderOBJ::ConvertToStaticMesh(const FObjInfo& RawData, OBJ::FStaticMeshRenderData& OutStaticMesh)
{
    OutStaticMesh.ObjectName = RawData.ObjectName;
    OutStaticMesh.PathName = RawData.PathName;
    OutStaticMesh.DisplayName = RawData.DisplayName;

    // 고유 정점을 기반으로 FVertexSimple 배열 생성
    TMap<std::string, uint32> vertexMap; // 중복 체크용

    for (int32 i = 0; i < RawData.VertexIndices.Num(); i++)
    {
        uint32 vIdx = RawData.VertexIndices[i];
        uint32 tIdx = RawData.TextureIndices[i];
        uint32 nIdx = RawData.NormalIndices[i];

        // 키 생성 (v/vt/vn 조합)
        std::string key = std::to_string(vIdx) + "/" + 
                         std::to_string(tIdx) + "/" + 
                         std::to_string(nIdx);

        uint32 index;
        if (vertexMap.Find(key) == nullptr)
        {
            FVertexSimple vertex {};
            vertex.x = RawData.Vertices[vIdx].X;
            vertex.y = RawData.Vertices[vIdx].Y;
            vertex.z = RawData.Vertices[vIdx].Z;
            vertex.w = 1;

            vertex.r = 1.0f; vertex.g = 1.0f; vertex.b = 1.0f; vertex.a = 1.0f; // 기본 색상

            if (tIdx != UINT32_MAX && tIdx < RawData.UVs.Num())
            {
                vertex.u = RawData.UVs[tIdx].x;
                vertex.v = -RawData.UVs[tIdx].y;
            }

            if (nIdx != UINT32_MAX && nIdx < RawData.Normals.Num())
            {
                vertex.nx = RawData.Normals[nIdx].X;
                vertex.ny = RawData.Normals[nIdx].Y;
                vertex.nz = RawData.Normals[nIdx].Z;
            }

            for (int32 j = 0; j < OutStaticMesh.MaterialSubsets.Num(); j++)
            {
                const FMaterialSubset& subset = OutStaticMesh.MaterialSubsets[j];
                if ( i >= subset.IndexStart && i < subset.IndexStart + subset.IndexCount)
                {
                    vertex.MaterialIndex = subset.MaterialIndex;
                    break;
                }
            }
            
            index = OutStaticMesh.Vertices.Num();
            OutStaticMesh.Vertices.Add(vertex);
            vertexMap[key] = index;
        }
        else
        {
            index = vertexMap[key];
        }

        OutStaticMesh.Indices.Add(index);
        
    }

    for (int32 i = 0; i < OutStaticMesh.Indices.Num(); i += 3)
    {
        const int32 i0 = OutStaticMesh.Indices[i];
        const int32 i1 = OutStaticMesh.Indices[i + 1];
        const int32 i2 = OutStaticMesh.Indices[i + 2];

        FVertexSimple& v0 = OutStaticMesh.Vertices[i0];
        FVertexSimple& v1 = OutStaticMesh.Vertices[i1];
        FVertexSimple& v2 = OutStaticMesh.Vertices[i2];

        // 삼각형 변 계산
        const FVector edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
        const FVector edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

        // UV 차이 계산
        const FVector2D deltaUV1 = {v1.u - v0.u, v1.v - v0.v};
        const FVector2D deltaUV2 = {v2.u - v0.u, v2.v - v0.v};
        float deltaUV = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        // 접선 계산
        const float f = 1.0f / (deltaUV == 0 ? 1 : deltaUV);
        FVector tangent;
        tangent.X = f * (deltaUV2.y * edge1.X - deltaUV1.y * edge2.X);
        tangent.Y = f * (deltaUV2.y * edge1.Y - deltaUV1.y * edge2.Y);
        tangent.Z = f * (deltaUV2.y * edge1.Z - deltaUV1.y * edge2.Z);
        tangent = tangent.Normalize();

        // 정점별 접선 누적
        v0.Tangentnx += tangent.X; v0.Tangentny += tangent.Y; v0.Tangentnz += tangent.Z;
        v1.Tangentnx += tangent.X; v1.Tangentny += tangent.Y; v1.Tangentnz += tangent.Z;
        v2.Tangentnx += tangent.X; v2.Tangentny += tangent.Y; v2.Tangentnz += tangent.Z;
    }

    for (auto& vertex : OutStaticMesh.Vertices)
    {
        FVector tangent(vertex.Tangentnx, vertex.Tangentny, vertex.Tangentnz);
        if (tangent.X > 0.00001f || tangent.Y > 0.00001f || tangent.Z > 0.00001f)
        {
            tangent = tangent.Normalize();                
        } 
        else 
        {
            tangent = FVector(1.0f, 0.0f, 0.0f);
        }
        vertex.Tangentnx = tangent.X;
        vertex.Tangentny = tangent.Y;
        vertex.Tangentnz = tangent.Z;
    }
    
    // Calculate StaticMesh BoundingBox
    ComputeBoundingBox(OutStaticMesh.Vertices, OutStaticMesh.BoundingBoxMin, OutStaticMesh.BoundingBoxMax);
    
    return true;
}

bool FLoaderOBJ::CreateTextureFromFile(const FWString& Filename)
{
    if (UEditorEngine::ResourceManager.GetTexture(Filename))
    {
        return true;
    }

    HRESULT hr = UEditorEngine::ResourceManager.LoadTextureFromFile(UEditorEngine::graphicDevice.Device, UEditorEngine::graphicDevice.DeviceContext, Filename.c_str());

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void FLoaderOBJ::ComputeBoundingBox(const TArray<FVertexSimple>& InVertices, FVector& OutMinVector, FVector& OutMaxVector)
{
    FVector MinVector = { FLT_MAX, FLT_MAX, FLT_MAX };
    FVector MaxVector = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        
    for (int32 i = 0; i < InVertices.Num(); i++)
    {
        MinVector.X = std::min(MinVector.X, InVertices[i].x);
        MinVector.Y = std::min(MinVector.Y, InVertices[i].y);
        MinVector.Z = std::min(MinVector.Z, InVertices[i].z);

        MaxVector.X = std::max(MaxVector.X, InVertices[i].x);
        MaxVector.Y = std::max(MaxVector.Y, InVertices[i].y);
        MaxVector.Z = std::max(MaxVector.Z, InVertices[i].z);
    }

    OutMinVector = MinVector;
    OutMaxVector = MaxVector;
}

OBJ::FStaticMeshRenderData* FManagerOBJ::LoadObjStaticMeshAsset(const FString& PathFileName)
{
    OBJ::FStaticMeshRenderData* NewStaticMesh = new OBJ::FStaticMeshRenderData();
    
    if ( const auto It = ObjStaticMeshMap.Find(PathFileName))
    {
        return *It;
    }

    TArray<FString> MtlLibPaths = ExtractMTLLibraryNamesFromOBJ(PathFileName);
    std::filesystem::file_time_type MtlLastWriteTime;
    for (FString MtlLibPath : MtlLibPaths)
    {
        std::filesystem::file_time_type currentMtlWriteTime = std::filesystem::last_write_time(*MtlLibPath);
        if (MtlLastWriteTime < currentMtlWriteTime)
        {
            MtlLastWriteTime = currentMtlWriteTime;
        }
    }

    const std::filesystem::file_time_type ObjLastWriteTime = std::filesystem::last_write_time(*PathFileName);

    std::filesystem::file_time_type LastWriteTime = MtlLastWriteTime <= ObjLastWriteTime ? ObjLastWriteTime : MtlLastWriteTime;
    
    const FWString BinaryPath = (PathFileName + ".bin").ToWideString();
    if (std::ifstream(BinaryPath).good())
    {
        const std::filesystem::file_time_type BinLastWriteTime = std::filesystem::last_write_time(BinaryPath);
        if (LastWriteTime < BinLastWriteTime)
        {
            if (LoadStaticMeshFromBinary(BinaryPath, *NewStaticMesh))
            {
                ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
                // 패스 명 추가
                NewStaticMesh->PathName = PathFileName.ToWideString();
                return NewStaticMesh;
            }
        }
    }
    
    // Parse OBJ
    FObjInfo NewObjInfo;
    bool Result = FLoaderOBJ::ParseOBJ(PathFileName, NewObjInfo);

    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }


    // Material
    if (NewObjInfo.MaterialSubsets.Num() > 0)
    {
        Result = FLoaderOBJ::ParseMaterial(NewObjInfo, *NewStaticMesh);

        if (!Result)
        {
            delete NewStaticMesh;
            return nullptr;
        }

        CombineMaterialIndex(*NewStaticMesh);

        for (int materialIndex = 0; materialIndex < NewStaticMesh->Materials.Num(); materialIndex++) {
            CreateMaterial(NewStaticMesh->Materials[materialIndex]);
        }
    }
    
    // Convert FStaticMeshRenderData
    Result = FLoaderOBJ::ConvertToStaticMesh(NewObjInfo, *NewStaticMesh);
    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    SaveStaticMeshToBinary(BinaryPath, *NewStaticMesh);
    ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
    return NewStaticMesh;
}

bool FManagerOBJ::LoadStaticMeshFromBinary(const FWString& FilePath, OBJ::FStaticMeshRenderData& OutStaticMesh)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T OPEN STATIC MESH BINARY FILE");
        return false;
    }

    TArray<FWString> Textures;

    // Object Name
    Serializer::ReadFWString(File, OutStaticMesh.ObjectName);

    // Path Name
    Serializer::ReadFWString(File, OutStaticMesh.PathName);

    // Display Name
    Serializer::ReadFString(File, OutStaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = 0;
    File.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
    OutStaticMesh.Vertices.SetNum(VertexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Vertices.GetData()), VertexCount * sizeof(FVertexSimple));

    // Indices
    uint32 IndexCount = 0;
    File.read(reinterpret_cast<char*>(&IndexCount), sizeof(IndexCount));
    OutStaticMesh.Indices.SetNum(IndexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Material
    uint32 MaterialCount = 0;
    File.read(reinterpret_cast<char*>(&MaterialCount), sizeof(MaterialCount));
    OutStaticMesh.Materials.SetNum(MaterialCount);
    for (FObjMaterialInfo& Material : OutStaticMesh.Materials)
    {
        Serializer::ReadFString(File, Material.MTLName);
        File.read(reinterpret_cast<char*>(&Material.bHasTexture), sizeof(Material.bHasTexture));
        File.read(reinterpret_cast<char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.read(reinterpret_cast<char*>(&Material.Diffuse), sizeof(Material.Diffuse));
        File.read(reinterpret_cast<char*>(&Material.Specular), sizeof(Material.Specular));
        File.read(reinterpret_cast<char*>(&Material.Ambient), sizeof(Material.Ambient));
        File.read(reinterpret_cast<char*>(&Material.Emissive), sizeof(Material.Emissive));
        File.read(reinterpret_cast<char*>(&Material.SpecularScalar), sizeof(Material.SpecularScalar));
        File.read(reinterpret_cast<char*>(&Material.DensityScalar), sizeof(Material.DensityScalar));
        File.read(reinterpret_cast<char*>(&Material.TransparencyScalar), sizeof(Material.TransparencyScalar));
        File.read(reinterpret_cast<char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));
        Serializer::ReadFString(File, Material.DiffuseTextureName);
        Serializer::ReadFWString(File, Material.DiffuseTexturePath);
        Serializer::ReadFString(File, Material.AmbientTextureName);
        Serializer::ReadFWString(File, Material.AmbientTexturePath);
        Serializer::ReadFString(File, Material.SpecularTextureName);
        Serializer::ReadFWString(File, Material.SpecularTexturePath);
        Serializer::ReadFString(File, Material.BumpTextureName);
        Serializer::ReadFWString(File, Material.BumpTexturePath);
        Serializer::ReadFString(File, Material.AlphaTextureName);
        Serializer::ReadFWString(File, Material.AlphaTexturePath);
        Serializer::ReadFString(File, Material.NormalTextureName);
        Serializer::ReadFWString(File, Material.NormalTexturePath);

        
        if (!Material.DiffuseTexturePath.empty())
        {
            Textures.AddUnique(Material.DiffuseTexturePath);
        }
        if (!Material.AmbientTexturePath.empty())
        {
            Textures.AddUnique(Material.AmbientTexturePath);
        }
        if (!Material.SpecularTexturePath.empty())
        {
            Textures.AddUnique(Material.SpecularTexturePath);
        }
        if (!Material.BumpTexturePath.empty())
        {
            Textures.AddUnique(Material.BumpTexturePath);
        }
        if (!Material.AlphaTexturePath.empty())
        {
            Textures.AddUnique(Material.AlphaTexturePath);
        }
        if (!Material.NormalTexturePath.empty())
        {
            Textures.AddUnique(Material.NormalTexturePath);
        }
    }

    // Material Subset
    uint32 SubsetCount = 0;
    File.read(reinterpret_cast<char*>(&SubsetCount), sizeof(SubsetCount));
    OutStaticMesh.MaterialSubsets.SetNum(SubsetCount);
    for (FMaterialSubset& Subset : OutStaticMesh.MaterialSubsets)
    {
        Serializer::ReadFString(File, Subset.MaterialName);
        File.read(reinterpret_cast<char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.read(reinterpret_cast<char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.read(reinterpret_cast<char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMin), sizeof(FVector));
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMax), sizeof(FVector));
    
    File.close();

    // Texture Load
    if (Textures.Num() > 0)
    {
        for (const FWString& Texture : Textures)
        {
            if (UEditorEngine::ResourceManager.GetTexture(Texture) == nullptr)
            {
                UEditorEngine::ResourceManager.LoadTextureFromFile(UEditorEngine::graphicDevice.Device, UEditorEngine::graphicDevice.DeviceContext, Texture.c_str());
            }
        }
    }
    
    return true;
}

UMaterial* FManagerOBJ::CreateMaterial(const FObjMaterialInfo& materialInfo)
{
    if (materialMap[materialInfo.MTLName] != nullptr)
        return materialMap[materialInfo.MTLName];

    UMaterial* newMaterial = FObjectFactory::ConstructObject<UMaterial>();
    newMaterial->SetMaterialInfo(materialInfo);
    materialMap.Add(materialInfo.MTLName, newMaterial);
    return newMaterial;
}

UMaterial* FManagerOBJ::GetMaterial(const FString& name)
{
    if (materialMap.Contains(name))
    {
        return materialMap[name];
    }
    return nullptr;
}

UStaticMesh* FManagerOBJ::CreateStaticMesh(const FString& filePath)
{
    OBJ::FStaticMeshRenderData* staticMeshRenderData = FManagerOBJ::LoadObjStaticMeshAsset(filePath);

    if (staticMeshRenderData == nullptr) return nullptr;

    UStaticMesh* staticMesh = GetStaticMesh(staticMeshRenderData->ObjectName);
    if (staticMesh != nullptr) {
        return staticMesh;
    }

    staticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
    staticMesh->SetData(staticMeshRenderData);

    staticMeshMap.Add(staticMeshRenderData->ObjectName, staticMesh);

    return staticMesh;
}

UStaticMesh* FManagerOBJ::GetStaticMesh(const FWString& name)
{
    if (staticMeshMap.Contains(name))
    {
        return staticMeshMap[name];
    }
    return nullptr;
}

void FManagerOBJ::CombineMaterialIndex(OBJ::FStaticMeshRenderData& OutFStaticMesh)
{
    for (int32 i = 0; i < OutFStaticMesh.MaterialSubsets.Num(); i++)
    {
        FString MatName = OutFStaticMesh.MaterialSubsets[i].MaterialName;
        for (int32 j = 0; j < OutFStaticMesh.Materials.Num(); j++)
        {
            if (OutFStaticMesh.Materials[j].MTLName == MatName)
            {
                OutFStaticMesh.MaterialSubsets[i].MaterialIndex = j;
                break;
            }
        }
    }
}

bool FManagerOBJ::SaveStaticMeshToBinary(const FWString& FilePath, const OBJ::FStaticMeshRenderData& StaticMesh)
{
    std::ofstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T SAVE STATIC MESH BINARY FILE");
        return false;
    }

    // Object Name
    Serializer::WriteFWString(File, StaticMesh.ObjectName);

    // Path Name
    Serializer::WriteFWString(File, StaticMesh.PathName);

    // Display Name
    Serializer::WriteFString(File, StaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = StaticMesh.Vertices.Num();
    File.write(reinterpret_cast<const char*>(&VertexCount), sizeof(VertexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Vertices.GetData()), VertexCount * sizeof(FVertexSimple));

    // Indices
    uint32 IndexCount = StaticMesh.Indices.Num();
    File.write(reinterpret_cast<const char*>(&IndexCount), sizeof(IndexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Materials
    uint32 MaterialCount = StaticMesh.Materials.Num();
    File.write(reinterpret_cast<const char*>(&MaterialCount), sizeof(MaterialCount));
    for (const FObjMaterialInfo& Material : StaticMesh.Materials)
    {
        Serializer::WriteFString(File, Material.MTLName);
        File.write(reinterpret_cast<const char*>(&Material.bHasTexture), sizeof(Material.bHasTexture));
        File.write(reinterpret_cast<const char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.write(reinterpret_cast<const char*>(&Material.Diffuse), sizeof(Material.Diffuse));
        File.write(reinterpret_cast<const char*>(&Material.Specular), sizeof(Material.Specular));
        File.write(reinterpret_cast<const char*>(&Material.Ambient), sizeof(Material.Ambient));
        File.write(reinterpret_cast<const char*>(&Material.Emissive), sizeof(Material.Emissive));
        File.write(reinterpret_cast<const char*>(&Material.SpecularScalar), sizeof(Material.SpecularScalar));
        File.write(reinterpret_cast<const char*>(&Material.DensityScalar), sizeof(Material.DensityScalar));
        File.write(reinterpret_cast<const char*>(&Material.TransparencyScalar), sizeof(Material.TransparencyScalar));
        File.write(reinterpret_cast<const char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        Serializer::WriteFString(File, Material.DiffuseTextureName);
        Serializer::WriteFWString(File, Material.DiffuseTexturePath);
        Serializer::WriteFString(File, Material.AmbientTextureName);
        Serializer::WriteFWString(File, Material.AmbientTexturePath);
        Serializer::WriteFString(File, Material.SpecularTextureName);
        Serializer::WriteFWString(File, Material.SpecularTexturePath);
        Serializer::WriteFString(File, Material.BumpTextureName);
        Serializer::WriteFWString(File, Material.BumpTexturePath);
        Serializer::WriteFString(File, Material.AlphaTextureName);
        Serializer::WriteFWString(File, Material.AlphaTexturePath);
        Serializer::WriteFString(File, Material.NormalTextureName);
        Serializer::WriteFWString(File, Material.NormalTexturePath);

    }

    // Material Subsets
    const uint32 SubsetCount = StaticMesh.MaterialSubsets.Num();
    File.write(reinterpret_cast<const char*>(&SubsetCount), sizeof(SubsetCount));
    for (const FMaterialSubset& Subset : StaticMesh.MaterialSubsets)
    {
        Serializer::WriteFString(File, Subset.MaterialName);
        File.write(reinterpret_cast<const char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.write(reinterpret_cast<const char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.write(reinterpret_cast<const char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMin), sizeof(FVector));
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMax), sizeof(FVector));
    
    File.close();
    return true;
}

TArray<FString> FManagerOBJ::ExtractMTLLibraryNamesFromOBJ(const FString& ObjFilePath)
{
    TArray<FString> MTLNames;

    // FString에서 ANSI 문자열로 변환 (프로젝트에 따라 ToAnsiString() 또는 operator* 사용)
#if USE_WIDECHAR
    //TODO : WCHAR 처리
#else
    std::ifstream File(*ObjFilePath);
#endif
    
    if (!File.is_open())
    {
        return MTLNames;
    }

    std::string Line;
    bool bFoundMtllib = false;  // mtllib 지시문을 한 번이라도 찾았는지 여부
    while (std::getline(File, Line))
    {
        // 선행 공백을 제거한다.
        size_t start = 0;
        while (start < Line.size() && std::isspace(static_cast<unsigned char>(Line[start])))
        {
            ++start;
        }

        // 빈 라인이나 주석(#)은 무시
        if (start >= Line.size() || Line[start] == '#')
        {
            continue;
        }

        // 이미 mtllib 라인을 하나 이상 찾은 상태에서, 지금 읽은 줄이 "mtllib"으로 시작하지 않는다면
        // 헤더 부분의 mtllib 블록이 끝난 것으로 판단하여 반복 종료
        if (bFoundMtllib && Line.compare(start, 6, "mtllib") != 0)
        {
            break;
        }
        
        // "mtllib"으로 시작하는지 확인한다.
        if (Line.compare(start, 6, "mtllib") == 0)
        {
            bFoundMtllib = true;
            // "mtllib" 다음의 공백 뒤부터 파일명 추출
            size_t tokenStart = start + 6;
            while (tokenStart < Line.size() && std::isspace(static_cast<unsigned char>(Line[tokenStart])))
            {
                ++tokenStart;
            }
            // 파일명이 끝나는 위치(공백 또는 끝날 때까지)
            size_t tokenEnd = tokenStart;
            while (tokenEnd < Line.size() && !std::isspace(static_cast<unsigned char>(Line[tokenEnd])))
            {
                ++tokenEnd;
            }
            // 파일명 부분 추출
            std::string MtlFileName = Line.substr(tokenStart, tokenEnd - tokenStart);

            FString MTLName = MtlFileName;

            FString MTLFilePath = ConvertMTLRelativePathToCurrent(ObjFilePath, MTLName);
            // 추출한 이름을 FString으로 변환하여 배열에 추가
            MTLNames.Add(MTLFilePath);
        }
    }
    File.close();
    return MTLNames;
}

FString FManagerOBJ::ConvertMTLRelativePathToCurrent(const FString& ObjFilePath, const FString& MTLFileName)
{
    // OBJ 파일의 절대 경로를 std::filesystem::path로 변환 (FString -> std::wstring 또는 std::string)
    const std::filesystem::path ObjPath = *ObjFilePath;

    // OBJ 파일의 부모 디렉토리 추출
    const std::filesystem::path ObjDir = ObjPath.parent_path();

    // MTL 파일의 절대 경로 : OBJ 파일의 부모 디렉토리와 mtllib 경로 결합
    const std::filesystem::path AbsoluteMTLPath = ObjDir / *MTLFileName;

    // 현재 작업 디렉터리 기준 상대 경로 계산
    const std::filesystem::path CurrentDir = std::filesystem::current_path();
    std::filesystem::path RelativeMTLPath;
    try
    {
        RelativeMTLPath = std::filesystem::relative(AbsoluteMTLPath, CurrentDir);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // 만약 상대 경로 계산에 실패하면 절대 경로를 사용할 수도 있습니다.
        std::cerr << "Relative path calculation error: " << e.what() << std::endl;
        RelativeMTLPath = AbsoluteMTLPath;
    }

#if USE_WIDECHAR
    return FString(RelativeMTLPath.wstring());
#else
    return FString(RelativeMTLPath.string());
#endif
}

void QEMSimplifier::Simplify(FObjInfo& obj, int targetVertexCount)
{
    int numVertices = obj.Vertices.Num();
    int numFaces = obj.VertexIndices.Num() / 3;

    // 1. 각 정점에 대한 QEM 행렬 계산 (각 정점에 인접한 삼각형 면들로부터)
    std::vector<Quadric> quadrics(numVertices);
    for (int i = 0; i < numFaces; i++)
    {
        int v0 = obj.VertexIndices[i * 3 + 0];
        int v1 = obj.VertexIndices[i * 3 + 1];
        int v2 = obj.VertexIndices[i * 3 + 2];
        
        FVector p0 = obj.Vertices[v0];
        FVector p1 = obj.Vertices[v1];
        FVector p2 = obj.Vertices[v2];
        
        FVector normal = (p1 - p0).Cross(p2 - p0).Normalize();
        float d = -normal.Dot(p0);
        
        Quadric q;
        q.data[0] = normal.X * normal.X;
        q.data[1] = normal.X * normal.Y;
        q.data[2] = normal.X * normal.Z;
        q.data[3] = normal.X * d;
        q.data[4] = normal.Y * normal.Y;
        q.data[5] = normal.Y * normal.Z;
        q.data[6] = normal.Y * d;
        q.data[7] = normal.Z * normal.Z;
        q.data[8] = normal.Z * d;
        q.data[9] = d * d;
        
        quadrics[v0].Add(q);
        quadrics[v1].Add(q);
        quadrics[v2].Add(q);
    }

    // 2. 면 정보로부터 실제로 연결된 엣지 후보 목록을 생성합니다.
    // 각 면의 엣지 (v0-v1, v1-v2, v2-v0)만 고려합니다.
    std::set<std::pair<IndexSet, IndexSet>> edgeSet;
    for (int i = 0; i < numFaces; i++)
    {
        uint32 v[3] = { obj.VertexIndices[i * 3 + 0],
        obj.VertexIndices[i * 3 + 1],
        obj.VertexIndices[i * 3 + 2] };
        uint32 n[3]= { obj.NormalIndices[i * 3 + 0],
        obj.NormalIndices[i * 3 + 1],
        obj.NormalIndices[i * 3 + 2] };
        uint32 t[3] = {obj.TextureIndices[i * 3 + 0],
        obj.TextureIndices[i * 3 + 1],
        obj.TextureIndices[i * 3 + 2] };
        // 정렬하여 (min, max) 쌍으로 저장
        for (int j = 0; j < 3; j++)
        {
            uint32 a = v[j], b = v[(j+1)%3];
            uint32 c = n[j], d = n[(j+1)%3];
            uint32 e = t[j], f = t[(j+1)%3];
            if (a > b)
            {
                std::swap(a, b);
                std::swap(c, d);
                std::swap(e, f);
            }
            IndexSet minEdge(a,c,e), maxEdge(b,e,f);
            edgeSet.insert({minEdge, maxEdge});
        }
    }
    
    // 3. 후보 엣지들을 우선순위 큐에 추가 (병합 가능한지 검사)
    std::priority_queue<EdgeCollapse> collapseQueue;
    for (const auto& edge : edgeSet)
    {
        IndexSet minEdge = edge.first, maxEdge = edge.second;

        FVector midpoint = (obj.Vertices[minEdge.verIndex] + obj.Vertices[maxEdge.verIndex]) * 0.5f;
        float cost = ComputeCollapseCost(quadrics[minEdge.verIndex], quadrics[maxEdge.verIndex], midpoint);
        collapseQueue.push({minEdge, maxEdge, cost, midpoint});
    }

    // 4. 목표 정점 개수까지 단순화 수행
    while (obj.Vertices.Num() > static_cast<size_t>(targetVertexCount) && !collapseQueue.empty())
    {
        EdgeCollapse bestCollapse = collapseQueue.top();
        collapseQueue.pop();
        uint32 v1 = bestCollapse.i1.verIndex,v2 = bestCollapse.i2.verIndex;
        uint32 n1 = bestCollapse.i1.norIndex, n2 = bestCollapse.i2.norIndex;
        uint32 t1 = bestCollapse.i1.texIndex, t2 = bestCollapse.i2.texIndex;
        
        // 병합 전 원래 v1과 v2의 위치와 UV 기록
        FVector oldPos_v1 = obj.Vertices[v1];
        FVector oldPos_v2 = obj.Vertices[v2];
        FVector normal_v1 = obj.Normals[n1];
        FVector normal_v2 = obj.Normals[n2];
        FVector2D uv_v1 = obj.UVs[t1];
        FVector2D uv_v2 = obj.UVs[t2];
        
        // obj.Vertices[v1] = (oldPos_v1 + oldPos_v2) * .5f;
        // obj.Normals[n1] = (normal_v1 + normal_v2) * 0.5f;
        // obj.Normals[n1] = obj.Normals[n1].Normalize();
        //
        //
        // float totalDist = (oldPos_v2 - oldPos_v1).Magnitude();
        // float movedDist = (bestCollapse.newPos - oldPos_v1).Magnitude();
        // float t = (totalDist > 1e-6f) ? (movedDist / totalDist) : 0.5f;
        // obj.UVs[t1] =  FVector2D::Lerp(uv_v1, uv_v2, t);
        // obj.UVs[t1] =  uv_v1;

        
        // 정점 삭제: v2 정점을 제거 (삭제 후 인덱스 재매핑 필요)
        obj.Vertices.RemoveAt(v2);
        // obj.Normals.RemoveAt(n2);
        // obj.UVs.RemoveAt(t2);
        
        // 인덱스 재매핑: 삭제된 정점 v2에 해당하는 인덱스는 v1로, v2보다 큰 인덱스는 1씩 감소
        for (auto& index : obj.VertexIndices)
        {
            if (index == v2) index = v1;
            else if (index > v2) index--;
        }
        // for (auto& index : obj.NormalIndices) {
        //     if (index == n2) index = n1;
        //     else if (index > n2) index--;
        // }
        // for (auto& index : obj.TextureIndices) {
        //     if (index == t2) index = t1;
        //     else if (index > t2) index--;
        // }


        edgeSet.clear();
        quadrics.clear();
        collapseQueue = std::priority_queue<EdgeCollapse>();
        std::set<std::pair<IndexSet, IndexSet>> edgeSet;
        for (int i = 0; i < numFaces; i++)
        {
            uint32 v[3] = { obj.VertexIndices[i * 3 + 0],
            obj.VertexIndices[i * 3 + 1],
            obj.VertexIndices[i * 3 + 2] };
            uint32 n[3]= { obj.NormalIndices[i * 3 + 0],
            obj.NormalIndices[i * 3 + 1],
            obj.NormalIndices[i * 3 + 2] };
            uint32 t[3] = {obj.TextureIndices[i * 3 + 0],
            obj.TextureIndices[i * 3 + 1],
            obj.TextureIndices[i * 3 + 2] };
            if ((v[0] == v[1] || v[1] == v[2] || v[2] == v[0]))
            {
                obj.VertexIndices.RemoveAt((i*3+2));
                obj.VertexIndices.RemoveAt(i*3+1);
                obj.VertexIndices.RemoveAt(i*3);
                obj.NormalIndices.RemoveAt((i*3+2));
                obj.NormalIndices.RemoveAt(i*3+1);
                obj.NormalIndices.RemoveAt(i*3);
                obj.TextureIndices.RemoveAt((i*3+2));
                obj.TextureIndices.RemoveAt(i*3+1);
                obj.TextureIndices.RemoveAt(i*3);
 
                numFaces--;
                i--;
                continue;
            }
            // 정렬하여 (min, max) 쌍으로 저장
            for (int j = 0; j < 3; j++)
            {
                uint32 a = v[j], b = v[(j+1)%3];
                uint32 c = n[j], d = n[(j+1)%3];
                uint32 e = t[j], f = t[(j+1)%3];
                if (a > b)
                {
                    std::swap(a, b);
                    std::swap(c, d);
                    std::swap(e, f);
                }
                edgeSet.insert({{a,c,e}, {b,d,f}});
            }
        }
        
        numVertices = obj.Vertices.Num();
        numFaces = obj.VertexIndices.Num() / 3;
        quadrics.resize(numVertices);
        for (int i = 0; i < numFaces; i++)
        {
            int v0 = obj.VertexIndices[i * 3 + 0];
            int v1 = obj.VertexIndices[i * 3 + 1];
            int v2 = obj.VertexIndices[i * 3 + 2];
            
            FVector p0 = obj.Vertices[v0];
            FVector p1 = obj.Vertices[v1];
            FVector p2 = obj.Vertices[v2];
            
            FVector normal = (p1 - p0).Cross(p2 - p0).Normalize();
            float d = -normal.Dot(p0);
            
            Quadric q;
            q.data[0] = normal.X * normal.X;
            q.data[1] = normal.X * normal.Y;
            q.data[2] = normal.X * normal.Z;
            q.data[3] = normal.X * d;
            q.data[4] = normal.Y * normal.Y;
            q.data[5] = normal.Y * normal.Z;
            q.data[6] = normal.Y * d;
            q.data[7] = normal.Z * normal.Z;
            q.data[8] = normal.Z * d;
            q.data[9] = d * d;
            
            quadrics[v0].Add(q);
            quadrics[v1].Add(q);
            quadrics[v2].Add(q);
        }
        for (const auto& edge : edgeSet)
        {
            IndexSet minEdge = edge.first, maxEdge = edge.second;
            FVector midpoint = (obj.Vertices[minEdge.verIndex] + obj.Vertices[maxEdge.verIndex]) * 0.5f;
            float cost = ComputeCollapseCost(quadrics[minEdge.verIndex], quadrics[maxEdge.verIndex], midpoint);
            collapseQueue.push({minEdge, maxEdge, cost, midpoint});
        }
    }
}

float QEMSimplifier::ComputeCollapseCost(const Quadric& q1, const Quadric& q2, const FVector& newPos)
{
    Quadric q = q1;
    q.Add(q2);

    // 4D 벡터 확장: newPos = (x, y, z, 1)
    const float x = newPos.X;
    const float y = newPos.Y;
    const float z = newPos.Z;

    // newPos^T * Q * newPos 계산
    const float cost =
        q.data[0] * x * x + 2.0f * q.data[1] * x * y + 2.0f * q.data[2] * x * z + 2.0f * q.data[3] * x +  
        q.data[4] * y * y + 2.0f * q.data[5] * y * z + 2.0f * q.data[6] * y +
        q.data[7] * z * z + 2.0f * q.data[8] * z +
        q.data[9];  // d*d 항

    return cost;
}

float QEMSimplifier::ComputeCollapseCost(const Quadric& q1, const Quadric& q2, const FVector& newPos, const FVector2D& uv1, const FVector2D& uv2,
                                         const FVector& normal1, const FVector& normal2)
{
    Quadric q = q1;
    q.Add(q2);

    const float x = newPos.X;
    const float y = newPos.Y;
    const float z = newPos.Z;

    // 기본 기하학적 비용 계산: newPos^T * Q * newPos
    const float geomCost =
        q.data[0] * x * x + 2.0f * q.data[1] * x * y + 2.0f * q.data[2] * x * z + 2.0f * q.data[3] * x +
        q.data[4] * y * y + 2.0f * q.data[5] * y * z + 2.0f * q.data[6] * y +
        q.data[7] * z * z + 2.0f * q.data[8] * z +
        q.data[9];

    // UV 차이에 따른 비용 (단순 예제: 두 UV 사이의 유클리드 거리의 제곱)
    const float du = uv1.x - uv2.x;
    const float dv = uv1.y - uv2.y;
    const float uvCost = du * du + dv * dv;

    // 노말 차이에 따른 비용 (두 노말의 코사인 유사도 기반, 1 - DotProduct)
    const float normalDiff = 1.0f - normal1.Dot(normal2);
    const float normalCost = normalDiff * normalDiff;

    // 가중치 인자: alpha와 beta는 조절 가능한 파라미터
    constexpr float alpha = .1f;  // UV 비용에 대한 가중치
    constexpr float beta = .1f;   // 노말 비용에 대한 가중치

    const float totalCost = geomCost + alpha * uvCost + beta * normalCost;
    return totalCost;
}

