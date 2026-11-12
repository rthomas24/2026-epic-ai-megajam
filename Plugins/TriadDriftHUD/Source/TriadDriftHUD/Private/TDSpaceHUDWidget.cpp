#include "TDSpaceHUDWidget.h"
#include "Engine/GameInstance.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"

namespace TDHUD
{
const TCHAR* Names[] = { TEXT("RIFT"), TEXT("LANCER"), TEXT("BASTION") };
const TCHAR* Pilots[] = { TEXT("WREN / TRICKSTER"), TEXT("KATO / INTERCEPTOR"), TEXT("MOSS / HEAVY") };
const TCHAR* Powers[] = { TEXT("PORTAL BLASTER"), TEXT("AFTERBURNER RAM"), TEXT("MISSILE SWARM") };
const TCHAR* Abilities[] = { TEXT("CLOAK"), TEXT("EMP PULSE"), TEXT("GRAVITY WELL") };

class FInputObserver : public IInputProcessor
{
    TWeakObjectPtr<UTDSpaceHUDWidget> Widget;
public:
    explicit FInputObserver(UTDSpaceHUDWidget* InWidget):Widget(InWidget) {}
    virtual void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event) override
    { if(Widget.IsValid()) Widget->bGamepadGlyphs=Event.GetKey().IsGamepadKey(); return false; }
    virtual bool HandleAnalogInputEvent(FSlateApplication&,const FAnalogInputEvent& Event) override
    { if(Widget.IsValid() && FMath::Abs(Event.GetAnalogValue())>.2f) Widget->bGamepadGlyphs=Event.GetKey().IsGamepadKey(); return false; }
    virtual bool HandleMouseMoveEvent(FSlateApplication&,const FPointerEvent& Event) override
    { if(Widget.IsValid() && !Event.GetCursorDelta().IsNearlyZero()) Widget->bGamepadGlyphs=false; return false; }
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override
    { if(Widget.IsValid()) Widget->bGamepadGlyphs=false; return false; }
};

// The bridge calls the actual reflected ITDHUDData functions. No replacement interface,
// duplicate gameplay struct or dependency on a project module from a plugin is needed.
struct FResult
{
    UFunction* Function = nullptr;
    TUniquePtr<FStructOnScope> Parameters;
    FProperty* Return = nullptr;
    FResult(UObject* Object, FName Name)
    {
        if (!IsValid(Object)) return;
        Function = Object->FindFunction(Name);
        if (!Function) return;
        // This bridge is only for zero-input getters. Refuse an incompatible future signature.
        for (TFieldIterator<FProperty> It(Function); It; ++It)
            if (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm)) return;
        Return = Function->GetReturnProperty();
        if (!Return) return;
        Parameters = MakeUnique<FStructOnScope>(Function);
        Object->ProcessEvent(Function, Parameters->GetStructMemory());
    }
    void* Value() const { return Parameters && Return ? Return->ContainerPtrToValuePtr<void>(Parameters->GetStructMemory()) : nullptr; }
};
float Number(const UStruct* Type, const void* Memory, FName Name, float Default = 0)
{
    const FNumericProperty* P = FindFProperty<FNumericProperty>(Type, Name);
    if (!P || !Memory) return Default;
    const void* Value = P->ContainerPtrToValuePtr<void>(Memory);
    return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(Value) : P->GetSignedIntPropertyValue(Value);
}
bool Flag(const UStruct* Type, const void* Memory, FName Name)
{
    const FBoolProperty* P = FindFProperty<FBoolProperty>(Type, Name);
    return P && Memory && P->GetPropertyValue_InContainer(Memory);
}
float Scalar(UObject* O, FName Name)
{
    FResult R(O, Name);
    if(const FEnumProperty* E=CastField<FEnumProperty>(R.Return))
        if(R.Value()) return E->GetUnderlyingProperty()->GetSignedIntPropertyValue(R.Value());
    if (const FNumericProperty* P = CastField<FNumericProperty>(R.Return))
        if (R.Value()) return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(R.Value()) : P->GetSignedIntPropertyValue(R.Value());
    return 0;
}
bool Bool(UObject* O,FName Name)
{
    FResult R(O,Name); const FBoolProperty* P=CastField<FBoolProperty>(R.Return);
    return P && R.Value() && P->GetPropertyValue(R.Value());
}
UActorComponent* Component(AActor* Actor,const TCHAR* ClassPath)
{
    UClass* Class=FindObject<UClass>(nullptr,ClassPath);
    return Actor && Class ? Actor->FindComponentByClass(Class) : nullptr;
}
bool GameplayLead(UObject* Component,FVector& Out)
{
    UFunction* Function=Component?Component->FindFunction(TEXT("GetLeadIndicator")):nullptr;
    if(!Function) return false;
    const FStructProperty* Position=FindFProperty<FStructProperty>(Function,TEXT("OutWorldLocation"));
    const FBoolProperty* Result=CastField<FBoolProperty>(Function->GetReturnProperty());
    if(!Position || Position->Struct!=TBaseStructure<FVector>::Get() || !Result) return false;
    FStructOnScope Params(Function); Component->ProcessEvent(Function,Params.GetStructMemory());
    if(!Result->GetPropertyValue_InContainer(Params.GetStructMemory())) return false;
    Out=*Position->ContainerPtrToValuePtr<FVector>(Params.GetStructMemory()); return !Out.ContainsNaN();
}
AActor* Actor(UObject* O, FName Name)
{
    FResult R(O, Name);
    const FObjectPropertyBase* P = CastField<FObjectPropertyBase>(R.Return);
    return P && R.Value() ? Cast<AActor>(P->GetObjectPropertyValue(R.Value())) : nullptr;
}
void ActorArray(UObject* O, FName Name, TArray<TWeakObjectPtr<AActor>>& Out)
{
    Out.Reset();
    FResult R(O, Name);
    const FArrayProperty* P = CastField<FArrayProperty>(R.Return);
    const FObjectPropertyBase* Inner = P ? CastField<FObjectPropertyBase>(P->Inner) : nullptr;
    if (!Inner || !R.Value()) return;
    FScriptArrayHelper Array(P, R.Value());
    for (int32 I = 0; I < FMath::Min(Array.Num(), 24); ++I)
        if (AActor* A = Cast<AActor>(Inner->GetObjectPropertyValue(Array.GetRawPtr(I)))) Out.Add(A);
}
float Unit(float Value) { return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.f, 1.f) : 0.f; }

