#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
#include "render_export.h"


refimport_t	ri;


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/

#ifdef USE_RENDERER_DLOPEN
Q_EXPORT void QDECL GetRefAPI( int apiVersion, refimport_t *rimp , refexport_t* rexp)
{
#else
refexport_t* GetRefAPI(int apiVersion, refimport_t *rimp)
{
#endif

	ri = *rimp;

	if( apiVersion != REF_API_VERSION )
	{
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	rexp->Shutdown = RE_Shutdown;
	rexp->BeginRegistration = RE_BeginRegistration;
	rexp->RegisterModel = RE_RegisterModel;
	rexp->RegisterSkin = RE_RegisterSkin;
	rexp->RegisterShader = RE_RegisterShader;
	rexp->RegisterShaderNoMip = RE_RegisterShaderNoMip;
	rexp->LoadWorld = RE_LoadWorldMap;
	rexp->SetWorldVisData = RE_SetWorldVisData;
	rexp->EndRegistration = RE_EndRegistration;
	rexp->ClearScene = RE_ClearScene;
	rexp->AddRefEntityToScene = RE_AddRefEntityToScene;
	rexp->AddPolyToScene = RE_AddPolyToScene;
	rexp->LightForPoint = RE_LightForPoint;
	rexp->AddLightToScene = RE_AddLightToScene;
	rexp->AddAdditiveLightToScene = RE_AddAdditiveLightToScene;

	rexp->RenderScene = RE_RenderScene;
	rexp->SetColor = RE_SetColor;
	rexp->DrawStretchPic = RE_StretchPic;
	rexp->DrawStretchRaw = RE_StretchRaw;
	rexp->UploadCinematic = RE_UploadCinematic;

	rexp->BeginFrame = RE_BeginFrame;
	rexp->EndFrame = RE_EndFrame;
	rexp->MarkFragments = RE_MarkFragments;
	rexp->LerpTag = RE_LerpTag;
	rexp->ModelBounds = RE_ModelBounds;
	rexp->RegisterFont = RE_RegisterFont;
	rexp->RemapShader = RE_RemapShader;
	rexp->GetEntityToken = RE_GetEntityToken;
	rexp->inPVS = RE_inPVS;

	rexp->TakeVideoFrame = RE_TakeVideoFrame;

	return;
}
