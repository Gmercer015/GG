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

#include "GGApp.h"

#include "GGBrowseInfoWnd.h"
#include "GGControl.h"
#include "GGPluginInterface.h"
#include "GGStyleFactory.h"
#include "GGZList.h"

#include <cassert>
#include <fstream>
#include <list>


using namespace GG;

namespace {
    /* returns the storage value of key_mods that should be used with keyboard accelerators the accelerators don't care
       which side of the keyboard you use for CTRL, SHIFT, etc., and whether or not the numlock or capslock are
       engaged.*/
    Uint32 MassagedAccelKeyMods(Uint32 key_mods)
    {
        key_mods &= ~(GGKMOD_NUM | GGKMOD_CAPS);
        if (key_mods & GGKMOD_CTRL)
            key_mods |= GGKMOD_CTRL;
        if (key_mods & GGKMOD_SHIFT)
            key_mods |= GGKMOD_SHIFT;
        if (key_mods & GGKMOD_ALT)
            key_mods |= GGKMOD_ALT;
        if (key_mods & GGKMOD_META)
            key_mods |= GGKMOD_META;
        return key_mods;
    }
}

// implementation data types
struct GG::AppImplData
{
    AppImplData() :
        focus_wnd(0),
        mouse_pos(0,0),
        mouse_rel(0,0),
        key_mods(0),
        mouse_repeat_delay(0),
        mouse_repeat_interval(0),
        double_click_interval(500),
        min_drag_time(250),
        min_drag_distance(5),
        prev_button_press_time(-1),
        prev_wnd_under_cursor(0),
        prev_wnd_under_cursor_time(-1),
        curr_wnd_under_cursor(0),
        curr_drag_wnd_dragged(false),
        wnd_region(WR_NONE),
        delta_t(0),
        FPS(-1.0),
        calc_FPS(false),
        max_FPS(0.0),
        double_click_wnd(0),
        double_click_start_time(-1),
        double_click_time(-1),
        style_factory(new StyleFactory()),
        save_wnd_fn(0),
        load_wnd_fn(0)
    {
        button_state[0] = button_state[1] = button_state[2] = false;
        drag_wnds[0] = drag_wnds[1] = drag_wnds[2] = 0;
    }

    std::string  app_name;              // the user-defined name of the apllication

    ZList        zlist;                 // object that keeps the GUI windows in the correct depth ordering
    Wnd*         focus_wnd;             // GUI window that currently has the input focus (this is the base level focus window, used when no modal windows are active)
    std::list<std::pair<Wnd*, Wnd*> >
                 modal_wnds;            // modal GUI windows, and the window with focus for that modality (only the one in back is active, simulating a stack but allowing traversal of the list)

    bool         button_state[3];       // the up/down states of the three buttons on the mouse are kept here
    Pt           mouse_pos;             // absolute position of mouse, based on last MOUSEMOVE event
    Pt           mouse_rel;             // relative position of mouse, based on last MOUSEMOVE event
    Uint32       key_mods;              // currently-depressed modifier keys, based on last KEYPRESS event

    int          mouse_repeat_delay;    // see note above App class definition
    int          mouse_repeat_interval;
    int          double_click_interval; // the maximum interval allowed between clicks that is still considered a double-click, in ms
    int          min_drag_time;         // the minimum amount of time that a drag must be in progress before it is considered a drag, in ms
    int          min_drag_distance;     // the minimum distance that a drag must cover before it is considered a drag

    int          prev_button_press_time;// the time of the most recent mouse button press
    Pt           prev_button_press_pos; // the location of the most recent mouse button press
    Wnd*         prev_wnd_under_cursor; // GUI window most recently under the input cursor; may be 0
    int          prev_wnd_under_cursor_time; // the time at which prev_wnd_under_cursor was initially set to its current value
    Wnd*         curr_wnd_under_cursor; // GUI window currently under the input cursor; may be 0
    Wnd*         drag_wnds[3];          // GUI window currently being clicked or dragged by each mouse button
    Pt           wnd_drag_offset;       // offset from the cursor of either the upper-left corner of the GUI window currently being dragged
    bool         curr_drag_wnd_dragged; // true iff the currently-pressed window (drag_wnd[N]) has actually been dragged some distance (in which case releasing the mouse button is not a click)
    Pt           wnd_resize_offset;     // offset from the cursor of either the upper-left or lowe-right corner of the GUI window currently being resized
    WndRegion    wnd_region;            // window region currently being dragged or clicked; for non-frame windows, this will always be WR_NONE

    boost::shared_ptr<BrowseInfoWnd>
                 browse_info_wnd;       // the current browse info window, if any
    int          browse_info_mode;      // the current browse info mode (only valid if browse_info_wnd is non-null)