// Retained geometry stays in Slate/UMG. Only meters and short feedback animations interpolate.
struct FPainter
{
    const FGeometry& G;
    FSlateWindowElementList& List;
    int32 Layer;
    float Scale;
    FVector2D Origin;
    const FTDHUDStyle& Style;
    FPainter(const FGeometry& InG, FSlateWindowElementList& InList, int32 InLayer, const FTDHUDStyle& InStyle,
        FVector2D Design = FVector2D(1920,1080)) : G(InG), List(InList), Layer(InLayer), Style(InStyle)
    {
        Scale = FMath::Min(G.GetLocalSize().X / Design.X, G.GetLocalSize().Y / Design.Y);
        Origin = (G.GetLocalSize() - Design * Scale) * .5;
    }
    FPaintGeometry Geo(FVector2D Pos, FVector2D Size) const
    { return G.ToPaintGeometry(Size * Scale, FSlateLayoutTransform(Pos * Scale + Origin)); }
    void Box(float X,float Y,float W,float H,FLinearColor C) const
    { FSlateDrawElement::MakeBox(List, Layer, Geo({X,Y},{W,H}), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,C); }
    void Text(float X,float Y,const FString& S,int32 Size,FLinearColor C,bool bBold=false) const
    {
        FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular",FMath::Max(8,FMath::RoundToInt(Size*Scale*Style.TextScale)));
        FSlateDrawElement::MakeText(List, Layer+1, Geo({X,Y},{1,1}), S, Font, ESlateDrawEffect::None,C);
    }
    void Line(FVector2D A,FVector2D B,FLinearColor C,float Width=1) const
    {
        TArray<FVector2D> Points; Points.Add(A*Scale+Origin); Points.Add(B*Scale+Origin);
        FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,Width*Scale);
    }
    void Arc(FVector2D C,float R,float Fraction,FLinearColor Color,float Width=2,float Start=-PI*.5f) const
    {
        const int32 Count = FMath::Max(2,FMath::CeilToInt(48*Fraction));
        TArray<FVector2D> Points; Points.Reserve(Count+1);
        for(int32 I=0;I<=Count;++I)
        { const float T=Start+2*PI*Fraction*I/Count; Points.Add((C+FVector2D(FMath::Cos(T),FMath::Sin(T))*R)*Scale+Origin); }
        FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width*Scale);
    }
    void Meter(float X,float Y,float W,float Value,FLinearColor Color,float Height=4) const
    { Box(X,Y,W,Height,FLinearColor(.05f,.10f,.14f,.95f)); if(Value>0) Box(X,Y,W*Unit(Value),Height,Color); }
    void Brackets(FVector2D C,float Radius,FLinearColor Color) const
    {
        for(int32 X : {-1,1}) for(int32 Y : {-1,1})
        { FVector2D P=C+FVector2D(X,Y)*Radius; Line(P,P-FVector2D(X*14,0),Color,2); Line(P,P-FVector2D(0,Y*14),Color,2); }
    }
    void ShipIcon(float X,float Y,int32 Ship,FLinearColor C) const
    {
        // Port of the existing rift/lancer/bastion.svg paths; same silhouettes, no raster textures.
        auto Path=[&](std::initializer_list<FVector2D> Points)
        {
            TArray<FVector2D> DrawPoints; DrawPoints.Reserve(static_cast<int32>(Points.size()));
            for(const auto& Q:Points) DrawPoints.Add((FVector2D(X,Y)+(Q-FVector2D(32,32))*.85)*Scale+Origin);
            FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),DrawPoints,ESlateDrawEffect::None,C,true,1.7f*Scale);
        };
        if(Ship==0)
        {
            // Existing crescent uses cubic Beziers. Sample them, retaining the source control points.
            FVector2D Previous={32,7};
            TArray<FVector2D> Crescent; Crescent.Add((FVector2D(X,Y)+(Previous-FVector2D(32,32))*.85)*Scale+Origin);
            auto Curve=[&](FVector2D B,FVector2D D,FVector2D End)
            { FVector2D Begin=Previous; for(int32 I=1;I<=12;++I) { double T=I/12.,U=1-T; FVector2D Q=Begin*U*U*U+B*3*U*U*T+D*3*U*T*T+End*T*T*T; Crescent.Add((FVector2D(X,Y)+(Q-FVector2D(32,32))*.85)*Scale+Origin); Previous=Q; } };
            Curve({13,6},{4,20},{7,36}); Curve({10,51},{24,59},{38,56}); Curve({23,53},{15,42},{17,30}); Curve({19,18},{25,12},{32,7});
            FSlateDrawElement::MakeLines(List,Layer+1,G.ToPaintGeometry(),Crescent,ESlateDrawEffect::None,C,true,1.7f*Scale);
            Path({{37,17},{55,32},{37,47},{43,32},{37,17}});
        }
        else if(Ship==1) Path({{32,5},{40,34},{56,49},{36,42},{32,59},{28,42},{8,49},{24,34},{32,5}});
        else
        {
            Path({{16,15},{48,15},{58,38},{49,50},{15,50},{6,38},{16,15}});
            Path({{25,8},{39,8},{39,22},{25,22},{25,8}});
            Path({{18,28},{26,28},{26,40},{18,40},{18,28}});
            Path({{38,28},{46,28},{46,40},{38,40},{38,28}});
        }
    }
    void Card(float X,float Y,int32 Index,const FTDUIShip& S,const FTDUIShip& Smooth,float Pulse=0) const
    {
        const FLinearColor C=Style.ShipColor(Index), Dim=Style.Muted;
        Box(X,Y,356,196,Style.Panel);
        Box(X,Y,356,S.bActive?3:1,S.bActive?C:FLinearColor(.12f,.2f,.28f));
        if(S.bActive) { Box(X,Y,3,196,C); Box(X+3,Y+3,350,37,FLinearColor(C.R,C.G,C.B,.09f+.06f*Pulse)); }
        Text(X+18,Y+13,FString::Printf(TEXT("0%d  %s"),Index+1,Names[Index]),20,Style.Ink,true);
        Text(X+249,Y+17,S.bActive?TEXT("PILOTING"):S.bAlive?TEXT("WINGMAN"):TEXT("OFFLINE"),11,S.bActive?C:Dim,true);
        Text(X+18,Y+42,Pilots[Index],11,Dim);
        ShipIcon(X+313,Y+74,Index,C);
        if(!S.bAvailable) { Text(X+18,Y+85,TEXT("AWAITING TELEMETRY"),15,Dim); return; }
        if(!S.bAlive)
        {
            Text(X+18,Y+81,TEXT("SHIP DOWN"),20,Style.Hostile,true);
            Text(X+18,Y+115,FString::Printf(TEXT("RESPAWN  %.1f s"),S.RespawnSeconds),14,Style.Ink);
            return;
        }
        Text(X+18,Y+65,TEXT("HULL"),11,Dim); Text(X+220,Y+65,FString::Printf(TEXT("%03d"),FMath::RoundToInt(S.Hull*100)),13,Style.Ink,true);
        Meter(X+18,Y+86,238,Smooth.Hull,S.Hull<.25?Style.Hostile:C,5);
        Text(X+18,Y+96,TEXT("SHIELD"),11,Dim); Text(X+220,Y+96,FString::Printf(TEXT("%03d"),FMath::RoundToInt(S.Shield*100)),13,Style.Ink,true);
        Meter(X+18,Y+117,238,Smooth.Shield,Style.Lancer,3);
        Text(X+18,Y+130,Powers[Index],10,Dim);
        Text(X+249,Y+129,S.Power>=.999?TEXT("READY"):FString::Printf(TEXT("%d%%"),FMath::RoundToInt(S.Power*100)),12,C,true);
        Meter(X+18,Y+150,318,Smooth.Power,C,2);
        Text(X+18,Y+162, S.bUnlocked ? FString::Printf(TEXT("%s  %s"),Abilities[Index],S.Ability>=.999?TEXT("READY"):TEXT("CHARGING")) : FString::Printf(TEXT("%s  LOCKED"),Abilities[Index]),10, S.bUnlocked?C:Dim);
        Text(X+249,Y+162,FString::Printf(TEXT("AMMO %02d"),S.Ammo),11,Style.Ink);
        if(Index==2) { Arc({X+313,Y+74},32,1,Dim,1); Arc({X+313,Y+74},32,S.Power2,C,3); Text(X+277,Y+110,FString::Printf(TEXT("DOME %d%%"),FMath::RoundToInt(S.Power2*100)),9,Dim); }
    }
};
}

