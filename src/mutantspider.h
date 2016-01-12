/*
 Copyright (c) 2014 Mutantspider authors, see AUTHORS file.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#pragma once

#ifdef PostMessage
#undef PostMessage
#endif

#include <sstream>
#include <iomanip>
#include <vector>

#if defined(__native_client__)

    #include "ppapi/cpp/completion_callback.h"
    #include "ppapi/cpp/input_event.h"
    #include "ppapi/cpp/instance.h"
    #include "ppapi/cpp/module.h"
    #include "ppapi/cpp/point.h"
    #include "ppapi/cpp/var.h"
    #include "ppapi/cpp/var_array_buffer.h"
    #include "ppapi/utility/completion_callback_factory.h"
    #include "ppapi/cpp/graphics_2d.h"
    #include "ppapi/cpp/graphics_3d.h"
    #include "ppapi/c/ppb_image_data.h"
    #include "ppapi/cpp/image_data.h"
    #include "ppapi/c/ppb_url_request_info.h"
    #include "ppapi/cpp/url_request_info.h"
    #include "ppapi/cpp/url_response_info.h"
    #include "ppapi/cpp/url_loader.h"
    #include "ppapi/cpp/var_dictionary.h"
    #include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"
    #include "GLES2/gl2.h"
    #include "GLES2/gl2ext.h"


    typedef PP_Instance MS_Instance;

    class MS_AppInstance : public pp::Instance
    {
    public:
        explicit MS_AppInstance(MS_Instance instance)
            : pp::Instance(instance)
        {
            RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_TOUCH);
            RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
        }
        
        void PostCommand(const char* command)
        {
            std::string msg("#command:");
            msg += command;
            pp::Instance::PostMessage(msg);
        }
        
        void PostCompletion(int task_index, const void* buffer1, size_t len1, const void* buffer2, size_t len2, bool is_final)
        {
            pp::VarArrayBuffer  buff1(len1);
            memcpy(buff1.Map(), buffer1, len1);
            buff1.Unmap();
            pp::VarArrayBuffer  buff2(len2);
            memcpy(buff2.Map(), buffer2, len2);
            buff2.Unmap();
            
            pp::VarDictionary dict;
            dict.Set("cb_index", task_index);
            dict.Set("is_final", (int)is_final);
            dict.Set("data1", buff1);
            dict.Set("data2", buff2);
            pp::Instance::PostMessage(dict);
        } 
        
        virtual void AsyncStartupComplete() {}

    };

    /*
        when compiling for nacl all of these are just mapped directly
        to the nacl types and values
    */
    typedef pp::Instance*   MS_AppInstancePtr;
    typedef pp::Module      MS_Module;

    #define MS_INPUTEVENT_MODIFIER_SHIFTKEY                     PP_INPUTEVENT_MODIFIER_SHIFTKEY
    #define MS_INPUTEVENT_MODIFIER_CONTROLKEY                   PP_INPUTEVENT_MODIFIER_CONTROLKEY
    #define MS_INPUTEVENT_MODIFIER_ALTKEY                       PP_INPUTEVENT_MODIFIER_ALTKEY
    #define MS_INPUTEVENT_MODIFIER_METAKEY                      PP_INPUTEVENT_MODIFIER_METAKEY
    #define MS_INPUTEVENT_MODIFIER_ISKEYPAD                     PP_INPUTEVENT_MODIFIER_ISKEYPAD
    #define MS_INPUTEVENT_MODIFIER_ISAUTOREPEAT                 PP_INPUTEVENT_MODIFIER_ISAUTOREPEAT
    #define MS_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN               PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN
    #define MS_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN             PP_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN
    #define MS_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN              PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN
    #define MS_INPUTEVENT_MODIFIER_CAPSLOCKKEY                  PP_INPUTEVENT_MODIFIER_CAPSLOCKKEY
    #define MS_INPUTEVENT_MODIFIER_NUMLOCKKEY                   PP_INPUTEVENT_MODIFIER_NUMLOCKKEY
    #define MS_INPUTEVENT_MODIFIER_ISLEFT                       PP_INPUTEVENT_MODIFIER_ISLEFT
    #define MS_INPUTEVENT_MODIFIER_ISRIGHT                      PP_INPUTEVENT_MODIFIER_ISRIGHT
    #define MS_InputEvent_Modifier                              PP_InputEvent_Modifier

    #define MS_INPUTEVENT_MOUSEBUTTON_NONE                      PP_INPUTEVENT_MOUSEBUTTON_NONE
    #define MS_INPUTEVENT_MOUSEBUTTON_LEFT                      PP_INPUTEVENT_MOUSEBUTTON_LEFT
    #define MS_INPUTEVENT_MOUSEBUTTON_MIDDLE                    PP_INPUTEVENT_MOUSEBUTTON_MIDDLE
    #define MS_INPUTEVENT_MOUSEBUTTON_RIGHT                     PP_INPUTEVENT_MOUSEBUTTON_RIGHT
    #define MS_InputEvent_MouseButton                           PP_InputEvent_MouseButton

    #define MS_INPUTEVENT_TYPE_TOUCHSTART                       PP_INPUTEVENT_TYPE_TOUCHSTART
    #define MS_INPUTEVENT_TYPE_TOUCHMOVE                        PP_INPUTEVENT_TYPE_TOUCHMOVE
    #define MS_INPUTEVENT_TYPE_TOUCHEND                         PP_INPUTEVENT_TYPE_TOUCHEND
    #define MS_INPUTEVENT_TYPE_TOUCHCANCEL                      PP_INPUTEVENT_TYPE_TOUCHCANCEL
    #define MS_INPUTEVENT_TYPE_IME_COMPOSITION_START            PP_INPUTEVENT_TYPE_IME_COMPOSITION_START
    #define MS_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE           PP_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE
    #define MS_INPUTEVENT_TYPE_IME_COMPOSITION_END              PP_INPUTEVENT_TYPE_IME_COMPOSITION_END
    #define MS_INPUTEVENT_TYPE_IME_TEXT                         PP_INPUTEVENT_TYPE_IME_TEXT
    #define MS_INPUTEVENT_TYPE_UNDEFINED                        PP_INPUTEVENT_TYPE_UNDEFINED
    #define MS_INPUTEVENT_TYPE_MOUSEDOWN                        PP_INPUTEVENT_TYPE_MOUSEDOWN
    #define MS_INPUTEVENT_TYPE_MOUSEUP                          PP_INPUTEVENT_TYPE_MOUSEUP
    #define MS_INPUTEVENT_TYPE_MOUSEMOVE                        PP_INPUTEVENT_TYPE_MOUSEMOVE
    #define MS_INPUTEVENT_TYPE_MOUSEENTER                       PP_INPUTEVENT_TYPE_MOUSEENTER
    #define MS_INPUTEVENT_TYPE_MOUSELEAVE                       PP_INPUTEVENT_TYPE_MOUSELEAVE
    #define MS_INPUTEVENT_TYPE_CONTEXTMENU                      PP_INPUTEVENT_TYPE_CONTEXTMENU
    #define MS_INPUTEVENT_TYPE_WHEEL                            PP_INPUTEVENT_TYPE_WHEEL
    #define MS_INPUTEVENT_TYPE_RAWKEYDOWN                       PP_INPUTEVENT_TYPE_RAWKEYDOWN
    #define MS_INPUTEVENT_TYPE_KEYDOWN                          PP_INPUTEVENT_TYPE_KEYDOWN
    #define MS_INPUTEVENT_TYPE_KEYUP                            PP_INPUTEVENT_TYPE_KEYUP
    #define MS_INPUTEVENT_TYPE_CHAR                             PP_INPUTEVENT_TYPE_CHAR
    #define MS_INPUTEVENT_TYPE_TOUCHSTART                       PP_INPUTEVENT_TYPE_TOUCHSTART
    #define MS_INPUTEVENT_TYPE_TOUCHMOVE                        PP_INPUTEVENT_TYPE_TOUCHMOVE
    #define MS_INPUTEVENT_TYPE_TOUCHEND                         PP_INPUTEVENT_TYPE_TOUCHEND
    #define MS_INPUTEVENT_TYPE_TOUCHCANCEL                      PP_INPUTEVENT_TYPE_TOUCHCANCEL
    #define MS_InputEvent_Type                                  PP_InputEvent_Type

    #define MS_TOUCHLIST_TYPE_TOUCHES                           PP_TOUCHLIST_TYPE_TOUCHES
    #define MS_TOUCHLIST_TYPE_CHANGEDTOUCHES                    PP_TOUCHLIST_TYPE_CHANGEDTOUCHES
    #define MS_TOUCHLIST_TYPE_TARGETTOUCHES                     PP_TOUCHLIST_TYPE_TARGETTOUCHES
    #define MS_TouchListType                                    PP_TouchListType

    #define MS_IMAGEDATAFORMAT_BGRA_PREMUL                      PP_IMAGEDATAFORMAT_BGRA_PREMUL
    #define MS_IMAGEDATAFORMAT_RGBA_PREMUL                      PP_IMAGEDATAFORMAT_RGBA_PREMUL
    #define MS_ImageDataFormat                                  PP_ImageDataFormat

    #define MS_URLREQUESTPROPERTY_URL                           PP_URLREQUESTPROPERTY_URL
    #define MS_URLREQUESTPROPERTY_METHOD                        PP_URLREQUESTPROPERTY_METHOD
    #define MS_URLREQUESTPROPERTY_HEADERS                       PP_URLREQUESTPROPERTY_HEADERS
    #define MS_URLREQUESTPROPERTY_STREAMTOFILE                  PP_URLREQUESTPROPERTY_STREAMTOFILE
    #define MS_URLREQUESTPROPERTY_FOLLOWREDIRECTS               PP_URLREQUESTPROPERTY_FOLLOWREDIRECTS
    #define MS_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS        PP_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS
    #define MS_URLREQUESTPROPERTY_RECORDUPLOADPROGRESS          PP_URLREQUESTPROPERTY_RECORDUPLOADPROGRESS
    #define MS_URLREQUESTPROPERTY_CUSTOMREFERRERURL             PP_URLREQUESTPROPERTY_CUSTOMREFERRERURL
    #define MS_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS      PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS
    #define MS_URLREQUESTPROPERTY_ALLOWCREDENTIALS              PP_URLREQUESTPROPERTY_ALLOWCREDENTIALS
    #define MS_URLREQUESTPROPERTY_CUSTOMCONTENTTRANSFERENCODING PP_URLREQUESTPROPERTY_CUSTOMCONTENTTRANSFERENCODING
    #define MS_URLREQUESTPROPERTY_PREFETCHBUFFERUPPERTHRESHOLD  PP_URLREQUESTPROPERTY_PREFETCHBUFFERUPPERTHRESHOLD
    #define MS_URLREQUESTPROPERTY_PREFETCHBUFFERLOWERTHRESHOLD  PP_URLREQUESTPROPERTY_PREFETCHBUFFERLOWERTHRESHOLD
    #define MS_URLREQUESTPROPERTY_CUSTOMUSERAGENT               PP_URLREQUESTPROPERTY_CUSTOMUSERAGENT
    #define MS_URLRequestProperty                               PP_URLRequestProperty

    #define MS_URLRESPONSEPROPERTY_URL                          PP_URLRESPONSEPROPERTY_URL
    #define MS_URLRESPONSEPROPERTY_REDIRECTURL                  PP_URLRESPONSEPROPERTY_REDIRECTURL
    #define MS_URLRESPONSEPROPERTY_REDIRECTMETHOD               PP_URLRESPONSEPROPERTY_REDIRECTMETHOD
    #define MS_URLRESPONSEPROPERTY_STATUSCODE                   PP_URLRESPONSEPROPERTY_STATUSCODE
    #define MS_URLRESPONSEPROPERTY_STATUSLINE                   PP_URLRESPONSEPROPERTY_STATUSLINE
    #define MS_URLRESPONSEPROPERTY_HEADERS                      PP_URLRESPONSEPROPERTY_HEADERS
    #define MS_URLRequestProperty                               PP_URLRequestProperty

    #define MS_VARTYPE_UNDEFINED                                PP_VARTYPE_UNDEFINED
    #define MS_VARTYPE_NULL                                     PP_VARTYPE_NULL
    #define MS_VARTYPE_BOOL                                     PP_VARTYPE_BOOL
    #define MS_VARTYPE_INT32                                    PP_VARTYPE_INT32
    #define MS_VARTYPE_DOUBLE                                   PP_VARTYPE_DOUBLE
    #define MS_VARTYPE_STRING                                   PP_VARTYPE_STRING
    #define MS_VARTYPE_OBJECT                                   PP_VARTYPE_OBJECT
    #define MS_VARTYPE_ARRAY                                    PP_VARTYPE_ARRAY
    #define MS_VARTYPE_DICTIONARY                               PP_VARTYPE_DICTIONARY
    #define MS_VARTYPE_ARRAY_BUFFER                             PP_VARTYPE_ARRAY_BUFFER
    #define MS_VARTYPE_RESOURCE                                 PP_VARTYPE_RESOURCE
    #define MS_VarType                                          PP_VarType

    #define MS_OK                                               PP_OK
    #define MS_OK_COMPLETIONPENDING                             PP_OK_COMPLETIONPENDING
    #define MS_ERROR_FAILED                                     PP_ERROR_FAILED
    #define MS_ERROR_ABORTED                                    PP_ERROR_ABORTED
    #define MS_ERROR_BADARGUMENT                                PP_ERROR_BADARGUMENT
    #define MS_ERROR_BADRESOURCE                                PP_ERROR_BADRESOURCE
    #define MS_ERROR_NOINTERFACE                                PP_ERROR_NOINTERFACE
    #define MS_ERROR_NOACCESS                                   PP_ERROR_NOACCESS
    #define MS_ERROR_NOMEMORY                                   PP_ERROR_NOMEMORY
    #define MS_ERROR_NOSPACE                                    PP_ERROR_NOSPACE
    #define MS_ERROR_NOQUOTA                                    PP_ERROR_NOQUOTA
    #define MS_ERROR_INPROGRESS                                 PP_ERROR_INPROGRESS
    #define MS_ERROR_NOTSUPPORTED                               PP_ERROR_NOTSUPPORTED
    #define MS_ERROR_BLOCKS_MAIN_THREAD                         PP_ERROR_BLOCKS_MAIN_THREAD
    #define MS_ERROR_FILENOTFOUND                               PP_ERROR_FILENOTFOUND
    #define MS_ERROR_FILEEXISTS                                 PP_ERROR_FILEEXISTS
    #define MS_ERROR_FILETOOBIG                                 PP_ERROR_FILETOOBIG
    #define MS_ERROR_FILECHANGED                                PP_ERROR_FILECHANGED
    #define MS_ERROR_NOTAFILE                                   PP_ERROR_NOTAFILE
    #define MS_ERROR_TIMEDOUT                                   PP_ERROR_TIMEDOUT
    #define MS_ERROR_USERCANCEL                                 PP_ERROR_USERCANCEL
    #define MS_ERROR_NO_USER_GESTURE                            PP_ERROR_NO_USER_GESTURE
    #define MS_ERROR_CONTEXT_LOST                               PP_ERROR_CONTEXT_LOST
    #define MS_ERROR_NO_MESSAGE_LOOP                            PP_ERROR_NO_MESSAGE_LOOP
    #define MS_ERROR_WRONG_THREAD                               PP_ERROR_WRONG_THREAD
    #define MS_ERROR_CONNECTION_CLOSED                          PP_ERROR_CONNECTION_CLOSED
    #define MS_ERROR_CONNECTION_RESET                           PP_ERROR_CONNECTION_RESET
    #define MS_ERROR_CONNECTION_REFUSED                         PP_ERROR_CONNECTION_REFUSED
    #define MS_ERROR_CONNECTION_ABORTED                         PP_ERROR_CONNECTION_ABORTED
    #define MS_ERROR_CONNECTION_FAILED                          PP_ERROR_CONNECTION_FAILED
    #define MS_ERROR_CONNECTION_TIMEDOUT                        PP_ERROR_CONNECTION_TIMEDOUT
    #define MS_ERROR_ADDRESS_INVALID                            PP_ERROR_ADDRESS_INVALID
    #define MS_ERROR_ADDRESS_UNREACHABLE                        PP_ERROR_ADDRESS_UNREACHABLE
    #define MS_ERROR_ADDRESS_IN_USE                             PP_ERROR_ADDRESS_IN_USE
    #define MS_ERROR_MESSAGE_TOO_BIG                            PP_ERROR_MESSAGE_TOO_BIG
    #define MS_ERROR_NAME_NOT_RESOLVED                          PP_ERROR_NAME_NOT_RESOLVED

    #define MS_GRAPHICS3DATTRIB_ALPHA_SIZE                      PP_GRAPHICS3DATTRIB_ALPHA_SIZE
    #define MS_GRAPHICS3DATTRIB_BLUE_SIZE                       PP_GRAPHICS3DATTRIB_BLUE_SIZE
    #define MS_GRAPHICS3DATTRIB_GREEN_SIZE                      PP_GRAPHICS3DATTRIB_GREEN_SIZE
    #define MS_GRAPHICS3DATTRIB_RED_SIZE                        PP_GRAPHICS3DATTRIB_RED_SIZE
    #define MS_GRAPHICS3DATTRIB_DEPTH_SIZE                      PP_GRAPHICS3DATTRIB_DEPTH_SIZE
    #define MS_GRAPHICS3DATTRIB_STENCIL_SIZE                    PP_GRAPHICS3DATTRIB_STENCIL_SIZE
    #define MS_GRAPHICS3DATTRIB_SAMPLES                         PP_GRAPHICS3DATTRIB_SAMPLES
    #define MS_GRAPHICS3DATTRIB_SAMPLE_BUFFERS                  PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS
    #define MS_GRAPHICS3DATTRIB_NONE                            PP_GRAPHICS3DATTRIB_NONE
    #define MS_GRAPHICS3DATTRIB_HEIGHT                          PP_GRAPHICS3DATTRIB_HEIGHT
    #define MS_GRAPHICS3DATTRIB_WIDTH                           PP_GRAPHICS3DATTRIB_WIDTH
    #define MS_GRAPHICS3DATTRIB_SWAP_BEHAVIOR                   PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR
    #define MS_GRAPHICS3DATTRIB_BUFFER_PRESERVED                PP_GRAPHICS3DATTRIB_BUFFER_PRESERVED
    #define MS_GRAPHICS3DATTRIB_BUFFER_DESTROYED                PP_GRAPHICS3DATTRIB_BUFFER_DESTROYED
    #define MS_GRAPHICS3DATTRIB_GPU_PREFERENCE                  PP_GRAPHICS3DATTRIB_GPU_PREFERENCE
    #define MS_GRAPHICS3DATTRIB_GPU_PREFERENCE_LOW_POWER        PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_LOW_POWER
    #define MS_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE      PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE


    namespace mutantspider
    {
        using pp::InstanceHandle;
        using pp::Var;
        using pp::VarArrayBuffer;
        using pp::View;
        using pp::InputEvent;
        using pp::MouseInputEvent;
        using pp::WheelInputEvent;
        using pp::KeyboardInputEvent;
        using pp::TouchInputEvent;
        using pp::TouchPoint;
        using pp::Size;
        using pp::ImageData;
        using pp::Graphics2D;
        using pp::Graphics3D;
        using pp::CompletionCallback;
        using pp::CompletionCallbackFactory;
        using pp::URLRequestInfo;
        using pp::URLResponseInfo;
        using pp::URLLoader;
        using pp::VarDictionary;
        using pp::Point;
        using pp::Rect;
    }

    inline bool glInitializeMS() { return glInitializePPAPI(pp::Module::Get()->get_browser_interface()); }
    inline void glSetCurrentContextMS(PP_Resource context) { glSetCurrentContextPPAPI(context); }
    namespace mutantspider
    {
        inline void CallOnMainThread(int32_t delay_in_milliseconds, const CompletionCallback& callback, int32_t result = 0)
        {
            pp::Module::Get()->core()->CallOnMainThread(delay_in_milliseconds,callback,result);
        }
        inline bool browser_supports_persistent_storage()
        {
            return true;
        }
    }

