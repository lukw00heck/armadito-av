#include "libuhuru-config.h"

#include <libuhuru/core.h>

#include "modulep.h"
#include "statusp.h"
#include "uhurup.h"
#include "os/string.h"
#include "os/mimetype.h"
#include "os/string.h"
#include "os/dir.h"
#include "builtin-modules/ondemandmod.h"
#ifdef HAVE_ALERT_MODULE
#include "builtin-modules/alert.h"
#endif
#ifdef HAVE_QUARANTINE_MODULE
#include "builtin-modules/quarantine.h"
#endif
#ifdef HAVE_ON_ACCESS_WINDOWS_MODULE
#include "builtin-modules/onaccess_windows.h"
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct uhuru {
  struct module_manager *module_manager;
};

static struct uhuru *uhuru_new(void)
{
  struct uhuru *u = g_new(struct uhuru, 1);

  u->module_manager = module_manager_new(u);

  return u;
}

static void uhuru_free(struct uhuru *u)
{
  module_manager_free(u->module_manager);
  g_free(u);
}

static void uhuru_add_builtin_modules(struct uhuru *u)
{
  module_manager_add(u->module_manager, &on_demand_module);

#ifdef HAVE_ON_ACCESS_WINDOWS_MODULE
  module_manager_add(u->module_manager, &on_access_win_module);
#endif
#ifdef HAVE_ALERT_MODULE
  module_manager_add(u->module_manager, &alert_module);
#endif
#ifdef HAVE_QUARANTINE_MODULE
  module_manager_add(u->module_manager, &quarantine_module);
#endif
}

struct uhuru *uhuru_open(uhuru_error **error)
{
  struct uhuru *u;
  const char *modules_dir;

#ifdef HAVE_GTHREAD_INIT
  g_thread_init(NULL);
#endif

  os_mime_type_init();

  u = uhuru_new();

  uhuru_add_builtin_modules(u);

  modules_dir = uhuru_std_path(MODULES_LOCATION);
  if (modules_dir == NULL)
    goto error;
  if (module_manager_load_path(u->module_manager, modules_dir, error))
    goto error;
  free((void *)modules_dir);

  if (module_manager_init_all(u->module_manager, error))
    goto error;

  // apply conf to all modules
  // TODO

  if (module_manager_post_init_all(u->module_manager, error))
    goto error;

  return u;

 error:
  uhuru_free(u);
  return NULL;
}

struct uhuru_module **uhuru_get_modules(struct uhuru *u)
{
  return module_manager_get_modules(u->module_manager);
}

struct uhuru_module *uhuru_get_module_by_name(struct uhuru *u, const char *module_name)
{
  struct uhuru_module **modv;

  for (modv = module_manager_get_modules(u->module_manager); *modv != NULL; modv++)
    if (!strcmp((*modv)->name, module_name))
      return *modv;

  return NULL;
}

int uhuru_close(struct uhuru *u, uhuru_error **error)
{
  return module_manager_close_all(u->module_manager, error);
}

#ifdef DEBUG
static void mod_print_name(gpointer data, gpointer user_data)
{
  struct uhuru_module *mod = (struct uhuru_module *)data;

  printf("%s ", mod->name);
}

const char *uhuru_debug(struct uhuru *u)
{
  struct uhuru_module **modv;
  GString *s = g_string_new("");
  const char *ret;

  g_string_append_printf(s, "Uhuru:\n");

  for (modv = module_manager_get_modules(u->module_manager); *modv != NULL; modv++)
    g_string_append_printf(s, "%s\n", module_debug(*modv));

  g_string_append_printf(s, "  mime types:\n");

  ret = s->str;
  g_string_free(s, FALSE);

  return ret;
}
#endif
