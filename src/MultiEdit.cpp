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

#include "GGMultiEdit.h"
 
#include <GGApp.h>
#include <GGDrawUtil.h>
#include <GGScroll.h>
#include <GGStyleFactory.h>
#include <GGWndEditor.h>

using namespace GG;

namespace {
    bool LineEndsWithEndlineCharacter(const std::vector<Font::LineData>& lines, int line, const std::string& original_string)
    {
        assert(0 <= line && line < static_cast<int>(lines.size()));
        if (lines[line].Empty())
            return false;
        else
            return original_string[lines[line].char_data.back().original_char_index] == '\n';
    }

    struct SetStyleAction : AttributeChangedAction<Uint32>
    {
        SetStyleAction(MultiEdit* multi_edit) : m_multi_edit(multi_edit) {}
        void operator()(const Uint32& style) {m_multi_edit->SetText(m_multi_edit->WindowText());}
    private:
        MultiEdit* m_multi_edit;
    };
}

////////////////////////////////////////////////
// GG::MultiEdit
////////////////////////////////////////////////
// static(s)
const int MultiEdit::SCROLL_WIDTH = 14;

MultiEdit::MultiEdit() :
    Edit(),
    m_first_col_shown(0),
    m_first_row_shown(0),
    m_max_lines_history(0),
    m_vscroll(0),
    m_hscroll(0)
{}

MultiEdit::MultiEdit(int x, int y, int w, int h, const std::string& str, const boost::shared_ptr<Font>& font, Clr color, 
              Uint32 style/* = TF_LINEWRAP*/, Clr text_color/* = CLR_BLACK*/, Clr interior/* = CLR_ZERO*/, 
              Uint32 flags/* = CLICKABLE | DRAG_KEEPER*/) : 
    Edit(x, y, w, str, font, color, text_color, interior, flags),
    m_style(style),
    m_cursor_begin(0, 0),
    m_cursor_end(0, 0),
    m_first_col_shown(0),
    m_first_row_shown(0),
    m_max_lines_history(0),
    m_vscroll(0),
    m_hscroll(0)
{
    SetColor(color);
    Resize(Pt(w, h));
    Init();
}

MultiEdit::~MultiEdit()
{
    delete m_vscroll;
    delete m_hscroll;
}

Pt MultiEdit::ClientLowerRight() const
{
    return Edit::ClientLowerRight() - Pt(RightMargin(), BottomMargin());
}

int MultiEdit::MaxLinesOfHistory() const
{
    return m_max_lines_history;
}

