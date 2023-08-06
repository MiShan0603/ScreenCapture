//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
// Port of _gl.h to _d3d11.h by Chris Maughan
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef NANOVG_D3D11_H
#define NANOVG_D3D11_H

// Hide nameless struct/union warning for D3D headers
#pragma warning (disable : 4201)
#include <d3d11.h>
#include <D3DX11tex.h>
#pragma warning (default : 4201)

#ifdef __cplusplus
extern "C" {
#endif

// Flag indicating if geoemtry based anti-aliasing is used (may not be needed when using MSAA).
#define NVG_ANTIALIAS 1

// Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
// slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
#define NVG_STENCIL_STROKES 2

struct NVGcontext* nvgCreateD3D11(ID3D11Device* pDevice, int edgeaa);
void nvgDeleteD3D11(struct NVGcontext* ctx);

// These are additional flags on top of NVGimageFlags.
enum NVGimageFlagsD3D11 {
	NVG_IMAGE_NODELETE			= 1<<16,	// Do not delete texture object.
};

// Not done yet.  Simple enough to do though...
// #ifdef IMPLEMENTED_IMAGE_FUNCS
/// create image from dx share handle 
int nvd3dCreateImageFromHandle(struct NVGcontext* ctx, HANDLE handle, int flags);
int nvd3dCreateImageFromHandleEx(struct NVGcontext* ctx, ID3D11DeviceContext *context, HANDLE handle, int flags);

/// create image from d3d texture
int nvd3dCreateImageFromSurface(struct NVGcontext* ctx, ID3D11Texture2D* texture, int w, int h, int flags);
void* nvd3dImageHandle(struct NVGcontext* ctx, int image);
void nvd3dImageFlags(struct NVGcontext* ctx, int image, int flags);
// #endif

#ifdef __cplusplus
}
#endif

// #ifdef NANOVG_D3D11_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "nanovg.h"
#include <d3d11.h>

// #include "D3D11VertexShader.h"
// #include "D3D11PixelShaderAA.h"
// #include "D3D11PixelShader.h"

// The cpp calling is much simpler.
// For 'c' calling of DX, we need to do pPtr->lpVtbl->Func(pPtr, ...)
// There's probably a better way... (but note we can't use the IInterace_Method() helpers because
// They won't work when compiled for cpp)
#ifdef __cplusplus
#define D3D_API(p, name, arg1) p->name()
#define D3D_API_1(p, name, arg1) p->name(arg1)
#define D3D_API_2(p, name, arg1, arg2) p->name(arg1, arg2)
#define D3D_API_3(p, name, arg1, arg2, arg3) p->name(arg1, arg2, arg3)
#define D3D_API_4(p, name, arg1, arg2, arg3, arg4) p->name(arg1, arg2, arg3, arg4)
#define D3D_API_5(p, name, arg1, arg2, arg3, arg4, arg5) p->name(arg1, arg2, arg3, arg4, arg5)
#define D3D_API_6(p, name, arg1, arg2, arg3, arg4, arg5, arg6) p->name(arg1, arg2, arg3, arg4, arg5, arg6)
#define D3D_API_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = NULL; } }
#else
#define D3D_API(p, name) p->lpVtbl->name(p)
#define D3D_API_1(p, name, arg1) p->lpVtbl->name(p, arg1)
#define D3D_API_2(p, name, arg1, arg2) p->lpVtbl->name(p, arg1, arg2)
#define D3D_API_3(p, name, arg1, arg2, arg3) p->lpVtbl->name(p, arg1, arg2, arg3)
#define D3D_API_4(p, name, arg1, arg2, arg3, arg4) p->lpVtbl->name(p, arg1, arg2, arg3, arg4)
#define D3D_API_5(p, name, arg1, arg2, arg3, arg4, arg5) p->lpVtbl->name(p, arg1, arg2, arg3, arg4, arg5)
#define D3D_API_6(p, name, arg1, arg2, arg3, arg4, arg5, arg6) p->lpVtbl->name(p, arg1, arg2, arg3, arg4, arg5, arg6)
#define D3D_API_RELEASE(p) { if ( (p) ) { (p)->lpVtbl->Release((p)); (p) = NULL; } }
#endif

