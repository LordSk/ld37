#include "renderer.h"
#include <lsk/lsk_gl.h>
#include <lsk/lsk_file.h>
#include <lsk/lsk_utils.h>
#include <algorithm>
#include "texture.h"

#define MAKE_STR(something) #something

// TODO: interpolation

bool Shader_Color::loadAndinit()
{
	constexpr const char* colorVert = MAKE_STR(
		#version 330 core\n
		layout(location = 0) in vec2 position;\n
		layout(location = 2) in mat4 model;\n
		uniform mat4 uViewMatrix;\n
		uniform int uInstanceOffset;\n
		uniform samplerBuffer uMatID;\n

		flat out int vert_matID;\n

		void main()\n
		{\n
		   vert_matID = int(texelFetch(uMatID, gl_InstanceID + uInstanceOffset).r * 65535); // un-normalize to get the proper ID\n
		   gl_Position = uViewMatrix * model * vec4(position, 0.0, 1.0);\n
		}
	);

	i32 colorVertLen = lsk_strLen(colorVert);

	GLuint vertShader = lsk_glMakeShader(GL_VERTEX_SHADER, colorVert, colorVertLen);
	if(!vertShader) return false;

	constexpr const char* colorFrag = MAKE_STR(
		#version 330 core\n
		struct Material {\n
			vec4 color;\n
		};\n

		layout(std140) uniform uMaterialData\n
		{
			Material uMaterial[512];\n
		};\n

		flat in int vert_matID;\n
		out vec4 fragmentColor;\n

		void main()\n
		{\n
			fragmentColor = uMaterial[vert_matID].color;\n
		}
	);

	i32 colorFragLen = lsk_strLen(colorFrag);

	GLuint fragShader = lsk_glMakeShader(GL_FRAGMENT_SHADER, colorFrag, colorFragLen);
	if(!fragShader) return false;

	GLuint shaders[] = {vertShader, fragShader};
	_program = lsk_glMakeProgram(shaders, 2);
	if(!_program) return false;

	_uViewMatrix = glGetUniformLocation(_program, "uViewMatrix");
	_uMatID = glGetUniformLocation(_program, "uMatID");
	_uInstanceOffset = glGetUniformLocation(_program, "uInstanceOffset");

	if(_uViewMatrix == -1 || _uMatID == -1 || _uInstanceOffset == -1) {
		lsk_errf("[MaterialType_Flat] Error: failed to locate all uniforms");
		return false;
	}

	_uMaterialData = glGetUniformBlockIndex(_program, "uMaterialData");

	if(_uMaterialData == GL_INVALID_INDEX) {
		lsk_errf("[MaterialType_Flat] Error: uMaterialData not found");
		return false;
	}

	return true;
}

void Shader_Color::use()
{
	assert(_program != -1);
	glUseProgram(_program);
}

void Shader_Color::setView(const lsk_Mat4& viewMatrix)
{
	glUniformMatrix4fv(_uViewMatrix, 1, GL_FALSE, viewMatrix.data);
}

void Shader_Color::setMaterialIDBufferTextureSlot(u32 slot)
{
	glUniform1i(_uMatID, slot);
}

void Shader_Color::setInstanceOffset(i32 offset)
{
	glUniform1i(_uInstanceOffset, offset);
}

void Shader_Textured::Material::setTexture(u32 textureNameHash)
{
	texNameHash_layerID = textureNameHash;
}