void MultiEdit::Render()
{
    if (DirtyLoad())
        SetText(WindowText());
    Clr color_to_use = Disabled() ? DisabledColor(Color()) : Color();
    Clr int_color_to_use = Disabled() ? DisabledColor(InteriorColor()) : InteriorColor();
    Clr sel_text_color_to_use = Disabled() ? DisabledColor(SelectedTextColor()) : SelectedTextColor();
    Clr hilite_color_to_use = Disabled() ? DisabledColor(HiliteColor()) : HiliteColor();
    Clr text_color_to_use = Disabled() ? DisabledColor(TextColor()) : TextColor();

    Pt ul = UpperLeft(), lr = LowerRight();
    Pt cl_ul = ClientUpperLeft();
    Pt cl_lr = ClientLowerRight();

    BeveledRectangle(ul.x, ul.y, lr.x, lr.y, int_color_to_use, color_to_use, false, 2);

    // clip text to client area
    BeginScissorClipping(cl_ul.x - 1, cl_ul.y, cl_lr.x, cl_lr.y);

    Font::RenderState state;
    int first_visible_row = FirstVisibleRow();
    int last_visible_row = LastVisibleRow();
    Uint32 text_format = TextFormat() & ~(TF_TOP | TF_BOTTOM) | TF_VCENTER;
    const std::vector<Font::LineData>& lines = GetLineData();
    for (int row = first_visible_row; row <= last_visible_row && row < static_cast<int>(lines.size()); ++row) {
        int row_y_pos = ((m_style & TF_TOP) || m_contents_sz.y - ClientSize().y < 0) ? 
            cl_ul.y + row * GetFont()->Lineskip() - m_first_row_shown : 
            cl_lr.y - (static_cast<int>(lines.size()) - row) * GetFont()->Lineskip() - m_first_row_shown + 
            (m_vscroll && m_hscroll ? BottomMargin() : 0);
        Pt text_pos(cl_ul.x + RowStartX(row), row_y_pos);
        int initial_text_x_pos = text_pos.x;

        if (!lines[row].Empty())
        {
            // if one or more chars of this row are selected, highlight, then draw the range in the selected-text color
            std::pair<int, int> low_cursor_pos  = LowCursorPos();
            std::pair<int, int> high_cursor_pos = HighCursorPos();
            if (low_cursor_pos.first <= row && row <= high_cursor_pos.first && MultiSelected()) {
                // idx0 to idx1 is unhilited, idx1 to idx2 is hilited, and idx2 to idx3 is unhilited; each range may be empty
                int idx0 = 0;
                int idx1 = low_cursor_pos.first == row ? std::max(idx0, low_cursor_pos.second) : idx0;
                int idx3 = lines[row].char_data.size();
                if (LineEndsWithEndlineCharacter(lines, row, WindowText()))
                    --idx3;
                int idx2 = high_cursor_pos.first == row ? std::min(high_cursor_pos.second, idx3) : idx3;

                // draw text
                glColor4ubv(text_color_to_use.v);
                Pt text_lr((idx0 != idx1 ? initial_text_x_pos + lines[row].char_data[idx1 - 1].extent : text_pos.x), text_pos.y + GetFont()->Height());
                GetFont()->RenderText(text_pos, text_lr, WindowText(), text_format, lines, state, row, idx0, row + 1, idx1);
                text_pos.x = text_lr.x;

                // draw hiliting
                text_lr.x = idx1 != idx2 ? initial_text_x_pos + lines[row].char_data[idx2 - 1].extent : text_lr.x;
                FlatRectangle(text_pos.x, text_pos.y, text_lr.x, text_pos.y + GetFont()->Lineskip(), hilite_color_to_use, CLR_ZERO, 0);
                // draw hilited text
                glColor4ubv(sel_text_color_to_use.v);
                GetFont()->RenderText(text_pos, text_lr, WindowText(), text_format, lines, state, row, idx1, row + 1, idx2);
                text_pos.x = text_lr.x;

                glColor4ubv(text_color_to_use.v);
                text_lr.x = idx2 != idx3 ? initial_text_x_pos + lines[row].char_data[idx3 - 1].extent : text_lr.x;
                GetFont()->RenderText(text_pos, text_lr, WindowText(), text_format, lines, state, row, idx2, row + 1, idx3);
            } else { // just draw normal text on this line
                Pt lr = text_pos + Pt(lines[row].char_data.back().extent, GetFont()->Height());
                glColor4ubv(text_color_to_use.v);
                GetFont()->RenderText(text_pos, text_pos + Pt(lines[row].char_data.back().extent, GetFont()->Height()), WindowText(), text_format, lines, state, row, 0, row + 1, lines[row].char_data.size());
            }
        }
        // if there's no selected text, but this row contains the caret (and READ_ONLY is not in effect)
        if (!MultiSelected() && m_cursor_begin.first == row && !(m_style & READ_ONLY)) {
            int caret_x = CharXOffset(m_cursor_begin.first, m_cursor_begin.second) + initial_text_x_pos;
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_LINES);
            glVertex2i(caret_x, row_y_pos);
            glVertex2i(caret_x, row_y_pos + GetFont()->Lineskip());
            glEnd();
            glEnable(GL_TEXTURE_2D);
        }
    }

    EndScissorClipping();
}

void MultiEdit::LButtonDown(const Pt& pt, Uint32 keys)
{
    // when a button press occurs, record the character position under the cursor, and remove any previous selection range
    if (!Disabled() && !(m_style & READ_ONLY)) {
        m_cursor_end = CharAt(ScreenToClient(pt));
        if (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_end.first, WindowText()))
            --m_cursor_end.second;
        m_cursor_begin = m_cursor_end;
        AdjustView();
    }
}

void MultiEdit::LDrag(const Pt& pt, const Pt& move, Uint32 keys)
{
    if (!Disabled() && !(m_style & READ_ONLY)) {
        // when a drag occurs, move m_cursor_end to where the mouse is, which selects a range of characters
        Pt click_pos = ScreenToClient(pt); // coord of click within text space
        m_cursor_end = CharAt(click_pos);
        // if we're dragging past the currently visible text, adjust the view so more text can be selected
        if (click_pos.x < 0 || click_pos.x > ClientSize().x || 
            click_pos.y < 0 || click_pos.y > ClientSize().y) 
            AdjustView();
    }
}

void MultiEdit::MouseWheel(const Pt& pt, int move, Uint32 keys)
{
    if (!Disabled() && m_vscroll) {
        for (int i = 0; i < move; ++i)
            m_vscroll->ScrollLineDecr();
        for (int i = 0; i < -move; ++i)
            m_vscroll->ScrollLineIncr();
    }
}

