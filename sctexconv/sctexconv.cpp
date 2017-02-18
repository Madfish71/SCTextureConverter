//--------------------------------------------------------------------------------------
// File: sctexconv.cpp
//
// DirectX 11 Star Citizen Texture Converter
// D. Fisher (Jan 2017)
//
// A modified (poorly) version of DirectX11 Texture Converter,
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// NOTE: This is software is not produced, endorsed nor supported 
//       by Cloud Imperium Games (CIG) or Microsoft.
//--------------------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <tchar.h>
#include <windows.h>
#include <memory>
#include <list>
#include <string>
#include <iostream>
#include <iterator>
#include <ctime>
#include <comdef.h>
#include <strsafe.h>
#include <regex>
#include <atlbase.h>
#include <vector>
#include <wrl\client.h>

#include <dxgiformat.h>
#include <FreeImage.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "directxtex.h"
#include "directxtexp.h"
#include "Unsplit.h"
 
using namespace DirectX;
using Microsoft::WRL::ComPtr;


LPCWSTR VERSION = L"1.3";
std::string	sVERSION = "1.3";

enum OPTIONS    // Note: dwOptions below assumes 64 or less options.
{
    OPT_WIDTH = 1,
    OPT_HEIGHT,
    OPT_MIPLEVELS,
    OPT_FORMAT,
    OPT_FILTER,
    OPT_SRGBI,
    OPT_SRGBO,
    OPT_SRGB,
    OPT_PREFIX,
    OPT_SUFFIX,
    OPT_OUTPUTDIR,
    OPT_FILETYPE,
    OPT_HFLIP,
    OPT_VFLIP,
    OPT_DDS_DWORD_ALIGN,
    OPT_USE_DX10,
    OPT_NOLOGO,
    OPT_TIMING,
    OPT_SEPALPHA,
    OPT_TYPELESS_UNORM,
    OPT_TYPELESS_FLOAT,
    OPT_PREMUL_ALPHA,
    OPT_EXPAND_LUMINANCE,
    OPT_TA_WRAP,
    OPT_TA_MIRROR,
    OPT_FORCE_SINGLEPROC,
    OPT_NOGPU,
    OPT_FEATURE_LEVEL,
    OPT_FIT_POWEROF2,
    OPT_ALPHA_WEIGHT,
    OPT_NORMAL_MAP,
    OPT_NORMAL_MAP_AMPLITUDE,
    OPT_COMPRESS_UNIFORM,
    OPT_COMPRESS_MAX,
    OPT_COMPRESS_DITHER,
    OPT_MAX
};

static_assert( OPT_MAX <= 64, "dwOptions is a DWORD64 bitfield" );

struct SConversion
{
    WCHAR szSrc [MAX_PATH];
    WCHAR szDest[MAX_PATH];
};

struct SValue
{
    LPCWSTR pName;
    DWORD dwValue;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

SValue g_pOptions[] = 
{
    { L"w",             OPT_WIDTH     },
    { L"h",             OPT_HEIGHT    },
    { L"m",             OPT_MIPLEVELS },
    { L"f",             OPT_FORMAT    },
    { L"if",            OPT_FILTER    },
    { L"srgbi",         OPT_SRGBI     },
    { L"srgbo",         OPT_SRGBO     },
    { L"srgb",          OPT_SRGB      },
    { L"px",            OPT_PREFIX    },
    { L"sx",            OPT_SUFFIX    },
    { L"o",             OPT_OUTPUTDIR },
    { L"ft",            OPT_FILETYPE  },
    { L"hflip",         OPT_HFLIP     },
    { L"vflip",         OPT_VFLIP     },
    { L"dword",         OPT_DDS_DWORD_ALIGN },
    { L"dx10",          OPT_USE_DX10  },
    { L"nologo",        OPT_NOLOGO    },
    { L"timing",        OPT_TIMING    },
    { L"sepalpha",      OPT_SEPALPHA  },
    { L"tu",            OPT_TYPELESS_UNORM },
    { L"tf",            OPT_TYPELESS_FLOAT },
    { L"pmalpha",       OPT_PREMUL_ALPHA },
    { L"xlum",          OPT_EXPAND_LUMINANCE },
    { L"wrap",          OPT_TA_WRAP },
    { L"mirror",        OPT_TA_MIRROR },
    { L"singleproc",    OPT_FORCE_SINGLEPROC },
    { L"nogpu",         OPT_NOGPU     },
    { L"fl",            OPT_FEATURE_LEVEL },
    { L"pow2",          OPT_FIT_POWEROF2 },
    { L"aw",            OPT_ALPHA_WEIGHT },
    { L"nmap",          OPT_NORMAL_MAP },
    { L"nmapamp",       OPT_NORMAL_MAP_AMPLITUDE },
    { L"bcuniform",     OPT_COMPRESS_UNIFORM },
    { L"bcmax",         OPT_COMPRESS_MAX },
    { L"bcdither",      OPT_COMPRESS_DITHER },
    { nullptr,          0             }
};

#define DEFFMT(fmt) { L#fmt, DXGI_FORMAT_ ## fmt }

SValue g_pFormats[] = 
{
    // List does not include _TYPELESS or depth/stencil formats
    DEFFMT(R32G32B32A32_FLOAT), 
    DEFFMT(R32G32B32A32_UINT), 
    DEFFMT(R32G32B32A32_SINT), 
    DEFFMT(R32G32B32_FLOAT), 
    DEFFMT(R32G32B32_UINT), 
    DEFFMT(R32G32B32_SINT), 
    DEFFMT(R16G16B16A16_FLOAT), 
    DEFFMT(R16G16B16A16_UNORM), 
    DEFFMT(R16G16B16A16_UINT), 
    DEFFMT(R16G16B16A16_SNORM), 
    DEFFMT(R16G16B16A16_SINT), 
    DEFFMT(R32G32_FLOAT), 
    DEFFMT(R32G32_UINT), 
    DEFFMT(R32G32_SINT), 
    DEFFMT(R10G10B10A2_UNORM), 
    DEFFMT(R10G10B10A2_UINT), 
    DEFFMT(R11G11B10_FLOAT), 
    DEFFMT(R8G8B8A8_UNORM), 
    DEFFMT(R8G8B8A8_UNORM_SRGB), 
    DEFFMT(R8G8B8A8_UINT), 
    DEFFMT(R8G8B8A8_SNORM), 
    DEFFMT(R8G8B8A8_SINT), 
    DEFFMT(R16G16_FLOAT), 
    DEFFMT(R16G16_UNORM), 
    DEFFMT(R16G16_UINT), 
    DEFFMT(R16G16_SNORM), 
    DEFFMT(R16G16_SINT), 
    DEFFMT(R32_FLOAT), 
    DEFFMT(R32_UINT), 
    DEFFMT(R32_SINT), 
    DEFFMT(R8G8_UNORM), 
    DEFFMT(R8G8_UINT), 
    DEFFMT(R8G8_SNORM), 
    DEFFMT(R8G8_SINT), 
    DEFFMT(R16_FLOAT), 
    DEFFMT(R16_UNORM), 
    DEFFMT(R16_UINT), 
    DEFFMT(R16_SNORM), 
    DEFFMT(R16_SINT), 
    DEFFMT(R8_UNORM), 
    DEFFMT(R8_UINT), 
    DEFFMT(R8_SNORM), 
    DEFFMT(R8_SINT), 
    DEFFMT(A8_UNORM), 
    DEFFMT(R9G9B9E5_SHAREDEXP), 
    DEFFMT(R8G8_B8G8_UNORM), 
    DEFFMT(G8R8_G8B8_UNORM), 
    DEFFMT(BC1_UNORM), 
    DEFFMT(BC1_UNORM_SRGB), 
    DEFFMT(BC2_UNORM), 
    DEFFMT(BC2_UNORM_SRGB), 
    DEFFMT(BC3_UNORM), 
    DEFFMT(BC3_UNORM_SRGB), 
    DEFFMT(BC4_UNORM), 
    DEFFMT(BC4_SNORM), 
    DEFFMT(BC5_UNORM), 
    DEFFMT(BC5_SNORM),
    DEFFMT(B5G6R5_UNORM),
    DEFFMT(B5G5R5A1_UNORM),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_UNORM),
    DEFFMT(B8G8R8X8_UNORM),
    DEFFMT(R10G10B10_XR_BIAS_A2_UNORM),
    DEFFMT(B8G8R8A8_UNORM_SRGB),
    DEFFMT(B8G8R8X8_UNORM_SRGB),
    DEFFMT(BC6H_UF16),
    DEFFMT(BC6H_SF16),
    DEFFMT(BC7_UNORM),
    DEFFMT(BC7_UNORM_SRGB),

    // DXGI 1.2 formats
    DEFFMT(AYUV),
    DEFFMT(Y410),
    DEFFMT(Y416),
    DEFFMT(YUY2),
    DEFFMT(Y210),
    DEFFMT(Y216),
    // No support for legacy paletted video formats (AI44, IA44, P8, A8P8)
    DEFFMT(B4G4R4A4_UNORM),

    { nullptr, DXGI_FORMAT_UNKNOWN }
};

SValue g_pReadOnlyFormats[] = 
{
    DEFFMT(R32G32B32A32_TYPELESS), 
    DEFFMT(R32G32B32_TYPELESS),
    DEFFMT(R16G16B16A16_TYPELESS),
    DEFFMT(R32G32_TYPELESS),
    DEFFMT(R32G8X24_TYPELESS),
    DEFFMT(D32_FLOAT_S8X24_UINT),
    DEFFMT(R32_FLOAT_X8X24_TYPELESS),
    DEFFMT(X32_TYPELESS_G8X24_UINT),
    DEFFMT(R10G10B10A2_TYPELESS),
    DEFFMT(R8G8B8A8_TYPELESS),
    DEFFMT(R16G16_TYPELESS),
    DEFFMT(R32_TYPELESS),
    DEFFMT(D32_FLOAT),
    DEFFMT(R24G8_TYPELESS),
    DEFFMT(D24_UNORM_S8_UINT),
    DEFFMT(R24_UNORM_X8_TYPELESS),
    DEFFMT(X24_TYPELESS_G8_UINT),
    DEFFMT(R8G8_TYPELESS),
    DEFFMT(R16_TYPELESS),
    DEFFMT(R8_TYPELESS),
    DEFFMT(BC1_TYPELESS),
    DEFFMT(BC2_TYPELESS),
    DEFFMT(BC3_TYPELESS),
    DEFFMT(BC4_TYPELESS),
    DEFFMT(BC5_TYPELESS),

    // DXGI 1.1 formats
    DEFFMT(B8G8R8A8_TYPELESS),
    DEFFMT(B8G8R8X8_TYPELESS),
    DEFFMT(BC6H_TYPELESS),
    DEFFMT(BC7_TYPELESS),

    // DXGI 1.2 formats
    DEFFMT(NV12),
    DEFFMT(P010),
    DEFFMT(P016),
    DEFFMT(420_OPAQUE),
    DEFFMT(NV11),

    { nullptr, DXGI_FORMAT_UNKNOWN }
};

SValue g_pFilters[] = 
{
    { L"POINT",                     TEX_FILTER_POINT },
    { L"LINEAR",                    TEX_FILTER_LINEAR },
    { L"CUBIC",                     TEX_FILTER_CUBIC },
    { L"FANT",                      TEX_FILTER_FANT },
    { L"BOX",                       TEX_FILTER_BOX },
    { L"TRIANGLE",                  TEX_FILTER_TRIANGLE },
    { L"POINT_DITHER",              TEX_FILTER_POINT  | TEX_FILTER_DITHER },
    { L"LINEAR_DITHER",             TEX_FILTER_LINEAR | TEX_FILTER_DITHER },
    { L"CUBIC_DITHER",              TEX_FILTER_CUBIC  | TEX_FILTER_DITHER },
    { L"FANT_DITHER",               TEX_FILTER_FANT   | TEX_FILTER_DITHER },
    { L"BOX_DITHER",                TEX_FILTER_BOX    | TEX_FILTER_DITHER },
    { L"TRIANGLE_DITHER",           TEX_FILTER_TRIANGLE | TEX_FILTER_DITHER },
    { L"POINT_DITHER_DIFFUSION",    TEX_FILTER_POINT  | TEX_FILTER_DITHER_DIFFUSION },
    { L"LINEAR_DITHER_DIFFUSION",   TEX_FILTER_LINEAR | TEX_FILTER_DITHER_DIFFUSION },
    { L"CUBIC_DITHER_DIFFUSION",    TEX_FILTER_CUBIC  | TEX_FILTER_DITHER_DIFFUSION },
    { L"FANT_DITHER_DIFFUSION",     TEX_FILTER_FANT   | TEX_FILTER_DITHER_DIFFUSION },
    { L"BOX_DITHER_DIFFUSION",      TEX_FILTER_BOX    | TEX_FILTER_DITHER_DIFFUSION },
    { L"TRIANGLE_DITHER_DIFFUSION", TEX_FILTER_TRIANGLE | TEX_FILTER_DITHER_DIFFUSION },
    { nullptr,                      TEX_FILTER_DEFAULT                              }
};

#define CODEC_DDS 0xFFFF0001 
#define CODEC_TGA 0xFFFF0002

SValue g_pSaveFileTypes[] =     // valid formats to write to
{
    { L"BMP",           WIC_CODEC_BMP  },
    { L"JPG",           WIC_CODEC_JPEG },
    { L"JPEG",          WIC_CODEC_JPEG },
    { L"PNG",           WIC_CODEC_PNG  },
    { L"DDS",           CODEC_DDS      },
    { L"TGA",           CODEC_TGA      },
    { L"TIF",           WIC_CODEC_TIFF },
    { L"TIFF",          WIC_CODEC_TIFF },
    { L"WDP",           WIC_CODEC_WMP  },
    { L"HDP",           WIC_CODEC_WMP  },
    { nullptr,          CODEC_DDS      }
};

