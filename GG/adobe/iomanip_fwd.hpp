/*
    Copyright 2005-2007 Adobe Systems Incorporated
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
*/

/*************************************************************************************************/

#ifdef ADOBE_STD_SERIALIZATION

/*************************************************************************************************/

#ifndef ADOBE_IOMANIP_FWD_HPP
#define ADOBE_IOMANIP_FWD_HPP

/*************************************************************************************************/

#include <GG/adobe/config.hpp>

#include <iosfwd>

/*************************************************************************************************/

namespace adobe {

/*************************************************************************************************/

class format_base;

/*************************************************************************************************/

int format_base_idx();

format_base* get_formatter(std::ostream& os);

/*************************************************************************************************/

} // namespace adobe

/*************************************************************************************************/

#endif

/*************************************************************************************************/

#endif

/*************************************************************************************************/