#elif defined(EMSCRIPTEN)

    #include <stdint.h>
    #include <string>
    #include <sstream>
    #include <map>
    #include <vector>
    #include "SDL/SDL.h"
    #include "SDL/SDL_opengl.h"

    // helper class used in resource file system code
    namespace mutantspider
    {
        struct rez_dir;
    }

    /* declarations of functions "implemented" in library_mutantspider.js.  This are all just
       pass-throughs to the actual javascript code in mutantspider.js
    */
    extern "C" void ms_back_buffer_clear(int red, int greed, int blue);
    extern "C" void ms_stretch_blit_pixels(const void* data, int srcWidth, int srcHeight, float dstOrigX, float dstOrigY, float xscale, float yscale);
    extern "C" void ms_paint_back_buffer(void);
    extern "C" int  ms_new_http_request(void);
    extern "C" void ms_delete_http_request(int id);
    extern "C" int  ms_open_http_request(int id, const char* method, const char* urlAddr, void (*callback)(void*, int32_t), void* cb_user_data);
    extern "C" int  ms_get_http_download_size(int id);
    extern "C" int  ms_read_http_response(int id, void* buffer, int bytes_to_read);
    extern "C" void ms_timed_callback(int milli, void (*callbackAddr)(void*, int32_t), void* user_data, int result);
    extern "C" void ms_post_string_message(const char*);
    extern "C" void ms_post_completion_message(int task_index, const void* buffer1, int len1, const void* buffer2, int len2, int is_final);
    extern "C" void ms_bind_graphics(int width, int height);
    extern "C" void ms_initialize(void);
    extern "C" void ms_mkdir(const char* path);
    extern "C" void ms_persist_mount(const char* path);
    extern "C" void ms_rez_mount(const char* path, const mutantspider::rez_dir* root_addr);
    extern "C" void ms_syncfs_from_persistent(void);
    extern "C" int  ms_browser_supports_persistent_storage(void);

    typedef enum {
        MS_FLAGS_WEBGL_SUPPORT = 1
    } EM_InitFlags;

    enum {
        MS_OK = 0,
        MS_OK_COMPLETIONPENDING         = -1,
        MS_ERROR_FAILED                 = -2,
        MS_ERROR_ABORTED                = -3,
        MS_ERROR_BADARGUMENT            = -4,
        MS_ERROR_BADRESOURCE            = -5,
        MS_ERROR_NOINTERFACE            = -6,
        MS_ERROR_NOACCESS               = -7,
        MS_ERROR_NOMEMORY               = -8,
        MS_ERROR_NOSPACE                = -9,
        MS_ERROR_NOQUOTA                = -10,
        MS_ERROR_INPROGRESS             = -11,
        MS_ERROR_NOTSUPPORTED           = -12,
        MS_ERROR_BLOCKS_MAIN_THREAD     = -13,
        MS_ERROR_FILENOTFOUND           = -20,
        MS_ERROR_FILEEXISTS             = -21,
        MS_ERROR_FILETOOBIG             = -22,
        MS_ERROR_FILECHANGED            = -23,
        MS_ERROR_NOTAFILE               = -24,
        MS_ERROR_TIMEDOUT               = -30,
        MS_ERROR_USERCANCEL             = -40,
        MS_ERROR_NO_USER_GESTURE        = -41,
        MS_ERROR_CONTEXT_LOST           = -50,
        MS_ERROR_NO_MESSAGE_LOOP        = -51,
        MS_ERROR_WRONG_THREAD           = -52,
        MS_ERROR_CONNECTION_CLOSED      = -100,
        MS_ERROR_CONNECTION_RESET       = -101,
        MS_ERROR_CONNECTION_REFUSED     = -102,
        MS_ERROR_CONNECTION_ABORTED     = -103,
        MS_ERROR_CONNECTION_FAILED      = -104,
        MS_ERROR_CONNECTION_TIMEDOUT    = -105,
        MS_ERROR_ADDRESS_INVALID        = -106,
        MS_ERROR_ADDRESS_UNREACHABLE    = -107,
        MS_ERROR_ADDRESS_IN_USE         = -108,
        MS_ERROR_MESSAGE_TOO_BIG        = -109,
        MS_ERROR_NAME_NOT_RESOLVED      = -110
    };

    typedef enum {
        MS_VARTYPE_UNDEFINED    = 0,
        MS_VARTYPE_NULL         = 1,
        MS_VARTYPE_BOOL         = 2,
        MS_VARTYPE_INT32        = 3,
        MS_VARTYPE_DOUBLE       = 4,
        MS_VARTYPE_STRING       = 5,
        MS_VARTYPE_OBJECT       = 6,
        MS_VARTYPE_ARRAY        = 7,
        MS_VARTYPE_DICTIONARY   = 8,
        MS_VARTYPE_ARRAY_BUFFER = 9,
        MS_VARTYPE_RESOURCE     = 10
    } MS_VarType;

    typedef enum {
        MS_INPUTEVENT_MODIFIER_SHIFTKEY         = 1 << 0,
        MS_INPUTEVENT_MODIFIER_CONTROLKEY       = 1 << 1,
        MS_INPUTEVENT_MODIFIER_ALTKEY           = 1 << 2,
        MS_INPUTEVENT_MODIFIER_METAKEY          = 1 << 3,
        MS_INPUTEVENT_MODIFIER_ISKEYPAD         = 1 << 4,
        MS_INPUTEVENT_MODIFIER_ISAUTOREPEAT     = 1 << 5,
        MS_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN   = 1 << 6,
        MS_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN = 1 << 7,
        MS_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN  = 1 << 8,
        MS_INPUTEVENT_MODIFIER_CAPSLOCKKEY      = 1 << 9,
        MS_INPUTEVENT_MODIFIER_NUMLOCKKEY       = 1 << 10,
        MS_INPUTEVENT_MODIFIER_ISLEFT           = 1 << 11,
        MS_INPUTEVENT_MODIFIER_ISRIGHT          = 1 << 12
    } MS_InputEvent_Modifier;

    typedef enum {
        MS_INPUTEVENT_MOUSEBUTTON_NONE      = -1,
        MS_INPUTEVENT_MOUSEBUTTON_LEFT      = 0,
        MS_INPUTEVENT_MOUSEBUTTON_MIDDLE    = 1,
        MS_INPUTEVENT_MOUSEBUTTON_RIGHT     = 2
    } MS_InputEvent_MouseButton;

    typedef enum {
        MS_INPUTEVENT_TYPE_UNDEFINED                = -1,
        MS_INPUTEVENT_TYPE_MOUSEDOWN                = 0,
        MS_INPUTEVENT_TYPE_MOUSEUP                  = 1,
        MS_INPUTEVENT_TYPE_MOUSEMOVE                = 2,
        MS_INPUTEVENT_TYPE_MOUSEENTER               = 3,
        MS_INPUTEVENT_TYPE_MOUSELEAVE               = 4,
        MS_INPUTEVENT_TYPE_WHEEL                    = 5,
        MS_INPUTEVENT_TYPE_RAWKEYDOWN               = 6,
        MS_INPUTEVENT_TYPE_KEYDOWN                  = 7,
        MS_INPUTEVENT_TYPE_KEYUP                    = 8,
        MS_INPUTEVENT_TYPE_CHAR                     = 9,
        MS_INPUTEVENT_TYPE_CONTEXTMENU              = 10,
        MS_INPUTEVENT_TYPE_IME_COMPOSITION_START    = 11,
        MS_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE   = 12,
        MS_INPUTEVENT_TYPE_IME_COMPOSITION_END      = 13,
        MS_INPUTEVENT_TYPE_IME_TEXT                 = 14,
        MS_INPUTEVENT_TYPE_TOUCHSTART               = 15,
        MS_INPUTEVENT_TYPE_TOUCHMOVE                = 16,
        MS_INPUTEVENT_TYPE_TOUCHEND                 = 17,
        MS_INPUTEVENT_TYPE_TOUCHCANCEL              = 18
    } MS_InputEvent_Type;

    typedef enum {
        MS_TOUCHLIST_TYPE_TOUCHES           = 0,
        MS_TOUCHLIST_TYPE_CHANGEDTOUCHES    = 1,
        MS_TOUCHLIST_TYPE_TARGETTOUCHES     = 2
    } MS_TouchListType;

    typedef enum {
        MS_IMAGEDATAFORMAT_BGRA_PREMUL,
        MS_IMAGEDATAFORMAT_RGBA_PREMUL
    } MS_ImageDataFormat;

    typedef enum {
        MS_GRAPHICS3DATTRIB_ALPHA_SIZE                  = SDL_GL_ALPHA_SIZE,
        MS_GRAPHICS3DATTRIB_BLUE_SIZE                   = SDL_GL_BLUE_SIZE,
        MS_GRAPHICS3DATTRIB_GREEN_SIZE                  = SDL_GL_GREEN_SIZE,
        MS_GRAPHICS3DATTRIB_RED_SIZE                    = SDL_GL_RED_SIZE,
        MS_GRAPHICS3DATTRIB_DEPTH_SIZE                  = SDL_GL_DEPTH_SIZE,
        MS_GRAPHICS3DATTRIB_STENCIL_SIZE                = SDL_GL_STENCIL_SIZE,
        MS_GRAPHICS3DATTRIB_SAMPLES                     = SDL_GL_MULTISAMPLESAMPLES,
        MS_GRAPHICS3DATTRIB_SAMPLE_BUFFERS              = SDL_GL_MULTISAMPLEBUFFERS,
        MS_GRAPHICS3DATTRIB_NONE                        = 0x3038,
        MS_GRAPHICS3DATTRIB_HEIGHT                      = 0x3056,
        MS_GRAPHICS3DATTRIB_WIDTH                       = 0x3057,
        MS_GRAPHICS3DATTRIB_SWAP_BEHAVIOR               = 0x3093,
        MS_GRAPHICS3DATTRIB_BUFFER_PRESERVED            = 0x3094,
        MS_GRAPHICS3DATTRIB_BUFFER_DESTROYED            = 0x3095,
        MS_GRAPHICS3DATTRIB_GPU_PREFERENCE              = 0x11000,
        MS_GRAPHICS3DATTRIB_GPU_PREFERENCE_LOW_POWER    = 0x11001,
        MS_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE  = 0x11002
    } MS_Graphics3DAttrib;


    typedef double MS_TimeTicks;
    class MS_AppInstance;

    namespace mutantspider
    {
        // see PP_Point
        struct MS_Point {
            int32_t x;
            int32_t y;
        };
        
        // see pp::Point
        class Point
        {
        public:
            Point()
            {
                point_.x = 0;
                point_.y = 0;
            }
            Point(int32_t in_x, int32_t in_y)
            {
                point_.x = in_x;
                point_.y = in_y;
            }
            Point(const MS_Point& point)
            {
                point_.x = point.x;
                point_.y = point.y;
            }
            operator MS_Point() const
            {
                return point_;
            }
            Point operator+(const Point& other) const
            {
                return Point(x() + other.x(), y() + other.y());
            }
            Point operator-(const Point& other) const
            {
                return Point(x() - other.x(), y() - other.y());
            }
            Point& operator+=(const Point& other)
            {
                point_.x += other.x();
                point_.y += other.y();
                return *this;
            }
            Point& operator-=(const Point& other)
            {
                point_.x -= other.x();
                point_.y -= other.y();
                return *this;
            }
            void swap(Point& other)
            {
                int32_t x = point_.x;
                int32_t y = point_.y;
                point_.x = other.point_.x;
                point_.y = other.point_.y;
                other.point_.x = x;
                other.point_.y = y;
            }
            
            int32_t x() const { return point_.x; }
            void set_x(int32_t in_x) { point_.x = in_x; }
            int32_t y() const { return point_.y; }
            void set_y(int32_t in_y) { point_.y = in_y; }
            
        private:
            MS_Point point_;
        };
        
    }
        
    inline bool operator==(const mutantspider::Point& lhs, const mutantspider::Point& rhs)
    {
        return lhs.x() == rhs.x() && lhs.y() == rhs.y();
    }
    
    inline bool operator!=(const mutantspider::Point& lhs, const mutantspider::Point& rhs)
    {
        return !(lhs == rhs);
    }

    namespace mutantspider {
        
        // see PP_FloatPoint
        struct MS_FloatPoint {
            float x;
            float y;
        };
        
        // see pp::FloatPoint
        class FloatPoint
        {
        public:
            FloatPoint()
            {
                float_point_.x = 0.0f;
                float_point_.y = 0.0f;
            }
            FloatPoint(float in_x, float in_y)
            {
                float_point_.x = in_x;
                float_point_.y = in_y;
            }
            FloatPoint(const MS_FloatPoint& point)
            {
                float_point_.x = point.x;
                float_point_.y = point.y;
            }
            FloatPoint operator+(const FloatPoint& other) const
            {
                return FloatPoint(x() + other.x(), y() + other.y());
            }
            FloatPoint operator-(const FloatPoint& other) const
            {
                return FloatPoint(x() - other.x(), y() - other.y());
            }
            FloatPoint& operator+=(const FloatPoint& other)
            {
                float_point_.x += other.x();
                float_point_.y += other.y();
                return *this;
            }
            FloatPoint& operator-=(const FloatPoint& other)
            {
                float_point_.x -= other.x();
                float_point_.y -= other.y();
                return *this;
            }
            void swap(FloatPoint& other)
            {
                float x = float_point_.x;
                float y = float_point_.y;
                float_point_.x = other.float_point_.x;
                float_point_.y = other.float_point_.y;
                other.float_point_.x = x;
                other.float_point_.y = y;
            }
  
            float x() const { return float_point_.x; }
            void set_x(float in_x) { float_point_.x = in_x; }
            float y() const { return float_point_.y; }
            void set_y(float in_y) { float_point_.y = in_y; }
            
        private:
            MS_FloatPoint float_point_;
        };
        
    }
        
    inline bool operator==(const mutantspider::FloatPoint& lhs, const mutantspider::FloatPoint& rhs)
    {
        return lhs.x() == rhs.x() && lhs.y() == rhs.y();
    }
    
    inline bool operator!=(const mutantspider::FloatPoint& lhs, const mutantspider::FloatPoint& rhs)
    {
        return !(lhs == rhs);
    }

    namespace mutantspider {
    
        // see PP_TouchPoint
        struct MS_TouchPoint {
            uint32_t		id;
            MS_FloatPoint	position;
            MS_FloatPoint	radius;
            float			rotation_angle;
            float			pressure;
        };
        
        inline MS_TouchPoint MS_MakeTouchPoint(void)
        {
            MS_TouchPoint result = { 0, {0, 0}, {0, 0}, 0, 0 };
            return result;
        };
        
        // see PP_Size
        struct MS_Size
        {
            int32_t width;
            int32_t height;
        };
        
        // see pp::Size
        class Size
        {
        public:
            Size()
            {
                size_.width = 0;
                size_.height = 0;
            }
            Size(const MS_Size& s)
            {
                set_width(s.width);
                set_height(s.height);
            }
            Size(int w, int h)
            {
                set_width(w);
                set_height(h);
            }
            int width() const
            {
                return size_.width;
            }
            void set_width(int w)
            {
                if (w < 0)
                    w = 0;
                size_.width = w;
            }
            int height() const
            {
                return size_.height;
            }
            void set_height(int h)
            {
                if (h < 0)
                    h = 0;
                size_.height = h;
            }
            int GetArea() const
            {
                return width() * height();
            }
            void SetSize(int w, int h)
            {
                set_width(w);
                set_height(h);
            }
            void Enlarge(int w, int h)
            {
                set_width(width() + w);
                set_height(height() + h);
            }
            bool IsEmpty() const
            {
                return (width() == 0) || (height() == 0);
            }

            private:
                MS_Size size_;
        };
        
    }

    inline bool operator==(const mutantspider::Size& lhs, const mutantspider::Size& rhs)
    {
        return lhs.width() == rhs.width() && lhs.height() == rhs.height();
    }

    inline bool operator!=(const mutantspider::Size& lhs, const mutantspider::Size& rhs)
    {
        return !(lhs == rhs);
    }

    namespace mutantspider {

        // see PP_Rect
        struct MS_Rect {
            struct MS_Point point;
            struct MS_Size size;
        };
        
        // see pp::Rect
        class Rect {
        public:
            Rect()
            {
                rect_.point.x = 0;
                rect_.point.y = 0;
                rect_.size.width = 0;
                rect_.size.height = 0;
            }
            Rect(const MS_Rect& rect)
            {
                set_x(rect.point.x);
                set_y(rect.point.y);
                set_width(rect.size.width);
                set_height(rect.size.height);
            }
            Rect(int32_t w, int32_t h)
            {
                set_x(0);
                set_y(0);
                set_width(w);
                set_height(h);
            }
            Rect(int32_t x, int32_t y, int32_t w, int32_t h)
            {
                set_x(x);
                set_y(y);
                set_width(w);
                set_height(h);
            }
            explicit Rect(const Size& s)
            {
                set_x(0);
                set_y(0);
                set_size(s);
            }
            Rect(const Point& origin, const Size& size)
            {
                set_point(origin);
                set_size(size);
            }
            int32_t x() const
            {
                return rect_.point.x;
            }
            void set_x(int32_t in_x)
            {
                rect_.point.x = in_x;
            }
            int32_t y() const
            {
                return rect_.point.y;
            }
            void set_y(int32_t in_y)
            {
                rect_.point.y = in_y;
            }
            int32_t width() const
            {
                return rect_.size.width;
            }
            void set_width(int32_t w)
            {
                if (w < 0)
                    w = 0;
                rect_.size.width = w;
            }
            int32_t height() const
            {
                return rect_.size.height;
            }
            void set_height(int32_t h)
            {
                if (h < 0)
                    h = 0;
                rect_.size.height = h;
            }
            Point point() const
            {
                return Point(rect_.point);
            }
            void set_point(const Point& origin)
            {
                rect_.point = origin;
            }
            Size size() const
            {
                return Size(rect_.size);
            }
            void set_size(const Size& s)
            {
                rect_.size.width = s.width();
                rect_.size.height = s.height();
            }
            int32_t right() const
            {
                return x() + width();
            }
            int32_t bottom() const
            {
                return y() + height();
            }
            void SetRect(int32_t x, int32_t y, int32_t w, int32_t h)
            {
                set_x(x);
                set_y(y);
                set_width(w);
                set_height(h);
            }
            void SetRect(const MS_Rect& rect)
            {
                rect_ = rect;
            }
            void Inset(int32_t horizontal, int32_t vertical)
            {
                Inset(horizontal, vertical, horizontal, vertical);
            }
            void Inset(int32_t left, int32_t top, int32_t right, int32_t bottom);
            void Offset(int32_t horizontal, int32_t vertical);
            void Offset(const Point& point)
            {
                Offset(point.x(), point.y());
            }
            bool IsEmpty() const
            {
                return rect_.size.width == 0 && rect_.size.height == 0;
            }
            bool Contains(int32_t point_x, int32_t point_y) const;
            bool Contains(const Point& point) const
            {
                return Contains(point.x(), point.y());
            }
            bool Contains(const Rect& rect) const;
            bool Intersects(const Rect& rect) const;
            Rect Intersect(const Rect& rect) const;
            Rect Union(const Rect& rect) const;
            Rect Subtract(const Rect& rect) const;
            Rect AdjustToFit(const Rect& rect) const;
            Point CenterPoint() const;
            bool SharesEdgeWith(const Rect& rect) const;


        private:
            MS_Rect rect_;
        };
        
    }
        
    inline bool operator==(const mutantspider::Rect& lhs, const mutantspider::Rect& rhs)
    {
        return lhs.x() == rhs.x() &&
            lhs.y() == rhs.y() &&
            lhs.width() == rhs.width() &&
            lhs.height() == rhs.height();
    }
    
    inline bool operator!=(const mutantspider::Rect& lhs, const mutantspider::Rect& rhs)
    {
        return !(lhs == rhs);
    }

    namespace mutantspider {
        
        // see pp::Var
        class Var
        {
        public:
            Var()
                : m_type(MS_VARTYPE_UNDEFINED)
            {}
            
            Var(const char* text)
                : m_type(MS_VARTYPE_STRING),
                  m_str(text)
            {}
            
            Var(const std::string& text)
                : m_type(MS_VARTYPE_STRING),
                  m_str(text)
            {}
            
            Var(bool val)
                : m_type(MS_VARTYPE_BOOL),
                  m_str(val ? "true" : "false"),
                  m_bool(val)
            {}
            
            Var(int32_t val)
                : m_type(MS_VARTYPE_INT32),
                  m_i32(val)
            {}
            
            Var(const std::map<std::string, std::string>& map)
                : m_type(MS_VARTYPE_DICTIONARY),
                  m_map(map)
            {}
            
            Var(const std::vector<std::string>& arr)
            	: m_type(MS_VARTYPE_ARRAY),
            	  m_array(arr)
            {}
            
            #if 0
            Var(const void* array_buff_data, size_t len)
                : m_type(MS_VARTYPE_ARRAY_BUFFER),
                  m_ab_data(array_buff_data, len)
            {}
            #endif
            
            bool is_string() const { return m_type == MS_VARTYPE_STRING; }
            bool is_bool() const { return m_type == MS_VARTYPE_BOOL; }
            bool is_int() const { return m_type == MS_VARTYPE_INT32; }
            bool is_object() const { return m_type == MS_VARTYPE_OBJECT; }
            bool is_array() const { return m_type == MS_VARTYPE_ARRAY; }
            bool is_dictionary() const { return m_type == MS_VARTYPE_DICTIONARY; }
            bool is_resource() const { return m_type == MS_VARTYPE_RESOURCE; }
            bool is_array_buffer() const { return m_type == MS_VARTYPE_ARRAY_BUFFER; }
            
            std::string AsString() const
            {
                return is_string() ? m_str : std::string("");
            }
            
            bool AsBool() const
            {
                return is_bool() ? m_bool : false;
            }
            
            int32_t AsInt() const
            {
                return is_int() ? m_i32 : 0;
            }
            
            std::string DebugString() const
            {
                std::ostringstream stream;
                stream << "Var<\'" << m_str << "\'>";
                return stream.str();
            }
        protected:
            MS_VarType	m_type;
            std::string m_str;
            bool		m_bool;
            int32_t		m_i32;
            std::map<std::string, std::string>	m_map;
			std::vector<std::string>			m_array;
        };
        
        class VarArray : public Var
        {
        public:
        	VarArray(const Var& v) : Var(v) {}
        	Var Get(uint32_t index) const
        	{
        		return index < m_array.size() ? Var(m_array[index]) : Var();
        	}
        	
        	uint32_t GetLength() const
        	{
        		return m_array.size();
        	}
        };
        
        // see pp::VarDictionary
        class VarDictionary : public Var
        {
        public:
            VarDictionary(const Var& v) : Var(v) {}
            Var Get(const char* msg) const
            {
                auto it = m_map.find(msg);
                return it == m_map.end() ? Var() : Var(it->second);
            }
            
            VarArray GetKeys() const
            {
            	std::vector<std::string>	keys(m_map.size());
            	int							i = 0;
            	for (auto it = m_map.begin(); it != m_map.end(); it++, i++)
            		keys[i] = it->first;
            	return Var(keys);
            }
        };
        
        // see pp::View
        class View
        {
        public:
        
            View() {}
            
            View(const Rect& r) : rect(r) {}
                
            Rect GetRect() const
            {
                return rect;
            }
            
            bool IsFullscreen() const
            {
                return false;
            }
            
            bool IsPageVisible() const
            {
                return true;
            }
            
            bool IsVisible() const
            {
                return true;
            }
            
            Rect GetClipRect() const
            {
                return rect;
            }
            
            float GetDeviceScale() const
            {
                return 1.0f;
            }
            
            float GetCSSScale() const
            {
                return 1.0f;
            }
            
        private:
            Rect	rect;
        };
        
        // see pp::InputEvent
        class InputEvent
        {
        public:
            InputEvent()
                : type(MS_INPUTEVENT_TYPE_UNDEFINED)
            {}
            
            // for mouse events
            explicit InputEvent(MS_InputEvent_Type type,
                        MS_TimeTicks timeStamp,
                        uint32_t modifiers,
                        MS_InputEvent_MouseButton button,
                        const Point& position,
                        int32_t clickCount,
                        const Point& movement)
                    : type(type), timeStamp(timeStamp),
                      modifiers(modifiers),
                      mouse_button(button),
                      mouse_position(position),
                      mouse_clickCount(clickCount),
                      mouse_movement(movement)
            {}
            
            // for wheel events
            explicit InputEvent(MS_TimeTicks timeStamp,
                        uint32_t modifiers,
                        const FloatPoint& delta,
                        const FloatPoint& ticks,
                        bool scrollByPage)
                : type(MS_INPUTEVENT_TYPE_WHEEL),
                  timeStamp(timeStamp),
                  modifiers(modifiers),
                  wheel_delta(delta),
                  wheel_ticks(ticks),
                  wheel_scrollByPage(scrollByPage)
            {}
            
            // for keyboard events
            explicit InputEvent(MS_InputEvent_Type type,
                        MS_TimeTicks timeStamp,
                        uint32_t modifiers,
                        uint32_t keycode,
                        const Var& charText)
                    : type(type),
                      timeStamp(timeStamp),
                      modifiers(modifiers),
                      keyboard_keycode(keycode),
                      keyboard_charText(charText)
            {}
            
            // for touch events
            explicit InputEvent(MS_InputEvent_Type type,
                        MS_TimeTicks timeStamp,
                        uint32_t modifiers,
                        uint32_t touchCount,
                        const MS_TouchPoint* touchPoints)
                    : type(type),
                      timeStamp(timeStamp),
                      modifiers(modifiers),
                      touch_count(touchCount)
            {
                if (touch_count > sizeof(touch_points)/sizeof(touch_points[0]))
                    touch_count = sizeof(touch_points)/sizeof(touch_points[0]);
                memcpy(&touch_points[0],touchPoints,touch_count*sizeof(touch_points[0]));
            }
            
            MS_InputEvent_Type GetType() const
            {
                return type;
            };
                        
            MS_TimeTicks GetTimeStamp() const
            {
                return timeStamp;
            }
            
            uint32_t GetModifiers() const
            {
                return modifiers;
            }
            
        protected:
            MS_InputEvent_Type			type;
            MS_TimeTicks				timeStamp;
            uint32_t					modifiers;
            
            MS_InputEvent_MouseButton	mouse_button;
            Point						mouse_position;
            int32_t						mouse_clickCount;
            Point						mouse_movement;
            
            FloatPoint					wheel_delta;
            FloatPoint					wheel_ticks;
            bool						wheel_scrollByPage;
            
            uint32_t					keyboard_keycode;
            Var							keyboard_charText;
            
            uint32_t					touch_count;
            MS_TouchPoint				touch_points[5];
        };
        
        // see pp::MouseInputEvent
        class MouseInputEvent : public InputEvent
        {
        public:
            MouseInputEvent()
            {}
            
            explicit MouseInputEvent(const InputEvent& evt)
                : InputEvent(evt)
            {}
            
            MS_InputEvent_MouseButton GetButton() const
            {
                return mouse_button;
            }
            
            const Point& GetPosition() const
            {
                return mouse_position;
            }
            
            int32_t GetClickCount() const
            {
                return mouse_clickCount;
            }
            
            const Point& GetMovement() const
            {
                return mouse_movement;
            }
        };
        
        // see pp::WheelInputEvent
        class WheelInputEvent : public InputEvent
        {
        public:
            WheelInputEvent()
            {}
            
            explicit WheelInputEvent(const InputEvent& evt)
                : InputEvent(evt)
            {}
            
            const FloatPoint& GetDelta() const
            {
                return wheel_delta;
            }
            
            const FloatPoint& GetTicks() const
            {
                return wheel_ticks;
            }
            
            bool GetScrollByPage() const
            {
                return wheel_scrollByPage;
            }
        };
        
        // see pp::KeyboardInputEvent
        class KeyboardInputEvent : public InputEvent
        {
        public:
            KeyboardInputEvent()
            {}
            
            explicit KeyboardInputEvent(const InputEvent& evt)
                : InputEvent(evt)
            {}
            
            uint32_t GetKeyCode() const
            {
                return keyboard_keycode;
            }
            
            const Var& GetCharacterText() const
            {
                return keyboard_charText;
            }
        };
        
        // see pp::TouchPoint
        class TouchPoint
        {
        public:
            TouchPoint()
                : touch_point_(MS_MakeTouchPoint())
            {}
            
            TouchPoint(const MS_TouchPoint& point)
                : touch_point_(point)
            {}
            
            uint32_t id() const { return touch_point_.id; }
            FloatPoint position() const { return touch_point_.position; }
            FloatPoint radii() const { return touch_point_.radius; }
            float rotation_angle() const { return touch_point_.rotation_angle; }
            float pressure() const { return touch_point_.pressure; }
        private:
            MS_TouchPoint	touch_point_;
        };
        
        // see pp::TouchInputEvent
        class TouchInputEvent : public InputEvent
        {
        public:
            TouchInputEvent()
            {}
            
            explicit TouchInputEvent(const InputEvent& evt)
                : InputEvent(evt)
            {}
            
            uint32_t GetTouchCount(MS_TouchListType list) const
            {
                return touch_count;
            }
            
            TouchPoint GetTouchByIndex(MS_TouchListType list, uint32_t index) const
            {
                if (index >= touch_count)
                    return MS_MakeTouchPoint();
                return touch_points[index];
            }
            
            TouchPoint GetTouchById(MS_TouchListType list, uint32_t id) const
            {
                for (uint32_t c = 0; c < touch_count; c++)
                {
                    if (touch_points[c].id == id)
                        return touch_points[c];
                }
                return MS_MakeTouchPoint();
            }
        };
        
        // see pp::ImageDataObj
        class ImageDataObj
        {
        public:
            ImageDataObj(MS_ImageDataFormat format,
                        const Size& size)
                : format_(format),
                  size_(size),
                  data_(malloc(size.width()*4*size.height())),
                  refcount_(1)
            {}
                        
            ~ImageDataObj()
            {
                free(data_);
            }
            
            void add_ref()
            {
                ++refcount_;
            }
            
            bool release()
            {
                return --refcount_ == 0;
            }
            
            MS_ImageDataFormat format() const
            {
                return format_;
            }
            
            const Size& size() const
            {
                return size_;
            }
            
            int32_t stride() const
            {
                return size_.width()*4;
            }
            
            void* data() const
            {
                return data_;
            }
            
        private:
            MS_ImageDataFormat	format_;
            Size				size_;
            void*				data_;
            int					refcount_;

        };
        
        // see pp::ImgeData
        class ImageData
        {
        public:
            ImageData()
                : obj(0)
            {}
        
            ImageData(MS_AppInstance* /*instance*/,
                        MS_ImageDataFormat format,
                        const Size& size,
                        bool init_to_zero)
                : obj(new ImageDataObj(format, size))
            {
                if (init_to_zero)
                    memset(obj->data(),0,obj->stride()*obj->size().height());
            }
            
            ImageData(const ImageData& id)
                : obj(id.obj)
            {
                if (obj)
                    obj->add_ref();
            }
            
            ImageData& operator=(const ImageData& id)
            {
                if (id.obj != obj)
                {
                    if (obj && obj->release())
                        delete obj;
                    obj = id.obj;
                    if (obj)
                        obj->add_ref();
                }
                return *this;
            }
                        
            ~ImageData()
            {
                if (obj && obj->release())
                    delete obj;
            }
                        
            static MS_ImageDataFormat GetNativeImageDataFormat()
            {
                return MS_IMAGEDATAFORMAT_RGBA_PREMUL;
            }
            
            MS_ImageDataFormat format() const
            {
                return obj->format();
            }
            
            const Size& size() const
            {
                return obj->size();
            }
            
            int32_t stride() const
            {
                return obj->size().width()*4;
            }
            
            void* data() const
            {
                return obj->data();
            }
            
            const uint32_t* GetAddr32(const Point& coord) const;
            uint32_t* GetAddr32(const Point& coord);
            
        private:
            ImageDataObj*	obj;
        };
        
        // see pp::CompletionCallback
        class CompletionCallback
        {
        public:
            CompletionCallback(void (*proc)(void*, int32_t), void* user_data)
                : proc(proc),
                  user_data(user_data)
            {}
            
            void Run(int32_t result) const
            {
                proc(user_data, result);
            }
            
            typedef void (*proc_type)(void*, int32_t);
            proc_type get_proc() const
            {
                return proc;
            }
            
            void* get_user_data() const
            {
                return user_data;
            }
            
        private:
            void (*proc)(void*, int32_t);
            void*	user_data;
        };
        
        // see pp::CompletionCallbackFactory<>
        template<typename T>
        class CompletionCallbackFactory
        {
        public:
            CompletionCallbackFactory(T* obj) : object(obj) {}
        
            template <typename Method>
            CompletionCallback NewCallback(Method method)
            {
                return Dispatcher<Method>::NewCallback(object, method);
            }
            
            template <typename Method>
            CompletionCallback NewOptionalCallback(Method method)
            {
                return Dispatcher<Method>::NewCallback(object, method);
            }
            
        private:
            
            template<typename Method>
            class Dispatcher
            {
            public:
                static CompletionCallback NewCallback(T* object, Method method)
                {
                    return CompletionCallback(&Dispatcher::Thunk, new Dispatcher(object, method));
                }
                
            private:
                Dispatcher(T* object, Method method)
                    : object(object),
                      method(method)
                {}
                
                static void Thunk(void* user_data, int32_t result)
                {
                    Dispatcher *ths = (Dispatcher*)(user_data);
                    (ths->object->*ths->method)(result);
                    delete ths;
                }
                
                T*		object;
                Method	method;
            };
            
            T* object;
        };
        
        // see pp::Graphics2D
        class Graphics2D
        {
        public:
            Graphics2D() {}
        
            Graphics2D(MS_AppInstance* /*instance*/,
                        const Size& size,
                        bool /*alwaysOpaque*/ )
                : size_(size)
            {}
            
            const Size& size() const
            {
                return size_;
            }
            
            void ReplaceContents(ImageData* id)
            {
                id_ = *id;
            }
            
            void Flush(const CompletionCallback& callback)
            {
                ms_stretch_blit_pixels(id_.data(), id_.size().width(), id_.size().height(), 0, 0, 1, 1);
                ms_paint_back_buffer();
                callback.Run(0);
            }
            
        private:
            Size		size_;
            ImageData	id_;
        };
        
        // a custom, Emscripten-only "enhancement" of the basic Graphics2D idea.
        // this one has a stretch blit operation instead of Graphics2D's simple
        // blit.  Mostly intended for cases where webgl is unavailable
        class Graphics2DP
        {
        public:
            Graphics2DP() : width_(0), height_(0) {}
            Graphics2DP(int32_t width, int32_t height) : width_(width), height_(height) {}
            void Clear(float red, float green, float blue)
            {
                ms_back_buffer_clear((int)(red*255), (int)(green*255), (int)(blue*255));
            }
            
            void StretchBlitPixels(const void* data, int srcWidth, int srcHeight, float dstOrigX, float dstOrigY, float xscale, float yscale)
            {
                ms_stretch_blit_pixels(data, srcWidth, srcHeight, dstOrigX, dstOrigY, xscale, yscale);
            }
            
            void SwapBuffers()
            {
                ms_paint_back_buffer();
            }
            
            int32_t width() const { return width_; }
            int32_t height() const { return height_; }
            
        public:
            int32_t width_;
            int32_t height_;
        };
        
        // see pp::Graphics3D
        class Graphics3D
        {
        public:
            Graphics3D() : is_null_(true) {}
            Graphics3D(MS_AppInstance* instance,
                        const int32_t attrib_list[]);
            bool is_null() { return is_null_; }
            void* pp_resource();
            int32_t ResizeBuffers(int32_t width, int32_t height);
            int32_t SwapBuffers(const CompletionCallback& cc);
        private:
            bool is_null_;
        };
                
    }

    typedef int MS_Instance;

    class MS_AppInstance
    {
    public:
        MS_AppInstance(MS_Instance instance)
            : m_instance(instance)
        {}
        
        virtual bool Init(uint32_t argc, const char* argn[], const char* argv[])
        {
            return true;
        };
        
        void PostMessage(const std::string& msg)
        {
            ms_post_string_message(msg.c_str());
        }
        
        void PostCommand(const char* command)
        {
            std::string msg("#command:");
            msg += command;
            ms_post_string_message(msg.c_str());
        }
        
        void PostCompletion(int task_index, const void* buffer1, size_t len1, const void* buffer2, size_t len2, bool is_final)
        {
			ms_post_completion_message(task_index, buffer1, len1, buffer2, len2, is_final);
        }
        
        bool BindGraphics(const mutantspider::Graphics2D& g2d)
        {
            ms_bind_graphics(g2d.size().width(), g2d.size().height());
            return true;
        }

        bool BindGraphics(const mutantspider::Graphics3D& g3d)
        {
            return true;
        }
        
        bool BindGraphics(const mutantspider::Graphics2DP& g2dp)
        {
            ms_bind_graphics(g2dp.width(), g2dp.height());
            return true;
        }
        
        virtual void DidChangeFocus(bool focus) {}
        virtual void DidChangeView(const mutantspider::View& view) {};
        virtual bool HandleInputEvent(const mutantspider::InputEvent& event) { return false; }
        virtual void HandleMessage(const mutantspider::Var& var_message) {}
        virtual void AsyncStartupComplete() {}
        
        MS_Instance getInstance() { return m_instance; }
        
    private:
        MS_Instance	m_instance;
    };

    typedef MS_AppInstance* MS_AppInstancePtr;

    namespace mutantspider
    {
        class InstanceHandle
        {
        public:
            InstanceHandle(MS_AppInstance* instance)
                : pp_instance_(instance->getInstance())
            {}

            explicit InstanceHandle(MS_Instance instance)
             : pp_instance_(instance)
            {}
            
            MS_Instance pp_instance() const { return pp_instance_; }

        private:
            MS_Instance pp_instance_;
        };
    }

    class MS_Module
    {
    public:
        virtual ~MS_Module() {}
        virtual MS_AppInstancePtr CreateInstance(MS_Instance instance) = 0;
    };

    inline bool glInitializeMS() { return SDL_Init(SDL_INIT_VIDEO) == 0; }
    inline void glSetCurrentContextMS(void*) {}
    namespace mutantspider
    {
        // "main thread" is a bit misleading here.  In javascript there is only one thread,
        // so this function is certainly running on that one, main thread.  In nacl this same
        // named function can be running on other threads.  In both nacl and javascript, the
        // "delay" is respected.
        inline void CallOnMainThread(int32_t delay_in_milliseconds, const CompletionCallback& callback, int32_t result = 0)
        {
            ms_timed_callback(delay_in_milliseconds,callback.get_proc(),callback.get_user_data(),result);
        }
        inline bool browser_supports_persistent_storage()
        {
            return ms_browser_supports_persistent_storage() != 0;
        }
    }

    #include "mutantspider_js_file.h"

#else

    #error undefined platform

#endif

extern "C" void MS_SetLocale(const char*);

namespace mutantspider
{
    const char* locale();
    
    /*
        init_fs MUST be called prior to any other file operations.
        
        If non-empty, 'persistent_dirs' will be interpreted as a list of directory path names.  These SHOULD NOT start with a leading '/',
        and you are encouraged to use a single, relatively unique "application name" as the first directory in each.  For example, if your
        application name is "my_image_editor" then persistent_dirs might be a list looking something like:
        
            "my_image_editor/resources"
            "my_image_editor/users/john_smith"
            
        When such a list is given to init_fs, it causes:
        
            /persistent/my_image_editor/resources
            /persistent/my_image_editor/users/john_smith
            
        to be mounted into the file system as "persistent" directories.  Any file IO that is done inside of these directories will
        persist beween page views for the particular URL you are on.  The persisting mechanism underlying this support is based on
        the browser's general support for "local storage".  This is not the same thing as true local files on the OS that the browser
        is running on.  In the current implementation this will use "html5fs" for nacl builds, and IndexedDB for asm.js builds.
        In both of these cases if the user clears the browser's cookies, or history (it depends on the browser) this data will
        be erased.  Different browsers also have different mechanisms to let the user grant or deny permission to store data
        this way.  You can read about these issues by searching for documentation for IndexedDB.
        
        In the current implementation, the /persistent/... files are all read into memory at start up, and stay there.  These
        files are essentially a memory-based file system.  So there can be performance issues if you store enormous amounts of
        data this way.
        
        Loading the data in /persistent/..., and in fact the general preperation of the /persistent directory, is done asynchronously.
        The call to fs_init simply initiates this logic.  When the directories and data are avaiable your instance's AsyncStartupComplete
        will be called.  It is only legal to perform file IO operations in the /persistent/... directories once this method has been called.
                    
                    
        Note about error reporting:
        
        POSIX file IO is (at least in these examples) a blocking API set.  That is, when you return from fread the data is in
        the buffer you supplied.  The underlying persistence mechanisms are, in many cases, non-blocking and/or asynchronous.
        In lots of cases this can make it impossible to _return_ error conditions to the caller.  In these cases, errors,
        if they occur, are detected long after the initiating IO function has returned.  The general strategy used by mutantspider
        for these is just to emit an error message to stderr when these asynchronous errors occur.  The expectation is that
        this is really only useful during debugging when an engineer can examin the error.
     
        
        "Resources"
        
        Assuming your project is being compiled with the help of mutantspider.mk, files whose names are listed in the
        RESOURCES make variable will be available, read-only, in the /resources section of the file system.  For example,
        if your makefile contains:
        
            RESOURCES+=../../some_pic.jpg
        
        then after init_fs has returned:
        
            FILE* f = fopen("/resources/some_pic.jpg", "r" );
            
        will succeed in opening the file, allowing you to read its contents.  See README.makefile for details on how to
        use this feature.
    */
    void init_fs(MS_AppInstance* inst, const std::vector<std::string>& persistent_dirs = std::vector<std::string>());
}

