#import <Cocoa/Cocoa.h>
#include <base/system.h>

extern int TWMain(int argc, const char **argv);

int main(int argc, const char **argv)
{
	BOOL FinderLaunch = argc >= 2 && !str_comp_num(argv[1], "-psn", 4);

	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	if(!resourcePath)
		return -1;

	[[NSFileManager defaultManager] changeCurrentDirectoryPath:resourcePath];

	if(FinderLaunch)
	{
		const char *paArgv[2] = { argv[0], NULL };
		return TWMain(1, paArgv);
	}
	else
		return TWMain(argc, argv);
}