#pragma pack(push)
#pragma pack(16)
struct D3DNVGfragUniforms
{
    float scissorMat[16];
    float scissorExt[4];
    float scissorScale[4];
    float paintMat[16];
    float extent[4];
    float radius[4];
    float feather[4];
    struct NVGcolor innerCol;
    struct NVGcolor outerCol;
    float strokeMult[4];
    int texType;
    int type;
};
#pragma pack(pop)

struct VS_CONSTANTS
{
    float dummy[16];
    float viewSize[2];
};

enum D3DNVGshaderType {
    NSVG_SHADER_FILLGRAD,
    NSVG_SHADER_FILLIMG,
    NSVG_SHADER_SIMPLE,
    NSVG_SHADER_IMG
};

struct D3DNVGshader {
    ID3D11PixelShader* frag;
    ID3D11VertexShader* vert;
    struct VS_CONSTANTS vc;
};

struct D3DNVGtexture {
    int id;
    ID3D11Texture2D* tex;
    ID3D11ShaderResourceView* resourceView0;
	ID3D11ShaderResourceView* resourceView1;
    int width, height;
    int type;
    int flags;
	HANDLE handle; // share hand
};

enum D3DNVGcallType {
    D3DNVG_NONE = 0,
    D3DNVG_FILL,
    D3DNVG_CONVEXFILL,
    D3DNVG_STROKE,
    D3DNVG_TRIANGLES
};

struct D3DNVGcall {
	int type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
};

struct D3DNVGpath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
};

struct D3DNVGBuffer {
    unsigned int MaxBufferEntries;
    unsigned int CurrentBufferEntry;
    ID3D11Buffer* pBuffer;
};

struct D3DNVGcontext {
    
    struct D3DNVGshader shader;
    struct D3DNVGtexture* textures;
    float view[2];    
    int ntextures;
    int ctextures;
    int textureId;
    ID3D11SamplerState* pSamplerState[4];

    int fragSize;
    int flags;

    // Per frame buffers
	struct D3DNVGcall* calls;
	int ccalls;
	int ncalls;
	struct D3DNVGpath* paths;
	int cpaths;
	int npaths;
	struct NVGvertex* verts;
	int cverts;
	int nverts;
	unsigned char* uniforms;
	int cuniforms;
	int nuniforms;

    // D3D
    // Geometry
    struct D3DNVGBuffer VertexBuffer;
    ID3D11Buffer* pFanIndexBuffer;
    ID3D11InputLayout* pLayoutRenderTriangles;

    // State
    ID3D11Buffer* pVSConstants;
    ID3D11Buffer* pPSConstants;

    ID3D11Device* pDevice;
    ID3D11DeviceContext* pDeviceContext;

    ID3D11BlendState* pBSBlend;
    ID3D11BlendState* pBSNoWrite;
    
    ID3D11RasterizerState* pRSNoCull;
    ID3D11RasterizerState* pRSCull;

    ID3D11DepthStencilState* pDepthStencilDrawShapes;
    ID3D11DepthStencilState* pDepthStencilDrawAA;
    ID3D11DepthStencilState* pDepthStencilFill;
    ID3D11DepthStencilState* pDepthStencilDefault;
};

static int D3Dnvg__maxi(int a, int b);

static struct D3DNVGtexture* D3Dnvg__allocTexture(struct D3DNVGcontext* D3D);

static struct D3DNVGtexture* D3Dnvg__findTexture(struct D3DNVGcontext* D3D, int id);

static struct D3DNVGtexture* D3Dnvg__findTexture(struct D3DNVGcontext* D3D, HANDLE handle);

static int D3Dnvg__deleteTexture(struct D3DNVGcontext* D3D, int id);

// BEGIN D3D specific

static void D3Dnvg_copyMatrix3to4(float* pDest, const float* pSource);
// END D3D specific

static int D3Dnvg__checkError(HRESULT hr, const char* str);

