Star Citizen Texture Converter  - Version 1.3
---------------------------------------------

This program is intended for use after you have extracted texture files from the Star Citizen .pak
archives.

Star Citizen Texture Converter recombines AND converts the DDS texture files extracted from Star 
Citizen asset archives. It replaces the functionality of two programs "UnsplitDXT10" and "texconv" 
previously used to re-assemble and convert Star Citizen texture files.

USAGE
	sctexconv_XXX.exe [inputdir]

	The name of the output files will based on the input files but with a new extension.
		
	EXAMPLE:   "rock_01_ddna.dds"    converts to    "rock_01_ddna.tif"

	Output files are written to same directory as input files.
 
----------------------------------------------------------------------------------------------------

INSTRUCTIONS

** IMPORTANT - DO NOT RUN THIS PROGRAM IN YOUR ORIGINAL GAME INSTALL FOLDER **

1. There is no installer wizard, just unzip all files into one directory to install: 
	
	sctexconv_XXX.exe,             <----- replace XXX with current version number.
	config.txt,
	opencv_world320.dll
	FreeImage.dll
	nvtt.dll
	nvdecompress.exe
	
2. Open the 'config.txt' file and check that the settings match what you want. The output format
	is "tif" by default.

3. The program is also RECURSIVE by default, so .dds files in sub-directories will also be converted.

4. THERE ARE TWO METHODS TO RUN THE PROGRAM:
	
	Method 1:
	=========
	- Place ALL the files listed above into the root directory of your textures.
	- Double-click on the sctexconv_XXX.exe icon, or open a new command terminal window at 
	  the same directory and type the name of the .exe file.
	
	Method 2:
	=========
	- Open a File Explorer window to the installed directory so that you can see the files listed above.
	- Open a second File Explorer window for the root directory of the texture files you wish to convert.
	- Drag and drop the texture folder onto the sctexconv_XXX.exe file.
	- Cheer \o/

5. Once the initial file search has finished, the program can be stopped during run-time by holding 
   down ESCAPE key until the confirm question appears in the console screen.

6. Running the program again will automatically skip any files that have already been converted.

7. When the program finished it will display the total time taken and the number of errors that occurred.

8. The program generates a unique log file (e.g. sctextureconverter_log_1485392722.txt) each time it runs.
   Log files can get very very long - search the file text for "FAILED" to find errors quickly.

----------------------------------------------------------------------------------------------------
NOTES
*   This program is a modified version of Microsoft (R) DirectX 11 Texture Converter
    Copyright (C) Microsoft Corp. All rights reserved.

*   This program uses the DirectX11 ToolKit, NVidia Texture Tools library, OpenCV image library,
	and the FreeImage library.

*   This software is not produced, endorsed nor supported by either 
    Cloud Imperium Games (CIG) or Microsoft.

----------------------------------------------------------------------------------------------------
 
 OPTIONS / CONFIG
 Edit the config.txt file to change the default settings. Use only lowercase letters (no capitals)
 and double check your spelling as the program will stop if it cannot understand the settings.


# verbose [true / false]     : prints all program messages to the screen (default = false). All messages
			       are saved in the log file regardless of this setting.

# recursive [true / false]   : checks all sub-directories for files to unsplit and convert (default = true).

# clean [true / false] 	     : deletes the dds part files (*.dds.0, *.dds.1, ...etc) after each file is 
			       successfully converted (default = false).

# merge_gloss [true / false] : recombines the normal and gloss map into one image file. the gloss map
				will be copied into the alpha channel of its matching *.ddna normal map 
				(for CryEngine/Lumberyard). If 'false' the ddna and gloss map will be saved 
				as two separate files.(default = true).

# format [tif / png / tga ]  : the file format you want for the texture file.(default = tif).


----------------------------------------------------------------------------------------------------
RELEASE HISTORY

February 18, 2017
	1.3	- Added 'drag & drop' feature.
		- Changed config.txt to merge_gloss = true by default.
		- Added NVidia and FreeImage libraries for correct gloss map conversion.
		- Added and extra check to ignore cubemap files.
		- Fixed a bug preventing the gloss map file from being correctly unsplit.
		- Added extra code to handle files which have an alpha channel but are not normal maps.
		  These include files labelled as _mask, _opa, _trans.
		- Added 'Time elapsed' and 'Est. time remaining' timers.
		- Added a counter to display during initial file search so that it does not look like the
		  program has frozen while it is building a large file list.
		- Added an escape routine to file builder in case user wants to abort operation.

January 30, 2017
	1.2.2	- Added an extra check so that _ddna & _gloss files are not skipped if 
		  merge_gloss is set to 'true'.
		- Added an extra check so that if a converted file (.tif, .png, .tga) exists but is
		  0 bytes in size it will be overwritten instead of skipped.

January 29, 2017
	1.2.1	- Added an extra check so that ddn and ddna files that have already been unsplit
		  are still identified as normal files for proper conversion.

January 28, 2017
	1.2	- Fixed a bug where file parts ending in .dds were not correctly identified.
		- Fixed a bug where _ddna files with BC5_SNORM format were not flagged as normals.
		- Fixed a bug that prevented correct version number showing in console & log file.
		- Included unsplit mode index in log reporting to assist with debug tracing.
		- Fixed a typo in console/log message.
		- Minor sobbing.

January 27, 2017
	1.1	- Fixed a bug that prevented DX10 file headers from being read properly.
		- Found and fixed another bug preventing BC6 and BC7 formats converting.
		- Cried a bit.

January 26, 2017
	1.0	- Released.
		- Happy tears.
		