    Wnd*         drag_drop_originating_wnd; // the window that originally owned the Wnds in drag_drop_wnds
    std::map<Wnd*, Pt>
                 drag_drop_wnds;        // the Wnds (and their offsets) that are being dragged and dropped between Wnds

    std::set<std::pair<Key, Uint32> >
                 accelerators;          // the keyboard accelerators

    std::map<std::pair<Key, Uint32>, boost::shared_ptr<App::AcceleratorSignalType> >
                 accelerator_sigs;      // the signals emitted by the keyboard accelerators

    int          delta_t;               // the number of ms since the last frame
    double       FPS;                   // the most recent calculation of the frames per second rendering speed (-1.0 if calcs are disabled)
    bool         calc_FPS;              // true iff FPS calcs are to be done
    double       max_FPS;               // the maximum allowed frames per second rendering speed

    Wnd*         double_click_wnd;      // GUI window most recently clicked
    int          double_click_button;   // the index of the mouse button used in the last click
    int          double_click_start_time;// the time from which we started measuring double_click_time, in ms
    int          double_click_time;     // time elapsed since last click, in ms

    boost::shared_ptr<StyleFactory> style_factory;

    App::SaveWndFn    save_wnd_fn;
    App::LoadWndFn    load_wnd_fn;
};

// static member(s)
App*                           App::s_app = 0;
boost::shared_ptr<AppImplData> App::s_impl;

// member functions
App::App(const std::string& app_name)
{
    assert(!s_app);
    s_app = this;
    assert(!s_impl);
    s_impl.reset(new AppImplData());
    s_impl->app_name = app_name;
}

App::~App()
{
}

Wnd* App::FocusWnd() const
{
    return s_impl->modal_wnds.empty() ? s_impl->focus_wnd : s_impl->modal_wnds.back().second;
}

Wnd* App::GetWindowUnder(const Pt& pt) const
{
    return s_impl->zlist.Pick(pt, ModalWindow());
}

int App::DeltaT() const
{
    return s_impl->delta_t;
}

bool App::FPSEnabled() const
{
    return s_impl->calc_FPS;
}

double App::FPS() const
{
    return s_impl->FPS;
}

std::string App::FPSString() const
{
    char buf[128];
    sprintf(buf, "%.2f frames per second", s_impl->FPS);
    return std::string(buf);
}

double App::MaxFPS() const
{
    return s_impl->max_FPS;
}

int App::MouseRepeatDelay() const
{
    return s_impl->mouse_repeat_delay;
}

int App::MouseRepeatInterval() const
{
    return s_impl->mouse_repeat_interval;
}

int App::DoubleClickInterval() const
{
    return s_impl->double_click_interval;
}

int App::MinDragTime() const
{
    return s_impl->min_drag_time;
}

int App::MinDragDistance() const
{
    return s_impl->min_drag_distance;
}

bool App::MouseButtonDown(int bn) const
{
    return (bn >= 0 && bn <= 2) ? s_impl->button_state[bn] : false;
}

Pt App::MousePosition() const
{
    return s_impl->mouse_pos;
}

Pt App::MouseMovement() const
{
    return s_impl->mouse_rel;
}

Uint32 App::KeyMods() const
{
    return s_impl->key_mods;
}

const boost::shared_ptr<StyleFactory>& App::GetStyleFactory() const
{
    return s_impl->style_factory;
}

App::const_accel_iterator App::accel_begin() const
{
    const AppImplData* impl = s_impl.get();
    return impl->accelerators.begin();
}

App::const_accel_iterator App::accel_end() const
{
    const AppImplData* impl = s_impl.get();
    return impl->accelerators.end();
}

App::AcceleratorSignalType& App::AcceleratorSignal(Key key, Uint32 key_mods) const
{
    boost::shared_ptr<AcceleratorSignalType>& sig_ptr = s_impl->accelerator_sigs[std::make_pair(key, key_mods)];
    if (!sig_ptr)
        sig_ptr.reset(new AcceleratorSignalType());
    return *sig_ptr;
}

void App::operator()()
{
    Run();
}

void App::SetFocusWnd(Wnd* wnd)
{
    // inform old focus wnd that it is losing focus
    if (FocusWnd())
        FocusWnd()->HandleEvent(Wnd::Event(Wnd::Event::LosingFocus));

    (s_impl->modal_wnds.empty() ? s_impl->focus_wnd : s_impl->modal_wnds.back().second) = wnd;

    // inform new focus wnd that it is gaining focus
    if (FocusWnd())
        FocusWnd()->HandleEvent(Wnd::Event(Wnd::Event::GainingFocus));
}

void App::Wait(int ms)
{
}

void App::Register(Wnd* wnd)
{
    if (wnd) s_impl->zlist.Add(wnd);
}

