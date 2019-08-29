/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
#ifdef USE_RENDERER_DLOPEN
extern "C" __declspec(dllexport) void QDECL GetRefAPI(int apiVersion, refimport_t *const rimp, refexport_t * const pRExp)
#else
void GetRefAPI(int apiVersion, refimport_t *rimp, refexport_t * re)
#endif
{
	ri = *rimp;

	memset(pRExp, 0, sizeof(refexport_t));

	if (apiVersion != REF_API_VERSION)
	{
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
			REF_API_VERSION, apiVersion);

		memset(pRExp, 0, sizeof(refexport_t));
		return;
	}

	// the RE_ functions are Renderer Entry points
	pRExp->Shutdown = RE_Shutdown;
	pRExp->BeginRegistration = RE_BeginRegistration;
	pRExp->RegisterModel = RE_RegisterModel;
	pRExp->RegisterSkin = RE_RegisterSkin;
	pRExp->RegisterShader = RE_RegisterShader;
	pRExp->RegisterShaderNoMip = RE_RegisterShaderNoMip;
	pRExp->LoadWorld = RE_LoadWorldMap;
	pRExp->SetWorldVisData = RE_SetWorldVisData;
	pRExp->EndRegistration = RE_EndRegistration;
	pRExp->ClearScene = RE_ClearScene;
	pRExp->AddRefEntityToScene = RE_AddRefEntityToScene;
	pRExp->AddPolyToScene = RE_AddPolyToScene;
	pRExp->LightForPoint = R_LightForPoint;
	pRExp->AddLightToScene = RE_AddLightToScene;
	pRExp->AddAdditiveLightToScene = RE_AddAdditiveLightToScene;

	pRExp->RenderScene = RE_RenderScene;
	pRExp->SetColor = RE_SetColor;
	pRExp->DrawStretchPic = RE_StretchPic;
	pRExp->DrawStretchRaw = RE_StretchRaw;
	pRExp->UploadCinematic = RE_UploadCinematic;

	pRExp->BeginFrame = RE_BeginFrame;
	pRExp->EndFrame = RE_EndFrame;
	pRExp->MarkFragments = R_MarkFragments;
	pRExp->LerpTag = R_LerpTag;
	pRExp->ModelBounds = R_ModelBounds;
	pRExp->RegisterFont = RE_RegisterFont;
	pRExp->RemapShader = R_RemapShader;
	pRExp->GetEntityToken = R_GetEntityToken;
	pRExp->inPVS = R_inPVS;

	// pRExp->WinMessage = RE_WinMessage;

}