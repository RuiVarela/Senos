#include "Platform.hpp"
#include "../engine/core/Log.hpp"
#include "../engine/core/Text.hpp"

#include <pwd.h>
#include <unistd.h>

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UTType.h>


#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>


//
// Menu
//
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


//
// Sleep
//
namespace sns {

    class SleepDetector {
    public:
        SleepDetector(std::vector<PlatformEventCallback>* cbs)
            :root_port(0), notify_port_ref(nullptr), notifier_object(0), callbacks(cbs)
        {
        }

        virtual ~SleepDetector() {
            stop();
        }

        bool start() {
            if (!root_port) {
                root_port = IORegisterForSystemPower(this, &notify_port_ref, callback_static, &notifier_object);
                if (!root_port)
                    return false;
                CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notify_port_ref), kCFRunLoopCommonModes);
            }
            return true;
        }

        void stop() {
            if (root_port) {
                // remove the sleep notification port from the application runloop
                CFRunLoopRemoveSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notify_port_ref), kCFRunLoopCommonModes);

                // deregister for system sleep notifications
                IODeregisterForSystemPower(&notifier_object);

                // IORegisterForSystemPower implicitly opens the Root Power Domain IOService so we close it here
                IOServiceClose(root_port);

                // destroy the notification port allocated by IORegisterForSystemPower
                IONotificationPortDestroy(notify_port_ref);

                // reset object members
                root_port = 0;
                notify_port_ref = nullptr;
                notifier_object = 0;
            }
        }

        void notifySleep() {
            for (auto const& cb: *callbacks) 
                cb(PlatformEvent::Sleep);
        }

        void notifyWakeup() {
            for (auto const& cb: *callbacks) 
                cb(PlatformEvent::Wakeup);
        }

    private:
        static void callback_static(void* arg, io_service_t service, natural_t message_type, void *message_argument) {
            SleepDetector* self = (SleepDetector*)arg;
            self->callback(service, message_type, message_argument);
        }

        void callback(io_service_t service, natural_t message_type, void *message_argument) {
            switch (message_type) {
            case kIOMessageCanSystemSleep:
                IOAllowPowerChange(root_port, (long)message_argument);
                break;
            case kIOMessageSystemWillSleep:
                notifySleep();
                IOAllowPowerChange(root_port, (long)message_argument);
                break;
            case kIOMessageSystemHasPoweredOn:
                notifyWakeup();
                break;
            }
        }

        std::vector<PlatformEventCallback>* callbacks;

        io_connect_t root_port;                 // a reference to the Root Power Domain IOService
        IONotificationPortRef notify_port_ref;  // notification port allocated by IORegisterForSystemPower
        io_object_t notifier_object;            // notifier object, used to deregister later
    };


    //
    // Singleton
    //
    struct PlatformSingleton {
        std::vector<PlatformEventCallback> callbacks;
        SleepDetector sleep_detector;

        PlatformSingleton()
            :sleep_detector(&callbacks) {

        }
    };

    static PlatformSingleton& platform() {
        static PlatformSingleton singleton;
        return singleton;
    }

	
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


	void platformRegisterCallback(PlatformEventCallback callback) {
        PlatformSingleton& p = platform();
        p.callbacks.push_back(callback);

        if (p.callbacks.size() == 1) {
            p.sleep_detector.start();
        }
    }

	void platformClearCallbacks() {
        PlatformSingleton& p = platform();

        p.callbacks.clear();
        p.sleep_detector.stop();
    }
}
