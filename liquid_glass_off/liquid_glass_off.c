/*
 * liquid_glass_off — Ammonia tweak that switches Liquid Glass ("Solarium")
 * off inside the process it is injected into, and nowhere else.
 *
 * AppKit and SwiftUI both decide whether to draw Liquid Glass by asking
 * libsystem_featureflags for the SwiftUI/Solarium flag, once, early in
 * process start-up. Ammonia loads this library from DYLD_INSERT_LIBRARIES
 * before main() runs, so replacing that one query here is enough to make the
 * whole process render the pre-Tahoe look. No file on disk is changed and no
 * other process is affected; the tweak's .whitelist decides where it loads.
 */
#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

typedef bool (*feature_enabled_fn)(const char *domain, const char *feature);
typedef int (*gum_replace_fn)(void *interceptor, void *target, void *replacement, void *user_data, void **out_original);
typedef void (*gum_transaction_fn)(void *interceptor);

static feature_enabled_fn original_feature_enabled;

/*
 * feature_enabled_hook — replacement for _os_feature_enabled_impl.
 *
 * Inputs:
 *   domain  — feature-flag domain name, e.g. "SwiftUI"
 *   feature — feature name inside that domain, e.g. "Solarium"
 *
 * Output:
 *   false for SwiftUI/Solarium; whatever the real implementation returns for
 *   every other flag.
 */
static bool
feature_enabled_hook(const char *domain, const char *feature)
{
    if (domain != NULL && feature != NULL &&
        strcmp(domain, "SwiftUI") == 0 && strcmp(feature, "Solarium") == 0) {
        return false;
    }
    return original_feature_enabled(domain, feature);
}

/*
 * LoadFunction — Ammonia entry point, called right after the tweak is loaded.
 *
 * Inputs:
 *   interceptor — the frida-gum GumInterceptor that Ammonia hands to tweaks
 *
 * Output:
 *   none; logs to syslog and leaves the process untouched if the hook cannot
 *   be installed.
 */
void
LoadFunction(void *interceptor)
{
    gum_replace_fn gum_replace = dlsym(RTLD_DEFAULT, "gum_interceptor_replace");
    gum_transaction_fn gum_begin = dlsym(RTLD_DEFAULT, "gum_interceptor_begin_transaction");
    gum_transaction_fn gum_end = dlsym(RTLD_DEFAULT, "gum_interceptor_end_transaction");
    void *target = dlsym(RTLD_DEFAULT, "_os_feature_enabled_impl");
    void *original = NULL;
    int rc;

    if (interceptor == NULL || gum_replace == NULL || gum_begin == NULL || gum_end == NULL) {
        syslog(LOG_ERR, "liquid_glass_off: frida-gum interceptor unavailable; not hooking");
        return;
    }
    if (target == NULL) {
        syslog(LOG_ERR, "liquid_glass_off: _os_feature_enabled_impl not found; not hooking");
        return;
    }

    gum_begin(interceptor);
    rc = gum_replace(interceptor, target, (void *)feature_enabled_hook, NULL, &original);
    gum_end(interceptor);

    if (rc != 0 || original == NULL) {
        syslog(LOG_ERR, "liquid_glass_off: gum_interceptor_replace failed (%d)", rc);
        return;
    }
    original_feature_enabled = (feature_enabled_fn)original;
}
