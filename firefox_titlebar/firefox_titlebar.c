/*
 * firefox_titlebar — Ammonia tweak that centers the window title in Firefox
 * browser windows on macOS 26.
 *
 * Firefox draws its own tab strip and toolbars but leaves the titlebar to
 * AppKit. macOS 26 leading-aligns that title whenever Liquid Glass is on and
 * the window has neither a Preference-style toolbar nor a leading titlebar
 * accessory; the decision is made in -[NSThemeFrame _defaultTitlebarTitleRect].
 * This tweak replaces that one method so Firefox browser windows (Gecko's
 * ToolbarWindow class) get AppKit's own centered rect, and every other window
 * in the process keeps the stock answer. Nothing else about the window is
 * touched: the style mask, the full-size content view and Gecko's own titlebar
 * padding stay exactly as Firefox set them. Clearing the full-size-content
 * flag, as a generic classic-titlebar tweak would, leaves Gecko's reserved
 * titlebar strip visible as a blank band under the real titlebar.
 */
#include <CoreGraphics/CoreGraphics.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <syslog.h>

typedef CGRect (*rect_method_fn)(id self, SEL cmd);
typedef id (*id_method_fn)(id self, SEL cmd);
typedef void (*void_method_fn)(id self, SEL cmd);
typedef BOOL (*class_test_fn)(id self, SEL cmd, Class cls);
typedef unsigned long (*count_method_fn)(id self, SEL cmd);
typedef id (*index_method_fn)(id self, SEL cmd, unsigned long index);

static rect_method_fn original_title_rect;

/*
 * is_browser_window — decide whether a window is a Firefox browser window.
 *
 * Inputs:
 *   window — an NSWindow, or NULL
 *
 * Output:
 *   true when the window is an instance of Gecko's ToolbarWindow class, the
 *   class Firefox uses for its browser windows; false for NULL, for Gecko's
 *   popup and borderless windows, and when the class is not loaded yet.
 */
static bool
is_browser_window(id window)
{
    Class toolbar_window = objc_getClass("ToolbarWindow");

    if (window == NULL || toolbar_window == NULL) {
        return false;
    }
    return ((class_test_fn)objc_msgSend)(window, sel_registerName("isKindOfClass:"), toolbar_window);
}

/*
 * title_rect_hook — replacement for -[NSThemeFrame _defaultTitlebarTitleRect].
 *
 * Inputs:
 *   self — the NSThemeFrame asking where its title belongs
 *   cmd  — the selector being invoked
 *
 * Output:
 *   AppKit's centered title rect when the frame belongs to a Firefox browser
 *   window; the original method's answer for every other window.
 */
static CGRect
title_rect_hook(id self, SEL cmd)
{
    id window = ((id_method_fn)objc_msgSend)(self, sel_registerName("window"));

    if (is_browser_window(window)) {
        return ((rect_method_fn)objc_msgSend)(self, sel_registerName("_defaultCenteredTitlebarTitleRect"));
    }
    return original_title_rect(self, cmd);
}

/*
 * recenter_open_windows — re-run AppKit's title layout for browser windows
 * that already exist.
 *
 * Ammonia loads the tweak before main() runs, so under normal use there are
 * none; this covers loading the tweak into a Firefox that is already running,
 * which is how it is verified before installation.
 *
 * Inputs:  none (must run on the main thread)
 * Output:  none
 */
static void
recenter_open_windows(void)
{
    id app = ((id_method_fn)objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    id windows = ((id_method_fn)objc_msgSend)(app, sel_registerName("windows"));
    unsigned long count = ((count_method_fn)objc_msgSend)(windows, sel_registerName("count"));

    for (unsigned long i = 0; i < count; i++) {
        id window = ((index_method_fn)objc_msgSend)(windows, sel_registerName("objectAtIndex:"), i);
        id content_view;
        id theme_frame;

        if (!is_browser_window(window)) {
            continue;
        }
        content_view = ((id_method_fn)objc_msgSend)(window, sel_registerName("contentView"));
        theme_frame = ((id_method_fn)objc_msgSend)(content_view, sel_registerName("superview"));
        if (theme_frame != NULL) {
            ((void_method_fn)objc_msgSend)(theme_frame, sel_registerName("windowTitlePositionChanged"));
        }
    }
}

/*
 * firefox_titlebar_init — library constructor; installs the hook.
 *
 * Inputs:  none
 * Output:  none; logs to syslog and leaves the process untouched if AppKit's
 *          private title-rect methods are not present.
 */
__attribute__((constructor)) static void
firefox_titlebar_init(void)
{
    Class theme_frame = objc_getClass("NSThemeFrame");
    Method target = NULL;

    if (theme_frame != NULL) {
        target = class_getInstanceMethod(theme_frame, sel_registerName("_defaultTitlebarTitleRect"));
    }
    if (target == NULL ||
        class_getInstanceMethod(theme_frame, sel_registerName("_defaultCenteredTitlebarTitleRect")) == NULL) {
        syslog(LOG_ERR, "firefox_titlebar: NSThemeFrame title-rect methods not found; not hooking");
        return;
    }

    original_title_rect = (rect_method_fn)method_setImplementation(target, (IMP)title_rect_hook);
    dispatch_async(dispatch_get_main_queue(), ^{
        recenter_open_windows();
    });
}