SValue g_pFeatureLevels[] =     // valid feature levels for -fl for maximimum size
{
    { L"9.1",           2048 },
    { L"9.2",           2048 },
    { L"9.3",           4096 },
    { L"10.0",          8192 },
    { L"10.1",          8192 },
    { L"11.0",          16384 },
    { L"11.1",          16384 },
    { L"12.0",          16384 },
    { L"12.1",          16384 },
    { nullptr,          0 },
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4616 6211 )

inline static bool ispow2(size_t x)
{
    return ((x != 0) && !(x & (x - 1)));
}

#pragma prefast(disable : 26018, "Only used with static internal arrays")

DWORD LookupByName(const WCHAR *pName, const SValue *pArray)
{
    while(pArray->pName)
    {
        if(!_wcsicmp(pName, pArray->pName))
            return pArray->dwValue;

        pArray++;
    }

    return 0;
}

const WCHAR* LookupByValue(DWORD pValue, const SValue *pArray)
{
    while(pArray->pName)
    {
        if(pValue == pArray->dwValue)
            return pArray->pName;

        pArray++;
    }

    return L"";
}

void PrintFormat(DXGI_FORMAT Format, std::string &logfile)
{
    for(SValue *pFormat = g_pFormats; pFormat->pName; pFormat++)
    {
        if((DXGI_FORMAT) pFormat->dwValue == Format)
        {
			ATL::CW2A lpName(pFormat->pName);
			std::string sName = lpName;
			LogMessage(logfile, sName, true);
            wprintf( pFormat->pName );
            return;
        }
    }

    for(SValue *pFormat = g_pReadOnlyFormats; pFormat->pName; pFormat++)
    {
        if((DXGI_FORMAT) pFormat->dwValue == Format)
        {
			ATL::CW2A lpName(pFormat->pName);
			std::string sName = lpName;
			LogMessage(logfile, sName, true);
            wprintf( pFormat->pName );
            return;
        }
    }
	std::string msg = "*UNKNOWN*";
	LogMessage(logfile, msg, true);
    wprintf( L"*UNKNOWN*" );
}

void PrintInfo( const TexMetadata& info, string &logfile)
{
    wprintf( L" (%Iux%Iu", info.width, info.height);
	std:: string msg = ( "(" + to_string(info.width) + 'x' + to_string(info.height));

	if (TEX_DIMENSION_TEXTURE3D == info.dimension)
	{
		msg = msg + "x" + to_string((int)info.depth) + " ";	
		wprintf(L"x%Iu", info.depth);
	}

	if (info.mipLevels > 1)
	{
		msg = msg + ", " + to_string((int)info.mipLevels);
		wprintf(L",%Iu", info.mipLevels);
	}

	if (info.arraySize > 1)
	{
		msg = msg + ", " + to_string((int)info.arraySize);
		wprintf(L",%Iu", info.arraySize);
	}
	LogMessage(logfile, msg, true);
    wprintf( L" ");
    PrintFormat( info.format, logfile );
	wprintf(L" ");
    switch ( info.dimension )
    {
    case TEX_DIMENSION_TEXTURE1D:
        //wprintf( (info.arraySize > 1) ? L" 1DArray" : L"1D" );
		msg = ((info.arraySize > 1) ? " 1DArray" : "1D");
        break;

    case TEX_DIMENSION_TEXTURE2D:
        if ( info.IsCubemap() )
        {
           // wprintf( (info.arraySize > 6) ? L" CubeArray" : L" Cube" );
			msg = ((info.arraySize > 6) ? " CubeArray" : "Cube");
        }
        else
        {
           // wprintf( (info.arraySize > 1) ? L" 2DArray" : L"2D" );
			msg = ((info.arraySize > 1) ? " 2DArray" : "2D");
        }
        break;

    case TEX_DIMENSION_TEXTURE3D:
        //wprintf( L" 3D");
		msg = "3D";
        break;
    }

    //wprintf( L")");
	msg = msg + ")";
	MessageOut(logfile, msg, true, false);
}

void PrintList(size_t cch, SValue *pValue)
{
    while(pValue->pName)
    {
        size_t cchName = wcslen(pValue->pName);
        
        if(cch + cchName + 2>= 80)
        {
            wprintf( L"\n      ");
            cch = 6;
        }

        wprintf( L"%ls ", pValue->pName );
        cch += cchName + 2;
        pValue++;
    }

    wprintf( L"\n");
}

void PrintLogo()
{
	wprintf( L" Star Citizen Texture Converter (v%ls) \n", VERSION);
    wprintf( L" A modified version of Microsoft (R) DirectX 11 Texture Converter\n");
    wprintf( L" Copyright (C) Microsoft Corp. All rights reserved.\n");
	wprintf( L" NOTE: This software is not produced, endorsed nor supported \n");
	wprintf( L" by Cloud Imperium Games (CIG) or Microsoft...\n\n");
	wprintf( L" [ - Hold down ESCAPE key to cancel conversion process - ] \n");
#ifdef _DEBUG
    wprintf( L"*** Debug build ***\n");
#endif
    wprintf( L"\n");
}

// Print logo text to log file
void PrintLogo(string logfile)
{
	std::ofstream log(logfile, std::ofstream::out | std::ofstream::app);

	if (!log)
	{
		std::cerr << "Cannot open the output file." << std::endl;
		return;
	}
		
	log << " Star Citizen Texture Converter (v" << sVERSION << ")" << std::endl;
	log << " A modified version of Microsoft (R) DirectX 11 Texture Converter" << std::endl;
	log << " Copyright (C) Microsoft Corp. All rights reserved." << std::endl;
	log << " NOTE: This software is not produced, endorsed nor supported " << std::endl;
	log << " by Cloud Imperium Games (CIG) or Microsoft..." << std::endl;
#ifdef _DEBUG
	log << " *** Debug build ***" << std::endl;
#endif
	log << "\n" << std::ends;

	log.close();
}

void PrintUsage()
{
    PrintLogo();

    wprintf( L"Usage: sctexconv <options> <files>\n");
    wprintf( L"\n");
    wprintf( L"   -w <n>              width\n");
    wprintf( L"   -h <n>              height\n");
    wprintf( L"   -m <n>              miplevels\n");
    wprintf( L"   -f <format>         format\n");
    wprintf( L"   -if <filter>        image filtering\n");
    wprintf( L"   -srgb{i|o}          sRGB {input, output}\n");
    wprintf( L"   -px <string>        name prefix\n");
    wprintf( L"   -sx <string>        name suffix\n");
    wprintf( L"   -o <directory>      output directory\n");
    wprintf( L"   -ft <filetype>      output file type\n");
    wprintf( L"   -hflip              horizonal flip of source image\n");
    wprintf( L"   -vflip              vertical flip of source image\n");
    wprintf( L"   -sepalpha           resize/generate mips alpha channel separately\n");
    wprintf( L"                       from color channels\n");
    wprintf( L"   -wrap, -mirror      texture addressing mode (wrap, mirror, or clamp)\n");
    wprintf( L"   -pmalpha            convert final texture to use premultiplied alpha\n");
    wprintf( L"   -pow2               resize to fit a power-of-2, respecting aspect ratio\n" );
    wprintf (L"   -nmap <options>     converts height-map to normal-map\n"
             L"                       options must be one or more of\n"
             L"                          r, g, b, a, l, m, u, v, i, o \n" );
    wprintf (L"   -nmapamp <weight>   normal map amplitude (defaults to 1.0)\n" );
    wprintf( L"   -fl <feature-level> Set maximum feature level target (defaults to 11.0)\n");
    wprintf( L"\n                       (DDS input only)\n");
    wprintf( L"   -t{u|f}             TYPELESS format is treated as UNORM or FLOAT\n");
    wprintf( L"   -dword              Use DWORD instead of BYTE alignment\n");
    wprintf( L"   -xlum               expand legacy L8, L16, and A8P8 formats\n");
    wprintf( L"\n                       (DDS output only)\n");
    wprintf( L"   -dx10               Force use of 'DX10' extended header\n");
    wprintf( L"\n   -nologo             suppress copyright message\n");
    wprintf( L"   -timing             Display elapsed processing time\n\n");
#ifdef _OPENMP
    wprintf( L"   -singleproc         Do not use multi-threaded compression\n");
#endif
    wprintf( L"   -nogpu              Do not use DirectCompute-based codecs\n");
    wprintf( L"   -bcuniform          Use uniform rather than perceptual weighting for BC1-3\n");
    wprintf( L"   -bcdither           Use dithering for BC1-3\n");
    wprintf( L"   -bcmax              Use exchaustive compression (BC7 only)\n");
    wprintf( L"   -aw <weight>        BC7 GPU compressor weighting for alpha error metric\n"
             L"                       (defaults to 1.0)\n" );

    wprintf( L"\n");
    wprintf( L"   <format>: ");
    PrintList(13, g_pFormats);

    wprintf( L"\n");
    wprintf( L"   <filter>: ");
    PrintList(13, g_pFilters);

    wprintf( L"\n");
    wprintf( L"   <filetype>: ");
    PrintList(15, g_pSaveFileTypes);

    wprintf( L"\n");
    wprintf( L"   <feature-level>: ");
    PrintList(13, g_pFeatureLevels);

}

_Success_(return != false)
bool CreateDevice( _Outptr_ ID3D11Device** pDevice )
{
    if ( !pDevice )
        return false;

    *pDevice  = nullptr;

    static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;
   
    if ( !s_DynamicD3D11CreateDevice )
    {            
        HMODULE hModD3D11 = LoadLibrary( L"d3d11.dll" );
        if ( !hModD3D11 )
            return false;

        s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void*>( GetProcAddress( hModD3D11, "D3D11CreateDevice" ) ) ); 
        if ( !s_DynamicD3D11CreateDevice )
            return false;
    }

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = s_DynamicD3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevels, _countof(featureLevels), 
                                             D3D11_SDK_VERSION, pDevice, &fl, nullptr );
    if ( SUCCEEDED(hr) )
    {
        if ( fl < D3D_FEATURE_LEVEL_11_0 )
        {
            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
            hr = (*pDevice)->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts) );
            if ( FAILED(hr) )
                memset( &hwopts, 0, sizeof(hwopts) );

            if ( !hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x )
            {
                if ( *pDevice )
                {
                    (*pDevice)->Release();
                    *pDevice = nullptr;
                }
                hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
        }
    }

    if ( SUCCEEDED(hr) )
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        hr = (*pDevice)->QueryInterface( __uuidof( IDXGIDevice ), reinterpret_cast< void** >( dxgiDevice.GetAddressOf() ) );
        if ( SUCCEEDED(hr) )
        {
            ComPtr<IDXGIAdapter> pAdapter;
            hr = dxgiDevice->GetAdapter( pAdapter.GetAddressOf() );
            if ( SUCCEEDED(hr) )
            {
                DXGI_ADAPTER_DESC desc;
                hr = pAdapter->GetDesc( &desc );
                if ( SUCCEEDED(hr) )
                {
                    wprintf( L"\n[Using DirectCompute on \"%ls\"]\n", desc.Description );
                }
            }
        }

        return true;
    }
    else
        return false;
}

void FitPowerOf2( size_t origx, size_t origy, size_t& targetx, size_t& targety, size_t maxsize )
{
    float origAR = float(origx) / float(origy);

    if ( origx > origy )
    {
        size_t x;
        for( x = maxsize; x > 1; x >>= 1 ) { if ( x <= targetx ) break; };
        targetx = x;

        float bestScore = FLT_MAX;
        for( size_t y = maxsize; y > 0; y >>= 1 )
        {
            float score = fabs( (float(x) / float(y)) - origAR );
            if ( score < bestScore )
            {
                bestScore = score;
                targety = y;
            }
        }
    }
    else
    {
        size_t y;
        for( y = maxsize; y > 1; y >>= 1 ) { if ( y <= targety ) break; };
        targety = y;

        float bestScore = FLT_MAX;
        for( size_t x = maxsize; x > 0; x >>= 1 )
        {
            float score = fabs( (float(x) / float(y)) - origAR );
            if ( score < bestScore )
            {
                bestScore = score;
                targetx = x;
            }
        }
    }
}

#pragma region CONVERTER

//--------------------------------------------------------------------------------------
// Previous Entry-point for stand-alone MS Texture Converter
//--------------------------------------------------------------------------------------
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")

