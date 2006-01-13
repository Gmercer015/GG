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

#include "GGFileDlg.h"

#include "../GGApp.h"
#include "../GGButton.h"
#include "../GGEdit.h"
#include "../GGDropDownList.h"
#include "../GGDrawUtil.h"
#include "../GGStyleFactory.h"
#include "../GGTextControl.h"

#include "GGThreeButtonDlg.h"

// HACK! MSVC #defines int64_t to be __int64, which breaks the code in boost's cstdint.hpp
#ifdef int64_t
#undef int64_t
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/spirit.hpp>
#include <boost/spirit/dynamic.hpp>

using namespace GG;

namespace {
    using namespace boost::spirit;
    // these functors are used by the if_p, while_p, and for_p parsers in UpdateList()
    struct LeadingWildcard
    {
        LeadingWildcard(const std::string& str) : m_value(!str.empty() && *str.begin() == '*') {}
        bool operator()() const {return m_value;}
        bool m_value;
    };
    struct TrailingWildcard
    {
        TrailingWildcard(const std::string& str) : m_value(!str.empty() && *str.rbegin() == '*') {}
        bool operator()() const {return m_value;}
        bool m_value;
    };

    struct Index
    {
        Index(int i = 0) : m_initial_value(i) {}
        void operator()() const {value = m_initial_value;}
        int m_initial_value;
        static int value;
    };
    int Index::value;
    struct IndexLess
    {
        IndexLess(int val) : m_value(val) {}
        bool operator()() const {return Index::value <  m_value;}
        int m_value;
    };
    struct IndexIncr  
    {
        void operator()() const {++Index::value;}
    };

    struct FrontStringBegin
    {
        FrontStringBegin(const boost::shared_ptr<std::vector<std::string> >& strings) : m_strings(strings) {}
        const char* operator()() const {return m_strings->front().c_str();}
        boost::shared_ptr<std::vector<std::string> > m_strings;
    };
    struct FrontStringEnd
    {
        FrontStringEnd(const boost::shared_ptr<std::vector<std::string> >& strings) : m_strings(strings) {}
        const char* operator()() const {return m_strings->front().c_str() + m_strings->front().size();}
        boost::shared_ptr<std::vector<std::string> > m_strings;
    };
    struct IndexedStringBegin
    {
        IndexedStringBegin(const boost::shared_ptr<std::vector<std::string> >& strings) : m_strings(strings) {}
        const char* operator()() const {return (*m_strings)[Index::value].c_str();}
        boost::shared_ptr<std::vector<std::string> > m_strings;
    };
    struct IndexedStringEnd
    {
        IndexedStringEnd(const boost::shared_ptr<std::vector<std::string> >& strings) : m_strings(strings) {}
        const char* operator()() const {return (*m_strings)[Index::value].c_str() + (*m_strings)[Index::value].size();}
        boost::shared_ptr<std::vector<std::string> > m_strings;
    };

    bool WindowsRoot(const std::string& root_name)
    {
        return root_name.size() == 2 && std::isalpha(root_name[0]) && root_name[1] == ':';
    }

    const bool WIN32_PATHS = WindowsRoot(boost::filesystem::initial_path().root_name());

    const int H_SPACING = 10;
    const int V_SPACING = 10;
}

namespace fs = boost::filesystem;

// static member definition(s)
fs::path FileDlg::s_working_dir = fs::initial_path();


FileDlg::FileDlg() :
    Wnd(),
    m_in_win32_drive_selection(false),
    m_curr_dir_text(0),
    m_files_list(0),
    m_files_edit(0),
    m_filter_list(0),
    m_ok_button(0),
    m_cancel_button(0),
    m_files_label(0),
    m_file_types_label(0)
{}

