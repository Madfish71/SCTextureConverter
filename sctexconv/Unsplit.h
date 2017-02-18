#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>

using namespace std;

const int HEADER_SIZE_DDS   = 128;
const int HEADER_SIZE_DX10  = 20;
const int MAX_MIPMAPS		= 12;
const string GLOSS_KEY		= "_glossMap";

enum UNSPLIT_MODE
{
	UNSPLIT_MODE_NONE = 0,	// Vaild DDS file - no Unsplit required. 
	UNSPLIT_MODE_FIRST = 1,	// 2014 SC naming format is used: header = *.dds, lowest mip file = *.dds.1
	UNSPLIT_MODE_SECOND = 2,// 2016 SC naming format is used: header = *.dds.0 lowest mip file = *.dds.1
	UNSPLIT_MODE_STRIP = 3	// Valid DDS but needs .0 extension removed.
};

struct SUFileData
{
	std::vector<unsigned char>  finData;		  //  Data loaded in via ifstream.
	std::vector<unsigned char>  mipmapZero;		  //  first mipmap data.

	std::string		fourcc;
	DXGI_FORMAT		dxgi;
	unsigned char	headerDDS[128];   //  Standard DDS header.
	unsigned char	headerDDS_Ext[20];//  Extended DDS header.
	int				mipPos;			  //  Byte index to start of mipmapZero data.
	bool			bIsDX10;		  //  File has extended DX10 header.
	bool			bIsGloss;		  //  Will be TRUE for any parsed file ending in "a"
	bool			bIsNormal;		  //  BC5 and ATI2 files will be converted as normals.
	bool			bIsMaskAlpha;	  //  Some opacity mask files have ".a" file for alpha channel. These
									  //   mask files have _mask / _opa / _trans in their name. The ".a"
								      //   file header needs to be modified to 'ATI1' type before conversion.
};

struct SUnsplitFileNameInfo
{
	string	sFullPath;		//	mch[0] : Directory path + Filename + ".dds" [+ ".0"]
	string	sDirectory;		//	mch[1] : Directory path
	string	sName;			//	mch[2] : Filename (no extensions)
	string	sExt1;			//	mch[3] : ".dds"
	string	sExt2;			//	mch[4] : ".0" or ""
	string	sBaseName;		//	Filename + ".dds"
	string  sNameAllExt;	//  Filename + ".dds" [+ ".0"]
	bool	hasGloss;		//  For .ddn and .ddna files. Shows a matching gloss/alpha file exists.
	string	sGlossName;		//  Output name of gloss file including .dds ext
};

// User selected options
struct SUnsplitOptions
{
	int		Umode;
	bool	bClean;
	bool	bRecursive;
	bool	bVerbose;
	bool	bMergeGloss;
	string	sFileType;
};

void Cleanup (SUnsplitFileNameInfo &NameInfo, SUFileData &Udata);
UNSPLIT_MODE DetermineUnsplitMode (SUnsplitFileNameInfo &NameInfo);
bool FileExists	(string filename);
bool ReassembleDDS (SUnsplitFileNameInfo &NameInfo, SUFileData &Udata, SUnsplitOptions &Uoptions, std::string &logfile);
void RenameFile	(string oldFileName, string newFileName, bool overwrite);
void RemoveFile	(string filename);
bool Unsplit(SUnsplitFileNameInfo &Name, SUnsplitOptions &Options, SUFileData &Udata, std::string &logfile);
void PrintOptions(SUnsplitOptions &Options);
bool LoadConfigFile(string filepath, SUnsplitOptions &options);
void LogMessage(string &logfile, string &message, bool bkspace);
void LogOptions(string &logfile, SUnsplitOptions &Options);
void MessageOut(string &logfile, string &message, bool bkspace, bool newline);

