// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RMeshImportData.generated.h"

USTRUCT(BlueprintType)
struct FRMeshVertex
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position;

	UPROPERTY()
	FVector2D UV;

	UPROPERTY()
	FVector Normal;

	UPROPERTY()
	FColor Color;

	FRMeshVertex()
		: Position(FVector::ZeroVector)
		, UV(FVector2D::ZeroVector)
		, Normal(FVector::UpVector)
		, Color(FColor::White)
	{
	}
};

USTRUCT(BlueprintType)
struct FRMeshTriangle
{
	GENERATED_BODY()

	UPROPERTY()
	int32 VertexIndex0;

	UPROPERTY()
	int32 VertexIndex1;

	UPROPERTY()
	int32 VertexIndex2;

	UPROPERTY()
	int32 MaterialIndex;

	UPROPERTY()
	FVector FaceNormal;

	UPROPERTY()
	FVector2D UV0;

	UPROPERTY()
	FVector2D UV1;

	UPROPERTY()
	FVector2D UV2;

	FRMeshTriangle()
		: VertexIndex0(0)
		, VertexIndex1(0)
		, VertexIndex2(0)
		, MaterialIndex(0)
		, FaceNormal(FVector::UpVector)
	{
		
	}
};

USTRUCT(BlueprintType)
struct FRMeshMaterial
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	TArray<FString> Textures;

	FRMeshMaterial()
	{
	}
};

UCLASS()
class URMeshImportData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FRMeshVertex> Vertices;

	UPROPERTY()
	TArray<FRMeshTriangle> Triangles;

	UPROPERTY()
	TArray<FRMeshMaterial> Materials;

	UPROPERTY()
	float RoomScale;

	URMeshImportData()
		: RoomScale(1.0f)
	{
	}
};
