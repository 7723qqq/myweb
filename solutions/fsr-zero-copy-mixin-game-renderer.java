package com.framegeneration.mixin;

import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.inject.Injector;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.CallbackInfo;
import com.framegeneration.renderer.ZeroCopyFSRRenderer;

/**
 * GameRenderer 拦截器 - ZERO-COPY FSR 渲染管道入口
 * 
 * HEAD 点插入：在 Minecraft 开始渲染前设置零拷贝状态
 * RETURN 点插入：在 Minecraft 渲染完成后启动 Vulkan FSR 计算
 */
@Mixin(GameRenderer.class)
public class GameRendererMixin {

    @Inject(
        method = "render(Lnet/minecraft/client/GameInfo;ZDD)V",
        at = @At("HEAD"),
        cancellable = false,
        remap = false
    )
    private void onRenderHead(net.minecraft.client.GameInfo gameInfo, boolean blizz, double d, double e, CallbackInfo ci) {
        ZeroCopyFSRRenderer renderer = ZeroCopyFSRRenderer.getInstance();
        
        // HEAD 阶段：准备零拷贝渲染状态
        // - 绑定低分辨率共享纹理 FBO
        // - 设置视口为低分辨率（40%~80% 原分辨率）
        // - 保存原始 OpenGL 状态
        
        if (!renderer.beginFrame()) {
            // 初始化失败，记录错误并回退到标准模式
            renderer.logFatal("ZeroCopyFSRRenderer BEGIN_FRAME failed, falling back to standard mode");
            // 保持渲染继续，但使用 CPU 拷贝路径（如果已初始化）或原生渲染
        }
        
        // HEAD 阶段无需 cancellable = true，让渲染继续执行
    }

    @Inject(
        method = "render(Lnet/minecraft/client/GameInfo;ZDD)V",
        at = @At("RETURN"),
        remap = false
    )
    private void onRenderReturn(net.minecraft.client.GameInfo gameInfo, boolean blizz, double d, double e, CallbackInfo ci) {
        ZeroCopyFSRRenderer.getInstance().endFrame();
    }
}