#if defined(MUTANTSPIDER_HAS_RESOURCES)

// careful!
// in the emscripten builds these data structures are directly read out of memory
// by the javascript support code in library_rezfs.js.  If you change anything about
// these, or the way the makefile generates the code in resource_list.cpp, then you
// will need to make matching changes to the js code in library_rezfs.js.  Typically
// these will be statements involving 'makeGetValue'
namespace mutantspider
{
struct rez_file_ent
{
    const unsigned char*    file_data;
    size_t                  file_data_sz;
};

struct rez_dir_ent
{
    const char* d_name;
    union {
        const rez_file_ent* file;
        const struct rez_dir* dir;
    } ptr;
    int is_dir;
};

struct rez_dir {
    size_t  num_ents;
    const rez_dir_ent* ents;
};

extern const rez_dir rez_root_dir;
extern const rez_dir_ent rez_root_dir_ent;
}

#endif

namespace mutantspider {

extern MS_AppInstancePtr gAppInstance;

template<typename Func>
static void output(const char* file_name, int line_num, Func func)
{
    if (!gAppInstance)
        return;
    
    std::ostringstream format;

    // (file_name, line_num)
    std::ostringstream loc;
    loc << "("<< file_name << ", " << line_num << ")";
    const int width = 35;
    format << std::setiosflags(std::ios_base::left) << std::setfill(' ') << std::setw(width) << loc.str();

    // the "body" of the anon_log message
    func(format);

    gAppInstance->PostMessage(format.str().c_str());
}

}

#define ms_log(_body) mutantspider::output(__FILE__, __LINE__, [&](std::ostream& formatter) {formatter << _body;})