void App::RegisterModal(Wnd* wnd)
{
    if (wnd && wnd->Modal()) {
        s_impl->modal_wnds.push_back(std::make_pair(wnd, wnd));
        wnd->HandleEvent(Wnd::Event(Wnd::Event::GainingFocus));
    }
}

void App::Remove(Wnd* wnd)
{
    if (wnd) {
        if (!s_impl->modal_wnds.empty() && s_impl->modal_wnds.back().first == wnd) // if it's the current modal window, remove it from the modal list
            s_impl->modal_wnds.pop_back();
        else // if it's not a modal window, remove it from the z-order
            s_impl->zlist.Remove(wnd);
    }
}

void App::WndDying(Wnd* wnd)
{
    if (wnd) {
        Remove(wnd);
        if (MatchesOrContains(wnd, s_impl->focus_wnd))
            s_impl->focus_wnd = 0;
        for (std::list<std::pair<Wnd*, Wnd*> >::iterator it = s_impl->modal_wnds.begin(); it != s_impl->modal_wnds.end(); ++it) {
            if (MatchesOrContains(wnd, it->second)) {
                if (MatchesOrContains(wnd, it->first)) {
                    it->second = 0;
                } else { // if the modal window for the removed window's focus level is available, revert focus to the modal window
                    if ((it->second = it->first))
                        it->first->HandleEvent(Wnd::Event(Wnd::Event::GainingFocus));
                }
            }
        }
        if (MatchesOrContains(wnd, s_impl->prev_wnd_under_cursor))
            s_impl->prev_wnd_under_cursor = 0;
        if (MatchesOrContains(wnd, s_impl->curr_wnd_under_cursor))
            s_impl->curr_wnd_under_cursor = 0;
        if (MatchesOrContains(wnd, s_impl->drag_wnds[0])) {
            s_impl->drag_wnds[0] = 0;
            s_impl->wnd_region = WR_NONE;
        }
        if (MatchesOrContains(wnd, s_impl->drag_wnds[1])) {
            s_impl->drag_wnds[1] = 0;
            s_impl->wnd_region = WR_NONE;
        }
        if (MatchesOrContains(wnd, s_impl->drag_wnds[2])) {
            s_impl->drag_wnds[2] = 0;
            s_impl->wnd_region = WR_NONE;
        }
        if (MatchesOrContains(wnd, s_impl->double_click_wnd)) {
            s_impl->double_click_wnd = 0;
            s_impl->double_click_start_time = -1;
            s_impl->double_click_time = -1;
        }
        s_impl->drag_drop_wnds.erase(wnd);
    }
}

void App::EnableFPS(bool b/* = true*/)
{
    s_impl->calc_FPS = b;
    if (!b) 
        s_impl->FPS = -1.0f;
}

void App::SetMaxFPS(double max)
{
    if (max && max < 0.1)
        max = 0.1;
    s_impl->max_FPS = max;
}

void App::MoveUp(Wnd* wnd)
{
    if (wnd) s_impl->zlist.MoveUp(wnd);
}

void App::MoveDown(Wnd* wnd)
{
    if (wnd) s_impl->zlist.MoveDown(wnd);
}

void App::RegisterDragDropWnd(Wnd* wnd, const Pt& offset, Wnd* originating_wnd)
{
    if (!s_impl->drag_drop_wnds.empty() && originating_wnd != s_impl->drag_drop_originating_wnd) {
        throw std::runtime_error("App::RegisterDragDropWnd() : Attempted to register a drag drop item dragged from "
                                 "one window, when another window already has items being dragged from it.");
    }
    s_impl->drag_drop_wnds[wnd] = offset;
    s_impl->drag_drop_originating_wnd = originating_wnd;
}

void App::CancelDragDrop()
{
    s_impl->drag_drop_wnds.clear();
}

void App::EnableMouseDragRepeat(int delay, int interval)
{
    if (!delay) { // setting delay = 0 completely disables mouse drag repeat
        s_impl->mouse_repeat_delay = 0;
        s_impl->mouse_repeat_interval = 0;
    } else {
        s_impl->mouse_repeat_delay = delay;
        s_impl->mouse_repeat_interval = interval;
    }
}

void App::SetDoubleClickInterval(int interval)
{
    s_impl->double_click_interval = interval;
}

void App::SetMinDragTime(int time)
{
    s_impl->min_drag_time = time;
}

void App::SetMinDragDistance(int distance)
{
    s_impl->min_drag_distance = distance;
}

void App::SetAccelerator(Key key, Uint32 key_mods)
{
    key_mods = MassagedAccelKeyMods(key_mods);
    s_impl->accelerators.insert(std::make_pair(key, key_mods));
}

void App::RemoveAccelerator(Key key, Uint32 key_mods)
{
    key_mods = MassagedAccelKeyMods(key_mods);
    s_impl->accelerators.erase(std::make_pair(key, key_mods));
}

