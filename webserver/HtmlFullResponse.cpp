#include <sstream>

#include "webserver/HtmlFullResponse.h"

namespace webserver
{
	HtmlFullResponse::HtmlFullResponse(const responseCode_t responseCode)
	:	HtmlResponse(responseCode)
	{}

	HtmlFullResponse::HtmlFullResponse(const std::string& title, const HtmlTag body)
	:	HtmlResponse(title, body)
	{}

	HtmlFullResponse::HtmlFullResponse(const responseCode_t responseCode, const std::string& title, const HtmlTag body)
	:	HtmlResponse(responseCode, title, body)
	{}

	HtmlFullResponse::operator std::string()
	{
		std::stringstream reply;
		reply << *this;
		return reply.str();
	}

	std::ostream& operator<<(std::ostream& stream, const HtmlFullResponse& response)
	{
		stream << "HTTP/1.0 " << response.responseCode << " " << HtmlResponse::responseTexts.at(response.responseCode) << "\r\n";
		for(auto header : response.headers)
		{
			stream << header.first << ": " << header.second << "\r\n";
		}
		stream << "\r\n";
		stream << "<!DOCTYPE html>";

		HtmlTag head("head");
		head.AddChildTag(HtmlTag("title").AddContent(response.title));
		head.AddChildTag(HtmlTag("link").AddAttribute("rel", "stylesheet").AddAttribute("type", "text/css").AddAttribute("href", "/style.css"));
		head.AddChildTag(HtmlTag("script").AddAttribute("type", "application/javascript").AddAttribute("src", "/jquery-3.1.1.min.js"));
		head.AddChildTag(HtmlTag("script").AddAttribute("type", "application/javascript").AddAttribute("src", "/javascript.js"));
		head.AddChildTag(HtmlTag("meta").AddAttribute("name", "viewport").AddAttribute("content", "width=device-width, initial-scale=1.0"));
		head.AddChildTag(HtmlTag("meta").AddAttribute("name", "robots").AddAttribute("content", "noindex,nofollow"));

		stream << HtmlTag("html").AddChildTag(head).AddChildTag(response.content);
		return stream;
	}
};