bool Shader_Textured::loadAndInit()
{
	constexpr const char* texturedVert = MAKE_STR(
		#version 330 core\n
		layout(location = 0) in vec2 position;\n
		layout(location = 1) in vec2 uv;\n
		layout(location = 2) in mat4 model;\n
		uniform mat4 uViewMatrix;\n
		uniform int uInstanceOffset;\n
		uniform samplerBuffer uMatID;\n

		out vec2 vert_uv;\n
		flat out int vert_matID;\n

		void main()\n
		{\n
			vert_uv = uv;\n
			vert_matID = int(texelFetch(uMatID, gl_InstanceID + uInstanceOffset).r * 65535); // un-normalize to get the proper ID\n
			gl_Position = uViewMatrix * model * vec4(position, 0.0, 1.0);\n
		}
	);

	i32 texturedVertLen = lsk_strLen(texturedVert);

	GLuint vertShader = lsk_glMakeShader(GL_VERTEX_SHADER, texturedVert, texturedVertLen);
	if(!vertShader) return false;

	constexpr const char* texturedFrag = MAKE_STR(
		#version 330 core\n
		struct Material {\n
			vec4 color;\n
			vec2 uvOffset;\n
			vec2 uvScale;\n
			vec2 uvMax;\n
			uint texID;\n
			uint layer;\n
		};\n

		layout(std140) uniform uMaterialData\n
		{\n
			Material uMaterial[512];\n
		};\n

		uniform sampler2DArray uTextureArray[3];\n

		in vec2 vert_uv;\n
		flat in int vert_matID;\n
		out vec4 fragmentColor;\n

		void main()\n
		{\n
			vec3 uv = vec3((uMaterial[vert_matID].uvOffset + vert_uv) * uMaterial[vert_matID].uvScale, float(uMaterial[vert_matID].layer));\n
			// enable smooth repeat pattern for texture smaller than array texture size\n
			uv.x = mod(uv.x, uMaterial[vert_matID].uvMax.x);\n
			uv.y = mod(uv.y, uMaterial[vert_matID].uvMax.y);\n

			vec4 diffColor = texture(uTextureArray[uMaterial[vert_matID].texID], uv);\n
			fragmentColor = diffColor * uMaterial[vert_matID].color;\n
		}\n
	);

	i32 texturedFragLen = lsk_strLen(texturedFrag);

	GLuint fragShader = lsk_glMakeShader(GL_FRAGMENT_SHADER, texturedFrag, texturedFragLen);
	if(!fragShader) return false;

	GLuint shaders[] = {vertShader, fragShader};
	_program = lsk_glMakeProgram(shaders, 2);
	if(!_program) return false;

	_uViewMatrix = glGetUniformLocation(_program, "uViewMatrix");
	_uMatID = glGetUniformLocation(_program, "uMatID");
	_uInstanceOffset = glGetUniformLocation(_program, "uInstanceOffset");
	_uTextureArray[0] = glGetUniformLocation(_program, "uTextureArray[0]");
	_uTextureArray[1] = glGetUniformLocation(_program, "uTextureArray[1]");
	_uTextureArray[2] = glGetUniformLocation(_program, "uTextureArray[2]");

	if(_uViewMatrix == -1 || _uMatID == -1 || _uInstanceOffset == -1 ||
	   _uTextureArray[0] == -1 || _uTextureArray[1] == -1 || _uTextureArray[2] == -1) {
		lsk_errf("[MaterialType_FlatTextured] Error: failed to locate all uniforms");
		return false;
	}

	_uMaterialData = glGetUniformBlockIndex(_program, "uMaterialData");

	if(_uMaterialData == GL_INVALID_INDEX) {
		lsk_errf("[MaterialType_FlatTextured] Error: uMaterialData not found");
		return false;
	}

	/*i32 size;
	glGetActiveUniformBlockiv(_program, _uMaterialData, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
	lsk_printf("uniform block size=%d", size);
	lsk_printf("material size=%d", sizeof Material);

	constexpr u32 uniformCount = 5;
	u32 uniformIndices[uniformCount];
	lsk_arrayFillIncr(uniformIndices, uniformCount, 0, 1);
	i32 offsets[uniformCount];
	glGetActiveUniformsiv(_program, uniformCount, uniformIndices, GL_UNIFORM_OFFSET, offsets);

	for(u32 i = 0; i < uniformCount; ++i) {
		char name[64];
		i32 length, size;
		u32 type;
		glGetActiveUniform(_program, i, 64, &length, &size, &type, name);
		lsk_printf("%s: %d [%d, %d, %d]", name, offsets[i], length, size, type);
	}*/

	return true;
}

void Shader_Textured::use()
{
	assert(_program != -1);
	glUseProgram(_program);
}

void Shader_Textured::setView(const lsk_Mat4& viewMatrix)
{
	glUniformMatrix4fv(_uViewMatrix, 1, GL_FALSE, viewMatrix.data);
}

void Shader_Textured::setMaterialIDBufferTextureSlot(u32 slot)
{
	glUniform1i(_uMatID, slot);
}

void Shader_Textured::setInstanceOffset(i32 offset)
{
	glUniform1i(_uInstanceOffset, offset);
}

void Shader_Textured::setTextureArraySlots(i32* slots_, u32 count)
{
	for(u32 i = 0; i < count; ++i) {
		glUniform1i(_uTextureArray[i], slots_[i]);
	}
}

void MaterialManager::init()
{
	_allocMatData.init(&AllocDefault, Megabyte(5));
	_dataMap.init(256, &_allocMatData);
	_typeMap.init(256);
}

void MaterialManager::destroy()
{
	_dataMap.destroy();
	_typeMap.destroy();
	_allocMatData.release();
}

