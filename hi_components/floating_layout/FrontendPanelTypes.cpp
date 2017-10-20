/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/** Type-ID: `TypeID`

Description

![%TYPE% Screenshot](http://hise.audio/images/floating_tile_gifs/%TYPE%.gif)

### Used base properties

| ID | Description |
| --- | --- |
`ColourData::textColour`  | the text colour
`ColourData::bgColour`    | the background colour
`ColourData::itemColour1` | the first item colour
`Font`					  | the font
`FontSize`				  | the font size

### Example JSON

```
const var data = {%EXAMPLE_JSON};
```

*/



ActivityLedPanel::ActivityLedPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setOpaque(true);

	startTimer(100);
}

var ActivityLedPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, (int)SpecialPanelIds::OffImage, offName);
	storePropertyInObject(obj, (int)SpecialPanelIds::OnImage, onName);
	storePropertyInObject(obj, (int)SpecialPanelIds::ShowMidiLabel, showMidiLabel);

	return obj;
}

void ActivityLedPanel::timerCallback()
{
	const bool midiFlag = getMainController()->checkAndResetMidiInputFlag();

	setOn(midiFlag);
}

void ActivityLedPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	showMidiLabel = getPropertyWithDefault(object, (int)SpecialPanelIds::ShowMidiLabel);

	onName = getPropertyWithDefault(object, (int)SpecialPanelIds::OnImage);

	if (onName.isNotEmpty())
		on = ImagePool::loadImageFromReference(getMainController(), onName);

	offName = getPropertyWithDefault(object, (int)SpecialPanelIds::OffImage);

	if (offName.isNotEmpty())
		off = ImagePool::loadImageFromReference(getMainController(), offName);
}



Identifier ActivityLedPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OffImage, "OffImage");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::OnImage, "OnImage");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ShowMidiLabel, "ShowMidiLabel");

	jassertfalse;
	return{};
}

var ActivityLedPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OffImage, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::OnImage, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ShowMidiLabel, true);

	jassertfalse;
	return{};
}

void ActivityLedPanel::paint(Graphics &g)
{
	g.fillAll(Colours::black);
	g.setColour(Colours::white);

	g.setFont(getFont());

	if (showMidiLabel)
		g.drawText("MIDI", 0, 0, 100, getHeight(), Justification::centredLeft, false);

	g.drawImageWithin(isOn ? on : off, showMidiLabel ? 38 : 2, 2, 24, getHeight(), RectanglePlacement::centred);
}

void ActivityLedPanel::setOn(bool shouldBeOn)
{
	isOn = shouldBeOn;
	repaint();
}


MidiKeyboardPanel::MidiKeyboardPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::transparentBlack);

	setInterceptsMouseClicks(false, true);

	addAndMakeVisible(keyboard = new CustomKeyboard(parent->getMainController()));

	keyboard->setLowestVisibleKey(12);
}

MidiKeyboardPanel::~MidiKeyboardPanel()
{
	keyboard = nullptr;
}

bool MidiKeyboardPanel::showTitleInPresentationMode() const
{
	return false;
}

CustomKeyboard* MidiKeyboardPanel::getKeyboard() const
{
	return keyboard;
}

int MidiKeyboardPanel::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numProperyIds;
}

var MidiKeyboardPanel::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::KeyWidth, keyboard->getKeyWidth());



	storePropertyInObject(obj, SpecialPanelIds::DisplayOctaveNumber, keyboard->isShowingOctaveNumbers());
	storePropertyInObject(obj, SpecialPanelIds::LowKey, keyboard->getRangeStart());
	storePropertyInObject(obj, SpecialPanelIds::HiKey, keyboard->getRangeEnd());
	storePropertyInObject(obj, SpecialPanelIds::CustomGraphics, keyboard->isUsingCustomGraphics());
	storePropertyInObject(obj, SpecialPanelIds::DefaultAppearance, defaultAppearance);
	storePropertyInObject(obj, SpecialPanelIds::BlackKeyRatio, keyboard->getBlackNoteLengthProportion());
	storePropertyInObject(obj, SpecialPanelIds::ToggleMode, keyboard->isToggleModeEnabled());

	return obj;
}

void MidiKeyboardPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	keyboard->setUseCustomGraphics(getPropertyWithDefault(object, SpecialPanelIds::CustomGraphics));

	keyboard->setRange(getPropertyWithDefault(object, SpecialPanelIds::LowKey),
		getPropertyWithDefault(object, SpecialPanelIds::HiKey));

	keyboard->setKeyWidth(getPropertyWithDefault(object, SpecialPanelIds::KeyWidth));

	defaultAppearance = getPropertyWithDefault(object, SpecialPanelIds::DefaultAppearance);

	keyboard->setShowOctaveNumber(getPropertyWithDefault(object, SpecialPanelIds::DisplayOctaveNumber));

	keyboard->setBlackNoteLengthProportion(getPropertyWithDefault(object, SpecialPanelIds::BlackKeyRatio));

	keyboard->setEnableToggleMode(getPropertyWithDefault(object, SpecialPanelIds::ToggleMode));
}

Identifier MidiKeyboardPanel::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::CustomGraphics, "CustomGraphics");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::KeyWidth, "KeyWidth");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::LowKey, "LowKey");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::HiKey, "HiKey");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::BlackKeyRatio, "BlackKeyRatio");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DefaultAppearance, "DefaultAppearance");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::DisplayOctaveNumber, "DisplayOctaveNumber");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ToggleMode, "ToggleMode");

	jassertfalse;
	return{};
}