//int __cdecl SCTexConvert(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
int SCTexConvert(int argc, wchar_t* argv[], std::string &logfile, int &failcount)
{
    // Parameters and defaults
    size_t width = 0;
    size_t height = 0; 
    size_t mipLevels = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    DWORD dwFilter = TEX_FILTER_DEFAULT;
    DWORD dwSRGB = 0;
    DWORD dwCompress = TEX_COMPRESS_DEFAULT;
    DWORD dwFilterOpts = 0;
    DWORD FileType = CODEC_DDS;
    DWORD maxSize = 16384;
    float alphaWeight = 1.f;
    DWORD dwNormalMap = 0;
    float nmapAmplitude = 1.f;

    WCHAR szPrefix   [MAX_PATH];
    WCHAR szSuffix   [MAX_PATH];
    WCHAR szOutputDir[MAX_PATH];

    szPrefix[0]    = 0;
    szSuffix[0]    = 0;
    szOutputDir[0] = 0;

	std::string msg = "";

    // Initialize COM (needed for WIC)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if( FAILED(hr) )
    {
		failcount++;
       // wprintf( L" Failed to initialize COM (%08X)\n", hr);

		MessageOut(logfile, (" Failed to initialize COM " + to_string(hr)), false, true);
        return 1;
    }

    // Process command line
    DWORD64 dwOptions = 0;
    std::list<SConversion> conversion;

    for(int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if(('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for(pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if(*pValue)
                *pValue++ = 0;

            DWORD dwOption = LookupByName(pArg, g_pOptions);

            if(!dwOption || (dwOptions & (DWORD64(1) << dwOption)))
            {
			//	wprintf(L"Invalid option passed to converter\n");
				failcount++;
				MessageOut(logfile, (msg + " Invalid option passed to converter"), false, true);
				//PrintUsage();
                return 1;
            }

            dwOptions |= (DWORD64(1) << dwOption);

            if( (OPT_NOLOGO != dwOption) && (OPT_TIMING != dwOption) && (OPT_TYPELESS_UNORM != dwOption) && (OPT_TYPELESS_FLOAT != dwOption)
                && (OPT_SEPALPHA != dwOption) && (OPT_PREMUL_ALPHA != dwOption) && (OPT_EXPAND_LUMINANCE != dwOption)
                && (OPT_TA_WRAP != dwOption) && (OPT_TA_MIRROR != dwOption)
                && (OPT_FORCE_SINGLEPROC != dwOption) && (OPT_NOGPU != dwOption) && (OPT_FIT_POWEROF2 != dwOption)
                && (OPT_SRGB != dwOption) && (OPT_SRGBI != dwOption) && (OPT_SRGBO != dwOption)
                && (OPT_HFLIP != dwOption) && (OPT_VFLIP != dwOption)
                && (OPT_COMPRESS_UNIFORM != dwOption) && (OPT_COMPRESS_MAX != dwOption) && (OPT_COMPRESS_DITHER != dwOption)
                && (OPT_DDS_DWORD_ALIGN != dwOption) && (OPT_USE_DX10 != dwOption) )
            {
                if(!*pValue)
                {
                    if((iArg + 1 >= argc))
                    {
					//	wprintf(L"Invalid option count passed to converter\n");
						failcount++;
						MessageOut(logfile, (msg + " Invalid option passed to converter"), false, true);
                        //PrintUsage();
                        return 1;
                    }

                    iArg++;
                    pValue = argv[iArg];
                }
            }

            switch(dwOption)
            {
            case OPT_WIDTH:
                if (swscanf_s(pValue, L"%Iu", &width) != 1)
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -w "), false, true);
                 //   wprintf( L"Invalid value specified with -w (%ls)\n", pValue);
                    wprintf( L"\n");
                   // PrintUsage();
                    return 1;
                }
                break;

            case OPT_HEIGHT:
                if (swscanf_s(pValue, L"%Iu", &height) != 1)
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -h "), false, true);
                 //   wprintf( L"Invalid value specified with -h (%ls)\n", pValue);
                    printf("\n");
                    //PrintUsage();
                    return 1;
                }
                break;

            case OPT_MIPLEVELS:
                if (swscanf_s(pValue, L"%Iu", &mipLevels) != 1)
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -m "), false, true);
                 //   wprintf( L"Invalid value specified with -m (%ls)\n", pValue);
                    wprintf( L"\n");
                    //PrintUsage();
                    return 1;
                }
                break;

            case OPT_FORMAT:
                format = (DXGI_FORMAT) LookupByName(pValue, g_pFormats);
                if ( !format )
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -f "), false, true);
                  //  wprintf( L"Invalid value specified with -f (%ls)\n", pValue);
                    wprintf( L"\n");
                    //PrintUsage();
                    return 1;
                }
                break;

            case OPT_FILTER:
                dwFilter = LookupByName(pValue, g_pFilters);
                if ( !dwFilter )
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -if "), false, true);
                  //  wprintf( L"Invalid value specified with -if (%ls)\n", pValue);
                    wprintf( L"\n");
                    //PrintUsage();
                    return 1;
                }
                break;

            case OPT_SRGBI:
                dwSRGB |= TEX_FILTER_SRGB_IN;
                break;

            case OPT_SRGBO:
                dwSRGB |= TEX_FILTER_SRGB_OUT;
                break;

            case OPT_SRGB:
                dwSRGB |= TEX_FILTER_SRGB;
                break;

            case OPT_SEPALPHA:
                dwFilterOpts |= TEX_FILTER_SEPARATE_ALPHA;
                break;

            case OPT_PREFIX:
                wcscpy_s(szPrefix, MAX_PATH, pValue);
                break;

            case OPT_SUFFIX:
                wcscpy_s(szSuffix, MAX_PATH, pValue);
                break;

            case OPT_OUTPUTDIR:
                wcscpy_s(szOutputDir, MAX_PATH, pValue);
                break;

            case OPT_FILETYPE:
                FileType = LookupByName(pValue, g_pSaveFileTypes);
                if ( !FileType )
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -ft "), false, true);
                  //  wprintf( L"Invalid value specified with -ft (%ls)\n", pValue);
                    wprintf( L"\n");
                    //PrintUsage();
                    return 1;
                }
                break;

            case OPT_TA_WRAP:
                if ( dwFilterOpts & TEX_FILTER_MIRROR )
                {
					failcount++;
					MessageOut(logfile, (msg + " Can't use -wrap and -mirror at same time"), false, true);
                  //  wprintf( L"Can't use -wrap and -mirror at same time\n\n");
                   // PrintUsage();
                    return 1;
                }
                dwFilterOpts |= TEX_FILTER_WRAP;
                break;

            case OPT_TA_MIRROR:
                if ( dwFilterOpts & TEX_FILTER_WRAP )
                {
					failcount++;
					MessageOut(logfile, (msg + " Can't use -wrap and -mirror at same time"), false, true);
                   // wprintf( L"Can't use -wrap and -mirror at same time\n\n");
                    //PrintUsage();
                    return 1;
                }
                dwFilterOpts |= TEX_FILTER_MIRROR;
                break;

            case OPT_NORMAL_MAP:
                {
                    dwNormalMap = 0;

                    if ( wcschr( pValue, L'l' ) )
                    {
                        dwNormalMap |= CNMAP_CHANNEL_LUMINANCE;
                    }
                    else if ( wcschr( pValue, L'r' ) )
                    {
                        dwNormalMap |= CNMAP_CHANNEL_RED;
                    }
                    else if ( wcschr( pValue, L'g' ) )
                    {
                        dwNormalMap |= CNMAP_CHANNEL_GREEN;
                    }
                    else if ( wcschr( pValue, L'b' ) )
                    {
                        dwNormalMap |= CNMAP_CHANNEL_BLUE;
                    }
                    else if ( wcschr( pValue, L'a' ) )
                    {
                        dwNormalMap |= CNMAP_CHANNEL_ALPHA;
                    }
                    else
                    {
						failcount++;
						MessageOut(logfile, (msg + " Invalid value specified for -nmap, missing l, r, g, b, or a"), false, true);
                      //  wprintf( L"Invalid value specified for -nmap (%ls), missing l, r, g, b, or a\n\n", pValue );
                       // PrintUsage();
                        return 1;                        
                    }

                    if ( wcschr( pValue, L'm' ) )
                    {
                        dwNormalMap |= CNMAP_MIRROR;
                    }
                    else
                    {
                        if ( wcschr( pValue, L'u' ) )
                        {
                            dwNormalMap |= CNMAP_MIRROR_U;
                        }
                        if ( wcschr( pValue, L'v' ) )
                        {
                            dwNormalMap |= CNMAP_MIRROR_V;
                        }
                    }

                    if ( wcschr( pValue, L'i' ) )
                    {
                        dwNormalMap |= CNMAP_INVERT_SIGN;
                    }

                    if ( wcschr( pValue, L'o' ) )
                    {
                        dwNormalMap |= CNMAP_COMPUTE_OCCLUSION;
                    }   
                }
                break;

            case OPT_NORMAL_MAP_AMPLITUDE:
                if ( !dwNormalMap )
                {
					failcount++;
					MessageOut(logfile, (msg + " -nmapamp requires -nmap"), false, true);
                   // wprintf( L"-nmapamp requires -nmap\n\n" );
                   // PrintUsage();
                    return 1;
                }
                else if (swscanf_s(pValue, L"%f", &nmapAmplitude) != 1)
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -nmapamp"), false, true);
                 //   wprintf( L"Invalid value specified with -nmapamp (%ls)\n\n", pValue);
                   // PrintUsage();
                    return 1;
                }
                else if ( nmapAmplitude < 0.f )
                {
					failcount++;
					MessageOut(logfile, (msg + " Normal map amplitude must be positive"), false, true);
                //    wprintf( L"Normal map amplitude must be positive (%ls)\n\n", pValue);
                  //  PrintUsage();
                    return 1;
                }
                break;

            case OPT_FEATURE_LEVEL:
                maxSize = LookupByName( pValue, g_pFeatureLevels );
                if ( !maxSize )
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -fl"), false, true);
                  //  wprintf( L"Invalid value specified with -fl (%ls)\n", pValue);
                    wprintf( L"\n");
                   // PrintUsage();
                    return 1;
                }
                break;

            case OPT_ALPHA_WEIGHT:
                if (swscanf_s(pValue, L"%f", &alphaWeight) != 1)
                {
					failcount++;
					MessageOut(logfile, (msg + " Invalid value specified with -aw "), false, true);
                  //  wprintf( L"Invalid value specified with -aw (%ls)\n", pValue);
                    wprintf( L"\n");
                  //  PrintUsage();
                    return 1;
                }
                else if ( alphaWeight < 0.f )
                {
					failcount++;
					MessageOut(logfile, (msg + " -aw parameter must be positive "), false, true);
                  //  wprintf( L"-aw (%ls) parameter must be positive\n", pValue);
                    wprintf( L"\n");
                    return 1;
                }
                break;

            case OPT_COMPRESS_UNIFORM:
                dwCompress |= TEX_COMPRESS_UNIFORM;
                break;

            case OPT_COMPRESS_MAX:
                dwCompress |= TEX_COMPRESS_BC7_USE_3SUBSETS;
                break;

            case OPT_COMPRESS_DITHER:
                dwCompress |= TEX_COMPRESS_DITHER;
                break;

			}
        }
        else
        {
            SConversion conv;
            wcscpy_s(conv.szSrc, MAX_PATH, pArg);

            conv.szDest[0] = 0;

            conversion.push_back(conv);
        }
    }

    if(conversion.empty())
    {
		failcount++;
		MessageOut(logfile, (msg + " No files passed to converter "), false, true);
		//wprintf(L"No files passed to converter\n");
       // PrintUsage();
        return 0;
    }

    //if(~dwOptions & (DWORD64(1) << OPT_NOLOGO))
    //    PrintLogo();

    // Work out out filename prefix and suffix
    if(szOutputDir[0] && (L'\\' != szOutputDir[wcslen(szOutputDir) - 1]))
        wcscat_s( szOutputDir, MAX_PATH, L"\\" );

    if(szPrefix[0])
        wcscat_s(szOutputDir, MAX_PATH, szPrefix);

    wcscpy_s(szPrefix, MAX_PATH, szOutputDir);

    const WCHAR* fileTypeName = LookupByValue(FileType, g_pSaveFileTypes);

    if (fileTypeName)
    {
        wcscat_s(szSuffix, MAX_PATH, L".");
        wcscat_s(szSuffix, MAX_PATH, fileTypeName);
    }
    else
    {
        wcscat_s(szSuffix, MAX_PATH, L".unknown");
    }

    if (FileType != CODEC_DDS)
    {
        mipLevels = 1;
    }

    LARGE_INTEGER qpcFreq;
    if ( !QueryPerformanceFrequency( &qpcFreq ) )
    {
        qpcFreq.QuadPart = 0;
    }


    LARGE_INTEGER qpcStart;
    if ( !QueryPerformanceCounter( &qpcStart ) )
    {
        qpcStart.QuadPart = 0;
    }

    // Convert images
    bool nonpow2warn = false;
    bool non4bc = false;
    ComPtr<ID3D11Device> pDevice;

    for( auto pConv = conversion.begin(); pConv != conversion.end(); ++pConv )
    {
		if (pConv != conversion.begin())
		{
			//wprintf( L"\n");
			LogMessage(logfile, msg + "\n", false);
		}

        // Load source image
		ATL::CW2A lpSrc(pConv->szSrc);
		string src = lpSrc;
		MessageOut(logfile, ("\n\t\treading " + src + "\n\t\t"), false, false);
        //wprintf( L" reading %ls", pConv->szSrc );
        fflush(stdout);

        WCHAR ext[_MAX_EXT];
        WCHAR fname[_MAX_FNAME];
        _wsplitpath_s( pConv->szSrc, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT );

        TexMetadata info;
        std::unique_ptr<ScratchImage> image( new (std::nothrow) ScratchImage );

        if ( !image )
        {
			failcount++;
			MessageOut(logfile, (msg + " ERROR: Memory allocation failed (ScratchImage image)."), false, true);
            //wprintf( L" ERROR: Memory allocation failed\n" );
            return 1;
        }

        if ( _wcsicmp( ext, L".dds" ) == 0 )
        {
            DWORD ddsFlags = DDS_FLAGS_NONE;
            if ( dwOptions & (DWORD64(1) << OPT_DDS_DWORD_ALIGN) )
                ddsFlags |= DDS_FLAGS_LEGACY_DWORD;
            if ( dwOptions & (DWORD64(1) << OPT_EXPAND_LUMINANCE) )
                ddsFlags |= DDS_FLAGS_EXPAND_LUMINANCE;

            hr = LoadFromDDSFile( pConv->szSrc, ddsFlags, &info, *image );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED to load DDS image from file." + to_string(hr)), false, true);
              //  wprintf( L" FAILED (%x)\n", hr);
				
                continue;
            }

            if ( IsTypeless( info.format ) )
            {
                if ( dwOptions & (DWORD64(1) << OPT_TYPELESS_UNORM) )
                {
                    info.format = MakeTypelessUNORM( info.format );
                }
                else if ( dwOptions & (DWORD64(1) << OPT_TYPELESS_FLOAT) )
                {
                    info.format = MakeTypelessFLOAT( info.format );
                }

                if ( IsTypeless( info.format ) )
                {
					failcount++;
					MessageOut(logfile, (msg + " FAILED due to Typeless format." + to_string(hr)), false, true);
                   // wprintf( L" FAILED due to Typeless format %d\n", info.format );
                    continue;
                }

                image->OverrideFormat( info.format );
            }
        }
        else if ( _wcsicmp( ext, L".tga" ) == 0 )
        {
            hr = LoadFromTGAFile( pConv->szSrc, &info, *image );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED to load TGA image from file." + to_string(hr)), false, true);
               // wprintf( L" FAILED (%x)\n", hr);
                continue;
            }
        }
        else
        {
            // WIC shares the same filter values for mode and dither
            static_assert( WIC_FLAGS_DITHER == TEX_FILTER_DITHER, "WIC_FLAGS_* & TEX_FILTER_* should match" );
            static_assert( WIC_FLAGS_DITHER_DIFFUSION == TEX_FILTER_DITHER_DIFFUSION, "WIC_FLAGS_* & TEX_FILTER_* should match"  );
            static_assert( WIC_FLAGS_FILTER_POINT == TEX_FILTER_POINT, "WIC_FLAGS_* & TEX_FILTER_* should match"  );
            static_assert( WIC_FLAGS_FILTER_LINEAR == TEX_FILTER_LINEAR, "WIC_FLAGS_* & TEX_FILTER_* should match"  );
            static_assert( WIC_FLAGS_FILTER_CUBIC == TEX_FILTER_CUBIC, "WIC_FLAGS_* & TEX_FILTER_* should match"  );
            static_assert( WIC_FLAGS_FILTER_FANT == TEX_FILTER_FANT, "WIC_FLAGS_* & TEX_FILTER_* should match"  );

            DWORD wicFlags = dwFilter;
            if (FileType == CODEC_DDS)
                wicFlags |= WIC_FLAGS_ALL_FRAMES;

            hr = LoadFromWICFile( pConv->szSrc, wicFlags, &info, *image );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " Windows Imaging Component FAILED to load image from file. " + to_string(hr)), false, true);
              //  wprintf( L" FAILED (%x)\n", hr);
                continue;
            }
        }

        PrintInfo( info , logfile );

        size_t tMips = ( !mipLevels && info.mipLevels > 1 ) ? info.mipLevels : mipLevels;

        bool sizewarn = false;

        size_t twidth = ( !width ) ? info.width : width;
        if ( twidth > maxSize )
        {
            if ( !width )
                twidth = maxSize;
            else
                sizewarn = true;
        }

        size_t theight = ( !height ) ? info.height : height;
        if ( theight > maxSize )
        {
            if ( !height )
                theight = maxSize;
            else
                sizewarn = true;
        }

        if ( sizewarn )
        {
			failcount++;
			MessageOut(logfile, (msg + " WARNING: Target size exceeds maximum size for feature level."), false, true);
           // wprintf( L"\nWARNING: Target size exceeds maximum size for feature level (%u)\n", maxSize );
        }

        if (dwOptions & (DWORD64(1) << OPT_FIT_POWEROF2))
        {
            FitPowerOf2( info.width, info.height, twidth, theight, maxSize );
        }

        // Convert texture
		MessageOut(logfile, (msg + "as"), true, false);
        //wprintf( L" as");
        fflush(stdout);

        // --- Planar ------------------------------------------------------------------
        if ( IsPlanar( info.format ) )
        {
            auto img = image->GetImage(0,0,0);
            assert( img );
            size_t nimg = image->GetImageCount();

            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (ScratchImage timage)."), false, true);
                //wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            hr = ConvertToSinglePlane( img, nimg, info, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [converttosingeplane]" + to_string(hr)), false, true);
               // wprintf( L" FAILED [converttosingeplane] (%x)\n", hr);
                continue;
            }

            auto& tinfo = timage->GetMetadata();

            info.format = tinfo.format;

            assert( info.width == tinfo.width );
            assert( info.height == tinfo.height );
            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.mipLevels == tinfo.mipLevels );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.dimension == tinfo.dimension );

            image.swap( timage );
        }

        DXGI_FORMAT tformat = ( format == DXGI_FORMAT_UNKNOWN ) ? info.format : format;

        // --- Decompress --------------------------------------------------------------
        std::unique_ptr<ScratchImage> cimage;
        if ( IsCompressed( info.format ) )
        {
            auto img = image->GetImage(0,0,0);
            assert( img );
            size_t nimg = image->GetImageCount();

            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (Decompress ScratchImage timage)."), false, true);
                //wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            hr = Decompress( img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [decompress] " + to_string(hr)), false, true);
             //   wprintf( L" FAILED [decompress] (%x)\n", hr);
                continue;
            }

            auto& tinfo = timage->GetMetadata();

            info.format = tinfo.format;

            assert( info.width == tinfo.width );
            assert( info.height == tinfo.height );
            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.mipLevels == tinfo.mipLevels );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.dimension == tinfo.dimension );

            if ( FileType == CODEC_DDS )
            {
                // Keep the original compressed image in case we can reuse it
                cimage.reset( image.release() );
                image.reset( timage.release() );
            }
            else
            {
                image.swap( timage );
            }
        }

        // --- Flip/Rotate -------------------------------------------------------------
        if ( dwOptions & ( (DWORD64(1) << OPT_HFLIP) | (DWORD64(1) << OPT_VFLIP) ) )
        {
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (FlipRotate ScratchImage timage)."), false, true);
              //  wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            DWORD dwFlags = 0;

            if ( dwOptions & (DWORD64(1) << OPT_HFLIP) )
                dwFlags |= TEX_FR_FLIP_HORIZONTAL;

            if ( dwOptions & (DWORD64(1) << OPT_VFLIP) )
                dwFlags |= TEX_FR_FLIP_VERTICAL;

            assert( dwFlags != 0 );

            hr = FlipRotate( image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFlags, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [fliprotate] " + to_string(hr)), false, true);
               // wprintf( L" FAILED [fliprotate] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert( tinfo.width == twidth && tinfo.height == theight );

            info.width = tinfo.width;
            info.height = tinfo.height;

            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.mipLevels == tinfo.mipLevels );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.format == tinfo.format );
            assert( info.dimension == tinfo.dimension );

            image.swap( timage );
            cimage.reset();
        }

        // --- Resize ------------------------------------------------------------------
        if ( info.width != twidth || info.height != theight )
        {
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (Resize ScratchImage timage)."), false, true);
                //wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            hr = Resize( image->GetImages(), image->GetImageCount(), image->GetMetadata(), twidth, theight, dwFilter | dwFilterOpts, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [resize] " + to_string(hr)), false, true);
              //  wprintf( L" FAILED [resize] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert( tinfo.width == twidth && tinfo.height == theight && tinfo.mipLevels == 1 );
            info.width = tinfo.width;
            info.height = tinfo.height;
            info.mipLevels = 1;

            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.format == tinfo.format );
            assert( info.dimension == tinfo.dimension );

            image.swap( timage );
            cimage.reset();
        }

        // --- Convert -----------------------------------------------------------------
        if ( dwOptions & (DWORD64(1) << OPT_NORMAL_MAP) )
        {
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (Convert ScratchImage timage(1))."), false, true);
               // wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            DXGI_FORMAT nmfmt = tformat;
            if ( IsCompressed( tformat ) )
            {
                nmfmt = (dwNormalMap & CNMAP_COMPUTE_OCCLUSION) ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R32G32B32_FLOAT;
            }

            hr = ComputeNormalMap( image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwNormalMap, nmapAmplitude, nmfmt, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [normalmap] " + to_string(hr)), false, true);
              //  wprintf( L" FAILED [normalmap] (%x)\n", hr);
                return 1;
            }        

			auto& tinfo = timage->GetMetadata();

			assert(tinfo.format == nmfmt);
			info.format = tinfo.format;

			assert(info.width == tinfo.width);
			assert(info.height == tinfo.height);
			assert(info.depth == tinfo.depth);
			assert(info.arraySize == tinfo.arraySize);
			assert(info.mipLevels == tinfo.mipLevels);
			assert(info.miscFlags == tinfo.miscFlags);
			assert(info.miscFlags2 == tinfo.miscFlags2);
			assert(info.dimension == tinfo.dimension);


            image.swap( timage );
            cimage.reset();
        }
        else if ( info.format != tformat && !IsCompressed( tformat ) )
        {
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (Convert ScratchImage timage(2))."), false, true);
               // wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            hr = Convert( image->GetImages(), image->GetImageCount(), image->GetMetadata(), tformat, dwFilter | dwFilterOpts | dwSRGB, 0.5f, *timage );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [convert] " + to_string(hr)),true , true);
              //  wprintf( L" FAILED [convert] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();

            assert( tinfo.format == tformat );
            info.format = tinfo.format;

            assert( info.width == tinfo.width );
            assert( info.height == tinfo.height );
            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.mipLevels == tinfo.mipLevels );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.dimension == tinfo.dimension );

            image.swap( timage );
            cimage.reset();
        }

        // --- Generate mips -----------------------------------------------------------
        if ( !ispow2(info.width) || !ispow2(info.height) || !ispow2(info.depth) )
        {
            if ( info.dimension == TEX_DIMENSION_TEXTURE3D )
            {
                if ( !tMips )
                {
                    tMips = 1;
                }
                else
                {
					failcount++;
					MessageOut(logfile, (msg + " ERROR: Cannot generate mips for non-power-of-2 volume textures."), false, true);
                  //  wprintf( L" ERROR: Cannot generate mips for non-power-of-2 volume textures\n" );
                    return 1;
                }
            }
            else if ( !tMips || info.mipLevels != 1 )
            {
                nonpow2warn = true;
            }
        }

        if ( (!tMips || info.mipLevels != tMips) && ( info.mipLevels != 1 ) )
        {
            // Mips generation only works on a single base image, so strip off existing mip levels
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (mips(1) ScratchImage)."), false, true);
               // wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            TexMetadata mdata = info;
            mdata.mipLevels = 1;
            hr = timage->Initialize( mdata );
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [copy to single level 1] " + to_string(hr)), true, true);
             //   wprintf( L" FAILED [copy to single level] (%x)\n", hr);
                return 1;
            }

            if ( info.dimension == TEX_DIMENSION_TEXTURE3D )
            {
                for( size_t d = 0; d < info.depth; ++d )
                {
                    hr = CopyRectangle( *image->GetImage( 0, 0, d ), Rect( 0, 0, info.width, info.height ),
                                        *timage->GetImage( 0, 0, d ), TEX_FILTER_DEFAULT, 0, 0 );
                    if ( FAILED(hr) )
                    {
						failcount++;
						MessageOut(logfile, (msg + " FAILED [copy to single level 2] " + to_string(hr)), true, true);
                       // wprintf( L" FAILED [copy to single level] (%x)\n", hr);
                        return 1;
                    }
                }
            }
            else
            {
                for( size_t i = 0; i < info.arraySize; ++i )
                {
                    hr = CopyRectangle( *image->GetImage( 0, i, 0 ), Rect( 0, 0, info.width, info.height ),
                                        *timage->GetImage( 0, i, 0 ), TEX_FILTER_DEFAULT, 0, 0 );
                    if ( FAILED(hr) )
                    {
						failcount++;
						MessageOut(logfile, (msg + " FAILED [copy to single level 3] " + to_string(hr)), true, true);
                        //wprintf( L" FAILED [copy to single level] (%x)\n", hr);
                        return 1;
                    }
                }
            }

            image.swap( timage );
            info.mipLevels = image->GetMetadata().mipLevels;

            if ( cimage && ( tMips == 1 ) )
            {
                // Special case for trimming mips off compressed images and keeping the original compressed highest level mip
                mdata = cimage->GetMetadata();
                mdata.mipLevels = 1;
                hr = timage->Initialize( mdata );
                if ( FAILED(hr) )
                {
					failcount++;
					MessageOut(logfile, (msg + " FAILED [copy compressed to single level] " + to_string(hr)), true, true);
                   // wprintf( L" FAILED [copy compressed to single level] (%x)\n", hr);
                    return 1;
                }

                if ( mdata.dimension == TEX_DIMENSION_TEXTURE3D )
                {
                    for( size_t d = 0; d < mdata.depth; ++d )
                    {
                        auto simg = cimage->GetImage( 0, 0, d );
                        auto dimg = timage->GetImage( 0, 0, d );

                        memcpy_s( dimg->pixels, dimg->slicePitch, simg->pixels, simg->slicePitch );
                    }
                }
                else
                {
                    for( size_t i = 0; i < mdata.arraySize; ++i )
                    {
                        auto simg = cimage->GetImage( 0, i, 0 );
                        auto dimg = timage->GetImage( 0, i, 0 );

                        memcpy_s( dimg->pixels, dimg->slicePitch, simg->pixels, simg->slicePitch );
                    }
                }

                cimage.swap( timage );
            }
            else
            {
                cimage.reset();
            }
        }

        if ( ( !tMips || info.mipLevels != tMips ) && ( info.width > 1 || info.height > 1 || info.depth > 1 ) )
        {
            std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
            if ( !timage )
            {
				failcount++;
				MessageOut(logfile, (msg + " ERROR: Memory allocation failed (mips(2) ScratchImage)."), false, true);
               // wprintf( L" ERROR: Memory allocation failed\n" );
                return 1;
            }

            if ( info.dimension == TEX_DIMENSION_TEXTURE3D )
            {
                hr = GenerateMipMaps3D( image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFilter | dwFilterOpts, tMips, *timage );
            }
            else
            {
                hr = GenerateMipMaps( image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFilter | dwFilterOpts, tMips, *timage );
            }
            if ( FAILED(hr) )
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED [mipmaps] " + to_string(hr)), true, true);
               // wprintf( L" FAILED [mipmaps] (%x)\n", hr);
                return 1;
            }

            auto& tinfo = timage->GetMetadata();
            info.mipLevels = tinfo.mipLevels;

            assert( info.width == tinfo.width );
            assert( info.height == tinfo.height );
            assert( info.depth == tinfo.depth );
            assert( info.arraySize == tinfo.arraySize );
            assert( info.mipLevels == tinfo.mipLevels );
            assert( info.miscFlags == tinfo.miscFlags );
            assert( info.miscFlags2 == tinfo.miscFlags2 );
            assert( info.dimension == tinfo.dimension );

            image.swap( timage );
            cimage.reset();
        }

        // --- Premultiplied alpha (if requested) --------------------------------------
        if ( ( dwOptions & (DWORD64(1) << OPT_PREMUL_ALPHA) )
             && HasAlpha( info.format )
             && info.format != DXGI_FORMAT_A8_UNORM )
        {
            if ( info.IsPMAlpha() )
            {
                printf("WARNING: Image is already using premultiplied alpha\n");
            }
            else
            {
                auto img = image->GetImage(0,0,0);
                assert( img );
                size_t nimg = image->GetImageCount();

                std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
                if ( !timage )
                {
					failcount++;
					MessageOut(logfile, (msg + " ERROR: Memory allocation failed (PMalpha ScratchImage)."), false, true);
                   // wprintf( L" ERROR: Memory allocation failed\n" );
                    return 1;
                }

                hr = PremultiplyAlpha( img, nimg, info, dwSRGB, *timage );
                if ( FAILED(hr) )
                {
					failcount++;
					MessageOut(logfile, (msg + " FAILED [premultiply alpha] " + to_string(hr)), true, true);
                  //  wprintf( L" FAILED [premultiply alpha] (%x)\n", hr);
                    continue;
                }

                auto& tinfo = timage->GetMetadata();
                info.miscFlags2 = tinfo.miscFlags2;
 
                assert( info.width == tinfo.width );
                assert( info.height == tinfo.height );
                assert( info.depth == tinfo.depth );
                assert( info.arraySize == tinfo.arraySize );
                assert( info.mipLevels == tinfo.mipLevels );
                assert( info.miscFlags == tinfo.miscFlags );
                assert( info.miscFlags2 == tinfo.miscFlags2 );
                assert( info.dimension == tinfo.dimension );

                image.swap( timage );
                cimage.reset();
            }
        }

        // --- Compress ----------------------------------------------------------------
        if ( IsCompressed( tformat ) && (FileType == CODEC_DDS) )
        {
            if ( cimage && ( cimage->GetMetadata().format == tformat ) )
            {
                // We never changed the image and it was already compressed in our desired format, use original data
                image.reset( cimage.release() );

                auto& tinfo = image->GetMetadata();

                if ( (tinfo.width % 4) != 0 || (tinfo.height % 4) != 0 )
                { 
                    non4bc = true;
                }

                info.format = tinfo.format;
                assert( info.width == tinfo.width );
                assert( info.height == tinfo.height );
                assert( info.depth == tinfo.depth );
                assert( info.arraySize == tinfo.arraySize );
                assert( info.mipLevels == tinfo.mipLevels );
                assert( info.miscFlags == tinfo.miscFlags );
                assert( info.miscFlags2 == tinfo.miscFlags2 );
                assert( info.dimension == tinfo.dimension );
            }
            else
            {
                cimage.reset();

                auto img = image->GetImage(0,0,0);
                assert( img );
                size_t nimg = image->GetImageCount();

                std::unique_ptr<ScratchImage> timage( new (std::nothrow) ScratchImage );
                if ( !timage )
                {
					failcount++;
					MessageOut(logfile, (msg + " ERROR: Memory allocation failed (Compress ScratchImage)."), false, true);
                   // wprintf( L" ERROR: Memory allocation failed\n" );
                    return 1;
                }

                bool bc6hbc7=false;
                switch( tformat )
                {
                case DXGI_FORMAT_BC6H_TYPELESS:
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC7_TYPELESS:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    bc6hbc7=true;

                    {
                        static bool s_tryonce = false;

                        if ( !s_tryonce )
                        {
                            s_tryonce = true;

                            if ( !(dwOptions & (DWORD64(1) << OPT_NOGPU) ) )
                            {
                                if ( !CreateDevice( pDevice.GetAddressOf() ) )
                                    wprintf( L"\nWARNING: DirectCompute is not available, using BC6H / BC7 CPU codec\n" );
                            }
                            else
                            {
                                wprintf( L"\nWARNING: using BC6H / BC7 CPU codec\n" );
                            }
                        }
                    }
                    break;
                }

                DWORD cflags = dwCompress;
#ifdef _OPENMP
                if ( !(dwOptions & (DWORD64(1) << OPT_FORCE_SINGLEPROC) ) )
                {
                    cflags |= TEX_COMPRESS_PARALLEL;
                }
#endif

                if ( (img->width % 4) != 0 || (img->height % 4) != 0 )
                { 
                    non4bc = true;
                }

                if ( bc6hbc7 && pDevice )
                {
                    hr = Compress( pDevice.Get(), img, nimg, info, tformat, dwCompress | dwSRGB, alphaWeight, *timage );
                }
                else
                {
                    hr = Compress( img, nimg, info, tformat, cflags | dwSRGB, 0.5f, *timage );
                }
                if ( FAILED(hr) )
                {
					failcount++;
					MessageOut(logfile, (msg + " FAILED [compress] " + to_string(hr)), true, true);
                   // wprintf( L" FAILED [compress] (%x)\n", hr);
                    continue;
                }

                auto& tinfo = timage->GetMetadata();

                info.format = tinfo.format;
                assert( info.width == tinfo.width );
                assert( info.height == tinfo.height );
                assert( info.depth == tinfo.depth );
                assert( info.arraySize == tinfo.arraySize );
                assert( info.mipLevels == tinfo.mipLevels );
                assert( info.miscFlags == tinfo.miscFlags );
                assert( info.miscFlags2 == tinfo.miscFlags2 );
                assert( info.dimension == tinfo.dimension );

                image.swap( timage );
            }
        }
        else
        {
            cimage.reset();
        }

        // --- Set alpha mode ----------------------------------------------------------
        if ( HasAlpha( info.format )
             && info.format != DXGI_FORMAT_A8_UNORM )
        {
            if ( image->IsAlphaAllOpaque() )
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_OPAQUE);
            }
            else if ( info.IsPMAlpha() )
            {
                // Aleady set TEX_ALPHA_MODE_PREMULTIPLIED
            }
            else if ( dwOptions & (DWORD64(1) << OPT_SEPALPHA) )
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_CUSTOM);
            }
            else
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_STRAIGHT);
            }
        }
        else
        {
            info.miscFlags2 &= ~TEX_MISC2_ALPHA_MODE_MASK;
        }

        // --- Save result -------------------------------------------------------------
        {
            auto img = image->GetImage(0,0,0);
            assert( img );
            size_t nimg = image->GetImageCount();

            PrintInfo( info , logfile);
           // wprintf( L"\n");

            // Figure out dest filename
            WCHAR *pchSlash, *pchDot;

            wcscpy_s(pConv->szDest, MAX_PATH, szPrefix);

            pchSlash = wcsrchr(pConv->szSrc, L'\\');
            if(pchSlash != 0)
                wcscat_s(pConv->szDest, MAX_PATH, pchSlash + 1);
            else
                wcscat_s(pConv->szDest, MAX_PATH, pConv->szSrc);

            pchSlash = wcsrchr(pConv->szDest, '\\');
            pchDot = wcsrchr(pConv->szDest, '.');

            if(pchDot > pchSlash)
                *pchDot = 0;

            wcscat_s(pConv->szDest, MAX_PATH, szSuffix);

            // Write texture
			ATL::CW2A lpDst(pConv->szDest);
			string dst = lpDst;
			MessageOut(logfile, ("\n\t\twriting " + dst + "..."), false, false);
          //  wprintf( L" writing %ls", pConv->szDest);
            fflush(stdout);

            switch( FileType )
            {
            case CODEC_DDS:
                hr = SaveToDDSFile( img, nimg, info,
                                    (dwOptions & (DWORD64(1) << OPT_USE_DX10) ) ? (DDS_FLAGS_FORCE_DX10_EXT|DDS_FLAGS_FORCE_DX10_EXT_MISC2) : DDS_FLAGS_NONE, 
                                    pConv->szDest );
                break;

            case CODEC_TGA:
                hr = SaveToTGAFile( img[0], pConv->szDest );
                break;

            default:
                hr = SaveToWICFile( img, nimg, WIC_FLAGS_ALL_FRAMES, GetWICCodec( static_cast<WICCodecs>(FileType) ), pConv->szDest );
                break;
            }

            if(FAILED(hr))
            {
				failcount++;
				MessageOut(logfile, (msg + " FAILED image save " + to_string(hr)), true, true);
               // wprintf( L" FAILED (%x)\n", hr);
                continue;
            }
			msg = "OK.";
			MessageOut(logfile, msg, true, false);
            //wprintf( L"\n");
        }
    }

    if ( nonpow2warn )
        wprintf( L"\n WARNING: Not all feature levels support non-power-of-2 textures with mipmaps\n" );

    if ( non4bc )
        wprintf( L"\n WARNING: Direct3D requires BC image to be multiple of 4 in width & height\n" );

    if(dwOptions & (DWORD64(1) << OPT_TIMING))
    {
        LARGE_INTEGER qpcEnd;
        if ( QueryPerformanceCounter( &qpcEnd ) )
        {
            LONGLONG delta = qpcEnd.QuadPart - qpcStart.QuadPart;
            wprintf( L"\n Processing time: %f seconds\n", double(delta) / double(qpcFreq.QuadPart) );
        }
    }

    return 0;
}
#pragma endregion

