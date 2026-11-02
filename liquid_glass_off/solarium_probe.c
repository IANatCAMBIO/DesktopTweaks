/*
 * solarium_probe — report whether Liquid Glass ("Solarium") is active in this
 * process, optionally after loading an Ammonia tweak the way Ammonia's own
 * opener does (fridagum.dylib + LoadFunction). Lets the tweak be verified
 * without root, without touching Ammonia's tweak directory and without
 * relaunching any app.
 *
 * Usage:
 *   solarium_probe                 baseline for an untouched process
 *   solarium_probe <tweak.dylib>   same, with the tweak loaded first
 */
#include <dlfcn.h>
#include <stdio.h>

#define GUM_PATH "/private/var/ammonia/core/fridagum.dylib"

typedef int (*bool_fn)(void);
typedef int (*flag_fn)(const char *domain, const char *feature);
typedef void (*gum_init_fn)(void);
typedef void *(*gum_obtain_fn)(void);
typedef void (*load_fn)(void *interceptor);

/*
 * load_tweak — mimic Ammonia's opener: initialise frida-gum and hand the
 * interceptor to the tweak's LoadFunction.
 *
 * Inputs:
 *   path — filesystem path of the tweak dylib
 *
 * Output:
 *   0 on success, -1 if frida-gum or the tweak could not be loaded.
 */
static int
load_tweak(const char *path)
{
    void *gum = dlopen(GUM_PATH, RTLD_NOW | RTLD_GLOBAL);
    gum_init_fn gum_init = gum ? dlsym(gum, "gum_init_embedded") : NULL;
    gum_obtain_fn gum_obtain = gum ? dlsym(gum, "gum_interceptor_obtain") : NULL;
    void *tweak;
    load_fn load;

    if (gum_init == NULL || gum_obtain == NULL) {
        fprintf(stderr, "cannot load %s: %s\n", GUM_PATH, dlerror());
        return -1;
    }
    gum_init();

    tweak = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    load = tweak ? dlsym(tweak, "LoadFunction") : NULL;
    if (load == NULL) {
        fprintf(stderr, "cannot load tweak %s: %s\n", path, dlerror());
        return -1;
    }
    load(gum_obtain());
    return 0;
}

/*
 * print_probe — print one labelled probe result.
 *
 * Inputs:
 *   label — human-readable name of the probe
 *   value — 1 = enabled, 0 = disabled, -1 = symbol not found
 *
 * Output:
 *   none; writes one line to stdout.
 */
static void
print_probe(const char *label, int value)
{
    if (value < 0) {
        printf("%-32s unavailable\n", label);
    } else {
        printf("%-32s %s\n", label, value ? "ON" : "OFF");
    }
}

/*
 * main — optionally load a tweak, then ask the feature-flag library, AppKit
 * and SwiftUI whether Liquid Glass is on, and print the three answers.
 *
 * Inputs:
 *   argv[1] — optional path to a tweak dylib
 *
 * Output:
 *   0 when every probe resolved, 1 on any failure.
 */
int
main(int argc, char **argv)
{
    void *appkit;
    void *swiftui;
    flag_fn feature_flag;
    bool_fn appkit_enabled;
    bool_fn swiftui_enabled;
    int flag, appkit_state, swiftui_state;

    if (argc > 1 && load_tweak(argv[1]) != 0) {
        return 1;
    }

    appkit = dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_NOW);
    swiftui = dlopen("/System/Library/Frameworks/SwiftUI.framework/SwiftUI", RTLD_NOW);
    feature_flag = dlsym(RTLD_DEFAULT, "_os_feature_enabled_impl");
    appkit_enabled = appkit ? dlsym(appkit, "NSSolariumEnabled") : NULL;
    swiftui_enabled = swiftui ? dlsym(swiftui, "$s7SwiftUI8SolariumVAAE9isEnabledSbvgZ") : NULL;

    flag = feature_flag ? (feature_flag("SwiftUI", "Solarium") & 1) : -1;
    appkit_state = appkit_enabled ? (appkit_enabled() & 1) : -1;
    swiftui_state = swiftui_enabled ? (swiftui_enabled() & 1) : -1;

    print_probe("feature flag SwiftUI/Solarium", flag);
    print_probe("AppKit NSSolariumEnabled", appkit_state);
    print_probe("SwiftUI Solarium.isEnabled", swiftui_state);

    return (flag < 0 || appkit_state < 0 || swiftui_state < 0) ? 1 : 0;
}
