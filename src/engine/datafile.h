
// raw datafile access
struct datafile;

// read access
datafile *datafile_load(const char *filename);
datafile *datafile_load_old(const char *filename);
void *datafile_get_data(datafile *df, int index);
void *datafile_get_item(datafile *df, int index, int *type, int *id);
void datafile_get_type(datafile *df, int type, int *start, int *num);
void *datafile_find_item(datafile *df, int type, int id);
int datafile_num_items(datafile *df);
void datafile_unload(datafile *df);

// write access
struct datafile_out;
datafile_out *datafile_create(const char *filename);
int datafile_add_data(datafile_out *df, int size, void *data);
int datafile_add_item(datafile_out *df, int type, int id, int size, void *data);
int datafile_finish(datafile_out *df);