void MultiEdit::Keypress(Key key, Uint32 key_mods)
{
    if (!Disabled()) {
        if (!(m_style & READ_ONLY)) {
            bool shift_down = key_mods & (GGKMOD_LSHIFT | GGKMOD_RSHIFT);
            bool emit_signal = false;
            switch (key) {
            case GGK_RETURN:
            case GGK_KP_ENTER: {
                if (MultiSelected())
                    ClearSelected();
                Insert(StringIndexOf(m_cursor_end.first, m_cursor_end.second), '\n');
                ++m_cursor_end.first;
                m_cursor_end.second = 0;
                // the cursor might be off the bottom if the bottom row was just chopped off to satisfy m_max_lines_history
                if (static_cast<int>(GetLineData().size()) <= m_cursor_end.first) {
                    m_cursor_end.first = static_cast<int>(GetLineData().size()) - 1;
                    m_cursor_end.second = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size());
                    if (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_end.first, WindowText()))
                        --m_cursor_end.second;
                }
                m_cursor_begin = m_cursor_end;
                emit_signal = true;
                break;
            }

            case GGK_LEFT: {
                if (MultiSelected() && !shift_down) {
                    m_cursor_begin = m_cursor_end = LowCursorPos();
                } else if (0 < m_cursor_end.second) {
                    --m_cursor_end.second;
                } else if (0 < m_cursor_end.first) {
                    --m_cursor_end.first;
                    m_cursor_end.second = GetLineData()[m_cursor_end.first].char_data.size();
                    if (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_end.first, WindowText()))
                        --m_cursor_end.second;
                }
                if (!shift_down)
                    m_cursor_begin = m_cursor_end;
                break;
            }

            case GGK_RIGHT: {
                if (MultiSelected() && !shift_down) {
                    m_cursor_begin = m_cursor_end = HighCursorPos();
                } else if (m_cursor_end.second <
                           static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size()) -
                           (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_end.first, WindowText()) ? 1 : 0)) {
                    ++m_cursor_end.second;
                } else if (m_cursor_end.first < static_cast<int>(GetLineData().size()) - 1) {
                    ++m_cursor_end.first;
                    m_cursor_end.second = 0;
                }
                if (!shift_down)
                    m_cursor_begin = m_cursor_end;
                break;
            }

            case GGK_UP: {
                if (MultiSelected() && !shift_down) {
                    m_cursor_begin = m_cursor_end = LowCursorPos();
                } else if (0 < m_cursor_end.first) {
                    int row_start = RowStartX(m_cursor_end.first);
                    int char_offset = CharXOffset(m_cursor_end.first, m_cursor_end.second);
                    --m_cursor_end.first;
                    m_cursor_end.second = CharAt(m_cursor_end.first, row_start + char_offset);
                    if (!shift_down)
                        m_cursor_begin = m_cursor_end;
                }
                break;
            }

            case GGK_DOWN: {
                if (MultiSelected() && !shift_down) {
                    m_cursor_begin = m_cursor_end = HighCursorPos();
                } else if (m_cursor_end.first < static_cast<int>(GetLineData().size()) - 1) {
                    int row_start = RowStartX(m_cursor_end.first);
                    int char_offset = CharXOffset(m_cursor_end.first, m_cursor_end.second);
                    ++m_cursor_end.first;
                    m_cursor_end.second = CharAt(m_cursor_end.first, row_start + char_offset);
                    if (!shift_down)
                        m_cursor_begin = m_cursor_end;
                }
                break;
            }

            case GGK_HOME: {
                m_cursor_end.second = 0;
                if (!shift_down)
                    m_cursor_begin = m_cursor_end;
                break;
            }

            case GGK_END: {
                m_cursor_end.second = GetLineData()[m_cursor_end.first].char_data.size();
                if (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_end.first, WindowText()))
                    --m_cursor_end.second;
                if (!shift_down)
                    m_cursor_begin = m_cursor_end;
                break;
            }

            case GGK_PAGEUP: {
                if (m_vscroll) {
                    m_vscroll->ScrollPageDecr();
                    int rows_moved = m_vscroll->PageSize() / GetFont()->Lineskip();
                    m_cursor_end.first = std::max(0, m_cursor_end.first - rows_moved);
                    if (static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size()) < m_cursor_end.second)
                        m_cursor_end.second = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size());
                    m_cursor_begin = m_cursor_end;
                }
                break;
            }

            case GGK_PAGEDOWN: {
                if (m_vscroll) {
                    m_vscroll->ScrollPageIncr();
                    int rows_moved = m_vscroll->PageSize() / GetFont()->Lineskip();
                    m_cursor_end.first = std::min(m_cursor_end.first + rows_moved, static_cast<int>(GetLineData().size()) - 1);
                    if (static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size()) < m_cursor_end.second)
                        m_cursor_end.second = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size());
                    m_cursor_begin = m_cursor_end;
                }
                break;
            }

            case GGK_BACKSPACE: {
                if (MultiSelected()) {
                    ClearSelected();
                    emit_signal = true;
                } else if (0 < m_cursor_begin.second) {
                    m_cursor_end.second = --m_cursor_begin.second;
                    Erase(StringIndexOf(m_cursor_begin.first, m_cursor_begin.second));
                    emit_signal = true;
                } else if (0 < m_cursor_begin.first) {
                    m_cursor_end.first = --m_cursor_begin.first;
                    m_cursor_begin.second = GetLineData()[m_cursor_begin.first].char_data.size();
                    if (LineEndsWithEndlineCharacter(GetLineData(), m_cursor_begin.first, WindowText()))
                        --m_cursor_begin.second;
                    m_cursor_end.second = m_cursor_begin.second;
                    Erase(StringIndexOf(m_cursor_begin.first, m_cursor_begin.second));
                    emit_signal = true;
                }
                break;
            }

            case GGK_DELETE: {
                if (MultiSelected()) {
                    ClearSelected();
                    emit_signal = true;
                } else if (m_cursor_begin.second < static_cast<int>(GetLineData()[m_cursor_begin.first].char_data.size())) {
                    Erase(StringIndexOf(m_cursor_begin.first, m_cursor_begin.second));
                    emit_signal = true;
                } else if (m_cursor_begin.first < static_cast<int>(GetLineData().size()) - 1) {
                    int begin = StringIndexOf(m_cursor_begin.first, m_cursor_begin.second);
                    int length = StringIndexOf(m_cursor_begin.first + 1, 0) - begin;
                    Erase(begin, length);
                    emit_signal = true;
                }
                break;
            }

            default: {
                // only process it if it's a printable character, and no significant modifiers are in use
                KeypadKeyToPrintable(key, key_mods);
                if (key < GGK_DELETE && isprint(key) && !(key_mods & (GGKMOD_CTRL | GGKMOD_ALT | GGKMOD_META | GGKMOD_MODE))) {
                    if (MultiSelected())
                        ClearSelected();
                    // insert the character to the right of the caret
                    Insert(StringIndexOf(m_cursor_begin.first, m_cursor_begin.second), key);
                    // then move the caret fwd one.
                    if (m_cursor_begin.second < static_cast<int>(GetLineData()[m_cursor_begin.first].char_data.size())) {
                        ++m_cursor_begin.second;
                    } else {
                        ++m_cursor_begin.first;
                        m_cursor_begin.second = 1;
                    }
                    // the cursor might be off the bottom if the bottom row was just chopped off to satisfy m_max_lines_history
                    if (static_cast<int>(GetLineData().size()) - 1 < m_cursor_begin.first) {
                        m_cursor_begin.first = static_cast<int>(GetLineData().size()) - 1;
                        m_cursor_begin.second = static_cast<int>(GetLineData()[m_cursor_begin.first].char_data.size());
                    }
                    m_cursor_end = m_cursor_begin;
                    emit_signal = true;
                } else if (Parent()) {
                    Parent()->Keypress(key, key_mods);
                }
                break;
            }
            }
            AdjustView();
            if (emit_signal)
                EditedSignal(WindowText());
        }
    } else {
        if (Parent())
            Parent()->Keypress(key, key_mods);
    }
}

