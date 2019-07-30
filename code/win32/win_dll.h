#pragma once

// general development dll loading for virtual machine testing
// fqpath param added 7/20/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
void* QDECL Sys_LoadDll( const char * const name, char * fqpath,
	intptr_t( QDECL **entryPoint)(int, ...),
	intptr_t( QDECL *systemcalls)(intptr_t, ...) );

void Sys_UnloadDll(void * const dllHandle);