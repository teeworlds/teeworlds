/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/datafile.h>

int main(int argc, char **argv)
{
	int i, id, type, size;
	void *ptr;
	DATAFILE *df;
	DATAFILE_OUT *df_out;

	if(argc != 3)
		return -1;

	df = datafile_load(argv[1]);
	df_out = datafile_create(argv[2]);

	/* add all items */
	for(i = 0; i < datafile_num_items(df); i++)
	{
		ptr = datafile_get_item(df, i, &type, &id);
		size = datafile_get_itemsize(df, i);
		datafile_add_item(df_out, type, id, size, ptr);
	}

	/* add all data */
	for(i = 0; i < datafile_num_data(df); i++)
	{
		ptr = datafile_get_data(df, i);
		size = datafile_get_datasize(df, i);
		datafile_add_data(df_out, size, ptr);
	}

	datafile_unload(df);
	datafile_finish(df_out);
	return 0;
}