FileDlg::FileDlg(const std::string& directory, const std::string& filename, bool save, bool multi,
                 const boost::shared_ptr<Font>& font, Clr color, Clr border_color, Clr text_color/* = CLR_BLACK*/) : 
    Wnd((App::GetApp()->AppWidth() - WIDTH) / 2, (App::GetApp()->AppHeight() - HEIGHT) / 2, WIDTH, HEIGHT, CLICKABLE | DRAGABLE | MODAL),
    m_color(color),
    m_border_color(border_color),
    m_text_color(text_color),
    m_font(font),
    m_save(save),
    m_in_win32_drive_selection(false),
    m_save_str("Save"),
    m_open_str("Open"),
    m_cancel_str("Cancel"),
    m_malformed_filename_str("Invalid file name."),
    m_overwrite_prompt_str("%1% exists.\nOk to overwrite it?"),
    m_invalid_filename_str("\"%1%\"\nis an invalid file name."),
    m_filename_is_a_directory_str("\"%1%\"\nis a directory."),
    m_file_does_not_exist_str("File \"%1%\"\ndoes not exist."),
    m_three_button_dlg_ok_str("Ok"),
    m_three_button_dlg_cancel_str("Cancel"),
    m_curr_dir_text(0),
    m_files_list(0),
    m_files_edit(0),
    m_filter_list(0),
    m_ok_button(0),
    m_cancel_button(0),
    m_files_label(0),
    m_file_types_label(0)
{
    CreateChildren(filename, multi);
    Init(directory);
}

std::set<std::string> FileDlg::Result() const
{
    return m_result;
}

const std::string& FileDlg::FilesString() const
{
    return m_files_label->WindowText();
}

const std::string& FileDlg::FileTypesString() const
{
    return m_file_types_label->WindowText();
}

const std::string& FileDlg::SaveString() const
{
    return m_save_str;
}

const std::string& FileDlg::OpenString() const
{
    return m_open_str;
}

const std::string& FileDlg::CancelString() const
{
    return m_cancel_str;
}

const std::string& FileDlg::MalformedFilenameString() const
{
    return m_malformed_filename_str;
}

const std::string& FileDlg::OverwritePromptString() const
{
    return m_overwrite_prompt_str;
}

const std::string& FileDlg::InvalidFilenameString() const
{
    return m_invalid_filename_str;
}

const std::string& FileDlg::FilenameIsADirectoryString() const
{
    return m_filename_is_a_directory_str;
}

const std::string& FileDlg::FileDoesNotExistString() const
{
    return m_file_does_not_exist_str;
}

const std::string& FileDlg::DeviceIsNotReadyString() const
{
    return m_device_is_not_ready_str;
}

const std::string& FileDlg::ThreeButtonDlgOKString() const
{
    return m_three_button_dlg_ok_str;
}

const std::string& FileDlg::ThreeButtonDlgCancelString() const
{
    return m_three_button_dlg_cancel_str;
}

void FileDlg::Render()
{
    FlatRectangle(UpperLeft().x, UpperLeft().y, LowerRight().x, LowerRight().y, m_color, m_border_color, 1);
}

void FileDlg::Keypress(Key key, Uint32 key_mods)
{
    if (key == GGK_RETURN || key == GGK_KP_ENTER)
        OkClicked();
    else if (key == GGK_ESCAPE)
        CancelClicked();
}

void FileDlg::SetFileFilters(const std::vector<std::pair<std::string, std::string> >& filters)
{
    m_file_filters = filters;
    PopulateFilters();
    UpdateList();
}

void FileDlg::SetFilesString(const std::string& str)
{
    m_files_label->SetText(str);
    PlaceLabelsAndEdits(Width() / 4 - H_SPACING, m_files_edit->Height());
}

void FileDlg::SetFileTypesString(const std::string& str)
{
    m_file_types_label->SetText(str);
    PlaceLabelsAndEdits(Width() / 4 - H_SPACING, m_files_edit->Height());
}

void FileDlg::SetSaveString(const std::string& str)
{
    bool set_button_text = m_ok_button->WindowText() == m_save_str;
    m_save_str = str;
    if (set_button_text)
        m_ok_button->SetText(m_save_str);
}

