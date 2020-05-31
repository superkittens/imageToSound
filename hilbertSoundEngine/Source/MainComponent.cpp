/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : fft(fftOrder)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    portNumberLabel.setBounds(10, 18, 130, 25);
    addAndMakeVisible(portNumberLabel);
    
    portNumberField.setEditable(true, true, true);
    portNumberField.setBounds(140, 18, 50, 25);
    addAndMakeVisible(portNumberField);
    
    connectButton.setBounds(210, 18, 100, 25);
    addAndMakeVisible(connectButton);
    connectButton.onClick = [this]{connectButtonClicked();};
    
    connectionStatusLabel.setBounds(450, 18, 240, 25);
    updateConnectionStatusLabel();
    addAndMakeVisible(connectionStatusLabel);
    
    oscReceiver.addListener(this);
    
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    //bufferToFill.clearActiveBufferRegion();
    
    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        if (okToSwitchBuffer)
        {
            if (readBuffer == 0)
                readBuffer = 1;
            
            if (readBuffer == 1)
                readBuffer = 0;
            
            if (writeBuffer == 0)
                writeBuffer = 1;
            
            if (writeBuffer == 1)
                writeBuffer = 0;
            
            okToSwitchBuffer = false;
            okToWrite = true;
        }
        
        auto *channelData = bufferToFill.buffer->getWritePointer(0);
        for (auto i = 0; i < bufferToFill.numSamples; ++i)
        {
            if (trackHead > fftSize / 2)
                trackHead = 0;
            
            channelData[i] = X[readBuffer][trackHead++];
        }
        
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    g.setOpacity (1.0f);
    g.setColour (Colours::white);

    // You can add your drawing code here!
    if (okToWrite && firstSampleReceived)
    {
        auto height = getLocalBounds().getHeight();
        for (int i = 1; i < fftSize - 1; ++i)
        {
            g.drawLine(i, (height / 2) - X[readBuffer][i] * 1000.0, i + 1, (height / 2) - X[readBuffer][i + 1] * 1000.0);
        }
    }
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    
    Image img;
}

void MainComponent::oscMessageReceived(const OSCMessage &message)
{

}


void MainComponent::oscBundleReceived(const OSCBundle &bundle)
{
    if (okToWrite)
    {
        for (auto *element = bundle.begin(); element != bundle.end(); ++element)
        {
            if (element->isMessage())
            {
                auto &message = element->getMessage();
                if (message.getAddressPattern().toString() == "/spectrum")
                {
                    okToWrite = false;
                    std::fill(X[writeBuffer].begin(), X[writeBuffer].end(), 0.0);
                    
                    for (int i = 0; i < fftSize / 2; ++i)
                    {
                        X[writeBuffer][i * 2] = message[i].getFloat32();
                    }
                    
                    processIncomingSpectrum(X[writeBuffer].data());
                }
            }
        }
    }
}


void MainComponent::processIncomingSpectrum(float *X)
{
    fft.performRealOnlyInverseTransform(X);
    X[0] = X[fftSize - 1];
    okToSwitchBuffer = true;
    firstSampleReceived = true;
}


void MainComponent::timerCallback()
{
    repaint();
}


void MainComponent::connectButtonClicked()
{
    if (!isConnected())
        connectPort(portNumberField.getText().getIntValue());
    
    else
        disconnectPort();
    
    updateConnectionStatusLabel();
}


void MainComponent::connectPort(int portNumber)
{
    if (!isPortValid(portNumber))
    {
        handleInvalidPortNumber();
        return;
    }
    
    if (oscReceiver.connect(portNumber))
    {
        currentPortNumber = portNumber;
        connectButton.setButtonText("Disconnect");
    }
    
    else
        handleConnectError(portNumber);
}


void MainComponent::disconnectPort()
{
    if (oscReceiver.disconnect())
    {
        currentPortNumber = -1;
        connectButton.setButtonText("Connect");
    }
    
    else
        handleDisconnectError();
}


bool MainComponent::isPortValid(int portNumber)
{
    return portNumber > 0 && portNumber < 65536;
}


bool MainComponent::isConnected()
{
    return currentPortNumber != -1;
}


void MainComponent::updateConnectionStatusLabel()
{
    String text = "Status: ";
    if (isConnected())
        text += "Connected to UDP Port " + String(currentPortNumber);
    
    else
        text += "Disconnected";
    
    auto textColour = isConnected() ? Colours::green : Colours::red;
    
    connectionStatusLabel.setText(text, dontSendNotification);
    connectionStatusLabel.setFont(Font(15.0f, Font::bold));
    connectionStatusLabel.setColour(Label::textColourId, textColour);
    connectionStatusLabel.setJustificationType(Justification::centredRight);
}


void MainComponent::handleConnectError (int failedPort)
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "OSC Connection error",
                                      "Error: could not connect to port " + String (failedPort),
                                      "OK");
}


void MainComponent::handleInvalidPortNumber()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "Invalid port number",
                                      "Error: you have entered an invalid UDP port number.",
                                      "OK");
}


void MainComponent::handleDisconnectError()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                      "Unknown error",
                                      "An unknown error occured while trying to disconnect from UDP port.",
                                      "OK");
}
