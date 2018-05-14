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

#ifndef EXTERNALFILEPOOL_H_INCLUDED
#define EXTERNALFILEPOOL_H_INCLUDED

namespace hise { using namespace juce;

namespace MetadataIDs
{
const Identifier SampleRate("SampleRate");
const Identifier LoopEnabled("LoopEnabled");
const Identifier LoopStart("LoopStart");
const Identifier LoopEnd("LoopEnd");
}

struct PoolHelpers
{
	

	// Using an empty parameter to get the correct Subdirectory type
	static ProjectHandler::SubDirectories getSubDirectoryType(const AudioSampleBuffer& emptyData);
	static ProjectHandler::SubDirectories getSubDirectoryType(const Image& emptyImage);

	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, AudioSampleBuffer& data, var& additionalData);
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, Image& data, var& additionalData);

	static size_t getDataSize(const AudioSampleBuffer& buffer);

	static size_t getDataSize(const Image& img);

	static bool isValid(const AudioSampleBuffer& buffer);
	static bool isValid(const Image& buffer);

	static Image getEmptyImage(int width, int height);

	struct Reference
	{
		enum Mode
		{
			Invalid,
			AbsolutePath,
			ExpansionPath,
			ProjectPath,
			EmbeddedResource,
			numModes_
		};

		Reference();

		Reference(const MainController* mc, const String& referenceStringOrFile, ProjectHandler::SubDirectories directoryType);
		Reference(MemoryBlock& mb, const String& referenceString, ProjectHandler::SubDirectories directoryType);

		bool operator ==(const Reference& other) const;
		bool operator !=(const Reference& other) const;

		Mode getMode() const { return m; }
		String getReferenceString() const;
		Identifier getId() const;
		File getFile() const;
		bool isRelativeReference() const;
		bool isAbsoluteFile() const;
		bool isEmbeddedReference() const;;
		InputStream* createInputStream();
		int64 getHashCode() const;
		bool isValid() const;
		ProjectHandler::SubDirectories getFileType() const;

	private:

		void parseReferenceString(const MainController* mc, const String& input);

		String reference;
		File f;
		Identifier id;
		Mode m;

		int64 hashCode;

		void* memoryLocation;
		size_t memorySize;

		ProjectHandler::SubDirectories directoryType;
	};

};


class PoolCollection;

using PoolReference = PoolHelpers::Reference;



class PoolBase : public RestorableObject,
				 public ControlledObject
{
public:

	enum EventType
	{
		Added,
		Removed,
		Changed,
		numEventTypes
	};

	class Listener
	{
	public:

		virtual ~Listener() {};

		virtual void poolEntryAdded() = 0;
		virtual void poolEntryRemoved() = 0;
		virtual void poolEntryChanged(int indexInPool) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener)
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v(getFileTypeName());

		for (int i = 0; i < getNumLoadedFiles(); i++)
		{
			ValueTree child("PoolData");
			storeItemInValueTree(child, i);
			v.addChild(child, -1, nullptr);
		}

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		clearData();

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			ValueTree child = v.getChild(i);
			restoreItemFromValueTree(child);
		}

		notifyTable(Added);
	}

	void notifyTable(EventType t, NotificationType notify=sendNotificationAsync, int index=-1)
	{
		lastType = t;
		lastEventIndex = index;


		auto& tmp = listeners;

		auto f = [t, index, tmp]()
		{
			
		};

		if (notify == sendNotificationAsync)
			notifier.triggerAsyncUpdate();
		else
			notifier.handleAsyncUpdate();
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}


	virtual int getNumLoadedFiles() const = 0;
	virtual PoolReference getReference(int index) const = 0;
	virtual void clearData() = 0;
	virtual var getAdditionalData(PoolReference r) const = 0;
	virtual StringArray getTextDataForId(int index) const = 0;

protected:

	virtual void storeItemInValueTree(ValueTree& child, int index) const = 0;
	virtual void restoreItemFromValueTree(ValueTree& child) = 0;

	virtual Identifier getFileTypeName() const = 0;

	PoolBase(MainController* mc):
	  ControlledObject(mc),
	  notifier(*this)
	{

	}

private:

	struct Notifier: public AsyncUpdater
	{
		Notifier(PoolBase& parent_) :
			parent(parent_)
		{};

		~Notifier()
		{
			cancelPendingUpdate();
		}

		void handleAsyncUpdate() override
		{
			for (auto l : parent.listeners)
			{
				if (l != nullptr)
				{
					switch (parent.lastType)
					{
					case Added: l->poolEntryAdded(); break;
					case Removed: l->poolEntryRemoved(); break;
					case Changed: l->poolEntryChanged(parent.lastEventIndex); break;
					default:
						break;
					}
				}
			}
		}


		PoolBase& parent;

	};

	Notifier notifier;

	EventType lastType;
	int lastEventIndex = -1;

	Array<WeakReference<Listener>> listeners;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PoolBase)
};

template <class DataType> class PoolEntry : public ReferenceCountedObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<PoolEntry>;

	PoolEntry(PoolReference& r) :
		ref(r),
		data(DataType())
	{};

	PoolEntry() :
		ref(PoolReference()),
		data(DataType())
	{};

	bool operator ==(const PoolEntry& other) const
	{
		return other.ref == ref;
	}

	explicit operator bool() const { return ref.isValid() && PoolHelpers::isValid(data); };

	StringArray getTextData() const
	{
		StringArray sa;

		sa.add(ref.getReferenceString());
		auto dataSize = PoolHelpers::getDataSize(data);

		String s = String((float)dataSize / 1024.0f, 1) + " kB";

		sa.add(s);

		sa.add(String(getReferenceCount() - 1));

		return sa;
	}

	PoolReference ref;
	DataType data;
	var additionalData;
};

