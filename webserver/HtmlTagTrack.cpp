#include <sstream>

#include "webserver/HtmlTagTrack.h"
#include "webserver/HtmlTagJavascript.h"

using std::string;
using std::to_string;

namespace webserver
{
	HtmlTagTrack::HtmlTagTrack(const datamodel::Track* track)
	{
		layoutPosition_t posX;
		layoutPosition_t posY;
		layoutPosition_t posZ;
		layoutItemSize_t w;
		layoutItemSize_t h;
		layoutRotation_t r;
		track->position(posX, posY, posZ, w, h, r);
		trackType_t type = track->Type();
		unsigned int layoutPosX = posX * 35;
		unsigned int layoutPosY = posY * 35;

		HtmlTag div1("div");
		string trackIdString = to_string(track->objectID);
		string id("t_" + trackIdString);
		div1.AddAttribute("id", id);
		div1.AddClass("layout_item");
		div1.AddClass("track_item");
		div1.AddAttribute("style", "left:" + to_string(layoutPosX) + "px;top:" + to_string(layoutPosY) + "px;");
		std::string image;
		string layoutHeight = to_string(35 * track->height);
		switch (type)
		{
			case TrackTypeLeft:
				image = "<polygon points=\"0,22 0,13 22,35 13,35\" fill=\"black\"/>";
				break;

			case TrackTypeRight:
				image = "<polygon points=\"35,13 35,22 22,35 13,35\" fill=\"black\"/>";
				break;

			case TrackTypeStraight:
			default:
				image = "<polygon points=\"13,0 22,0 22,35 13,35\" fill=\"black\"/>";
				break;
		}

		int translateX = 0;
		int translateY = 0;
		if (track->height > Height1)
		{
			image += "<polygon points=\"13,35 22,35 22," + layoutHeight + " 13," + layoutHeight + "\" fill=\"black\"/>";
			if (track->rotation == Rotation90)
			{
				translateX = -((((track->height - 1) * 35) + 1) / 2);
				translateY = -(((track->height - 1) * 35) / 2);
			}
			else if (track->rotation == Rotation270)
			{
				translateX = ((((track->height - 1) * 35) + 1) / 2);
				translateY = (((track->height - 1) * 35) / 2);
			}
		}

		div1.AddChildTag(HtmlTag().AddContent("<svg width=\"35\" height=\"" + layoutHeight + "\" id=\"" + id + "_img\" style=\"transform:rotate(" + datamodel::LayoutItem::Rotation(track->rotation) + "deg) translate(" + to_string(translateX) + "px," + to_string(translateY) + "px);\">" + image + "</svg>"));
		div1.AddChildTag(HtmlTag("span").AddClass("tooltip").AddContent(track->name));
		div1.AddAttribute("oncontextmenu", "return onContextTrack(event, " + trackIdString + ");");
		AddChildTag(div1);

		HtmlTag div2("div");
		div2.AddClass("contextmenu");
		div2.AddAttribute("id", id + "_context");
		div2.AddAttribute("style", "left:" + to_string(layoutPosX + 5) + "px;top:" + to_string(layoutPosY + 30) + "px;");
		div2.AddChildTag(HtmlTag("ul").AddClass("contextentries")
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddContent(track->name))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddContent("Edit").AddAttribute("onClick", "loadPopup('/?cmd=trackedit&track=" + trackIdString + "');"))
			.AddChildTag(HtmlTag("li").AddClass("contextentry").AddContent("Delete").AddAttribute("onClick", "loadPopup('/?cmd=trackaskdelete&track=" + trackIdString + "');"))
			);
		AddChildTag(div2);
	}
};