static int D3Dnvg__createShader(struct D3DNVGcontext* D3D, struct D3DNVGshader* shader, const void* vshader, unsigned int vshader_size, const void* fshader, unsigned int fshader_size);

static void D3Dnvg__deleteShader(struct D3DNVGshader* shader);

void D3Dnvg_buildFanIndices(struct D3DNVGcontext* D3D);

struct NVGvertex* D3Dnvg_beginVertexBuffer(struct D3DNVGcontext* D3D, unsigned int maxCount, unsigned int* baseOffset);

void D3Dnvg_endVertexBuffer(struct D3DNVGcontext* D3D);

static void D3Dnvg__copyVerts(struct NVGvertex* pDest, const struct NVGvertex* pSource, unsigned int num);

static unsigned int D3Dnvg_updateVertexBuffer(ID3D11DeviceContext* pContext, struct D3DNVGBuffer* buffer, const struct NVGvertex* verts, unsigned int nverts);

static void D3Dnvg_setBuffers(struct D3DNVGcontext* D3D, unsigned int dynamicOffset);

static int D3Dnvg__renderCreate(void* uptr);

static int D3Dnvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);

static int D3Dnvg__renderDeleteTexture(void* uptr, int image);

static int D3Dnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);

static int D3Dnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h);

static void D3Dnvg__xformToMat3x3(float* m3, float* t);

static struct NVGcolor D3Dnvg__premulColor(struct NVGcolor c);

static int D3Dnvg__convertPaint(struct D3DNVGcontext* D3D, struct D3DNVGfragUniforms* frag,
	struct NVGpaint* paint, struct NVGscissor* scissor,
	float width, float fringe, float strokeThr);

static struct D3DNVGfragUniforms* nvg__fragUniformPtr(struct D3DNVGcontext* D3D, int i);

static void D3Dnvg__setUniforms(struct D3DNVGcontext* D3D, int uniformOffset, int image);

static void D3Dnvg__renderViewport(void* uptr, int width, int height);

static void D3Dnvg__fill(struct D3DNVGcontext* D3D, struct D3DNVGcall* call);

static void D3Dnvg__convexFill(struct D3DNVGcontext* D3D, struct D3DNVGcall* call);

static void D3Dnvg__stroke(struct D3DNVGcontext* D3D, struct D3DNVGcall* call);

static void D3Dnvg__triangles(struct D3DNVGcontext* D3D, struct D3DNVGcall* call);

static void D3Dnvg__renderCancel(void* uptr);

static void D3Dnvg__renderFlush(void* uptr);

static int D3Dnvg__maxVertCount(const struct NVGpath* paths, int npaths);

static struct D3DNVGcall* D3Dnvg__allocCall(struct D3DNVGcontext* D3D);

static int D3Dnvg__allocPaths(struct D3DNVGcontext* D3D, int n);

static int D3Dnvg__allocVerts(struct D3DNVGcontext* D3D, int n);

static int D3Dnvg__allocFragUniforms(struct D3DNVGcontext* D3D, int n);

static struct D3DNVGfragUniforms* nvg__fragUniformPtr(struct D3DNVGcontext* D3D, int i);

static void D3Dnvg__vset(struct NVGvertex* vtx, float x, float y, float u, float v);

static void D3Dnvg__renderFill(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
	const float* bounds, const struct NVGpath* paths, int npaths);

static void D3Dnvg__renderStroke(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor, float fringe,
	float strokeWidth, const struct NVGpath* paths, int npaths);

static void D3Dnvg__renderTriangles(void* uptr, struct NVGpaint* paint, struct NVGscissor* scissor,
    const struct NVGvertex* verts, int nverts);

static void D3Dnvg__renderDelete(void* uptr);

struct NVGcontext* nvgCreateD3D11(ID3D11Device* pDevice, int flags);

void nvgDeleteD3D11(struct NVGcontext* ctx);

// #ifdef IMPLEMENTED_IMAGE_FUNCS


// #endif

// #endif //NANOVG_D3D11_IMPLEMENTATION
#endif //NANOVG_D3D11_H