void FileDlg::SetOpenString(const std::string& str)
{
    bool set_button_text = m_ok_button->WindowText() == m_open_str;
    m_open_str = str;
    if (set_button_text)
        m_ok_button->SetText(m_open_str);
}

void FileDlg::SetCancelString(const std::string& str)
{
    m_cancel_str = str;
    m_cancel_button->SetText(m_cancel_str);
}

void FileDlg::SetMalformedFilenameString(const std::string& str)
{
    m_malformed_filename_str = str;
}

void FileDlg::SetOverwritePromptString(const std::string& str)
{
    m_overwrite_prompt_str = str;
}

void FileDlg::SetInvalidFilenameString(const std::string& str)
{
    m_invalid_filename_str = str;
}

void FileDlg::SetFilenameIsADirectoryString(const std::string& str)
{
    m_filename_is_a_directory_str = str;
}

void FileDlg::SetFileDoesNotExistString(const std::string& str)
{
    m_file_does_not_exist_str = str;
}

void FileDlg::SetThreeButtonDlgOKString(const std::string& str)
{
    m_three_button_dlg_ok_str = str;
}

void FileDlg::SetThreeButtonDlgCancelString(const std::string& str)
{
    m_three_button_dlg_cancel_str = str;
}

const fs::path& FileDlg::WorkingDirectory()
{
    return s_working_dir;
}

void FileDlg::CreateChildren(const std::string& filename, bool multi)
{
    if (m_save)
        multi = false;

    const int USABLE_WIDTH = Width() - 4 * H_SPACING;
    const int BUTTON_WIDTH = USABLE_WIDTH / 4;

    boost::shared_ptr<StyleFactory> style = GetStyleFactory();

    fs::path filename_path = fs::complete(fs::path(filename, fs::native));
    m_files_edit = style->NewEdit(0, 0, 1, "", m_font, m_border_color, m_text_color);
    m_files_edit->SetText(filename_path.leaf());
    m_filter_list = style->NewDropDownList(0, 0, 100, m_font->Lineskip(), m_font->Lineskip() * 3, m_border_color);
    m_filter_list->SetStyle(LB_NOSORT);

    m_files_edit->Resize(Pt(100, m_font->Height() + 2 * 5));
    m_files_edit->MoveTo(Pt(0, 0));
    m_filter_list->Resize(Pt(100, m_filter_list->Height()));
    m_filter_list->MoveTo(Pt(0, 0));

    const int BUTTON_HEIGHT = m_files_edit->Height(); // use the edit's height for the buttons as well

    m_curr_dir_text = style->NewTextControl(H_SPACING, V_SPACING / 2, "", m_font, m_text_color);
    m_files_label = style->NewTextControl(0, Height() - (BUTTON_HEIGHT + V_SPACING) * 2, Width() - (3 * BUTTON_WIDTH + 3 * H_SPACING), BUTTON_HEIGHT, "File(s):", m_font, m_text_color, TF_RIGHT | TF_VCENTER);
    m_file_types_label = style->NewTextControl(0, Height() - (BUTTON_HEIGHT + V_SPACING) * 1, Width() - (3 * BUTTON_WIDTH + 3 * H_SPACING), BUTTON_HEIGHT, "Type(s):", m_font, m_text_color, TF_RIGHT | TF_VCENTER);

    m_ok_button = style->NewButton(0, 0, 1, 1, m_save ? m_save_str : m_open_str, m_font, m_color, m_text_color);
    m_cancel_button = style->NewButton(0, 0, 1, 1, m_cancel_str, m_font, m_color, m_text_color);

    m_ok_button->Resize(Pt(BUTTON_WIDTH, BUTTON_HEIGHT));
    m_ok_button->MoveTo(Pt(Width() - (BUTTON_WIDTH + H_SPACING), Height() - (BUTTON_HEIGHT + V_SPACING) * 2));
    m_cancel_button->Resize(Pt(BUTTON_WIDTH, BUTTON_HEIGHT));
    m_cancel_button->MoveTo(Pt(Width() - (BUTTON_WIDTH + H_SPACING), Height() - (BUTTON_HEIGHT + V_SPACING)));

    // finally, we can create the listbox with the files in it, sized to fill the available space
    m_files_list = style->NewListBox(0, 0, 1, 1, m_border_color);
    m_files_list->SetStyle(LB_NOSORT | (multi ? 0 : LB_SINGLESEL));

    PlaceLabelsAndEdits(BUTTON_WIDTH, BUTTON_HEIGHT);
}

