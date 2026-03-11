// vulkan_fsr.cpp - 续接部分

/**
 * 创建可导出纹理（续接）
 */
extern "C" JNIEXPORT jlong JNICALL
Java_com_yourmod_renderer_FSR3GLVulkanInterop_nativeCreateExportableTexture(
    JNIEnv* env,
    jclass clazz,
    jint width,
    jint height,
    jint format
) {
    if (!g_vkContext.initialized) {
        logMessage(LOG_ERROR, "Vulkan not initialized");
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(g_resourceMutex);
    
    try {
        logMessage(LOG_DEBUG, "Creating exportable texture: %dx%d, format=%d", width, height, format);
        
        // 1. 创建 Vulkan 图像
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // 匹配 GL_RGBA8
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | 
                         VK_IMAGE_USAGE_SAMPLED_BIT | 
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;
        
        // 外部内存创建信息
        VkExternalMemoryImageCreateInfo externalInfo = {};
        externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#ifdef _WIN32
        externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        imageInfo.pNext = &externalInfo;
        
        VkImage image;
        VK_CHECK(vkCreateImage(g_vkContext.device, &imageInfo, nullptr, &image));
        
        // 2. 获取内存需求
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(g_vkContext.device, image, &memRequirements);
        
        // 3. 分配可导出内存
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = 0; // 需要实际查询合适的类型
        
        VkExportMemoryAllocateInfo exportAllocInfo = {};
        exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN32
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        allocInfo.pNext = &exportAllocInfo;
        
        VkDeviceMemory memory;
        VK_CHECK(vkAllocateMemory(g_vkContext.device, &allocInfo, nullptr, &memory));
        
        // 4. 绑定内存到图像
        VK_CHECK(vkBindImageMemory(g_vkContext.device, image, memory, 0));
        
        // 5. 创建图像视图
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VkImageView imageView;
        VK_CHECK(vkCreateImageView(g_vkContext.device, &viewInfo, nullptr, &imageView));
        
        // 6. 获取外部内存句柄
#ifdef _WIN32
        VkExternalMemoryHandleTypeFlagBits handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        VkExternalMemoryHandleTypeFlagBits handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        uint64_t externalHandle = getExternalMemoryHandle(memory, handleType);
        
        if (externalHandle == 0) {
            logMessage(LOG_ERROR, "Failed to get external memory handle");
            vkDestroyImageView(g_vkContext.device, imageView, nullptr);
            vkDestroyImage(g_vkContext.device, image, nullptr);
            vkFreeMemory(g_vkContext.device, memory, nullptr);
            return 0;
        }
        
        // 7. 创建纹理资源对象
        TextureResource* resource = new TextureResource();
        resource->image = image;
        resource->memory = memory;
        resource->view = imageView;
        resource->format = VK_FORMAT_R8G8B8A8_UNORM;
        resource->width = static_cast<uint32_t>(width);
        resource->height = static_cast<uint32_t>(height);
        resource->externalHandle = externalHandle;
        resource->isExported = true;
        resource->isImported = false;
        
        // 8. 存储到映射表
        jlong resourceId = reinterpret_cast<jlong>(resource);
        g_textureResources[resourceId] = resource;
        
        logMessage(LOG_INFO, "Created exportable texture: handle=0x%llx, size=%dx%d", 
                  externalHandle, width, height);
        
        return resourceId;
        
    } catch (const std::exception& e) {
        logMessage(LOG_ERROR, "createExportableTexture failed: %s", e.what());
        return 0;
    }
}

/**
 * 创建可导出信号量
 */
extern "C" JNIEXPORT jlong JNICALL
Java_com_yourmod_renderer_FSR3GLVulkanInterop_nativeCreateExportableSemaphore(
    JNIEnv* env,
    jclass clazz
) {
    if (!g_vkContext.initialized) {
        logMessage(LOG_ERROR, "Vulkan not initialized");
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(g_resourceMutex);
    
    try {
        // 1. 创建信号量
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        // 外部信号量创建信息
        VkExportSemaphoreCreateInfo exportInfo = {};
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#ifdef _WIN32
        exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        semaphoreInfo.pNext = &exportInfo;
        
        VkSemaphore semaphore;
        VK_CHECK(vkCreateSemaphore(g_vkContext.device, &semaphoreInfo, nullptr, &semaphore));
        
        // 2. 获取外部句柄
#ifdef _WIN32
        VkExternalSemaphoreHandleTypeFlagBits handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        VkExternalSemaphoreHandleTypeFlagBits handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        uint64_t externalHandle = getExternalSemaphoreHandle(semaphore, handleType);
        
        if (externalHandle == 0) {
            logMessage(LOG_ERROR, "Failed to get external semaphore handle");
            vkDestroySemaphore(g_vkContext.device, semaphore, nullptr);
            return 0;
        }
        
        // 3. 创建信号量资源对象
        SemaphoreResource* resource = new SemaphoreResource();
        resource->semaphore = semaphore;
        resource->externalHandle = externalHandle;
        resource->isExported = true;
        resource->isImported = false;
        
        // 4. 存储到映射表
        jlong resourceId = reinterpret_cast<jlong>(resource);
        g_semaphoreResources[resourceId] = resource;
        
        logMessage(LOG_INFO, "Created exportable semaphore: handle=0x%llx", externalHandle);
        
        return resourceId;
        
    } catch (const std::exception& e) {
        logMessage(LOG_ERROR, "createExportableSemaphore failed: %s", e.what());
        return 0;
    }
}

/**
 * 运行 FSR 处理（核心函数）
 */
extern "C" JNIEXPORT jboolean JNICALL
Java_com_yourmod_renderer_ZeroCopyFSRRenderer_nativeRunFSR(
    JNIEnv* env,
    jclass clazz,
    jlong inputTextureId,
    jlong outputTextureId,
    jlong waitSemaphoreId,
    jlong signalSemaphoreId,
    jint srcWidth,
    jint srcHeight,
    jint dstWidth,
    jint dstHeight,
    jint fsrVersion
) {
    if (!g_vkContext.initialized) {
        logMessage(LOG_ERROR, "Vulkan not initialized");
        return JNI_FALSE;
    }
    
    std::lock_guard<std::recursive_mutex> lock(g_vkContext.contextMutex);
    
    try {
        logMessage(LOG_DEBUG, "Running FSR: %dx%d -> %dx%d, version=%d", 
                  srcWidth, srcHeight, dstWidth, dstHeight, fsrVersion);
        
        // 1. 获取资源对象
        TextureResource* inputTexture = g_textureResources[inputTextureId];
        TextureResource* outputTexture = g_textureResources[outputTextureId];
        SemaphoreResource* waitSemaphore = g_semaphoreResources[waitSemaphoreId];
        SemaphoreResource* signalSemaphore = g_semaphoreResources[signalSemaphoreId];
        
        if (!inputTexture || !outputTexture) {
            logMessage(LOG_ERROR, "Invalid texture resources");
            return JNI_FALSE;
        }
        
        // 2. 创建命令缓冲区
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = VK_NULL_HANDLE; // 需要创建命令池
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        // VK_CHECK(vkAllocateCommandBuffers(g_vkContext.device, &allocInfo, &commandBuffer));
        
        // 3. 开始命令缓冲区
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        // VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        
        // 4. 图像布局转换
        // transitionImageLayout(commandBuffer, inputTexture->image, 
        //                      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        // transitionImageLayout(commandBuffer, outputTexture->image,
        //                      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        
        // 5. 绑定 FSR 计算管线
        // 这里需要实际的 FSR SDK 集成
        // FfxFsr3DispatchDescription dispatchDesc = {};
        // dispatchDesc.commandList = ffxGetCommandListVK(commandBuffer);
        // dispatchDesc.color = ffxGetTextureResourceVK(inputTexture->image, ...);
        // dispatchDesc.output = ffxGetTextureResourceVK(outputTexture->image, ...);
        // dispatchDesc.renderSize = {srcWidth, srcHeight};
        // dispatchDesc.displaySize = {dstWidth, dstHeight};
        // 
        // ffxFsr3ContextDispatch(&g_vkContext.fsrContext, &dispatchDesc);
        
        // 6. 结束命令缓冲区
        // VK_CHECK(vkEndCommandBuffer(commandBuffer));
        
        // 7. 提交命令缓冲区
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        // 等待信号量
        VkSemaphore waitSemaphores[] = { waitSemaphore ? waitSemaphore->semaphore : VK_NULL_HANDLE };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        
        if (waitSemaphore) {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
        }
        
        // 信号量
        VkSemaphore signalSemaphores[] = { signalSemaphore ? signalSemaphore->semaphore : VK_NULL_HANDLE };
        
        if (signalSemaphore) {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
        }
        
        // 命令缓冲区
        // submitInfo.commandBufferCount = 1;
        // submitInfo.pCommandBuffers = &commandBuffer;
        
        // VK_CHECK(vkQueueSubmit(g_vkContext.computeQueue, 1, &submitInfo, VK_NULL_HANDLE));
        
        // 8. 等待完成
        // VK_CHECK(vkQueueWaitIdle(g_vkContext.computeQueue));
        
        logMessage(LOG_INFO, "FSR processing completed (simulated)");
        
        return JNI_TRUE;
        
    } catch (const std::exception& e) {
        logMessage(LOG_ERROR, "nativeRunFSR failed: %s", e.what());
        return JNI_FALSE;
    }
}

/**
 * 获取纹理的外部句柄
 */
extern "C" JNIEXPORT jlong JNICALL
Java_com_yourmod_renderer_FSR3GLVulkanInterop_nativeGetTextureHandle(
    JNIEnv* env,
    jclass clazz,
    jlong textureId
) {
    std::lock_guard<std::mutex> lock(g_resourceMutex);
    
    TextureResource* resource = g_textureResources[textureId];
    if (!resource) {
        logMessage(LOG_ERROR, "Texture resource not found: %lld", textureId);
        return 0;
    }
    
    if (!resource->isExported) {
        logMessage(LOG_ERROR, "Texture is not exported: %lld", textureId);
        return 0;
    }
    
    return static_cast<jlong>(resource->externalHandle);
}

/**
 * 获取信号量的外部句柄
 */
extern "C" JNIEXPORT jlong JNICALL
Java_com_yourmod_renderer_FSR3GLVulkanInterop_nativeGetSemaphoreHandle(
    JNIEnv* env,
    jclass clazz,
    jlong semaphoreId
) {
    std::lock_guard<std::mutex> lock(g_resourceMutex);
    
    SemaphoreResource* resource = g_semaphoreResources[semaphoreId];
    if (!resource) {
        logMessage(LOG_ERROR, "Semaphore resource not found: %lld", semaphoreId);
        return 0;
    }
    
    if (!resource->isExported) {
        logMessage(LOG_ERROR, "Semaphore is not exported: %lld", semaphoreId);
        return 0;
    }
    
    return static_cast<jlong>(resource->externalHandle);
}

/**
 * 清理 Vulkan 资源（正确资源释放）
 */
extern "C" JNIEXPORT void JNICALL
Java_com_yourmod_renderer_FSR3GLVulkanInterop_nativeCleanup(
    JNIEnv* env,
    jclass clazz
) {
    std::lock_guard<std::mutex> lock(g_resourceMutex);
    std::lock_guard<std::recursive_mutex> contextLock(g_vkContext.contextMutex);
    
    logMessage(LOG_INFO, "Cleaning up Vulkan resources...");
    
    try {
        // 清理纹理资源
        for (auto& pair : g_textureResources) {
            TextureResource* resource = pair.second;
            if (resource) {
                if (resource->view != VK_NULL_HANDLE) {
                    vkDestroyImageView(g_vkContext.device, resource->view, nullptr);
                }
                if (resource->image != VK_NULL_HANDLE) {
                    vkDestroyImage(g_vkContext.device, resource->