void MultiEdit::SizeMove(const Pt& ul, const Pt& lr)
{
    Pt lower_right = lr;
    if (m_style & INTEGRAL_HEIGHT)
        lower_right.y -= ((lr.y - ul.y) - (2 * PIXEL_MARGIN)) % GetFont()->Lineskip();
    Edit::SizeMove(ul, lower_right);
    AdjustScrolls();
    AdjustView();
}

void MultiEdit::SelectAll()
{
    m_cursor_begin = std::pair<int, int>(0, 0);
    m_cursor_end = std::pair<int, int>(GetLineData().size() - 1, GetLineData()[GetLineData().size() - 1].char_data.size());
}

void MultiEdit::SetText(const std::string& str)
{
     bool scroll_to_end = (m_style & TERMINAL_STYLE) &&
          (!m_vscroll || m_vscroll->ScrollRange().second - m_vscroll->PosnRange().second <= 1);

    // trim the rows, if required by m_max_lines_history
    Pt cl_sz = ClientSize();
    Uint32 format = TextFormat();
    if (0 < m_max_lines_history) {
        std::vector<Font::LineData> lines;
        GetFont()->DetermineLines(str, format, cl_sz.x, lines);
        if (m_max_lines_history < static_cast<int>(lines.size())) {
            int first_line = 0;
            int last_line = m_max_lines_history - 1;
            int cursor_begin_idx = -1; // used to correct the cursor range when lines get chopped
            int cursor_end_idx = -1;
            int first_line_char_idx = StringIndexOf(first_line, 0);
            int last_line_char_idx = StringIndexOf(last_line, 0);
            if (m_style & TERMINAL_STYLE) {
                first_line = (lines.size() - 1) - (m_max_lines_history - 1);
                last_line = lines.size() - 1;
                first_line_char_idx = StringIndexOf(first_line, 0);
                last_line_char_idx = StringIndexOf(last_line, 0);
                // chopping these lines off the front will invalidate the cursor range unless we do this
                cursor_begin_idx = StringIndexOf(m_cursor_begin.first, m_cursor_begin.second) - first_line_char_idx;
                cursor_end_idx = StringIndexOf(m_cursor_end.first, m_cursor_end.second) - first_line_char_idx;
            }
            TextControl::SetText(str.substr(first_line_char_idx, last_line_char_idx - first_line_char_idx));
            if (cursor_begin_idx != -1 && cursor_end_idx != -1) {
                bool found_cursor_begin = false;
                bool found_cursor_end = false;
                for (unsigned int i = 0; i < GetLineData().size(); ++i) {
                    if (!found_cursor_begin && cursor_begin_idx <= GetLineData()[i].char_data.back().original_char_index) {
                        m_cursor_begin.first = i;
                        m_cursor_begin.second = cursor_begin_idx - GetLineData()[i].char_data.front().original_char_index;
                        found_cursor_begin = true;
                    }
                    if (!found_cursor_end && cursor_end_idx <= GetLineData()[i].char_data.back().original_char_index) {
                        m_cursor_end.first = i;
                        m_cursor_end.second = cursor_end_idx - GetLineData()[i].char_data.front().original_char_index;
                        found_cursor_end = true;
                    }
                }
            }
        } else {
            TextControl::SetText(str);
        }
    } else {
        TextControl::SetText(str);
    }

    // make sure the change in text did not make the cursor position invalid
    if (static_cast<int>(GetLineData().size()) <= m_cursor_end.first) {
        m_cursor_end.first = static_cast<int>(GetLineData().size()) - 1;
        m_cursor_end.second = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size());
    } else if (static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size()) < m_cursor_end.second) {
        m_cursor_end.second = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size());
    }
    m_cursor_begin = m_cursor_end; // eliminate any hiliting

    m_contents_sz = GetFont()->TextExtent(WindowText(), format, (format & (TF_WORDBREAK | TF_LINEWRAP)) ? cl_sz.x : 0);

    AdjustScrolls();
    AdjustView();
    if (scroll_to_end && m_vscroll)
        m_vscroll->ScrollTo(m_vscroll->ScrollRange().second - m_vscroll->PageSize());
    EditedSignal(str);
}