#pragma region UTILITIES

bool UserWantsToExit()
{
	// User exit.
	char input;
	if (GetAsyncKeyState(VK_ESCAPE))
	{
		std::cout << "\n *** Do you wish to stop conversion? [ y / n ]: " << std::endl;
		std::cin >> input;
		if (input == 'Y' || input == 'y') { return true; }
	}
	return false;
}

// Return path of current working directory
string CurrentDir()
{
	WCHAR Buffer[MAX_PATH];
	DWORD dwRet;

	dwRet = GetCurrentDirectory(MAX_PATH, Buffer);

	if (dwRet == 0)
	{
		printf(" GetCurrentDirectory failed (%d)\n", GetLastError());
		return 0;
	}
	if (dwRet > MAX_PATH)
	{
		printf(" Path length exceeds maximum length of %d characters \n", MAX_PATH);
		std::cout << " ERROR FILE PATH LENGTH TOO LONG" << std::endl;
		return 0;
	}

	CW2A pszBfr(Buffer);
	string sDir(pszBfr);
	return sDir;
}

// Recursive file search
void BuildFileList(string wrkdir, std::list<string> &list, string &logfile, bool isRecursive, int &count)
{
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	
	if (!wrkdir.empty() && (wrkdir[wrkdir.length() - 1] != L'\\'))
	{
		wrkdir = wrkdir + "\\";
	}

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	if (wrkdir.length() > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\n Directory path is too long.\n"));
		MessageOut(logfile, "\n FILE SEARCH ERROR: Directory path is too long - " + wrkdir + "\n", false, true);
		return;
	}
	CA2CT lptDir(wrkdir.c_str());
	
	//std::cout << "Target directory: " << wrkdir << std::endl;

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	
	StringCchCopy(szDir, MAX_PATH, lptDir);

	// Find the first file in the directory.
	wstring wsff = szDir;
	wsff += L"*";
	hFind = FindFirstFile(wsff.c_str(), &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		_tprintf(TEXT("\n FindFirstFile() returned an error.\n"));
		MessageOut(logfile, "\n FILE SEARCH ERROR: FindFirstFile() returned an error in: " + wrkdir + "\n", false, true);
		return;
	}

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ((wcscmp(ffd.cFileName, L".") != 0) &&
				(wcscmp(ffd.cFileName, L"..") != 0))
			{	
				ATL::CW2A psName(ffd.cFileName);
				ATL::CW2A psDir(szDir);
				string sDir = psDir;
				string sName = psName;
				if (isRecursive)
				{
					BuildFileList((sDir + sName), list, logfile, true, count);
				}
			}
		}
		else if ((ffd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0)
		{
			ATL::CW2A psName(ffd.cFileName);
			ATL::CW2A psDir(szDir);
			string sDir = psDir;
			string sName = psName;
			char back0 = sName[(sName.length() - 1)];
			char back1 = sName[(sName.length() - 2)];
			char back2 = sName[(sName.length() - 3)];

			if ((sName.find(".dds.0") != string::npos) || (((back0 == 's') && (back1 == 'd') && (back2) == 'd')) && (back0 != 'a'))
			{	
				if (!(sName.find("_cm") != string::npos)) // Ignore cubemaps
				{					
					list.push_back(sDir + sName);
					
					string number = to_string(count);
					for (size_t i = 0; i <= number.length(); i++)	{ std::cout << '\b'; }
					count++;
					std::cout << count << std::ends;
				}
			}
		}
		if (UserWantsToExit()) { exit(EXIT_SUCCESS); }

	} while (FindNextFile(hFind, &ffd));
		
	FindClose(hFind);
	return;
}
#pragma endregion

