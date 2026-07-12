#pragma once

#include <string>
#include <vector>
#include <map>

namespace Instances
{
	typedef struct
	{
		ULevel* CurLevel;
		std::string LevelName;
		std::string HostSteamID;
	} LevelData;

	
	void Initialize();
	void Update(bool GameThread = true);
	
	UWorld* GetWorld();
	LevelData* GetLevelData();
	UEngine* GetEngine();
	APawn* GetLocalPawn();
	ULocalPlayer* GetLocalPlayer();
	UGameplayStatics* GetGPStatics();
	UKismetTextLibrary* GetTextLib();
	UKismetStringLibrary* GetStringLib();
	UKismetSystemLibrary* GetSystemLib();
	APlayerController* GetPlayerController();
	ABPCharacter_Demo_C* GetPlayerCharacter();
	FVector GetPlayerLocation();


	UClass* FindStaticClass(const std::string& className);
	UFunction* FindStaticFunction(const std::string& functionName);

	std::string GetSteamID(ABPCharacter_Demo_C* Demo);

	
	template<typename T>
	__declspec(noinline) T* GetInstanceOf()
	{
		if (!std::is_base_of<UObject, T>::value) return nullptr;
		
		for (int i = UObject::GObjects->Num() - 1; i >= 0; --i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;
			
			if (obj->GetFullName().find(xorstr_("Default__")) != std::string::npos)
				continue;

			return static_cast<T*>(obj);
		}

		return nullptr;
	}

	
	template<typename T>
	__declspec(noinline) std::vector<T*> GetAllInstancesOf()
	{
		std::vector<T*> instances;

		if (!std::is_base_of<UObject, T>::value) return instances;

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;
			
			if (obj->GetFullName().find(xorstr_("Default__")) != std::string::npos)
				continue;

			instances.push_back(static_cast<T*>(obj));
		}

		return instances;
	}
	
	template<typename T>
	__declspec(noinline) T* FindObject(const std::string& objectName)
	{
		if (!std::is_base_of<UObject, T>::value) return nullptr;

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;

			if (obj->GetName().find(objectName) != std::string::npos)
			{
				return static_cast<T*>(obj);
			}
		}

		return nullptr;
	}


}