void MultiEdit::SetStyle(Uint32 style)
{
    m_style = style;
    ValidateStyle();
    SetText(WindowText());
}

void MultiEdit::SetMaxLinesOfHistory(int max)
{
    m_max_lines_history = max;
    SetText(m_text);
}

void MultiEdit::DefineAttributes(WndEditor* editor)
{
    if (!editor)
        return;
    Edit::DefineAttributes(editor);
    editor->Label("MultiEdit");
    boost::shared_ptr<SetStyleAction> set_style_action(new SetStyleAction(this));
    editor->BeginFlags(m_style, set_style_action);
    editor->FlagGroup("V. Alignment", TF_TOP, TF_BOTTOM);
    editor->FlagGroup("H. Alignment", TF_CENTER, TF_RIGHT);
    editor->Flag("Word-break", TF_WORDBREAK);
    editor->Flag("Line-wrap", TF_LINEWRAP);
    editor->Flag("Ignore Tags", TF_IGNORETAGS);
    editor->Flag("Read Only", READ_ONLY);
    editor->Flag("Terminal Style", TERMINAL_STYLE);
    editor->Flag("Integral Height", INTEGRAL_HEIGHT);
    editor->Flag("No V. Scrolling", NO_VSCROLL);
    editor->Flag("No H. Scrolling", NO_HSCROLL);
    editor->EndFlags();
    editor->Attribute("Lines of History", m_max_lines_history);
}

bool MultiEdit::MultiSelected() const
{
    return m_cursor_begin != m_cursor_end;
}

int MultiEdit::RightMargin() const
{
    return (m_vscroll ? SCROLL_WIDTH : 0);
}

int MultiEdit::BottomMargin() const
{
    return (m_hscroll ? SCROLL_WIDTH : 0);
}

std::pair<int, int> MultiEdit::CharAt(const Pt& pt) const
{
    std::pair<int, int> retval;
    retval.first = std::max(0, std::min(RowAt(pt.y), static_cast<int>(GetLineData().size() - 1)));
    retval.second = std::max(0, std::min(CharAt(retval.first, pt.x), static_cast<int>(GetLineData()[retval.first].char_data.size())));
    return retval;
}

int MultiEdit::StringIndexOf(int row, int char_idx) const
{
    int retval;
    const std::vector<Font::LineData>& lines = GetLineData();
    if (lines[row].Empty()) {
        if (!row)
            return 0;
        --row;
        char_idx = lines[row].char_data.size();
    }
    if (char_idx == static_cast<int>(lines[row].char_data.size())) {
        retval = lines[row].char_data.back().original_char_index + 1;
    } else {
        retval = lines[row].char_data[char_idx].original_char_index;
        // "rewind" the first position to encompass all tag text that is associated with that position
        for (unsigned int i = 0; i < lines[row].char_data[char_idx].tags.size(); ++i) {
            retval -= lines[row].char_data[char_idx].tags[i]->OriginalStringChars();
        }
    }
    return retval;
}

int MultiEdit::RowStartX(int row) const
{
    int retval = -m_first_col_shown;

    Pt cl_sz = ClientSize();
    int excess_width = m_contents_sz.x - cl_sz.x;
    if (m_style & TF_RIGHT)
        retval -= excess_width;
    else if (m_style & TF_CENTER)
        retval -= excess_width / 2;

    if (!GetLineData()[row].Empty()) {
        int line_width = GetLineData()[row].char_data.back().extent;
        if (GetLineData()[row].justification == TF_LEFT) {
            retval += (m_vscroll && m_hscroll ? RightMargin() : 0);
        } else if (GetLineData()[row].justification == TF_RIGHT) {
            retval += m_contents_sz.x - line_width + (m_vscroll && m_hscroll ? RightMargin() : 0);
        } else if (GetLineData()[row].justification == TF_CENTER) {
            retval += (m_contents_sz.x - line_width + (m_vscroll && m_hscroll ? RightMargin() : 0)) / 2;
        }
    }

    return retval;
}

