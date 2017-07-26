// Copyright 2017 The NXT Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BACKEND_COMMANDS_H_
#define BACKEND_COMMANDS_H_

#include "Context.h"
#include "RefCounted.h"

using namespace vulkan;

namespace backend {
	// Definition of the commands that are present in the CommandIterator given by the
	// CommandBufferBuilder. There are not defined in CommandBuffer.h to break some header
	// dependencies: Ref<Object> needs Object to be defined.

	enum class Command {
		BeginRenderPass,
		DrawArrays,
		EndRenderPass,
		SetPipeline,
		SetVertex,
		CopyImageBuffer,
	};

	struct BeginRenderPassCmd {
		Ref<RenderPass> renderPass;
		Ref<Framebuffer> framebuffer;
	};

	struct DrawArraysCmd {
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;
	};

	struct EndRenderPassCmd {
	};

	struct SetRenderPipelineCmd {
		Ref<Pipeline> pipeline;
	};

	struct SetVertexBufferCmd {
		Ref<Buffer> buffer;
		uint32_t offset;
	};

	struct CopyImageToBufferCmd {
		Ref<Image> srcImage;
		Ref<Buffer> dstBuffer;
	};

	// This needs to be called before the CommandIterator is freed so that the Ref<> present in
	// the commands have a chance to run their destructor and remove internal references.
	void FreeCommands(CommandIterator* commands);
	void SkipCommand(CommandIterator* commands, Command type);
}

#endif // BACKEND_COMMANDS_H_