////////////////////////////////////////////////////////////////////////////////////////
//
//   UNSPLIT OPERATIONS
//
////////////////////////////////////////////////////////////////////////////////////////
void GetFileData(SUnsplitFileNameInfo &info, SUFileData &data, string &logfile)
{
	char* buffer	  = new char();
	string sMagic	  = "DDS ";
	string mTestDDS	  = "";
	int counter		  = 0;
	int offset		  = 0;		// position adjustment if "DDS " magic word is missing.
	bool modify		  = false;

	data.fourcc		  = "";
	data.bIsNormal	  = false;
	data.bIsGloss	  = false;
	data.bIsDX10	  = false;
	data.bIsMaskAlpha = false;

	string infilepath = info.sDirectory + info.sNameAllExt;

	// Create fstream objects
	ifstream fin(infilepath, ios::in | ios::binary);
	if (!fin)
	{
		MessageOut(logfile, " FAILED: GetFileData() could not open input file: " + infilepath, false, true);
		return;
	}

	// Read data
	while (fin.read((char*)buffer, 1).good()) {
		data.finData.push_back(*buffer);
	}
	fin.close();
	
	// Check file name for file type
	if (info.sName.find("_ddn") != string::npos || info.sName.find("_norm") != string::npos)
	{
		data.bIsNormal = true;
	}

	if (!(data.bIsGloss) && (FileExists(info.sDirectory + info.sBaseName + ".a")
		|| FileExists(info.sDirectory + info.sBaseName + ".0a")))
	{
		info.hasGloss = true;
	}

	if (info.sName.find(GLOSS_KEY) != string::npos || info.sExt2 == ".a" || info.sExt2 == ".0a")
	{
		data.bIsNormal = false;
		data.bIsGloss = true;

		/*if (info.sName.find("_mask") != string::npos || info.sName.find("_opa") != string::npos
			|| info.sName.find("_trans") != string::npos)*/
		if (!data.bIsNormal)
		{
			data.bIsMaskAlpha = true;
		}
	}
		
	// Check for Magic Word "DDS " at beginning of file. 
	// No magic word means the file is CIG a custom file e.g. Gloss map or MaskAlpha

	for (const auto &c : data.finData) {
		if (counter > 3) { break; }
		mTestDDS += c;
		counter++;
	}
	if (mTestDDS != sMagic) {
		for (size_t i = 0; i < sMagic.length(); i++)
			data.headerDDS[i] = sMagic[i];

		offset = 4;
		data.bIsGloss = true;
	}

	data.fourcc += (data.finData[84 - offset]);
	data.fourcc += (data.finData[85 - offset]);
	data.fourcc += (data.finData[86 - offset]);
	data.fourcc += (data.finData[87 - offset]);

	// Check for 'DX10' FourCC header code at bytes 84-87.
	if (data.fourcc == "DX10")
	{
		data.bIsDX10 = true;

		data.dxgi = (DXGI_FORMAT)(data.finData[128 - offset]);

		switch (data.dxgi)
		{
			case DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT_BC5_UNORM:
			{
				data.bIsNormal = true;
				break;
			}
			default:
				break;
		}
		
		// Mask alpha files with extended DX10 header are modified to be std DDS with fourcc = 'ATI1'.
		if (data.bIsMaskAlpha)
		{
			data.mipPos = HEADER_SIZE_DDS - offset;
			modify = true;	
		}
		else
		{
			// Now read in DXT10 Extended Header data (20 bytes)
			for (int i = 0; i < HEADER_SIZE_DX10; i++)
			{
				data.headerDDS_Ext[i] = data.finData[i + HEADER_SIZE_DDS - offset];
			}

			data.mipPos = HEADER_SIZE_DDS + HEADER_SIZE_DX10 - offset;
		}
	}
	else
	{
		data.mipPos = HEADER_SIZE_DDS - offset;
	}

	// Get DDS header data (124 or 128 bytes)
	for (int i = 0; i < HEADER_SIZE_DDS - offset; i++)
	{
		data.headerDDS[i + offset] = data.finData[i];
	}

	if (modify)
	{
		data.bIsDX10 = false;

		data.headerDDS[84] = 'A';
		data.headerDDS[85] = 'T';
		data.headerDDS[86] = 'I';
		data.headerDDS[87] = '1';
	}

	// Early CIG files used ATI1 & ATI2 format codes to identify normal & gloss map file pairs.
	if (data.fourcc == "ATI1")
	{
		data.bIsGloss = true;
	}

	if (data.fourcc == "ATI2")
	{
		data.bIsNormal = true;
	}
}

