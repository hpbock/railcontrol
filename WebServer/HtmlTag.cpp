/*
RailControl - Model Railway Control Software

Copyright (c) 2017-2019 Dominik (Teddy) Mahrer - www.railcontrol.org

RailControl is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

RailControl is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RailControl; see the file LICENCE. If not see
<http://www.gnu.org/licenses/>.
*/

#include <sstream>

#include "WebServer/HtmlTag.h"

namespace WebServer
{
	HtmlTag HtmlTag::AddAttribute(const std::string& name, const std::string& value)
	{
		if (name.size() == 0)
		{
			return *this;
		}
		this->attributes[name] = value;
		return *this;
	}

	HtmlTag HtmlTag::AddChildTag(const HtmlTag& child)
	{
		this->childTags.push_back(child);
		return *this;
	}

	HtmlTag HtmlTag::AddContent(const std::string& content)
	{
		this->content += content;
		return *this;
	}

	HtmlTag HtmlTag::AddClass(const std::string& _class)
	{
		classes.push_back(_class);
		return *this;
	}

	HtmlTag::operator std::string () const
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

	std::ostream& operator<<(std::ostream& stream, const HtmlTag& tag)
	{
		if (tag.name.size() > 0)
		{
			stream << "<" << tag.name;
			for (auto attribute : tag.attributes)
			{
				stream << " " << attribute.first;
				if (attribute.second.size() > 0)
				{
					stream << "=" << "\"" << attribute.second << "\"";
				}
			}
			if (tag.classes.size() > 0)
			{
				stream << " class=\"";
				for(auto c : tag.classes)
				{
					stream << " " << c;
				}
				stream << "\"";
			}

			stream << ">";

			if (tag.childTags.size() == 0 && tag.content.size() == 0 && (
				tag.name.compare("input") == 0 ||
				tag.name.compare("link") == 0 ||
				tag.name.compare("meta") == 0 ||
				tag.name.compare("br") == 0))
			{
				return stream;
			}
		}

		for (auto child : tag.childTags)
		{
			stream << child;
		}

		stream << tag.content;

		if (tag.name.size() > 0)
		{
			stream << "</" << tag.name << ">";
		}
		return stream;
	}
};