int MultiEdit::CharXOffset(int row, int idx) const
{
    return (0 < idx ? GetLineData()[row].char_data[idx - 1].extent : 0);
}

int MultiEdit::RowAt(int y) const
{
    int retval = 0;
    Uint32 format = TextFormat();
    y += m_first_row_shown;
    if ((format & TF_TOP) || m_contents_sz.y - ClientSize().y < 0) {
        retval = y / GetFont()->Lineskip();
    } else { // TF_BOTTOM
        retval = (static_cast<int>(GetLineData().size()) - 1) - 
            (ClientSize().y + (m_vscroll && m_hscroll ? BottomMargin() : 0) - y - 1) / GetFont()->Lineskip();
    }
    return retval;
}

int MultiEdit::CharAt(int row, int x) const
{
    int retval = 0;
    x -= RowStartX(row);
    while (retval < static_cast<int>(GetLineData()[row].char_data.size()) && GetLineData()[row].char_data[retval].extent < x)
        ++retval;
    if (retval < static_cast<int>(GetLineData()[row].char_data.size())) {
        int prev_extent = retval ? GetLineData()[row].char_data[retval - 1].extent : 0;
        int half_way = (prev_extent + GetLineData()[row].char_data[retval].extent) / 2;
        if (half_way < x) // if the point is more than halfway across the character, put the cursor *after* the character
            ++retval;
    }
    return retval;
}

int MultiEdit::FirstVisibleRow() const
{
    return std::max(0, std::min(RowAt(0), static_cast<int>(GetLineData().size()) - 1));
}

int MultiEdit::LastVisibleRow() const
{
    return std::max(0, std::min(RowAt(ClientSize().y), static_cast<int>(GetLineData().size()) - 1));
}

int MultiEdit::FirstFullyVisibleRow() const
{
    int retval = RowAt(0);
    if (m_first_row_shown % GetFont()->Lineskip())
        ++retval;
    return std::max(0, std::min(retval, static_cast<int>(GetLineData().size()) - 1));
}

int MultiEdit::LastFullyVisibleRow() const
{
    int retval = RowAt(ClientSize().y);
    if ((m_first_row_shown + ClientSize().y + BottomMargin()) % GetFont()->Lineskip())
        --retval;
    return std::max(0, std::min(retval, static_cast<int>(GetLineData().size()) - 1));
}

int MultiEdit::FirstVisibleChar(int row) const
{
    if (GetLineData()[row].Empty())
        return std::max(0, CharAt(row, 0));
    else
        return std::max(0, std::min(CharAt(row, 0), GetLineData()[row].char_data.back().original_char_index));
}

int MultiEdit::LastVisibleChar(int row) const
{
    if (GetLineData()[row].Empty())
        return std::max(0, CharAt(row, ClientSize().x));
    else
        return std::max(0, std::min(CharAt(row, ClientSize().x), GetLineData()[row].char_data.back().original_char_index));
}

std::pair<int, int> MultiEdit::HighCursorPos() const
{
    if (m_cursor_begin.first < m_cursor_end.first || 
        (m_cursor_begin.first == m_cursor_end.first && m_cursor_begin.second < m_cursor_end.second))
        return m_cursor_end;
    else
        return m_cursor_begin;
}

std::pair<int, int> MultiEdit::LowCursorPos() const
{
    if (m_cursor_begin.first < m_cursor_end.first || 
        (m_cursor_begin.first == m_cursor_end.first && m_cursor_begin.second < m_cursor_end.second))
        return m_cursor_begin;
    else
        return m_cursor_end;
}

void MultiEdit::RecreateScrolls()
{
    delete m_vscroll;
    delete m_hscroll;
    m_vscroll = m_hscroll = 0;
    AdjustScrolls();
}

void MultiEdit::Init()
{
    ValidateStyle();
    SetTextFormat(m_style);
    SetText(WindowText());
    SizeMove(UpperLeft(), LowerRight()); // do this to set up the scrolls, and in case INTEGRAL_HEIGHT is in effect
}

void MultiEdit::ValidateStyle()
{
    if (m_style & TERMINAL_STYLE) {
        m_style &= ~(TF_TOP | TF_VCENTER);
        m_style |= TF_BOTTOM;
    } else {
        m_style &= ~(TF_VCENTER | TF_BOTTOM);
        m_style |= TF_TOP;
    }

    int dup_ct = 0;   // duplication count
    if (m_style & TF_LEFT) ++dup_ct;
    if (m_style & TF_RIGHT) ++dup_ct;
    if (m_style & TF_CENTER) ++dup_ct;
    if (dup_ct != 1) {   // exactly one must be picked; when none or multiples are picked, use TF_LEFT by default
        m_style &= ~(TF_RIGHT | TF_LEFT);
        m_style |= TF_LEFT;
    }

    if (m_style & (TF_LINEWRAP | TF_WORDBREAK)) {
        m_style |= NO_HSCROLL;
    }
}

