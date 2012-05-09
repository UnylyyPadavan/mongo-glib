#include "test-helper.h"

#include <mongo-glib/mongo-glib.h>

static GMainLoop *gMainLoop;
static MongoClient *gClient;

static void
test1_count_cb (GObject      *object,
                GAsyncResult *result,
                gpointer      user_data)
{
   MongoCursor *cursor = (MongoCursor *)object;
   gboolean *success = user_data;
   GError *error = NULL;
   guint64 count = 0;

   *success = mongo_cursor_count_finish(cursor, result, &count, &error);
   g_assert_no_error(error);
   g_assert(*success);

   g_assert_cmpint(count, >, 0);

   g_main_loop_quit(gMainLoop);
}

static void
test1_connect_cb (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
   MongoCollection *col;
   MongoDatabase *db;
   MongoCursor *cursor;
   MongoBson *query = NULL;
   MongoBson *fields = NULL;
   gboolean ret;
   GError *error = NULL;

   ret = mongo_client_connect_finish(gClient, result, &error);
   g_assert_no_error(error);
   g_assert(ret);

   db = mongo_client_get_database(gClient, "dbtest1");
   g_assert(db);

   col = mongo_database_get_collection(db, "dbcollection1");
   g_assert(col);

   cursor = mongo_collection_find(col, query, fields, 0, 100, MONGO_QUERY_NONE);
   g_assert(cursor);

   mongo_cursor_count_async(cursor, NULL, test1_count_cb, user_data);
}

static void
test1 (void)
{
   gboolean success = FALSE;

   gClient = mongo_client_new();
   mongo_client_add_seed(gClient, "localhost", 27017);
   mongo_client_connect_async(gClient, NULL, test1_connect_cb, &success);

   g_main_loop_run(gMainLoop);

   g_assert_cmpint(success, ==, TRUE);
}

static gboolean
test2_foreach_func (MongoCursor *cursor,
                    MongoBson   *bson,
                    gpointer     user_data)
{
   gchar *str;
   guint *count = user_data;

   str = mongo_bson_to_string(bson, FALSE);
   g_print("%s\n", str);
   g_free(str);

   (*count)++;

   return TRUE;
}

static void
test2_foreach_cb (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
   gboolean ret;
   GError *error = NULL;

   ret = mongo_cursor_foreach_finish(MONGO_CURSOR(object), result, &error);
   g_assert_no_error(error);
   g_assert(ret);

   g_main_loop_quit(gMainLoop);
}

static void
test2_connect_cb (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
   MongoCollection *col;
   MongoDatabase *db;
   MongoCursor *cursor;
   MongoBson *query = NULL;
   MongoBson *fields = NULL;
   gboolean ret;
   GError *error = NULL;

   ret = mongo_client_connect_finish(gClient, result, &error);
   g_assert_no_error(error);
   g_assert(ret);

   db = mongo_client_get_database(gClient, "dbtest1");
   g_assert(db);

   col = mongo_database_get_collection(db, "dbcollection1");
   g_assert(col);

   cursor = mongo_collection_find(col, query, fields, 0, 100, MONGO_QUERY_NONE);
   g_assert(cursor);

   mongo_cursor_foreach_async(cursor,
                              test2_foreach_func,
                              user_data,
                              NULL,
                              NULL,
                              test2_foreach_cb,
                              user_data);
}

static void
test2 (void)
{
   guint count = 0;

   gClient = mongo_client_new();
   mongo_client_add_seed(gClient, "localhost", 27017);
   mongo_client_connect_async(gClient, NULL, test2_connect_cb, &count);

   g_main_loop_run(gMainLoop);

   g_assert_cmpint(count, >, 1);
   g_assert_cmpint(count, <=, 100);
}

gint
main (gint   argc,
      gchar *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_type_init();

   gMainLoop = g_main_loop_new(NULL, FALSE);

   g_test_add_func("/MongoCursor/count", test1);
   g_test_add_func("/MongoCursor/foreach", test2);

   return g_test_run();
}
