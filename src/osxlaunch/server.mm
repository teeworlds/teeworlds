#import <Cocoa/Cocoa.h>

@interface ServerView : NSTextView
{
	NSTask *task;
	NSFileHandle *file;
}
- (void)listenTo: (NSTask*)t;
@end

@implementation ServerView
- (void)listenTo: (NSTask*)t;
{
	NSPipe *pipe;
	task = t;
	pipe = [NSPipe pipe];
	[task setStandardOutput: pipe];
	file = [pipe fileHandleForReading];

	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(outputNotification:) name: NSFileHandleReadCompletionNotification object: file];

	[file readInBackgroundAndNotify];
}

- (void) outputNotification: (NSNotification *) notification
{
	NSData *data = [[[notification userInfo] objectForKey: NSFileHandleNotificationDataItem] retain];
	NSString *string = [[NSString alloc] initWithData: data encoding: NSASCIIStringEncoding];
	NSAttributedString *attrstr = [[NSAttributedString alloc] initWithString: string];

	[[self textStorage] appendAttributedString: attrstr];
	int length = [[self textStorage] length];
	NSRange range = NSMakeRange(length, 0);
	[self scrollRangeToVisible: range];

	[attrstr release];
	[string release];
	[file readInBackgroundAndNotify];
}

-(void)windowWillClose:(NSNotification *)notification
{
	[task terminate];
	[NSApp terminate:self];
}
@end

void runServer()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSApp = [NSApplication sharedApplication];
	NSBundle* mainBundle = [NSBundle mainBundle];
	NSTask *task;
	task = [[NSTask alloc] init];
	[task setCurrentDirectoryPath: [mainBundle resourcePath]];

	// get a server config
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setCanChooseFiles:YES];

	if([openDlg runModalForDirectory:nil file:nil] != NSOKButton)
		return;

	NSArray* filenames = [openDlg filenames];
	if([filenames count] != 1)
		return;

	NSString* filename = [filenames objectAtIndex: 0];
	NSArray* arguments = [NSArray arrayWithObjects: @"-f", filename, nil];

	// run server
	NSWindow *window;
	ServerView *view;
	NSRect graphicsRect;

	graphicsRect = NSMakeRect(100.0, 1000.0, 600.0, 400.0);

	window = [[NSWindow alloc]
		initWithContentRect: graphicsRect
		styleMask: NSTitledWindowMask
		| NSClosableWindowMask
		| NSMiniaturizableWindowMask
		backing: NSBackingStoreBuffered
		defer: NO];

	[window setTitle: @"Teeworlds Server"];

	view = [[[ServerView alloc] initWithFrame: graphicsRect] autorelease];
	[view setEditable: NO];
	[view setRulerVisible: YES];

	[window setContentView: view];
	[window setDelegate: (id<NSWindowDelegate>)view];
	[window makeKeyAndOrderFront: nil];

	[view listenTo: task];
	[task setLaunchPath: [mainBundle pathForAuxiliaryExecutable: @"teeworlds_srv"]];
	[task setArguments: arguments];
	[task launch];
	[NSApp run];
	[task terminate];

	[NSApp release];
	[pool release];
}

int main (int argc, char **argv)
{
	runServer();

	return 0;
}