Shader_Color::Material& MaterialManager::getColor(u32 materialNameHash)
{
	lsk_Block* pb = _dataMap.geth(materialNameHash);
	assert(pb);
	return *(Shader_Color::Material*)pb->ptr;
}

Shader_Textured::Material& MaterialManager::getTextured(u32 materialNameHash)
{
	lsk_Block* pb = _dataMap.geth(materialNameHash);
	assert(pb);
	return *(Shader_Textured::Material*)pb->ptr;
}

void DrawCommand::setMaterial(u32 nameHash)
{
	assert(Renderer.materials.exists(nameHash));
	_materialType = Renderer.materials.getType(nameHash);
	// TODO: This can be easily invalidated if the material hash map grows
	// after this draw command is created
	_pMaterialData = Renderer.materials.getAnyData(nameHash);
}

bool RendererSingle::init()
{
	f32 vertices[] = {
		0.f, 0.f,
		1.f, 0.f,
		1.f, 1.f,
		0.f, 1.f,
	};

	i32 indices[] = {
		0, 2, 1,
		0, 3, 2
	};

	glGenVertexArrays(1, &_quadVao);
	glBindVertexArray(_quadVao);

	glGenBuffers(1, &_quadVertexBuff);
	glGenBuffers(1, &_quadIndexBuff);

	glBindBuffer(GL_ARRAY_BUFFER, _quadVertexBuff);
	glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 8, vertices, GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(
		Layout::POSITION,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(GLfloat)*2,
		(void*)0
	);
	glEnableVertexAttribArray(Layout::POSITION);

	// texture coordinates
	glVertexAttribPointer(
		Layout::TEXTURE_COORDINATES,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(GLfloat)*2,
		(void*)0
	);
	glEnableVertexAttribArray(Layout::TEXTURE_COORDINATES);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadIndexBuff);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(i32) * 6, indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	if(!_materialType_color.loadAndinit()) {
		return false;
	}

	if(!_materialType_textured.loadAndInit()) {
		return false;
	}

	glGenBuffers(1, &_gpuModelMatrixBuff);
	_gpuModelMatrixBuffSize = -1;

	// material uniform block
	glGenBuffers(1, &_flat_materialBuff);
	glGenBuffers(1, &_textured_materialBuff);

	u32 blockPointIndex = 0;
	_flat_materialBuffSize = Megabyte(5);
	glBindBuffer(GL_UNIFORM_BUFFER, _flat_materialBuff);
	glBufferData(GL_UNIFORM_BUFFER, _flat_materialBuffSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, blockPointIndex, _flat_materialBuff);
	glUniformBlockBinding(_materialType_color._program, _materialType_color._uMaterialData,
						  blockPointIndex);

	blockPointIndex = 1;
	_textured_materialBuffSize = Megabyte(5);
	glBindBuffer(GL_UNIFORM_BUFFER, _textured_materialBuff);
	glBufferData(GL_UNIFORM_BUFFER, _textured_materialBuffSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, blockPointIndex, _textured_materialBuff);
	glUniformBlockBinding(_materialType_textured._program, _materialType_textured._uMaterialData,
						  blockPointIndex);

	// material ID buffer
	glGenTextures(1, &_materialIDBuffText);
	glGenBuffers(1, &_materialIDBuff);

	_materialIDBuffSize = sizeof(u16) * 256;
	glBindBuffer(GL_TEXTURE_BUFFER, _materialIDBuff);
	glBufferData(GL_TEXTURE_BUFFER, _materialIDBuffSize, nullptr, GL_DYNAMIC_DRAW);

	_materialIDBuffTextSlot = Textures._gpuNextActiveTextureSlot++;
	glActiveTexture(GL_TEXTURE0 + _materialIDBuffTextSlot);
	glBindTexture(GL_TEXTURE_BUFFER, _materialIDBuffText);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R16, _materialIDBuff);

	materials.init();

	_viewPosMatrix = lsk_Mat4Identity();

	return true;
}

void RendererSingle::destroy()
{
	materials.destroy();

	glDeleteBuffers(1, &_quadVertexBuff);
	glDeleteBuffers(1, &_quadIndexBuff);
	glDeleteVertexArrays(1, &_quadVao);

	_drawCmdGroupCount.destroy();
	_drawCmdList.destroy();
}

void RendererSingle::viewResize(i32 width, i32 height, f32 zoom)
{
	_orthoMatrix = lsk_Mat4Orthographic(0, width * zoom, height * zoom, 0, -1, 1);
}

void RendererSingle::viewSetPos(f32 x, f32 y)
{
	_viewPosMatrix = lsk_Mat4Translate({-x, -y, 0});
}

