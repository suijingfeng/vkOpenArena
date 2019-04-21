#ifndef FIX_RENDER_COMMNAD_LIST_H_
#define FIX_RENDER_COMMNAD_LIST_H_

//void FixRenderCommandList( int newShader );
//void SortNewShader( shader_t* pShader );
shader_t* R_GeneratePermanentShader(shaderStage_t* pStgTab, shader_t* pShader );
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );

void R_ClearSortedShaders(void);


void listSortedShader_f (void);

#endif
