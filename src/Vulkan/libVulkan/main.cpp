// Copyright 2017 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// main.cpp: DLL entry point and management of thread-local data.

#include "main.h"
#include "libVulkan.h"
#include "../../Common/Thread.hpp"
#include "../../Common/SharedLibrary.hpp"
#include "../../OpenGL/common/debug.h"

#if !defined(_MSC_VER)
#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR __attribute__((destructor))
#else
#define CONSTRUCTOR
#define DESTRUCTOR
#endif

void PRINT_API Print(void)
{
	return vulkan::test_print();
}

static void vkAttachThread()
{
	TRACE("()");
}

static void vkDetachThread()
{
	TRACE("()");
}

CONSTRUCTOR static void vkAttachProcess()
{
	TRACE("()");

	vkAttachThread();
}

DESTRUCTOR static void vkDetachProcess()
{
	TRACE("()");

	vkDetachThread();
}

#if defined(_WIN32)
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		vkAttachProcess();
		break;
	case DLL_THREAD_ATTACH:
		vkAttachThread();
		break;
	case DLL_THREAD_DETACH:
		vkDetachThread();
		break;
	case DLL_PROCESS_DETACH:
		vkDetachProcess();
		break;
	default:
		break;
	}

	return TRUE;
}
#endif