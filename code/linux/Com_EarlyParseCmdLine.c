/*
===================
returns qtrue if both vid_xpos and vid_ypos was set
===================
*/
qboolean Com_EarlyParseCmdLine( char *commandLine, char *con_title, int title_size, int *vid_xpos, int *vid_ypos ) 
{
	int		flags = 0;
	int		i;
	
	*con_title = '\0';
	Com_ParseCommandLine( commandLine );

	for ( i = 0 ; i < com_numConsoleLines ; i++ )
    {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmpn( Cmd_Argv(0), "set", 3 ) && !Q_stricmp( Cmd_Argv(1), "con_title" ) )
        {
			com_consoleLines[i][0] = '\0';
			Q_strncpyz( con_title, Cmd_ArgsFrom( 2 ), title_size );
			continue;
		}
		if ( !Q_stricmp( Cmd_Argv(0), "con_title" ) )
        {
			com_consoleLines[i][0] = '\0';
			Q_strncpyz( con_title, Cmd_ArgsFrom( 1 ), title_size );
			continue;
		}
		if ( !Q_stricmpn( Cmd_Argv(0), "set", 3 ) && !Q_stricmp( Cmd_Argv(1), "vid_xpos" ) )
        {
			*vid_xpos = atoi( Cmd_Argv( 2 ) );
			flags |= 1;
			continue;
		}
		if ( !Q_stricmp( Cmd_Argv(0), "vid_xpos" ) )
        {
			*vid_xpos = atoi( Cmd_Argv( 1 ) );
			flags |= 1;
			continue;
		}
		if ( !Q_stricmpn( Cmd_Argv(0), "set", 3 ) && !Q_stricmp( Cmd_Argv(1), "vid_ypos" ) )
        {
			*vid_ypos = atoi( Cmd_Argv( 2 ) );
			flags |= 2;
			continue;
		}
		if ( !Q_stricmp( Cmd_Argv(0), "vid_ypos" ) )
        {
			*vid_ypos = atoi( Cmd_Argv( 1 ) );
			flags |= 2;
			continue;
		}
		if ( !Q_stricmpn( Cmd_Argv(0), "set", 3 ) && !Q_stricmp( Cmd_Argv(1), "rconPassword2" ) )
        {
			com_consoleLines[i][0] = '\0';
			Q_strncpyz( rconPassword2, Cmd_Argv( 2 ), sizeof( rconPassword2 ) );
			continue;
		}
	}

	return (flags == 3) ? qtrue : qfalse ;
}
