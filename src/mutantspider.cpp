#include "mutantspider.h"

static char gLocale[128];

extern "C" void MS_SetLocale(const char* locale)
{
	size_t len = strlen(locale);
	if (len >= sizeof(gLocale))
		len = sizeof(gLocale)-1;
	memcpy(&gLocale[0], locale, len);
	gLocale[len] = 0;
}

namespace mutantspider
{

const char* locale()
{
	return &gLocale[0];
}

MS_AppInstancePtr gAppInstance;

}

#if defined(EMSCRIPTEN)

#include "emscripten.h"
#include <stdarg.h>

static MS_Module* gModule;

namespace pp
{
	MS_Module* CreateModule();
}

extern "C" {

void MS_Init(int init_flags)
{
	gModule = pp::CreateModule();
	mutantspider::gAppInstance = gModule->CreateInstance(0);
    const char* argn = "has_webgl";
    const char* argv = (init_flags & MS_FLAGS_WEBGL_SUPPORT) ? "true" : "false";
	mutantspider::gAppInstance->Init(1, &argn, &argv);
}

int MS_MouseProc( int eventType, int timeStamp, int modifiers, int button, int positionX, int positionY, int clickCount, int movementX, int movementY )
{
	if ( mutantspider::gAppInstance )
	{
		mutantspider::InputEvent evt(
					(MS_InputEvent_Type)eventType,
					(MS_TimeTicks)(timeStamp / 1000.0),
					(uint32_t)modifiers,
					(MS_InputEvent_MouseButton)button,
					mutantspider::Point(positionX, positionY),
					(int32_t)clickCount,
					mutantspider::Point(movementX, movementY) );
		return mutantspider::gAppInstance->HandleInputEvent(evt);
	}
	return 0;
}

int MS_TouchProc( int eventType, int timeStamp, int modifiers, int* touchData )
{
	if ( mutantspider::gAppInstance )
	{		
		mutantspider::MS_TouchPoint	touches[5];

		int numTouches = *touchData++;
		if (numTouches > sizeof(touches)/sizeof(touches[0]))
			numTouches = sizeof(touches)/sizeof(touches[0]);
			
		for (int i = 0; i < numTouches; i++)
		{
			touches[0].id = *touchData++;
			touches[0].position.x = *touchData++;
			touches[0].position.y = *touchData++;
			touches[0].radius.x = 0;
			touches[0].radius.y = 0;
			touches[0].rotation_angle = 0;
			touches[0].pressure = 0;
		}
		
		mutantspider::InputEvent evt(
					(MS_InputEvent_Type)eventType,
					(MS_TimeTicks)timeStamp,
					(uint32_t)modifiers,
					(uint32_t)numTouches,
					&touches[0]);
		return mutantspider::gAppInstance->HandleInputEvent(evt);
	}
	return 0;
}

void MS_FocusProc( int focus )
{
	if ( mutantspider::gAppInstance )
		mutantspider::gAppInstance->DidChangeFocus(focus != 0);
}

void MS_KeyProc(int eventType, int timeStamp, int modifiers, int keycode, int keytext)
{
	if ( mutantspider::gAppInstance )
	{
		char	buff[2];
		buff[0] = (char)keytext;
		buff[1] = 0;
		mutantspider::Var		v(buff);
		mutantspider::InputEvent	evt(
					(MS_InputEvent_Type)eventType,
					(MS_TimeTicks)(timeStamp / 1000.0),
					(uint32_t)modifiers,
					(uint32_t)keycode,
					v );
		mutantspider::gAppInstance->HandleInputEvent(evt);
	}
}

void MS_DidChangeView(int x, int y, int width, int height)
{
	if ( mutantspider::gAppInstance )
	{
		mutantspider::Rect view_rect(x, y, width, height);
		mutantspider::gAppInstance->DidChangeView(view_rect);
	}
}

/*
	vararg functions aren't easily callable from javascript because there would need to be significantly more
	support built into ccall and cwrap to let it understand the types you were trying to pass.  So here we
	just put a limit on the total number of string pairs we can accept vs. doing something like passing an
	array of string pointers.  The ccall mechanism for passing a string ends up doing a better job of allocating
	the string (it does it on the stack) than we could easily do ourselves if we tried to manually construct
	all the strings.
*/
#define DO_ONE_PAIR(x)									\
if (count >= x)											\
{														\
	std::string val;									\
	if (!vl##x)											\
		val = std::string(v##x);						\
	else												\
	{													\
		val = std::string(v##x, vl##x);					\
		free((void*)v##x);								\
	}													\
	map.insert(std::make_pair(std::string(k##x), val));	\
}

void MS_MessageProc(int count, const char* k1, int vl1, const char* v1, const char* k2, int vl2, const char* v2,
						const char* k3, int vl3, const char* v3, const char* k4, int vl4, const char* v4,
						const char* k5, int vl5, const char* v5, const char* k6, int vl6, const char* v6,
						const char* k7, int vl7, const char* v7, const char* k8, int vl8, const char* v8 )
{
	if ( mutantspider::gAppInstance )
	{
		std::map<std::string, std::string>	map;
		DO_ONE_PAIR(1)
		DO_ONE_PAIR(2)
		DO_ONE_PAIR(3)
		DO_ONE_PAIR(4)
		DO_ONE_PAIR(5)
		DO_ONE_PAIR(6)
		DO_ONE_PAIR(7)
		DO_ONE_PAIR(8)
		mutantspider::gAppInstance->HandleMessage(mutantspider::Var(map));
	}
}

void MS_DoCallbackProc(void (*proc)(void*, int32_t), void* user_data, int32_t result)
{
	proc(user_data, result);
}

void MS_AsyncStartupComplete()
{
    if (mutantspider::gAppInstance)
        mutantspider::gAppInstance->AsyncStartupComplete();
}

}

////////////////////////////////////////////////////////////////////////////////////

namespace {

void AdjustAlongAxis(int32_t dst_origin, int32_t dst_size,
                     int32_t* origin, int32_t* size) {
  if (*origin < dst_origin) {
    *origin = dst_origin;
    *size = std::min(dst_size, *size);
  } else {
    *size = std::min(dst_size, *size);
    *origin = std::min(dst_origin + dst_size, *origin + *size) - *size;
  }
}

}  // namespace

namespace mutantspider {

void Rect::Inset(int32_t left, int32_t top, int32_t right, int32_t bottom) {
  Offset(left, top);
  set_width(std::max<int32_t>(width() - left - right, 0));
  set_height(std::max<int32_t>(height() - top - bottom, 0));
}

void Rect::Offset(int32_t horizontal, int32_t vertical) {
  rect_.point.x += horizontal;
  rect_.point.y += vertical;
}

bool Rect::Contains(int32_t point_x, int32_t point_y) const {
  return (point_x >= x()) && (point_x < right()) &&
         (point_y >= y()) && (point_y < bottom());
}

bool Rect::Contains(const Rect& rect) const {
  return (rect.x() >= x() && rect.right() <= right() &&
          rect.y() >= y() && rect.bottom() <= bottom());
}

bool Rect::Intersects(const Rect& rect) const {
  return !(rect.x() >= right() || rect.right() <= x() ||
           rect.y() >= bottom() || rect.bottom() <= y());
}

Rect Rect::Intersect(const Rect& rect) const {
  int32_t rx = std::max(x(), rect.x());
  int32_t ry = std::max(y(), rect.y());
  int32_t rr = std::min(right(), rect.right());
  int32_t rb = std::min(bottom(), rect.bottom());

  if (rx >= rr || ry >= rb)
    rx = ry = rr = rb = 0;  // non-intersecting

  return Rect(rx, ry, rr - rx, rb - ry);
}

Rect Rect::Union(const Rect& rect) const {
  // special case empty rects...
  if (IsEmpty())
    return rect;
  if (rect.IsEmpty())
    return *this;

  int32_t rx = std::min(x(), rect.x());
  int32_t ry = std::min(y(), rect.y());
  int32_t rr = std::max(right(), rect.right());
  int32_t rb = std::max(bottom(), rect.bottom());

  return Rect(rx, ry, rr - rx, rb - ry);
}

Rect Rect::Subtract(const Rect& rect) const {
  // boundary cases:
  if (!Intersects(rect))
    return *this;
  if (rect.Contains(*this))
    return Rect();

  int32_t rx = x();
  int32_t ry = y();
  int32_t rr = right();
  int32_t rb = bottom();

  if (rect.y() <= y() && rect.bottom() >= bottom()) {
    // complete intersection in the y-direction
    if (rect.x() <= x()) {
      rx = rect.right();
    } else {
      rr = rect.x();
    }
  } else if (rect.x() <= x() && rect.right() >= right()) {
    // complete intersection in the x-direction
    if (rect.y() <= y()) {
      ry = rect.bottom();
    } else {
      rb = rect.y();
    }
  }
  return Rect(rx, ry, rr - rx, rb - ry);
}

Rect Rect::AdjustToFit(const Rect& rect) const {
  int32_t new_x = x();
  int32_t new_y = y();
  int32_t new_width = width();
  int32_t new_height = height();
  AdjustAlongAxis(rect.x(), rect.width(), &new_x, &new_width);
  AdjustAlongAxis(rect.y(), rect.height(), &new_y, &new_height);
  return Rect(new_x, new_y, new_width, new_height);
}

Point Rect::CenterPoint() const {
  return Point(x() + (width() + 1) / 2, y() + (height() + 1) / 2);
}

bool Rect::SharesEdgeWith(const Rect& rect) const {
  return (y() == rect.y() && height() == rect.height() &&
             (x() == rect.right() || right() == rect.x())) ||
         (x() == rect.x() && width() == rect.width() &&
             (y() == rect.bottom() || bottom() == rect.y()));
}

bool URLRequestInfo::SetProperty(MS_URLRequestProperty property, const Var& value)
{
	switch (property)
	{
		case MS_URLREQUESTPROPERTY_URL:
			m_url = value.AsString();
			return true;
			
		case MS_URLREQUESTPROPERTY_METHOD:
			m_method = value.AsString();
			return true;
			
		case MS_URLREQUESTPROPERTY_HEADERS:
			m_headers = value.AsString();
			return true;
			
		case MS_URLREQUESTPROPERTY_STREAMTOFILE:
			m_stream_to_file = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_FOLLOWREDIRECTS:
			m_follow_redirects = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS:
			m_record_downlaod_progress = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_RECORDUPLOADPROGRESS:
			m_record_upload_progress = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_CUSTOMREFERRERURL:
			m_custom_referrer_url = value.AsString();
			return true;
			
		case MS_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS:
			m_allow_COR = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_ALLOWCREDENTIALS:
			m_allow_credentials = value.AsBool();
			return true;
			
		case MS_URLREQUESTPROPERTY_CUSTOMCONTENTTRANSFERENCODING:
			m_custom_content_encoding = value.AsString();
			return true;
			
		case MS_URLREQUESTPROPERTY_PREFETCHBUFFERUPPERTHRESHOLD:
			m_buffer_upper_threshold = value.AsInt();
			return true;
			
		case MS_URLREQUESTPROPERTY_PREFETCHBUFFERLOWERTHRESHOLD:
			m_buffer_lower_threshold = value.AsInt();
			return true;
			
		case MS_URLREQUESTPROPERTY_CUSTOMUSERAGENT:
			m_custom_user_agent = value.AsString();
			return true;
	}
	return false;
}

////////////////////////////////////////////

Graphics3D::Graphics3D(MS_AppInstance* instance,
						const int32_t attrib_list[])
	: is_null_(false)
{
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
	int i = 0;
	int32_t width = 0, height = 0;
	bool done = false;
	while (!done)
	{
		switch (attrib_list[i++])
		{
			case MS_GRAPHICS3DATTRIB_NONE:
				done = true;
				break;
			case MS_GRAPHICS3DATTRIB_WIDTH:
				width = attrib_list[i++];
				break;
			case MS_GRAPHICS3DATTRIB_HEIGHT:
				height = attrib_list[i++];
				break;
			default:
			{
				auto v = attrib_list[i-1];
				if (v > MS_GRAPHICS3DATTRIB_NONE)
					++i;	// ignore this one, must be some pepper-special one
				else
					SDL_GL_SetAttribute( (SDL_GLattr)v, attrib_list[i++] );
				break;
			}
		}
	}

    ResizeBuffers(width, height);
}

void* Graphics3D::pp_resource()
{
	return 0;
}

int32_t Graphics3D::ResizeBuffers(int32_t width, int32_t height)
{
    if ( !SDL_SetVideoMode( width, height, 0, SDL_OPENGL ) )
        fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());

	return 0;
}

int32_t Graphics3D::SwapBuffers(const CompletionCallback& cc)
{
	SDL_GL_SwapBuffers();
	cc.Run(0);
	return 0;
}

////////////////////////////////////////////


}  // namespace mutantspider

extern "C" int main(int, char**)
{
	ms_initialize();
	return 0;
}

// EMSCRIPTEN
#endif


