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

#pragma once

namespace hise {
using namespace juce;

struct MarkdownParser::DefaultLinkResolver : public LinkResolver
{
	DefaultLinkResolver(MarkdownParser* parser_);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("DefaultLinkResolver"); }
	String getContent(const MarkdownLink& url) override { return {}; }

	ResolveType getPriority() const override { return ResolveType::Fallback; }

	bool linkWasClicked(const MarkdownLink& url) override;
	LinkResolver* clone(MarkdownParser* newParser) const override;

	MarkdownParser* parser;
};

struct MarkdownParser::FileLinkResolver : public LinkResolver
{
	FileLinkResolver(const File& root_);;

	String getContent(const MarkdownLink& url) override;
	bool linkWasClicked(const MarkdownLink& url) override;
	ResolveType getPriority() const override { return ResolveType::FileBased; }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileLinkResolver"); };
	LinkResolver* clone(MarkdownParser* p) const override { return new FileLinkResolver(root); }

	File root;
};


class MarkdownParser::FolderTocCreator : public LinkResolver
{
public:

	FolderTocCreator(const File& rootFile_);

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FolderTocCreator"); };

	ResolveType getPriority() const override { return ResolveType::FileBased; }

	String getContent(const MarkdownLink& url) override;
	LinkResolver* clone(MarkdownParser* parent) const override;

	File rootFile;
};

class MarkdownParser::GlobalPathProvider : public ImageProvider
{
public:

	struct GlobalPool
	{
		OwnedArray<PathFactory> factories;
	};

	constexpr static char path_wildcard[] = "/images/icon_";

	GlobalPathProvider(MarkdownParser* parent);;

	ResolveType getPriority() const override { return ResolveType::EmbeddedPath; };

	ImageProvider* clone(MarkdownParser* newParser) const override { return new GlobalPathProvider(newParser); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("GlobalPathProvider"); };

	Image getImage(const MarkdownLink& urlName, float width) override;

	template <class T> void registerFactory()
	{
		factories->factories.add(new T());
		factories->factories.getLast()->createPath("");
	}

	SharedResourcePointer<GlobalPool> factories;

};



class MarkdownParser::URLImageProvider : public ImageProvider
{
public:

	URLImageProvider(File tempdirectory_, MarkdownParser* parent);;

	Image getImage(const MarkdownLink& urlName, float width) override;

	ResolveType getPriority() const override { return ResolveType::WebBased; }

	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("URLImageProvider"); };
	ImageProvider* clone(MarkdownParser* newParser) const { return new URLImageProvider(tempDirectory, newParser); }

	File tempDirectory;
};

class MarkdownParser::FileBasedImageProvider : public ImageProvider
{
public:

	static Image createImageFromSvg(Drawable* drawable, float width)
	{
		if (drawable != nullptr)
		{
			float maxWidth = jmax(10.0f, width);
			float height = drawable->getOutlineAsPath().getBounds().getAspectRatio(false) * maxWidth;

			Image img(Image::PixelFormat::ARGB, (int)maxWidth, (int)height, true);
			Graphics g(img);
			drawable->drawWithin(g, { 0.0f, 0.0f, maxWidth, height }, RectanglePlacement::centred, 1.0f);

			return img;
		}

		return {};
	}

	FileBasedImageProvider(MarkdownParser* parent, const File& root);;

	Image getImage(const MarkdownLink& imageURL, float width) override;

	ResolveType getPriority() const override { return ResolveType::FileBased; }

	ImageProvider* clone(MarkdownParser* newParent) const override { return new FileBasedImageProvider(newParent, r); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("FileBasedImageProvider"); }

	File r;
};

template <class FactoryType> class PathProvider : public MarkdownParser::ImageProvider
{
public:

	PathProvider(MarkdownParser* parent) :
		ImageProvider(parent)
	{};

	virtual Image getImage(const MarkdownLink& imageURL, float width) override
	{
		Path p = f.createPath(imageURL.toString(MarkdownLink::UrlFull));

		if (!p.isEmpty())
		{
			auto b = p.getBounds();
			auto r = (float)b.getWidth() / (float)b.getHeight();
			p.scaleToFit(0.0f, 0.0f, floorf(width), floorf(width / r), true);

			Image img(Image::PixelFormat::ARGB, (int)p.getBounds().getWidth(), (int)p.getBounds().getHeight(), true);
			Graphics g(img);
			g.setColour(parent->getStyleData().textColour);
			g.fillPath(p);

			return img;
		}
		else
			return {};
	}

	MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::Autogenerated; }

	ImageProvider* clone(MarkdownParser* newParent) const override { return new PathProvider<FactoryType>(newParent); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("PathProvider"); }

private:

	FactoryType f;
};

}