void FileDlg::PlaceLabelsAndEdits(int button_width, int button_height)
{
    int file_list_top = m_curr_dir_text->Height() + V_SPACING;
    m_files_list->Resize(Pt(Width() - 2 * H_SPACING,
                            Height() - (button_height + V_SPACING) * 2 - file_list_top - V_SPACING));
    m_files_list->MoveTo(Pt(H_SPACING, file_list_top));

    // determine the space needed to display both text labels in the chosen font; use this to expand the edit as far as
    // possible
    int labels_width = std::max(m_font->TextExtent(m_files_label->WindowText()).x, 
                                m_font->TextExtent(m_file_types_label->WindowText()).x) + H_SPACING;
    m_files_label->Resize(Pt(labels_width - H_SPACING / 2, m_files_label->Height()));
    m_file_types_label->Resize(Pt(labels_width - H_SPACING / 2, m_file_types_label->Height()));
    m_files_edit->SizeMove(Pt(labels_width, Height() - (button_height + V_SPACING) * 2),
                           Pt(Width() - (button_width + 2 * H_SPACING), Height() - (button_height + 2 * V_SPACING)));
    m_filter_list->SizeMove(Pt(labels_width, Height() - (button_height + V_SPACING)),
                            Pt(Width() - (button_width + 2 * H_SPACING), Height() - V_SPACING));
}

void FileDlg::Init(const std::string& directory)
{
    AttachChild(m_files_edit);
    AttachChild(m_filter_list);
    AttachChild(m_ok_button);
    AttachChild(m_cancel_button);
    AttachChild(m_files_list);
    AttachChild(m_curr_dir_text);
    AttachChild(m_files_label);
    AttachChild(m_file_types_label);

    if (directory != "") {
        fs::path dir_path = fs::complete(fs::path(directory, fs::native));
        if (!fs::exists(dir_path))
            throw BadInitialDirectory("FileDlg::Init() : Initial directory \"" + dir_path.native_directory_string() + "\" does not exist.");
        SetWorkingDirectory(dir_path);
    }

    UpdateDirectoryText();
    PopulateFilters();
    UpdateList();
    ConnectSignals();
}

void FileDlg::ConnectSignals()
{
    Connect(m_ok_button->ClickedSignal, &FileDlg::OkClicked, this);
    Connect(m_cancel_button->ClickedSignal, &FileDlg::CancelClicked, this);
    Connect(m_files_list->SelChangedSignal, &FileDlg::FileSetChanged, this);
    Connect(m_files_list->DoubleClickedSignal, &FileDlg::FileDoubleClicked, this);
    Connect(m_files_edit->EditedSignal, &FileDlg::FilesEditChanged, this);
    Connect(m_filter_list->SelChangedSignal, &FileDlg::FilterChanged, this);
}

