#include "Modules/ModuleManager.h"
#include "TDMainMenuWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"

class FTriadDriftHUDModule : public FDefaultModuleImpl
{
public:
    virtual void StartupModule() override
    {
#if !UE_BUILD_SHIPPING
        // Explicit development review only: the real engineering host still owns all flow/actions.
        // No map, data asset, configuration or package is saved. Never activates in Shipping.
        if(FParse::Param(FCommandLine::Get(),TEXT("TDUIMenuReview")))
            ReviewHandle=FWorldDelegates::OnPostWorldInitialization.AddRaw(this,&FTriadDriftHUDModule::PrepareMenuWorld);
#endif
    }
    virtual void ShutdownModule() override
    {
#if !UE_BUILD_SHIPPING
        FWorldDelegates::OnPostWorldInitialization.Remove(ReviewHandle);
        for(const auto& Entry:ReviewWorlds)
            if(UWorld* World=Entry.Key.Get()) World->RemoveOnActorSpawnedHandler(Entry.Value);
        ReviewWorlds.Empty();
#endif
    }
private:
#if !UE_BUILD_SHIPPING
    FDelegateHandle ReviewHandle;
    TMap<TWeakObjectPtr<UWorld>,FDelegateHandle> ReviewWorlds;
    void PrepareMenuWorld(UWorld* World,const UWorld::InitializationValues)
    {
        if(!World || !World->IsGameWorld() || !World->GetMapName().Contains(TEXT("L_MainMenu"))) return;
        ReviewWorlds.Add(World,World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateRaw(this,&FTriadDriftHUDModule::PrepareMenuReview)));
        if(AGameModeBase* Existing=World->GetAuthGameMode()) PrepareMenuReview(Existing);
    }
    void PrepareMenuReview(AActor* Actor)
    {
        // Set the actual newly spawned host instance, before StartPlay resolves its widget.
        // A native actor's constructor may reset a CDO-only runtime override.
        UClass* Host=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDMenuGameMode"));
        if(!Actor || !Host || !Actor->IsA(Host)) return;
        FClassProperty* Slot=Host?FindFProperty<FClassProperty>(Host,TEXT("MenuWidgetClass")):nullptr;
        if(!Slot || !UTDMainMenuWidget::StaticClass()->IsChildOf(Slot->MetaClass))
        {
            UE_LOG(LogTemp,Error,TEXT("TD_MENU_REVIEW refused: real host/widget slot unavailable"));
            return;
        }
        // Refuse to replace a configured menu; never overwrite another implementation.
        if(Slot->GetObjectPropertyValue_InContainer(Actor))
        {
            UE_LOG(LogTemp,Error,TEXT("TD_MENU_REVIEW refused: host already has an explicit widget"));
            return;
        }
        Slot->SetObjectPropertyValue_InContainer(Actor,UTDMainMenuWidget::StaticClass());
        UE_LOG(LogTemp,Display,TEXT("TD_MENU_REVIEW actual host instance=%s widget=%s; no Shipping/default/style activation"),*Actor->GetPathName(),*GetNameSafe(Slot->GetObjectPropertyValue_InContainer(Actor)));
    }
#endif
};
IMPLEMENT_MODULE(FTriadDriftHUDModule,TriadDriftHUD)
