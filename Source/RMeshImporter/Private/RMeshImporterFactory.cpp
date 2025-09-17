// Fill out your copyright notice in the Description page of Project Settings.


#include "RMeshImporterFactory.h"

#include "ObjectTools.h"
#include "PluginUtils.h"
#include "RMeshImportData.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInstanceConstant.h"


#define LOCTEXT_NAMESPACE "RMeshImporter"

URMeshImporterFactory::URMeshImporterFactory()
{
    SupportedClass = UStaticMesh::StaticClass();
    Formats.Add(TEXT("rmesh;RMesh File"));
    bCreateNew = false;
    bEditorImport = true;
}

UObject* URMeshImporterFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    bOutOperationCanceled = false;

    // Create import data object
    URMeshImportData* ImportData = NewObject<URMeshImportData>(GetTransientPackage(), NAME_None, Flags);
    ImportData->RoomScale = 1.0f; // Adjust as needed

    // Import the RMesh file
    if (!ImportRMesh(Filename, ImportData, Warn))
    {
        return nullptr;
    }

    // Create static mesh
    FString MeshName = ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(Filename));
    FString PackageName = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName()) + TEXT("/") + MeshName;
    UPackage* Package = CreatePackage(*PackageName);
    UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, *MeshName, Flags | RF_Public | RF_Standalone);

    // Process the imported data
    ProcessImportData(ImportData, StaticMesh, Package, *MeshName, Flags);

    // Set import data
    UAssetImportData* ImportDataAsset = NewObject<UAssetImportData>(StaticMesh, TEXT("AssetImportData"));
    ImportDataAsset->SourceData.SourceFiles.Add(FAssetImportInfo::FSourceFile(Filename));
    StaticMesh->AssetImportData = ImportDataAsset;

    // Build the mesh
    StaticMesh->Build();
    StaticMesh->PostEditChange();

    return StaticMesh;
}

bool URMeshImporterFactory::FactoryCanImport(const FString& Filename)
{
    return FPaths::GetExtension(Filename).Equals(TEXT("rmesh"), ESearchCase::IgnoreCase);
}