bool ReassembleDDS(SUnsplitFileNameInfo &info, SUFileData &data, SUnsplitOptions &options, std::string &logfile){
		
	// Store mipmap zero data.
	for (size_t i = 0; i < (data.finData.size() - (size_t)data.mipPos); i++) {
		data.mipmapZero.push_back(data.finData.at(i + data.mipPos));
	}

#pragma region DEBUG_PRINT_DATA
	
	/*std::cout << " FILE :" << Name.sNameAllExt << std::endl;
	
	std::cout << '\n';
	std::cout << " HEADER_DDS[" << sizeof(data.headerDDS) << "]:" ;
	for (size_t i = 0; i < sizeof(data.headerDDS) ; i++)
		std::cout << (char)data.headerDDS[i] << std::ends;
		
	std::cout << '\n';
	std::cout << " HEADER_DDS_EXT[" << sizeof(data.headerDDS_Ext) << "]:";
	for (size_t i = 0; i < sizeof(data.headerDDS_Ext); i++)
		std::cout << data.headerDDS_Ext[i] << std::ends;
	std::cout.flush();
	std::cout << '\n';
	std::cout << " FIN_DATA[" << data.finData.size() << "]:";
	for (const auto &c : data.finData)
		std::cout << (char)c << std::flush;
	
	std::cout << '\n';
	std::cout << " MIP_POSN(" << data.mipPos << ")" << std::endl;
	std::cout << " MIP_ZERO[" << data.mipmapZero.size() << "]:";
	
	for (const auto &c : data.mipmapZero) 
		std::cout << c ;
	std::cout << "\n-------------------------------------------------------------------" << std::endl;*/
#pragma endregion

	// Rename ".dds" infile to ".dds.0" and delete original
	if ((options.Umode == UNSPLIT_MODE_FIRST) && !(data.bIsGloss)) {
		RenameFile(info.sNameAllExt, info.sBaseName + ".0", true);
	}
	
	string outName = (data.bIsGloss ? info.sDirectory + info.sGlossName : info.sDirectory + info.sBaseName);
	
	ofstream fout(outName, ios::out | ios::binary);
	if (!fout) {
		std::cout << " ERROR: Unsplit reconstructor could not open ouput file: " << info.sBaseName << endl;
		return false;
	}

	info.sExt2 = "";

	// Write headers
	for (size_t i = 0; i < sizeof(data.headerDDS); i++)
		fout << data.headerDDS[i];

	if (data.bIsDX10) {
		for (size_t i = 0; i < sizeof(data.headerDDS_Ext); i++)
			fout << data.headerDDS_Ext[i];
	}
	
	/*
	Mipmaps are be numbered in order of	increasing resolution (*.dds.0, *.dds.1, *.dds.2, ...) so we can just
	append files from highest extension number to lowest without needing to check filesize.

	The mip maps must be joined in this order of decreasing size for the DDS file to work as expected.	*/

	ifstream fin;
	
	// Append mipmap data to file
	char* obuffer = new char();
	string mipfileName;

	for (int i = MAX_MIPMAPS; i >= 0; i--)	{
		mipfileName = info.sDirectory + info.sBaseName + "." + to_string(i);
		if (data.bIsGloss) { mipfileName += "a"; }

		fin.open(mipfileName, ios::in | ios::binary);
		if (fin.good())	{
			while (fin.read(obuffer, 1).good())	{
				fout.write(obuffer, 1);
			}
		}
		fin.close();
	}
	fout.close();

	return true;
}

bool FileExists(string filename)
{
	ifstream infile(filename);
	return infile.good();
}

bool DirectoryExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void RemoveFile(string filename)
{
	const char* target = filename.c_str();
	remove(target);
}

// Delete the .dds.* file parts
void Cleanup(SUnsplitFileNameInfo &info, SUFileData &data)
{
	string baseName = info.sBaseName;
	string target;
	
	for (int i = 0; i <= MAX_MIPMAPS; i++)
	{
		target = baseName + "." + to_string(i);
		
		if (data.bIsGloss)
		{
			target += "a";
		}
		RemoveFile(target);
	}
}

void RenameFile(string oldFileName, string newFileName, bool overwrite)
{
	if (oldFileName.compare(newFileName) == 0)
		return;

	char* fileData = new char();
	ifstream fin(oldFileName, ios::in | ios::binary);
	ofstream fout(newFileName, ios::out | ios::binary);
	while (fin.read(fileData, 1).good())
	{
		fout.write(fileData, 1);
	}
	fout.close();
	fin.close();
	if (overwrite)
	{
		RemoveFile(oldFileName);
	}

	delete[] fileData;
}

UNSPLIT_MODE DetermineUnsplitMode(SUnsplitFileNameInfo &info)
{
	// Determine the Unsplit process for the file.
	if(!(info.sExt2.empty())) //A1
	{	
		// Target is a ".dds.0" file
		string sOneFile(info.sDirectory + info.sBaseName + ".1");
		if (FileExists(sOneFile)) //B1
		{
			return UNSPLIT_MODE_SECOND;
		}
		else  //B2
		{
			return UNSPLIT_MODE_STRIP;
		}
	}
	else //A2
	{
		//Target is a ".dds" file
		string sZeroFile(info.sDirectory + info.sBaseName + ".0");
		if (FileExists(sZeroFile))  //C1
		{
			return UNSPLIT_MODE_NONE;	// File should be an already unsplit DDS -> no unsplit required
		}
		else  //C2
		{
			string sOneFile(info.sDirectory + info.sBaseName + ".1");
			if (FileExists(sOneFile)) //D1
			{
				return UNSPLIT_MODE_FIRST;
			}
			else //D2
			{
				return UNSPLIT_MODE_NONE;	// File is a valid DDS -> no unsplit required
			}
		}
	}
	cout << "UMODE: ******* FALLBACK *******" << endl;
	return UNSPLIT_MODE_NONE; //FALLBACK <<<<<<<<<<<<<<<<<<<<<<<<<<<<
}

void InitializeGlossInfo(SUnsplitFileNameInfo &normal, SUnsplitFileNameInfo &gloss, string ext2)
{
	gloss.sFullPath = normal.sDirectory + normal.sBaseName + ext2;
	gloss.sDirectory = normal.sDirectory;
	gloss.sName = normal.sName;
	gloss.sExt1 = normal.sExt1;
	gloss.sExt2 = ext2;
	gloss.sBaseName = gloss.sName + gloss.sExt1;
	gloss.sNameAllExt = gloss.sName + gloss.sExt1 + gloss.sExt2;
	gloss.sGlossName = normal.sGlossName;
}

void ProcessGlossMap(SUnsplitFileNameInfo &info, SUnsplitOptions &options, string &logfile)
{
	string msg = "";
	SUnsplitFileNameInfo infoGloss = SUnsplitFileNameInfo();
	if (FileExists(info.sDirectory + info.sBaseName + ".a"))
	{
		InitializeGlossInfo(info, infoGloss, ".a");
	}
	else if (FileExists(info.sDirectory + info.sBaseName + ".0a"))
	{
		InitializeGlossInfo(info, infoGloss, ".0a");
	}

	MessageOut(logfile, msg + "...OK.", true, false);
	MessageOut(logfile, "\n\t\tgloss map found: " + infoGloss.sNameAllExt + ", unsplit:", true, false);

	SUFileData dataGloss = SUFileData();
	GetFileData(infoGloss, dataGloss, logfile);

	if (!ReassembleDDS(infoGloss, dataGloss, options, logfile))
		return;

	if (options.bClean)
	{
		Cleanup(infoGloss, dataGloss);
	}
}

bool Unsplit(SUnsplitFileNameInfo &info, SUnsplitOptions &options, SUFileData &data, string &logfile)
{
			
	std::string msg = "";
	options.Umode = DetermineUnsplitMode(info);
	MessageOut(logfile, msg + "(mode: " + to_string(options.Umode) + ")", false, false);

	switch (options.Umode)
	{
	case UNSPLIT_MODE_FIRST:
	case UNSPLIT_MODE_SECOND:
		if (!ReassembleDDS(info, data, options, logfile))
			return false;
		
		if (options.bClean)
		{
			Cleanup(info, data);
		}

		// If 'a' files exist - create new NameInfo struct and repeat unsplit
		if (info.hasGloss)
		{
			ProcessGlossMap(info, options, logfile);		
		}	
		break;

	//case UNSPLIT_MODE_SECOND:
	//	if (!ReassembleDDS(info, data, options, logfile))
	//		return false;
	//
	//	if (options.bClean)
	//	{
	//		Cleanup(info, data);
	//	}
	//
	//	// If 'a' files exist - change NameInfo strings and repeat unsplit
	//	if (info.hasGloss)
	//	{
	//		ProcessGlossMap(info, options, logfile);
	//	}			
	//	break;

	case UNSPLIT_MODE_STRIP:
		RenameFile(info.sNameAllExt, info.sBaseName, true);
		info.sExt2 = "";
		break;

	case UNSPLIT_MODE_NONE:
		if (info.hasGloss)
		{
			ProcessGlossMap(info, options, logfile);
		}
		break;
	}

	return true;
}

