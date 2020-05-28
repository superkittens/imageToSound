/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent,
                        private OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
    OSCReceiver oscReceiver;
    void connectPort(int portNumber);
    void disconnectPort();
    bool isPortValid(int portNumber);
    bool isConnected();
    void connectButtonClicked();
    
    void oscMessageReceived(const OSCMessage &message) override;
    void oscBundleReceived(const OSCBundle &bundle) override;
    
    void handleConnectError(int failedPort);
    void handleInvalidPortNumber();
    void handleDisconnectError();
    void updateConnectionStatusLabel();
    
    void processIncomingSpectrum(float *X);
    
    dsp::FFT fft;
    constexpr static auto fftOrder = 11;
    constexpr static auto fftSize = 1 << fftOrder;
    
    bool okToWrite = true;
    bool okToSwitchBuffer = false;
    std::array<std::array<float, fftSize * 2>, 2> X;
    size_t readBuffer = 0;
    size_t writeBuffer = 1;
    int trackHead = 0;
    
    Label connectionStatusLabel;
    Label portNumberLabel {{}, "UDP Port Number: "};
    Label portNumberField {{}, "13050"};
    TextButton connectButton {"Connect"};
    int currentPortNumber = -1;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
