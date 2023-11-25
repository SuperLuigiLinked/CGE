/**
 * @file cge/cvk.mm
 */

#if defined(WYN_COCOA)

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

namespace cvk
{
    extern void* create_metal_layer(void* ns_view) noexcept;
    extern double backing_scale(void* ns_window) noexcept;
}

namespace cvk
{
    void* create_metal_layer(void* ns_view) noexcept
    {
        NSView* const view{ static_cast<NSView*>(ns_view) };
        view.wantsLayer = YES;
        view.layer = [CAMetalLayer layer];
        return static_cast<void*>(view.layer);
    }

    double backing_scale(void* ns_window) noexcept
    {
        // https://developer.apple.com/documentation/appkit/nswindow/1419459-backingscalefactor?language=objc
        NSWindow* const window{ static_cast<NSWindow*>(ns_window) };
        const CGFloat scale{ [window backingScaleFactor]};
        return static_cast<double>(scale);
    }
}

#endif
