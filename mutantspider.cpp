#include "mutantspider.h"

static char gLocale[128];

namespace mutantspider
{

  const char* locale()
  {
    return &gLocale[0];
  }

}

extern "C" void MS_SetLocale(const char* locale)
{
  size_t len = strlen(locale);
  if (len >= sizeof(gLocale))
    len = sizeof(gLocale)-1;
  memcpy(&gLocale[0], locale, len);
  gLocale[len] = 0;
}

#ifdef EMSCRIPTEN

// called by the emscripten runtime once our
// <component>.js.mem file has been downloaded
// and used to initialize the intial state of
// our heap
extern "C" int main(int, char**)
{
  MS_SetLocale(ms_get_browser_language());

  // MS_Init calls init_fs which calls async_startup_complete
  MS_Init("");
  return 0;
}

extern "C" void MS_Callback(ms_callback_base* cb)
{
  cb->exec();
  delete cb;
}

#endif

#if defined(__native_client__)

pp::Instance* gGlobalPPInstance;
int gModuleID;

#if 0

#include "ppapi/cpp/var.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/cpp/url_response_info.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/utility/completion_callback_factory.h"


void ms_post_message(const char* json_msg)
{
  gGlobalPPInstance->PostMessage(std::string(json_msg));
}

// this call is made during startup, before the json
// strategy starts in the JS code, so does _not_ use
// a json encoded message.
void ms_async_startup_complete(const char* err)
{
  if (err)
    ms_post_message("async_startup_failed");
  else
    ms_post_message("async_startup_complete");
}

static std::string escape(std::string str) {
    size_t start_pos = 0;
    while (true) {
      size_t quote_pos = str.find("\"", start_pos);
      size_t slash_pos = str.find("\\", start_pos);
      if (quote_pos < slash_pos) {
        str.replace(quote_pos, 1, "\\\"");
        start_pos = quote_pos + 2;
      } else if (slash_pos != std::string::npos) {
        str.replace(start_pos, 1, "\\\\");
        start_pos = slash_pos + 2;
      } else
        break;
    }
    return str;
}

void ms_consolelog(const char* message)
{
  std::ostringstream msg;
  msg << "{\"api\":\"consolelog\",\"args\":[\"" << escape(message) << "\"]}";
  ms_post_message(msg.str().c_str());
}

namespace {

	class url_handler
	{
	public:
		url_handler(const char* url, void (*proc)(void*, void*, size_t, const char*), void* user_data)
	    : proc_(proc),
	      callback_factory_(this),
	      reqInfo_(gGlobalPPInstance),
	      loader_(gGlobalPPInstance),
	      file_size_(0),
	      file_offset_(0),
	      read_buff_(0),
	      user_data_(user_data)
    {
	    reqInfo_.SetMethod("GET");
	    reqInfo_.SetURL(std::string(url));
	    reqInfo_.SetRecordDownloadProgress(true);
	    reqInfo_.SetAllowCrossOriginRequests(true);
	    auto rslt = loader_.Open(reqInfo_, callback_factory_.NewCallback(&url_handler::on_open));
    }
		
    ~url_handler()
    {
      free(read_buff_);
    }

	private:
		// callback methods to deal with async-io data reading
		void on_open(int32_t result)
		{
      if (result != PP_OK) {
        proc_(user_data_, 0, 0, "Open failed");
        delete this;
        return;
      }

      int64_t bytes_received = 0;
      int64_t total_bytes_to_be_received = 0;
      if (loader_.GetDownloadProgress(&bytes_received, &total_bytes_to_be_received)) {
        file_size_ = (size_t)total_bytes_to_be_received;
        read_buff_ = (char*)malloc(file_size_);
        if (!read_buff_) {
          proc_(user_data_, 0, 0, "malloc failed");
          delete this;
          return;
        }
      }

      reqInfo_.SetRecordDownloadProgress(false);
      read_body();
		}
		
		void read_body()
		{
      pp::CompletionCallback cc =
        callback_factory_.NewOptionalCallback(&url_handler::on_read);

      int32_t result = PP_OK;
      do {
        size_t sz = file_size_ - file_offset_;
        result = loader_.ReadResponseBody(&read_buff_[file_offset_], sz, cc);
        if (result > 0)
          file_offset_ += result;
      } while (result > 0);

      if (result != PP_OK_COMPLETIONPENDING)
        cc.Run(result);
		}
		
		void on_read(int32_t result)
		{
      if (result == PP_OK) {
        proc_(user_data_, read_buff_, file_size_, "");
        read_buff_ = 0;
      } else if (result > 0) {
        file_offset_ += result;
        if (file_offset_ < file_size_) {
          read_body();
          return;
        } else {
          proc_(user_data_, read_buff_, file_size_, "");
          read_buff_ = 0;
        }
      }
      else
        proc_(user_data_, 0, 0, "url download failed");
      delete this;
		}

		void (*proc_)(void*, void*, size_t, const char*);
		void*                                       user_data_;
		pp::CompletionCallbackFactory<url_handler>  callback_factory_;
		pp::URLRequestInfo                          reqInfo_;
		pp::URLLoader                               loader_;
		size_t                                      file_size_;
		size_t                                      file_offset_;
		char*                                       read_buff_;
	};
}

void ms_url_fetch_glue(const char* url, void (*proc)(void*, void*, size_t, const char*), void* user_data)
{
  new url_handler(url, proc, user_data);
}

#endif

#endif



