# FSR3 独立模组开发规则

## 📅 记录时间
- **记录时间**: 2026-03-11 04:42 GMT+8
- **开发者**: 主人
- **负责人**: 打工人 (AI助理)

## 📋 规则分类

### 一、核心原则
- 代码必须真实可运行，禁止占位符、TODO、模拟实现。
- 每个功能完成后必须有明确的验证方法。
- 所有 API 调用必须检查返回值，失败时输出详细错误信息。
- 资源必须正确释放，遵循 RAII 原则。
- 共享数据必须用互斥锁保护，确保线程安全。

### 二、FSR3 核心数据要求
- **运动矢量**: 必须从顶点着色器生成，基于当前帧和上一帧变换矩阵，格式为屏幕空间像素位移。
- **深度缓冲**: 正确配置深度反转、无限远平面标志，并传递给 FSR3。
- **相机抖动**: 每帧应用 Halton(2,3) 序列子像素抖动。
- **响应性掩码**: 标记透明物体、粒子、UI 等区域（可选但推荐）。
- **UI 分离处理**: UI 必须渲染到独立图像，在 FSR3 处理后合成。
- **严禁**使用随机数或常量代替真实数据。

### 三、独立性与依赖约束
- 禁止调用任何第三方模组 API（如 VulkanMod、OptiFine）。
- 图形后端必须直接使用标准 Vulkan API，不得依赖封装库。
- 只能参考其他项目的思路，不得复制代码。

### 四、技术栈
- Java 层: Minecraft 1.20.1 Forge，通过 Mixin 修改渲染流程。
- 渲染层: Vulkan SDK 1.3.280+，自包含实现。
- FSR3 集成: AMD FidelityFX SDK 的 Vulkan 后端。
- 通信: JNI。

### 五、代码质量规范
1. **模块化设计**: 分离 Vulkan 初始化、纹理共享、FSR3 调度、UI 合成。
2. **调试优先**: 运动矢量可视化、抖动验证、帧 ID 连续性检查。
3. **兼容性检测**: 启动时检查 Vulkan 版本、硬件能力，自动降级并提示。
4. **配置热重载**: 支持实时修改配置。
5. **日志分级**: INFO（状态）、DEBUG（细节）、WARN（降级）、ERROR（崩溃信息）。
6. **代码风格统一**: 
   - Java 使用 Forge 风格
   - C++ 使用 snake_case 函数、CamelCase 类型
7. **资源泄漏防护**: 使用 RAII/智能指针管理 Vulkan 对象。

### 六、项目结构要求
- 最终发布为单个 JAR 文件
- 包含 Java 类、资源、解压加载的原生库和 AMD 运行时 DLL
- SDK 核心文件复制到项目目录，不依赖系统全局安装

### 七、垃圾文件清理规则
1. 维护 .gitignore，忽略 build/、run/、*.log、*.tmp 等
2. 定期清理未使用的资源文件（如着色器、语言文件）
3. 游戏退出时主动删除解压的临时 DLL
4. 配置 gradle clean 任务彻底清理中间文件
5. 发布前执行完整清理并检查 JAR 内容

### 八、开发流程建议
1. 实现 Vulkan 基础渲染
2. 添加运动矢量生成
3. 集成深度处理
4. 实现响应性掩码
5. 集成 FSR3 核心算法
6. 添加 UI 分离处理
7. 全面调试与优化

## 🎯 FSRRenderer 完整实现流程

### 一、核心设计理念
1. **降低内部渲染分辨率**: 游戏以较低分辨率渲染，通过 FSR 放大到显示分辨率
2. **零拷贝共享内存**: OpenGL 和 Vulkan 共享同一块 GPU 内存
3. **Vulkan 计算 FSR**: Vulkan 直接读取共享内存执行 EASU 和 RCAS
4. **无缝显示**: OpenGL 直接使用处理后的共享纹理 blit 到屏幕
5. **精确同步**: 使用跨 API 信号量确保渲染-计算-显示顺序

