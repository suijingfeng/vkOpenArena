// tr_init.c -- functions that are not called every frame

#include "tr_globals.h"
#include "tr_model.h"
#include "tr_cvar.h"

#include "vk_init.h"

#include "vk_screenshot.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_image.h"

#include "tr_fog.h"
#include "tr_backend.h"
#include "glConfig.h"
#include "ref_import.h"
#include "R_ShaderCommands.h"
#include "R_ShaderText.h"
#include "tr_font.h"
#include "tr_cmds.h"
#include "tr_noise.h"
#include "tr_scene.h"
#include "render_export.h"
#include "FixRenderCommandList.h"
#include "vk_utils.h"

#include "vk_khr_display.h"

void R_Init( void )
{	
	int i;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );

    R_ClearSortedShaders();

    R_ClearShaderCommand();

    R_ClearBackendState();

	if ( (intptr_t)tess.xyz & 15 ) {
		ri.Printf( PRINT_ALL, "WARNING: tess.xyz not 16 byte aligned\n" );
	}


	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( i * ( 2.0 * M_PI / (float)(FUNCTABLE_SIZE - 1) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

    R_InitDisplayResolution();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	// make sure all the commands added here are also
	// removed in R_Shutdown
    ri.Cmd_AddCommand( "displayResoList", R_DisplayResolutionList_f );
    ri.Cmd_AddCommand( "monitorInfo", vk_displayInfo_f );

    ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
    ri.Cmd_AddCommand( "listSortedShader", listSortedShader_f );

	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );


    ri.Cmd_AddCommand( "vkinfo", printVulkanInfo_f );
    ri.Cmd_AddCommand( "printDeviceExtensions", printDeviceExtensionsSupported_f );
    ri.Cmd_AddCommand( "printInstanceExtensions", printInstanceExtensionsSupported_f );

    ri.Cmd_AddCommand( "minimize", vk_minimizeWindowImpl );

    ri.Cmd_AddCommand( "pipelineList", R_PipelineList_f );

    ri.Cmd_AddCommand( "gpuMem", gpuMemUsageInfo_f );

    ri.Cmd_AddCommand( "printOR", R_PrintBackEnd_OR_f );

    ri.Cmd_AddCommand( "printImgHashTable", printImageHashTable_f );

    ri.Cmd_AddCommand( "printShaderTextHashTable", printShaderTextHashTable_f);

    R_InitScene();

    glConfig_Init();

    // VULKAN
	if ( !isVKinitialied() )
	{
		vk_initialize();
        
        glConfig_FillString();
        // print info
        // vulkanInfo_f();
	}
    
    vk_InitShaderStagePipeline();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

    ri.Printf( PRINT_ALL, "----- R_Init finished -----\n" );
}




void RE_Shutdown( qboolean destroyWindow )
{	

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );
    
    ri.Cmd_RemoveCommand("displayResoList");
    ri.Cmd_RemoveCommand("monitorInfo");

	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("skinlist");

    ri.Cmd_RemoveCommand("minimize");
	
	ri.Cmd_RemoveCommand("vkinfo");
    ri.Cmd_RemoveCommand("printDeviceExtensions");
    ri.Cmd_RemoveCommand("printInstanceExtensions");


    ri.Cmd_RemoveCommand("pipelineList");
    ri.Cmd_RemoveCommand("gpuMem");
    ri.Cmd_RemoveCommand("printOR");
    ri.Cmd_RemoveCommand("printImgHashTable");
    ri.Cmd_RemoveCommand("listSortedShader");

    ri.Cmd_RemoveCommand("printShaderTextHashTable");


	R_DoneFreeType();

    // VULKAN
    // Releases vulkan resources allocated during program execution.
    // This effectively puts vulkan subsystem into initial state 
    // (the state we have after vk_initialize call).

    // contains vulkan resources/state, reinitialized on a map change.
    
    R_ClearSortedShaders();

    vk_destroyShaderStagePipeline();
 
    vk_resetGeometryBuffer();

	if ( tr.registered )
    {	
		vk_destroyImageRes();
        tr.registered = qfalse;
	}

    if (destroyWindow)
    {
        vk_shutdown();
        vk_destroyWindowImpl();
        
        // It is cleared not for renderer_vulkan,
        // but fot rendergl1, renderergl2 to create the window
        glConfig_Clear();
    }

}


void RE_BeginRegistration(glconfig_t * const pGlCfg)
{
	R_Init();
    
    glConfig_Get(pGlCfg);

	tr.viewCluster = -1; // force markleafs to regenerate

	RE_ClearScene();

	tr.registered = qtrue;

   	ri.Printf(PRINT_ALL, "RE_BeginRegistration finished.\n");
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void )
{
	if ( tr.registered ) {
		R_IssueRenderCommands( qfalse );
	}
}
