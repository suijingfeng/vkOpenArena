#ifndef FIX_RENDER_COMMNAD_LIST_H_
#define FIX_RENDER_COMMNAD_LIST_H_

//void FixRenderCommandList( int newShader );
//void SortNewShader( shader_t* pShader );
struct shader_s * R_GeneratePermanentShader(struct shaderStage_s * pStgTab, struct shader_s * pShader );
void R_DecomposeSort( unsigned sort, int *entityNum, struct shader_s **shader,
					 int *fogNum, int *dlightMap );

void R_ClearSortedShaders(void);


void listSortedShader_f (void);

#endif
