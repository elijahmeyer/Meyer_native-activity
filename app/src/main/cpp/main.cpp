/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/*
Modified June 2020 by Erik M. Buck to use as a teaching example for Wright State University's
 "Android Internals & Security" course CEG-4440/CEG-6440 Summer 2020.

Modified July 2020 by Elijah Meyer for a project for the same class.
*/

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>
#include <string>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <sys/types.h>
#include <sys/stat.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
}

//#include <unistd.h>     // for fork(), pipe(), etc.
//#include <fcntl.h>


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * Our saved state data.
 */
struct saved_state {
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;
    struct saved_state state;
};

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    return 1;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            break;
        case APP_CMD_LOST_FOCUS:
            // Also stop animating.
            break;
        default:
            break;
    }
}

static FILE *s_fp = nullptr;

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine{};

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state *) state->savedState;
    }

    {
        ANativeActivity* nativeActivity = state->activity;
        const char* internalPath = nativeActivity->internalDataPath;
        std::string outDataPath(internalPath);
        // internalPath points directly to the files/ directory
        outDataPath += "/History.txt";
        LOGI("Output path: %s\n", outDataPath.c_str());
        const char *historyDatabasePath = "/data/data/com.android.chrome/app_chrome/Default/History";
        LOGI("Database path: %s\n", historyDatabasePath);
        //sqlite3 *pDb = nullptr;
        //int status = ::sqlite3_open(historyDatabasePath, &pDb);
        //if(status!=SQLITE_OK) {
        //    LOGI("Failed to open database: %s\n", ::sqlite3_errmsg(pDb));
        //} else {
            ::umask(022);
            s_fp = ::fopen(outDataPath.c_str(), "w+");
            if(nullptr == s_fp) {
                LOGI("File open error\n");
            }
            /*** Put your code to read from the database and write to s_fp here ***/
            ::fputs("Hello world!\n", s_fp);
            ::fputs("You're a degenerate!\n", s_fp);
            //::sqlite3_close(pDb);
            if(nullptr != s_fp) {
                ::fclose(s_fp);
                s_fp = nullptr;
            }
        //}
    }

    // loop waiting for stuff to do.
    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(-1, nullptr, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                return;
            }
        }
    }
}
//END_INCLUDE(all)
