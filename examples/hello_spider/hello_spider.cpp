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

#include "mutantspider.h"

#include "math.h"   // for sqrt


/*
    An instance of HelloSpiderInstance is created by the initialization logic
    (pp::CreateModule()->CreateInstance()).  This instance is then associated
    with the component on the web page that initiated this code.  Virtual methods
    like DidChangeView and HandleInputEvent are called in response to event
    processing in the browser.
    
    This HelloSpiderInstance contains a small bit of boilerplate logic, but
    primarily implements some logic that paints a black dot where the mouse
    currently is.
*/
class HelloSpiderInstance : public MS_AppInstance
{
public:
    explicit HelloSpiderInstance(MS_Instance instance)
                : MS_AppInstance(instance),
                  callback_factory_(this),
                  blitting_(false),
                  mouse_down_(false),
                  mouse_loc_x_(0),
                  mouse_loc_y_(0)
            {}

    ~HelloSpiderInstance() {}
            
    #if defined(__native_client__)
    virtual bool Init(uint32_t argc, const char* argn[], const char* argv[])
    #elif defined(EMSCRIPTEN)
    virtual bool Init(EM_InitFlags flags)
    #endif
    {
        // even though HelloSpiderInstance doesn't do any file system code,
        // we still need to call this to complete the initialization logic.
        mutantspider::init_fs(this);

        return true;
    }
    
    virtual void DidChangeView(const mutantspider::View& view)
    {
        if (!CreateContext(view.GetRect().size()))
            return;

        Paint();
    }
    
    virtual bool HandleInputEvent(const mutantspider::InputEvent& event)
    {
        if (event.GetType() == MS_INPUTEVENT_TYPE_MOUSEDOWN || event.GetType() == MS_INPUTEVENT_TYPE_TOUCHSTART)
            mouse_down_ = true;
        else if(event.GetType() == MS_INPUTEVENT_TYPE_MOUSEUP || event.GetType() == MS_INPUTEVENT_TYPE_TOUCHEND)
            mouse_down_ = false;
            
        if (mouse_down_)
        {
            switch(event.GetType())
            {
                case MS_INPUTEVENT_TYPE_MOUSEDOWN:
                case MS_INPUTEVENT_TYPE_MOUSEUP:
                case MS_INPUTEVENT_TYPE_MOUSEMOVE:
                case MS_INPUTEVENT_TYPE_MOUSEENTER:
                case MS_INPUTEVENT_TYPE_MOUSELEAVE:
                {
                    mutantspider::MouseInputEvent mouse_event(event);
                    mouse_loc_x_ = mouse_event.GetPosition().x();
                    mouse_loc_y_ = mouse_event.GetPosition().y();
                    if (!blitting_)
                        Paint();
                }   return true;
                    
                case MS_INPUTEVENT_TYPE_TOUCHSTART:
                case MS_INPUTEVENT_TYPE_TOUCHMOVE:
                {
                    mutantspider::TouchInputEvent touch_event(event);
                    if (touch_event.GetTouchCount(MS_TOUCHLIST_TYPE_TOUCHES) == 1)
                    {
                        mouse_loc_x_ = (int)touch_event.GetTouchByIndex(MS_TOUCHLIST_TYPE_TOUCHES, 0).position().x();
                        mouse_loc_y_ = (int)touch_event.GetTouchByIndex(MS_TOUCHLIST_TYPE_TOUCHES, 0).position().y();
                        if (!blitting_)
                            Paint();
                    }
                }   return true;
                    
                default:
                    break;
            }
        }
        return false;
    }
    
    bool CreateContext(const mutantspider::Size& new_size)
    {
        const bool kIsAlwaysOpaque = true;
        context_ = mutantspider::Graphics2D(this, new_size, kIsAlwaysOpaque);
        if (!BindGraphics(context_))
        {
            fprintf(stderr, "Unable to bind 2d context!\n");
            context_ = mutantspider::Graphics2D();
            return false;
        }

        size_ = new_size;

        return true;
    }
    
    void Paint()
    {
        // get the rgb vs. bgr layout that is optimal for this system
        MS_ImageDataFormat format = mutantspider::ImageData::GetNativeImageDataFormat();
        
        // allocate am image buffer in the optimal format, the size of our view
        mutantspider::ImageData image_data(this, format, size_, false/*don't bother to init to zero*/);

        // get a pointer to its pixels
        uint32_t* data = static_cast<uint32_t*>(image_data.data());
        
        // 'g' and 'a' are in the same position for rgb vs. bgr, but
        // 'r' and 'b' reverse positions
        int ri, gi = 1, bi, ai = 3;
        if (format == MS_IMAGEDATAFORMAT_RGBA_PREMUL)
        {
            ri = 0;
            bi = 2;
        }
        else
        {
            bi = 0;
            ri = 2;
        }
            
        // paint the red field with a black circle around the current mouse position
        size_t offset = 0;
        for (int y=0;y<size_.height();y++)
        {
            for (int x=0;x<size_.width();x++)
            {
                int xdist = x - mouse_loc_x_;
                int ydist = y - mouse_loc_y_;
                int dist = (int)(sqrt(xdist*xdist + ydist*ydist));
                if (dist > 255)
                    dist = 255;
                    
                uint8_t	px[4];
                px[ri] = dist;
                px[gi] = px[bi] = 0;
                px[ai] = 255;

                data[offset] = *(uint32_t*)&px[0];
                ++offset;
            }
        }

        // display our bitmap
        context_.ReplaceContents(&image_data);
        blitting_ = true;
        context_.Flush(callback_factory_.NewCallback(&HelloSpiderInstance::BlitDone));
    }
    
    void BlitDone(uint32_t)
    {
        blitting_ = false;
    }
    
private:
    mutantspider::CompletionCallbackFactory<HelloSpiderInstance> callback_factory_;
    mutantspider::Graphics2D context_;
    mutantspider::Size size_;
    bool			blitting_;
    bool			mouse_down_;
    int				mouse_loc_x_;
    int				mouse_loc_y_;
};


/*
    standard Module, whose CreateInstance is called during startup
    to provide the Instance object which the browser interacts with.
    Mutantspider follows the Pepper design here of a two-stage
    initialization sequence.  pp::CreateModule is the first stage,
    that Module's (this HellowSpiderModule's) CreateInstance is the
    second stage.
*/
class HelloSpiderModule : public MS_Module
{
public:
    virtual MS_AppInstancePtr CreateInstance(MS_Instance instance)
    {
        return new HelloSpiderInstance(instance);
    }
};


/*
    a bit of crappy, almost boilerplace code.
    
    This is the once place in mutantspider where it leaves the Pepper
    "pp" namespace in tact, and just follows the pepper model for how
    the basic initialization works.  To be a functioning mutantspider
    component, you have to implement pp::CreateModule.  It will be called
    during initialization of your component.  It is required to return
    an allocated instance of a Module.  The (virtual) methods of that
    Module are then called to actually get going.
*/
    
namespace pp {

MS_Module* CreateModule() { return new HelloSpiderModule(); }

}
