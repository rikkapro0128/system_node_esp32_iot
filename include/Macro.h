
/**
 * ********* MACRO GENERAL *********************************************************************************************************************************************************************************************************************************
 */

#define WDT_TIMEOUT 15 // WDT TIMER RESET AFTER SECOND

#define SSL_HANDSHAKE_REQUIRE 100
#define MAX_CONNECTION_FAILURE 5
#define WAIT_CONNECTION 10

#define PIN_RESET_DEFLAUT GPIO_NUM_13

#define STATUS_PIN_START GPIO_NUM_16
#define STATUS_PIN_WIFI GPIO_NUM_17

#define TIME_RESET_DEFAULT 5000
#define POOLING_CHECK_CONNECT_AP 5000
#define POOLING_CHECK_STREAM 5200
#define POOLING_RESTART_CONNECT_AP 700

#define STATUS_START_DELAY_FLICKER_SHORT 100
#define STATUS_START_DELAY_FLICKER_LONG 1000

// => disable macro firebase

// Comment to exclude Cloud Firestore 
#undef ENABLE_FIRESTORE

// Comment to exclude Firebase Cloud Messaging
#undef ENABLE_FCM

// Comment to exclude Firebase Storage
#undef ENABLE_FB_STORAGE

// Comment to exclude Cloud Storage
#undef ENABLE_GC_STORAGE

// Comment to exclude Cloud Function for Firebase
#undef ENABLE_FB_FUNCTIONS

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* MACRO DEFINE NODE TYPE *********************************************************************************************************************************************************************************************************************************
 */

// #define LOGIC
#define COLOR
// #define DIMMER

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* MODE DEV & DEPLOY *********************************************************************************************************************************************************************************************************************************
 */

// #define DEBUG
#define RELEASE

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */
