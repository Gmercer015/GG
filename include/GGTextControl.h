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

#ifndef _GGTextControl_h_
#define _GGTextControl_h_

#ifndef _GGControl_h_
#include "GGControl.h"
#endif

#ifndef _GGText_h_
#include "GGText.h"
#endif

namespace GG {

/** This is an abstract interface class for StaticText and DynamicText.
    This class is provides a uniform interface to StaticText and DynamicText, so all text can be treated the same, without
    consideration being given to implementation. All TextControl objects know how to center, left- or right-justify, etc. themselves within 
    their window areas.  The format flags used with TextControl are defined in enum GG::TextFormat in GGBase.h. TextControl has std::string-like operators 
    and functions that allow the m_text member string to be manipulated directly.  In addition, the << and >> operators allow virtually 
    any type (int, float, char, etc.) to be read from a Text object as if it were an input or output stream, thanks to boost::lexical_cast 
    and std::stringstream.  Note that the Text stream operators only read the first instance of the specified type from m_text, and overwrite
    the entire m_text string when writing to it; both operators may throw.*/
class TextControl : public Control
{
public:
    /** \name Accessors */ //@{
    Uint32         TextFormat() const               {return m_format;}                     ///< returns the text format (vertical and horizontal justification, use of word breaks and line wrapping, etc.)
    Clr            TextColor() const                {return m_text_color;}                 ///< returns the text color (may differ from the Control::Color() in some subclasses)
   
    /** sets the value of \a t to the interpreted value of the control's text.
        If the control's text can be interpreted as an object of type T by boost::lexical_cast (and thus by a stringstream), 
        then the >> operator will do so.  Note that the return type is void, so multiple >> operations cannot be strung 
        together.  Also, because lexical_cast attempts to convert the entire contents of the string to a single value, a 
        TextControl containing the string "4.15 3.8" will fill a float with 0.0 (the default construction of float), 
        even though there is a perfectly valid 4.15 value that occurs first in the string.  \note boost::lexical_cast 
        usually throws boost::bad_lexical_cast when it cannot perform a requested cast, though >> will return a 
        default-constructed T if one cannot be deduced from the control's text. */
    template <class T> void operator>>(T& t) const
    {
        try {t = boost::lexical_cast<T>(Control::m_text);}
        catch (boost::bad_lexical_cast) {t = T();}
    }
   
    /** sets the value of \a t to the interpreted value of the control's text.
        If the control's text can be interpreted as an object of type T by boost::lexical_cast (and thus by a stringstream), 
        then GetValue() will do so.  Because lexical_cast attempts to convert the entire contents of the string to a 
        single value, a TextControl containing the string "4.15 3.8" will throw, even though there is a perfectly 
        valid 4.15 value that occurs first in the string.  \throw boost::bad_lexical_cast boost::lexical_cast throws 
        boost::bad_lexical_cast when it cannot perform a requested cast. This is handy for validating data in a dialog box;
        Otherwise, using operator>>(), you may get the default value, even though the text in the control may not be the 
        default value at all, but garbage. */
    template <class T> void GetValue(T& t) const {t = boost::lexical_cast<T, string>(Control::m_text);}
   
    operator const string&() const         {return Control::m_text;}  ///< returns the control's text; allows TextControl's to be used as std::string's

    bool  Empty() const                    {return Control::m_text.empty();}   ///< returns true when text string equals ""
    int   Length() const                   {return Control::m_text.length();}  ///< returns length of text string
    //@}
   
    /** \name Mutators */ //@{
    virtual void   SetText(const string& str) = 0;
    virtual void   SetText(const char* str)         {SetText(string(str));}
    void           SetTextFormat(Uint32 format)     {m_format = format; ValidateFormat();} ///< sets the text format; ensures that the flags are sane
    void           SetTextColor(Clr color)          {m_text_color = color;}                ///< sets the text color
    virtual void   SetColor(Clr c)                  {SetColor(c); m_text_color = c;}       ///< just like Color::SetColor(), except that this one also adjusts the text color

    /** Sets the value of the control's text to the stringified version of t.
        If t can be converted to a string representation by a boost::lexical_cast (and thus by a stringstream), then the << operator
        will do so, eg double(4.15) to string("4.15").  Note that the return type is void, so multiple << operations cannot be 
        strung together.  \throw boost::bad_lexical_cast boost::lexical_cast throws boost::bad_lexical_cast when it is confused.*/
    template <class T> void operator<<(T t) {SetText(boost::lexical_cast<string>(t));}
   
    void  operator+=(const string& str)    {SetText(Control::m_text + str);}   ///< appends \a str to text string by way of SetText()
    void  operator+=(const char* str)      {SetText(Control::m_text + str);}   ///< appends \a str to text string by way of SetText()
    void  operator+=(char ch)              {SetText(Control::m_text + ch);}    ///< appends \a ch to text string by way of SetText()
    void  Clear()                          {Control::m_text = "";}             ///< sets text string to ""
    void  Insert(int pos, char ch)         {Control::m_text.insert(pos, 1, ch); SetText(Control::m_text);}   ///< allows access to text string much as a std::string
    void  Erase(int pos, int num = 1)      {Control::m_text.erase(pos, num); SetText(Control::m_text);}      ///< allows access to text string much as a std::string

    virtual XMLElement XMLEncode() const; ///< constructs an XMLElement from a TextControl object
    //@}
   
protected:
    /** \name Structors */ //@{
    TextControl(int x, int y, int w, int h, const string& str, Uint32 text_fmt = 0, Clr color = CLR_BLACK, Uint32 flags = 0); ///< ctor
    TextControl(const XMLElement& elem); ///< ctor that constructs a TextControl object from an XMLElement. \throw std::invalid_argument May throw std::invalid_argument if \a elem does not encode a TextControl object
    //@}

