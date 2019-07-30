#include "tr_cvar.h"
#include "ref_import.h"

cvar_t	*r_railWidth;
cvar_t	*r_railCoreWidth;
cvar_t	*r_railSegmentLength;

cvar_t	*r_verbose;

cvar_t	*r_znear;


cvar_t	*r_inGameVideo;
cvar_t	*r_dynamiclight;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;



cvar_t* r_fullscreen;
// display refresh rate
cvar_t* r_displayRefresh;

cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_flares;
cvar_t	*r_singleShader;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
// cvar_t	*r_offsetFactor;
// cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

// r_overbrightBits->integer, but set to 0 if no hw gamma
cvar_t	*r_brightness;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;

cvar_t	*r_maxpolys;
cvar_t	*r_maxpolyverts;

cvar_t* r_allowResize; // make window resizable
cvar_t* r_mode;

cvar_t	*r_gpuIndex;

void R_Register( void ) 
{
	// latched and archived variables
    //
	// temporary latched variables that can only change over a restart
	//
	// archived variables that can change at any time

	r_picmip = ri.Cvar_Get ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
    ri.Cvar_CheckRange( r_picmip, 0, 4, qtrue );
	r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri.Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );


    // image/linghtmap brightness
    r_gamma = ri.Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE | CVAR_LATCH );
    ri.Cvar_CheckRange( r_gamma, 1.0f, 3.0f, qfalse );
	
    r_brightness = ri.Cvar_Get ("r_brightness", "2", CVAR_ARCHIVE | CVAR_LATCH );
    ri.Cvar_CheckRange( r_brightness, 1.0f, 3.0f, qfalse );
    
    r_mapOverBrightBits = ri.Cvar_Get ("r_mapOverBrightBits", "2", CVAR_ARCHIVE | CVAR_LATCH );
	ri.Cvar_CheckRange( r_mapOverBrightBits, 1, 3, qtrue );

    

    r_vertexLight = ri.Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri.Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = ri.Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

	r_singleShader = ri.Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	r_lodCurveError = ri.Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
    r_flares = ri.Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_CHEAT );
    ri.Cvar_CheckRange( r_znear, 0.001f, 200.0f, qfalse );

	r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	
	
    r_facePlaneCull = ri.Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_railWidth = ri.Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = ri.Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE );
	r_railSegmentLength = ri.Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

	r_ambientScale = ri.Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );
	r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri.Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = ri.Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = ri.Cvar_Get ("r_lightmap", "0", 0 );
	r_portalOnly = ri.Cvar_Get ("r_portalOnly", "0", CVAR_TEMP );


	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_nocull = ri.Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri.Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri.Cvar_Get ("r_showcluster", "0", CVAR_TEMP);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", CVAR_TEMP);
	r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_TEMP);
	r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_TEMP);
	r_showsky = ri.Cvar_Get ("r_showsky", "0", CVAR_TEMP);
	r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_TEMP);
	r_lockpvs = ri.Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);

	r_maxpolys = ri.Cvar_Get( "r_maxpolys", "600", 0);
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", "3000", 0);

    
    r_mode = ri.Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );
    r_fullscreen = ri.Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );
    r_displayRefresh = ri.Cvar_Get( "r_displayRefresh", "60", CVAR_LATCH );
    ri.Cvar_CheckRange( r_displayRefresh, 30.0f, 200.0f, qfalse );

    r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );


	r_gpuIndex = ri.Cvar_Get( "r_gpuIndex", "0", CVAR_ARCHIVE );
}
