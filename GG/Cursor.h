// -*- C++ -*-
/* GG is a GUI for SDL and OpenGL.
   Copyright (C) 2007 T. Zachary Laine

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
   
/** \file Cursor.h
    Contains Cursor class, which encapsulates the rendering of the input cursor. */

#ifndef _GG_Cursor_h_
#define _GG_Cursor_h_

#include <GG/PtRect.h>

#include <boost/shared_ptr.hpp>


namespace GG {

class Texture;

/** Cursor is the base class for GUI-renderable cursors.  A Cursor can be set in the GUI and will be rendered if GUI's
    RenderCursor() member returns true.  Note that it may be necessary to disable the underlying platform's cursor . */
class GG_API Cursor
{
public:
    /** \name Structors */ //@{
    Cursor();          ///< ctor
    virtual ~Cursor(); ///< virtual dtor
    //@}

    /** \name Mutators */ //@{
    /** Renders the cursor at the specified location.  Subclasses should take care to ensure that the cursor's "hotspot"
        is rendered at \a pt. */
    virtual void Render(const Pt& pt) = 0;
    //@}

private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

/** TextureCursor is a very simple subclass of Cursor.  It renders a texture such that the point within the texture that
    represents the hotspot of the cursor is rendered at the click-point of the cursor. */
class GG_API TextureCursor :
    public Cursor
{
public:
    /** \name Structors */ //@{
    /** Ctor.  \a texture is the texture to render and \a hotspot is the offset within \a texture where the click-point
        is located.  \a hotspot is clamped to \a texture's valid area. */
    TextureCursor(const boost::shared_ptr<Texture>& texture, const Pt& hotspot = Pt());
    //@}

    /** \name Mutators */ //@{
    virtual void Render(const Pt& pt);
    //@}

private:
    boost::shared_ptr<Texture> m_texture;
    Pt                         m_hotspot;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
};

} // namespace GG

// template implementations
template <class Archive>
void GG::Cursor::serialize(Archive& ar, const unsigned int version)
{}

template <class Archive>
void GG::TextureCursor::serialize(Archive& ar, const unsigned int version)
{
    ar  & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Cursor)
        & m_texture
        & m_hotspot;
}

#endif // _GG_Cursor_h_
