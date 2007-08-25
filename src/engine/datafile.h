
/* raw datafile access */
typedef struct DATAFILE_t DATAFILE;

/* read access */
DATAFILE *datafile_load(const char *filename);
DATAFILE *datafile_load_old(const char *filename);
void *datafile_get_data(DATAFILE *df, int index); // automaticly load the data for the item
int datafile_get_datasize(DATAFILE *df, int index);
void datafile_unload_data(DATAFILE *df, int index);
void *datafile_get_item(DATAFILE *df, int index, int *type, int *id);
int datafile_get_itemsize(DATAFILE *df, int index);
void datafile_get_type(DATAFILE *df, int type, int *start, int *num);
void *datafile_find_item(DATAFILE *df, int type, int id);
int datafile_num_items(DATAFILE *df);
int datafile_num_data(DATAFILE *df);
void datafile_unload(DATAFILE *df);

/* write access */
typedef struct DATAFILE_OUT_t DATAFILE_OUT;
DATAFILE_OUT *datafile_create(const char *filename);
int datafile_add_data(DATAFILE_OUT *df, int size, void *data);
int datafile_add_item(DATAFILE_OUT *df, int type, int id, int size, void *data);
int datafile_finish(DATAFILE_OUT *df);
