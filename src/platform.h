#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define PLATFORM_FB  0 // 通用平台
#define PLATFORM_T31 1 // T31平台

// 接收来自 Makefile 的传参
#ifdef MAKE_PLATFORM
#define WV_PLATFORM MAKE_PLATFORM
#else
#define WV_PLATFORM PLATFORM_FB
#endif

#ifdef MAKE_FREETYPE
#define WV_FREETYPE MAKE_FREETYPE
#else
#define WV_FREETYPE 1
#endif

#ifdef MAKE_JPEG
#define WV_JPEG MAKE_JPEG
#else
#define WV_JPEG 1
#endif

#ifdef MAKE_ICONV
#define WV_ICONV MAKE_ICONV
#else
#define WV_ICONV 1
#endif

// ===== T31平台对接 =====
#if(MAKE_PLATFORM == PLATFORM_T31)

// ===== 通用fb平台对接 =====
#else

#endif

#endif // end of file