void RendererSingle::beginFrame()
{
	_drawCmdGroupCount.clear();
	_drawCmdList.clear();
	_listMutex.unlock(); // everything is rendered, unlock
}

void RendererSingle::endFrame()
{
	_listMutex.lock(); // lock list until we rendered it

	const u32 drawCmdCount = _drawCmdList.count();
	if(drawCmdCount == 0) return; // nothing to do here

	// sort by vao, material
	auto compare = [](const void* pa, const void* pb) -> i32 {
		DrawCommand& a = *(DrawCommand*)pa;
		DrawCommand& b = *(DrawCommand*)pb;

		if(a.z < b.z) {
			return -1;
		}
		if(a.z > b.z) {
			return 1;
		}
		if(a.vao < b.vao) {
			return -1;
		}
		if(a.vao > b.vao) {
			return 1;
		}
		if(a._materialType < b._materialType) {
			return -1;
		}
		if(a._materialType > b._materialType) {
			return 1;
		}
		if(a._pMaterialData < b._pMaterialData) {
			return -1;
		}
		if(a._pMaterialData > b._pMaterialData) {
			return 1;
		}
		return 0;
	};

	qsort(_drawCmdList.data(), drawCmdCount, sizeof(DrawCommand), compare);

	// TODO: makes groups, draw instanced, make a Modelmatrix buffer
	u32 curVao = _drawCmdList[0].vao;
	MaterialType pCurMatType = _drawCmdList[0]._materialType;
	u32* pCurCount = &_drawCmdGroupCount.push(0);

	for(const auto& cmd: _drawCmdList) {
		if(curVao != cmd.vao || pCurMatType != cmd._materialType) {
			pCurCount = &_drawCmdGroupCount.push(1);
			curVao = cmd.vao;
			pCurMatType = cmd._materialType;
		}
		else {
			++(*pCurCount);
		}
	}

	i32 modelMatrixBuffSize = sizeof(lsk_Mat4) * drawCmdCount;
	lsk_Block modelMatrixBlock = _pAlloc->allocate(modelMatrixBuffSize, alignof(lsk_Mat4));
	lsk_Mat4* modelMatrices = (lsk_Mat4*)modelMatrixBlock.ptr;

	// TODO: move _drawCmdList[i].modelMatrix to a separate array? how is sort handled then?
	for(u32 i = 0; i < drawCmdCount; ++i) {
		modelMatrices[i] = _drawCmdList[i].modelMatrix;
	}

	// upload model matrices to gpu
	glBindBuffer(GL_ARRAY_BUFFER, _gpuModelMatrixBuff);
	if(_gpuModelMatrixBuffSize < modelMatrixBuffSize) {
		glBufferData(GL_ARRAY_BUFFER, modelMatrixBuffSize, modelMatrixBlock.ptr, GL_DYNAMIC_DRAW);
		_gpuModelMatrixBuffSize = modelMatrixBuffSize;
	}
	else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, modelMatrixBuffSize, modelMatrixBlock.ptr);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	_pAlloc->deallocate(modelMatrixBlock);

	lsk_DArray<Shader_Color::Material> flatData(256);
	lsk_DArray<Shader_Textured::Material> texturedData(256);
	lsk_DArray<u16> matIDs(_drawCmdList.count());

	const void* curMatDataPtr = nullptr;
	i32 curMatID = 0;
	for(const auto& cmd: _drawCmdList) {
		if(cmd._materialType == MaterialType::COLOR &&
		   curMatDataPtr != cmd._pMaterialData) {
			flatData.push(*(Shader_Color::Material*)cmd._pMaterialData);
			curMatID = flatData.count() - 1;
			curMatDataPtr = cmd._pMaterialData;
		}

		if(cmd._materialType == MaterialType::TEXTURED &&
		   curMatDataPtr != cmd._pMaterialData) {
			texturedData.push(*(Shader_Textured::Material*)cmd._pMaterialData);
			curMatID = texturedData.count() - 1;
			curMatDataPtr = cmd._pMaterialData;
		}

		matIDs.push(curMatID);
	}

	// load required textures
	lsk_DArray<u32> texHashToLoad(texturedData.count());

	for(auto& td: texturedData) {
		texHashToLoad.push(td.texNameHash_layerID);
	}

	Textures.loadToGpu(texHashToLoad.data(), texHashToLoad.count());
	texHashToLoad.destroy();

	// get texture info
	for(auto& td: texturedData) {
		const auto& gpuTex = Textures.getGpuTex(td.texNameHash_layerID);
		td.texArrayID = gpuTex.texArrayID;
		td.texNameHash_layerID = gpuTex.layerID;
		td.uvMax_x = gpuTex.nx;
		td.uvMax_y = gpuTex.ny;
		td.uvParams.z *= td.uvMax_x;
		td.uvParams.w *= td.uvMax_y;
	}


	/*lsk_printf("Renderer:endFrame(): flatData.count()=%d matIDs.count()=%d",
			   flatData.count(), matIDs.count());*/

	glBindBuffer(GL_UNIFORM_BUFFER, _flat_materialBuff);
	glBufferSubData(GL_UNIFORM_BUFFER, 0,
					sizeof(Shader_Color::Material) * flatData.count(),
					flatData.data());

	glBindBuffer(GL_UNIFORM_BUFFER, _textured_materialBuff);
	glBufferSubData(GL_UNIFORM_BUFFER, 0,
					sizeof(Shader_Textured::Material) * texturedData.count(),
					texturedData.data());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	u64 matIDBuffSize = matIDs.count() * sizeof(u16);
	glBindBuffer(GL_TEXTURE_BUFFER, _materialIDBuff);
	if(_materialIDBuffSize < matIDBuffSize) {
		glBufferData(GL_TEXTURE_BUFFER, matIDBuffSize, matIDs.data(), GL_DYNAMIC_DRAW);
		_materialIDBuffSize = matIDBuffSize;
	}
	else {
		glBufferSubData(GL_TEXTURE_BUFFER, 0, matIDBuffSize, matIDs.data());
	}
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void RendererSingle::queue(const DrawCommand& cmd)
{
	assert(cmd.vao > 0 && cmd._materialType != MaterialType::INVALID && cmd._pMaterialData);
	_listMutex.lock();
	_drawCmdList.push(cmd);
	_listMutex.unlock();
}