boost::shared_ptr<Font> App::GetFont(const std::string& font_filename, int pts, Uint32 range/* = Font::ALL_CHARS*/)
{
    return GetFontManager().GetFont(font_filename, pts, range);
}

void App::FreeFont(const std::string& font_filename, int pts)
{
    GetFontManager().FreeFont(font_filename, pts);
}

boost::shared_ptr<Texture> App::StoreTexture(Texture* texture, const std::string& texture_name)
{
    return GetTextureManager().StoreTexture(texture, texture_name);
}

boost::shared_ptr<Texture> App::StoreTexture(boost::shared_ptr<Texture> texture, const std::string& texture_name)
{
    return GetTextureManager().StoreTexture(texture, texture_name);
}

boost::shared_ptr<Texture> App::GetTexture(const std::string& name, bool mipmap/* = false*/)
{
    return GetTextureManager().GetTexture(name, mipmap);
}

void App::FreeTexture(const std::string& name)
{
    GetTextureManager().FreeTexture(name);
}

void App::SetStyleFactory(const boost::shared_ptr<StyleFactory>& factory) const
{
    s_impl->style_factory = factory;
    if (!s_impl->style_factory)
        s_impl->style_factory.reset(new StyleFactory());
}

void App::SaveWnd(const Wnd* wnd, const std::string& name, boost::archive::xml_oarchive& ar)
{
    if (!s_impl->save_wnd_fn)
        throw BadFunctionPointer("App::SaveWnd() : Attempted call on null function pointer.");
    s_impl->save_wnd_fn(wnd, name, ar);
}

void App::LoadWnd(Wnd*& wnd, const std::string& name, boost::archive::xml_iarchive& ar)
{
    if (!s_impl->load_wnd_fn)
        throw BadFunctionPointer("App::LoadWnd() : Attempted call on null function pointer.");
    s_impl->load_wnd_fn(wnd, name, ar);
}

void App::SetSaveWndFunction(SaveWndFn fn)
{
    s_impl->save_wnd_fn = fn;
}

void App::SetLoadWndFunction(LoadWndFn fn)
{
    s_impl->load_wnd_fn = fn;
}

void App::SetSaveLoadFunctions(const PluginInterface& interface)
{
    s_impl->save_wnd_fn = interface.SaveWnd;
    s_impl->load_wnd_fn = interface.LoadWnd;
}

App* App::GetApp()
{
    return s_app;
}

void App::RenderWindow(Wnd* wnd)
{
    if (wnd) {
        if (wnd->Visible())
            wnd->Render();
        bool clip = wnd->ClipChildren();
        if (clip)
            wnd->BeginClipping();
        for (std::list<Wnd*>::iterator it = wnd->m_children.begin(); it != wnd->m_children.end(); ++it) {
            if ((*it)->Visible())
                RenderWindow(*it);
        }
        if (clip)
            wnd->EndClipping();
    }
}