### 二、详细实现步骤（8步）
#### 第1步: 确认 OpenGL 扩展支持
- 检测 GL_EXT_memory_object、GL_EXT_semaphore 等扩展
- 动态选择零拷贝模式或回退到标准模式

#### 第2步: Vulkan 端创建可导出的图像和内存
- 创建 Vulkan 图像（带 VkExternalMemoryImageCreateInfo）
- 分配可导出内存（带 VkExportMemoryAllocateInfo）
- 获取内存句柄（vkGetMemoryWin32HandleKHR/vkGetMemoryFdKHR）
- 创建可导出信号量

#### 第3步: OpenGL 端导入 Vulkan 内存并创建共享纹理
- 导入内存对象（glImportMemoryWin32HandleEXT/glImportMemoryFdEXT）
- 创建纹理并绑定内存（glTextureStorageMem2DEXT）
- 导入信号量（glImportSemaphoreWin32HandleEXT/glImportSemaphoreFdEXT）

#### 第4步: 修改渲染目标
- 创建自定义 FBO，将共享纹理作为颜色附着点
- 在 beginFrame() 中绑定 FBO，设置低分辨率视口
- 确保 Minecraft 渲染到 FBO

#### 第5步: 修改 endFrame() 实现同步和 FSR 触发
1. 恢复 OpenGL 状态
2. OpenGL 发出信号量（glSignalSemaphoreEXT）
3. 触发 Vulkan FSR 计算（JNI 调用）
4. OpenGL 等待信号量（glWaitSemaphoreEXT）
5. 将共享纹理 blit 到屏幕

#### 第6步: 实现分辨率缩放
- 方法A: 修改 Minecraft 的 windowWidth/windowHeight（推荐）
- 方法B: 调整视口和投影矩阵
- 方法C: 使用 framebuffer blit

#### 第7步: 处理多重采样和深度缓冲
- 禁用 MSAA 或后处理前解析
- 如需共享深度纹理，创建格式匹配的共享深度纹理

#### 第8步: 资源管理与清理
- 窗口大小变化时重新创建共享纹理
- 模组卸载时正确释放所有资源

### 三、代码修改示例
#### 3.1 FSR3GLVulkanInterop 类
- 检测扩展支持
- createSharedTextureFromVulkan() 方法
- importSemaphore()、signalSemaphore()、waitSemaphore() 方法

#### 3.2 OptimizedGLVulkanFSR 类
- 创建共享纹理和 FBO
- beginFrame(): 绑定 FBO，设置低分辨率视口
- endFrame(): 同步信号量，触发 FSR，blit 到屏幕

#### 3.3 C++ 层 nativeRunFSR
- 接收 Vulkan 图像句柄
- 创建计算管线执行 FSR
- 正确同步：等待 OpenGL 信号量，完成后发出 Vulkan 信号量

### 四、调试与验证
1. **验证扩展启用**: 检查日志中的扩展支持情况
2. **验证共享内存**: 使用 RenderDoc 抓帧检查内存地址
3. **验证同步**: 检查画面是否撕裂、闪烁
4. **性能测量**: 对比启用零拷贝前后的帧率和 GPU 时间
5. **处理异常**: 扩展不支持时自动回退到标准模式

## 📁 已创建的文件
1. **GameRendererMixin.java** - Mixin 拦截器
2. **ZeroCopyFSRRenderer.java** - 核心渲染管理器
3. **FSR3GLVulkanInterop.java** - OpenGL-Vulkan 互操作
4. **vulkan_fsr.cpp** - Vulkan FSR JNI 实现

## 🔄 下一步工作
1. 创建 **FSR3Mod.java** - 主类
2. 创建 **build.gradle** - 构建配置
3. 创建 **mixins.json** - Mixin 配置
4. 整合所有模块进行测试

---
**记录完成时间**: 2026-03-11 04:43 GMT+8
**文件位置**: `C:\Users\34730\.openclaw-autoclaw\workspace\FSR3独立模组开发规则.md`