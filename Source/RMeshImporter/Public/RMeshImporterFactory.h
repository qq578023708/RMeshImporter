// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "UObject/Object.h"
#include "RMeshImporterFactory.generated.h"

class URMeshImportData;
/**
 * 
 */
UCLASS()
class RMESHIMPORTER_API URMeshImporterFactory : public UFactory
{
	GENERATED_BODY()
public:
	URMeshImporterFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	//~ End UFactory Interface

private:
	bool ImportRMesh(const FString& Filename, URMeshImportData* ImportData, FFeedbackContext* Warn);
	void ProcessImportData(URMeshImportData* ImportData, UStaticMesh* StaticMesh, UObject* InParent, FName InName, EObjectFlags Flags);
	bool IgnoreCollision=true;
};
