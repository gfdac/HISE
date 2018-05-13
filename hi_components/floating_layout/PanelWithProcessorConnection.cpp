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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

PanelWithProcessorConnection::PanelWithProcessorConnection(FloatingTile* parent) :
	FloatingTileContent(parent),
	showConnectionBar("showConnectionBar")
{
	addAndMakeVisible(connectionSelector = new ComboBox());
	connectionSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*connectionSelector);

	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::transparentBlack);
	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::transparentBlack);
	connectionSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	connectionSelector->setTextWhenNothingSelected("Disconnected");

	addAndMakeVisible(indexSelector = new ComboBox());
	indexSelector->addListener(this);
	getMainSynthChain()->getMainController()->skin(*indexSelector);

	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colours::transparentBlack);
	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colours::transparentBlack);
	indexSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::transparentBlack);
	indexSelector->setTextWhenNothingSelected("Disconnected");

	BACKEND_ONLY(getMainController()->getProcessorChangeHandler().addProcessorChangeListener(this);)
}

PanelWithProcessorConnection::~PanelWithProcessorConnection()
{
	content = nullptr;

#if USE_BACKEND
	getMainController()->getProcessorChangeHandler().removeProcessorChangeListener(this);
#endif
}

void PanelWithProcessorConnection::paint(Graphics& g)
{
	auto bounds = getParentShell()->getContentBounds();

	const bool scb = getStyleProperty(showConnectionBar, true) && findParentComponentOfClass<ScriptContentComponent>() == nullptr;

    
    
	if (scb)
	{
		const bool connected = getProcessor() != nullptr && (!hasSubIndex() || currentIndex != -1);

		//g.setColour(Colour(0xFF3D3D3D));
		//g.fillRect(0, bounds.getY(), getWidth(), 18);

		g.setColour(connected ? getProcessor()->getColour() : Colours::white.withAlpha(0.1f));

		Path p;
		p.loadPathFromData(ColumnIcons::connectionIcon, sizeof(ColumnIcons::connectionIcon));
		p.scaleToFit(2.0, (float)bounds.getY() + 2.0f, 14.0f, 14.0f, true);
		g.fillPath(p);
	}
	
}

var PanelWithProcessorConnection::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	storePropertyInObject(obj, SpecialPanelIds::ProcessorId, getConnectedProcessor() != nullptr ? getConnectedProcessor()->getId() : "");
	storePropertyInObject(obj, SpecialPanelIds::Index, currentIndex);
	storePropertyInObject(obj, SpecialPanelIds::Index, currentIndex);
	
	return obj;
}

void PanelWithProcessorConnection::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	const String id = getPropertyWithDefault(object, SpecialPanelIds::ProcessorId);

	int index = getPropertyWithDefault(object, SpecialPanelIds::Index);

	if (id.isNotEmpty())
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(getParentShell()->getMainController()->getMainSynthChain(), id);

		if (p != nullptr)
		{
			setContentWithUndo(p, index);
		}
	}
}

int PanelWithProcessorConnection::getNumDefaultableProperties() const
{
	return SpecialPanelIds::numSpecialPanelIds;
}

Identifier PanelWithProcessorConnection::getDefaultablePropertyId(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultablePropertyId(index);

	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::ProcessorId, "ProcessorId");
	RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::Index, "Index");

	jassertfalse;
	return{};
}

var PanelWithProcessorConnection::getDefaultProperty(int index) const
{
	if (index < (int)PanelPropertyId::numPropertyIds)
		return FloatingTileContent::getDefaultProperty(index);

	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::ProcessorId, var(""));
	RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::Index, var(-1));

	jassertfalse;
	return{};
}

void PanelWithProcessorConnection::incIndex(bool up)
{
	int newIndex = currentIndex;

	if (up)
	{
		newIndex = jmin<int>(currentIndex + 1, indexSelector->getNumItems()-1);
	}
	else
	{
		newIndex = jmax<int>(newIndex - 1, 0);
	}

	setContentWithUndo(currentProcessor, newIndex);
}

void PanelWithProcessorConnection::moduleListChanged(Processor* b, MainController::ProcessorChangeHandler::EventType type)
{
	if (type == MainController::ProcessorChangeHandler::EventType::ProcessorBypassed ||
		type == MainController::ProcessorChangeHandler::EventType::ProcessorColourChange)
		return;

	if (type == MainController::ProcessorChangeHandler::EventType::ProcessorRenamed)
	{
		if (getConnectedProcessor() == b || getConnectedProcessor() == nullptr)
		{
			int tempId = connectionSelector->getSelectedId();

			refreshConnectionList();

			connectionSelector->setSelectedId(tempId, dontSendNotification);
		}
	}
	else
	{
		refreshConnectionList();
	}

	
}

void PanelWithProcessorConnection::resized()
{
	const bool isInScriptContent = findParentComponentOfClass<ScriptContentComponent>() != nullptr;

	if (isInScriptContent)
	{
		connectionSelector->setVisible(false);
		indexSelector->setVisible(false);

		if (content != nullptr)
		{
			content->setVisible(true);
			content->setBounds(getLocalBounds());
		}
			
	}
	else
	{
		if (!listInitialised)
		{
			// Do this here the first time to avoid pure virtual function call...
			refreshConnectionList();
			listInitialised = true;
		}

		auto bounds = getParentShell()->getContentBounds();

		if (bounds.isEmpty())
			return;

		const bool scb = getStyleProperty(showConnectionBar, true);

		Rectangle<int> contentArea = getParentShell()->getContentBounds();

		if (scb)
		{
			connectionSelector->setVisible(!getParentShell()->isFolded());
			connectionSelector->setBounds(18, bounds.getY(), 128, 18);

			indexSelector->setVisible(!getParentShell()->isFolded() && hasSubIndex());
			indexSelector->setBounds(connectionSelector->getRight() + 5, bounds.getY(), 128, 18);

			contentArea = contentArea.withTrimmedTop(18);
		}
		else
		{
			connectionSelector->setVisible(false);
		}

		if (content != nullptr)
		{
			if (getHeight() > 18)
			{
				content->setVisible(true);
				content->setBounds(contentArea);
			}
			else
				content->setVisible(false);
		}
	}
}

