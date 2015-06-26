#include <pebble.h>
	
// Constants
static const AccelSamplingRate SAMPLE_RATE = ACCEL_SAMPLING_10HZ;
static const uint8_t DATA_LOG_ID = 164;
enum states {
  RECORDING,
  PAUSED,
  PRERUN
};

// Global Variables
static Window *window;
static TextLayer  *text_layer;
DataLoggingSessionRef logging_session;
static enum states state = PRERUN;


// Push data from the accelerometer data service to the data logging service
static void cache_accel(AccelData * data, uint32_t num_samples) {
  // Array of 6 byte arrays
  unsigned char packed_data[num_samples][6];
  // Store the XYZ magnitudes in the data log
  for (uint32_t i = 0; i < num_samples; i++) {
    packed_data[i][0] = data[i].x >> 8;
    packed_data[i][1] = data[i].x;
    packed_data[i][2] = data[i].y >> 8;
    packed_data[i][3] = data[i].y;
    packed_data[i][4] = data[i].z >> 8;
    packed_data[i][5] = data[i].z;
  }
  data_logging_log(logging_session, &packed_data, num_samples);
}

// Start data recording
static void start(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state != PRERUN) {
    // Next state will be recording
    state = PRERUN;
    return;
  }
  state = RECORDING;
  // Register acceleration event handler with a 25 sample buffer
  accel_data_service_subscribe(25, cache_accel);
  // Display the pre-run message
  text_layer_set_text(text_layer, "Logging...\n\n(press any button to pause)");
}

// Analyse and dislay data
static void pause(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state != RECORDING)
    return;
  state = PAUSED;
  // De-register acceleration event handler
  accel_data_service_unsubscribe();
  // Display appropriate message
  text_layer_set_text(text_layer, "Paused.\n\nPress any button to resume.");
}

// Setup button handling
void click_config_provider3(Window *window) {
  // We start recording once they take their finger off the button,
  //  and stop recording once they push down on any button
  window_raw_click_subscribe(BUTTON_ID_DOWN, pause, start, NULL);
  window_raw_click_subscribe(BUTTON_ID_UP, pause, start, NULL);
  window_raw_click_subscribe(BUTTON_ID_SELECT, pause, start, NULL);
	//window_single_click_subscribe(BUTTON_ID_BACK, back_button);
	
}


void logging_init(void){
	window = window_create();
	text_layer = text_layer_create(layer_get_bounds(window_get_root_layer(window)));
  text_layer_set_text(text_layer, "Press any button on the right to begin logging.");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
	
  window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider3);
	
	// Set Accelerometer to sample rate
  accel_service_set_sampling_rate(SAMPLE_RATE);
	
	// Start the data logging service, we use only one for the application duration
  logging_session = data_logging_create(DATA_LOG_ID, DATA_LOGGING_BYTE_ARRAY, 6, false);
	
	window_stack_push(window, true);
}

void logging_deinit(void){
  // When we don't need to log anything else, we can close off the session.
  data_logging_finish(logging_session);
	
	layer_remove_from_parent(text_layer_get_layer(text_layer));
	text_layer_destroy(text_layer);
	window_destroy(window);
}