// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Core/GridLevelAsset.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GRIMROCKPROTOTYPE_GridLevelAsset_generated_h
#error "GridLevelAsset.generated.h already included, missing '#pragma once' in GridLevelAsset.h"
#endif
#define GRIMROCKPROTOTYPE_GridLevelAsset_generated_h

#define FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGridLevelAsset(); \
	friend struct Z_Construct_UClass_UGridLevelAsset_Statics; \
public: \
	DECLARE_CLASS(UGridLevelAsset, UDataAsset, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GrimrockPrototype"), NO_API) \
	DECLARE_SERIALIZER(UGridLevelAsset)


#define FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UGridLevelAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGridLevelAsset(UGridLevelAsset&&); \
	UGridLevelAsset(const UGridLevelAsset&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGridLevelAsset); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGridLevelAsset); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UGridLevelAsset) \
	NO_API virtual ~UGridLevelAsset();


#define FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_12_PROLOG
#define FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_15_INCLASS_NO_PURE_DECLS \
	FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GRIMROCKPROTOTYPE_API UClass* StaticClass<class UGridLevelAsset>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Development_GrimrockPrototype_Source_GrimrockPrototype_Public_Core_GridLevelAsset_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
