// Triad Drift - late-bound calls into other areas' classes (sandbox glue). Owner: Teammate 5.

#include "TriadDrift/Flow/TDFlowReflection.h"
#include "TriadDrift/Core/TDTypes.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"

namespace TDFlowReflection
{
	namespace
	{
		/** Owns a zeroed, initialized parameter block for one UFunction call. */
		struct FParamBlock
		{
			UFunction* Func = nullptr;
			uint8* Memory = nullptr;

			explicit FParamBlock(UFunction* InFunc) : Func(InFunc)
			{
				Memory = static_cast<uint8*>(FMemory::Malloc(FMath::Max<int32>(1, Func->ParmsSize), 16));
				FMemory::Memzero(Memory, FMath::Max<int32>(1, Func->ParmsSize));
				Func->InitializeStruct(Memory);
			}
			~FParamBlock()
			{
				Func->DestroyStruct(Memory);
				FMemory::Free(Memory);
			}

			FProperty* FirstInputParam() const
			{
				for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
				{
					if (!It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm) || It->HasAnyPropertyFlags(CPF_ReferenceParm))
					{
						return *It;
					}
				}
				return nullptr;
			}

			FProperty* ReturnParam() const
			{
				return Func->GetReturnProperty();
			}
		};

		UFunction* FindFunc(const UObject* Target, FName Function)
		{
			if (!IsValid(Target) || Function.IsNone())
			{
				return nullptr;
			}
			UFunction* Func = Target->FindFunction(Function);
			if (!Func)
			{
				UE_LOG(LogTriadDrift, Verbose, TEXT("TDFlowReflection: %s has no UFUNCTION %s."), *Target->GetClass()->GetName(), *Function.ToString());
			}
			return Func;
		}
	}

	UClass* ResolveClass(const FSoftClassPath& Path)
	{
		if (Path.IsNull())
		{
			return nullptr;
		}
		if (UClass* Loaded = Path.ResolveClass())
		{
			return Loaded;
		}
		// Native classes resolve without loading; this only loads if it's a Blueprint class.
		return Path.TryLoadClass<UObject>();
	}

	bool HasFunction(const UObject* Target, FName Function)
	{
		return IsValid(Target) && Target->FindFunction(Function) != nullptr;
	}

	bool Call(UObject* Target, FName Function)
	{
		UFunction* Func = FindFunc(Target, Function);
		if (!Func)
		{
			return false;
		}
		FParamBlock Params(Func);
		Target->ProcessEvent(Func, Params.Memory);
		return true;
	}

	bool CallInt(UObject* Target, FName Function, int32 Value)
	{
		UFunction* Func = FindFunc(Target, Function);
		if (!Func)
		{
			return false;
		}
		FParamBlock Params(Func);
		FIntProperty* Prop = CastField<FIntProperty>(Params.FirstInputParam());
		if (!Prop)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TDFlowReflection: %s's first parameter is not int32."), *Function.ToString());
			return false;
		}
		Prop->SetPropertyValue_InContainer(Params.Memory, Value);
		Target->ProcessEvent(Func, Params.Memory);
		return true;
	}

	bool CallTransform(UObject* Target, FName Function, const FTransform& Value)
	{
		UFunction* Func = FindFunc(Target, Function);
		if (!Func)
		{
			return false;
		}
		FParamBlock Params(Func);
		FStructProperty* Prop = CastField<FStructProperty>(Params.FirstInputParam());
		if (!Prop || Prop->Struct != TBaseStructure<FTransform>::Get())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TDFlowReflection: %s's first parameter is not FTransform."), *Function.ToString());
			return false;
		}
		*Prop->ContainerPtrToValuePtr<FTransform>(Params.Memory) = Value;
		Target->ProcessEvent(Func, Params.Memory);
		return true;
	}

	UObject* CallReturnObject(UObject* Target, FName Function)
	{
		UFunction* Func = FindFunc(Target, Function);
		if (!Func)
		{
			return nullptr;
		}
		FParamBlock Params(Func);
		FObjectPropertyBase* Ret = CastField<FObjectPropertyBase>(Params.ReturnParam());
		if (!Ret)
		{
			return nullptr;
		}
		Target->ProcessEvent(Func, Params.Memory);
		return Ret->GetObjectPropertyValue_InContainer(Params.Memory);
	}
}
