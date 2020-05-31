/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : Thread("Background Thread"), fft(fftOrder), w(512,dsp::WindowingFunction<float>::WindowingMethod::hann)
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
    
    formatManager.registerBasicFormats();
    
    addAndMakeVisible(openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this]{ openButtonClicked(); };
    
    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this]{ clearButtonClicked(); };
    
    startThread();
    startTimerHz(30);
    
    img = Image(Image::RGB, 256, 256, true);
    
    hc = createHilbertCurve(8);
    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    stopThread(4000);
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
    
    ReferenceCountedBuffer::Ptr retainedCurrentBuffer(currentBuffer);
    
    if (retainedCurrentBuffer == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    auto *currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    auto position = retainedCurrentBuffer->position;
    auto numInputChannels = currentAudioSampleBuffer->getNumChannels();
    auto numOutputChannels = bufferToFill.buffer->getNumChannels();
    
    auto outputSamplesRemaining = bufferToFill.numSamples;
    auto outputSamplesOffset = 0;
    
    while (outputSamplesRemaining > 0)
    {
        auto bufferSamplesRemaining = currentAudioSampleBuffer->getNumSamples() - position;
        auto samplesThisTime = jmin(bufferSamplesRemaining, outputSamplesRemaining);
        
        for (auto channel = 0; channel < numOutputChannels; ++channel)
        {
            bufferToFill.buffer->copyFrom(channel, bufferToFill.startSample + outputSamplesOffset, *currentAudioSampleBuffer, channel % numInputChannels, position, samplesThisTime);
        }
        
        outputSamplesRemaining -= samplesThisTime;
        outputSamplesOffset += samplesThisTime;
        position += samplesThisTime;
        
        if (position == currentAudioSampleBuffer->getNumSamples())
            position = 0;
    }
    
    retainedCurrentBuffer->position = position;
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
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    if (!okToMeasureSpectrum)
    {
        // You can add your drawing code here!
        for (auto i = 0; i < hc.size(); ++i)
        {
            auto leftScaler = x_left[i];
            auto rightScaler = x_right[i];
            img.setPixelAt(hc[i].x, hc[i].y, Colour(255 * leftScaler, 0, 255 * rightScaler));
        }
        
        g.drawImage(img, 50, 100, 300, 300, 0, 0, 256, 256);
        
        okToMeasureSpectrum = true;
    }
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    openButton.setBounds(10, 10, getWidth() - 20, 20);
    clearButton.setBounds(10, 40, getWidth() - 20, 20);
}

void MainComponent::run()
{
    while (!threadShouldExit())
    {
        checkForPathToOpen();
        checkForBuffersToFree();
        wait(500);
    }
}


void MainComponent::checkForBuffersToFree()
{
    for (auto i = buffers.size(); --i >= 0; )
    {
        ReferenceCountedBuffer::Ptr buffer(buffers.getUnchecked(i));
        if (buffer->getReferenceCount() == 2)
            buffers.remove(i);
    }
}


void MainComponent::timerCallback()
{
    if (okToMeasureSpectrum)
        getSpectrum();
    
    repaint();
}


void MainComponent::openButtonClicked()
{
    FileChooser chooser("Select File to Play", {}, "*.wav;*.aif");
    
    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        auto path = file.getFullPathName();
        chosenPath.swapWith(path);
        notify();
    }
}


void MainComponent::clearButtonClicked()
{
    currentBuffer = nullptr;
}


void MainComponent::checkForPathToOpen()
{
    String pathToOpen;
    pathToOpen.swapWith(chosenPath);
    
    if (pathToOpen.isNotEmpty())
    {
        File file(pathToOpen);
        std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
        
        if (reader.get() != nullptr)
        {
            ReferenceCountedBuffer::Ptr newBuffer = new ReferenceCountedBuffer(file.getFileName(), reader->numChannels, (int)reader->lengthInSamples);
            
            reader->read(newBuffer->getAudioSampleBuffer(), 0, (int)reader->lengthInSamples, 0, true, true);
            currentBuffer = newBuffer;
            buffers.add(newBuffer);
        }
    }
}