bool URMeshImporterFactory::ImportRMesh(const FString& Filename, URMeshImportData* ImportData, FFeedbackContext* Warn)
{
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *Filename))
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("Failed to load file: %s"), *Filename);
        return false;
    }

    FMemoryReader Reader(FileData, true);
    Reader.SetByteSwapping(false);

    // Read header string
    int32 StringLength;
    Reader << StringLength;
    
    TArray<uint8> StringBytes;
    StringBytes.SetNum(StringLength);
    Reader.Serialize(StringBytes.GetData(), StringLength);
    FString HeaderStr=FString(StringLength,(const ANSICHAR*)StringBytes.GetData());
    //FString HeaderStr = FString(UTF8_TO_TCHAR(StringBytes.GetData()));
    
    if (HeaderStr != TEXT("RoomMesh") && HeaderStr != TEXT("RoomMesh.HasTriggerBox"))
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("Invalid header: %s"), *HeaderStr);
        return false;
    }

    // Read surface count
    int32 SurfaceCount;
    Reader << SurfaceCount;

    // Process surfaces
    for (int32 i = 0; i < SurfaceCount; i++)
    {
        FRMeshMaterial Material;
        Material.Name = FString::Printf(TEXT("drawnmesh%d"), i);

        // Read blend types and textures
        for (int32 j = 0; j < 2; j++)
        {
            uint8 BlendType;
            Reader << BlendType;
            
            if (BlendType != 0)
            {
                int32 TexStringLength;
                Reader << TexStringLength;
                
                TArray<uint8> TexStringBytes;
                TexStringBytes.SetNum(TexStringLength);
                Reader.Serialize(TexStringBytes.GetData(), TexStringLength);
                //FString TextureName = FString(UTF8_TO_TCHAR(TexStringBytes.GetData())); 
                FString TextureName = FString(TexStringLength,(const ANSICHAR*)TexStringBytes.GetData());
                if (!TextureName.IsEmpty())
                {
                    FString BaseName= FPaths::GetBaseFilename(TextureName);
                    if (BaseName.Contains("_lm1")
                        || BaseName.Contains("_lm2")
                        || BaseName.Contains("_lm3")
                            || BaseName.Contains("_lm4")
                            || BaseName.Contains("_lm5")
                            || BaseName.Contains("_lm6")
                            || BaseName.Contains("_lm7"))
                    {
                        continue;
                    }
                    Material.Textures.Add(TextureName);
                }
            }
        }

        ImportData->Materials.Add(Material);

        // Read vertices
        int32 VertexCount;
        Reader << VertexCount;
        
        int32 VertexStartIndex = ImportData->Vertices.Num();
        
        for (int32 j = 0; j < VertexCount; j++)
        {
            FRMeshVertex Vertex;
            
            float X, Y, Z;
            Reader << X;
            Reader << Y;
            Reader << Z;
            
            Vertex.Position = FVector(-X,Z,Y) * ImportData->RoomScale;
            
            float U, V;
            Reader << U;
            Reader << V;
            Vertex.UV = FVector2D(U, V);
            
            // Skip two floats (unknown data)
            float Unknown1, Unknown2;
            Reader << Unknown1;
            Reader << Unknown2;
            
            // Read color
            uint8 R, G, B;
            Reader << R;
            Reader << G;
            Reader << B;
            Vertex.Color = FColor(R, G, B, 255);
            
            ImportData->Vertices.Add(Vertex);
        }

        // Read triangles
        int32 TriangleCount;
        Reader << TriangleCount;
        
        for (int32 j = 0; j < TriangleCount; j++)
        {
            FRMeshTriangle Triangle;
            
            Reader << Triangle.VertexIndex0;
            Reader << Triangle.VertexIndex1;
            Reader << Triangle.VertexIndex2;
            
            // Convert indices to global indices
            Triangle.VertexIndex0 += VertexStartIndex;
            Triangle.VertexIndex1 += VertexStartIndex;
            Triangle.VertexIndex2 += VertexStartIndex;
            
            Triangle.MaterialIndex = i;
            
            // Calculate face normal
            FVector V0 = ImportData->Vertices[Triangle.VertexIndex0].Position;
            FVector V1 = ImportData->Vertices[Triangle.VertexIndex1].Position;
            FVector V2 = ImportData->Vertices[Triangle.VertexIndex2].Position;
            
            FVector U = V1 - V0;
            FVector V = V2 - V0;
            
            Triangle.FaceNormal = FVector(
                (U.Y * V.Z) - (U.Z * V.Y),
                (U.Z * V.X) - (U.X * V.Z),
                (U.X * V.Y) - (U.Y * V.X)
            );
            Triangle.FaceNormal = Triangle.FaceNormal.GetSafeNormal();
            
            // Set UVs
            Triangle.UV0 = ImportData->Vertices[Triangle.VertexIndex0].UV;
            Triangle.UV1 = ImportData->Vertices[Triangle.VertexIndex1].UV;
            Triangle.UV2 = ImportData->Vertices[Triangle.VertexIndex2].UV;
            
            ImportData->Triangles.Add(Triangle);
        }
    }

    if (!IgnoreCollision)
    {
        // Read collision surfaces
        Reader << SurfaceCount;

        for (int32 i = 0; i < SurfaceCount; i++)
        {
            FRMeshMaterial Material;
            Material.Name = FString::Printf(TEXT("collisionmesh%d"), i);
            ImportData->Materials.Add(Material);

            // Read vertices
            int32 VertexCount;
            Reader << VertexCount;

            int32 VertexStartIndex = ImportData->Vertices.Num();

            for (int32 j = 0; j < VertexCount; j++)
            {
                FRMeshVertex Vertex;

                float X, Y, Z;
                Reader << X;
                Reader << Y;
                Reader << Z;

                Vertex.Position = FVector(X, Y, -Z) * ImportData->RoomScale;
                Vertex.UV = FVector2D::ZeroVector;
                Vertex.Color = FColor::White;

                ImportData->Vertices.Add(Vertex);
            }

            // Read triangles
            int32 TriangleCount;
            Reader << TriangleCount;

            for (int32 j = 0; j < TriangleCount; j++)
            {
                FRMeshTriangle Triangle;

                Reader << Triangle.VertexIndex0;
                Reader << Triangle.VertexIndex1;
                Reader << Triangle.VertexIndex2;

                // Convert indices to global indices
                Triangle.VertexIndex0 += VertexStartIndex;
                Triangle.VertexIndex1 += VertexStartIndex;
                Triangle.VertexIndex2 += VertexStartIndex;

                Triangle.MaterialIndex = ImportData->Materials.Num() - 1; // Last material index

                // Calculate face normal
                FVector V0 = ImportData->Vertices[Triangle.VertexIndex0].Position;
                FVector V1 = ImportData->Vertices[Triangle.VertexIndex1].Position;
                FVector V2 = ImportData->Vertices[Triangle.VertexIndex2].Position;

                FVector U = V1 - V0;
                FVector V = V2 - V0;

                Triangle.FaceNormal = FVector::CrossProduct(U, V).GetSafeNormal();
    
                
                Triangle.FaceNormal = FVector(
                    Triangle.FaceNormal.X,  
                    Triangle.FaceNormal.Z,   
                    Triangle.FaceNormal.Y    
                ).GetSafeNormal();

                

                // Set UVs
                Triangle.UV0 = ImportData->Vertices[Triangle.VertexIndex0].UV;
                Triangle.UV1 = ImportData->Vertices[Triangle.VertexIndex1].UV;
                Triangle.UV2 = ImportData->Vertices[Triangle.VertexIndex2].UV;

                ImportData->Triangles.Add(Triangle);
            }
        }
    }

    return true;
}