using PooledImage = PoolEntry<Image>::Ptr;
using PooledAudioFile = PoolEntry<AudioSampleBuffer>::Ptr;

template <class DataType> class SharedPoolBase : public PoolBase
{
public:

    

	SharedPoolBase(MainController* mc_) :
		PoolBase(mc_),
		type(PoolHelpers::getSubDirectoryType(DataType()))
	{
		if (type == FileHandlerBase::SubDirectories::AudioFiles)
		{
			afm.registerBasicFormats();
			afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
		}

	};


	~SharedPoolBase() {};

	Identifier getFileTypeName() const override
	{
		return ProjectHandler::getIdentifier(type);
	}

	int getNumLoadedFiles() const override
	{
		return pool.size();
	}

	void clearData() override
	{
		pool.clear();
		notifyTable(Removed);
	}

	bool contains(int64 hashCode) const
	{
		for (auto d : pool)
		{
			if (d->ref.getHashCode() == hashCode)
				return true;
		}

		return false;
	}

	PoolReference getReference(int index) const override
	{
		if (auto entry = pool[index])
			return entry.get()->ref;

		return PoolReference();
	}

	var getAdditionalData(PoolReference r) const override
	{
		for (auto d : pool)
		{
			if (d->ref == r)
				return d->additionalData;
		}

		return {};
	}

	StringArray getTextDataForId(int index) const override
	{
		if (auto entry = pool[index])
			return entry.get()->getTextData();

		return {};
	}

	StringArray getIdList() const
	{
		StringArray sa;

		for (auto d : pool)
			sa.add(d->ref.getReferenceString());

		return sa;
	}

	PoolEntry<DataType>* loadFromReference(PoolReference r)
	{
		for (auto d : pool)
		{
			if (d->ref == r)
			{
				notifyTable(PoolBase::Changed, sendNotificationAsync, pool.indexOf(d));
				return d;
			}
				
		}

		ReferenceCountedObjectPtr<PoolEntry<DataType>> ne = new PoolEntry<DataType>(r);
		auto inputStream = r.createInputStream();

		PoolHelpers::loadData(afm, inputStream, r.getHashCode(), ne->data, ne->additionalData);

		pool.add(ne);

		notifyTable(PoolBase::Added);

		return ne;
	}

private:

	ReferenceCountedArray<PoolEntry<DataType>> pool;

	void storeItemInValueTree(ValueTree& child, int index) const
	{
		if (auto entry = pool[index].get())
		{
			child.setProperty("ID", entry->ref.getReferenceString(), nullptr);

			ScopedPointer<InputStream> inputStream = entry->ref.createInputStream();

			MemoryBlock mb = MemoryBlock();
			MemoryOutputStream out(mb, false);
			out.writeFromInputStream(*inputStream, inputStream->getTotalLength());

			child.setProperty("Data", var(mb.getData(), mb.getSize()), nullptr);
		}
	}

	void restoreItemFromValueTree(ValueTree& child)
	{
		String r = child.getProperty("ID", String()).toString();

		var x = child.getProperty("Data", var::undefined());
		auto mb = x.getBinaryData();

		jassert(mb != nullptr);

		PoolReference ref(*mb, r, type);

		loadFromReference(ref);
	}

	ProjectHandler::SubDirectories type;

	AudioFormatManager afm;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SharedPoolBase<DataType>);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedPoolBase)
};

using AudioSampleBufferPool = SharedPoolBase<AudioSampleBuffer>;
using ImagePool = SharedPoolBase<Image>;

class PoolCollection: public ControlledObject
{
public:

	PoolCollection(MainController* mc):
		ControlledObject(mc)
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			switch ((ProjectHandler::SubDirectories)i)
			{
			case ProjectHandler::SubDirectories::AudioFiles:
				dataPools[i] = new AudioSampleBufferPool(mc);
				break;
			case ProjectHandler::SubDirectories::Images:
				dataPools[i] = new ImagePool(mc);
				break;
			default:
				dataPools[i] = nullptr;
			}
		}
	};

	~PoolCollection()
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			if (dataPools[i] != nullptr)
			{
				delete dataPools[i];
				dataPools[i] = nullptr;
			}
		}
	}

	void clear()
	{
		for (int i = 0; i < (int)ProjectHandler::SubDirectories::numSubDirectories; i++)
		{
			if (dataPools[i] != nullptr)
			{
				dataPools[i]->clearData();
			}
		}
	}

	template<class DataType> SharedPoolBase<DataType>* getPool()
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		jassert(dataPools[type] != nullptr);

		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}

	template<class DataType> const SharedPoolBase<DataType>* getPool() const
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		jassert(dataPools[type] != nullptr);

		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}


	const AudioSampleBufferPool& getAudioSampleBufferPool() const
	{
		return *getPool<AudioSampleBuffer>();
	}

	AudioSampleBufferPool& getAudioSampleBufferPool()
	{
		return *getPool<AudioSampleBuffer>();
	}

	const ImagePool& getImagePool() const
	{
		return *getPool<Image>();
	}

	ImagePool& getImagePool()
	{
		return *getPool<Image>();
	}

	AudioFormatManager afm;

private:

	PoolBase * dataPools[(int)ProjectHandler::SubDirectories::numSubDirectories];

	JUCE_DECLARE_WEAK_REFERENCEABLE(PoolCollection);
};



} // namespace hise

#endif  // EXTERNALFILEPOOL_H_INCLUDED