var MidiKeyboardPanel::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::CustomGraphics, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::KeyWidth, 14);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::LowKey, 9);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::HiKey, 127);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::BlackKeyRatio, 0.7);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DefaultAppearance, true);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::DisplayOctaveNumber, false);
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ToggleMode, false);

	jassertfalse;
	return{};
}

void MidiKeyboardPanel::paint(Graphics& g)
{
	g.setColour(findPanelColour(PanelColourId::bgColour));
	g.fillAll();
}

void MidiKeyboardPanel::resized()
{
	if (defaultAppearance)
	{
		int width = jmin<int>(getWidth(), CONTAINER_WIDTH);

		keyboard->setBounds((getWidth() - width) / 2, 0, width, 72);
	}
	else
	{
		keyboard->setBounds(0, 0, getWidth(), getHeight());
	}
}

int MidiKeyboardPanel::getFixedHeight() const
{
	return defaultAppearance ? 72 : FloatingTileContent::getFixedHeight();
}


Note::Note(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(editor = new TextEditor());
	editor->setFont(GLOBAL_BOLD_FONT());
	editor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
	editor->setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));
	editor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.5f));
	editor->setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor->addListener(this);
	editor->setReturnKeyStartsNewLine(true);
	editor->setMultiLine(true, true);

	editor->setLookAndFeel(&plaf);
}

void Note::resized()
{
	editor->setBounds(getLocalBounds().withTrimmedTop(16));
}


var Note::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::Text, editor->getText(), String());

	return obj;
}

void Note::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	editor->setText(getPropertyWithDefault(object, SpecialPanelIds::Text));
}

int Note::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numSpecialPanelIds;
}

Identifier Note::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Text, "Text");

	jassertfalse;
	return{};
}

var Note::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Text, var(""));

	jassertfalse;
	return{};
}

void Note::labelTextChanged(Label*)
{

}

int Note::getFixedHeight() const
{
	return 150;
}


PerformanceLabelPanel::PerformanceLabelPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(statisticLabel = new Label());
	statisticLabel->setEditable(false, false);
	statisticLabel->setColour(Label::ColourIds::textColourId, Colours::white);

	setDefaultPanelColour(PanelColourId::textColour, Colours::white);

	statisticLabel->setFont(GLOBAL_BOLD_FONT());

	startTimer(200);
}

void PerformanceLabelPanel::timerCallback()
{
	auto mc = getMainController();

	const int cpuUsage = (int)mc->getCpuUsage();
	const int voiceAmount = mc->getNumActiveVoices();
	const double ramUsage = (double)mc->getSampleManager().getModulatorSamplerSoundPool()->getMemoryUsageForAllSamples() / 1024.0 / 1024.0;

	//const bool midiFlag = mc->checkAndResetMidiInputFlag();

	//activityLed->setOn(midiFlag);

	String stats = "CPU: ";
	stats << String(cpuUsage) << "%, RAM: " << String(ramUsage, 1) << "MB , Voices: " << String(voiceAmount);
	statisticLabel->setText(stats, dontSendNotification);
}



void PerformanceLabelPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	statisticLabel->setColour(Label::ColourIds::textColourId, findPanelColour(PanelColourId::textColour));
}

void PerformanceLabelPanel::resized()
{
	statisticLabel->setBounds(getLocalBounds());
}

bool PerformanceLabelPanel::showTitleInPresentationMode() const
{
	return false;
}

TooltipPanel::TooltipPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colour(0xFF383838));
	setDefaultPanelColour(PanelColourId::itemColour1, Colours::white.withAlpha(0.2f));
	setDefaultPanelColour(PanelColourId::textColour, Colours::white.withAlpha(0.8f));

	addAndMakeVisible(tooltipBar = new TooltipBar());;
}

TooltipPanel::~TooltipPanel()
{
	tooltipBar = nullptr;
}

int TooltipPanel::getFixedHeight() const
{
	return 30;
}

bool TooltipPanel::showTitleInPresentationMode() const
{
	return false;
}

void TooltipPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	tooltipBar->setColour(TooltipBar::backgroundColour, findPanelColour(PanelColourId::bgColour));
	tooltipBar->setColour(TooltipBar::iconColour, findPanelColour(PanelColourId::itemColour1));
	tooltipBar->setColour(TooltipBar::textColour, findPanelColour(PanelColourId::textColour));

	tooltipBar->setFont(getFont());
}

void TooltipPanel::resized()
{
	tooltipBar->setBounds(getLocalBounds());
}

PresetBrowserPanel::PresetBrowserPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, Colours::black.withAlpha(0.97f));
	setDefaultPanelColour(PanelColourId::itemColour1, Colour(SIGNAL_COLOUR));

	addAndMakeVisible(presetBrowser = new MultiColumnPresetBrowser(getMainController()));
}

PresetBrowserPanel::~PresetBrowserPanel()
{
	presetBrowser = nullptr;
}

void PresetBrowserPanel::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	presetBrowser->setHighlightColourAndFont(findPanelColour(PanelColourId::itemColour1), findPanelColour(PanelColourId::bgColour), getFont());
}

bool PresetBrowserPanel::showTitleInPresentationMode() const
{
	return false;
}

void PresetBrowserPanel::resized()
{
	presetBrowser->setBounds(getLocalBounds());
	presetBrowser->setHighlightColourAndFont(findPanelColour(PanelColourId::itemColour1), findPanelColour(PanelColourId::bgColour), getFont());
}