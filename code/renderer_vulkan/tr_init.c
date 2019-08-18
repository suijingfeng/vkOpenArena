// tr_init.c -- functions that are not called every frame

#include "tr_globals.h"
#include "tr_model.h"
#include "tr_cvar.h"

#include "vk_init.h"

#include "vk_screenshot.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "vk_image.h"
#include "vk_cinematic.h"

#include "tr_fog.h"
#include "tr_backend.h"
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
#include "vk_buffers.h"
#include "vk_khr_display.h"
#include "vk_descriptor_sets.h"

static glconfig_t glConfig;

void glConfig_Init(void)
{
    ri.Printf(PRINT_ALL,  "--- R_glConfigInit() ---\n");

    // These values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

    // Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
    glConfig.deviceSupportsGamma = qtrue;

    glConfig.textureEnvAddAvailable = 0; // not used
    glConfig.textureCompression = TC_NONE; // not used
	// init command buffers and SMP
	glConfig.stereoEnabled = 0;
	glConfig.smpActive = qfalse; // not used

    // hardcode it
    glConfig.colorBits = 32;
    glConfig.depthBits = 24;
    glConfig.stencilBits = 8;
}


static void glConfig_FillString( void )
{
    ri.Printf( PRINT_ALL, "\nActive 3D API: Vulkan\n" );

    // To query general properties of physical devices once enumerated
    VkPhysicalDeviceProperties props;
    
    qvkGetPhysicalDeviceProperties(vk.physical_device, &props);

    uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
    uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
    uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

    const char* device_type;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        device_type = " INTEGRATED_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        device_type = " DISCRETE_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        device_type = " VIRTUAL_GPU";
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        device_type = " CPU";
    else
        device_type = " Unknown";

    const char* vendor_name = "unknown";
    if (props.vendorID == 0x1002) {
        vendor_name = "AMD";
    } else if (props.vendorID == 0x10DE) {
        vendor_name = "NVIDIA";
    } else if (props.vendorID == 0x8086) {
        vendor_name = "INTEL";
    }


    char tmpBuf[128] = {0};
    snprintf(tmpBuf, 128, " Vk api version: %d.%d.%d ", major, minor, patch);
    strncpy( glConfig.version_string, tmpBuf, sizeof( glConfig.version_string ) );
	strncpy( glConfig.vendor_string, vendor_name, sizeof( glConfig.vendor_string ) );
    strncpy( glConfig.renderer_string, props.deviceName, sizeof( glConfig.renderer_string ) );
    strcat(glConfig.renderer_string, device_type);

    if (*glConfig.renderer_string && 
            glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
         glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;  
    
	
    /////////////////// extention /////////////////////

    uint32_t nDevExts = 0;

    // To query the extensions available to a given physical device
    VK_CHECK( qvkEnumerateDeviceExtensionProperties( vk.physical_device, NULL, &nDevExts, NULL) );

    assert(nDevExts > 0);

    VkExtensionProperties* pDevExt = 
        (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * nDevExts);

    qvkEnumerateDeviceExtensionProperties(
            vk.physical_device, NULL, &nDevExts, pDevExt);


    uint32_t indicator = 0;
    
    // There much more device extentions, beyound UI driver info can display
    for (uint32_t i = 0; i < nDevExts; ++i)
    {   
        uint32_t len = (uint32_t)strlen(pDevExt[i].extensionName);
        memcpy(glConfig.extensions_string + indicator, pDevExt[i].extensionName, len);
        indicator += len;
        glConfig.extensions_string[indicator++] = ' ';
    }
    
    free(pDevExt);
}


void R_Init( void )
{	

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );

    R_ClearSortedShaders();

    R_ClearShaderCommand();

    R_ClearBackendState();

	ri.Printf(PRINT_ALL, "Init function tables. \n");
	//
	// init function tables
	//
	for (uint32_t i = 0; i < FUNCTABLE_SIZE; ++i )
	{
		tr.sinTable[i] = sinf( i * ( 2.0 * M_PI / (float)(FUNCTABLE_SIZE - 1) ) );
		tr.squareTable[i] = ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
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

	ri.Printf(PRINT_ALL, "R_InitDisplayResolution. \n");


	R_InitFogTable();
	ri.Printf(PRINT_ALL, "R_InitFogTable. \n");
	R_NoiseInit();
	ri.Printf(PRINT_ALL, "R_NoiseInit. \n");
	R_Register();

	// make sure all the commands added here are also
	// removed in R_Shutdown

    ri.Cmd_AddCommand( "monitorInfo", vk_displayInfo_f );

    ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
    ri.Cmd_AddCommand( "listSortedShader", listSortedShader_f );

	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );

    ri.Cmd_AddCommand( "pipelineList", R_PipelineList_f );



    ri.Cmd_AddCommand( "printOR", R_PrintBackEnd_OR_f );
    ri.Cmd_AddCommand( "vkinfo", printVulkanInfo_f );
    ri.Cmd_AddCommand( "printMemUsage", printMemUsageInfo_f );
    ri.Cmd_AddCommand( "printDeviceExtensions", printDeviceExtensionsSupported_f );
    ri.Cmd_AddCommand( "printInstanceExtensions", printInstanceExtensionsSupported_f );
    ri.Cmd_AddCommand( "printImgHashTable", printImageHashTable_f );
    ri.Cmd_AddCommand( "printShaderTextHashTable", printShaderTextHashTable_f);
    ri.Cmd_AddCommand( "printPresentModes", printPresentModesSupported_f);
    ri.Cmd_AddCommand( "printSurfaceFormatSupported", printSurfaceFormatSupported_f);

    ri.Cmd_AddCommand( "screenshotBMP", R_ScreenShotBMP_f );
    ri.Cmd_AddCommand( "screenshotPNG", R_ScreenShotPNG_f );
    ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
    ri.Cmd_AddCommand( "screenshotJPG", R_ScreenShotJPG_f );
    ri.Cmd_AddCommand( "screenshotTGA", R_ScreenShotTGA_f );

	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );

    R_InitScene();


    // VULKAN
    if ( !isVKinitialied() )
    {
        ri.Printf(PRINT_ALL, " Create window fot vulkan . \n");

        // This function set the render window's height and width.
        void * pWinContext;

        // Create window.
        ri.WinSysInit(&pWinContext);

        vk_initialize(pWinContext);

        // print info
        // vulkanInfo_f();
    }
    
	vk_InitShaderStagePipeline();
	
	vk_initScratchImage();

	//vk_createStagingBuffer(1024*1024);

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

    R_Set2dProjectMatrix(vk.renderArea.extent.width, vk.renderArea.extent.height);

    ri.Printf( PRINT_ALL, "----- R_Init finished -----\n" );
}