void App::HandleGGEvent(EventType event, Key key, Uint32 key_mods, const Pt& pos, const Pt& rel)
{
    s_impl->key_mods = key_mods;

    int curr_ticks = Ticks();

    // track double-click time and time-out any pending double-click that has outlived its interval
    if (s_impl->double_click_time >= 0) {
        s_impl->double_click_time = curr_ticks - s_impl->double_click_start_time;
        if (s_impl->double_click_time >= s_impl->double_click_interval) {
            s_impl->double_click_start_time = -1;
            s_impl->double_click_time = -1;
            s_impl->double_click_wnd = 0;
        }
    }

    switch (event) {
    case IDLE:{
        if (s_impl->curr_wnd_under_cursor)
            ProcessBrowseInfo();
        break;}
    case KEYPRESS:{
        s_impl->browse_info_wnd.reset();
        s_impl->browse_info_mode = -1;
        bool processed = false;
        // only process accelerators when there are no modal windows active; otherwise, accelerators would be an end-run around modality
        if (s_impl->modal_wnds.empty()) {
            // the focus_wnd may care about the state of the numlock and capslock, or which side of the keyboard's 
            // CTRL, SHIFT, etc. was pressed, but the accelerators don't
            Uint32 massaged_mods = MassagedAccelKeyMods(key_mods);
            if (s_impl->accelerators.find(std::make_pair(key, massaged_mods)) != s_impl->accelerators.end())
                processed = AcceleratorSignal(key, massaged_mods)();
        }
        if (!processed && FocusWnd())
            FocusWnd()->HandleEvent(Wnd::Event(Wnd::Event::Keypress, key, key_mods));
        break;}
    case MOUSEMOVE:{
        s_impl->curr_wnd_under_cursor = GetWindowUnder(pos); // get window under mouse position

        // record these
        s_impl->mouse_pos = pos; // mouse position
        s_impl->mouse_rel = rel; // mouse movement

        // then act on mouse motion
        if (s_impl->drag_wnds[0]) { // only respond to left mouse button drags
            if (s_impl->wnd_region == WR_MIDDLE || s_impl->wnd_region == WR_NONE) { // send drag message to window or initiate drag-drop
                Pt diff = s_impl->prev_button_press_pos - pos;
                int drag_distance = diff.x * diff.x + diff.y * diff.y;
                // ensure that the minimum drag requirements are met
                if (s_impl->min_drag_time < (curr_ticks - s_impl->prev_button_press_time) &&
                    (s_impl->min_drag_distance * s_impl->min_drag_distance < drag_distance)) {
                    if (!s_impl->drag_wnds[0]->Dragable() &&
                        s_impl->drag_wnds[0]->DragDropDataType() != "" &&
                        s_impl->drag_drop_wnds.find(s_impl->drag_wnds[0]) == s_impl->drag_drop_wnds.end()) {
                        Wnd* parent = s_impl->drag_wnds[0]->Parent();
                        Pt offset = s_impl->prev_button_press_pos - s_impl->drag_wnds[0]->UpperLeft();
                        RegisterDragDropWnd(s_impl->drag_wnds[0], offset, parent);
                        if (parent)
                            parent->StartingChildDragDrop(s_impl->drag_wnds[0], offset);
                    } else {
                        Pt start_pos = s_impl->drag_wnds[0]->UpperLeft();
                        Pt move = pos + s_impl->wnd_drag_offset - s_impl->drag_wnds[0]->UpperLeft();
                        s_impl->drag_wnds[0]->HandleEvent(Wnd::Event(Wnd::Event::LDrag, pos, move, key_mods));
                        if (start_pos != s_impl->drag_wnds[0]->UpperLeft())
                            s_impl->curr_drag_wnd_dragged = true;
                    }
                }
            } else if (s_impl->drag_wnds[0]->Resizable()) { // send appropriate resize message to window
                Pt offset_pos = pos + s_impl->wnd_resize_offset;
                if (Wnd* parent = s_impl->drag_wnds[0]->Parent())
                    offset_pos -= parent->ClientUpperLeft();
                switch (s_impl->wnd_region)
                {
                case WR_TOPLEFT:
                    s_impl->drag_wnds[0]->SizeMove(offset_pos, s_impl->drag_wnds[0]->RelativeLowerRight());
                    break;
                case WR_TOP:
                    s_impl->drag_wnds[0]->SizeMove(Pt(s_impl->drag_wnds[0]->RelativeUpperLeft().x,
                                                      offset_pos.y),
                                                   s_impl->drag_wnds[0]->RelativeLowerRight());
                    break;
                case WR_TOPRIGHT:
                    s_impl->drag_wnds[0]->SizeMove(Pt(s_impl->drag_wnds[0]->RelativeUpperLeft().x,
                                                      offset_pos.y),
                                                   Pt(offset_pos.x,
                                                      s_impl->drag_wnds[0]->RelativeLowerRight().y));
                    break;
                case WR_MIDLEFT:
                    s_impl->drag_wnds[0]->SizeMove(Pt(offset_pos.x,
                                                      s_impl->drag_wnds[0]->RelativeUpperLeft().y),
                                                   s_impl->drag_wnds[0]->RelativeLowerRight());
                    break;
                case WR_MIDRIGHT:
                    s_impl->drag_wnds[0]->SizeMove(s_impl->drag_wnds[0]->RelativeUpperLeft(),
                                                   Pt(offset_pos.x,
                                                      s_impl->drag_wnds[0]->RelativeLowerRight().y));
                    break;
                case WR_BOTTOMLEFT:
                    s_impl->drag_wnds[0]->SizeMove(Pt(offset_pos.x,
                                                      s_impl->drag_wnds[0]->RelativeUpperLeft().y),
                                                   Pt(s_impl->drag_wnds[0]->RelativeLowerRight().x,
                                                      offset_pos.y));
                    break;
                case WR_BOTTOM:
                    s_impl->drag_wnds[0]->SizeMove(s_impl->drag_wnds[0]->RelativeUpperLeft(),
                                                   Pt(s_impl->drag_wnds[0]->RelativeLowerRight().x,
                                                      offset_pos.y));
                    break;
                case WR_BOTTOMRIGHT:
                    s_impl->drag_wnds[0]->SizeMove(s_impl->drag_wnds[0]->RelativeUpperLeft(), offset_pos);
                    break;
                default:
                    break;
                }
            } else if (s_impl->drag_wnds[0]->DragKeeper()) {
                Pt start_pos = s_impl->drag_wnds[0]->UpperLeft();
                Pt move = pos + s_impl->wnd_drag_offset - s_impl->drag_wnds[0]->UpperLeft();
                s_impl->drag_wnds[0]->HandleEvent(Wnd::Event(Wnd::Event::LDrag, pos, move, key_mods));
                if (start_pos != s_impl->drag_wnds[0]->UpperLeft())
                    s_impl->curr_drag_wnd_dragged = true;
            }
        } else if (s_impl->curr_wnd_under_cursor && s_impl->prev_wnd_under_cursor == s_impl->curr_wnd_under_cursor) { // if !s_impl->drag_wnds[0] and we're moving over the same (valid) object we were during the last iteration
            s_impl->curr_wnd_under_cursor->HandleEvent(Wnd::Event(Wnd::Event::MouseHere, pos, 0));
            ProcessBrowseInfo();
        } else { // if !s_impl->drag_wnds[0] and s_impl->prev_wnd_under_cursor != s_impl->curr_wnd_under_cursor, we're just moving around
            if (s_impl->prev_wnd_under_cursor)
                s_impl->prev_wnd_under_cursor->HandleEvent(Wnd::Event(Wnd::Event::MouseLeave, pos, 0));
            if (s_impl->curr_wnd_under_cursor)
                s_impl->curr_wnd_under_cursor->HandleEvent(Wnd::Event(Wnd::Event::MouseEnter, pos, 0));
        }
        if (s_impl->prev_wnd_under_cursor != s_impl->curr_wnd_under_cursor) {
            s_impl->browse_info_wnd.reset();
            s_impl->prev_wnd_under_cursor_time = curr_ticks;
        }
        s_impl->prev_wnd_under_cursor = s_impl->curr_wnd_under_cursor; // update this for the next time around
        break;}
    case LPRESS:
    case MPRESS:
    case RPRESS:{
        s_impl->curr_wnd_under_cursor = GetWindowUnder(pos);  // update window under mouse position
        s_impl->browse_info_wnd.reset();
        s_impl->prev_wnd_under_cursor_time = curr_ticks;
        s_impl->prev_button_press_time = curr_ticks;
        s_impl->prev_button_press_pos = pos;
        if (s_impl->curr_wnd_under_cursor)
            s_impl->wnd_drag_offset = s_impl->curr_wnd_under_cursor->UpperLeft() - pos;
        switch (event) {
        case LPRESS:{
            s_impl->button_state[0] = true;
            s_impl->drag_wnds[0] = s_impl->curr_wnd_under_cursor; // track this window as the one being dragged by the left mouse button
            // if this window is not a disabled Control window, it becomes the focus window
            Control* control = 0;
            if (s_impl->drag_wnds[0] && (!(control = dynamic_cast<Control*>(s_impl->drag_wnds[0])) || !control->Disabled()))
                SetFocusWnd(s_impl->drag_wnds[0]);
            if (s_impl->drag_wnds[0]) {
                s_impl->wnd_region = s_impl->drag_wnds[0]->WindowRegion(pos); // and determine whether a resize-region of it is being dragged
                if (s_impl->wnd_region % 3 == 0) // left regions
                    s_impl->wnd_resize_offset.x = s_impl->drag_wnds[0]->UpperLeft().x - pos.x;
                else
                    s_impl->wnd_resize_offset.x = s_impl->drag_wnds[0]->LowerRight().x - pos.x;
                if (s_impl->wnd_region < 3) // top regions
                    s_impl->wnd_resize_offset.y = s_impl->drag_wnds[0]->UpperLeft().y - pos.y;
                else
                    s_impl->wnd_resize_offset.y = s_impl->drag_wnds[0]->LowerRight().y - pos.y;
                Wnd* drag_wnds_root_parent = s_impl->drag_wnds[0]->RootParent();
                MoveUp(drag_wnds_root_parent ? drag_wnds_root_parent : s_impl->drag_wnds[0]); // move root window up to top of z-order
                s_impl->drag_wnds[0]->HandleEvent(Wnd::Event(Wnd::Event::LButtonDown, pos, key_mods));
            }
            break;}
        case MPRESS:{
            s_impl->button_state[1] = true;
            break;}
        case RPRESS:{
            s_impl->button_state[2] = true;
            s_impl->drag_wnds[2] = s_impl->curr_wnd_under_cursor;  // track this window as the one being dragged by the right mouse button
            if (s_impl->drag_wnds[2])
                s_impl->drag_wnds[2]->HandleEvent(Wnd::Event(Wnd::Event::RButtonDown, pos, key_mods));
            break;}
        default: break;
        }
        s_impl->prev_wnd_under_cursor = s_impl->curr_wnd_under_cursor; // update this for the next time around
        break;}
    case LRELEASE:
    case MRELEASE:
    case RRELEASE:{
        s_impl->browse_info_wnd.reset();
        s_impl->prev_wnd_under_cursor_time = curr_ticks;
        switch (event) {
        case LRELEASE:{
            Wnd* click_wnd = s_impl->drag_wnds[0];
            s_impl->curr_wnd_under_cursor = s_impl->zlist.Pick(pos, ModalWindow(), s_impl->curr_drag_wnd_dragged ? click_wnd : 0);
            s_impl->button_state[0] = false;
            s_impl->drag_wnds[0] = 0;       // if the mouse button is released, stop the tracking the drag window
            s_impl->wnd_region = WR_NONE;   // and clear this, just in case
            // if the release is over the Wnd where the button-down event occurred, and that Wnd has not been dragged
            if (click_wnd && s_impl->curr_wnd_under_cursor == click_wnd) {
                // if this is second l-click over a window that just received an l-click within
                // the time limit -- it's a double-click, not a click
                if (s_impl->double_click_time > 0 && s_impl->double_click_wnd == click_wnd &&
                    s_impl->double_click_button == 0) {
                    s_impl->double_click_wnd = 0;
                    s_impl->double_click_start_time = -1;
                    s_impl->double_click_time = -1;
                    click_wnd->HandleEvent(Wnd::Event(Wnd::Event::LDoubleClick, pos, key_mods));
                } else {
                    if (s_impl->double_click_time > 0) {
                        s_impl->double_click_wnd = 0;
                        s_impl->double_click_start_time = -1;
                        s_impl->double_click_time = -1;
                    } else {
                        s_impl->double_click_start_time = curr_ticks;
                        s_impl->double_click_time = 0;
                        s_impl->double_click_wnd = click_wnd;
                        s_impl->double_click_button = 0;
                    }
                    click_wnd->HandleEvent(Wnd::Event(Wnd::Event::LClick, pos, key_mods));
                }
            } else {
                s_impl->double_click_wnd = 0;
                s_impl->double_click_time = -1;
                if (click_wnd)
                    click_wnd->HandleEvent(Wnd::Event(Wnd::Event::LButtonUp, pos, key_mods));
                if (s_impl->curr_wnd_under_cursor) {
                    // this is a normal (unregistered) drag that results in a drag-drop, or possibly one of several Wnds
                    // being dragged in a registered drag
                    if (click_wnd) {
                        Wnd* parent = click_wnd->Parent();
                        if (click_wnd->DragDropDataType() != "" && s_impl->curr_wnd_under_cursor->AcceptDrop(click_wnd, pos) && parent)
                            parent->ChildDraggedAway(click_wnd, s_impl->curr_wnd_under_cursor);
                        s_impl->drag_drop_wnds.erase(click_wnd);
                    }
                    // process registered drag-drop Wnds
                    for (std::map<Wnd*, Pt>::iterator it = s_impl->drag_drop_wnds.begin();
                         it != s_impl->drag_drop_wnds.end(); ++it) {
                        if (s_impl->curr_wnd_under_cursor->AcceptDrop(it->first, pos) && s_impl->drag_drop_originating_wnd)
                            s_impl->drag_drop_originating_wnd->ChildDraggedAway(it->first, s_impl->curr_wnd_under_cursor);
                    }
                }
            }
            s_impl->drag_drop_wnds.clear();
            break;}
        case MRELEASE:{
            Wnd* click_wnd = s_impl->drag_wnds[1];
            s_impl->curr_wnd_under_cursor = s_impl->zlist.Pick(pos, ModalWindow(), s_impl->curr_drag_wnd_dragged ? click_wnd : 0);
            s_impl->button_state[1] = false;
            s_impl->double_click_wnd = 0;
            s_impl->double_click_time = -1;
            break;}
        case RRELEASE:{
            Wnd* click_wnd = s_impl->drag_wnds[2];
            s_impl->curr_wnd_under_cursor = s_impl->zlist.Pick(pos, ModalWindow(), s_impl->curr_drag_wnd_dragged ? click_wnd : 0);
            s_impl->button_state[2] = false;
            s_impl->drag_wnds[2] = 0;
            if (click_wnd && s_impl->curr_wnd_under_cursor == click_wnd) { // if the release is over the place where the button-down event occurred
                // if this is second r-click over a window that just received an r-click within
                // the time limit -- it's a double-click, not a click
                if (s_impl->double_click_time > 0 && s_impl->double_click_wnd == click_wnd &&
                    s_impl->double_click_button == 2) {
                    s_impl->double_click_wnd = 0;
                    s_impl->double_click_time = -1;
                    click_wnd->HandleEvent(Wnd::Event(Wnd::Event::RDoubleClick, pos, key_mods));
                } else {
                    if (s_impl->double_click_time > 0) {
                        s_impl->double_click_wnd = 0;
                        s_impl->double_click_time = -1;
                    } else {
                        s_impl->double_click_time = 0;
                        s_impl->double_click_wnd = click_wnd;
                        s_impl->double_click_button = 2;
                    }
                    click_wnd->HandleEvent(Wnd::Event(Wnd::Event::RClick, pos, key_mods));
                }
            } else {
                s_impl->double_click_wnd = 0;
                s_impl->double_click_time = -1;
            }
            break;}
        default:
            break;
        }
        s_impl->prev_wnd_under_cursor = s_impl->curr_wnd_under_cursor; // update this for the next time around
        s_impl->curr_drag_wnd_dragged = false;
        break;}
    case MOUSEWHEEL:{
        s_impl->curr_wnd_under_cursor = GetWindowUnder(pos);  // update window under mouse position
        s_impl->browse_info_wnd.reset();
        s_impl->prev_wnd_under_cursor_time = curr_ticks;
        // don't send out 0-movement wheel messages, or send wheel messages when a button is depressed
        if (s_impl->curr_wnd_under_cursor && rel.y && !(s_impl->button_state[0] || s_impl->button_state[1] || s_impl->button_state[2]))
            s_impl->curr_wnd_under_cursor->HandleEvent(Wnd::Event(Wnd::Event::MouseWheel, pos, rel.y, key_mods));
        s_impl->prev_wnd_under_cursor = s_impl->curr_wnd_under_cursor; // update this for the next time around
        break;}
    default:
        break;
    }
}

