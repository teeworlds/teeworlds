
struct snapshot
{
	int num_items;
	int offsets[1];

	struct item
	{
		int type_and_id;
		char data[1];
		
		int type() { return type_and_id>>16; }
		int id() { return type_and_id&(0xffff); }
	};
	
	char *data_start() { return (char *)&offsets[num_items]; }
	item *get_item(int index) { return (item *)(data_start() + offsets[index]); };
};

