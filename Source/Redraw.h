//
//  Redraw.h
//  MidiDemo
//
//  Created by Sébastien Portrait on 11/11/2018.
//  Copyright © 2018 JUCE. All rights reserved.
//

#ifndef Redraw_h
#define Redraw_h
void paint (Graphics& g) override {
    // fichier image pour le fond
    File knobImageFile1 = File::getSpecialLocation
    (File::SpecialLocationType::userDesktopDirectory).getChildFile("SY77.png");
    imgBack = ImageCache::getFromFile(knobImageFile1);
    
    g.drawImageAt(imgBack,getLocalBounds().getWidth() - imgBack.getWidth() - 20, 8);
    
}

void resized() override
{
    auto margin = 10;
    auto row = getBounds().removeFromTop(margeBt);
    row.setY(2);
    row.setX(2);
    for (auto* b : radioButtons)
        b->setBounds (row.removeFromLeft (100));
//    row= getBounds().removeFromTop(getHeight()-margeBt);
    
    // row vaut désormais la zone d'affichage des elements
    row.setBounds( 0, 50, getWidth(), getHeight()-(margeKey+margeBt));
    
    midiInputLabel.setBounds (row.getX()+ marge ,row.getY() + margin,
                              (row.getWidth() / 2) - (2 * margin), 24);
    
    midiOutputLabel.setBounds ((row.getWidth() / 2) + margin,row.getY()+ margin,
                               (row.getWidth() / 2) - (2 * margin), 24);
    
    midiInputSelector->setBounds (row.getX()+ margin, row.getY()+(2 * margin) + 24,
                                  (row.getWidth() / 2) - (2 * margin)+50,
                                  (row.getHeight() / 2) - ((4 * margin) + 24 + 24));
    
    midiOutputSelector->setBounds ((row.getWidth() / 2) + margin, row.getY()+(2 * margin) + 24,
                                   (row.getWidth() / 2) - (2 * margin),
                                   (row.getHeight() / 2) - ((4 * margin) + 24 + 24));
    
    pairButton.setBounds (row.getX()+ margin, (row.getHeight() / 2) - (margin + 24),
                          row.getWidth() - (2 * margin), 24);
    
    incomingMidiLabel.setBounds (row.getX()+ margin, row.getHeight() / 2 + margin, row.getWidth() - (2 * margin), 24);
    midiMonitor.setBounds (row.getX()+ margin, (row.getHeight() / 2) +  margin + 24,
                           row.getWidth() - (2 * margin), row.getHeight()/2 - margin);
    
    
    
    //   auto y = (getHeight() / 2) + ((3 * 24) + (4 * margin) + 64);
    midiKeyboard.setBounds (margin, getHeight()-100, getWidth() - (2 * margin), margeKey - 20);
    ledMidiIn.setBounds(getWidth()-16, 8, 12, 12);
    ledMidiOut.setBounds(getWidth()-16, 26, 12, 12);
    
// Session Controller

    ctrSwitchLabel.setBounds (row.getX()+ marge ,row.getY() + margin,
                              (row.getWidth() / 2) - (2 * margin), 24);
    ctrModLabel.setBounds (row.getX()+ marge ,row.getHeight()/2 +24+margin,
                              (row.getWidth() / 2) - (2 * margin), 24);
    tableSwitch.setBounds (row.getX()+ marge ,row.getY() + margin+ 24,
                           (row.getWidth() / 2) - (2 * margin), row.getHeight()/2 -48);
    tableMod.setBounds (row.getX()+ marge ,row.getHeight()/2 + margin+ 48,
                     (row.getWidth() / 2) - (2 * margin), row.getHeight()/2-48);
}

#endif /* Redraw_h */
