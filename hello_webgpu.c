#include <stdio.h>
#include <emscripten/emscripten.h>
#include <webgpu/webgpu.h>

#define CANVAS_W 640
#define CANVAS_H 480

static WGPUInstance instance;
static WGPUAdapter adapter;
static WGPUDevice device;
static WGPUQueue queue;
static WGPUSurface surface;
static WGPURenderPipeline pipeline;
static WGPUTextureFormat surfaceFormat;

static const char shaderSrc[] =
    "@vertex fn vs_main(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f {\n"
    "    let pos = array<vec2f, 3>(vec2f(0.0, 0.5), vec2f(-0.5, -0.5), vec2f(0.5, -0.5));\n"
    "    return vec4f(pos[i], 0.0, 1.0);\n"
    "}\n"
    "@fragment fn fs_main() -> @location(0) vec4f {\n"
    "    return vec4f(1.0, 0.3, 0.1, 1.0);\n"
    "}\n";

static void onUncapturedError(WGPUDevice const* dev, WGPUErrorType type, WGPUStringView message, void* ud1, void* ud2) {
    (void)dev; (void)type; (void)ud1; (void)ud2;
    printf("WebGPU error: %.*s\n", (int)message.length, message.data);
}

static void frame(void) {
    WGPUSurfaceTexture surfTex;
    wgpuSurfaceGetCurrentTexture(surface, &surfTex);
    WGPUTextureView view = wgpuTextureCreateView(surfTex.texture, NULL);

    WGPURenderPassColorAttachment colorAtt = {0};
    colorAtt.view = view;
    colorAtt.loadOp = WGPULoadOp_Clear;
    colorAtt.storeOp = WGPUStoreOp_Store;
    colorAtt.clearValue = (WGPUColor){ 0.05, 0.05, 0.08, 1.0 };
    colorAtt.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor passDesc = {0};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &colorAtt;

    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(device, NULL);
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(enc, &passDesc);
    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(pass);

    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, NULL);
    wgpuQueueSubmit(queue, 1, &cmd);

    wgpuCommandBufferRelease(cmd);
    wgpuRenderPassEncoderRelease(pass);
    wgpuCommandEncoderRelease(enc);
    wgpuTextureViewRelease(view);
}

static void initGraphics(void) {
    WGPUSurfaceCapabilities caps = {0};
    wgpuSurfaceGetCapabilities(surface, adapter, &caps);
    surfaceFormat = caps.formats[0];

    WGPUSurfaceConfiguration surfConf = {0};
    surfConf.device = device;
    surfConf.format = surfaceFormat;
    surfConf.usage = WGPUTextureUsage_RenderAttachment;
    surfConf.width = CANVAS_W;
    surfConf.height = CANVAS_H;
    surfConf.presentMode = WGPUPresentMode_Fifo;
    surfConf.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface, &surfConf);

    WGPUShaderSourceWGSL wgsl = {0};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = (WGPUStringView){ shaderSrc, WGPU_STRLEN };

    WGPUShaderModuleDescriptor shaderDesc = {0};
    shaderDesc.nextInChain = &wgsl.chain;
    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(device, &shaderDesc);

    WGPUColorTargetState colorTarget = {0};
    colorTarget.format = surfaceFormat;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState = {0};
    fragState.module = shader;
    fragState.entryPoint = (WGPUStringView){ "fs_main", WGPU_STRLEN };
    fragState.targetCount = 1;
    fragState.targets = &colorTarget;

    WGPURenderPipelineDescriptor pipeDesc = {0};
    pipeDesc.vertex.module = shader;
    pipeDesc.vertex.entryPoint = (WGPUStringView){ "vs_main", WGPU_STRLEN };
    pipeDesc.fragment = &fragState;
    pipeDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeDesc.multisample.count = 1;
    pipeDesc.multisample.mask = 0xFFFFFFFF;

    pipeline = wgpuDeviceCreateRenderPipeline(device, &pipeDesc);
    wgpuShaderModuleRelease(shader);

    emscripten_set_main_loop(frame, 0, 0);
}

static void onDeviceReceived(WGPURequestDeviceStatus status, WGPUDevice result, WGPUStringView message, void* ud1, void* ud2) {
    (void)ud1; (void)ud2;
    if (status != WGPURequestDeviceStatus_Success) {
        printf("RequestDevice failed: %.*s\n", (int)message.length, message.data);
        return;
    }
    device = result;
    queue = wgpuDeviceGetQueue(device);
    initGraphics();
}

static void onAdapterReceived(WGPURequestAdapterStatus status, WGPUAdapter result, WGPUStringView message, void* ud1, void* ud2) {
    (void)ud1; (void)ud2;
    if (status != WGPURequestAdapterStatus_Success) {
        printf("RequestAdapter failed: %.*s\n", (int)message.length, message.data);
        return;
    }
    adapter = result;

    WGPUUncapturedErrorCallbackInfo errInfo = {0};
    errInfo.callback = onUncapturedError;

    WGPUDeviceDescriptor devDesc = {0};
    devDesc.uncapturedErrorCallbackInfo = errInfo;

    WGPURequestDeviceCallbackInfo cbInfo = {0};
    cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    cbInfo.callback = onDeviceReceived;

    wgpuAdapterRequestDevice(adapter, &devDesc, cbInfo);
}

int main(void) {
    instance = wgpuCreateInstance(NULL);

    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {0};
    canvasDesc.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
    canvasDesc.selector = (WGPUStringView){ "canvas", WGPU_STRLEN };

    WGPUSurfaceDescriptor surfDesc = {0};
    surfDesc.nextInChain = &canvasDesc.chain;
    surface = wgpuInstanceCreateSurface(instance, &surfDesc);

    WGPURequestAdapterCallbackInfo cbInfo = {0};
    cbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    cbInfo.callback = onAdapterReceived;

    wgpuInstanceRequestAdapter(instance, NULL, cbInfo);
    return 0;
}