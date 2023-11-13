/**
 * @file mvk.mm
 */

#if defined(WYN_COCOA)

#include "mvk.hpp"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
// #include <vulkan/vulkan_metal.h>

namespace cge::mvk
{
    extern void* create_layer(void* const window)
    {
        NSWindow* const ns_window{ static_cast<NSWindow*>(window) };
        NSView* const ns_view{ [ns_window contentView] };
        ns_view.wantsLayer = YES;
        return static_cast<void*>(ns_view.layer = reinterpret_cast<CALayer*>([CAMetalLayer layer]));
    }
}

#endif
