#pragma once

#include "Misc/AssertionMacros.h"

#define GAR_STRINGIFY_IMPLEMENTATION(Value) #Value

#define GAR_STRINGIFY(Value) GAR_STRINGIFY_IMPLEMENTATION(Value)

#define GAR_GET_TYPE_STRING(Type) \
	((void) sizeof UEAsserts_Private::GetMemberNameCheckedJunk(static_cast<Type*>(nullptr)), TEXTVIEW(#Type))