// Copy the gloss image data (RED channel) onto the alpha channel of the normal image
bool MergeGlossWithNormal(string norm, string gloss, string logfile)
{
	string err = "ERROR: converted gloss map not found!";
	if (!(FileExists(gloss)))
	{
		MessageOut(logfile, err, true, false);
		return false;
	}
	cv::Mat imgNorm;
	cv::Mat imgGloss;

	cv::Mat red, grn, blu;
	cv::Mat vecRGBA[4];
	cv::Mat glossRGBA[4];
	cv::Mat channels[4];
	
	imgNorm = cv::imread(norm, CV_8UC4);
	imgGloss = cv::imread(gloss, CV_8UC4);

	// check for equal img dimensions
	if (imgNorm.cols != imgGloss.cols || imgNorm.rows != imgGloss.rows)
	{
		err = "ERROR: image size mismatch!";
		MessageOut(logfile, err, true, false);
		return false;
	}

	if (imgNorm.depth() != imgGloss.depth())
	{
		err = "ERROR: image depth mismatch!";
		MessageOut(logfile, err, true, false);
		return false;
	}

	cv::split(imgNorm, vecRGBA);
	cv::split(imgGloss, glossRGBA);
	
	channels[0] = vecRGBA[0];
	channels[1] = vecRGBA[1];
	channels[2] = vecRGBA[2];
	channels[3] = glossRGBA[0];

	cv::Mat imgOut(imgNorm.rows, imgNorm.cols, CV_8UC4);
	
	cv::merge(channels, 4, imgOut);

	cv::imwrite(norm, imgOut);

	return true;
}

void PrintOptions(SUnsplitOptions &options)
{
	string value = "";
	std::cout << "Selected options..." << std::endl;
	value = (options.bVerbose ? "true" : "false");
	std::cout << "\tVerbose     : " << value << std::endl;
	value = (options.bRecursive ? "true" : "false");
	std::cout << "\tRecursive   : " << value << std::endl;
	value = (options.bClean ? "true" : "false");
	std::cout << "\tClean-up    : " << value << std::endl;
	value = (options.bMergeGloss ? "true" : "false");
	std::cout << "\tMerge gloss : " << value << std::endl;
	std::cout << "\tSave format : " << options.sFileType << std::endl;
}

bool LoadConfigFile(string filepath, SUnsplitOptions &options)
{
	//std::cout << "filepath: " << filepath << std::endl;
	std::ifstream ifs(filepath);
	if (!ifs.good())
	{
		std::cout << "ERROR: Failed to open config file." << std::endl;
		std::cout << "HINT - check that 'config.txt' file is in the same folder as sctexconv.exe," << std::endl;
		std::cout << " or that it is not currently open in another program." << std::endl;
		return false;
	}

	// Yes I chose a long and ugly way of doing this but I'm tired and this works so I'm moving on.
	std::string line;
	while (std::getline(ifs, line))
	{
		if (line != "")
		{
			if (line.front() == '#')
				continue;
			if (line.front() == ' ')
				continue;

			if (line.find("verbose") != string::npos)
			{
				if (line.find("true") != string::npos)
				{
					options.bVerbose = true;
					std::cout << " verbose = true" << std::endl;
				}
				else if (line.find("false") != string::npos)
				{
					options.bVerbose = false;
					std::cout << " verbose = false" << std::endl;
				}
				else {
					std::cout << " ERROR: verbose value not recognized as valid!" << std::endl;
					std::cout << " - HINT - Use 'true' or 'false' only. Don't use UPPERCASE characters or extra spaces." << std::endl;
					return false;
				}

			}
			else if (line.find("recursive") != string::npos)
			{
				if (line.find("true") != string::npos)
				{
					options.bRecursive = true;
					std::cout << " recursive = true" << std::endl;
				}
				else if (line.find("false") != string::npos)
				{
					options.bRecursive = false;
					std::cout << " recursive = false" << std::endl;
				}
				else {
					std::cout << " ERROR: recursive value not recognized as valid!" << std::endl;
					std::cout << " - HINT - Use 'true' or 'false' only. Don't use UPPERCASE characters or extra spaces." << std::endl;
					return false;
				}
			}
			else if (line.find("clean") != string::npos)
			{
				if (line.find("true") != string::npos)
				{
					options.bClean = true;
					std::cout << " clean = true" << std::endl;
				}
				else if (line.find("false") != string::npos)
				{
					options.bClean = false;
					std::cout << " clean = false" << std::endl;
				}
				else {
					std::cout << " ERROR: clean value not recognized as valid!" << std::endl;
					std::cout << " - HINT - Use 'true' or 'false' only. Don't use UPPERCASE characters or extra spaces." << std::endl;
					return false;
				}
			}
			else if (line.find("merge_gloss") != string::npos)
			{
				if (line.find("true") != string::npos)
				{
					options.bMergeGloss = true;
					std::cout << " merge_gloss = true" << std::endl;
				}
				else if (line.find("false") != string::npos)
				{
					options.bMergeGloss = false;
					std::cout << " merge_gloss = false" << std::endl;
				}
				else {
					std::cout << " ERROR: merge_gloss value not recognized as valid!" << std::endl;
					std::cout << " - HINT - Use 'true' or 'false' only. Don't use UPPERCASE characters or extra spaces." << std::endl;
					return false;
				}

			}
			else if (line.find("format") != string::npos)
			{
				if (line.find("tif") != string::npos)
				{
					options.sFileType = "tif";
					std::cout << " format = tif" << std::endl;
				}
				else if (line.find("png") != string::npos)
				{
					options.sFileType = "png";
					std::cout << " format = png" << std::endl;
				}
				else if (line.find("tga") != string::npos)
				{
					options.sFileType = "tga";
					std::cout << " format = tga" << std::endl;
				}
				else if (line.find("jpg") != string::npos)
				{
					options.sFileType = "jpg";
					std::cout << " format = jpg" << std::endl;
				}
				else if (line.find("dds") != string::npos)
				{
					options.sFileType = "dds";
					std::cout << " format = dds" << std::endl;
				}
				else if (line.find("bmp") != string::npos)
				{
					options.sFileType = "bmp";
					std::cout << " format = bmp" << std::endl;
				}

				else {
					std::cout << " ERROR: format value not recognized as valid!" << std::endl;
					std::cout << " - HINT - Use one of 'tif' 'png' 'tga' 'jpg' 'dds' or 'bmp' only. Don't use UPPERCASE characters or extra spaces." << std::endl;
					return false;
				}
			}
			else
			{
				std::cout << " ERROR: '" << line << "' not recognized as a valid config field!" << std::endl;
				std::cout << " - HINT - don't use UPPERCASE characters or extra spaces." << std::endl;
				return false;
			}
		}
	}

	ifs.close();
	
	return true;
}

void LogMessage(string &logfile, string &message, bool bkspace)
{
	std::ofstream log(logfile, std::ofstream::out | std::ofstream::app);

	if (!log)
	{
		std::cerr << "Cannot open the output file." << std::endl;
		return;
	}
	// 'bkspace' overwrites the 'new line' char at end of log file so that 
	//  this message appears on the same line as the last one.
	if (bkspace)
	{
		log.seekp(ios::end);
	}

	log << message << std::ends;

	log.close();
}

void LogOptions(string &logfile, SUnsplitOptions &options)
{
	std::ofstream log(logfile, std::ofstream::out | std::ofstream::app);

	if (!log)
	{
		std::cerr << "Cannot open the output file." << std::endl;
		return;
	}

	string value = "";
	log << " Selected options..." << std::endl;
	value = (options.bVerbose ? "true" : "false");
	log << "\tVerbose     : " << value << std::endl;
	value = (options.bRecursive ? "true" : "false");
	log << "\tRecursive   : " << value << std::endl;
	value = (options.bClean ? "true" : "false");
	log << "\tClean-up    : " << value << std::endl;
	value = (options.bMergeGloss ? "true" : "false");
	log << "\tMerge gloss : " << value << std::endl;
	log << "\tSave format : " << options.sFileType << "\n" << std::endl;

	log.close();
}

void MessageOut(std::string &logfile, std::string &message, bool bkspace, bool newline)
{
	LogMessage(logfile, message, bkspace);
	if (newline)
	{
		std::cout << message << std::endl;
	} 
	else 
	{
		std::cout << message << std::ends;;
	}
}

int SizeOfFile(string file)
{
	int size = 0;

	streampos begin, end;
	ifstream infile(file, ios::binary);
	begin = infile.tellg();
	infile.seekg(0, ios::end);
	end = infile.tellg();
	infile.close();

	size = (int)(end - begin);

	return size;
}

// fname must be filename with no extension
bool FIconvert(string fname, string ftype)
{
	string infile = fname + ".tga";

	// Assume file from nvdecompress.exe is of TARGA format.
	FIBITMAP *bitmap = FreeImage_Load(FIF_TARGA, infile.c_str(), TARGA_DEFAULT);
	if (!bitmap) {
		return false;
	}

	FREE_IMAGE_FORMAT FIfmt;
	if (ftype == "png")
		FIfmt = FIF_PNG;

	if (ftype == "tif")
		FIfmt = FIF_TIFF;

	string outfile = fname + "." + ftype;
	if (FreeImage_Save(FIfmt, bitmap, outfile.c_str(), 0)) {
		// bitmap successfully saved!
		FreeImage_Unload(bitmap);
		return true;
	}
	FreeImage_Unload(bitmap);
	return false;
}

bool RunNVDecompress(string nvpath, string file, DWORD &exitCode)
{
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	file = " " + file;
	ATL::CA2CT lpctApp(nvpath.c_str());
	ATL::CA2W lpwFile(file.c_str());

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	BOOL result = CreateProcess(lpctApp,   // the path
								lpwFile,        // Command line
								NULL,           // Process handle not inheritable
								NULL,           // Thread handle not inheritable
								FALSE,          // Set handle inheritance to FALSE
								0,              // No creation flags
								NULL,           // Use parent's environment block
								NULL,           // Use parent's starting directory 
								&si,            // Pointer to STARTUPINFO structure
								&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
	if (!result)
	{
		return false;
	}
	else
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		result = GetExitCodeProcess(pi.hProcess, &exitCode);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	
		return true;
	}
}

