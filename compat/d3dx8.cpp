// Minimal D3DX 8 compatibility layer (see d3dx8.h).
//
// Implements the matrix helpers, the matrix stack and the texture/surface
// loading entry points used by Wazzal, using the Direct3D 8 API plus GDI for
// BMP decoding.

#include "d3dx8.h"

#include <math.h>
#include <string.h>
#include <vector>

static const float D3DX_PI = 3.141592654f;

static void MatrixIdentity(D3DXMATRIX* pOut) {
    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 1.0f;
}

static void VectorNormalize(D3DXVECTOR3* v) {
    float len = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    if (len > 0.0f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

static D3DXVECTOR3 VectorCross(const D3DXVECTOR3* a, const D3DXVECTOR3* b) {
    return D3DXVECTOR3(a->y * b->z - a->z * b->y,
                       a->z * b->x - a->x * b->z,
                       a->x * b->y - a->y * b->x);
}

static float VectorDot(const D3DXVECTOR3* a, const D3DXVECTOR3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static D3DXMATRIX MatrixMultiply(const D3DXMATRIX& a, const D3DXMATRIX& b) {
    D3DXMATRIX r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] +
                        a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
        }
    }
    return r;
}

HRESULT WINAPI D3DXMatrixLookAtLH(D3DXMATRIX* pOut, const D3DXVECTOR3* pEye,
                                  const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp) {
    D3DXVECTOR3 zaxis(pAt->x - pEye->x, pAt->y - pEye->y, pAt->z - pEye->z);
    VectorNormalize(&zaxis);
    D3DXVECTOR3 xaxis = VectorCross(pUp, &zaxis);
    VectorNormalize(&xaxis);
    D3DXVECTOR3 yaxis = VectorCross(&zaxis, &xaxis);

    pOut->_11 = xaxis.x; pOut->_12 = yaxis.x; pOut->_13 = zaxis.x; pOut->_14 = 0.0f;
    pOut->_21 = xaxis.y; pOut->_22 = yaxis.y; pOut->_23 = zaxis.y; pOut->_24 = 0.0f;
    pOut->_31 = xaxis.z; pOut->_32 = yaxis.z; pOut->_33 = zaxis.z; pOut->_34 = 0.0f;
    pOut->_41 = -VectorDot(&xaxis, pEye);
    pOut->_42 = -VectorDot(&yaxis, pEye);
    pOut->_43 = -VectorDot(&zaxis, pEye);
    pOut->_44 = 1.0f;
    return S_OK;
}

HRESULT WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX* pOut, FLOAT fovy, FLOAT aspect,
                                          FLOAT zn, FLOAT zf) {
    float h = 1.0f / tanf(fovy * 0.5f);
    float w = h / aspect;

    memset(pOut, 0, sizeof(D3DMATRIX));
    pOut->_11 = w;
    pOut->_22 = h;
    pOut->_33 = zf / (zf - zn);
    pOut->_34 = 1.0f;
    pOut->_43 = -zn * zf / (zf - zn);
    return S_OK;
}

HRESULT WINAPI D3DXMatrixShadow(D3DXMATRIX* pOut, const D3DXVECTOR4* pLight,
                                const D3DXPLANE* pPlane) {
    float dot = pPlane->a * pLight->x + pPlane->b * pLight->y +
                pPlane->c * pLight->z + pPlane->d * pLight->w;

    pOut->_11 = dot - pLight->x * pPlane->a;
    pOut->_12 = -pLight->x * pPlane->b;
    pOut->_13 = -pLight->x * pPlane->c;
    pOut->_14 = -pLight->x * pPlane->d;
    pOut->_21 = -pLight->y * pPlane->a;
    pOut->_22 = dot - pLight->y * pPlane->b;
    pOut->_23 = -pLight->y * pPlane->c;
    pOut->_24 = -pLight->y * pPlane->d;
    pOut->_31 = -pLight->z * pPlane->a;
    pOut->_32 = -pLight->z * pPlane->b;
    pOut->_33 = dot - pLight->z * pPlane->c;
    pOut->_34 = -pLight->z * pPlane->d;
    pOut->_41 = -pLight->w * pPlane->a;
    pOut->_42 = -pLight->w * pPlane->b;
    pOut->_43 = -pLight->w * pPlane->c;
    pOut->_44 = dot - pLight->w * pPlane->d;
    return S_OK;
}