void PanelWithProcessorConnection::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged == connectionSelector)
	{
		indexSelector->clear(dontSendNotification);
		setConnectionIndex(-1);

		if (connectionSelector->getSelectedId() == 1)
		{
			setCurrentProcessor(nullptr);
			refreshContent();
		}
		else
		{
			const String id = comboBoxThatHasChanged->getText();

			auto p = ProcessorHelpers::getFirstProcessorWithName(getMainSynthChain(), id);

			connectedProcessor = p;

			if (hasSubIndex())
			{
				refreshIndexList();
				setContentWithUndo(p, 0);
			}
			else
			{
				setConnectionIndex(-1);
				setContentWithUndo(p, -1);
			}
		}
	}
	else if (comboBoxThatHasChanged == indexSelector)
	{
		int newIndex = -1;

		if (indexSelector->getSelectedId() != 1)
		{
			newIndex = indexSelector->getSelectedId() - 2;
			setContentWithUndo(connectedProcessor.get(), newIndex);
		}
		else
		{
			setConnectionIndex(newIndex);
			refreshContent();
		}
	}
}

void PanelWithProcessorConnection::refreshConnectionList()
{
	String currentId = connectionSelector->getText();

	connectionSelector->clear(dontSendNotification);

	StringArray items;

	fillModuleList(items);

	int index = items.indexOf(currentId);

	connectionSelector->addItem("Disconnect", 1);
	connectionSelector->addItemList(items, 2);

	if (index != -1)
	{
		connectionSelector->setSelectedId(index + 2, dontSendNotification);
	}
}

void PanelWithProcessorConnection::refreshIndexList()
{
	String currentId = indexSelector->getText();

	indexSelector->clear(dontSendNotification);

	StringArray items;

	fillIndexList(items);

	int index = items.indexOf(currentId);

	indexSelector->addItem("Disconnect", 1);
	indexSelector->addItemList(items, 2);

	if (index != -1)
		indexSelector->setSelectedId(index + 2, dontSendNotification);
}

ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain()
{
	return getMainController()->getMainSynthChain();
}

const ModulatorSynthChain* PanelWithProcessorConnection::getMainSynthChain() const
{
	return getMainController()->getMainSynthChain();
}

void PanelWithProcessorConnection::setContentWithUndo(Processor* newProcessor, int newIndex)
{
	StringArray indexes;
	fillIndexList(indexes);

	refreshIndexList();

#if USE_BACKEND
	auto undoManager = dynamic_cast<BackendProcessor*>(getMainController())->getViewUndoManager();

	String undoText;

	
	undoText << (currentProcessor.get() != nullptr ? currentProcessor->getId() : "Disconnected") << ": " << indexes[currentIndex] << " -> ";
	undoText << (newProcessor != nullptr ? newProcessor->getId() : "Disconnected") << ": " << indexes[newIndex] << " -> ";

	undoManager->beginNewTransaction(undoText);
	undoManager->perform(new ProcessorConnection(this, newProcessor, newIndex, getAdditionalUndoInformation()));
#else

	ScopedPointer<ProcessorConnection> connection = new ProcessorConnection(this, newProcessor, newIndex, getAdditionalUndoInformation());

	connection->perform();

	connection = nullptr;

#endif

	if (newIndex != -1)
	{
		indexSelector->setSelectedId(newIndex + 2, dontSendNotification);
	}

}

PanelWithProcessorConnection::ProcessorConnection::ProcessorConnection(PanelWithProcessorConnection* panel_, Processor* newProcessor_, int newIndex_, var additionalInfo_) :
	panel(panel_),
	newProcessor(newProcessor_),
	newIndex(newIndex_),
	additionalInfo(additionalInfo_)
{
	oldIndex = panel->currentIndex;
	oldProcessor = panel->currentProcessor.get();
}

bool PanelWithProcessorConnection::ProcessorConnection::perform()
{
	if (panel.getComponent() != nullptr)
	{
		panel->currentIndex = newIndex;
		panel->setCurrentProcessor(newProcessor.get());
		panel->refreshContent();
		
		return true;
	}

	return false;
}

bool PanelWithProcessorConnection::ProcessorConnection::undo()
{
	if (panel.getComponent())
	{
		panel->currentIndex = oldIndex;
		panel->setCurrentProcessor(oldProcessor.get());
		panel->refreshContent();
		panel->performAdditionalUndoInformation(additionalInfo);
		return true;
	}

	return false;
}

void PanelWithProcessorConnection::setContentForIdentifier(Identifier idToSearch)
    {
        auto parentContainer = getParentShell()->getParentContainer();
        
        if (parentContainer != nullptr)
        {
            FloatingTile::Iterator<PanelWithProcessorConnection> iter(parentContainer->getParentShell());
            
            while (auto p = iter.getNextPanel())
            {
                if (p == this)
                    continue;
                
                if (p->getProcessorTypeId() != idToSearch)
                    continue;
                
                p->setContentWithUndo(getProcessor(), 0);
            }
        }
    }
    
} // namespace hise