void URMeshImporterFactory::ProcessImportData(URMeshImportData* ImportData, UStaticMesh* StaticMesh, UObject* InParent, FName InName, EObjectFlags Flags)
{
    FString ModelName = StaticMesh->GetName();
    // Create mesh description
    FMeshDescription MeshDescription;
    FStaticMeshAttributes Attributes(MeshDescription);
    Attributes.Register();
    
    // Add vertex positions
    TArray<FVertexID> VertexIDs;
    VertexIDs.SetNum(ImportData->Vertices.Num());
    
    for (int32 i = 0; i < ImportData->Vertices.Num(); i++)
    {
        VertexIDs[i] = MeshDescription.CreateVertex();
        Attributes.GetVertexPositions()[VertexIDs[i]] = FVector3f(ImportData->Vertices[i].Position);
    }
    
    // Create material slots
    TArray<FPolygonGroupID> PolygonGroupIDs;
    PolygonGroupIDs.SetNum(ImportData->Materials.Num());
    
    UMaterial* ParentMaterial=LoadObject<UMaterial>(nullptr,TEXT("/RMeshImporter/M_BaseMaterial.M_BaseMaterial"));
    if (ParentMaterial==nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RMeshImporter: Can't find parent material"));
        return;
    }
    
    for (int32 i = 0; i < ImportData->Materials.Num(); i++)
    {
        PolygonGroupIDs[i] = MeshDescription.CreatePolygonGroup();
        
        // Create material
        FString OriginalMaterialName = ImportData->Materials[i].Name;
        FString MaterialName = FString::Printf(TEXT("%s_%s"),*ModelName,*OriginalMaterialName);
        MaterialName=ObjectTools::SanitizeObjectName(MaterialName);
        FString MaterialPath = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName()) + TEXT("/") + MaterialName;
        UPackage* MaterialPackage = CreatePackage(*MaterialPath);
        UMaterialInstanceConstant* Material = NewObject<UMaterialInstanceConstant>(MaterialPackage, *MaterialName, Flags | RF_Public | RF_Standalone);
        
        // Set parent material (use a default lit material)
        Material->SetParentEditorOnly(ParentMaterial);
        
        // TODO: Apply textures if available
        if (ImportData->Materials[i].Textures.Num() > 0)
        {
            for (int32 j=0;j<ImportData->Materials[i].Textures.Num();j++)
            {
                FString TexturePath=ImportData->Materials[i].Textures[j];
                if (TexturePath.IsEmpty()) continue;
                FString TextureName=FPaths::GetBaseFilename(TexturePath);
                UTexture* Texture=LoadObject<UTexture>(nullptr,  *FString::Printf(TEXT("/Game/CBAsset/Texture/%s"),*TextureName));
                if (Texture==nullptr)
                {
                    UE_LOG(LogTemp,Warning,TEXT("Can't find texture %s"),*TextureName);
                    continue;
                }
                if (j==0)
                {
                    Material->SetTextureParameterValueEditorOnly(FName("BaseColor"),Texture);
                }
                else if (j==1)
                {
                    // We have some normal maps, but it's not using them. Perhaps we can manually assign normal maps to the materials
                    Material->SetScalarParameterValueEditorOnly(FName("UseNormal"),1);
                    Material->SetTextureParameterValueEditorOnly(FName("Normal"),Texture);
                }
            }
        }
        
        // Assign material to static mesh
        StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Material, *MaterialName));
        bool bSuccess = MaterialPackage->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Material);
    }
    
    // Create UV and normal attributes
    TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
    TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
    
    // Create vertex instances and triangles
    for (const FRMeshTriangle& Triangle : ImportData->Triangles)
    {
        if (Triangle.VertexIndex0 >= VertexIDs.Num() || 
            Triangle.VertexIndex1 >= VertexIDs.Num() || 
            Triangle.VertexIndex2 >= VertexIDs.Num())
        {
            continue; // Skip invalid triangles
        }
        
        // Create vertex instances for this triangle
        TArray<FVertexInstanceID> VertexInstanceIDs;
        VertexInstanceIDs.SetNum(3);
        
        FVertexID VertexIDsForTriangle[3] = {
            VertexIDs[Triangle.VertexIndex0],
            // VertexIDs[Triangle.VertexIndex1],
            VertexIDs[Triangle.VertexIndex2],
            VertexIDs[Triangle.VertexIndex1]
        };
        
        FVector2D UVs[3] = {
            Triangle.UV0,
            // Triangle.UV1,
            Triangle.UV2,
            Triangle.UV1
        };
        
        for (int32 i = 0; i < 3; i++)
        {
            VertexInstanceIDs[i] = MeshDescription.CreateVertexInstance(VertexIDsForTriangle[i]);
            VertexInstanceUVs[VertexInstanceIDs[i]] = FVector2f(UVs[i]); // Convert FVector2D to FVector2f
            VertexInstanceNormals[VertexInstanceIDs[i]] = FVector3f(Triangle.FaceNormal); // Convert FVector to FVector3f
        }
        
        // Create polygon
        MeshDescription.CreatePolygon(PolygonGroupIDs[Triangle.MaterialIndex], VertexInstanceIDs);
    }
    
    // Build static mesh from description
    UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
    BuildParams.bBuildSimpleCollision = true;
    BuildParams.bFastBuild = true;
    
    TArray<const FMeshDescription*> MeshDescriptions;
    MeshDescriptions.Add(&MeshDescription);
    
    StaticMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams);
}

#undef LOCTEXT_NAMESPACE
