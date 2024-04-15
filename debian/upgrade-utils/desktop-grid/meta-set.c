#include "metatree.h"
#include <glib/gstdio.h>
#include <gio/gio.h>

static gboolean unset = FALSE;
static gboolean list = FALSE;
static gboolean use_dbus = FALSE;
static char *treename = NULL;
static GOptionEntry entries[] =
{
  { "tree", 't', 0, G_OPTION_ARG_STRING, &treename, "Tree", NULL},
  { "unset", 'u', 0, G_OPTION_ARG_NONE, &unset, "Unset", NULL },
  { "list", 'l', 0, G_OPTION_ARG_NONE, &list, "Set as list", NULL },
  { NULL }
};

int
main (int argc,
      char *argv[])
{
  MetaTree *tree;
  GError *error = NULL;
  GOptionContext *context;
  MetaLookupCache *lookup;
  struct stat statbuf;
  const char *path, *key;
  const char *metatreefile;
  char *tree_path;
  
  context = g_option_context_new ("<path> <key> <value> - set metadata");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("option parsing failed: %s\n", error->message);
      return 1;
    }

  if (argc < 2)
    {
      g_printerr ("no path specified\n");
      return 1;
    }
  path = argv[1];

  if (argc < 3)
    {
      g_printerr ("no key specified\n");
      return 1;
    }
  key = argv[2];

  if (!list && !unset && argc != 4)
    {
      g_print ("No value specified\n");
      return 1;
    }

  if (treename)
    {
      tree = meta_tree_lookup_by_name (treename, TRUE);
      if (tree)
	tree_path = g_strdup (path);

      if (tree == NULL)
	{
	  g_printerr ("can't open metadata tree %s\n", path);
	  return 1;
	}
    }
  else
    {
      lookup = meta_lookup_cache_new ();
      if (g_lstat (path, &statbuf) != 0)
	{
	  g_printerr ("can't find file %s\n", path);
	  return 1;
	}
      tree = meta_lookup_cache_lookup_path (lookup,
					    path,
					    statbuf.st_dev,
					    TRUE,
					    &tree_path);
      meta_lookup_cache_free (lookup);

      if (tree == NULL)
	{
	  g_printerr ("can't open metadata tree for file %s\n", path);
	  return 1;
	}
    }

  if (unset)
    {
	{
	  if (!meta_tree_unset (tree, tree_path, key))
	    {
	      g_printerr ("Unable to unset key\n");
	      return 1;
	    }
	}
    }
  else if (list)
    {
	{
	  if (!meta_tree_set_stringv (tree, tree_path, key, &argv[3]))
	    {
	      g_printerr ("Unable to set key\n");
	      return 1;
	    }
	}
    }
  else
    {
	{
	  if (!meta_tree_set_string (tree, tree_path, key, argv[3]))
	    {
	      g_printerr ("Unable to set key\n");
	      return 1;
	    }
	}
    }

  return 0;
}
