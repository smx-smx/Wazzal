// Minimal D3DX 9 compatibility layer.
//
// The original project was built against the DirectX 8.1 SDK, which is no
// longer distributed. This header/source re-implements only the small subset
// of D3DX9 that Wazzal actually uses, on top of the plain Direct3D 9 API.

#ifndef WAZZAL_COMPAT_D3DX9_H
#define WAZZAL_COMPAT_D3DX9_H

#include <d3d9.h>
#include <windows.h>
#include <math.h>
#include <string.h>

#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT ((UINT)0xFFFFFFFF)
#endif
#ifndef D3DX_DEFAULT_NONPOW2
#define D3DX_DEFAULT_NONPOW2 ((UINT)0xFFFFFFFE)
#endif
#ifndef D3DX_FILTER_NONE
#define D3DX_FILTER_NONE      0x00000001
#define D3DX_FILTER_POINT     0x00000000
#define D3DX_FILTER_LINEAR    0x00000002
#define D3DX_FILTER_TRIANGLE  0x00000004
#define D3DX_FILTER_BOX       0x00000008
#define D3DX_FILTER_MIRROR    0x00000010
#endif
#ifndef D3DX_SDK_VERSION
#define D3DX_SDK_VERSION 8
#endif

typedef struct D3DXIMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    DWORD ImageFileFormat;
} D3DXIMAGE_INFO;

#ifdef __cplusplus

struct D3DXMATRIX : public D3DMATRIX {
    D3DXMATRIX() {}
    D3DXMATRIX(const D3DMATRIX& m) { memcpy(this, &m, sizeof(D3DMATRIX)); }
    D3DXMATRIX(FLOAT m00, FLOAT m01, FLOAT m02, FLOAT m03,
               FLOAT m10, FLOAT m11, FLOAT m12, FLOAT m13,
               FLOAT m20, FLOAT m21, FLOAT m22, FLOAT m23,
               FLOAT m30, FLOAT m31, FLOAT m32, FLOAT m33) {
        _11 = m00; _12 = m01; _13 = m02; _14 = m03;
        _21 = m10; _22 = m11; _23 = m12; _24 = m13;
        _31 = m20; _32 = m21; _33 = m22; _34 = m23;
        _41 = m30; _42 = m31; _43 = m32; _44 = m33;
    }
    D3DXMATRIX& operator=(const D3DMATRIX& m) {
        memcpy(this, &m, sizeof(D3DMATRIX));
        return *this;
    }
    FLOAT& operator()(UINT row, UINT col) { return m[row][col]; }
    FLOAT operator()(UINT row, UINT col) const { return m[row][col]; }
};

struct D3DXPLANE {
    FLOAT a, b, c, d;
    D3DXPLANE() {}
    D3DXPLANE(FLOAT _a, FLOAT _b, FLOAT _c, FLOAT _d) : a(_a), b(_b), c(_c), d(_d) {}
};

struct D3DXVECTOR3 : public D3DVECTOR {
    D3DXVECTOR3() { x = y = z = 0.0f; }
    D3DXVECTOR3(FLOAT _x, FLOAT _y, FLOAT _z) { x = _x; y = _y; z = _z; }
    D3DXVECTOR3(const FLOAT* pf) { x = pf[0]; y = pf[1]; z = pf[2]; }
};

struct D3DXVECTOR4 {
    FLOAT x, y, z, w;
    D3DXVECTOR4() { x = y = z = w = 0.0f; }
    D3DXVECTOR4(FLOAT _x, FLOAT _y, FLOAT _z, FLOAT _w) { x = _x; y = _y; z = _z; w = _w; }
    D3DXVECTOR4(const FLOAT* pf) { x = pf[0]; y = pf[1]; z = pf[2]; w = pf[3]; }
};

struct ID3DXMatrixStack {
    virtual ~ID3DXMatrixStack() {}
    virtual ULONG Release() = 0;
    virtual HRESULT Push() = 0;
    virtual HRESULT Pop() = 0;
    virtual HRESULT LoadIdentity() = 0;
    virtual HRESULT LoadMatrix(const D3DXMATRIX* pMat) = 0;
    virtual HRESULT MultMatrix(const D3DXMATRIX* pMat) = 0;
    virtual HRESULT TranslateLocal(FLOAT x, FLOAT y, FLOAT z) = 0;
    virtual HRESULT ScaleLocal(FLOAT x, FLOAT y, FLOAT z) = 0;
    virtual HRESULT RotateYawPitchRollLocal(FLOAT yaw, FLOAT pitch, FLOAT roll) = 0;
    virtual D3DXMATRIX* GetTop() = 0;
};

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXMatrixLookAtLH(D3DXMATRIX* pOut, const D3DXVECTOR3* pEye,
                                  const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp);
HRESULT WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX* pOut, FLOAT fovy, FLOAT aspect,
                                          FLOAT zn, FLOAT zf);
HRESULT WINAPI D3DXMatrixShadow(D3DXMATRIX* pOut, const D3DXVECTOR4* pLight,
                                const D3DXPLANE* pPlane);
HRESULT WINAPI D3DXPlaneFromPointNormal(D3DXPLANE* pOut, const D3DXVECTOR3* pPoint,
                                        const D3DXVECTOR3* pNormal);

HRESULT WINAPI D3DXCreateMatrixStack(DWORD flags, ID3DXMatrixStack** ppStack);

HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT width, UINT height,
                                 UINT mipLevels, DWORD usage, D3DFORMAT format,
                                 D3DPOOL pool, LPDIRECT3DTEXTURE9* ppTexture);
HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE9 pDevice, LPCSTR pSrcFile,
                                            UINT width, UINT height, UINT mipLevels,
                                            DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                            DWORD filter, DWORD mipFilter, D3DCOLOR colorKey,
                                            D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
                                            LPDIRECT3DTEXTURE9* ppTexture);
HRESULT WINAPI D3DXLoadSurfaceFromFileA(LPDIRECT3DSURFACE9 pDestSurface,
                                        const PALETTEENTRY* pDestPalette,
                                        const RECT* pDestRect, LPCSTR pSrcFile,
                                        const RECT* pSrcRect, DWORD filter,
                                        D3DCOLOR colorKey, D3DXIMAGE_INFO* pSrcInfo);

#ifdef __cplusplus
}
#endif

#endif // WAZZAL_COMPAT_D3DX9_H
