void vk_getInstanceProcAddrImpl(void)
{

    qvkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)Sys_LoadFunction(vk_library_handle, "vkGetInstanceProcAddr");
    if( qvkGetInstanceProcAddr == NULL)
    {
        ri.Error(ERR_FATAL, "Failed to find entrypoint vkGetInstanceProcAddr\n"); 
    }
    
    ri.Printf(PRINT_ALL,  " Get instance proc address. (using XCB)\n");
}
