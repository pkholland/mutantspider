#pragma once

#if defined(EMSCRIPTEN)

typedef enum {
	MS_URLREQUESTPROPERTY_URL = 0,
	MS_URLREQUESTPROPERTY_METHOD = 1,
	MS_URLREQUESTPROPERTY_HEADERS = 2,
	MS_URLREQUESTPROPERTY_STREAMTOFILE = 3,
	MS_URLREQUESTPROPERTY_FOLLOWREDIRECTS = 4,
	MS_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS = 5,
	MS_URLREQUESTPROPERTY_RECORDUPLOADPROGRESS = 6,
	MS_URLREQUESTPROPERTY_CUSTOMREFERRERURL = 7,
	MS_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS = 8,
	MS_URLREQUESTPROPERTY_ALLOWCREDENTIALS = 9,
	MS_URLREQUESTPROPERTY_CUSTOMCONTENTTRANSFERENCODING = 10,
	MS_URLREQUESTPROPERTY_PREFETCHBUFFERUPPERTHRESHOLD = 11,
	MS_URLREQUESTPROPERTY_PREFETCHBUFFERLOWERTHRESHOLD = 12,
	MS_URLREQUESTPROPERTY_CUSTOMUSERAGENT = 13
} MS_URLRequestProperty;

typedef enum {
	MS_URLRESPONSEPROPERTY_URL = 0,
	MS_URLRESPONSEPROPERTY_REDIRECTURL = 1,
	MS_URLRESPONSEPROPERTY_REDIRECTMETHOD = 2,
	MS_URLRESPONSEPROPERTY_STATUSCODE = 3,
	MS_URLRESPONSEPROPERTY_STATUSLINE = 4,
	MS_URLRESPONSEPROPERTY_HEADERS = 5
} MS_URLResponseProperty;

namespace mutantspider
{

class URLRequestInfo
{
public:
	URLRequestInfo()
		: m_stream_to_file(false),
		  m_follow_redirects(false),
		  m_record_downlaod_progress(false),
		  m_record_upload_progress(false),
		  m_allow_COR(false),
		  m_allow_credentials(false),
		  m_buffer_upper_threshold(0),
		  m_buffer_lower_threshold(0)
	{}
	
	URLRequestInfo(MS_AppInstance* inst)
		: m_stream_to_file(false),
		  m_follow_redirects(false),
		  m_record_downlaod_progress(false),
		  m_record_upload_progress(false),
		  m_allow_COR(false),
		  m_allow_credentials(false),
		  m_buffer_upper_threshold(0),
		  m_buffer_lower_threshold(0)
	{}
	
//	explicit URLRequestInfo(const InstanceHandle& instance);
	bool SetProperty(MS_URLRequestProperty property, const Var& value);
	bool AppendDataToBody(const void* data, uint32_t len);
//	bool AppendFileToBody(const FileRef& file_ref,
//							PP_Time expected_last_modified_time = 0);
//	bool AppendFileRangeToBody(const FileRef& file_ref,
//							int64_t start_offset,
//							int64_t length,
//							PP_Time expected_last_modified_time = 0);

	bool SetURL(const Var& url_string)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_URL, url_string);
	}
	bool SetMethod(const Var& method_string)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_METHOD, method_string);
	}
	bool SetHeaders(const Var& headers_string)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_HEADERS, headers_string);
	}
	bool SetStreamToFile(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_STREAMTOFILE, enable);
	}
	bool SetFollowRedirects(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_FOLLOWREDIRECTS, enable);
	}
	bool SetRecordDownloadProgress(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS, enable);
	}
	bool SetRecordUploadProgress(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_RECORDUPLOADPROGRESS, enable);
	}
	bool SetCustomReferrerURL(const Var& url)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_CUSTOMREFERRERURL, url);
	}
	bool SetAllowCrossOriginRequests(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS, enable);
	}
	bool SetAllowCredentials(bool enable)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_ALLOWCREDENTIALS, enable);
	}
	bool SetCustomContentTransferEncoding(const Var& content_transfer_encoding)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_CUSTOMCONTENTTRANSFERENCODING,
							content_transfer_encoding);
	}
	bool SetPrefetchBufferUpperThreshold(int32_t size)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_PREFETCHBUFFERUPPERTHRESHOLD, size);
	}
	bool SetPrefetchBufferLowerThreshold(int32_t size)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_PREFETCHBUFFERLOWERTHRESHOLD, size);
	}
	bool SetCustomUserAgent(const Var& user_agent)
	{
		return SetProperty(MS_URLREQUESTPROPERTY_CUSTOMUSERAGENT, user_agent);
	}

private:

	friend class URLLoader;
	
	std::string	m_url;
	std::string m_method;
	std::string m_headers;
	bool		m_stream_to_file;
	bool		m_follow_redirects;
	bool		m_record_downlaod_progress;
	bool		m_record_upload_progress;
	std::string	m_custom_referrer_url;
	bool		m_allow_COR;
	bool		m_allow_credentials;
	std::string	m_custom_content_encoding;
	int32_t		m_buffer_upper_threshold;
	int32_t		m_buffer_lower_threshold;
	std::string	m_custom_user_agent;

};

class URLResponseInfo
{
public:
	URLResponseInfo() {}

	Var GetProperty(MS_URLResponseProperty property) const;

	//FileRef GetBodyAsFileRef() const;

	Var GetURL() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_URL);
	}

	Var GetRedirectURL() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_REDIRECTURL);
	}

	Var GetRedirectMethod() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_REDIRECTMETHOD);
	}

	int32_t GetStatusCode() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_STATUSCODE).AsInt();
	}

	Var GetStatusLine() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_STATUSLINE);
	}

	Var GetHeaders() const
	{
		return GetProperty(MS_URLRESPONSEPROPERTY_HEADERS);
	}
};

class URLLoader
{
public:
	URLLoader() : m_js_loader(-1) {}
	~URLLoader()
	{
		if ( m_js_loader != -1 )
			ms_delete_http_request(m_js_loader);
	}
	
	explicit URLLoader(const InstanceHandle& instance)
		: m_js_loader(ms_new_http_request())
	{}
	
	int32_t Open(const URLRequestInfo& request_info,
					const CompletionCallback& cc)
	{
		return ms_open_http_request(m_js_loader,request_info.m_method.c_str(),request_info.m_url.c_str(),cc.get_proc(),cc.get_user_data());
	}
	
	int32_t FollowRedirect(const CompletionCallback& cc);
	bool GetUploadProgress(int64_t* bytes_sent,
							int64_t* total_bytes_to_be_sent) const;
	bool GetDownloadProgress(int64_t* bytes_received,
							int64_t* total_bytes_to_be_received) const
	{
		*bytes_received = *total_bytes_to_be_received = ms_get_http_download_size(m_js_loader);
		return true;
	}
	
	URLResponseInfo GetResponseInfo() const;
	int32_t ReadResponseBody(void* buffer,
							int32_t bytes_to_read,
							const CompletionCallback& cc)
	{
		return ms_read_http_response(m_js_loader,buffer,bytes_to_read);
	}
	
	int32_t FinishStreamingToFile(const CompletionCallback& cc);
	void Close();
	
private:
	int	m_js_loader;

};

}

#endif
