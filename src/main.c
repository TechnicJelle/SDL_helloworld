#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static SDL_Window* window = NULL;
static SDL_GPUDevice* device = NULL;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	SDL_SetAppMetadata("SDL Hello World Example", "1.0", "com.example.sdl-hello-world");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	window = SDL_CreateWindow("Hello SDL GPU", 640, 480, SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (device == NULL) {
		SDL_Log("SDL_CreateGPUDevice() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(device, window)) {
		SDL_Log("SDL_ClaimWindowForGPUDevice() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	switch (event->type) {
		case SDL_EVENT_QUIT: // triggers on last window close and other things. End the program.
			return SDL_APP_SUCCESS;

		case SDL_EVENT_KEY_DOWN: // quit if user hits ESC key
			if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
				return SDL_APP_SUCCESS;
			}
			break;

		default:
			break;

		default:
			break;
	}
	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* appstate) {
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	if (commandBuffer == NULL) {
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, NULL, NULL)) {
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL) {
		SDL_GPUColorTargetInfo colorTargetInfo = {NULL};
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.clear_color = (SDL_FColor){0.3f, 0.4f, 0.5f, 1.0f};
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
