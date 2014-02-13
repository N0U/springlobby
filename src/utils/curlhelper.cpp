#include "curlhelper.h"
#include "settings.h"
#include <wx/log.h>

extern "C"
{
	size_t wxcurl_string_write(void* ptr, size_t size, size_t nmemb, void* pcharbuf)
	{
		size_t iRealSize = size * nmemb;
		wxCharBuffer* pStr = (wxCharBuffer*) pcharbuf;

		if(pStr)
		{
			wxString str = wxCURL_BUF2STRING(*pStr) + wxString((const char*)ptr, wxConvLibc);
			*pStr = wxCURL_STRING2BUF(str);
		}

		return iRealSize;
	}
	size_t wxcurl_stream_write(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t iRealSize = size * nmemb;

		wxOutputStream* pBuf = (wxOutputStream*)stream;

		if(pBuf)
		{
			pBuf->Write(ptr, iRealSize);

			return pBuf->LastWrite();
		}

		return 0;
	}
	/* reads from a stream */
	size_t wxcurl_stream_read(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t iRealSize = size * nmemb;

		wxInputStream* pBuf = (wxInputStream*)stream;

		if(pBuf)
		{
			pBuf->Read(ptr, iRealSize);

			return pBuf->LastRead();
		}

		return 0;
	}
}//end extern "C"


wxString Paste2Pastebin( const wxString& message )
{
	#ifndef __WXMAC__
	wxStringOutputStream response;
	wxStringOutputStream rheader;
	CURL *curl_handle;
	curl_handle = curl_easy_init();
	struct curl_slist* m_pHeaders = NULL;
	struct curl_httppost*   m_pPostHead = NULL;
	struct curl_httppost*   m_pPostTail = NULL;
    static const char* url = "http://paste.springfiles.com/api/create";
	// these header lines will overwrite/add to cURL defaults
	m_pHeaders = curl_slist_append(m_pHeaders, "Expect:") ;

	//we need to keep these buffers around for curl op duration
	wxCharBuffer message_buffer = message.mb_str();
	wxCharBuffer nick_buffer = sett().GetServerAccountNick( sett().GetDefaultServer() ).mb_str();

	curl_formadd(&m_pPostHead,
				 &m_pPostTail,
                 CURLFORM_COPYNAME, "text",
				 CURLFORM_COPYCONTENTS, (const char*)message_buffer,
				 CURLFORM_END);
	curl_formadd(&m_pPostHead,
				 &m_pPostTail,
                 CURLFORM_COPYNAME, "private",
                 CURLFORM_COPYCONTENTS, "1",
				 CURLFORM_END);
	curl_formadd(&m_pPostHead,
				 &m_pPostTail,
                 CURLFORM_COPYNAME, "name",
				 CURLFORM_COPYCONTENTS, (const char*)nick_buffer,
				 CURLFORM_END);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, m_pHeaders);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url );
//	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, wxcurl_stream_write);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void *)&rheader);
	curl_easy_setopt(curl_handle, CURLOPT_POST, TRUE);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, m_pPostHead);

	CURLcode ret = curl_easy_perform(curl_handle);

	wxLogError( rheader.GetString()  );

  /* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	curl_formfree(m_pPostHead);

	if(ret == CURLE_OK)
		return response.GetString();
	else
	#endif

	return wxEmptyString;
}