void MultiEdit::ClearSelected()
{
    int idx_1 = StringIndexOf(m_cursor_begin.first, m_cursor_begin.second);
    int idx_2 = StringIndexOf(m_cursor_end.first, m_cursor_end.second);
    m_cursor_begin = m_cursor_end = LowCursorPos();
    Erase(idx_1 < idx_2 ? idx_1 : idx_2, std::abs(idx_2 - idx_1));
}

void MultiEdit::AdjustView()
{
    Pt cl_sz = ClientSize();
    Uint32 format = TextFormat();
    int excess_width = m_contents_sz.x - cl_sz.x;
    int excess_height = m_contents_sz.y - cl_sz.y;
    int horz_min = 0;            // these are default values for TF_LEFT and TF_TOP
    int horz_max = excess_width;
    int vert_min = 0;
    int vert_max = excess_height;
    
    if (format & TF_RIGHT) {
        horz_min = -excess_width;
        horz_max = horz_min + m_contents_sz.x;
    } else if (format & TF_CENTER) {
        horz_min = -excess_width / 2;
        horz_max = horz_min + m_contents_sz.x;
    }
    if ((format & TF_BOTTOM) && 0 <= excess_height) {
        vert_min = -excess_height;
        vert_max = vert_min + m_contents_sz.y;
    }

    // make sure that m_first_row_shown and m_first_col_shown are within sane bounds
    if (excess_width <= 0 || !m_hscroll)
        m_first_col_shown = 0;
    else
        m_hscroll->ScrollTo(std::max(horz_min, std::min(m_first_col_shown, horz_max)));

    if (excess_height <= 0 || !m_vscroll)
        m_first_row_shown = 0;
    else
        m_vscroll->ScrollTo(std::max(vert_min, std::min(m_first_row_shown, vert_max)));

    // adjust m_first_row_shown position to bring the cursor into view
    int first_fully_vis_row = FirstFullyVisibleRow();
    if (m_cursor_end.first < first_fully_vis_row && m_vscroll) {
        int diff = (first_fully_vis_row - m_cursor_end.first);
        m_vscroll->ScrollTo(std::max(vert_min, m_first_row_shown - GetFont()->Lineskip() * diff));
    }
    int last_fully_vis_row = LastFullyVisibleRow();
    if (last_fully_vis_row < m_cursor_end.first && m_vscroll) {
        int diff = (m_cursor_end.first - last_fully_vis_row);
        m_vscroll->ScrollTo(std::min(m_first_row_shown + GetFont()->Lineskip() * diff, vert_max));
    }

    // adjust m_first_col_shown position to bring the cursor into view
    int first_visible_char = FirstVisibleChar(m_cursor_end.first);
    int last_visible_char = LastVisibleChar(m_cursor_end.first);
    int client_char_posn = RowStartX(m_cursor_end.first) + CharXOffset(m_cursor_end.first, m_cursor_end.second);
    if (client_char_posn < 0 && m_hscroll) { // if the caret is at a place left of the current visible area
        if (first_visible_char - m_cursor_end.second < 5) { // if the caret is fewer than five characters before first_visible_char
            // try to move the caret by five characters
            int five_char_distance = StringIndexOf(m_cursor_end.first, first_visible_char) -
                StringIndexOf(m_cursor_end.first, (5 < first_visible_char) ? first_visible_char - 5 : 0);
            m_hscroll->ScrollTo(m_first_col_shown - five_char_distance);
        } else { // if the caret is more than five characters before m_first_char_shown, just move straight to that spot
            m_hscroll->ScrollTo(horz_min + m_first_col_shown + client_char_posn);
        }
    } else if (cl_sz.x <= client_char_posn && m_hscroll) { // if the caret is moving to a place right of the current visible area
        if (m_cursor_end.second - last_visible_char < 5) { // if the caret is fewer than five characters after last_visible_char
            // try to move the caret by five characters
            int last_char_of_line = static_cast<int>(GetLineData()[m_cursor_end.first].char_data.size()) - 1;
            int five_char_distance = StringIndexOf(m_cursor_end.first, (last_visible_char + 5 < last_char_of_line) ? last_visible_char + 5 : last_char_of_line) -
                StringIndexOf(m_cursor_end.first, last_visible_char);
            m_hscroll->ScrollTo(m_first_col_shown + five_char_distance);
        } else { // if the caret is more than five characters before m_first_char_shown, just move straight to that spot
            m_hscroll->ScrollTo(std::min(horz_min + m_first_col_shown + client_char_posn, horz_max));
        }
    }
}

