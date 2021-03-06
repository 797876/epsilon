#include <SDL.h>
#include <windows.h>
#include <olectl.h>
#include <gdiplus.h>

/* Loading images using GDI+
 * On Windows, we decompress JPEG images using GDI+ which is widely available.
 * Note that this adds an extra runtime dependency (as compared to just SDL),
 * but this should not be an issue. */

HRESULT CreateStreamOnResource(const char * name, LPSTREAM * stream) {
  HINSTANCE hInstance = GetModuleHandle(0);
  *stream = nullptr;
  HRSRC hC = FindResource(hInstance, name, RT_RCDATA);
  if (!hC) {
    SDL_Log("Could not find resource %s", name);
    return E_INVALIDARG;
  }
    // This is not really a HGLOBAL http://msdn.microsoft.com/en-us/library/windows/desktop/ms648046(v=vs.85).aspx
  HGLOBAL hG = LoadResource(hInstance, hC);
  if (!hG) {
    SDL_Log("Could not load resource %s", name);
    return E_INVALIDARG;
  }
  void * bytes = LockResource(hG);
  ULONG size = SizeofResource(hInstance, hC);
  HRESULT hr = CreateStreamOnHGlobal(NULL, true, stream);
  if (SUCCEEDED(hr)) {
    ULONG written;
    hr = (*stream)->Write(bytes, size, &written);
  }
  return hr;
}

extern "C" SDL_Texture * loadImage(SDL_Renderer * renderer, const char * identifier) {
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

  LPSTREAM stream;
  const char * resname = MAKEINTRESOURCE(300);
  CreateStreamOnResource(resname, &stream);

  Gdiplus::Bitmap * image = Gdiplus::Bitmap::FromStream(stream);

  int width = (int)image->GetWidth();
  int height = (int)image->GetHeight();
  Gdiplus::Rect rc(0, 0, width, height);

  Gdiplus::BitmapData * bitmapData = new Gdiplus::BitmapData;
  image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);

  SDL_Texture * texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STATIC,
    width,
    height
  );

  SDL_UpdateTexture(
    texture,
    NULL,
    bitmapData->Scan0,
    4 * width
  );

  image->UnlockBits(bitmapData);
  delete bitmapData;
  delete image;
  Gdiplus::GdiplusShutdown(gdiplusToken);

  return texture;
}
