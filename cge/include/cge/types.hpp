/**
 * @file cge/types.hpp
 */

#pragma once

#ifndef CGE_TYPES_H
#define CGE_TYPES_H

namespace cge
{
    using Coord = double;

    using Extent = double;

    struct Point
    {
        Coord x, y;
    };
    
    struct Size
    {
        Extent w, h;
    };
    
    struct Rect
    {
        Point origin;
        Size size;
    };
}

#endif
