/* GG is a GUI for SDL and OpenGL.
   Copyright (C) 2003 T. Zachary Laine

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1
   of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
    
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA

   If you do not wish to comply with the terms of the LGPL please
   contact the author as other terms are available for a fee.
    
   Zach Laine
   whatwasthataddress@hotmail.com */

/* $Header$ */

/** This notice came from the original file from which all the XML parsing code 
    was taken, part of the spirit distribution.  The code was modified slightly 
    by me, and doesn't contain all the original code.  Thanks to Daniel Nuffer 
    for his great work. */
/*=============================================================================
    simplexml.cpp

    Spirit V1.3
    URL: http://spirit.sourceforge.net/

    Copyright (c) 2001, Daniel C. Nuffer

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the copyright holder be held liable for
    any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute
    it freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must
        not claim that you wrote the original software. If you use this
        software in a product, an acknowledgment in the product documentation
        would be appreciated but is not required.

    2.  Altered source versions must be plainly marked as such, and must
        not be misrepresented as being the original software.

    3.  This notice may not be removed or altered from any source
        distribution.
=============================================================================*/


#include "XMLDoc.h"

#include <boost/spirit.hpp>


namespace GG {

namespace {
const string INDENT_STR = "  "; // indents are 2 spaces
string element_name;
string attribute_name;

using namespace boost::spirit;

typedef chset<unsigned char> chset_t;
typedef chlit<unsigned char> chlit_t;

// XML grammar rules
rule<> document, prolog, element, Misc, Reference,CData, doctypedecl, 
    XMLDecl, SDDecl, VersionInfo, EncodingDecl, VersionNum, Eq, 
    EmptyElemTag, STag, content, ETag, Attribute, AttValue, CharData, 
    Comment, CDSect, CharRef, EntityRef, EncName, Name, Comment1, S;

// XML Character classes
chset_t Char("\x9\xA\xD\x20-\xFF");
chset_t Letter("\x41-\x5A\x61-\x7A\xC0-\xD6\xD8-\xF6\xF8-\xFF");
chset_t Digit("0-9");
chset_t Extender('\xB7');
chset_t NameChar = Letter | Digit | chset_t("._:-") | Extender;
chset_t Sch("\x20\x9\xD\xA");
}

XMLDoc*              XMLDoc::s_curr_parsing_doc = 0;
vector<XMLElement*>  XMLDoc::s_element_stack;

////////////////////////////////////////////////
// GG::XMLElement
////////////////////////////////////////////////
bool XMLElement::ContainsChild(const string& child) const
{
    return ChildIndex(child) != -1;
}

bool XMLElement::ContainsAttribute(const string& attrib) const
{
    return m_attributes.find(attrib) != m_attributes.end();
}

int XMLElement::ChildIndex(const string& child) const
{
    int retval = -1;
    for (unsigned int i = 0; i < m_children.size(); ++i) {
        if (m_children[i].m_tag == child) {
            retval = i;
            break;
        }
    }
    return retval;
}

const XMLElement& XMLElement::Child(const string& child) const
{
    unsigned int i = 0;
    for (; i < m_children.size(); ++i) {
        if (m_children[i].m_tag == child)
            break;
    }
    return m_children[i];
}

const string& XMLElement::Attribute(const string& attrib) const
{
    static const string empty_str("");
    map<string, string>::const_iterator it = m_attributes.find(attrib);
    if (it != m_attributes.end())
        return it->second;
    else
        return empty_str;
}

ostream& XMLElement::WriteElement(ostream& os, int indent/* = 0*/, bool whitespace/* = true*/) const
{
    if (whitespace)
        for (int i = 0; i < indent; ++i)
            os << INDENT_STR;
    os << '<' << m_tag;
    for (std::map<string, string>::const_iterator it = m_attributes.begin(); it != m_attributes.end(); ++it)
        os << ' ' << it->first << "=\"" << it->second << "\"";
    if (m_children.empty() && m_text.empty() && !m_root) {
        os << "/>";
        if (whitespace)
            os << "\n";
    } else {
        os << ">";
        if (!m_text.empty()) {
            os << "<![CDATA[" << m_text << "]]>";
        }
        if (whitespace && !m_children.empty())
            os << "\n";
        for (unsigned int i = 0; i < m_children.size(); ++i)
            m_children[i].WriteElement(os, indent + 1, whitespace);
        if (whitespace && !m_children.empty()) {
            for (int i = 0; i < indent; ++i) {
                os << INDENT_STR;
            }
        }
        os << "</" << m_tag << ">";
        if (whitespace) os << "\n";
    }
    return os;
}

XMLElement& XMLElement::Child(const string& child)
{
    unsigned int i = 0;
    for (; i < m_children.size(); ++i) {
        if (m_children[i].m_tag == child)
            break;
    }
    return m_children[i];
}


////////////////////////////////////////////////
// GG::XMLDoc
////////////////////////////////////////////////
ostream& XMLDoc::WriteDoc(ostream& os, bool whitespace/* = true*/) const
{
    os << "<?xml version=\"1.0\"?>";
    if (whitespace) os << "\n";
    return root_node.WriteElement(os, 0, whitespace);
}

istream& XMLDoc::ReadDoc(istream& is)
{
    root_node = XMLElement(); // clear doc contents
    s_element_stack.clear();  // clear this to start a fresh read
    s_curr_parsing_doc = this;  // indicate where to add elements
    string parse_str;
    string temp_str;
    while (is) {
        getline(is, temp_str);
        parse_str += temp_str + '\n';
    }
    parse(parse_str.c_str(), document);
    s_curr_parsing_doc = 0;
    return is;
}

void XMLDoc::PushElem(const char* first, const char* last)
{
    static string last_pushed_elem;
    string name(first, last);
    if (XMLDoc* this_ = XMLDoc::s_curr_parsing_doc) {
        if (s_element_stack.empty()) {
            this_->root_node = XMLElement(name, true);
            s_element_stack.push_back(&this_->root_node);
            last_pushed_elem = name;
        } else {
            // this is a dirty hack that avoids one-time duplicate element pushes
            if (name == last_pushed_elem) {
                last_pushed_elem = "";
            } else {
                s_element_stack.back()->AppendChild(name);
                s_element_stack.push_back(&s_element_stack.back()->LastChild());
                last_pushed_elem = name;
            }
        }
    }
}

void XMLDoc::PopElem(const char*, const char*)
{
    if (!s_element_stack.empty())
        s_element_stack.pop_back();
}

void XMLDoc::SetAttributeName(const char* first, const char* last)
{
    attribute_name = string(first, last);
}

void XMLDoc::AddAttribute(const char* first, const char* last)
{
    if (!s_element_stack.empty())
        s_element_stack.back()->SetAttribute(attribute_name, string(first, last));
}

void XMLDoc::AppendToText(const char* first, const char* last)
{
    if (!s_element_stack.empty()) {
        string text(first, last);
        unsigned int first_good_posn = (text[0] != '\"') ? 0 : 1;
        unsigned int last_good_posn = text.find_last_not_of(" \t\n\"\r\f");
        // strip of leading quote and/or trailing quote, and/or trailing whitespace
        if (last_good_posn != string::npos)
            s_element_stack.back()->m_text += text.substr(first_good_posn, (last_good_posn + 1) - first_good_posn);
    }
}

XMLDoc::RuleDefiner::RuleDefiner()
{
    static string temp_elem_name;

    // This is the start rule for XML
    document =
        prolog >> element >> *Misc
        ;
        
    S =
        +(Sch)
        ;
        
    Name =
        (Letter | '_' | ':')
            >> *(NameChar)
        ;
        
    AttValue =
        '"'
            >> (
                (*(anychar_p - (chset_t('<') | '&' | '"')))[&XMLDoc::AddAttribute]
                | *(Reference)
               )
            >> '"'
        |   '\''
            >> (
                (*(anychar_p - (chset_t('<') | '&' | '\'')))[&XMLDoc::AddAttribute]
                | *(Reference)
               )
            >> '\''
        ;

    chset_t CharDataChar(anychar_p - (chset_t('<') | chset_t('&')));

    CharData =
        (*(CharDataChar - str_p("]]>")))[&XMLDoc::AppendToText]
        ;

    Comment1 =
        *(
          (Char - ch_p('-'))
          | (ch_p('-') >> (Char - ch_p('-')))
          )
        ;

    Comment =
        str_p("<!--") >> Comment1 >> str_p("-->")
        ;

    CDSect =
        str_p("<![CDATA[") >> CData >> str_p("]]>")
        ;

    CData =
        (*(Char - str_p("]]>")))[&XMLDoc::AppendToText]
        ;

    prolog =
        !XMLDecl >> *Misc >> !(doctypedecl >> *Misc)
        ;

    XMLDecl =
        str_p("<?xml")
            >> VersionInfo
            >> !EncodingDecl
            >> !SDDecl
            >> !S
            >> str_p("?>")
        ;

    VersionInfo =
        S
            >> str_p("version")
            >> Eq
            >> (
                ch_p('\'') >> VersionNum >> '\''
                | ch_p('"')  >> VersionNum >> '"'
                )
        ;

    Eq =
        !S >> '=' >> !S
        ;

    chset_t VersionNumCh("A-Za-z0-9_.:-");

    VersionNum =
        +(VersionNumCh)
        ;

    Misc =
        Comment | S
        ;

    doctypedecl =
        str_p("<!DOCTYPE")
            >> *(Char - (chset_t('[') | '>'))
            >> !('[' >> *(Char - ']') >> ']')
            >> '>'
        ;

    SDDecl =
        S
            >> str_p("standalone")
            >> Eq
            >> (
                (ch_p('\'') >> (str_p("yes") | str_p("no")) >> '\'')
                | (ch_p('"')  >> (str_p("yes") | str_p("no")) >> '"')
                )
        ;

    element =
        EmptyElemTag
        |   STag >> content >> ETag
        ;

    STag =
        '<'
            >> Name[&XMLDoc::PushElem]
            >> *(S >> Attribute)
            >> !S
            >> '>'
        ;

    Attribute =
        Name[&XMLDoc::SetAttributeName] >> Eq >> AttValue
        ;

    ETag =
        str_p("</") >> Name[&XMLDoc::PopElem] >> !S >> '>'
        ;

    content =
        !CharData
            >> *(
                 (
                  element
                  | Reference
                  | CDSect
                  | Comment
                  )
                 >> !CharData
                 )
        ;

    EmptyElemTag =
        '<'
            >> Name[&XMLDoc::PushElem]
            >> *(S >> Attribute)
            >> !S
            >> str_p("/>")[&XMLDoc::PopElem]
        ;

    CharRef =
        str_p("&#") >> +digit_p >> ';'
        | str_p("&#x") >> +xdigit_p >> ';'
        ;

    Reference =
        EntityRef
        | CharRef
        ;

    EntityRef =
        '&' >> Name >> ';'
        ;

    EncodingDecl =
        S
            >> str_p("encoding")
            >> Eq
            >> (
                ch_p('"')  >> EncName >> '"'
                | ch_p('\'') >> EncName >> '\''
                )
        ;

    chset_t EncNameCh = VersionNumCh - chset_t(':');

    EncName =
        alpha_p >> *(EncNameCh)
        ;
}

} // namespace GG

