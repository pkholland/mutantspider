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

// to pass 'use strict' we need this declared in a similar scope
// to where emscripten will declare it.
var Module;

(function (scope) {
    'use strict';
 
    if (scope.mutantspider != undefined)
        return;
 
    // internal function
    var using_web_gl = false;
    function set_using_web_gl(using)
    {
        using_web_gl = using;
    }
 
    // if the given message is a string and starts with '#' then it is some kind of
    // internal command-like message and we deal with that separately.  If not, it is
    // a general, probably debugging, message that we forward to the caller.
    var main_on_status;
    function post_js_message(msg)
    {
        function starts_with(full_str, sub_str)
        {
            return full_str.substring(0,sub_str.length) == sub_str;
        }
 
        if ((typeof msg == 'string') && (msg.charAt(0) == '#'))
        {
            var tok = '#command:';
            if (starts_with(msg,tok))
            {
                msg = msg.substring(tok.length, msg.lengh);
                var exe_type;
                if (nacl_moduleEl != null)
                    exe_type = 'PNaCl';
                else
                    exe_type = 'Javascript';
 
                if (starts_with(msg,'async_startup_complete:'))
                {
                    main_on_status({status: 'running', exe_type: exe_type, using_web_gl: using_web_gl});
                    return;
                }
 
                tok = 'async_startup_failed:';
                if (starts_with(msg, tok))
                {
                    main_on_status({status: 'error', message: msg.substring(tok.length, msg.length)});
                    return;
                }
            }
 
            console.log('unrecognized command message: ' + msg);
            return;
        }
        main_on_status({status: 'message', message: msg});
    }
 
    // support code for the case where the component is running asm.js
    // (instead of nacl/pnacl)
    var asm = (function() {
        
        var MS_FLAGS_WEBGL_SUPPORT = 1;
        
        // Various error codes passed to the asm.js module.
        // These must match the names and values listed in mutantspider.h
        var MS_OK = 0,
            MS_OK_COMPLETIONPENDING = -1,
            MS_ERROR_FAILED = -2,
            MS_ERROR_ABORTED = -3,
            MS_ERROR_BADARGUMENT = -4,
            MS_ERROR_BADRESOURCE = -5,
            MS_ERROR_NOINTERFACE = -6,
            MS_ERROR_NOACCESS = -7,
            MS_ERROR_NOMEMORY = -8,
            MS_ERROR_NOSPACE = -9,
            MS_ERROR_NOQUOTA = -10,
            MS_ERROR_INPROGRESS = -11,
            MS_ERROR_NOTSUPPORTED = -12,
            MS_ERROR_BLOCKS_MAIN_THREAD = -13,
            MS_ERROR_FILENOTFOUND = -20,
            MS_ERROR_FILEEXISTS = -21,
            MS_ERROR_FILETOOBIG = -22,
            MS_ERROR_FILECHANGED = -23,
            MS_ERROR_NOTAFILE = -24,
            MS_ERROR_TIMEDOUT = -30,
            MS_ERROR_USERCANCEL = -40,
            MS_ERROR_NO_USER_GESTURE = -41,
            MS_ERROR_CONTEXT_LOST = -50,
            MS_ERROR_NO_MESSAGE_LOOP = -51,
            MS_ERROR_WRONG_THREAD = -52,
            MS_ERROR_CONNECTION_CLOSED = -100,
            MS_ERROR_CONNECTION_RESET = -101,
            MS_ERROR_CONNECTION_REFUSED = -102,
            MS_ERROR_CONNECTION_ABORTED = -103,
            MS_ERROR_CONNECTION_FAILED = -104,
            MS_ERROR_CONNECTION_TIMEDOUT = -105,
            MS_ERROR_ADDRESS_INVALID = -106,
            MS_ERROR_ADDRESS_UNREACHABLE = -107,
            MS_ERROR_ADDRESS_IN_USE = -108,
            MS_ERROR_MESSAGE_TOO_BIG = -109,
            MS_ERROR_NAME_NOT_RESOLVED = -110;
		
        // Various event types and modifiers
        // These must match the names and values listed in mutantspider.h
        var MS_INPUTEVENT_TYPE_UNDEFINED = -1,
            MS_INPUTEVENT_TYPE_MOUSEDOWN = 0,
            MS_INPUTEVENT_TYPE_MOUSEUP = 1,
            MS_INPUTEVENT_TYPE_MOUSEMOVE = 2,
            MS_INPUTEVENT_TYPE_MOUSEENTER = 3,
            MS_INPUTEVENT_TYPE_MOUSELEAVE = 4,
            MS_INPUTEVENT_TYPE_WHEEL = 5,
            MS_INPUTEVENT_TYPE_RAWKEYDOWN = 6,
            MS_INPUTEVENT_TYPE_KEYDOWN = 7,
            MS_INPUTEVENT_TYPE_KEYUP = 8,
            MS_INPUTEVENT_TYPE_CHAR = 9,
            MS_INPUTEVENT_TYPE_CONTEXTMENU = 10,
            MS_INPUTEVENT_TYPE_IME_COMPOSITION_START = 11,
            MS_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE = 12,
            MS_INPUTEVENT_TYPE_IME_COMPOSITION_END = 13,
            MS_INPUTEVENT_TYPE_IME_TEXT = 14,
            MS_INPUTEVENT_TYPE_TOUCHSTART = 15,
            MS_INPUTEVENT_TYPE_TOUCHMOVE = 16,
            MS_INPUTEVENT_TYPE_TOUCHEND = 17,
            MS_INPUTEVENT_TYPE_TOUCHCANCEL = 18,
            MS_INPUTEVENT_MODIFIER_SHIFTKEY = 1 << 0,
            MS_INPUTEVENT_MODIFIER_CONTROLKEY = 1 << 1,
            MS_INPUTEVENT_MODIFIER_ALTKEY = 1 << 2,
            MS_INPUTEVENT_MODIFIER_METAKEY = 1 << 3,
            MS_INPUTEVENT_MODIFIER_ISKEYPAD = 1 << 4,
            MS_INPUTEVENT_MODIFIER_ISAUTOREPEAT = 1 << 5,
            MS_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN = 1 << 6,
            MS_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN = 1 << 7,
            MS_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN = 1 << 8,
            MS_INPUTEVENT_MODIFIER_CAPSLOCKKEY = 1 << 9,
            MS_INPUTEVENT_MODIFIER_NUMLOCKKEY = 1 << 10,
            MS_INPUTEVENT_MODIFIER_ISLEFT = 1 << 11,
            MS_INPUTEVENT_MODIFIER_ISRIGHT = 1 << 12,
            MS_INPUTEVENT_MOUSEBUTTON_NONE = -1,
            MS_INPUTEVENT_MOUSEBUTTON_LEFT = 0,
            MS_INPUTEVENT_MOUSEBUTTON_MIDDLE = 1,
            MS_INPUTEVENT_MOUSEBUTTON_RIGHT = 2;
            
        // state variables we use to communicate with and
        // support the asm.js module
        var mouse_proc,
            focus_proc,
            key_proc,
            touch_proc,
            do_callback,
            change_view_proc,
            ele_offsetX,
            ele_offsetY,
            canvas_elm,
            canvas_ctx,
            canvas_back_elm,
            canvas_back_ctx,
            canvas_ctx_width,
            canvas_ctx_height,
            tmp_scale_elm,
            computed_is_clamped = false,
            is_Uint8ClampedArray,
            httpRequests = new Object(),
            httpRequestIndex = 0,
            initialize;
            
        // return the mutantspider event modifies of the given event
        function getModifiers(evt)
        {
            var modifiers = 0;
            if (evt.modifiers)
            {
                if (evt.modifiers & Event.ALT_MASK)
                    modifiers |= MS_INPUTEVENT_MODIFIER_ALTKEY;
                if (evt.modifiers & Event.CONTROL_MASK)
                    modifiers |= MS_INPUTEVENT_MODIFIER_CONTROLKEY;
                if (evt.modifiers & Event.SHIFT_MASK)
                    modifiers |= MS_INPUTEVENT_MODIFIER_SHIFTKEY;
                if (evt.modifiers & Event.META_MASK)
                    modifiers |= MS_INPUTEVENT_MODIFIER_METAKEY;
            }
            else
            {
                if (evt.altKey)
                    modifiers |= MS_INPUTEVENT_MODIFIER_ALTKEY;
                if (evt.ctrlKey)
                    modifiers |= MS_INPUTEVENT_MODIFIER_CONTROLKEY;
                if (evt.shiftKey)
                    modifiers |= MS_INPUTEVENT_MODIFIER_SHIFTKEY;
                if (evt.metaKey)
                    modifiers |= MS_INPUTEVENT_MODIFIER_METAKEY;
            }
            return modifiers;
        }
        
        function getMovementX(evt)
        {
            if (evt.mozMovementX)
                return evt.mozMovementX;
            return 0;
        }
        
        function getMovementY(evt)
        {
            if (evt.mozMovementY)
                return evt.mozMovementY;
            return 0;
        }
        
        function getTimeStame(evt)
        {
            return evt.timeStamp;
        }
        
        // Various mouse functions.  If the asm.js module requested
        // mouse tracking these methods will be installed as listeners
        // on the component's element
        function jsMouseDown(evt)
        {
            if (mouse_proc(MS_INPUTEVENT_TYPE_MOUSEDOWN, getTimeStame(evt), getModifiers(evt), evt.button,
                        evt.pageX - ele_offsetX, evt.pageY - ele_offsetY, 1, getMovementX(evt), getMovementY(evt)) != 0)
                evt.stopPropagation();
        }
        
        function jsMouseMove(evt)
        {
            if (mouse_proc(MS_INPUTEVENT_TYPE_MOUSEMOVE, getTimeStame(evt), getModifiers(evt), evt.button,
                        evt.pageX - ele_offsetX, evt.pageY - ele_offsetY, 1, getMovementX(evt), getMovementY(evt)) != 0)
                evt.stopPropagation();
        }
        
        function jsMouseOver(evt)
        {
            if (mouse_proc(MS_INPUTEVENT_TYPE_MOUSEENTER, getTimeStame(evt), getModifiers(evt), evt.button,
                        evt.pageX - ele_offsetX, evt.pageY - ele_offsetY, 1, getMovementX(evt), getMovementY(evt)) != 0)
                evt.stopPropagation();
        }
        
        function jsMouseOut(evt)
        {
            if (mouse_proc(MS_INPUTEVENT_TYPE_MOUSELEAVE, getTimeStame(evt), getModifiers(evt), evt.button,
                        evt.pageX - ele_offsetX, evt.pageY - ele_offsetY, 1, getMovementX(evt), getMovementY(evt)) != 0)
                evt.stopPropagation();
        }
        
        function jsMouseUp(evt)
        {
            if (mouse_proc(MS_INPUTEVENT_TYPE_MOUSEUP, getTimeStame(evt), getModifiers(evt), evt.button,
                    evt.pageX - ele_offsetX, evt.pageY - ele_offsetY, 1, getMovementX(evt), getMovementY(evt)) != 0)
                evt.stopPropagation();
        }
        
        // Focus changing methods.  These call the asm.js module
        // to tell it when it gains and loses keyboard focus
        function jsGainFocus(evt)
        {
            focus_proc(1);
        }
        
        function jsLoseFocus(evt)
        {
            focus_proc(0);
        }
        
        // Various keyboard functions.  If the asm.js module requested
        // keyboard tracking these methods will be installed as listeners
        // on the component's element
        function jsKeyDown(evt)
        {
            key_proc(MS_INPUTEVENT_TYPE_KEYDOWN, getTimeStame(evt), getModifiers(evt),
                        evt.keyCode, evt.charCode);
        }
        
        function jsKeyPress(evt)
        {
            key_proc(MS_INPUTEVENT_TYPE_CHAR, getTimeStame(evt), getModifiers(evt),
                        evt.keyCode, evt.charCode);
        }
        
        function jsKeyUp(evt)
        {
            key_proc(MS_INPUTEVENT_TYPE_KEYUP, getTimeStame(evt), getModifiers(evt),
                        evt.keyCode, evt.charCode);
        }
        
        // Touch handling uses a variable-sized data structure
        // to pass the data to the asm.js module.  This function
        // computes the length of that data.
        function computeLength(touches)
        {
            // one for the "length" field,
            // then for each one:
            //      1) an id
            //      2) the x coord
            //      3) the y coord
            return 4 * (1 + touches.length*3);
        }
        
        // write the given touch data into the heap at 'addr4'.
        function writeTouches(touches, addr4)
        {
            Module.HEAP32[addr4++] = touches.length;
            for (var i = 0; i < touches.length; i++)
            {
                Module.HEAP32[addr4++] = touches[i].identifier;
                Module.HEAP32[addr4++] = touches[i].pageX - ele_offsetX;
                Module.HEAP32[addr4++] = touches[i].pageY - ele_offsetY;
            }
        }
        
        // send touch event information to the asm.js module
        function doTouch(evt, type)
        {
            var buflength = computeLength(evt.touches) + computeLength(evt.targetTouches) + computeLength(evt.changedTouches);
            var addr = Module._malloc(buflength);
            var addr4 = addr >> 2;	// we're going to access it with HEAP32
            writeTouches(evt.touches,addr4);
            writeTouches(evt.targetTouches,addr4);
            writeTouches(evt.changedTouches,addr4);

            if (touch_proc(type, 0/*getTimeStamp(evt)*/, getModifiers(evt), addr) != 0)
                evt.preventDefault();

            Module._free(addr);
        }
        
        function jsTouchStart(evt)
        {
            doTouch(evt, MS_INPUTEVENT_TYPE_TOUCHSTART);
        }
        
        function jsTouchMove(evt)
        {
            doTouch(evt, MS_INPUTEVENT_TYPE_TOUCHMOVE);
        }
        
        function jsTouchEnd(evt)
        {
            doTouch(evt, MS_INPUTEVENT_TYPE_TOUCHEND);
        }
        
        function jsTouchCancel(evt)
        {
            doTouch(evt, MS_INPUTEVENT_TYPE_TOUCHCANCEL);
        }

        // js_initialize gets called through a circuitous route.
        // when the asm.js file is loaded some of the emscripten support
        // code will request the download the of the <component>.js.mem file.
        // This is a binary file that contains the original state of the HEAP
        // and must be fully downloaded and used to initialize the module's
        // HEAP member.  This completes asynchronously after the asm.js file
        // begins executing.  Once the HEAP has been initialized the emscripten
        // code calls into the component's 'main' (C/C++) function, and that function
        // calls back here.  But in order to deal with the parameters that
        // we would otherwise pass to this function, we store them in
        // closure-local variables and then call this through the 'initialize'
        // function below (which takes no parameters, so is easy to call from C).
        function js_initialize(element)
        {
            var width = element.clientWidth;
            var height = element.clientHeight;
            
            // TODO, this should probably be addEventListener calls since
            // 'element' is a caller-supplied DOM element and we shouldn't
            // be completely replacing handlers that they may have installed.
            element.onmousedown = jsMouseDown;
            element.oncontextmenu = function(e){return false;};
            element.onfocus = jsGainFocus;
            element.setAttribute('tabindex', '-1');
            element.onblur = jsLoseFocus;
            element.onmousemove = jsMouseMove;
            element.onmouseover = jsMouseOver;
            element.onmouseout = jsMouseOut;
            element.onmouseup = jsMouseUp;
            element.onkeydown = jsKeyDown;
            element.onkeypress = jsKeyPress;
            element.onkeyup = jsKeyUp;

            element.ontouchstart = jsTouchStart;
            element.ontouchend = jsTouchEnd;
            element.ontouchmove = jsTouchMove;
            element.ontouchcancel = jsTouchCancel;

            var e = element;
            ele_offsetX = 0;
            ele_offsetY = 0;
            while (e != null)
            {
                ele_offsetX += e.offsetLeft;
                ele_offsetY += e.offsetTop;
                e = e.offsetParent;
            }

            // get the addresses of these functions in the asm.js module,
            // along with a description of their parameters and return type.
            // 'cwrap' then creates a new callbable object that marshals the
            // parameters and return value for us.
            var init_proc = Module.cwrap('MS_Init','number',['number']);
            var set_locale_proc = Module.cwrap('MS_SetLocale', 'null', ['string']);
            mouse_proc = Module.cwrap('MS_MouseProc','number',['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']);
            focus_proc = Module.cwrap('MS_FocusProc', 'null', ['number']);
            key_proc = Module.cwrap('MS_KeyProc', 'null', ['number', 'number', 'number', 'number', 'number']);
            change_view_proc = Module.cwrap('MS_DidChangeView', 'null', ['number', 'number', 'number', 'number']);
            touch_proc = Module.cwrap('MS_TouchProc', 'number', ['number', 'number', 'number', 'number']);
            do_callback = Module.cwrap('MS_DoCallbackProc', 'null', ['number', 'number', 'number']);

            // in both the open_gl_es and non-open_gl_es case
            // we use an html5 canvas object to implement the rendering
            // support for the asm.js module.  So create that here.
            canvas_elm = document.createElement('canvas');
            canvas_elm.setAttribute('width', width);
            canvas_elm.setAttribute('height', height);
            canvas_elm.setAttribute('style', 'background-color:black');
            element.appendChild(canvas_elm);

            // Mutantspider currently uses the SDL library out of the emscripten tech,
            // and its binding to webgl works by requiring that we set Module.canvas
            // to the canvas we are using.  So set that here (even if we don't end up
            // using webgl)
            Module['canvas'] = canvas_elm;

            // prevents the SDL library from capturing all keyboard events (which messes
            // up all kinds of things).
            Module['doNotCaptureKeyboard'] = true;

            var init_flags = 0;

            // does this browser understand webgl at all?
            if (window.WebGLRenderingContext)
            {
                // is it currently enabled?
                var tmp_elm = document.createElement('canvas');
                if (tmp_elm.getContext("webgl"))
                    init_flags |= MS_FLAGS_WEBGL_SUPPORT;
                else if (tmp_elm.getContext("experimental-webgl"))
                {
                    // here we run some heuristics to see if the browser
                    // seems to have reasonable support for webgl or not
                    // (as of 7/14/2014 Safari and Opera have usable support,
                    // IE does not).
                    if (confirm("Browser only has experimental support of webgl.  Do you want to try to use it?") == true)
                    {
                        init_flags |= MS_FLAGS_WEBGL_SUPPORT;
                    }
                }
            }
            mutantspider.set_using_web_gl((init_flags & MS_FLAGS_WEBGL_SUPPORT) != 0);

            // if we can't use webgl, then we do a little of our own double-buffered
            // rendering and so need to create a second canvas for that.
            canvas_back_elm = document.createElement('canvas');
            canvas_back_elm.setAttribute('width', width);
            canvas_back_elm.setAttribute('height', height);
            tmp_scale_elm = document.createElement('canvas');

            // some "pre-initialization" stuff...
            // note that in the (p)nacl case we do this with a 'locale' argv sort of thing
            set_locale_proc( navigator.language );

            // call into the asm.js module's init function
            init_proc( init_flags );

            // tell the asm.ms module about the current size of it
            change_view_proc(0,0,width,height);

            // We're being called (indirectly) by the asm.js module's 'main' function.
            // This assignment prevents the emscripten runtime from tearing down the C runtime
            // when main exits.
            Module['noExitRuntime'] = true;
        }
        
        // called to deal with variables we need in js_initialize, but can't
        // be easily passed through the C/C++ asm.js layer.
        function set_initialize(element)
        {
            this.initialize = function() { js_initialize(element); }
        }
        
        // The support code called by the asm.js...
        
        // The "graphics" methods are used when we find that we are _not_ able to use
        // webgl.  But the model and usage pattern is still vaguely influenced by
        // opengl's usage pattern.
        
        // called to let us know what size it will be rendering
        function bind_graphics(width, height)
        {
            canvas_ctx = canvas_elm.getContext('2d');
            canvas_back_ctx = canvas_back_elm.getContext('2d');
            canvas_ctx_width = width;
            canvas_ctx_height = height;
        }
        
        // paint the back buffer the given rgb color
        function back_buffer_clear(red, green, blue)
        {
            canvas_back_ctx.fillStyle = "#" + red.toString(16) + green.toString(16) + blue.toString(16);
            canvas_back_ctx.fillRect(0,0,canvas_ctx_width, canvas_ctx_height);
        }
        
        // paint the given bitmap (addr, srcWidth, srcHeight) at position <dstOrigX, dstOrigY>
        // into the back buffer, while scaling it by <xscale,yscale>.
        function stretch_blit_pixels(addr, srcWidth, srcHeight, dstOrigX, dstOrigY, xscale, yscale)
        {
            // first transfer the pixels (blit-style) to an ImageData object
            // associated with the canvas element 'tmp_scale_elm'
            tmp_scale_elm.setAttribute('width', srcWidth);
            tmp_scale_elm.setAttribute('height', srcHeight);
            var ctx = tmp_scale_elm.getContext('2d');

            var id = ctx.createImageData(srcWidth,srcHeight);

            // one time check for typeof ImageData.data == Uint8ClampedArray
            if (!computed_is_clamped)
            {
                try
                {
                    if (id.data instanceof Uint8ClampedArray)
                        is_Uint8ClampedArray = true;
                    else
                        is_Uint8ClampedArray = false;
                }
                catch(err)
                {
                    is_Uint8ClampedArray = false;
                }
                computed_is_clamped = true;
            }

            // newer browsers allow block transfer of pixel data with the
            // 'Uint8ClampedArray' type.  If that is what this browser does
            // then use it.  Otherwise copy the data in a loop.
            var length = srcWidth * srcHeight * 4;
            if (is_Uint8ClampedArray)
            {
                var subBuff = new Uint8ClampedArray( Module.HEAP8.buffer, addr, length );
                id.data.set(subBuff);
            }
            else
            {
                for (var i=0; i<length; i++)
                    id.data[i] = Module.HEAPU8[addr+i];
            }
            ctx.putImageData(id,0,0,0,0,srcWidth,srcHeight);

            // now we can use tmp_scale_elm as an image object in a (scaled) call to drawImage
            // to paint the scaled bitmap into the back canvas
            canvas_back_ctx.save();
            canvas_back_ctx.scale(xscale,yscale);
            canvas_back_ctx.drawImage(tmp_scale_elm,dstOrigX,dstOrigY);
            canvas_back_ctx.restore();
        }

        // blit the back buffer to the front buffer
        function paint_back_buffer()
        {
            canvas_ctx.drawImage(canvas_back_elm, 0, 0);
        }


        // HTTP support code.

        // creates a new http request object and returns an id for it
        // that can be used later in other http support calls to identify
        // which object you are working on.
        function new_http_request()
        {
            var id = httpRequestIndex;
            ++httpRequestIndex;
            httpRequests[id] = new XMLHttpRequest();
            return id;
        }

        // free the resources of the given http request object
        function delete_http_request(id)
        {
            delete httpRequests[id];
        }

        function status_code_to_error(status)
        {
            // TODO, provide all the translations
            return MS_ERROR_FAILED;
        }

        /*
            id              value previously returned from new_http_request
            method          address (offset) of 'GET', 'POST', etc... c-string in Module.HEAP
            urlAddr         address (offset) of the url c-string to be used in Module.HEAP
            callbackAddr    function address (index-like thing) of the callback function that will be passed as the first parameter to Module.MS_HttpCallbackProc
            user_data       address (offset) of user_data that will be passed as the second parameter to Module.MS_HttpCallbackProc
            
            TODO: Get error handling (for example, 404 returns, etc...) lined up with nacl's behavior
        */
        function open_http_request(id, method, urlAddr, callbackAddr, user_data)
        {
            var xhr = httpRequests[id];
            if (typeof xhr === 'undefined')
                do_callback(callbackAddr, user_data, MS_ERROR_BADARGUMENT);
            else if (typeof xhr.c_callback !== 'undefined')
                do_callback(callbackAddr, user_data, MS_ERROR_INPROGRESS);
            try
            {
                // store these two for when 'send' completes
                xhr.c_callback = callbackAddr;
                xhr.c_callback_user_data = user_data;
                
                // the 'on normal completion' function
                xhr.onload = function(e)
                {
                    var cb = this.c_callback;
                    delete this.c_callback;
                    if (this.status == 200 || this.status == 0)	// status == 0 is a firefox bug (perhaps only with blob urls??)
                    {
                        this.c_buffer = this.response;
                        this.c_read_pos = 0;
                        do_callback(cb, this.c_callback_user_data, MS_OK);
                    }
                    else
                        do_callback(cb, this.c_callback_user_data, status_code_to_error(this.status));
                }
                
                // the 'on error' function
                xhr.onerror = function(e)
                {
                    var cb = this.c_callback;
                    delete this.c_callback;
                    do_callback(cb, this.c_callback_user_data, status_code_to_error(this.status));
                }
                
                // convert the c-style string parameters (they are null-terminated, consecutive bytes in Module.HEAP)
                // to javascript strings and pass them to open.  Then tell the XMLHttpRequest what kind of object
                // we would like the reply in (an arraybuffer), and get it processing.  That results in eventually
                // calling 'onload' or 'onerror'.
                xhr.open(Module.Pointer_stringify(method), Module.Pointer_stringify(urlAddr), true);
                xhr.responseType = 'arraybuffer';
                xhr.send();
            }
            catch(err)
            {
                delete xhr.c_callback;
                do_callback(callbackAddr, user_data, MS_ERROR_FAILED);
            }
            return 0;
        }

        // only valid to call _after_ 'open_http_request' has called its given callbackAddr with MS_OK
        // returns the total download size of the file.
        function get_http_download_size(id)
        {
            return httpRequests[id].c_buffer.byteLength;
        }

        // only valid to call _after_ 'open_http_request' has called its given callbackAddr with MS_OK
        // copy 'len' bytes bytes from the download assocaited with 'id' into 'buffer'.  Can be called
        // in a streaming fashion.  In the initial state a virtual 'file position' is set to 0, and
        // on each call the first byte transfered to buffer[0] will be the byte at this 'file position'.
        // For each call it returns the number of bytes actually copied and updates this 'file position'
        // value by this same amount.  It may return MS_OK_COMPLETIONPENDING if it is not yet at the
        // end of the data expected to be downloaded, but does not have any current data available
        // to copy.
        function read_http_response(id, buffer, len)
        {
            if (len == 0)
                return 0;
            var xhr = httpRequests[id];
            if (typeof xhr === 'undefined')
                return MS_ERROR_BADARGUMENT;
            var buf = xhr.c_buffer;
            if (typeof buf === 'undefined')
                return MS_OK_COMPLETIONPENDING;
                
            var rpos = xhr.c_read_pos;
            
            var lenAvail = buf.byteLength - rpos;
            if (len > lenAvail)
                len = lenAvail;
            var retLen = len;
            
            if (((buffer & 3) == 0) && ((rpos & 3) == 0))	// both are 4-byte aligned, copy 4 at a time as many as we can
            {
                var thisLen = len >> 2;
                var u32 = new Uint32Array(buf,0,thisLen);
                var rp32 = rpos >> 2;
                var b32 = buffer >> 2;
                for (var i = 0; i < thisLen; i++)
                    Module.HEAPU32[b32+i] = u32[rp32+i];
                var bytes = thisLen << 2;
                rpos += bytes;
                len -= bytes;
                buffer += bytes;
            }
            if (len > 0)	// copy remaining 0-3 bytes if it was aligned, or all of them if it was not aligned
            {
                var u8 = new Uint8Array(buf);
                for (var i = 0; i < len; i++)
                    Module.HEAPU8[buffer+i] = u8[rpos+i];
            }
            
            xhr.c_read_pos += retLen;
            if (xhr.c_read_pos == buf.byteLength)
                delete xhr.c_buffer;
            
            return retLen;
        }
        
        // utilities...
        
        // call the given callbackAddr(user_data, result) after 'milli' milliseconds
        function timed_callback(milli, callbackAddr, user_data, result)
        {
            setTimeout( function() { do_callback(callbackAddr, user_data, result); }, milli );
        }
        
        // call the caller-supplied on_status with the given 'message' - simple debugging
        function post_string_message(msgAddr)
        {
            mutantspider.post_js_message(Module.Pointer_stringify(msgAddr));
        }
        
        function update_view(clientBounds)
        {
            if (change_view_proc != undefined)
                change_view_proc(0,0,clientBounds.width, clientBounds.height);
        }
        
        return {
            set_initialize:         set_initialize,
            initialize:             initialize,
            post_string_message:    post_string_message,
            bind_graphics:          bind_graphics,
            back_buffer_clear:      back_buffer_clear,
            stretch_blit_pixels:    stretch_blit_pixels,
            paint_back_buffer:      paint_back_buffer,
            update_view:            update_view,
            new_http_request:       new_http_request,
            delete_http_request:    delete_http_request,
            open_http_request:      open_http_request,
            get_http_download_size: get_http_download_size,
            read_http_response:     read_http_response,
            timed_callback:         timed_callback
        };
        
    }());

    var nacl_moduleEl;

    // send the given 'msg' to the component.  Supports both the case where the
    // component is nacl/pnacl, as well as asm.js -- although the support is
    // somewhat limited in the asm.js case.  The intended use is only to support
    // syntax of the form:
    //
    //		send_command( {someKey: 'someValue', someOtherKey: 'someOtherValue'} );
    //
    // where 'msg' is a dictionary (object), and the values are all strings.  It also
    // supports the case where a value is another object, but in this case it simply
    // copies that contents of that object as if it were a Uint8Array into raw memory
    // in Module.HEAP.  This form can be used on values that are things like typed arrays.
    function send_command(msg)
    {
        if (nacl_moduleEl != null)
        {
            nacl_moduleEl.postMessage(msg);
        }
        else
        {
            if ( typeof msg == 'object' )
            {
                var args = [];
                var argDesc = [];
                args.push(0);
                argDesc.push('number');
                var len = 0;
                for ( var prop in msg )
                {
                    if (typeof msg[prop] == 'string')
                    {
                        len += 1;
                        args.push(prop);
                        args.push(0);
                        args.push(msg[prop]);
                        argDesc.push('string');
                        argDesc.push('number');
                        argDesc.push('string');
                    }
                    else if (typeof msg[prop] == 'object')
                    {
                        len += 1;
                        args.push(prop);
                        var obj_len = msg[prop].byteLength;
                        args.push(obj_len);
                        var addr = Module._malloc(obj_len);
                        var ar = new Uint8Array(msg[prop]);
                        for (var i = 0; i < obj_len; i++)
                        {
                            Module.HEAPU8[addr+i] = ar[i];
                        }
                        args.push(addr);
                        argDesc.push('string');
                        argDesc.push('number');
                        argDesc.push('number');
                    }
                    else
                        throw "invalid type in send_command: " + typeof msg[prop];
                }
                args[0] = len;
                Module.ccall( 'MS_MessageProc', 'null', argDesc, args );
            }
        }
    }

    /*
        main entry point to initialize an element so that it is supported by a mutantspider component.
        
        element[in]     existing element in a DOM.  This should be the element that controls the visual
                        aspects of the component itself.
     
        params[in]      dictionary specifying various information about the component.  Must contain:
     
                            name:           string specifying the file name of the component, including the
                                            path prefix, for the file's location on the server, relative to
                                            the root directory being served.  This name should _not_ contain
                                            the file(s) suffix.  For a component named 'comp1' sitting in
                                            the server path 'components' there will be a collection of files
                                            including components/comp1.js, components/comp1.js.mem,
                                            components/comp1.pexe, etc...  All such component files must have
                                            the same base name and must be in the same location on the server.
                            asm_memory:     number of bytes to allocate to the asm.js heap.  This is only used
                                            in the asm.js case -- not the nacl/pnacl case.  This number should
                                            be a multiple of 4M.
     
        on_status[in]   function object that will be called to report various aspects of what is happening
                        to the component.  on_status will always be called with a dictionary, and that dictionary
                        will always have a 'status' attribute.  The following values for 'status' are possible:
                        
                            'loading'   - sent when the component is being downloaded from the server.  If your
                                          js/pnacl code is big this loading step can be long.  For the 'loading'
                                          status the following additional attributes will be defined:
                                          
                                          'exe_type'    - set to one of:
                                                            'PNaCl'
                                                            'NaCl'
                                                            'JavaScript'
                                                            'JavaScript Heap File'
                    
                            'running'   - sent after 'loading' has completed successfully.  Indicates that the
                                          component is now running correctly.  For the 'running' status the following
                                          additional attributes will be defined:
                                          
                                          'exe_type;    - set to one of:
                                                            'PNaCl'
                                                            'NaCl'
                                                            'JavaScript'
                    
                                          'using_web_gl'    - true if the underlying system is using webgl/opengl to render
                                          
                            'message'   - sent when the component itself sends a (typically debugging) message out
                                          to the web page.  For the 'message' status the following attributes will also
                                          be defined:
                                          
                                          'message'     - the message itself (typically a string)
                                          
                            'error'     - sent when the system is unable to load the component itself for some reason.
                                          For the 'error' status the following additional attributes will also be defined:
                                          
                                          'message'     - a description of the reason for the error
                                          
                            'crash'     - sent when a nacl or pnacl module crashes.  This message is never sent for JavaScript
                                          modules.  For the 'crash' status the following additional attributes will also be defined:
                                          
                                          'message'     - a description of the reason for the crash
                    
    */
    function initialize_element(element, params, on_status)
    {
        main_on_status = on_status;

        // does the browser support pnacl (probably just chrome)?
        if (navigator.mimeTypes['application/x-pnacl'] !== undefined)
        {
            using_web_gl = true;
            on_status({status: 'loading', exe_type: 'PNaCl'});

            element.addEventListener('message', function(event){post_js_message(event.data);}, true);
            element.addEventListener('error',   function(event){on_status({status: 'error',   message: event});}, true);
            element.addEventListener('crash',   function(event){on_status({status: 'crash',   message: event});}, true);

            nacl_moduleEl = document.createElement('embed');
            nacl_moduleEl.setAttribute('src', params.name + '.nmf');
            nacl_moduleEl.setAttribute('type', 'application/x-pnacl');
            nacl_moduleEl.setAttribute('style', 'background-color:#808080;width:100%;height:100%');
            nacl_moduleEl.setAttribute('locale', navigator.language );
            nacl_moduleEl.setAttribute('has_webgl', 'true');
            nacl_moduleEl.setAttribute('local_storage', 0);	// assume failure

            if (params.local_storage)
            {
                navigator.webkitPersistentStorage.requestQuota(params.local_storage,
                                                                function(bytes)
                                                                {
                                                                    nacl_moduleEl.setAttribute('local_storage', bytes);
                                                                    element.appendChild(nacl_moduleEl);
                                                                },
                                                                function(err)
                                                                {
                                                                    element.appendChild(nacl_moduleEl);
                                                                });
            }
            else
                element.appendChild(nacl_moduleEl);
        }
        else
        {
            Module = {};
            Module.TOTAL_MEMORY = params.asm_memory;

            on_status({status: 'loading', exe_type: 'JavaScript'});

            var scriptEl = document.createElement('script');
            scriptEl.type = 'text/javascript';
            scriptEl.src = params.name + '.js';
            scriptEl.setAttribute('style', 'background-color:#808080;width:inherit;100%:100%');
            scriptEl.onload = function()
            {
                on_status({status: 'loading', exe_type: 'JavaScript Heap File'});
                asm.set_initialize(element, on_status);
            };
            scriptEl.addEventListener('error', function(event){on_status({status: 'error', message: event});}, true);
            element.appendChild(scriptEl);

            // for now, just run a polling loop checking for element resize "events"
            // New proposals (as of July 2014) for extensions to MutationObserver could potentially
            // be used with tricks that use media queries and unused (or also currently proposed
            // "custom") style attributes could potentially solve this problem without needing
            // a polling loop.  But lots of developers look for solutions to the problem of
            // being notified when the size of an element changes, and perhaps some other solution
            // will be introduced.
            var curBounds = element.getBoundingClientRect();
            setInterval( function() {
                                        var cb = element.getBoundingClientRect();
                                        if (cb.width != curBounds.width || cb.height != curBounds.height)
                                        {
                                            asm.update_view(cb);
                                            curBounds = cb;
                                        }
                                    },
                                    125
                                    );
        }
    }

    scope.mutantspider = new Object();
    scope.mutantspider['asm_internal'] = asm;
    scope.mutantspider['set_using_web_gl'] = set_using_web_gl;
    scope.mutantspider['post_js_message'] = post_js_message;

    scope.mutantspider['send_command'] = send_command;
    scope.mutantspider['initialize_element'] = initialize_element;
 
})(window);