//######################################################################################
//--------------------------------------------------------------------------------------
// NEW Entry-point for SCTEXCONV
//--------------------------------------------------------------------------------------
//######################################################################################
#pragma comment(lib, "User32.lib")
int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
#pragma region INITIALIZE
	// Parameters and defaults
	int result			= 0;
	int count			= 0;
	int failCount		= 0;
	char tmpbuf[128]	= { 0 };
	wchar_t* args[128]	= { 0 };
	PWSTR  pWdir		= 0;
	string msg			= "";
	string sError		= "";
	string installdir	= "";	
	string targetdir	= "";

	// Get the path of the executable
	WCHAR path[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	ATL::CW2A szPath(path);
	installdir = szPath;

	// Parse command-line
	if (argc == 2)
	{
		pWdir = argv[1];
		ATL::CW2A lpWdir(pWdir);
		targetdir = lpWdir;

		if (DirectoryExists(pWdir))
		{
			std::cout << " Target directory found..." << std::endl;
		}
		else
		{
			std::cout << " Could not access or locate the directory " << targetdir << std::endl;
			std::cout << " Press ENTER key to close..." << std::ends;
			std::getchar();
			return 0;
		}
	}
	else
	{
		targetdir = installdir;
	}

	// Load and parse config.txt file
	SUnsplitOptions options;
	PrintLogo();
	std::cout << " Loading config..." << std::endl;
	string config_path = installdir + "\\" + "config.txt";

	if (!LoadConfigFile(config_path, options))
	{		
		std::cout << " Press ENTER key to close..." << std::ends;
		std::getchar();
		return 0;
	}

	// Display O/S date and time.   
	_tzset();
	_strdate_s(tmpbuf, 128);
	string startDate(tmpbuf);
	printf(" Conversion started:  %s, ", tmpbuf);
	_strtime_s(tmpbuf, 128);
	string startTime(tmpbuf);
	printf("%s\n", tmpbuf);
	time_t start;
	time(&start);

	time_t now;
	time_t elapsed;
	uint hrs, min, sec, remain; 
	string fmtHour, fmtMin, fmtSec;

	// Log file filename time-stamp is UNIX-style (i.e. number of seconds since 1st Jan 1970)	
	long long llstart(start);
	string log_file = "sctextureconverter_log_" + to_string(llstart) + ".txt";
	string log_file_path = installdir + "\\" + log_file;
	string nvDecompress_path = installdir + "\\nvdecompress.exe";
	
	PrintLogo(log_file_path);
	LogOptions(log_file_path, options);
	LogMessage(log_file_path, (" Conversion started: " + startDate + ", " + startTime), false);
	LogMessage(log_file_path, msg + "\n Directory: " + targetdir, false);

	//
	// -- Build a list of available DDS header files -----------------------------------
	std::cout << "\n Searching for DDS header files in " << targetdir << std::endl;
	
	std::cout << "\n\n Searching... " << std::ends;
	list<string> fileList;
	BuildFileList(targetdir, fileList, log_file_path, options.bRecursive, count);
	
	if (fileList.empty())
	{
		MessageOut(log_file_path, (msg + "\n FILE SEARCH ERROR: No valid files found. EXITING..."), false, true);
		std::cout << "\n Press ENTER key to close..." << std::ends;
		std::getchar();
		return 1;
	}

	LogMessage(log_file_path, msg + "\n Find errors in this log quickly by searching for 'FAILED' and 'ERROR'.", false);

	if (options.bVerbose)
	{
		int cnt = 0;
		for each (string i in fileList)
		{
			cnt++;
			MessageOut(log_file_path, ("\n File " + to_string(cnt) + ": " + i), false, true);
			if (UserWantsToExit()) { return 0; }

		}
	}

	MessageOut(log_file_path, ("\n " + to_string(count) + " valid file headers found.\n"), false, true);
#pragma endregion	

	// Using regex to split the file path into its components
	regex rgx(R"(^(.*)\\(\w+)(.[dD][dD][sS])([.][0])*$)"); 
	
	smatch mch;
	int index = 0;
		
	for each(string i in fileList)
	{
		bool doUnsplit = true;
		bool doConvert = true;

		index++;
		if (regex_search(i, mch, rgx))
		{
			string matchDir = mch[1].str();

			if (strcmp(matchDir.c_str(), targetdir.c_str()) != 0)
			{
				CA2T newdir(matchDir.c_str());
				if (SetCurrentDirectory(newdir))
				{
					//std::cout << "Opening directory " << matchDir << std::endl;
				}
				else
				{
					MessageOut(log_file_path, (" ERROR: Cannot open directory " + matchDir), false, true);
				}
			}

			SUnsplitFileNameInfo  info;
			info.sFullPath		= mch[0];
			info.sDirectory		= mch[1];
			info.sDirectory	   += "\\";
			info.sName			= mch[2];
			info.sExt1			= mch[3];
			info.sExt2			= mch[4];
			info.sBaseName		= mch[2];
			info.sBaseName	   += ".dds";
			info.sNameAllExt	= mch[2];
			info.sNameAllExt   += ".dds";
			info.sNameAllExt   += mch[4];
			info.hasGloss		= false;
			info.sGlossName		= mch[2];
			
			// Provide a matching gloss filename even though we don't know if the gloss file exists yet.
			info.sGlossName	   += GLOSS_KEY + ".dds";

			SUFileData data = SUFileData();
			GetFileData(info, data, log_file_path);		

#pragma region PROGRESS_OUTPUT
			// Output job progress to console and log ------------------------------------------
			if (!options.bVerbose)
			{
				system("cls");
				PrintLogo();
				PrintOptions(options);
			}
			
			time(&now);
			elapsed = now - start;
			hrs = elapsed / 3600;
			min = (elapsed /60) % 60;
			sec = elapsed % 60;
			fmtHour = (hrs < 10 ? "0" : "");
			fmtMin  = (min < 10 ? "0" : "");
			fmtSec  = (sec < 10 ? "0" : "");
			std::cout << "\n " << "Time elapsed: " << fmtHour << hrs << ":" << fmtMin << min << ":" << fmtSec << sec << std::ends;

			remain = ((elapsed / index > 0) ? ((count - index) * (elapsed / index)) : ((count - index) / 2));
			hrs = remain / 3600;
			min = (remain / 60) % 60;
			sec = remain % 60;
			fmtHour = (hrs < 10 ? "0" : "");
			fmtMin  = (min < 10 ? "0" : "");
			fmtSec  = (sec < 10 ? "0" : "");
			string errGrmr = (failCount == 1 ? " error." : " errors.");
			std::cout << ",  Est. time remaining: " << fmtHour << hrs << ":" << fmtMin << min << ":" << fmtSec << sec << std::ends;
			std::cout << ",  " << to_string(failCount) << errGrmr << std::ends;
			MessageOut(log_file_path, ("\n File " + to_string(index) + " of " + to_string(count) + " : " + info.sNameAllExt + ", "), false, false);
#pragma endregion

#pragma region CALL_UNSPLIT			
			// Skip if the converted file exists and is > 0 bytes
			string converted = info.sDirectory + info.sName + "." + options.sFileType;
			if (FileExists(converted))
			{
				int fsize = SizeOfFile(converted);
				MessageOut(log_file_path, msg + "File exists (" + to_string(fsize) + " bytes)", true, false);
				if (fsize >= 1 && !data.bIsNormal)
				{
					MessageOut(log_file_path, msg + "-> skipping...", true, false);
					doUnsplit = false;
					doConvert = false;
				}
				else
				{
					MessageOut(log_file_path, msg + "-> overwriting...", true, false);
					doUnsplit = true;
					doConvert = true;
				}
			}
			else
			{
				int fsize = SizeOfFile(info.sDirectory + info.sNameAllExt);
				int fsize2 = (data.bIsGloss ? SizeOfFile(info.sDirectory + info.sBaseName + ".2a") 
											 : SizeOfFile(info.sDirectory + info.sBaseName + ".2"));

				if (fsize > fsize2 && (FileExists(info.sDirectory + info.sGlossName)))
				{
					doUnsplit = false;
				}
			}

			if (doUnsplit)
			{
				MessageOut(log_file_path, msg + "Unsplit:", true, false);

				// Call Unsplit -------------------------------------------------
				if (Unsplit(info, options, data, log_file_path))
				{
					MessageOut(log_file_path, msg + "...OK.", true, false);
				}
				else
				{
					failCount++;
					MessageOut(log_file_path, msg + " FAILED <<<<<<<<<<<<<<<<<<<<<", true, true);
					continue;
				}
			}
#pragma endregion			

#pragma region CALL_CONVERTER
			if (info.sExt2 == ".0")
			{
				continue;
			}

			bool specifyFormat = false;
			string sReadFmt    = "";

			//Build arg list for texture converter
			int idxArgs = 0;
			std::fill(std::begin(args), std::end(args), nullptr);
						
			ATL::CA2T lptZero("sctexconvert");	//Zero element for the options array
			args[idxArgs] = lptZero;
			idxArgs++;

			//read format
			/*if (info.sName.find("_mask") != string::npos || info.sName.find("_opa") != string::npos
													   	 || info.sName.find("_trans") != string::npos)*/									
			if (data.bIsNormal) {
				MessageOut(log_file_path, msg + "(forced normal)", true, false);
				sReadFmt = "B8G8R8A8_UNORM";  // B5G5R5A1_UNORM also works but will reduce quality		
				specifyFormat = true;
			}
			else if (info.hasGloss) // This should cover all img types that are not normals but have a mask e.g. _opa, _mask, _trans.
			{
				MessageOut(log_file_path, msg + "(forced 4 channel)", true, false);
				sReadFmt = "R8G8B8A8_UNORM";
				specifyFormat = true;
			}

			ATL::CA2T lptRead("-f");
			ATL::CA2T lptReadFmt(sReadFmt.c_str());

			// If file is not a normal map then let converter choose best read format.
			if (specifyFormat)
			{
				args[idxArgs] = lptRead;
				idxArgs++;
				args[idxArgs] = lptReadFmt;
				idxArgs++;
			}

			// output directory flag
			ATL::CA2T lptOflag("-o");
			args[idxArgs] = lptOflag;
			idxArgs++;
			// output directory				
			ATL::CA2T lptOdir(info.sDirectory.c_str());
			args[idxArgs] = lptOdir;
			idxArgs++;

			// output file type flag
			ATL::CA2T lptFType("-ft");
			args[idxArgs] = lptFType;
			idxArgs++;
			// output file type				
			ATL::CA2T lptType(options.sFileType.c_str());
			args[idxArgs] = lptType;
			idxArgs++;

			//normal map
			ATL::CA2T lpNFlag("-nmap");		//options must be one or more of { r, g, b, a, l, m, u, v, i, o } 
			ATL::CA2T lpNopt("rgb");		// for .ddn & .ddna files: use rgb
			ATL::CA2T lpNamp("-nmapamp");
			ATL::CA2T lpNwt("10.0");        // weight for normal map amplitude
											//TODO: Test normal reconstruction via OpenCV functions or nvtt
			if (data.bIsNormal)
			{
				args[idxArgs] = lpNFlag;
				idxArgs++;

				args[idxArgs] = lpNopt;
				idxArgs++;

				args[idxArgs] = lpNamp;
				idxArgs++;

				args[idxArgs] = lpNwt;
				idxArgs++;
			}

			//Filename goes last in argument list
			string strFname = info.sDirectory + info.sBaseName;
			ATL::CA2T lpwName(strFname.c_str());
			args[idxArgs] = lpwName;
			idxArgs++;

			// Call Texture converter--------------------------------------------------
			if (doConvert)
			{
				int failCount_prev = failCount;
				if (!(data.bIsGloss))
				{
					SCTexConvert(idxArgs, args, log_file_path, failCount);
				}

				if (data.bIsGloss || failCount > failCount_prev)
				{				
				// Use NVidia Texture Tool for gloss maps or if DirectX Tool failed
					std::cout << "\n\t\t " << std::ends;
					DWORD exitcode;
					if (RunNVDecompress(nvDecompress_path, strFname, exitcode))
					{
						MessageOut(log_file_path, (msg + "NV exit code (" + to_string(exitcode) + ")"), true, false);
						if (options.sFileType != "tga")
						{
							if (FIconvert(info.sDirectory + info.sName, options.sFileType))
							{
								MessageOut(log_file_path, (msg + " FreeImage convert...OK. "), true, false);
							}
							else
							{
								MessageOut(log_file_path, (msg + " FreeImage convert...FAILED. "), true, false);
								failCount = (failCount == failCount_prev ? failCount + 1 : failCount + 0);
							}
						}
					}
					else
					{
						MessageOut(log_file_path, (msg + " NVidia convert...FAILED (" + to_string(exitcode) + ")"), true, false);
						failCount++;
					}
				}
			}

			// Process gloss map if required-------------------------------------------
			if (!info.hasGloss)
				continue;			
			
			if (FileExists(info.sDirectory + info.sName + GLOSS_KEY + "." + options.sFileType))
			{
				doConvert = false;
				if (!options.bMergeGloss)
				{
					continue;
				}
			}

			SUnsplitFileNameInfo ginfo = SUnsplitFileNameInfo();
			InitializeGlossInfo(info, ginfo, "");
			
			SUFileData gdata = SUFileData();
			GetFileData(ginfo, gdata, log_file_path);

			strFname = info.sDirectory + info.sGlossName;
			ATL::CA2T lpwGName(strFname.c_str());

			if (doConvert)
			{
				// Reset and re-build args list for converter
				idxArgs = 0;
				std::fill(std::begin(args), std::end(args), nullptr);
				//Pointers already created previously
				args[idxArgs] = lptZero;
				idxArgs++;

				//read format 
				sReadFmt;
				specifyFormat = false;

				if (data.bIsMaskAlpha)
				{
					sReadFmt = "R8G8B8A8_UNORM";
					specifyFormat = true;
				}
				
				ATL::CA2T lptReadFmt(sReadFmt.c_str());

				// If file is not a normal map then let converter choose best read format.
				if (specifyFormat)
				{
					args[idxArgs] = lptRead;
					idxArgs++;
					args[idxArgs] = lptReadFmt;
					idxArgs++;
				}

				// output directory
				args[idxArgs] = lptOflag;
				idxArgs++;		
				args[idxArgs] = lptOdir;
				idxArgs++;

				// output file type
				args[idxArgs] = lptFType;
				idxArgs++;
				args[idxArgs] = lptType;
				idxArgs++;

				// filename
				args[idxArgs] = lpwGName;
				idxArgs++;

				int failCount_prev = failCount;
				SCTexConvert(idxArgs, args, log_file_path, failCount);

				if (failCount > failCount_prev)
				{
					DWORD exitcode;
					std::wcout << "\n\t\t " << std::ends;
					if (RunNVDecompress(nvDecompress_path, strFname, exitcode))
					{
						MessageOut(log_file_path, (msg + ", trying Nvidia convert (" + to_string(exitcode) + "):"), true, false);
						if (exitcode == 0 && options.sFileType != "tga")
						{
							if (FIconvert(info.sDirectory + info.sName + GLOSS_KEY, options.sFileType))
							{
								MessageOut(log_file_path, (msg + " OK, FreeImage convert...OK. "), true, false);
								failCount--;
							}
							else
							{
								MessageOut(log_file_path, (msg + " OK, FreeImage convert...FAILED. "), true, false);
								failCount++;
							}
						}
						else if (exitcode != 0)
						{
							MessageOut(log_file_path, (msg + " FAILED. "), true, false);
							failCount++;
						}
					}
					else
					{
						MessageOut(log_file_path, (msg + "NVidia convert...FAILED (" + to_string(exitcode) + ")"), true, false);
						failCount++;
					}
				}
			}
			
			// Merge files if required---------------
			if (options.bMergeGloss)
			{
				MessageOut(log_file_path, (msg + "  Merging gloss data into normal map:"), false, false);
				
				if (!FileExists(converted))
				{
					MessageOut(log_file_path, (msg + "  FAILED: Converted normal file not found <<<<<<<<<<<<< "), false, false);
					break;
				}
				
				string converted_gloss = info.sDirectory + info.sName + GLOSS_KEY + "." + options.sFileType;
				
				if (MergeGlossWithNormal(converted, converted_gloss, log_file_path))
				{
					MessageOut(log_file_path, (msg + "...OK. "), true, false);
				}
				else
				{
					MessageOut(log_file_path, (msg + "...FAILED <<<<<<<<<<<<<<<< "), true, false);
					failCount++;
				}
			}
		}
#pragma endregion	
		
		if (UserWantsToExit()) { break; }		
	}
	
#pragma region END_REPORT	
	// Output performance data.
	string grmrNzi1 = (failCount == 1 ? " error." : " errors.");
	_strdate_s(tmpbuf, 128);
	startDate = tmpbuf; 
	_strtime_s(tmpbuf, 128);
	startTime = tmpbuf;

	MessageOut(log_file_path, ("\n\n Conversion finished:  " + startDate + ", " + startTime), false, false);
	MessageOut(log_file_path, (" with " + to_string(failCount) + grmrNzi1), true, true);

	time_t end;
	time(&end);
	time_t diff = end - start;
	string grmrNzi2 = ((diff / 60) == 1 ? " minute, " : " minutes, ");
	string grmrNzi3 = ((diff % 60) == 1 ? " second."  : " seconds.");
	MessageOut(log_file_path, ("\n " + to_string(count) + " Files processed in: " + to_string(diff / 60) + grmrNzi2 + to_string(diff % 60) + grmrNzi3), false, true) ;
	std::cout << "\nLog file created: " << log_file << std::endl;
#pragma endregion

#pragma region EXITING

	std::cout << " Press ENTER key to close..." << std::ends;
	std::getchar();
	return result;
#pragma endregion
}


