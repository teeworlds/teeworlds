#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

@interface ServerView : NSTextView
{
	NSFileHandle *file;
}
- (void)listenTo: (NSFileHandle*)f;
@end

@implementation ServerView
- (void)listenTo: (NSFileHandle*)f;
{
	file = f;

	[[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(outputNotification:) name: NSFileHandleReadCompletionNotification object: file];

	[file readInBackgroundAndNotify];
}

- (void) outputNotification: (NSNotification *) notification
{
	NSData *data = [[[notification userInfo] objectForKey: NSFileHandleNotificationDataItem] retain];
	NSString *string = [[NSString alloc] initWithData: data encoding: NSASCIIStringEncoding];

	NSRange end = NSMakeRange([[self string] length], 0);

	[self replaceCharactersInRange: end withString: string];
	end.location += [string length];
	[self scrollRangeToVisible: end];

	[string release];
	[file readInBackgroundAndNotify];
}

-(void)windowWillClose:(NSNotification *)notification
{
    [NSApp terminate:self];
}
@end

int main(int argc, char **argv)
{
	UInt32 mod = GetCurrentKeyModifiers();

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSApp = [NSApplication sharedApplication];
	NSBundle* mainBundle = [NSBundle mainBundle];
	NSTask *task;
    task = [[NSTask alloc] init];
	[task setCurrentDirectoryPath: [mainBundle resourcePath]]; 
	NSPipe *pipe;
	NSFileHandle *file;
    pipe = [NSPipe pipe];
    [task setStandardOutput: pipe];
    file = [pipe fileHandleForReading];


	if(mod & optionKey)
	{
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

		[window setTitle: @"Teewars Server"];

		view = [[[ServerView alloc] initWithFrame: graphicsRect] autorelease];
		[view setEditable: NO];

		[window setContentView: view];
		[window setDelegate: view];
		[window makeKeyAndOrderFront: nil];

		[view listenTo: file];
		[task setLaunchPath: [mainBundle pathForAuxiliaryExecutable: @"teewars_srv"]];
		[task launch];
		[NSApp run];
		[task terminate];
	}
	else
	{
		// run client
		[task setLaunchPath: [mainBundle pathForAuxiliaryExecutable: @"teewars"]];
		[task launch];
		[task waitUntilExit];
	}



    [NSApp release];
    [pool release];
    return(EXIT_SUCCESS);
}