void FileDlg::OkClicked()
{
    bool results_valid = false;

    // parse contents of edit control to determine file names
    m_result.clear();

    std::vector<std::string> files;
    parse(m_files_edit->WindowText().c_str(), (+anychar_p)[append(files)], space_p);
    std::sort(files.begin(), files.end());

    boost::shared_ptr<StyleFactory> style = GetStyleFactory();

    if (m_save) { // file save case
        if (m_ok_button->WindowText() != m_save_str) {
            OpenDirectory();
        } else if (files.size() == 1) {
            results_valid = true;
            std::string save_file = *files.begin();
            if (!fs::path::default_name_check()(save_file)) {
                boost::shared_ptr<ThreeButtonDlg> dlg(
                    style->NewThreeButtonDlg(150, 75, m_malformed_filename_str, m_font, m_color, m_border_color, m_color,
                                             m_text_color, 1, m_three_button_dlg_ok_str));
                dlg->Run();
                return;
            }
            fs::path p = s_working_dir / save_file;
            m_result.insert(p.native_directory_string());
            // check to see if file already exists; if so, ask if it's ok to overwrite
            if (fs::exists(p)) {
                std::string msg_str = boost::str(boost::format(m_overwrite_prompt_str) % save_file);
                boost::shared_ptr<ThreeButtonDlg> dlg(
                    style->NewThreeButtonDlg(300, 125, msg_str, m_font, m_color, m_border_color, m_color, m_text_color,
                                             2, m_three_button_dlg_ok_str, m_three_button_dlg_cancel_str));
                dlg->Run();
                results_valid = (dlg->Result() == 0);
            }
        }
    } else { // file open case
        if (files.empty()) {
            OpenDirectory();
        } else { // ensure the file(s) are valid before returning them
            for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {
                if (!fs::path::default_name_check()(*it)) {
                    std::string msg_str = boost::str(boost::format(m_invalid_filename_str) % (*it));
                    boost::shared_ptr<ThreeButtonDlg> dlg(
                        style->NewThreeButtonDlg(300, 125, msg_str, m_font, m_color, m_border_color, m_color,
                                                 m_text_color, 1, m_three_button_dlg_ok_str));
                    dlg->Run();
                    results_valid = false;
                    break;
                }
                fs::path p = s_working_dir / *it;
                if (fs::exists(p)) {
                    if (fs::is_directory(p)) {
                        std::string msg_str = boost::str(boost::format(m_filename_is_a_directory_str) % (*it));
                        boost::shared_ptr<ThreeButtonDlg> dlg(
                            style->NewThreeButtonDlg(300, 125, msg_str, m_font, m_color, m_border_color, m_color,
                                                     m_text_color, 1, m_three_button_dlg_ok_str));
                        dlg->Run();
                        results_valid = false;
                        break;
                    }
                    m_result.insert(p.native_directory_string());
                    results_valid = true; // indicate validity only if at least one good file was found
                } else {
                    std::string msg_str = boost::str(boost::format(m_file_does_not_exist_str) % (*it));
                    boost::shared_ptr<ThreeButtonDlg> dlg(
                        style->NewThreeButtonDlg(300, 125, msg_str, m_font, m_color, m_border_color, m_color,
                                                 m_text_color, 1, m_three_button_dlg_ok_str));
                    dlg->Run();
                    results_valid = false;
                    break;
                }
            }
        }
    }
    if (results_valid)
        m_done = true;
}

void FileDlg::CancelClicked()
{
    m_done = true;
    m_result.clear();
}

void FileDlg::FileSetChanged(const std::set<int>& files)
{
    std::string all_files;
    bool dir_selected = false;
    for (std::set<int>::const_iterator it = files.begin(); it != files.end(); ++it) {
        std::string filename = m_files_list->GetRow(*it)[0]->WindowText();
        if (filename[0] != '[') {
            if (!all_files.empty())
                all_files += " ";
            all_files += filename;
        } else {
            dir_selected = true;
        }
    }
    *m_files_edit << all_files;
    if (m_save && !dir_selected && m_ok_button->WindowText() != m_save_str)
        m_ok_button->SetText(m_save_str);
    else if (m_save && dir_selected && m_ok_button->WindowText() == m_save_str)
        m_ok_button->SetText(m_open_str);
}

void FileDlg::FileDoubleClicked(int n, ListBox::Row* row)
{
    std::string filename = (*row)[0]->WindowText();
    m_files_list->DeselectAll();
    m_files_list->SelectRow(n);
    OkClicked();
}

void FileDlg::FilesEditChanged(const std::string& str)
{
    if (m_save && m_ok_button->WindowText() != m_save_str)
        m_ok_button->SetText(m_save_str);
}