void App::ProcessBrowseInfo()
{
    if (s_impl->modal_wnds.empty() || s_impl->curr_wnd_under_cursor->RootParent() == s_impl->modal_wnds.back().first) {
        const std::vector<Wnd::BrowseInfoMode>& browse_modes = s_impl->curr_wnd_under_cursor->BrowseModes();
        int delta_t = Ticks() - s_impl->prev_wnd_under_cursor_time;
        for (unsigned int i = 0; i < browse_modes.size(); ++i) {
            if (browse_modes[i].time < delta_t && s_impl->browse_info_wnd != browse_modes[i].wnd) {
                s_impl->browse_info_wnd = browse_modes[i].wnd;
                s_impl->browse_info_mode = i;
                s_impl->browse_info_wnd->MoveTo(s_impl->mouse_pos);
                break;
            }
        }
    }
}

void App::Render()
{
    Enter2DMode();
    // render normal windows back-to-front
    for (ZList::reverse_iterator it = s_impl->zlist.rbegin(); it != s_impl->zlist.rend(); ++it) {
        RenderWindow(*it);
    }
    // render modal windows back-to-front
    for (std::list<std::pair<Wnd*, Wnd*> >::iterator it = s_impl->modal_wnds.begin(); it != s_impl->modal_wnds.end(); ++it) {
        RenderWindow(it->first);
    }
    // render the active browse info window, if any
    if (s_impl->browse_info_wnd) {
        if (!s_impl->curr_wnd_under_cursor) {
            s_impl->browse_info_wnd.reset();
            s_impl->browse_info_mode = -1;
            s_impl->prev_wnd_under_cursor_time = Ticks();
        } else {
            s_impl->browse_info_wnd->Update(s_impl->browse_info_mode, s_impl->curr_wnd_under_cursor);
            RenderWindow(s_impl->browse_info_wnd.get());
        }
    }
    // render drag-drop windows in arbitrary order (sorted by pointer value)
    for (std::map<Wnd*, Pt>::const_iterator it = s_impl->drag_drop_wnds.begin(); it != s_impl->drag_drop_wnds.end(); ++it) {
        if (it->first->Visible()) {
            Pt parent_offset = (it->first->Parent() ? it->first->Parent()->ClientUpperLeft() : Pt(0, 0));
            Pt old_pos = it->first->UpperLeft() - parent_offset;
            it->first->MoveTo(s_impl->mouse_pos - parent_offset - it->second);
            RenderWindow(it->first);
            it->first->MoveTo(old_pos);
        }
    }
    Exit2DMode();
}

Wnd* App::ModalWindow() const
{
    Wnd* retval = 0;
    if (!s_impl->modal_wnds.empty())
        retval = s_impl->modal_wnds.back().first;
    return retval;
}

void App::SetFPS(double FPS)
{
    s_impl->FPS = FPS;
}

void App::SetDeltaT(int delta_t)
{
    s_impl->delta_t = delta_t;
}


bool GG::MatchesOrContains(const Wnd* lwnd, const Wnd* rwnd)
{
    if (rwnd) {
        for (const Wnd* w = rwnd; w; w = w->Parent()) {
            if (w == lwnd)
                return true;
        }
    } else if (rwnd == lwnd) {
        return true;
    }
    return false;
}