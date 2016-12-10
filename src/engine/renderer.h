#pragma once
#include <lsk/lsk_math.h>
#include <lsk/lsk_array.h>
#include <lsk/lsk_thread.h>
#include <external/gl3w.h>
#include <external/gl3w.h>

struct Shader_Color
{
	GLuint _program = 0;
	GLint _uViewMatrix = -1;
	GLint _uMatID = -1;
	GLint _uInstanceOffset = -1;
	GLuint _uMaterialData = 0;

	struct Material {
		lsk_Vec4 color;
	};

	bool loadAndinit();
	void use();
	void setView(const lsk_Mat4& viewMatrix);
	void setMaterialIDBufferTextureSlot(u32 slot);
	void setInstanceOffset(i32 offset);
};

struct Shader_Textured
{
	GLuint _program = 0;
	GLint _uViewMatrix = -1;
	GLint _uMatID = -1;
	GLint _uInstanceOffset = -1;
	GLint _uTextureArray[3];
	GLuint _uMaterialData = 0;

	struct Material {
		lsk_Vec4 color = {1, 1, 1, 1};
		lsk_Vec4 uvParams = {0, 0, 1, 1}; // (x,y) = offset (z,w) = scale
		f32 uvMax_x;
		f32 uvMax_y;
		u32 texArrayID;
		u32 texNameHash_layerID; // disk material holds textureNameHash, gpu holds texture array layer ID

		void setTexture(u32 textureNameHash);
	};

	bool loadAndInit();
	void use();
	void setView(const lsk_Mat4& viewMatrix);
	void setMaterialIDBufferTextureSlot(u32 slot);
	void setInstanceOffset(i32 offset);
	void setTextureArraySlots(i32* slots_, u32 count);
};

enum class MaterialType: i32 {
	INVALID = -1,
	COLOR,
	TEXTURED,
};

struct MaterialManager
{
	lsk_AllocatorHeapCascade _allocMatData;
	lsk_DStrHashMap<lsk_Block> _dataMap;
	lsk_DStrHashMap<MaterialType> _typeMap;

	void init();
	void destroy();

	template<typename MatT>
	MatT& set(MaterialType type, u32 materialNameHash, const MatT& material) {
		lsk_Block* pob = _dataMap.geth(materialNameHash);
		if(pob) {
			_allocMatData.deallocate(*pob);
		}
		lsk_Block block = _allocMatData.allocate(sizeof(MatT), alignof(MatT));
		assert_msg(block.ptr, "Out of memory");
		new(block.ptr) MatT(material);
		_dataMap.seth(materialNameHash, block);
		_typeMap.seth(materialNameHash, type);
		return *(MatT*)block.ptr;
	}

	inline const MaterialType& getType(u32 materialNameHash) {
		MaterialType* type = _typeMap.geth(materialNameHash);
		assert(type);
		return *type;
	}

	inline const void* getAnyData(u32 materialNameHash) {
		lsk_Block* b = _dataMap.geth(materialNameHash);
		assert(b);
		return b->ptr;
	}

	inline bool exists(u32 materialNameHash) {
		return (_typeMap.geth(materialNameHash) != nullptr);
	}

	Shader_Color::Material& getColor(u32 materialNameHash);
	Shader_Textured::Material& getTextured(u32 materialNameHash);
};

struct DrawCommand
{
	MaterialType _materialType = MaterialType::INVALID;
	const void* _pMaterialData = nullptr;
	u32 vao = 0;
	i32 z = 0;
	lsk_Mat4 modelMatrix;

	void setMaterial(u32 nameHash);
};

struct RendererSingle
{
	SINGLETON_IMP(RendererSingle)

	// TODO: this should use a stackallocator
	// default size should be determined at runtime and saved
	// since it varies a lot from game to game
	lsk_IAllocator* _pAlloc = &AllocDefault;

	lsk_Mat4 _orthoMatrix;
	lsk_Mat4 _viewPosMatrix;

	GLuint _quadVao;
	GLuint _quadVertexBuff;
	GLuint _quadIndexBuff;

	GLuint _flat_materialBuff;
	GLuint _textured_materialBuff;
	u64 _flat_materialBuffSize = 0;
	u64 _textured_materialBuffSize = 0;

	GLuint _materialIDBuffText;
	u32 _materialIDBuffTextSlot = 0;
	GLuint _materialIDBuff;
	u64 _materialIDBuffSize = 0;

	MaterialManager materials;

	Shader_Color _materialType_color;
	Shader_Textured _materialType_textured;

	lsk_DArray<DrawCommand> _drawCmdList = lsk_DArray<DrawCommand>(2048);
	lsk_DArray<u32> _drawCmdGroupCount = lsk_DArray<u32>(1024);

	GLuint _gpuModelMatrixBuff;
	i32 _gpuModelMatrixBuffSize = -1;

	// TODO: use two lists: one in-between begin/endFrame and one final to be rendered
	// instead of locking one list
	lsk_Mutex _listMutex;

	enum Layout: u32 {
		POSITION = 0,
		TEXTURE_COORDINATES = 1,
		MODEL = 2, // model is mat4 and thus takes 4 slots
		NEXT = 6
	};

	bool init();
	void destroy();

	void viewResize(i32 width, i32 height, f32 zoom = 1.f);
	void viewSetPos(f32 x, f32 y);

	void beginFrame();
	void endFrame();
	void queue(const DrawCommand& cmd);
	void queueSprite(u32 materialNameHash, i32 z, const lsk_Vec2& pos,
					 const lsk_Vec2& size, const lsk_Quat& rot = lsk_Quat());
	void render();
};

#define Renderer RendererSingle::get()
