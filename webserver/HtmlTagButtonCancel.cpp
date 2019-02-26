#include <sstream>

#include "webserver/HtmlTagButtonCancel.h"

namespace webserver
{
	HtmlTagButtonCancel::HtmlTagButtonCancel()
	:	HtmlTagButton(HtmlTag("span").AddClass("symbola").AddContent("&#x2718;"), "popup_cancel")
	{
		AddAttribute("onclick", "document.getElementById('popup').style.display = 'none'; return false;");
	}
};
