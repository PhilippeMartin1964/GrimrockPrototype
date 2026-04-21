// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "GrimrockPrototype/Public/Core/GridLevelAsset.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGridLevelAsset() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
GRIMROCKPROTOTYPE_API UClass* Z_Construct_UClass_UGridLevelAsset();
GRIMROCKPROTOTYPE_API UClass* Z_Construct_UClass_UGridLevelAsset_NoRegister();
UPackage* Z_Construct_UPackage__Script_GrimrockPrototype();
// End Cross Module References

// Begin Class UGridLevelAsset
void UGridLevelAsset::StaticRegisterNativesUGridLevelAsset()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGridLevelAsset);
UClass* Z_Construct_UClass_UGridLevelAsset_NoRegister()
{
	return UGridLevelAsset::StaticClass();
}
struct Z_Construct_UClass_UGridLevelAsset_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Core/GridLevelAsset.h" },
		{ "ModuleRelativePath", "Public/Core/GridLevelAsset.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGridLevelAsset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGridLevelAsset_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_GrimrockPrototype,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGridLevelAsset_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGridLevelAsset_Statics::ClassParams = {
	&UGridLevelAsset::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGridLevelAsset_Statics::Class_MetaDataParams), Z_Construct_UClass_UGridLevelAsset_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGridLevelAsset()
{
	if (!Z_Registration_Info_UClass_UGridLevelAsset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGridLevelAsset.OuterSingleton, Z_Construct_UClass_UGridLevelAsset_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGridLevelAsset.OuterSingleton;
}
template<> GRIMROCKPROTOTYPE_API UClass* StaticClass<UGridLevelAsset>()
{
	return UGridLevelAsset::StaticClass();
}
UGridLevelAsset::UGridLevelAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGridLevelAsset);
UGridLevelAsset::~UGridLevelAsset() {}
// End Class UGridLevelAsset

// Begin Registration
struct Z_CompiledInDeferFile_FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGridLevelAsset, UGridLevelAsset::StaticClass, TEXT("UGridLevelAsset"), &Z_Registration_Info_UClass_UGridLevelAsset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGridLevelAsset), 1354509177U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_811281699(TEXT("/Script/GrimrockPrototype"),
	Z_CompiledInDeferFile_FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
