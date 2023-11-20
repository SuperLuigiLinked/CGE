/**
 * @file cge/cvk.mm
 */

#if defined(WYN_COCOA)

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

namespace cvk
{
    extern void* create_metal_layer(void* ns_view);
}

namespace cvk
{
    void* create_metal_layer(void* ns_view)
    {
        NSView* const view{ static_cast<NSView*>(ns_view) };
        view.wantsLayer = YES;
        view.layer = [CAMetalLayer layer];
        return static_cast<void*>(view.layer);
    }
}

#endif