void MultiEdit::AdjustScrolls()
{
    bool need_vert = false, need_horz = false;

    // this client area calculation disregards the thickness of scrolls
    Pt cl_sz = Edit::ClientLowerRight() - Edit::ClientUpperLeft();
    Pt contents_sz = GetFont()->TextExtent(WindowText(), TextFormat(), (TextFormat() & (TF_WORDBREAK | TF_LINEWRAP)) ? cl_sz.x : 0);
    contents_sz.y = GetLineData().size() * GetFont()->Lineskip();
    int excess_width = contents_sz.x - cl_sz.x;

    need_vert =
        (!(m_style & NO_VSCROLL) && (contents_sz.y > cl_sz.y ||
                                     (contents_sz.y > cl_sz.y - SCROLL_WIDTH && contents_sz.x > cl_sz.x - SCROLL_WIDTH)));
    need_horz =
        (!(m_style & NO_HSCROLL) && (contents_sz.x > cl_sz.x ||
                                     (contents_sz.x > cl_sz.x - SCROLL_WIDTH && contents_sz.y > cl_sz.y - SCROLL_WIDTH)));

    Pt orig_cl_sz = ClientSize();

    const int GAP = PIXEL_MARGIN - 2; // the space between the client area and the border

    boost::shared_ptr<StyleFactory> style = GetStyleFactory();

    int vscroll_min = (m_style & TERMINAL_STYLE) ? (cl_sz.y - contents_sz.y) : 0;
    int hscroll_min = 0; // default values for TF_LEFT
    if (m_style & TF_RIGHT) {
        hscroll_min = -excess_width;
    } else if (m_style & TF_CENTER) {
        hscroll_min = -excess_width / 2;
    }
    int vscroll_max = vscroll_min + contents_sz.y - 1;
    int hscroll_max = hscroll_min + contents_sz.x - 1;

    if (m_vscroll) { // if scroll already exists...
        if (!need_vert) { // remove scroll
            DeleteChild(m_vscroll);
            m_vscroll = 0;
        } else { // ensure vertical scroll has the right logical and physical dimensions
            m_vscroll->SizeScroll(vscroll_min, vscroll_max, cl_sz.y / 8, cl_sz.y - (need_horz ? SCROLL_WIDTH : 0));
            int scroll_x = cl_sz.x + GAP - SCROLL_WIDTH;
            int scroll_y = -GAP;
            m_vscroll->SizeMove(Pt(scroll_x, scroll_y), Pt(scroll_x + SCROLL_WIDTH, scroll_y + cl_sz.y + 2 * GAP - (need_horz ? SCROLL_WIDTH : 0)));
        }
    } else if (!m_vscroll && need_vert) { // if scroll doesn't exist but is needed
        m_vscroll = style->NewScroll(cl_sz.x + GAP - SCROLL_WIDTH, -GAP, SCROLL_WIDTH, cl_sz.y + 2 * GAP - (need_horz ? SCROLL_WIDTH : 0), VERTICAL, m_color, CLR_SHADOW);
        m_vscroll->SizeScroll(vscroll_min, vscroll_max, cl_sz.y / 8, cl_sz.y - (need_horz ? SCROLL_WIDTH : 0));
        AttachChild(m_vscroll);
        Connect(m_vscroll->ScrolledSignal, &MultiEdit::VScrolled, this);
    }

    if (m_hscroll) { // if scroll already exists...
        if (!need_horz) { // remove scroll
            DeleteChild(m_hscroll);
            m_hscroll = 0;
        } else { // ensure horizontal scroll has the right logical and physical dimensions
            m_hscroll->SizeScroll(hscroll_min, hscroll_max, cl_sz.x / 8, cl_sz.x - (need_vert ? SCROLL_WIDTH : 0));
            int scroll_x = -GAP;
            int scroll_y = cl_sz.y + GAP - SCROLL_WIDTH;
            m_hscroll->SizeMove(Pt(scroll_x, scroll_y), Pt(scroll_x + cl_sz.x + 2 * GAP - (need_vert ? SCROLL_WIDTH : 0), scroll_y + SCROLL_WIDTH));
        }
    } else if (!m_hscroll && need_horz) { // if scroll doesn't exist but is needed
        m_hscroll = style->NewScroll(-GAP, cl_sz.y + GAP - SCROLL_WIDTH, cl_sz.x + 2 * GAP - (need_vert ? SCROLL_WIDTH : 0), SCROLL_WIDTH, HORIZONTAL, m_color, CLR_SHADOW);
        m_hscroll->SizeScroll(hscroll_min, hscroll_max, cl_sz.x / 8, cl_sz.x - (need_vert ? SCROLL_WIDTH : 0));
        AttachChild(m_hscroll);
        Connect(m_hscroll->ScrolledSignal, &MultiEdit::HScrolled, this);
    }

    // if the new client dimensions changed after adjusting the scrolls, they are unequal to the extent of the text,
    // and there is some kind of wrapping going on, we need to re-SetText()
    Pt new_cl_sz = ClientSize();
    if (orig_cl_sz != new_cl_sz && (new_cl_sz.x != contents_sz.x || new_cl_sz.y != contents_sz.y) && 
        (m_style & (TF_WORDBREAK | TF_LINEWRAP))) {
        SetText(WindowText());
    }
}

void MultiEdit::VScrolled(int upper, int lower, int range_upper, int range_lower)
{
    m_first_row_shown = upper;
}

void MultiEdit::HScrolled(int upper, int lower, int range_upper, int range_lower)
{
    m_first_col_shown = upper;
}