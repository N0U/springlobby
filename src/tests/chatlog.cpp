#define BOOST_TEST_MODULE slconfig
#include <boost/test/unit_test.hpp>

#include "chatlog.h"

#include <wx/string.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/filefn.h>

void customMessageBox(int, wxString const&, wxString const&, long, int, int) {}


BOOST_AUTO_TEST_CASE( slconfig )
{
	const wxString line1 = _T("this is line 1");
	const wxString line2 = _T("this is line 2");
	const wxString line3 = _T("this is line 3");

	ChatLog* logfile;
	logfile = new ChatLog();
	BOOST_CHECK(logfile->SetLogFile(_T("test")));
	BOOST_CHECK(logfile->AddMessage(line1));
	BOOST_CHECK(logfile->AddMessage(line2));
	BOOST_CHECK(logfile->AddMessage(line3));

	delete logfile;
	logfile = new ChatLog();
	BOOST_CHECK(logfile->SetLogFile(_T("test")));

	wxArrayString lines;
	lines = logfile->GetLastLines();
	for(auto line: lines) {
		wxLogMessage(_T("line: '%s'"), line.c_str());
	}
	BOOST_CHECK_MESSAGE(lines.GetCount() > 3, lines.GetCount());

	const int skip = 11; //ignore date
	BOOST_CHECK(lines[lines.GetCount()-4].Mid(skip) == line1);
	BOOST_CHECK(lines[lines.GetCount()-3].Mid(skip) == line2);
	BOOST_CHECK(lines[lines.GetCount()-2].Mid(skip) == line3);

	delete logfile;
}