HRESULT WINAPI D3DXPlaneFromPointNormal(D3DXPLANE* pOut, const D3DXVECTOR3* pPoint,
                                        const D3DXVECTOR3* pNormal) {
    pOut->a = pNormal->x;
    pOut->b = pNormal->y;
    pOut->c = pNormal->z;
    pOut->d = -(pNormal->x * pPoint->x + pNormal->y * pPoint->y + pNormal->z * pPoint->z);
    return S_OK;
}

// ---------------------------------------------------------------------------
// Matrix stack
// ---------------------------------------------------------------------------

class CMatrixStack : public ID3DXMatrixStack {
public:
    CMatrixStack() {
        m_stack.resize(1);
        MatrixIdentity(&m_stack.back());
    }

    virtual ULONG Release() { delete this; return 0; }

    virtual HRESULT Push() {
        m_stack.push_back(m_stack.back());
        return S_OK;
    }

    virtual HRESULT Pop() {
        if (m_stack.size() > 1)
            m_stack.pop_back();
        return S_OK;
    }

    virtual HRESULT LoadIdentity() {
        MatrixIdentity(&m_stack.back());
        return S_OK;
    }

    virtual HRESULT LoadMatrix(const D3DXMATRIX* pMat) {
        m_stack.back() = *pMat;
        return S_OK;
    }

    virtual HRESULT MultMatrix(const D3DXMATRIX* pMat) {
        m_stack.back() = MatrixMultiply(m_stack.back(), *pMat);
        return S_OK;
    }

    virtual HRESULT TranslateLocal(FLOAT x, FLOAT y, FLOAT z) {
        D3DXMATRIX m;
        MatrixIdentity(&m);
        m._41 = x; m._42 = y; m._43 = z;
        return MultMatrix(&m);
    }

    virtual HRESULT ScaleLocal(FLOAT x, FLOAT y, FLOAT z) {
        D3DXMATRIX m;
        MatrixIdentity(&m);
        m._11 = x; m._22 = y; m._33 = z;
        return MultMatrix(&m);
    }

    virtual HRESULT RotateYawPitchRollLocal(FLOAT yaw, FLOAT pitch, FLOAT roll) {
        float sr = sinf(roll), cr = cosf(roll);
        float sp = sinf(pitch), cp = cosf(pitch);
        float sy = sinf(yaw), cy = cosf(yaw);

        D3DXMATRIX m;
        memset(&m, 0, sizeof(D3DMATRIX));
        m._11 = cr * cy + sr * sp * sy;
        m._12 = -cr * sy + sr * sp * cy;
        m._13 = sr * cp;
        m._21 = cp * sy;
        m._22 = cp * cy;
        m._23 = -sp;
        m._31 = -sr * cy + cr * sp * sy;
        m._32 = sr * sy + cr * sp * cy;
        m._33 = cr * cp;
        m._44 = 1.0f;
        return MultMatrix(&m);
    }

    virtual D3DXMATRIX* GetTop() { return &m_stack.back(); }

private:
    std::vector<D3DXMATRIX> m_stack;
};

HRESULT WINAPI D3DXCreateMatrixStack(DWORD flags, ID3DXMatrixStack** ppStack) {
    (void)flags;
    if (!ppStack)
        return D3DERR_INVALIDCALL;
    *ppStack = new CMatrixStack();
    return *ppStack ? S_OK : E_OUTOFMEMORY;
}

// ---------------------------------------------------------------------------
// Texture / surface loading (BMP through GDI)
// ---------------------------------------------------------------------------