void MainComponent::getSpectrum()
{
    okToMeasureSpectrum = false;
    ReferenceCountedBuffer::Ptr retainedCurrentBuffer(currentBuffer);
    
    if (retainedCurrentBuffer == nullptr)
    {
        okToMeasureSpectrum = true;
        return;
    }
    
    std::fill(x_left.begin(), x_left.end(), 0.0);
    std::fill(x_right.begin(), x_right.end(), 0.0);
    
    auto *currentAudioSampleBuffer = retainedCurrentBuffer->getAudioSampleBuffer();
    auto lastPosition = retainedCurrentBuffer->position;
    auto samplesToCopy = 512;
    
    if (samplesToCopy >= fftSize / 2)
        return;
    
    auto samplesRemainingInBuffer = currentAudioSampleBuffer->getNumSamples() - lastPosition;
    
    while(samplesToCopy > 0)
    {
        auto numSamples = jmin(samplesRemainingInBuffer, samplesToCopy);
        for (auto i = 0; i < numSamples; ++i)
        {
            x_left[i] = *currentAudioSampleBuffer->getReadPointer(0, lastPosition + i);
            x_right[i] = *currentAudioSampleBuffer->getReadPointer(1, lastPosition + i);
        }
        
        samplesToCopy -= numSamples;
        lastPosition += numSamples;
        
        if (lastPosition == currentAudioSampleBuffer->getNumSamples())
            lastPosition = 0;
    }
    
    w.multiplyWithWindowingTable(x_left.data(), 512);
    w.multiplyWithWindowingTable(x_right.data(), 512);
    
    fft.performFrequencyOnlyForwardTransform(x_left.data());
    fft.performFrequencyOnlyForwardTransform(x_right.data());
    auto leftMax = FloatVectorOperations::findMaximum(x_left.data(), fftSize / 2) / 2;
    auto rightMax = FloatVectorOperations::findMaximum(x_right.data(), fftSize / 2) / 2;
    
    for (auto i = 0; i < fftSize; ++i)
    {
        x_left[i] /= leftMax;
        x_right[i] /= rightMax;
    }
    
}


std::vector<MainComponent::mVec2f> MainComponent::hilbertCurveHelper(size_t currentM, size_t m, std::vector<mVec2f> &A)
{
    if (currentM > m) return A;
    
    size_t numPoints = A.size();
    float N = powf(2, currentM);
    float NPrev = powf(2, currentM - 1);
    
    for (int i = 0; i < numPoints; ++i)
        A.push_back(mVec2f(A[i].x, A[i].y + (N / 2)));
    
    for (int i = 0; i < numPoints; ++i)
        A.push_back(mVec2f(A[i].x + (N / 2), A[i].y + (N / 2)));
    
    for (int i = 0; i < numPoints; ++i)
    {
        float x = A[i].x;
        float y = A[i].y;
        A.push_back(mVec2f((N / 2) + (NPrev - 1) - y, (NPrev - 1) - x));
    }
    
    for (int i = 0; i < numPoints; ++i)
    {
        float x = A[i].x;
        float y = A[i].y;
        A[i].x = y;
        A[i].y = x;
    }
    
    hilbertCurveHelper(currentM + 1, m, A);
    
    return A;
}


std::vector<MainComponent::mVec2f> MainComponent::createHilbertCurve(size_t m)
{
    std::vector<MainComponent::mVec2f> A;
    if (m < 1) return A;
    
    //  Base case m = 1
    A.push_back(mVec2f(0, 0));
    A.push_back(mVec2f(0, 1));
    A.push_back(mVec2f(1, 1));
    A.push_back(mVec2f(1, 0));
    
    return hilbertCurveHelper(2, m, A);
}
