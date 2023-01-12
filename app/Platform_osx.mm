#include "Platform.hpp"
#include "../engine/core/Log.hpp"
#include "../engine/core/Text.hpp"

#include <pwd.h>
#include <unistd.h>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UTType.h>


@interface MacMenuItem : NSMenuItem 
@property sns::MenuCallback callback;
@property sns::MenuItem item;
- (id)initWithOptions:(sns::MenuItem) item callback:(sns::MenuCallback) callback;
- (void)menuActionPicked:(NSMenuItem *)item;
@end

@implementation MacMenuItem

- (id)initWithOptions:(sns::MenuItem) item callback:(sns::MenuCallback) callback;
{
    NSString *title = [NSString stringWithCString:item.name.c_str() encoding:[NSString defaultCStringEncoding]];
    NSString *key_equivalent = @"";
    NSEventModifierFlags mask = 0;

    if (!item.shortcut.empty()) {
        std::string key = item.shortcut;

        if (sns::startsWith(key, "Opt+")) {
            sns::replace(key, "Opt+", "");
            mask |= NSEventModifierFlagOption;
        }

        if (sns::startsWith(key, "Ctrl+")) {
            sns::replace(key, "Ctrl+", "");
            mask |= NSEventModifierFlagControl;
        }

        if (sns::startsWith(key, "Cmd+")) {
            sns::replace(key, "Cmd+", "");
            mask |= NSEventModifierFlagCommand;
        }

        if (sns::startsWith(key, "Shift+")) {
            sns::replace(key, "Shift+", "");
            mask |= NSEventModifierFlagShift;
        }

        sns::lowercase(key);

        key_equivalent = [NSString stringWithCString:key.c_str() encoding:[NSString defaultCStringEncoding]];
    }


    self = [super initWithTitle:title action:@selector(menuActionPicked:) 
                  keyEquivalent: key_equivalent];
    if (self) {
        [self setKeyEquivalentModifierMask: mask];

        [self setTarget:self];
        self.callback = callback;
        self.item = item;
    }
    return self;
}

- (void)menuActionPicked:(NSMenuItem *)item;{
    if (item == self)
        self.callback(self.item.command);
}
@end


namespace sns {
	
    std::string platformLocalFolder(std::string const& app) {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *folder_path = [paths firstObject];
        std::string base_folder = std::string([folder_path UTF8String], [folder_path lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
        return mergePaths(base_folder, app);
    }
	
	void platformShellOpen(std::string const& cmd) {
        std::string open_command = "open \"" + cmd + "\"";
        system(open_command.c_str());

    }	
	
    void platformPickLoadFile(std::string const& std_title, std::string const& std_message, std::string const& std_ext,
                              std::function<void(std::string const&)> callback) {
        NSString *folder_path = [NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) lastObject];

        NSString *message = [NSString stringWithCString:std_message.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString *title = [NSString stringWithCString:std_title.c_str() encoding:[NSString defaultCStringEncoding]];

        NSString *ext = [NSString stringWithCString:std_ext.c_str() encoding:[NSString defaultCStringEncoding]];

        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setMessage: message]; 
        [panel setTitle: title];
        [panel setDirectoryURL:[NSURL fileURLWithPath:folder_path]];

        [panel setAllowsOtherFileTypes:NO];
        [panel setExtensionHidden:NO];
        [panel setCanCreateDirectories: NO];

        UTType *type = [UTType typeWithFilenameExtension:ext];
        [panel setAllowedContentTypes:@[type]];

        //NSArray *file_types = [NSArray arrayWithObjects: ext,nil];
        //[panel setAllowedFileTypes:file_types];  

        [panel beginWithCompletionHandler:^(NSInteger result){
            std::string picked;

            if (result == NSModalResponseOK) {
                NSString *path = [[panel URL] path];
                picked = std::string([path UTF8String], [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
            } 
            callback(picked);
        }];
    }
    
    void platformPickSaveFile(std::string const& std_title, std::string const& std_message, std::string const& std_filename,
                              std::function<void(std::string const&)> callback) {
        NSString *folder_path = [NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) lastObject];

        NSString *message = [NSString stringWithCString:std_message.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString *title = [NSString stringWithCString:std_title.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString *filename = [NSString stringWithCString:std_filename.c_str() encoding:[NSString defaultCStringEncoding]];

        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setMessage: message]; 
        [panel setTitle: title];
        [panel setDirectoryURL:[NSURL fileURLWithPath:folder_path]];

        [panel setAllowsOtherFileTypes:NO];
        [panel setExtensionHidden:NO];
        [panel setCanCreateDirectories:YES];
        [panel setNameFieldStringValue:filename];

        [panel beginWithCompletionHandler:^(NSInteger result){
            std::string picked;

            if (result == NSModalResponseOK) {
                NSString *path = [[panel URL] path];
                picked = std::string([path UTF8String], [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
            } 
            callback(picked);
        }];
    }

    bool platformHasFileMenu() { return true; }
    void platformSetupFileMenu(Menu const& menu, MenuCallback callback) {
        NSMenu *menu_bar = [NSMenu new];
        [NSApp setMainMenu:menu_bar];

        for (auto const& column : menu) {
            NSMenuItem *ns_column = [menu_bar addItemWithTitle:@"" action:nil keyEquivalent:@""];

            NSString *column_title = [NSString stringWithCString:column.name.c_str() encoding:[NSString defaultCStringEncoding]];
            NSMenu *ns_column_menu = [[NSMenu alloc] initWithTitle:column_title];
            [menu_bar setSubmenu: ns_column_menu forItem:ns_column];
            for (auto const& item : column.items) {
                if (item.name == "-") {
                    [ns_column_menu addItem: [NSMenuItem separatorItem]];
                } else {
                    MacMenuItem *mac_item = [[MacMenuItem alloc] initWithOptions:item callback: callback];
                    [ns_column_menu addItem: mac_item];
                }
            }
        }
    }

    void platformSetupWindow(int min_w, int min_h) {
        NSWindow* window = [[NSApplication sharedApplication] mainWindow];

        [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        [window setTitlebarAppearsTransparent: true];
        window.backgroundColor = [NSColor colorWithRed:0.15f green:0.15f blue:0.15f alpha:1.];

        [window setMinSize: CGSize {float(min_w),  float(min_h)}];
    }

    void platformFullscreenChanged(bool on) {
        NSWindow* window = [[NSApplication sharedApplication] mainWindow];
        [NSMenu setMenuBarVisible:!on];
    }
}
