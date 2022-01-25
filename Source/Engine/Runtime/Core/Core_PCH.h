// This is a precompiled header.  Include a bunch of common stuff.
// This is kind of ugly in that it adds a bunch of dependency where it isn't needed.
// But on balance, the compile time is much lower (even incrementally) once the precompiled
// headers contain these headers.

#pragma once

#include <GlobalEngineConfig.h>

#include <Base/Base.h>
#include <Base/Object/Reflection/TypeDescriptor.h>
#include <Base/Object/Reflection/Reflection.h>