TSharedRef<SWidget> UTDShipCardWidget::RebuildWidget() { return SNew(SBox).WidthOverride(356).HeightOverride(196)[SNew(SSpacer)]; }
int32 UTDShipCardWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& R,FSlateWindowElementList& L,int32 Z,const FWidgetStyle& S,bool E) const
{
    const int32 Base=Super::NativePaint(A,G,R,L,Z,S,E);
    TDHUD::FPainter P(G,L,Base+1,Style,{356,196}); P.Card(0,0,FMath::Clamp(ShipIndex,0,2),State,State);
    return Base+3;
}
TSharedRef<SWidget> UTDSpaceHUDWidget::RebuildWidget() { return SNew(SBox)[SNew(SSpacer)]; }
void UTDSpaceHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if(Ships.Num()!=3) { Ships.SetNum(3); Display=Ships; }
    BindEvents(true);
    if(FSlateApplication::IsInitialized() && !InputObserver)
    { InputObserver=MakeShared<TDHUD::FInputObserver>(this); FSlateApplication::Get().RegisterInputPreProcessor(InputObserver); }
    if(UWorld* W=GetWorld()) W->GetTimerManager().SetTimer(SampleTimer,this,&UTDSpaceHUDWidget::RefreshData,FMath::Max(.05f,SampleSeconds),true);
}
void UTDSpaceHUDWidget::NativeDestruct()
{
    BindEvents(false);
    if(FSlateApplication::IsInitialized() && InputObserver) FSlateApplication::Get().UnregisterInputPreProcessor(InputObserver);
    InputObserver.Reset();
    if(UWorld* W=GetWorld()) W->GetTimerManager().ClearTimer(SampleTimer);
    Super::NativeDestruct();
}
void UTDSpaceHUDWidget::BindEvents(bool bBind)
{
    UObject* Bus=EventBus.Get();
    if(bBind)
    {
        UClass* Class=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDEventBus"));
        if(GetGameInstance() && Class) Bus=GetGameInstance()->GetSubsystemBase(Class);
        EventBus=Bus;
    }
    if(!Bus) return;
    const TPair<FName,FName> Bindings[]={{TEXT("OnHitConfirmed"),GET_FUNCTION_NAME_CHECKED(UTDSpaceHUDWidget,HandleHit)}, {TEXT("OnObjectiveUpdated"),GET_FUNCTION_NAME_CHECKED(UTDSpaceHUDWidget,HandleObjective)}};
    for(const auto& B:Bindings)
    {
        const FMulticastDelegateProperty* P=FindFProperty<FMulticastDelegateProperty>(Bus->GetClass(),B.Key);
        UFunction* Handler=FindFunction(B.Value);
        if(!P || !Handler || !P->SignatureFunction->IsSignatureCompatibleWith(Handler)) continue;
        FScriptDelegate D; D.BindUFunction(this,B.Value);
        P->RemoveDelegate(D,Bus);
        if(bBind) P->AddDelegate(D,Bus);
    }
    if(!bBind) EventBus.Reset();
}
void UTDSpaceHUDWidget::SetDataSource(UObject* Source)
{
    bPreview=false; DataSource.Reset(); Target.Reset(); Locked.Reset(); Objectives.Reset();
    ObjectivePrompt=FText::GetEmpty(); Warning=FText::GetEmpty(); WarningLeft=HitLeft=ReadyLeft=0;
    UClass* Contract=FindObject<UClass>(nullptr,TEXT("/Script/unrealGameJam.TDHUDData"));
    if(IsValid(Source) && Contract && Source->GetClass()->ImplementsInterface(Contract)) DataSource=Source;
    RefreshData();
}
void UTDSpaceHUDWidget::RefreshData()
{
    if(bPreview) return;
    TArray<FTDUIShip> Next; Next.SetNum(3);
    UObject* O=DataSource.Get();
    TDHUD::FResult R(O,TEXT("GetSquadHUD"));
    const FArrayProperty* Array=CastField<FArrayProperty>(R.Return);
    const FStructProperty* Item=Array?CastField<FStructProperty>(Array->Inner):nullptr;
    if(Item && R.Value())
    {
        FScriptArrayHelper Values(Array,R.Value());
        for(int32 I=0;I<Values.Num();++I)
        {
            const void* V=Values.GetRawPtr(I); int32 Index=INDEX_NONE;
            if(const FEnumProperty* Enum=FindFProperty<FEnumProperty>(Item->Struct,TEXT("Ship")))
                Index=Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(V));
            else Index=static_cast<int32>(TDHUD::Number(Item->Struct,V,TEXT("Ship"),-1));
            if(!Next.IsValidIndex(Index)) continue;
            FTDUIShip& S=Next[Index];
            S.bAvailable=true;
            S.Hull=TDHUD::Number(Item->Struct,V,TEXT("Health01")); S.Shield=TDHUD::Number(Item->Struct,V,TEXT("Shield01"));
            S.Boost=TDHUD::Number(Item->Struct,V,TEXT("Boost01")); S.Heat=TDHUD::Number(Item->Struct,V,TEXT("Heat01"));
            S.Power=TDHUD::Number(Item->Struct,V,TEXT("PowerCooldown01")); S.Power2=TDHUD::Number(Item->Struct,V,TEXT("Power2Cooldown01"));
            S.Ability=TDHUD::Number(Item->Struct,V,TEXT("AbilityCooldown01")); S.Ammo=TDHUD::Number(Item->Struct,V,TEXT("SecondaryAmmo"));
            S.bActive=TDHUD::Flag(Item->Struct,V,TEXT("bPlayerControlled")); S.bAlive=TDHUD::Flag(Item->Struct,V,TEXT("bAlive"));
            S.RespawnSeconds=TDHUD::Number(Item->Struct,V,TEXT("RespawnSeconds"));
            if(const FEnumProperty* Enum=FindFProperty<FEnumProperty>(Item->Struct,TEXT("Ability")))
                S.bUnlocked=Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(V))!=0;
        }
    }
    ApplySquadSnapshot(Next);
    Speed01=TDHUD::Unit(TDHUD::Scalar(O,TEXT("GetSpeed01"))); Hyperdrive01=TDHUD::Unit(TDHUD::Scalar(O,TEXT("GetHyperdriveCharge01")));
    TDHUD::FResult Bounds(O,TEXT("IsOutOfBounds"));
    const FBoolProperty* B=CastField<FBoolProperty>(Bounds.Return); bOutOfBounds=B&&Bounds.Value()&&B->GetPropertyValue(Bounds.Value());
    TDHUD::FResult WaveData(O,TEXT("GetWaveInfo"));
    const FStructProperty* W=CastField<FStructProperty>(WaveData.Return);
    Wave=W&&WaveData.Value()?TDHUD::Number(W->Struct,WaveData.Value(),TEXT("WaveNumber")):0;
    Enemies=W&&WaveData.Value()?TDHUD::Number(W->Struct,WaveData.Value(),TEXT("EnemiesAlive")):0;
    Target=TDHUD::Actor(O,TEXT("GetCurrentTarget")); TDHUD::ActorArray(O,TEXT("GetLockedTargets"),Locked); TDHUD::ActorArray(O,TEXT("GetObjectiveMarkers"),Objectives);
    UObject* Health=TDHUD::Component(Target.Get(),TEXT("/Script/unrealGameJam.TDHealthComponent"));
    bHasTargetHealth=Health!=nullptr;
    TargetHull=TDHUD::Unit(TDHUD::Scalar(Health,TEXT("GetHull01"))); TargetShield=TDHUD::Unit(TDHUD::Scalar(Health,TEXT("GetShield01")));
    AActor* Pawn=GetOwningPlayer()?GetOwningPlayer()->GetPawn():nullptr;
    UObject* Targeting=TDHUD::Component(Pawn,TEXT("/Script/unrealGameJam.TDTargetingComponent"));
    IncomingMissiles=TDHUD::Scalar(Targeting,TEXT("GetIncomingMissileCount"));
    bHasGameplayLead=TDHUD::GameplayLead(Targeting,LeadWorld);
    Hostiles.Reset();
    TDHUD::FResult Markers(Targeting,TEXT("K2_GetMarkers"));
    const FArrayProperty* MarkerArray=CastField<FArrayProperty>(Markers.Return);
    const FStructProperty* MarkerType=MarkerArray?CastField<FStructProperty>(MarkerArray->Inner):nullptr;
    if(MarkerType && Markers.Value())
    {
        const FWeakObjectProperty* ActorProperty=FindFProperty<FWeakObjectProperty>(MarkerType->Struct,TEXT("Actor"));
        FScriptArrayHelper Values(MarkerArray,Markers.Value());
        if(ActorProperty) for(int32 I=0;I<FMath::Min(Values.Num(),12);++I)
            if(AActor* Hostile=Cast<AActor>(ActorProperty->GetObjectPropertyValue(ActorProperty->ContainerPtrToValuePtr<void>(Values.GetRawPtr(I)))))
                if(Hostile!=Target.Get()) Hostiles.Add(Hostile);
    }
}
void UTDSpaceHUDWidget::ApplySquadSnapshot(const TArray<FTDUIShip>& Snapshot)
{
    TArray<FTDUIShip> Next=Snapshot; Next.SetNum(3);
    int32 NewActive=INDEX_NONE;
    for(int32 I=0;I<3;++I)
    {
        auto& S=Next[I];
        S.Hull=TDHUD::Unit(S.Hull); S.Shield=TDHUD::Unit(S.Shield); S.Boost=TDHUD::Unit(S.Boost); S.Heat=TDHUD::Unit(S.Heat);
        S.Power=TDHUD::Unit(S.Power); S.Power2=TDHUD::Unit(S.Power2); S.Ability=TDHUD::Unit(S.Ability);
        S.Ammo=FMath::Max(0,S.Ammo); S.RespawnSeconds=FMath::IsFinite(S.RespawnSeconds)?FMath::Max(0.f,S.RespawnSeconds):0;
        S.bActive=S.bActive&&S.bAlive&&S.bAvailable&&NewActive==INDEX_NONE;
        if(S.bActive) NewActive=I;
        if(Ships.IsValidIndex(I) && Ships[I].bAvailable && Ships[I].Power<.999f && S.Power>=.999f)
        { ReadyPrompt=FText::FromString(FString::Printf(TEXT("%s / %s READY"),TDHUD::Names[I],TDHUD::Powers[I])); ReadyLeft=2; ReadyShip=I; }
    }
    if(NewActive!=ActiveIndex && NewActive!=INDEX_NONE) SwitchLeft=Style.SwitchSeconds;
    ActiveIndex=NewActive; Ships=MoveTemp(Next);
    if(Display.Num()!=3) Display=Ships;
}
void UTDSpaceHUDWidget::ShowWarning(FText Message) { Warning=Message; WarningLeft=WarningSeconds; }
void UTDSpaceHUDWidget::ConfirmHit(bool bKill) { bKillConfirmed=bKill; HitLeft=bKill?.65f:.24f; }
void UTDSpaceHUDWidget::SetObjectivePrompt(FText Message) { ObjectivePrompt=Message; }
void UTDSpaceHUDWidget::HandleHit(AActor* Actor,bool bKill) { if(!bPreview) ConfirmHit(bKill); }
void UTDSpaceHUDWidget::HandleObjective(FName Id,int32 Step,FText Text) { if(!bPreview) SetObjectivePrompt(Text); }
void UTDSpaceHUDWidget::SetPreviewMode(bool bEnabled)
{
    bPreview=bEnabled;
    Warning=FText::GetEmpty(); ObjectivePrompt=FText::GetEmpty(); WarningLeft=HitLeft=ReadyLeft=0;
    if(!bEnabled) { RefreshData(); return; }
    DataSource.Reset(); Target.Reset(); Locked.Reset(); Objectives.Reset(); Hostiles.Reset(); bHasTargetHealth=bHasGameplayLead=false; IncomingMissiles=0;
    TArray<FTDUIShip> Demo; Demo.SetNum(3);
    for(int32 I=0;I<3;++I)
    { auto& S=Demo[I]; S.bAvailable=S.bAlive=true; S.bActive=I==1; S.Hull=1.f-I*.17f; S.Shield=.9f-I*.16f; S.Boost=.78f; S.Heat=.32f; S.Power=I==0?.62f:1.f; S.Power2=.72f; S.Ability=1; S.Ammo=I==2?24:8; S.bUnlocked=I!=0; }
    ApplySquadSnapshot(Demo); Display=Ships; Wave=3; Enemies=7; Speed01=.72f; Hyperdrive01=.64f;
    ObjectivePrompt=FText::FromString(TEXT("INTERCEPT THE NULL ARMADA"));
}
void UTDSpaceHUDWidget::PreviewSelectShip(int32 Index)
{
    if(!bPreview || !Ships.IsValidIndex(Index) || !Ships[Index].bAlive) return;
    auto Next=Ships; for(int32 I=0;I<3;++I) Next[I].bActive=I==Index;
    ApplySquadSnapshot(Next);
}
void UTDSpaceHUDWidget::PreviewDamage()
{
    if(!bPreview || !Ships.IsValidIndex(ActiveIndex)) return;
    auto Next=Ships; Next[ActiveIndex].Shield=.12f; Next[ActiveIndex].Hull=.22f;
    ApplySquadSnapshot(Next); ShowWarning(FText::FromString(TEXT("INCOMING MISSILE / EVADE")));
}
void UTDSpaceHUDWidget::PreviewDowned()
{
    if(!bPreview) return;
    auto Next=Ships; const int32 I=ActiveIndex==2?0:2;
    Next[I].bAlive=!Next[I].bAlive; Next[I].RespawnSeconds=8;
    ApplySquadSnapshot(Next);
}
void UTDSpaceHUDWidget::PreviewPower()
{
    if(!bPreview || !Ships.IsValidIndex(ActiveIndex)) return;
    auto Next=Ships; Next[ActiveIndex].Power=0; ApplySquadSnapshot(Next);
}
void UTDSpaceHUDWidget::NativeTick(const FGeometry& G,float Dt)
{
    Super::NativeTick(G,Dt); Dt=FMath::Clamp(Dt,0.f,.1f); Elapsed+=Dt;
    SwitchLeft=FMath::Max(0.f,SwitchLeft-Dt); HitLeft=FMath::Max(0.f,HitLeft-Dt); WarningLeft=FMath::Max(0.f,WarningLeft-Dt); ReadyLeft=FMath::Max(0.f,ReadyLeft-Dt);
    if(bPreview)
    {
        auto Next=Ships;
        for(auto& S:Next)
        {
            S.Power=FMath::Min(1.f,S.Power+Dt*.15f);
            if(!S.bAlive) { S.RespawnSeconds=FMath::Max(0.f,S.RespawnSeconds-Dt); if(S.RespawnSeconds<=0) { S.bAlive=true; S.Hull=1; S.Shield=1; } }
        }
        ApplySquadSnapshot(Next);
    }
    if(Display.Num()!=3) Display=Ships;
    const float Alpha=Style.bReducedMotion?1.f:1.f-FMath::Exp(-Style.MeterResponse*Dt);
    for(int32 I=0;I<Ships.Num();++I)
    { Display[I].Hull=FMath::Lerp(Display[I].Hull,Ships[I].Hull,Alpha); Display[I].Shield=FMath::Lerp(Display[I].Shield,Ships[I].Shield,Alpha); Display[I].Power=FMath::Lerp(Display[I].Power,Ships[I].Power,Alpha); }
}
bool UTDSpaceHUDWidget::SolveIntercept(const FVector& P,const FVector& V,float Speed,FVector& Out)
{
    if(Speed<=0 || P.ContainsNaN() || V.ContainsNaN() || !FMath::IsFinite(Speed)) return false;
    const double A=V.SizeSquared()-Speed*Speed, B=2*FVector::DotProduct(P,V), C=P.SizeSquared();
    double T=-1;
    if(FMath::Abs(A)<1.e-6) { if(FMath::Abs(B)>1.e-6) T=-C/B; }
    else
    {
        const double D=B*B-4*A*C; if(D<0) return false;
        const double T1=(-B-FMath::Sqrt(D))/(2*A), T2=(-B+FMath::Sqrt(D))/(2*A);
        T=T1>0 && T2>0?FMath::Min(T1,T2):FMath::Max(T1,T2);
    }
    if(T<=0 || !FMath::IsFinite(T)) return false;
    Out=P+V*T; return true;
}

