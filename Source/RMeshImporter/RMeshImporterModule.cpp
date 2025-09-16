// Fill out your copyright notice in the Description page of Project Settings.


#include "RMeshImporterModule.h"

#define LOCTEXT_NAMESPACE "FRMeshImporterModule"

void FRMeshImporterModule::StartupModule()
{
	IModuleInterface::StartupModule();
}

void FRMeshImporterModule::ShutdownModule()
{
	IModuleInterface::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FRMeshImporterModule, RMeshImporter)
