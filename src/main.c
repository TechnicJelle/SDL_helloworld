#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <volk.h>

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static VkInstance instance = NULL;
static uint32_t deviceCount = 0; //number of Vulkan devices found
static char** deviceNames = NULL; //array of device names

// Macro for validating Vulkan function call results
#define VK_CHECK(x, msg) \
	do { \
		VkResult err = x; \
		if (err) { \
			SDL_Log("Detected Vulkan error: %s", msg); \
			SDL_TriggerBreakpoint(); \
			return SDL_APP_FAILURE; \
		} \
	} while (0)


SDL_AppResult SDL_AppIterate(void* appstate) {
	// draw a black background.
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	// get the safe area for drawing the text into.
	SDL_Rect safeArea;
	SDL_GetRenderSafeArea(renderer, &safeArea);

	// draw the array of device names.
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, (float) safeArea.x + 10.0f, (float) safeArea.y + 10.0f, "Vulkan Devices:");
	if (deviceNames) {
		for (uint32_t i = 0; i < deviceCount; i++) {
			SDL_RenderDebugText(renderer, (float) safeArea.x + 30.0f, (float) safeArea.y + 30.0f + (float) i * 20.0f, deviceNames[i]);
		}
	} else {
		SDL_RenderDebugText(renderer, (float) safeArea.x + 10.0f, (float) safeArea.y + 30.0f, "No Vulkan devices found");
	}

	// put everything we drew to the screen.
	SDL_RenderPresent(renderer);

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
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	SDL_SetAppMetadata("SDL Hello World Example", "1.0", "com.example.sdl-hello-world");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
#ifdef SDL_PLATFORM_ANDROID
	// on android, we want to use the whole screen.
	flags |= SDL_WINDOW_FULLSCREEN;
#endif
	if (!SDL_CreateWindowAndRenderer("Hello SDL", 640, 480, flags, &window, &renderer)) {
		SDL_Log("SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// initialize Volk, the Vulkan loader.
	VK_CHECK(volkInitialize(), "Couldn't initialize Volk");

	// create a Vulkan instance.
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	const VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
	};

	VK_CHECK(vkCreateInstance(&createInfo, NULL, &instance), "Failed to create Vulkan instance");

	volkLoadInstance(instance);

	// get the number of physical Vulkan devices available.
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		SDL_Log("No Vulkan devices found");
		return SDL_APP_FAILURE;
	}

	// store the devices in an array.
	VkPhysicalDevice* devices = SDL_malloc(deviceCount * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

	deviceNames = SDL_malloc(deviceCount * sizeof(char*));

	//loop through the devices to log and save their names.
	for (uint32_t i = 0; i < deviceCount; i++) {
		// get the properties of the physical device.
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(devices[i], &properties);

		// temporary variables.
		char* deviceName = properties.deviceName;
		const size_t strLen = SDL_strlen(deviceName) + 1;

		// log the device name.
		SDL_Log("Device %d: %s", i, deviceName);

		// allocate memory for the device name and copy it.
		deviceNames[i] = SDL_malloc(strLen);
		SDL_strlcpy(deviceNames[i], deviceName, strLen);
	}

	// free the Vulkan devices array (our own names array is kept).
	SDL_free(devices);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	for (uint32_t i = 0; i < deviceCount; i++) {
		SDL_free(deviceNames[i]); // free each device name.
	}
	SDL_free(deviceNames); // free the array of device names.

	vkDestroyInstance(instance, NULL);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