int32 UTDSpaceHUDWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& R,FSlateWindowElementList& L,int32 Z,const FWidgetStyle& W,bool E) const
{
    const int32 Base=Super::NativePaint(A,G,R,L,Z,W,E);
    TDHUD::FPainter P(G,L,Base+1,Style);
    const FLinearColor Accent=Style.ShipColor(FMath::Max(0,ActiveIndex)), Dim=Style.Muted, Ink=Style.Ink;
    if(bPreview)
    {
        P.Box(0,0,1920,1080,FLinearColor(.004f,.008f,.019f));
        // Quiet original star-grid for UI contrast review; never installed in live gameplay.
        for(int32 I=0;I<110;++I) { float X=FMath::Fmod(I*173.7f+81,1920.f), Y=FMath::Fmod(I*119.3f+33,1080.f); P.Box(X,Y,I%5?1:2,I%5?1:2,FLinearColor(.13f,.21f,.3f,.65f)); }
        P.Arc({1450,260},390,.28f,FLinearColor(.04f,.13f,.19f),1.5f,PI*.6f);
        P.Text(66,33,TEXT("HUD PREVIEW / MOCK DATA"),13,Style.Bastion,true);
    }
    else if(!DataSource.IsValid()) P.Text(66,33,TEXT("TELEMETRY OFFLINE / CONNECT ITDHUDData"),13,Dim);
    P.Box(66,72,4,49,Accent); P.Text(86,68,TEXT("TRIAD / DRIFT"),23,Ink,true);
    P.Text(87,104,TEXT("SQUADRON 03     /     COMBAT LINK"),11,Dim);
    P.Text(1540,70,Wave>0?FString::Printf(TEXT("WAVE %02d"),Wave):TEXT("WAVE --"),24,Ink,true);
    P.Text(1540,108,bPreview||DataSource.IsValid()?FString::Printf(TEXT("%02d HOSTILES REMAINING"),Enemies):TEXT("HOSTILES --"),12,Dim);
    P.Text(727,66,TEXT("O B J E C T I V E"),11,Accent,true);
    P.Text(727,91,ObjectivePrompt.IsEmpty()?TEXT("AWAITING MISSION DATA"):ObjectivePrompt.ToString().Left(62),17,Ink,true);
    P.Line({727,128},{1193,128},FLinearColor(.15f,.26f,.34f));

    const FVector2D Center(960,540);
    P.Arc(Center,26,.20f,Accent,1.5,PI*.15f); P.Arc(Center,26,.20f,Accent,1.5,PI*1.15f);
    P.Line(Center+FVector2D(-10,0),Center+FVector2D(-4,0),Ink,1.5);
    P.Line(Center+FVector2D(4,0),Center+FVector2D(10,0),Ink,1.5);
    P.Line(Center+FVector2D(0,4),Center+FVector2D(0,10),Ink,1.5);
    P.Box(959,539,2,2,Ink);

    auto Project=[&](AActor* Actor,FVector2D& Out)->bool
    {
        APlayerController* PC=GetOwningPlayer(); if(!PC || !IsValid(Actor) || P.Scale<=0) return false;
        FVector2D UI;
        if(!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC,Actor->GetActorLocation(),UI,true)) return false;
        Out=(UI-P.Origin)/P.Scale; return true;
    };
    FVector2D TargetPos=FVector2D::ZeroVector;
    const bool bTarget=bPreview || Project(Target.Get(),TargetPos);
    if(bPreview) TargetPos={1156,397};
    if(bTarget && TargetPos.X>110 && TargetPos.X<1810 && TargetPos.Y>175 && TargetPos.Y<755)
    {
        P.Brackets(TargetPos,48,Style.Hostile);
        P.Text(TargetPos.X-48,TargetPos.Y-76,bPreview?TEXT("NULL / INTERCEPTOR"):TEXT("HOSTILE TARGET"),12,Style.Hostile,true);
        if(bPreview || bHasTargetHealth)
        { P.Meter(TargetPos.X-48,TargetPos.Y+62,96,bPreview?.68f:TargetHull,Style.Hostile,3); P.Meter(TargetPos.X-48,TargetPos.Y+68,96,bPreview?.25f:TargetShield,Style.Lancer,2); }
        if(Target.IsValid())
        {
            const double Distance=GetOwningPlayer()->GetPawn()?FVector::Dist(Target->GetActorLocation(),GetOwningPlayer()->GetPawn()->GetActorLocation())*.01:0;
            P.Text(TargetPos.X-48,TargetPos.Y+72,FString::Printf(TEXT("%.0f m"),Distance),12,Dim);
        }
        FVector2D Lead=FVector2D::ZeroVector;
        bool bLead=bPreview;
        if(bPreview) Lead={1097,432};
        else if(bHasGameplayLead && GetOwningPlayer())
        { FVector2D UI; bLead=UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),LeadWorld,UI,true); Lead=(UI-P.Origin)/P.Scale; }
        else if(Target.IsValid() && GetOwningPlayer() && ProjectileSpeedCm>0)
        {
            FVector Eye; FRotator Rot; GetOwningPlayer()->GetPlayerViewPoint(Eye,Rot); FVector Aim;
            const FVector Inherited=GetOwningPlayer()->GetPawn()?GetOwningPlayer()->GetPawn()->GetVelocity():FVector::ZeroVector;
            if(SolveIntercept(Target->GetActorLocation()-Eye,Target->GetVelocity()-Inherited,ProjectileSpeedCm,Aim))
            { FVector2D UI; bLead=UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(),Eye+Aim,UI,true); Lead=(UI-P.Origin)/P.Scale; }
        }
        if(bLead) { P.Arc(Lead,8,1,Accent,1.5f); P.Line(Lead+FVector2D(13,0),TargetPos,Dim,.6f); }
        if(bPreview || Locked.Contains(Target)) { P.Arc(TargetPos,58,.88f,Style.Hostile,1,0); P.Text(TargetPos.X-29,TargetPos.Y+91,TEXT("LOCKED"),11,Style.Hostile,true); }
    }
    else if(Target.IsValid())
    {
        FVector2D Direction=FVector2D::ZeroVector;
        if(bTarget) Direction=TargetPos-Center;
        else if(GetOwningPlayer())
        { FVector Eye; FRotator Rot; GetOwningPlayer()->GetPlayerViewPoint(Eye,Rot); FVector Local=Rot.UnrotateVector(Target->GetActorLocation()-Eye); Direction={Local.Y,-Local.Z}; }
        if(Direction.IsNearlyZero()) Direction={0,1}; Direction.Normalize();
        const FVector2D Edge=Center+Direction*FMath::Min(800.f/FMath::Max(.01,FMath::Abs(Direction.X)),230.f/FMath::Max(.01,FMath::Abs(Direction.Y)));
        const FVector2D Side(-Direction.Y,Direction.X);
        P.Line(Edge,Edge-Direction*17+Side*9,Style.Hostile,2); P.Line(Edge,Edge-Direction*17-Side*9,Style.Hostile,2);
        P.Text(Edge.X-20,Edge.Y+15,TEXT("TARGET"),10,Style.Hostile);
    }
    for(const auto& Hostile:Hostiles)
    {
        FVector2D Pos=FVector2D::ZeroVector;
        if(Project(Hostile.Get(),Pos) && Pos.X>110 && Pos.X<1810 && Pos.Y>175 && Pos.Y<755) P.Brackets(Pos,18,Style.Hostile.CopyWithNewOpacity(.45f));
        else if(Hostile.IsValid() && GetOwningPlayer())
        {
            FVector Eye; FRotator Rot; GetOwningPlayer()->GetPlayerViewPoint(Eye,Rot);
            FVector Local=Rot.UnrotateVector(Hostile->GetActorLocation()-Eye); FVector2D D(Local.Y,-Local.Z); if(D.IsNearlyZero()) D={0,1}; D.Normalize();
            FVector2D PosEdge=Center+D*FMath::Min(800.f/FMath::Max(.01,FMath::Abs(D.X)),230.f/FMath::Max(.01,FMath::Abs(D.Y)));
            FVector2D Side(-D.Y,D.X); P.Line(PosEdge,PosEdge-D*12+Side*6,Style.Hostile,1.5f); P.Line(PosEdge,PosEdge-D*12-Side*6,Style.Hostile,1.5f);
        }
    }
    for(int32 I=0;I<FMath::Min(4,Objectives.Num());++I)
    {
        FVector2D Pos; AActor* Marker=Objectives[I].Get(); if(!Project(Marker,Pos)) continue;
        Pos.X=FMath::Clamp(Pos.X,115.,1805.); Pos.Y=FMath::Clamp(Pos.Y,180.,730.);
        P.Line(Pos+FVector2D(0,-11),Pos+FVector2D(11,0),Style.Bastion,2); P.Line(Pos+FVector2D(11,0),Pos+FVector2D(0,11),Style.Bastion,2);
        P.Line(Pos+FVector2D(0,11),Pos+FVector2D(-11,0),Style.Bastion,2); P.Line(Pos+FVector2D(-11,0),Pos+FVector2D(0,-11),Style.Bastion,2);
        if(GetOwningPlayer() && GetOwningPlayer()->GetPawn()) P.Text(Pos.X+19,Pos.Y-8,FString::Printf(TEXT("OBJECTIVE  %.0f m"),FVector::Dist(Marker->GetActorLocation(),GetOwningPlayer()->GetPawn()->GetActorLocation())*.01),12,Style.Bastion);
    }
    if(bPreview) { P.Text(1444,278,TEXT("<>  VERDANCE / 12.4 km"),13,Style.Bastion); P.Text(1735,548,TEXT("< HOSTILE"),12,Style.Hostile); }

    const FTDUIShip* Active=Ships.IsValidIndex(ActiveIndex)?&Ships[ActiveIndex]:nullptr;
    if(Active)
    {
        P.Text(87,743,TEXT("FLIGHT TELEMETRY"),11,Dim,true);
        P.Text(87,770,FString::Printf(TEXT("%03d"),FMath::RoundToInt(Speed01*100)),42,Ink,true); P.Text(194,798,TEXT("% SPEED"),11,Dim);
        P.Text(87,839,TEXT("BOOST"),11,Dim); P.Meter(87,863,227,Active->Boost,Accent,5);
        P.Text(87,893,TEXT("HYPERDRIVE"),11,Dim); P.Meter(87,918,227,Hyperdrive01,Dim,3);
        P.Text(1580,785,TEXT("PRIMARY / HEAT"),11,Dim); P.Text(1580,811,FString::Printf(TEXT("%02d %%"),FMath::RoundToInt(Active->Heat*100)),30,Active->Heat>.85f?Style.Hostile:Ink,true);
        P.Meter(1580,862,254,Active->Heat,Active->Heat>.85f?Style.Hostile:Accent,5);
        P.Text(1580,893,FString::Printf(TEXT("SECONDARY   %02d"),Active->Ammo),18,Ink,true);
        if(Active->Hull<=LowHullThreshold) { P.Text(772,714,TEXT("LOW HULL / SWITCH SHIP"),17,Style.Hostile,true); }
    }
    const float Pulse=Style.bReducedMotion?0:(FMath::Sin(Elapsed*5)+1)*.5f;
    for(int32 I=0;I<3;++I) if(Ships.IsValidIndex(I) && Display.IsValidIndex(I)) P.Card(398+I*374,804,I,Ships[I],Display[I],Pulse);
    if(HitLeft>0)
    {
        const FLinearColor C=bKillConfirmed?Style.Bastion:Ink;
        for(int32 X:{-1,1}) for(int32 Y:{-1,1}) P.Line(Center+FVector2D(X*14,Y*14),Center+FVector2D(X*21,Y*21),C,2);
        if(bKillConfirmed) P.Text(882,591,TEXT("TARGET ELIMINATED"),13,C,true);
    }
    if(bOutOfBounds || IncomingMissiles>0 || WarningLeft>0)
    {
        P.Box(704,640,512,42,Style.Panel); P.Box(704,640,3,42,Style.Hostile);
        const FString Message=bOutOfBounds?TEXT("BOUNDARY / RETURN TO COMBAT"):IncomingMissiles>0?FString::Printf(TEXT("INCOMING MISSILE x%d / EVADE"),IncomingMissiles):Warning.ToString().Left(46);
        P.Text(729,651,Message,16,Style.Hostile,true);
    }
    else if(ReadyLeft>0) P.Text(729,651,ReadyPrompt.ToString(),15,Style.ShipColor(ReadyShip),true);
    if(SwitchLeft>0 && Active)
    {
        const float Offset=Style.bReducedMotion?0:18*FMath::Square(SwitchLeft/FMath::Max(.05f,Style.SwitchSeconds));
        P.Box(66+Offset,169,4,42,Accent); P.Text(83+Offset,169,FString::Printf(TEXT("CUT TO / %s"),TDHUD::Names[ActiveIndex]),18,Ink,true);
        P.Text(83+Offset,196,TDHUD::Pilots[ActiveIndex],11,Accent);
    }
    const FString Hint=bPreview?(bGamepadGlyphs?TEXT("PREVIEW / [LB / RB] SWITCH   [X] DAMAGE   [A] POWER   [Y] KILL"):TEXT("PREVIEW / [1 / 2 / 3] SWITCH SHIP")):(bGamepadGlyphs?GamepadHint.ToString():KeyboardHint.ToString());
    P.Text(66,1031,Hint,12,Dim);
    if(bPreview) P.Text(995,1031,TEXT("[H] DAMAGE   [K] KILL   [P] POWER   [R] RESPAWN   [F5] RESET   [F6] MOTION"),11,Dim);
    return Base+4;
}
