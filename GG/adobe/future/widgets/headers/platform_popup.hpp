/*
    Copyright 2005-2007 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/****************************************************************************************************/

#ifndef ADOBE_UI_CORE_POPUP_HPP
#define ADOBE_UI_CORE_POPUP_HPP

/****************************************************************************************************/

#include <GG/adobe/config.hpp>

#include <GG/Clr.h>
#include <GG/adobe/any_regular.hpp>
#include <GG/adobe/dictionary.hpp>
#include <GG/adobe/future/widgets/headers/platform_label.hpp>
#include <GG/adobe/future/widgets/headers/widget_utils.hpp>
#include <GG/adobe/future/widgets/headers/popup_common_fwd.hpp>

#include <boost/noncopyable.hpp>


namespace GG {
    class DropDownList;
}

/****************************************************************************************************/

namespace adobe {

/****************************************************************************************************/

struct popup_t : boost::noncopyable
{
    typedef dictionary_t               menu_item_t;
    typedef std::vector<menu_item_t>   menu_item_set_t;

    typedef any_regular_t              model_type;
    typedef popup_setter_type          setter_type;
    typedef popup_extended_setter_type extended_setter_type;

    popup_t(const std::string& name,
            const std::string& alt_text,
            int max_characters,
            const std::string& custom_item_name,
            const menu_item_t* first,
            const menu_item_t* last,
            GG::Clr            color,
            GG::Clr            label_color,
            GG::Clr            item_text_color,
            name_t             signal_id);

    void                reset_menu_item_set(const menu_item_t* first, const menu_item_t* last);

    inline void         reset_menu_item_set(const menu_item_set_t& menu_item_set)
        {
            if (menu_item_set.empty())
                reset_menu_item_set(0, 0);
            else
                reset_menu_item_set(&menu_item_set[0], &menu_item_set[0] + menu_item_set.size());
        }

    void                select_with_text(const std::string& text);

    void                measure(extents_t& result);

    void                place(const place_data_t& place_data);

    void                display(const any_regular_t& item);

    void                monitor(const setter_type& proc);

    void                monitor_extended(const extended_setter_type& proc);

    void                enable(bool make_enabled);

    void                set_item_text_color(GG::Clr color);

    GG::DropDownList*    control_m;
    unsigned int         original_height_m;
    theme_t              theme_m;
    label_t              name_m;
    std::string          alt_text_m;
    int                  max_characters_m;
    bool                 using_label_m;
    setter_type          value_proc_m;
    setter_type          selection_changed_proc_m;
    extended_setter_type extended_value_proc_m;
    menu_item_set_t      menu_items_m;
    GG::Clr              color_m;
    GG::Clr              item_text_color_m;
    name_t               signal_id_m;
    model_type           last_m;
    bool                 custom_m;
    std::string          custom_item_name_m;
    const row_factory_t* row_factory_m;

    implementation::color_proxy_t color_proxy_m;
    implementation::color_proxy_t item_text_color_proxy_m;

private:
    void display_custom();
};

/****************************************************************************************************/

namespace view_implementation {

/****************************************************************************************************/

inline void set_value_from_model(popup_t& value, const any_regular_t& new_value)
{ value.display(new_value); }

/****************************************************************************************************/

} // namespace view_implementation

/****************************************************************************************************/

} // namespace adobe

/****************************************************************************************************/

#endif

/****************************************************************************************************/