void RendererSingle::queueSprite(u32 materialNameHash, i32 z, const lsk_Vec2& pos,
								 const lsk_Vec2& size, const lsk_Quat& rot)
{
	DrawCommand cmd;
	lsk_Mat4 model = lsk_Mat4Translate({pos, 0});
	if(!lsk_QuatIsNull(rot)) {
		model = model * lsk_QuatMatrix(rot);
	}
	model = model * lsk_Mat4Scale({size, 1});
	cmd.vao = Renderer._quadVao;
	cmd.z = z;
	cmd.modelMatrix = model;
	cmd.setMaterial(materialNameHash);
	queue(cmd);
}

void RendererSingle::render()
{
	if(_drawCmdList.count() == 0) return; // nothing to do here

	lsk_Mat4 viewMat = _orthoMatrix * _viewPosMatrix;

	const DrawCommand* pCmd;
	u32 curVao = 0;
	MaterialType curMatType = MaterialType::INVALID;
	u32 modelStartId = 0;

	for(u32 groupCount: _drawCmdGroupCount) {
		pCmd = &_drawCmdList[modelStartId];

		if(curMatType != pCmd->_materialType) {
			curMatType = pCmd->_materialType;
			if(curMatType == MaterialType::COLOR) {
				_materialType_color.use();
				_materialType_color.setView(viewMat);
				_materialType_color.setMaterialIDBufferTextureSlot(_materialIDBuffTextSlot);
				_materialType_color.setInstanceOffset(modelStartId);
			}
			else if(curMatType == MaterialType::TEXTURED) {
				_materialType_textured.use();
				_materialType_textured.setView(viewMat);
				_materialType_textured.setMaterialIDBufferTextureSlot(_materialIDBuffTextSlot);
				_materialType_textured.setInstanceOffset(modelStartId);
				i32 slots_[] = {
					Textures._textureArray[0].slot,
					Textures._textureArray[1].slot,
					Textures._textureArray[2].slot
				};
				_materialType_textured.setTextureArraySlots(slots_, 3);
			}
		}

		if(curVao != pCmd->vao) {
			curVao = pCmd->vao;
			glBindVertexArray(pCmd->vao);
		}

		glBindBuffer(GL_ARRAY_BUFFER, _gpuModelMatrixBuff);

		u32 stride = sizeof(lsk_Mat4);
		u64 offset = sizeof(f32) * 4;
		u64 baseOffset = modelStartId * sizeof(lsk_Mat4);

		for(u32 i = 0; i < 4; ++i) {
			glVertexAttribPointer(
				Layout::MODEL + i,
				4,
				GL_FLOAT,
				GL_FALSE,
				stride,
				(void*)(baseOffset + offset * i));
			glEnableVertexAttribArray(Layout::MODEL + i);
			glVertexAttribDivisor(Layout::MODEL + i, 1);
		}

		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL, groupCount);

		modelStartId += groupCount;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}