void RE_Shutdown( qboolean destroyWindow )
{	

    ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );
    
    ri.Cmd_RemoveCommand("monitorInfo");

    ri.Cmd_RemoveCommand("modellist");

    ri.Cmd_RemoveCommand("shaderlist");
    ri.Cmd_RemoveCommand("skinlist");

	
    ri.Cmd_RemoveCommand("vkinfo");
    ri.Cmd_RemoveCommand("printDeviceExtensions");
    ri.Cmd_RemoveCommand("printInstanceExtensions");


    ri.Cmd_RemoveCommand("pipelineList");
    ri.Cmd_RemoveCommand("gpuMem");
    ri.Cmd_RemoveCommand("printOR");
    ri.Cmd_RemoveCommand("printImgHashTable");
    ri.Cmd_RemoveCommand("listSortedShader");

    ri.Cmd_RemoveCommand("printShaderTextHashTable");
    
    ri.Cmd_RemoveCommand("printPresentModes");
    ri.Cmd_RemoveCommand("printSurfaceFormatSupported");

    ri.Cmd_RemoveCommand("screenshotPNG");
    ri.Cmd_RemoveCommand("screenshotBMP");
    ri.Cmd_RemoveCommand("screenshotJPEG");
    ri.Cmd_RemoveCommand("screenshotJPG");
    ri.Cmd_RemoveCommand("screenshotTGA");
    ri.Cmd_RemoveCommand("screenshot");
    
    R_DoneFreeType();

    // VULKAN
    // Releases vulkan resources allocated during program execution.
    // This effectively puts vulkan subsystem into initial state 
    // (the state we have after vk_initialize call).

    // contains vulkan resources/state, reinitialized on a map change.
        
    R_ClearSortedShaders();

    vk_resetGeometryBuffer();

    vk_clearScreenShotManager();

    NO_CHECK( qvkDeviceWaitIdle(vk.device) );


    if ( tr.registered )
    {
	vk_destroyScratchImage();
		
	vk_destroyImageRes(); 
		
	// need ?
	vk_reset_descriptor_pool();
		
        vk_destroyStagingBuffer();
        
        tr.registered = qfalse;
    }


    vk_destroyShaderStagePipeline();



    if (destroyWindow)
    {
        vk_shutdown();
		
		VK_CleanInstanceProcAddrImpl();

		ri.WinSysShutdown();

		ri.Printf(PRINT_ALL, " Destroying Vulkan window. \n");
    }
}


void RE_BeginRegistration(glconfig_t * const pConfig)
{
	R_Init();
    
	// *pGlCfg = glConfig;

    pConfig->isFullscreen =  qfalse;
	pConfig->stereoEnabled = qfalse;
	pConfig->smpActive = qfalse;
	pConfig->displayFrequency = 60;
	// allways enable stencil
	pConfig->stencilBits = 8;
	pConfig->depthBits = 24;
	pConfig->colorBits = 32;
	pConfig->deviceSupportsGamma = qfalse;

    pConfig->textureEnvAddAvailable = 0; // not used
    pConfig->textureCompression = 0; // not used

    // These values force the UI to disable driver selection
	pConfig->driverType = GLDRV_ICD;
	pConfig->hardwareType = GLHW_GENERIC;

    pConfig->vidWidth = vk_getWinWidth();
    pConfig->vidHeight = vk_getWinHeight();

    glConfig_FillString();

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
