            int sky_mins_subd[2], sky_maxs_subd[2];
            int s, t;

            sky_mins[0][i] = floor( sky_mins[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
            sky_mins[1][i] = floor( sky_mins[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
            sky_maxs[0][i] = ceil( sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;
            sky_maxs[1][i] = ceil( sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS ) / HALF_SKY_SUBDIVISIONS;

            if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) ||
                    ( sky_mins[1][i] >= sky_maxs[1][i] ) )
            {
                continue;
            }

            sky_mins_subd[0] = sky_mins[0][i] * HALF_SKY_SUBDIVISIONS;
            sky_mins_subd[1] = sky_mins[1][i] * HALF_SKY_SUBDIVISIONS;
            sky_maxs_subd[0] = sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS;
            sky_maxs_subd[1] = sky_maxs[1][i] * HALF_SKY_SUBDIVISIONS;

            if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
                sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
            else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS ) 
                sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
            if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )
                sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
            else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS ) 
                sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

            if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS ) 
                sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
            else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS ) 
                sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
            if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS ) 
                sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
            else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS ) 
                sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

            //
            // iterate through the subdivisions
            //
            for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t <= sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
            {
                for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s <= sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
                {
                    MakeSkyVec( ( s - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
                            ( t - HALF_SKY_SUBDIVISIONS ) / ( float ) HALF_SKY_SUBDIVISIONS, 
                            i, 
                            s_skyTexCoords[t][s], 
                            s_skyPoints[t][s] );
                }
            }


            // VULKAN: draw skybox side

            updateCurDescriptor(tess.shader->sky.outerbox[sky_texorder[i]]->descriptor_set, 0);

            tess.numVertexes = 0;
            tess.numIndexes = 0;

            for ( t = sky_mins_subd[1]+HALF_SKY_SUBDIVISIONS; t < sky_maxs_subd[1]+HALF_SKY_SUBDIVISIONS; t++ )
            {
                for ( s = sky_mins_subd[0]+HALF_SKY_SUBDIVISIONS; s < sky_maxs_subd[0]+HALF_SKY_SUBDIVISIONS; s++ )
                {
                    int ndx = tess.numVertexes;

                    tess.indexes[ tess.numIndexes ] = ndx;
                    tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
                    tess.indexes[ tess.numIndexes + 2 ] = ndx + 2;

                    tess.indexes[ tess.numIndexes + 3 ] = ndx + 2;
                    tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
                    tess.indexes[ tess.numIndexes + 5 ] = ndx + 3;
                    tess.numIndexes += 6;

                    VectorCopy(s_skyPoints[t][s], tess.xyz[ndx]);
                    tess.svars.texcoords[0][ndx][0] = s_skyTexCoords[t][s][0];
                    tess.svars.texcoords[0][ndx][1] = s_skyTexCoords[t][s][1];

                    VectorCopy(s_skyPoints[t + 1][s], tess.xyz[ndx + 1]);
                    tess.svars.texcoords[0][ndx + 1][0] = s_skyTexCoords[t + 1][s][0];
                    tess.svars.texcoords[0][ndx + 1][1] = s_skyTexCoords[t + 1][s][1];

                    VectorCopy(s_skyPoints[t][s + 1], tess.xyz[ndx + 2]);
                    tess.svars.texcoords[0][ndx + 2][0] = s_skyTexCoords[t][s + 1][0];
                    tess.svars.texcoords[0][ndx + 2][1] = s_skyTexCoords[t][s + 1][1];

                    VectorCopy(s_skyPoints[t + 1][s + 1], tess.xyz[ndx + 3]);
                    tess.svars.texcoords[0][ndx + 3][0] = s_skyTexCoords[t + 1][s + 1][0];
                    tess.svars.texcoords[0][ndx + 3][1] = s_skyTexCoords[t + 1][s + 1][1];

                    tess.numVertexes += 4;
                }
            }

            memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
        {
            float skybox_translate[16] QALIGN(16) = {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1], backEnd.viewParms.or.origin[2], 1
            };

            float tmp[16] QALIGN(16);
            MatrixMultiply4x4_SSE(skybox_translate, getptr_modelview_matrix(), tmp);
            updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, tmp);
        }
            vk_UploadXYZI(tess.xyz, tess.numVertexes, tess.indexes, tess.numIndexes);

            vk_shade_geometry(g_stdPipelines.skybox_pipeline, VK_FALSE, r_showsky->integer ? DEPTH_RANGE_ZERO : DEPTH_RANGE_ONE, VK_TRUE);

        }