void FileDlg::FilterChanged(int idx)
{
    UpdateList();
}

void FileDlg::SetWorkingDirectory(const fs::path& p)
{
    m_files_edit->Clear();
    s_working_dir = p;
    UpdateDirectoryText();
    UpdateList();
}

void FileDlg::PopulateFilters()
{
    m_filter_list->Clear();
    if (m_file_filters.empty()) {
        m_file_types_label->Disable();
        m_filter_list->Disable();
    } else {
        for (unsigned int i = 0; i < m_file_filters.size(); ++i) {
            ListBox::Row* row = new ListBox::Row();
            row->push_back(m_file_filters[i].first, m_font, m_text_color);
            m_filter_list->Insert(row);
        }
        m_filter_list->Select(0);
    }
}

void FileDlg::UpdateList()
{
    m_files_list->Clear();
    fs::directory_iterator end_it;

    // define a wildcard ('*') as any combination of characters
    rule<> wildcard = anychar_p;

    // define file filters based on the filter strings in the filter drop list
    std::vector<rule<> > file_filters;

    int idx = m_filter_list->CurrentItemIndex();
    if (idx != -1) {
        std::vector<std::string> filter_specs; // the filter specifications (e.g. "*.png")
        parse(m_file_filters[idx].second.c_str(), *(!ch_p(',') >> (+(anychar_p - ','))[append(filter_specs)]), space_p);
        file_filters.resize(filter_specs.size());
        for (unsigned int i = 0; i < filter_specs.size(); ++i) {
            boost::shared_ptr<std::vector<std::string> > non_wildcards(new std::vector<std::string>); // the parts of the filter spec that are not wildcards
            parse(filter_specs[i].c_str(), *(*ch_p('*') >> (+(anychar_p - '*'))[append(*non_wildcards)]));

            if (non_wildcards->empty()) {
                file_filters[i] = *anychar_p;
            } else {
                file_filters[i] = 
                    if_p (LeadingWildcard(filter_specs[i])) [
                        *(wildcard - f_str_p(FrontStringBegin(non_wildcards), FrontStringEnd(non_wildcards))) 
                        >> f_str_p(FrontStringBegin(non_wildcards), FrontStringEnd(non_wildcards))
                    ] .else_p [
                        f_str_p(FrontStringBegin(non_wildcards), FrontStringEnd(non_wildcards))
                    ] 
                    >> for_p (Index(1), IndexLess(static_cast<int>(non_wildcards->size()) - 1), IndexIncr()) [
                        *(wildcard - f_str_p(IndexedStringBegin(non_wildcards), IndexedStringEnd(non_wildcards)))
                        >> f_str_p(IndexedStringBegin(non_wildcards), IndexedStringEnd(non_wildcards))
                    ] 
                    >> if_p (TrailingWildcard(filter_specs[i])) [
                        *wildcard
                    ];
            }
        }
    }

    if (!m_in_win32_drive_selection) {
        if ((s_working_dir.string() != s_working_dir.root_path().string() &&
             s_working_dir.branch_path().string() != "") ||
            WIN32_PATHS) {
            ListBox::Row* row = new ListBox::Row();
            row->push_back("[..]", m_font, m_text_color);
            m_files_list->Insert(row);
        }
        for (fs::directory_iterator it(s_working_dir); it != end_it; ++it) {
            try {
                if (fs::exists(*it) && fs::is_directory(*it) && it->leaf()[0] != '.') {
                    ListBox::Row* row = new ListBox::Row();
                    row->push_back("[" + it->leaf() + "]", m_font, m_text_color);
                    m_files_list->Insert(row);
                }
            } catch (const fs::filesystem_error& e) {
                // ignore files for which permission is denied, and rethrow other exceptions
                if (e.error() != fs::security_error)
                    throw;
            }
        }
        for (fs::directory_iterator it(s_working_dir); it != end_it; ++it) {
            try {
                if (fs::exists(*it) && !fs::is_directory(*it) && it->leaf()[0] != '.') {
                    bool meets_filters = file_filters.empty();
                    for (unsigned int i = 0; i < file_filters.size() && !meets_filters; ++i) {
                        if (parse(it->leaf().c_str(), file_filters[i]).full)
                            meets_filters = true;
                    }
                    if (meets_filters) {
                        ListBox::Row* row = new ListBox::Row();
                        row->push_back(it->leaf(), m_font, m_text_color);
                        m_files_list->Insert(row);
                    }
                }
            } catch (const fs::filesystem_error& e) {
                // ignore files for which permission is denied, and rethrow other exceptions
                if (e.error() != fs::security_error)
                    throw;
            }
        }
    } else {
        for (char c = 'C'; c <= 'Z'; ++c) {
            try {
                fs::path path(c + std::string(":"), fs::native);
                if (fs::exists(path)) {
                    ListBox::Row* row = new ListBox::Row();
                    row->push_back("[" + path.root_name() + "]", m_font, m_text_color);
                    m_files_list->Insert(row);
                }
            } catch (const fs::filesystem_error& e) {
            }
        }
    }
}

