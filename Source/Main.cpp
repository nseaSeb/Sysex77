/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MidiDemo.h"
#include "Tests.h"
inline File getExamplesDirectory() noexcept
{
#ifdef PIP_JUCE_EXAMPLES_DIRECTORY
    MemoryOutputStream mo;
    
    auto success = Base64::convertFromBase64 (mo, JUCE_STRINGIFY (PIP_JUCE_EXAMPLES_DIRECTORY));
    ignoreUnused (success);
    jassert (success);
    
    return mo.toString();
#else
    auto currentFile = File::getSpecialLocation (File::SpecialLocationType::currentApplicationFile);
    auto exampleDir = currentFile.getParentDirectory().getChildFile ("examples");
    
    if (exampleDir.exists())
        return exampleDir;
    
    int numTries = 0; // keep track of the number of parent directories so we don't go on endlessly
    
    while (currentFile.getFileName() != "examples" && numTries++ < 15)
        currentFile = currentFile.getParentDirectory();
            Logger::writeToLog(currentFile.getFullPathName());
        return currentFile;
#endif
}
inline InputStream* createAssetInputStream (const char* resourcePath)
{
#if JUCE_ANDROID
    ZipFile apkZip (File::getSpecialLocation (File::invokedExecutableFile));
    return apkZip.createStreamForEntry (apkZip.getIndexOfFileName ("assets/" + String (resourcePath)));
#else
#if JUCE_IOS
    auto assetsDir = File::getSpecialLocation (File::currentExecutableFile)
    .getParentDirectory().getChildFile ("Assets");
#elif JUCE_MAC
    auto assetsDir = File::getSpecialLocation (File::currentExecutableFile)
    .getParentDirectory().getParentDirectory().getChildFile ("Resources").getChildFile ("Assets");
    
    if (! assetsDir.exists())
        assetsDir = getExamplesDirectory().getChildFile ("Assets");
#else
    auto assetsDir = getExamplesDirectory().getChildFile ("Assets");
#endif
    
    auto resourceFile = assetsDir.getChildFile (resourcePath);
//    jassert (resourceFile.existsAsFile()); // Bug que je ne comprends pas pour le moment
    Logger::writeToLog(String(resourceFile.getFullPathName()));
    return resourceFile.createInputStream().release();
#endif
}


inline Image getImageFromAssets (const char* assetName)
{
    auto hashCode = (String (assetName) + "@juce_demo_assets").hashCode64();
    auto img = ImageCache::getFromHashCode (hashCode);
    
    if (img.isNull())
    {
        std::unique_ptr<InputStream> juceIconStream (createAssetInputStream (assetName));
        
        if (juceIconStream == nullptr)
            return {};
        
        img = ImageFileFormat::loadFrom (*juceIconStream);
        
        ImageCache::addImageToCache (img, hashCode);
    }
    
    return img;
}
//==============================================================================
#if JUCE_WINDOWS || JUCE_LINUX || JUCE_MAC
// Just add a simple icon to the Window system tray area or Mac menu bar..
struct DemoTaskbarComponent  : public SystemTrayIconComponent,
private Timer
{
    DemoTaskbarComponent()
    {
        setIconImage (getImageFromAssets ("juce_icon.png"),
                      getImageFromAssets ("juce_icon.png"));
        setIconTooltip ("JUCE demo runner!");
    }
    
    void mouseDown (const MouseEvent&) override
    {
        // On OSX, there can be problems launching a menu when we're not the foreground
        // process, so just in case, we'll first make our process active, and then use a
        // timer to wait a moment before opening our menu, which gives the OS some time to
        // get its act together and bring our windows to the front.
        
        Process::makeForegroundProcess();
        startTimer (50);
    }
    
    // This is invoked when the menu is clicked or dismissed
    static void menuInvocationCallback (int chosenItemID, DemoTaskbarComponent*)
    {
        if (chosenItemID == 1)
            JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
    void timerCallback() override
    {
        stopTimer();
        
        PopupMenu m;
        m.addItem (1, "Quit");
        
        // It's always better to open menus asynchronously when possible.
        m.showMenuAsync (PopupMenu::Options(),
                         ModalCallbackFunction::forComponent (menuInvocationCallback, this));
    }
};
#endif


class Application    : public JUCEApplication
{
public:
    //==============================================================================
    Application()
    {
        // Langue de l'UI : préférence "Language" (réglable dans Settings) ; à défaut, langue système.
        // "fr" charge French.txt, "en" garde les clés source anglaises. Cf. syApplyLanguage (Config.h).
        Logger::writeToLog( SystemStats::getUserLanguage());
        const String defaultLang = SystemStats::getUserLanguage().startsWith ("fr") ? "fr" : "en";
        syApplyLanguage (getAppSettings()->getValue ("Language", defaultLang));
    }

    const String getApplicationName() override       { return "Sysex77"; }
    const String getApplicationVersion() override    { return Sysex77::kVersion; }

    void initialise (const String& commandLine) override
    {
        if (commandLine.contains ("--test"))
        {
            runUnitTestsAndQuit();
            return;
        }

        mainWindow.reset (new MainWindow ("Sysex77 v" + String (Sysex77::kVersion), new MidiDemo(), *this));


        auto splash = new SplashScreen ("SYSEX77",ImageFileFormat::loadFrom(BinaryData::SY77_YAMAHA_png, (size_t) BinaryData::SY77_YAMAHA_pngSize),true);

        splash->deleteAfterDelay (RelativeTime::seconds (4), true);

    }

    void runUnitTestsAndQuit()
    {
        UnitTestRunner runner;
        runner.setAssertOnFailure (false);
        runner.runAllTests();

        int failures = 0, total = 0;
        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            auto* r = runner.getResult (i);
            failures += r->failures;
            total    += r->passes + r->failures;
        }

        std::printf ("\n=== UnitTests: %d/%d assertions passed, %d failure(s) ===\n",
                     total - failures, total, failures);
        std::fflush (stdout);
        setApplicationReturnValue (failures > 0 ? 1 : 0);
        quit();
    }
    void myInitialisationWorkFinished()
    {

    }
    void shutdown() override                         {
        
        mainWindow = nullptr; }

private:
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (const String& name, Component* c, JUCEApplication& a)
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons),
              app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);
        
           #if JUCE_ANDROID || JUCE_IOS
            setFullScreen (true);
           #else
            setResizable (true, false);
            setResizeLimits (640, 480, 10000, 10000);
            // Restaure la taille/position mémorisées, sinon centre à la taille par défaut.
            const auto state = getAppSettings()->getValue ("WindowState");
            if (state.isNotEmpty())
                restoreWindowStateFromString (state);
            else
                centreWithSize (getWidth(), getHeight());
           #endif
#if JUCE_WINDOWS || JUCE_LINUX || JUCE_MAC
            taskbarIcon.reset (new DemoTaskbarComponent());
#endif
            setVisible (true);
        }

        ~MainWindow() override
        {
            // Mémorise la taille/position de la fenêtre (toutes voies de fermeture).
            getAppSettings()->setValue ("WindowState", getWindowStateAsString());
            getAppSettings()->saveIfNeeded();
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();

        }

    private:
        JUCEApplication& app;
        std::unique_ptr<Component> taskbarIcon;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (Application)
namespace BinaryData_TXT
{
    
}

