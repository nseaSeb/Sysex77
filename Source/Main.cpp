/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiDemo.h"
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
            Logger::writeToLog(String(currentFile));
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
    return resourceFile.createInputStream();
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
        setIconImage (getImageFromAssets ("juce_icon.png"));
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
        
        Logger::writeToLog( SystemStats::getUserLanguage());
        if(SystemStats::getUserLanguage()=="fr")
        {
        File languageFile("../../../../Ressources/French.txt");
        Logger::writeToLog(languageFile.getFullPathName());
        if (languageFile.exists())
        {
            juce::LocalisedStrings *currentMappings = new juce::LocalisedStrings(languageFile, true);
            juce::LocalisedStrings::setCurrentMappings(currentMappings);
        }
        }
        
    }

    const String getApplicationName() override       { return "MidiDemo"; }
    const String getApplicationVersion() override    { return "1.0.0"; }

    void initialise (const String&) override         {
        mainWindow.reset (new MainWindow ("MidiDemo", new MidiDemo(), *this));
        
 
        auto splash = new SplashScreen ("SYSEX77",ImageFileFormat::loadFrom(BinaryData::SY77_YAMAHA_png, (size_t) BinaryData::SY77_YAMAHA_pngSize),true);

        splash->deleteAfterDelay (RelativeTime::seconds (4), true);

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
            centreWithSize (getWidth(), getHeight());
           #endif
#if JUCE_WINDOWS || JUCE_LINUX || JUCE_MAC
            taskbarIcon.reset (new DemoTaskbarComponent());
#endif
            setVisible (true);
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