void FileDlg::UpdateDirectoryText()
{
    std::string str = s_working_dir.native_directory_string();
    const int H_SPACING = 10;
    while (m_font->TextExtent(str).x > Width() - 2 * H_SPACING) {
        unsigned int slash_idx = str.find('/', 1);
        unsigned int backslash_idx = str.find('\\', 1);
        if (slash_idx != std::string::npos) {
            slash_idx = str.find_first_not_of('/', slash_idx);
            str = "..." + str.substr(slash_idx);
        } else if (backslash_idx != std::string::npos) {
            backslash_idx = str.find_first_not_of('\\', backslash_idx);
            str = "..." + str.substr(backslash_idx);
        } else {
            break;
        }
    }
    *m_curr_dir_text << str;
    PlaceLabelsAndEdits(Width() / 4 - H_SPACING, m_files_edit->Height());
}

void FileDlg::OpenDirectory()
{
    // see if there is a directory selected; if so open the directory.  if more than one is selected, take the first one
    const std::set<int>& sels = m_files_list->Selections();
    std::string directory;
    if (!sels.empty()) {
        directory = m_files_list->GetRow(*sels.begin())[0]->WindowText();
        if (directory.size() < 2 || directory[0] != '[')
            return;
        directory = directory.substr(1, directory.size() - 2); // strip off '[' and ']'
        if (directory == "..") {
            if (s_working_dir.string() != s_working_dir.root_path().string() &&
                s_working_dir.branch_path().string() != "") {
                SetWorkingDirectory(s_working_dir.branch_path());
            } else {
                m_in_win32_drive_selection = true;
                m_files_edit->Clear();
                m_curr_dir_text->SetText("");
                PlaceLabelsAndEdits(Width() / 4 - H_SPACING, m_files_edit->Height());
                UpdateList();
            }
        } else {
            if (!m_in_win32_drive_selection) {
                SetWorkingDirectory(s_working_dir / directory);
            } else {
                m_in_win32_drive_selection = false;
                try {
                    SetWorkingDirectory(fs::path(directory + "\\", fs::native));
                } catch (const fs::filesystem_error& e) {
                    if (e.error() == fs::io_error) {
                        m_in_win32_drive_selection = true;
                        m_files_edit->Clear();
                        m_curr_dir_text->SetText("");
                        PlaceLabelsAndEdits(Width() / 4 - H_SPACING, m_files_edit->Height());
                        UpdateList();
                        boost::shared_ptr<ThreeButtonDlg> dlg(
                            GetStyleFactory()->NewThreeButtonDlg(175, 75, m_device_is_not_ready_str, m_font, m_color,
                                                                 m_border_color, m_color, m_text_color, 1));
                        dlg->Run();
                    } else {
                        throw;
                    }
                }
            }
        }
        if (m_save && m_ok_button->WindowText() != m_save_str)
            m_ok_button->SetText(m_save_str);
    }
}