    Uint32   m_format;      ///< the formatting used to display the text (vertical and horizontal alignment, etc.)
    Clr      m_text_color;  ///< the color of the text itself (may differ from GG::Control::m_color)

private:
    void ValidateFormat();  ///< ensures that the format flags are consistent
};


/** This is a simple, fast-rendered static text control.
    This class is inherited from TextImage; the text is rendered using the SDL_ttf library once, and the resulting texture is rendered
    to the screen very quickly. Any change to the m_text member is expensive: it causes the old OpenGL texture to be released and causes 
    the text to be rendered again into a new SDL Surface (which is then freed), and finally to create a new OpenGL texture.  See TextImage 
    for details.  Note that due to the way TextImage and Texture handle empty strings and empty SDL_Surfaces respectively, no 
    initialization performed, and no space or OpenGL texture is used when the string rendered is "".*/
class StaticText : public TextControl, protected TextImage
{
public:
    /** \name Structors */ //@{
    StaticText(int x, int y, int w, int h, const string& str, const string& font_filename, int pts, Uint32 text_fmt = 0, Clr color = CLR_BLACK, Uint32 flags = 0); ///< ctor
   
    /** ctor that does not require window size.  
        Window size is determined from the string and font; the window will be large enough to fit the text as rendered, and no
        larger.  The private member m_fit_to_text is also set to true. \see StaticText::SetText()*/
    StaticText(int x, int y, const string& str, const string& font_filename, int pts, Clr color = CLR_BLACK, Uint32 flags = 0);
   
    StaticText(const XMLElement& elem); ///< ctor that constructs a StaticText object from an XMLElement. \throw std::invalid_argument May throw std::invalid_argument if \a elem does not encode a StaticText object
    //@}

    /** \name Accessors */ //@{
    int      TextWidth() const  {return TextImage::DefaultWidth();}  ///< returns the width of the underlying text image
    int      TextHeight() const {return TextImage::DefaultHeight();} ///< returns the height of the underlying text image
    //@}

    /** \name Mutators */ //@{
    virtual int    Render();
   
    /** sets the text to \a str, and re-renders the underlying text image; may resizze the window.  
        If the private member m_fit_to_text is true (i.e. the second ctor was used), calls to this function cause the window to 
        be resized to whatever space the newly rendered text occupies.*/
    virtual void   SetText(const string& str); 

    virtual XMLElement XMLEncode() const; ///< constructs an XMLElement from a StaticText object
    //@}

private:
    bool m_fit_to_text;  ///< when true, this window will maintain a minimum width and height that encloses the text
};


/** This is a text control based on pre-rendered font glyphs.
    This class is inherited from TextImage; the text is rendered character by character from a prerendered font. The font used is 
    gotten from the application's font manager.  Since a shared_ptr to the font is kept, the font is guaranteed to live at least as long
    as the DynamicText object that refers to it.  This also means that if the font is explicitly released from the font manager but is
    still held by at least one DynamicText object, it will not be destroyed, due to the shared_ptr.  Note that if "" is supplied as the 
    font_filename parameter, no text will be rendered, but a valid DynamicText object will be constructed, which may later contain
    renderable text. DynamicText objects support text with formatting tags. See GG::Font for details.*/
class DynamicText : public TextControl
{
public:
    /** \name Structors */ //@{
    DynamicText(int x, int y, int w, int h, const string& str, const shared_ptr<Font>& font, Uint32 text_fmt = 0, Clr color = CLR_BLACK, Uint32 flags = 0); ///< ctor taking a font directly
    DynamicText(int x, int y, int w, int h, const string& str, const string& font_filename, int pts, Uint32 text_fmt = 0, Clr color = CLR_BLACK, Uint32 flags = 0); ///< ctor taking a font filename and font point size

    /** ctor that does not require window size.
        Window size is determined from the string and font; the window will be large enough to fit the text as rendered, 
        and no larger.  The private member m_fit_to_text is also set to true. \see DynamicText::SetText() */
    DynamicText(int x, int y, const string& str, const shared_ptr<Font>& font, Clr color = CLR_BLACK, Uint32 flags = 0);
   
    /** ctor that does not require window size.
        Window size is determined from the string and font; the window will be large enough to fit the text as rendered, 
        and no larger.  The private member m_fit_to_text is also set to true. \see DynamicText::SetText() */
    DynamicText(int x, int y, const string& str, const string& font_filename, int pts, Clr color = CLR_BLACK, Uint32 flags = 0);
   
    DynamicText(const XMLElement& elem); ///< ctor that constructs a DynamicText object from an XMLElement. \throw std::invalid_argument May throw std::invalid_argument if \a elem does not encode a DynamicText object
    //@}

    /** \name Mutators */ //@{
    virtual int    Render();

    /** sets the text to \a str; may resize the window.  If the private member m_fit_to_text is true (i.e. if the second 
        ctor type was used), calls to this function cause the window to be resized to whatever space the newly rendered 
        text occupies. */
    virtual void   SetText(const string& str);

    virtual XMLElement XMLEncode() const; ///< constructs an XMLElement from a DynamicText object
    //@}

protected:
    /** \name Accessors */ //@{
    const vector<Font::LineData>& GetLineData() const {return m_line_data;}
    const shared_ptr<Font>&       GetFont() const  {return m_font;}
    //@}

private:
    vector<Font::LineData>  m_line_data;
    shared_ptr<Font>        m_font;
    bool                    m_fit_to_text;  ///< when true, this window will maintain a minimum width and height that encloses the text
};

} // namespace GG

#endif // _GGTextControl_h_

