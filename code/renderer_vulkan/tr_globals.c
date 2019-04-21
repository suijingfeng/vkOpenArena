#include "tr_globals.h"
#include "tr_world.h"
#include "tr_model.h"
#include "ref_import.h"

trGlobals_t		tr;


void R_ShaderList_f (void)
{
	int	i;
	int	count = tr.numShaders;
 

    ri.Printf (PRINT_ALL, " -------- tr.shaders: %d -------- \n", count);

    if ( ri.Cmd_Argc() > 1 )
    {
        ri.Printf (PRINT_ALL, "%s", ri.Cmd_Argv(1));
    }

	for ( i = 0; i < count; ++i )
    {
		ri.Printf( PRINT_ALL, "%i ", tr.shaders[i]->numUnfoggedPasses );

		if (tr.shaders[i]->lightmapIndex >= 0 )
			ri.Printf (PRINT_ALL, "L ");
		else
			ri.Printf (PRINT_ALL, "  ");


		if ( tr.shaders[i]->multitextureEnv == GL_ADD )
			ri.Printf( PRINT_ALL, "MT(a) " );
		else if ( tr.shaders[i]->multitextureEnv == GL_MODULATE )
			ri.Printf( PRINT_ALL, "MT(m) " );
		else
			ri.Printf( PRINT_ALL, "      " );
	

		if ( tr.shaders[i]->explicitlyDefined )
			ri.Printf( PRINT_ALL, "E " );
		else
			ri.Printf( PRINT_ALL, "  " );


		if ( tr.shaders[i]->isSky )
			ri.Printf( PRINT_ALL, "sky " );
		else
			ri.Printf( PRINT_ALL, "gen " );


		if ( tr.shaders[i]->defaultShader )
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", tr.shaders[i]->name);
		else
			ri.Printf (PRINT_ALL,  ": %s\n", tr.shaders[i]->name);
	}
	ri.Printf (PRINT_ALL, "-------------------\n");
}