static bool LoadBmp(std::vector<DWORD>& pixels, UINT& width, UINT& height, LPCSTR file) {
    HBITMAP hbm = (HBITMAP)LoadImageA(NULL, file, IMAGE_BITMAP, 0, 0,
                                      LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hbm)
        return false;

    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm)) {
        DeleteObject(hbm);
        return false;
    }

    width = (UINT)bm.bmWidth;
    height = (UINT)bm.bmHeight;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)width;
    bmi.bmiHeader.biHeight = -(LONG)height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    pixels.resize(width * height);

    HDC hdc = CreateCompatibleDC(NULL);
    if (!hdc) {
        DeleteObject(hbm);
        return false;
    }
    BOOL ok = GetDIBits(hdc, hbm, 0, height, &pixels[0], &bmi, DIB_RGB_COLORS);
    DeleteDC(hdc);
    DeleteObject(hbm);
    return ok ? true : false;
}

// pixels hold 0x00RRGGBB (from GetDIBits); writes count bytes for the format.
static void StorePixel(BYTE* dst, D3DFORMAT format, DWORD rgb) {
    BYTE r = (BYTE)((rgb >> 16) & 0xFF);
    BYTE g = (BYTE)((rgb >> 8) & 0xFF);
    BYTE b = (BYTE)(rgb & 0xFF);

    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *(DWORD*)dst = 0xFF000000u | rgb;
        break;
    case D3DFMT_R5G6B5:
        *(WORD*)dst = (WORD)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        break;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        *(WORD*)dst = (WORD)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3) |
                             (format == D3DFMT_A1R5G5B5 ? 0x8000 : 0));
        break;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        *(WORD*)dst = (WORD)(((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4) |
                             (format == D3DFMT_A4R4G4B4 ? 0xF000 : 0));
        break;
    case D3DFMT_R8G8B8:
        dst[0] = b; dst[1] = g; dst[2] = r;
        break;
    default:
        *(DWORD*)dst = 0xFF000000u | rgb;
        break;
    }
}

static UINT BytesPerPixel(D3DFORMAT format) {
    switch (format) {
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        return 2;
    case D3DFMT_R8G8B8:
        return 3;
    default:
        return 4;
    }
}

HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE8 pDevice, UINT width, UINT height,
                                 UINT mipLevels, DWORD usage, D3DFORMAT format,
                                 D3DPOOL pool, LPDIRECT3DTEXTURE8* ppTexture) {
    if (!pDevice || !ppTexture)
        return D3DERR_INVALIDCALL;
    if (mipLevels == D3DX_DEFAULT || mipLevels == D3DX_DEFAULT_NONPOW2)
        mipLevels = 1;
    if (format == D3DFMT_UNKNOWN)
        format = D3DFMT_A8R8G8B8;
    return pDevice->CreateTexture(width, height, mipLevels, usage, format, pool, ppTexture);
}

HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE8 pDevice, LPCSTR pSrcFile,
                                            UINT width, UINT height, UINT mipLevels,
                                            DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                            DWORD filter, DWORD mipFilter, D3DCOLOR colorKey,
                                            D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette,
                                            LPDIRECT3DTEXTURE8* ppTexture) {
    (void)width; (void)height; (void)filter; (void)mipFilter;
    (void)colorKey; (void)pPalette;

    if (!pDevice || !pSrcFile || !ppTexture)
        return D3DERR_INVALIDCALL;

    std::vector<DWORD> pixels;
    UINT w = 0, h = 0;
    if (!LoadBmp(pixels, w, h, pSrcFile))
        return E_FAIL;

    if (pSrcInfo) {
        memset(pSrcInfo, 0, sizeof(*pSrcInfo));
        pSrcInfo->Width = w;
        pSrcInfo->Height = h;
        pSrcInfo->Depth = 24;
        pSrcInfo->MipLevels = 1;
        pSrcInfo->Format = format;
        pSrcInfo->ResourceType = D3DRTYPE_TEXTURE;
    }

    if (mipLevels == D3DX_DEFAULT || mipLevels == D3DX_DEFAULT_NONPOW2)
        mipLevels = 1;
    if (format == D3DFMT_UNKNOWN)
        format = D3DFMT_A8R8G8B8;

    LPDIRECT3DTEXTURE8 texture = NULL;
    HRESULT hr = pDevice->CreateTexture(w, h, mipLevels, usage, format, pool, &texture);
    if (FAILED(hr))
        return hr;

    D3DLOCKED_RECT locked;
    hr = texture->LockRect(0, &locked, NULL, 0);
    if (FAILED(hr)) {
        texture->Release();
        return hr;
    }

    UINT bpp = BytesPerPixel(format);
    BYTE* base = (BYTE*)locked.pBits;
    for (UINT y = 0; y < h; ++y) {
        BYTE* row = base + (INT)y * locked.Pitch;
        for (UINT x = 0; x < w; ++x)
            StorePixel(row + x * bpp, format, pixels[y * w + x]);
    }
    texture->UnlockRect(0);

    *ppTexture = texture;
    return S_OK;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileA(LPDIRECT3DSURFACE8 pDestSurface,
                                        const PALETTEENTRY* pDestPalette,
                                        const RECT* pDestRect, LPCSTR pSrcFile,
                                        const RECT* pSrcRect, DWORD filter,
                                        D3DCOLOR colorKey, D3DXIMAGE_INFO* pSrcInfo) {
    (void)pDestPalette; (void)filter; (void)colorKey;

    if (!pDestSurface || !pSrcFile)
        return D3DERR_INVALIDCALL;

    std::vector<DWORD> pixels;
    UINT sw = 0, sh = 0;
    if (!LoadBmp(pixels, sw, sh, pSrcFile))
        return E_FAIL;

    if (pSrcInfo) {
        memset(pSrcInfo, 0, sizeof(*pSrcInfo));
        pSrcInfo->Width = sw;
        pSrcInfo->Height = sh;
        pSrcInfo->Depth = 24;
        pSrcInfo->MipLevels = 1;
        pSrcInfo->ResourceType = D3DRTYPE_SURFACE;
    }

    D3DSURFACE_DESC desc;
    HRESULT hr = pDestSurface->GetDesc(&desc);
    if (FAILED(hr))
        return hr;

    RECT dst;
    if (pDestRect) dst = *pDestRect;
    else { dst.left = 0; dst.top = 0; dst.right = (LONG)desc.Width; dst.bottom = (LONG)desc.Height; }

    RECT src;
    if (pSrcRect) src = *pSrcRect;
    else { src.left = 0; src.top = 0; src.right = (LONG)sw; src.bottom = (LONG)sh; }

    LONG dstW = dst.right - dst.left;
    LONG dstH = dst.bottom - dst.top;
    LONG srcW = src.right - src.left;
    LONG srcH = src.bottom - src.top;
    if (dstW <= 0 || dstH <= 0 || srcW <= 0 || srcH <= 0)
        return D3DERR_INVALIDCALL;

    D3DLOCKED_RECT locked;
    hr = pDestSurface->LockRect(&locked, &dst, 0);
    if (FAILED(hr))
        return hr;

    UINT bpp = BytesPerPixel(desc.Format);
    BYTE* base = (BYTE*)locked.pBits;
    for (LONG y = 0; y < dstH; ++y) {
        LONG sy = src.top + (LONG)((float)y * srcH / dstH);
        if (sy >= (LONG)sh) sy = (LONG)sh - 1;
        BYTE* row = base + y * locked.Pitch;
        for (LONG x = 0; x < dstW; ++x) {
            LONG sx = src.left + (LONG)((float)x * srcW / dstW);
            if (sx >= (LONG)sw) sx = (LONG)sw - 1;
            StorePixel(row + x * bpp, desc.Format, pixels[sy * sw + sx]);
        }
    }

    pDestSurface->UnlockRect();
    return S_OK;
}
